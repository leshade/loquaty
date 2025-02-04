
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 実行コンテキスト
//////////////////////////////////////////////////////////////////////////////

#define	VERIFY_INSTRUCTION(name)	\
	assert( s_instruction[LCodeBuffer::code##name] == &LContext::instruction_##name )

LContext::LContext( LVirtualMachine& vm, size_t nInitSize )
	: m_vm( vm ), m_pCodeBuf( nullptr ),
		m_stack( std::make_shared<LStackBuffer>(nInitSize) ),
		m_ip( LCodeBuffer::InvalidCodePos ),
		m_eximm( LValue::MakeVoidPtr(nullptr) ),
		m_interrupt( false ), m_throwing( false ),
		m_jumped( 0 ), m_signal( interruptNo )
{
	m_nullThrown = new LExceptionObj( vm.GetExceptionClass(), L"null" ) ;

	VERIFY_INSTRUCTION( NoOperation ) ;
	VERIFY_INSTRUCTION( EnterFunc ) ;
	VERIFY_INSTRUCTION( LeaveFunc ) ;
	VERIFY_INSTRUCTION( EnterTry ) ;
	VERIFY_INSTRUCTION( CallFinally ) ;
	VERIFY_INSTRUCTION( RetFinally ) ;
	VERIFY_INSTRUCTION( LeaveFinally ) ;
	VERIFY_INSTRUCTION( LeaveTry ) ;
	VERIFY_INSTRUCTION( Throw ) ;
	VERIFY_INSTRUCTION( AllocStack ) ;
	VERIFY_INSTRUCTION( FreeStack ) ;
	VERIFY_INSTRUCTION( FreeStackLocal ) ;
	VERIFY_INSTRUCTION( FreeLocalObj ) ;
	VERIFY_INSTRUCTION( PushObjChain ) ;
	VERIFY_INSTRUCTION( MakeObjChain ) ;
	VERIFY_INSTRUCTION( Synchronize ) ;
	VERIFY_INSTRUCTION( Return ) ;
	VERIFY_INSTRUCTION( SetRetValue ) ;
	VERIFY_INSTRUCTION( MoveAP ) ;
	VERIFY_INSTRUCTION( Call ) ;
	VERIFY_INSTRUCTION( CallVirtual ) ;
	VERIFY_INSTRUCTION( CallIndirect ) ;
	VERIFY_INSTRUCTION( ExImmPrefix ) ;
	VERIFY_INSTRUCTION( ClearException ) ;
	VERIFY_INSTRUCTION( LoadRetValue ) ;
	VERIFY_INSTRUCTION( LoadException ) ;
	VERIFY_INSTRUCTION( LoadArg ) ;
	VERIFY_INSTRUCTION( LoadImm ) ;
	VERIFY_INSTRUCTION( LoadLocal ) ;
	VERIFY_INSTRUCTION( LoadLocalOffset ) ;
	VERIFY_INSTRUCTION( LoadStack ) ;
	VERIFY_INSTRUCTION( LoadFetchAddr ) ;
	VERIFY_INSTRUCTION( LoadFetchAddrOffset ) ;
	VERIFY_INSTRUCTION( LoadFetchLAddr ) ;
	VERIFY_INSTRUCTION( CheckLPtrAlign ) ;
	VERIFY_INSTRUCTION( CheckPtrAlign ) ;
	VERIFY_INSTRUCTION( LoadLPtr ) ;
	VERIFY_INSTRUCTION( LoadLPtrOffset ) ;
	VERIFY_INSTRUCTION( LoadByPtr ) ;
	VERIFY_INSTRUCTION( LoadByLAddr ) ;
	VERIFY_INSTRUCTION( LoadByAddr ) ;
	VERIFY_INSTRUCTION( LoadLocalPtr ) ;
	VERIFY_INSTRUCTION( LoadObjectPtr ) ;
	VERIFY_INSTRUCTION( LoadLObjectPtr ) ;
	VERIFY_INSTRUCTION( LoadObject ) ;
	VERIFY_INSTRUCTION( NewObject ) ;
	VERIFY_INSTRUCTION( AllocBuf ) ;
	VERIFY_INSTRUCTION( RefObject ) ;
	VERIFY_INSTRUCTION( FreeObject ) ;
	VERIFY_INSTRUCTION( Move ) ;
	VERIFY_INSTRUCTION( StoreLocalImm ) ;
	VERIFY_INSTRUCTION( StoreLocal ) ;
	VERIFY_INSTRUCTION( ExchangeLocal ) ;
	VERIFY_INSTRUCTION( StoreByPtr ) ;
	VERIFY_INSTRUCTION( StoreByLPtr ) ;
	VERIFY_INSTRUCTION( StoreByLAddr ) ;
	VERIFY_INSTRUCTION( StoreByAddr ) ;
	VERIFY_INSTRUCTION( BinaryOperate ) ;
	VERIFY_INSTRUCTION( UnaryOperate ) ;
	VERIFY_INSTRUCTION( CastObject ) ;
	VERIFY_INSTRUCTION( Jump ) ;
	VERIFY_INSTRUCTION( JumpConditional ) ;
	VERIFY_INSTRUCTION( JumpNonConditional ) ;
	VERIFY_INSTRUCTION( GetElement ) ;
	VERIFY_INSTRUCTION( GetElementAs ) ;
	VERIFY_INSTRUCTION( GetElementName ) ;
	VERIFY_INSTRUCTION( SetElement ) ;
	VERIFY_INSTRUCTION( SetElementAs ) ;
	VERIFY_INSTRUCTION( ObjectToInt ) ;
	VERIFY_INSTRUCTION( ObjectToFloat ) ;
	VERIFY_INSTRUCTION( ObjectToString ) ;
	VERIFY_INSTRUCTION( IntToObject ) ;
	VERIFY_INSTRUCTION( FloatToObject ) ;
	VERIFY_INSTRUCTION( IntToFloat ) ;
	VERIFY_INSTRUCTION( UintToFloat ) ;
	VERIFY_INSTRUCTION( FloatToInt ) ;
	VERIFY_INSTRUCTION( FloatToUint ) ;
}

#undef	VERIFY_INSTRUCTION

LContext::~LContext( void )
{
	if ( GetCurrent() == this )
	{
		SetCurrent( nullptr ) ;
	}
}


#if !defined(_DLL_IMPORT_LOQUATY)

static thread_local LContext *	t_pCurrent = nullptr ;

// 現在のスレッドに設定された LContext 取得
LOQUATY_DLL_DECL(LContext *) LContext::GetCurrent( void )
{
	return	t_pCurrent ;
}

// スレッドへ設定
LOQUATY_DLL_DECL(void) LContext::SetCurrent( void )
{
	t_pCurrent = this ;
}

LOQUATY_DLL_DECL(void) LContext::SetCurrent( LContext * pCurrent )
{
	t_pCurrent = pCurrent ;
}

#endif

// 関数実行
//////////////////////////////////////////////////////////////////////////////

// 関数実行（同期）
std::tuple<LValue,LObjPtr>
	LContext::SyncCallFunction
		( LFunctionObj * pFunc,
			const LValue * pArgValues, size_t nArgCount )
{
	AsyncState	state ;
	if ( !AsyncCallFunction( state, pFunc, pArgValues, nArgCount ) )
	{
		while ( !AsyncProceed( state, 10 ) )
		{
		}
	}
	return	std::make_tuple<LValue,LObjPtr>
					( LValue(state.valRet), LObjPtr(state.exception) ) ;
}

// 関数実行開始（非同期）
// （true が返された場合、関数は完了した）
bool LContext::AsyncCallFunction
		( AsyncState& state,
			LFunctionObj * pFunc,
			const LValue * pArgValues, size_t nArgCount )
{
	auto	lock = LockRunning() ;
	Current	current( *this ) ;

	// 実行中の状態保存
	state.finished = false ;
	SaveContextState( state.ctxSave ) ;

	assert( pFunc != nullptr ) ;
	if ( pFunc == nullptr )
	{
		state.finished = true ;
		state.valRet = LValue() ;
		state.exception = new_Exception(exceptionNullPointer) ;
		return	true ;
	}
	else if ( !pFunc->IsNativeFunc()
			&& (pFunc->GetCodeBuffer() == nullptr) )
	{
		state.finished = true ;
		state.valRet = LValue() ;
		state.exception = new_Exception(exceptionUnimplemented) ;
		return	true ;
	}

	// 状態を初期化
	ResetState( statusRunning ) ;

	// 引数をプッシュ
	const size_t	nArgPushed =
		PushArgument( pFunc, pArgValues, nArgCount, nullptr ) ;

	// 関数実行
	if ( pFunc->IsNativeFunc() )
	{
		// ネイティブ関数
		pFunc->CallNativeFunc( *this ) ;

		if ( !m_interrupt && !IsRunningStatus( m_status ) )
		{
			FinishFunctionResult( state ) ;
			return	true ;
		}
		return	false ;
	}
	else
	{
		// スクリプト関数
		assert( pFunc->GetCodeBuffer() != nullptr ) ;
		micro_instruction_Call( pFunc ) ;

		return	false ;
	}
}

// 実行を継続する（非同期）
bool LContext::AsyncProceed( AsyncState& state, std::int64_t msecTimeout )
{
	auto	lock = LockRunning() ;
	Current	current( *this ) ;

	if ( !IsRunningStatus( AsyncRun( msecTimeout ) ) )
	{
		FinishFunctionResult( state ) ;
		return	true ;
	}
	return	false ;
}

// 関数割り込み実行設定
void LContext::InterruptFunction
	( LFunctionObj * pFunc,
		const LValue * pArgValues, size_t nArgCount,
			std::function<void(LContext&,LValue&)> funcReturned )
{
	auto	lock = LockRunning() ;
	assert( IsRunningStatus( m_status ) ) ;

	// 状態を保存
	InterruptSave	save ;
	save.ap = m_stack->m_ap ;
	save.fp = m_stack->m_fp ;
	save.valReturn = m_valReturn ;

	// 一時的な関数オブジェクトをプッシュして解放チェーンに追加する
	LPtr<LFunctionObj>	pFuncGate = new LFunctionObj( m_vm.GetFunctionClass() ) ;
	LPtr<LFunctionObj>	pFuncAfter = new LFunctionObj( m_vm.GetFunctionClass() ) ;

	m_stack->Push( LValue::MakeObjectPtr( pFuncGate.Get() ) ) ;
	m_stack->Push( LValue::MakeLong( m_stack->m_yp ) ) ;
	m_stack->m_yp = m_stack->m_sp - 2 ;

	m_stack->Push( LValue::MakeObjectPtr( pFuncAfter.Get() ) ) ;
	m_stack->Push( LValue::MakeLong( m_stack->m_yp ) ) ;
	m_stack->m_yp = m_stack->m_sp - 2 ;

	// 戻りアドレスをプッシュ
	m_stack->Push( LValue::MakeLong( m_ip ) ) ;
	m_stack->Push( LValue::MakeVoidPtr( m_pCodeBuf ) ) ;

	// 関数の引数をプッシュする
	size_t	nArgPushed = PushArgument( pFunc, pArgValues, nArgCount, nullptr ) ;

	// ゲート用コード
	LCodeBuffer::ImmediateOperand	immopNull ;
	immopNull.value.longValue = 0 ;

	LCodeBuffer::ImmediateOperand	immopFuncAfter ;
	immopFuncAfter.pFunc = pFuncAfter.Ptr() ;

	LCodeBuffer::Word	words[] =
	{
		// pFunc 関数の引数をスタック上から掃除する
		{ LCodeBuffer::codeFreeStack, 0, 0, 0,
								(std::int32_t) nArgPushed, immopNull },

		// 後始末関数を呼び出す
		{ LCodeBuffer::codeCall, 0, 0, 0, 0, immopFuncAfter },

		// 元の位置に戻った後に一時的な関数を破棄する
		{ LCodeBuffer::codeReturn, 0, 0, 0, 4, immopNull },
	} ;

	// ゲート関数コード生成
	std::shared_ptr<LCodeBuffer>
			pGateCodeBuf = std::make_shared<LCodeBuffer>() ;
	for ( size_t i = 0; i < sizeof(words)/sizeof(words[0]); i ++ )
	{
		pGateCodeBuf->m_buffer.push_back( words[i] ) ;
	}
	pFuncGate->SetFuncCode( pGateCodeBuf, 0 ) ;

	// 後始末関数
	pFuncAfter->SetNative( [save,funcReturned]( LContext& context )
	{
		LValue	valRet = save.valReturn ;
		if ( funcReturned != nullptr )
		{
			funcReturned( context, valRet ) ;
		}
		context.m_stack->m_ap = save.ap ;
		context.SetReturnValue( valRet ) ;
	} ) ;

	// ゲートへの復帰アドレスをプッシュする
	m_stack->Push( LValue::MakeLong( 0 ) ) ;
	m_stack->Push( LValue::MakeVoidPtr( pGateCodeBuf.get() ) ) ;

	// 割り込み関数アドレスへジャンプ
	m_pCodeBuf = pFunc->GetCodeBuffer().get() ;
	m_ip = pFunc->GetCodePoint() ;
	m_interrupt = true ;
}

// 引数をプッシュ
size_t LContext::PushArgument
	( LFunctionObj * pFunc,
		const LValue * pArgValues, size_t nArgCount,
		std::vector<LObjPtr>* pCastTempObjs )
{
	LPrototype *	pProto = pFunc->GetPrototype().get() ;
	assert( pProto != nullptr ) ;

	size_t	nArgPushed = 0 ;
	m_stack->m_ap = m_stack->m_sp ;

	LClass *	pThisClass = pProto->GetThisClass() ;
	size_t		iArgFirst = 0 ;
	if ( pThisClass != nullptr )
	{
		// this オブジェクト
		assert( nArgCount >= 1 ) ;
		if ( nArgCount >= 1 )
		{
			if ( pCastTempObjs != nullptr )
			{
				// 一応キャストする
				LObjPtr	pTempObj = pArgValues[0].GetObject()->CastClassTo( pThisClass ) ;
				pCastTempObjs->push_back( pTempObj ) ;
				LValue	valArg( pTempObj ) ;
				m_stack->Push( valArg.Value() ) ;
			}
			else
			{
				m_stack->Push( pArgValues[0].Value() ) ;
			}
			nArgPushed ++ ;
		}
		iArgFirst = 1 ;
	}

	for ( size_t i = 0; iArgFirst + i < nArgCount; i ++ )
	{
		LValue	valArg = pArgValues[iArgFirst + i] ;

		bool	flagObject = false ;
		if ( (pProto != nullptr)
			&& (i < pProto->GetArgListType().size())
			&& (pCastTempObjs != nullptr) )
		{
			LType	typeArg = pProto->GetArgListType().GetArgTypeAt(i) ;
			if ( typeArg.IsObject() && !valArg.IsNull() )
			{
				// オブジェクトの場合、一応キャストしておく
				LObjPtr	pTempObj = valArg.GetObject()->CastClassTo( typeArg.GetClass() ) ;
				pCastTempObjs->push_back( pTempObj ) ;
				valArg = LValue( pTempObj ) ;
			}
		}

		m_stack->Push( valArg.Value() ) ;
		nArgPushed ++ ;
	}

	for ( size_t i = nArgCount; i < iArgFirst + pProto->GetDefaultArgList().size(); i ++ )
	{
		// デフォルトの引数
		LValue	valDef = pProto->GetDefaultArgAt(i - iArgFirst).Clone() ;
		m_stack->Push( valDef.Value() ) ;
		nArgPushed ++ ;
	}

	// キャプチャー・オブジェクト
	for ( size_t i = 0; i < pFunc->GetCaptureObjectCount(); i ++ )
	{
		LValue	valCapture = pFunc->GetCaptureObjectAt( i ) ;
		m_stack->Push( valCapture.Value() ) ;
//		nArgPushed ++ ;		// ※キャプチャー引数のスタックは呼び出された関数の
							//   return で解放するのでカウントしない
	}

	return	nArgPushed ;
}

// 実行状態をリセット
void LContext::ResetState( LContext::ExecutionStatus status )
{
	m_stack->m_xp = LStackBuffer::InvalidPtr ;
	m_stack->m_yp = LStackBuffer::InvalidPtr ;

	m_ip = LCodeBuffer::InvalidCodePos ;
	m_pCodeBuf = nullptr ;

	m_thrown = nullptr ;
	m_exception = nullptr ;

	m_status = status ;
	m_awaiting = nullptr ;
	m_signal = interruptNo ;
}

// 実行状態を保存
void LContext::SaveContextState( LContext::ContextState& state ) const
{
	state.sp = m_stack->m_sp ;
	state.ap = m_stack->m_ap ;
	state.fp = m_stack->m_fp ;
	state.xp = m_stack->m_xp ;
	state.yp = m_stack->m_yp ;
	state.ip = m_ip ;
	state.valReturn = m_valReturn ;
	state.thrown = m_thrown ;
	state.exception = m_exception ;
	state.status = m_status ;
	state.pCodeBuf = m_pCodeBuf ;
	state.awaiting = m_awaiting ;
}

// 実行状態を復元
void LContext::RestoreContextState( const LContext::ContextState& state )
{
	const bool	flagAborted = (m_status == statusAborted) ;

	m_stack->m_sp = state.sp ;
	m_stack->m_ap = state.ap ;
	m_stack->m_fp = state.fp ;
	m_stack->m_xp = state.xp ;
	m_stack->m_yp = state.yp ;
	m_ip = state.ip ;
	m_valReturn = state.valReturn ;
	m_thrown = state.thrown ;
	m_exception = state.exception ;
	m_status = state.status ;
	m_pCodeBuf = state.pCodeBuf ;
	m_awaiting = state.awaiting ;

	if ( flagAborted )
	{
		m_signal = interruptAbort ;
	}
}

// 実行結果を取得し完了する
void LContext::FinishFunctionResult( LContext::AsyncState& state )
{
	// 結果取得
	state.valRet = m_valReturn ;
	state.exception = m_exception ;
	state.finished = true ;

	// オブジェクトの後始末
	assert( state.ctxSave.fp <= state.ctxSave.sp ) ;
	micro_instruction_FreeFor( state.ctxSave.sp ) ;

	// 状態復元
	RestoreContextState( state.ctxSave ) ;
}


// 例外
//////////////////////////////////////////////////////////////////////////////

// 例外クリア
void LContext::ClearException( void )
{
	auto	lock = LockRunning() ;
	m_exception = nullptr ;
}

// 例外送出
void LContext::ThrowException( LObjPtr pObj )
{
	auto	lock = LockRunning() ;
	if ( !IsRunningStatus( m_status )
		|| (m_pCodeBuf == nullptr)
		|| (m_ip == LCodeBuffer::InvalidCodePos) )
	{
		m_exception = pObj ;
	}
	else
	{
		if ( GetCurrent() == this )
		{
			if ( m_status == statusAwaiting )
			{
				std::unique_lock<std::mutex>	lockAwaiting( m_mutexStatus ) ;
				m_status = statusRunning ;
				m_awaiting = nullptr ;
			}
			m_stack->Push( LValue::MakeObjectPtr( pObj.Detach() ) ) ;
			micro_instruction_Throw() ;
		}
		else
		{
			SetInterruptSignal( interruptThrow, pObj ) ;
		}
	}
}

void LContext::ThrowException
	( const wchar_t * pwszErrMsg, const wchar_t * pwszClassName )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs( pwszClassName ) ;
	ThrowException( new LExceptionObj( pClass, pwszErrMsg ) ) ;
}

void LContext::ThrowException( ErrorMessageIndex error )
{
	ThrowException( GetErrorMessage(error), GetExceptionClassName(error) ) ;
}

void LContext::ThrowExceptionError
	( const wchar_t * pwszErrMsg, const wchar_t * pwszClassName )
{
	if ( GetCurrent() != nullptr )
	{
		GetCurrent()->ThrowException( pwszErrMsg, pwszClassName ) ;
	}
	else
	{
		ErrorMessageIndex	err = GetErrorIndexOf( pwszErrMsg ) ;
		if ( err != errorNothing )
		{
			LCompiler::Error( err, pwszClassName ) ;
		}
		std::string	strClass, strErrMsg ;
		LTrace( "%s: %s\n",
				LString(pwszClassName).ToString(strClass).c_str(),
				LString(pwszErrMsg).ToString(strErrMsg).c_str() ) ;
	}
}

void LContext::ThrowExceptionError( ErrorMessageIndex error )
{
	ThrowExceptionError( GetErrorMessage(error), GetExceptionClassName(error) ) ;
}


// 待機関数設定
//////////////////////////////////////////////////////////////////////////////
void LContext::SetAwaiting( AwaitingFunc func )
{
	std::unique_lock<std::mutex>	lockAwaiting( m_mutexStatus ) ;
	assert( m_status == statusRunning ) ;
	assert( m_awaiting == nullptr ) ;

	m_status = statusAwaiting ;
	m_awaiting = func ;
}

// 割り込み信号を設定する
//////////////////////////////////////////////////////////////////////////////
void LContext::SetInterruptSignal( InterruptSignal signal, LObjPtr pObj )
{
	std::unique_lock<std::mutex>	lockSignal( m_mutexStatus ) ;
	if ( m_signal < signal )
	{
		m_signal = signal ;
		m_pSignalObj = pObj ;
	}
	m_interrupt = true ;
}

// 実行ステータス取得
LContext::ExecutionStatus LContext::GetExecutionStatus( void ) const
{
	return	m_status ;
}

bool LContext::IsRunning( void ) const
{
	return	IsRunningStatus( m_status ) ;
}

// 実行ループ
//////////////////////////////////////////////////////////////////////////////

// 同期実行ループ（動作が完了するまで復帰しない）
void LContext::SyncRun( void )
{
	while ( m_status != statusHalt )
	{
		AsyncRun( 10 ) ;

		while ( m_status == statusAwaiting )
		{
			assert( m_awaiting != nullptr ) ;
			if ( (m_awaiting == nullptr) || m_awaiting( 100 ) )
			{
				m_status = statusRunning ;
				m_awaiting = nullptr ;
				break ;
			}
		}
	}
}

// 非同期実行ループ（待機状態になった場合脱出する）
LContext::ExecutionStatus LContext::AsyncRun( std::int64_t msecTimeout )
{
	auto	lock = LockRunning() ;

	// 開始時間
	std::chrono::system_clock::time_point
			tpStart = std::chrono::system_clock::now() ;

	if ( m_signal != interruptNo )
	{
		// 割り込み処理
		if( ProcessSignal() )
		{
			return	m_status ;
		}
	}
	if ( m_status == statusAwaiting )
	{
		// 待機中
		assert( m_awaiting != nullptr ) ;
		if ( (m_awaiting != nullptr) && !m_awaiting( msecTimeout ) )
		{
			// 待機継続
			return	m_status ;
		}
		std::unique_lock<std::mutex>	lockAwaiting( m_mutexStatus ) ;
		m_status = statusRunning ;
		m_awaiting = nullptr ;
	}

	// 実行ループ
	while ( m_status == statusRunning )
	{
		if ( (m_pCodeBuf == nullptr)
			|| (m_ip == LCodeBuffer::InvalidCodePos) )
		{
			// 実行終了
			m_status = statusHalt ;
			break ;
		}

		RunCoreLoop() ;

		if ( m_signal != interruptNo )
		{
			// 割り込み処理
			if( ProcessSignal() )
			{
				break ;
			}
		}
		if ( (msecTimeout >= 0)
			&& (std::chrono::duration_cast<std::chrono::milliseconds>
					( std::chrono::system_clock::now() - tpStart ).count() > msecTimeout) )
		{
			// タイムアウト
			break ;
		}
	}
	return	m_status ;
}

// 割り込み信号処理
bool LContext::ProcessSignal( void )
{
	std::unique_lock<std::mutex>	lockSignal( m_mutexStatus ) ;
	if ( m_signal == interruptAbort )
	{
		m_valReturn = LValue() ;
		m_status = statusAborted ;
		m_awaiting = nullptr ;
		m_signal = interruptNo ;
		return	true ;
	}
	if ( m_signal == interruptThrow )
	{
		assert ( GetCurrent() == this ) ;
		LObjPtr	pThrow = m_pSignalObj ;
		m_signal = interruptNo ;
		m_pSignalObj = nullptr ;
		lockSignal.unlock() ;
		//
		ThrowException( pThrow ) ;
		return	false ;
	}
	if ( m_signal == interruptBreak )
	{
		m_signal = interruptNo ;
		return	true ;
	}
	return	false ;
}

// 実行コアループ
void LContext::RunCoreLoop( void )
{
	if ( m_pCodeBuf == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	const LCodeBuffer::Word *	pCodeBuf = m_pCodeBuf->m_buffer.data() ;
	if ( pCodeBuf == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	if ( (size_t) m_ip >= m_pCodeBuf->m_buffer.size() )
	{
		micro_instruction_ThrowIndexOutOfBounds() ;
		return ;
	}

	m_interrupt = false ;
	m_throwing = false ;
	m_jumped = 0 ;

	while ( !m_interrupt )
	{
		assert( m_pCodeBuf != nullptr ) ;
		assert( m_pCodeBuf->m_buffer.data() == pCodeBuf ) ;
		assert( (size_t) m_ip < m_pCodeBuf->m_buffer.size() ) ;

		const LCodeBuffer::Word&	word = pCodeBuf[m_ip ++] ;
		(this->*(s_instruction[word.code]))( word ) ;
	}
}


// オブジェクト生成
//////////////////////////////////////////////////////////////////////////////

// 文字列生成
LStringObj * LContext::new_String( const LString& str )
{
	return	new LStringObj( m_vm.GetStringClass(), str ) ;
}

LStringObj * LContext::new_String( const wchar_t * str )
{
	return	new LStringObj( m_vm.GetStringClass(), str ) ;
}

// Integer 生成
LIntegerObj * LContext::new_Integer( LLong val )
{
	return	new LIntegerObj( m_vm.GetIntegerObjClass(), val ) ;
}

// Double 生成
LDoubleObj * LContext::new_Double( LDouble val )
{
	return	new LDoubleObj( m_vm.GetDoubleObjClass(), val ) ;
}

// ポインタ生成
LPointerObj * LContext::new_Pointer( size_t nBytes )
{
	return	new_Pointer( std::make_shared<LArrayBufStorage>( nBytes ) ) ;
}

LPointerObj * LContext::new_Pointer( const void * buf, size_t nBytes )
{
	if ( buf == nullptr )
	{
		return	nullptr ;
	}
	return	new_Pointer( std::make_shared<LArrayBufStorage>
							( (const std::uint8_t*) buf, nBytes ) ) ;
}

LPointerObj * LContext::new_Pointer( std::shared_ptr<LArrayBuffer> buf )
{
	return	new LPointerObj( m_vm.GetPointerClass(), buf ) ;
}

// Array 生成
LArrayObj * LContext::new_Array( void )
{
	return	new LArrayObj( m_vm.GetArrayClass() ) ;
}

LArrayObj * LContext::new_Array( LClass * pElementType )
{
	return	new LArrayObj( m_vm.GetArrayClassAs( pElementType ) ) ;
}

// 例外オブジェクト生成
LExceptionObj * LContext::new_Exception( ErrorMessageIndex err )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs( GetExceptionClassName(err) ) ;
	return	new LExceptionObj( pClass, GetErrorMessage(err) ) ;
}

// オブジェクト生成
LObject * LContext::new_Object( const wchar_t * pwszClassPath )
{
	LClass *	pClass = m_vm.GetClassPathAs( pwszClassPath ) ;
	if ( pClass == nullptr )
	{
		return	nullptr ;
	}
	return	pClass->CreateInstance() ;
}


// 命令実行関数（特殊化無し）
//////////////////////////////////////////////////////////////////////////////
const LCodeBuffer::PFN_Instruction	
			LContext::s_instruction[LCodeBuffer::codeInstructionCount] =
{
	&LContext::instruction_NoOperation,
	&LContext::instruction_EnterFunc,
	&LContext::instruction_LeaveFunc,
	&LContext::instruction_EnterTry,
	&LContext::instruction_CallFinally,
	&LContext::instruction_RetFinally,
	&LContext::instruction_LeaveFinally,
	&LContext::instruction_LeaveTry,
	&LContext::instruction_Throw,
	&LContext::instruction_AllocStack,
	&LContext::instruction_FreeStack,
	&LContext::instruction_FreeStackLocal,
	&LContext::instruction_FreeLocalObj,
	&LContext::instruction_PushObjChain,
	&LContext::instruction_MakeObjChain,
	&LContext::instruction_Synchronize,
	&LContext::instruction_Return,
	&LContext::instruction_SetRetValue,
	&LContext::instruction_MoveAP,
	&LContext::instruction_Call,
	&LContext::instruction_CallVirtual,
	&LContext::instruction_CallIndirect,
	&LContext::instruction_ExImmPrefix,
	&LContext::instruction_ClearException,
	&LContext::instruction_LoadRetValue,
	&LContext::instruction_LoadException,
	&LContext::instruction_LoadArg,
	&LContext::instruction_LoadImm,
	&LContext::instruction_LoadLocal,
	&LContext::instruction_LoadLocalOffset,
	&LContext::instruction_LoadStack,
	&LContext::instruction_LoadFetchAddr,
	&LContext::instruction_LoadFetchAddrOffset,
	&LContext::instruction_LoadFetchLAddr,
	&LContext::instruction_CheckLPtrAlign,
	&LContext::instruction_CheckPtrAlign,
	&LContext::instruction_LoadLPtr,
	&LContext::instruction_LoadLPtrOffset,
	&LContext::instruction_LoadByPtr,
	&LContext::instruction_LoadByLAddr,
	&LContext::instruction_LoadByAddr,
	&LContext::instruction_LoadLocalPtr,
	&LContext::instruction_LoadObjectPtr,
	&LContext::instruction_LoadLObjectPtr,
	&LContext::instruction_LoadObject,
	&LContext::instruction_NewObject,
	&LContext::instruction_AllocBuf,
	&LContext::instruction_RefObject,
	&LContext::instruction_FreeObject,
	&LContext::instruction_Move,
	&LContext::instruction_StoreLocalImm,
	&LContext::instruction_StoreLocal,
	&LContext::instruction_ExchangeLocal,
	&LContext::instruction_StoreByPtr,
	&LContext::instruction_StoreByLPtr,
	&LContext::instruction_StoreByLAddr,
	&LContext::instruction_StoreByAddr,
	&LContext::instruction_BinaryOperate,
	&LContext::instruction_UnaryOperate,
	&LContext::instruction_CastObject,
	&LContext::instruction_Jump,
	&LContext::instruction_JumpConditional,
	&LContext::instruction_JumpNonConditional,
	&LContext::instruction_GetElement,
	&LContext::instruction_GetElementAs,
	&LContext::instruction_GetElementName,
	&LContext::instruction_SetElement,
	&LContext::instruction_SetElementAs,
	&LContext::instruction_ObjectToInt,
	&LContext::instruction_ObjectToFloat,
	&LContext::instruction_ObjectToString,
	&LContext::instruction_IntToObject,
	&LContext::instruction_FloatToObject,
	&LContext::instruction_IntToFloat,
	&LContext::instruction_UintToFloat,
	&LContext::instruction_FloatToInt,
	&LContext::instruction_FloatToUint,
} ;

// NoOperation
void LContext::instruction_NoOperation( const LCodeBuffer::Word& word )
{
}

// EnterFunc
void LContext::instruction_EnterFunc( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	pStack->Push( LValue::MakeLong( pStack->m_fp ) ) ;
	pStack->m_fp = pStack->m_sp ;
	pStack->AddPointer( (size_t) word.imm ) ;
}

// LeaveFunc
void LContext::instruction_LeaveFunc( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	micro_instruction_FreeFor( pStack->m_fp ) ;
	pStack->m_sp = pStack->m_fp ;
	pStack->m_fp = (size_t) pStack->Pop().longValue ;
}

// EnterTry
void LContext::instruction_EnterTry( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	pStack->Push( LValue::MakeVoidPtr( m_pCodeBuf ) ) ;
	pStack->Push( LValue::MakeLong( pStack->m_fp ) ) ;
	pStack->Push( LValue::MakeLong( pStack->m_xp ) ) ;
	pStack->m_xp = pStack->m_sp - LCodeBuffer::ExceptDescSize ;
}

// CallFinally
void LContext::instruction_CallFinally( const LCodeBuffer::Word& word )
{
	micro_instruction_CallFinally() ;
}

// RetFinally
void LContext::instruction_RetFinally( const LCodeBuffer::Word& word )
{
	micro_instruction_RetFinally() ;
}

// LeaveFinally
void LContext::instruction_LeaveFinally( const LCodeBuffer::Word& word )
{
	micro_instruction_LeaveFinally() ;
}

// LeaveTry
void LContext::instruction_LeaveTry( const LCodeBuffer::Word& word )
{
	micro_instruction_LeaveTry() ;
}

// Throw
void LContext::instruction_Throw( const LCodeBuffer::Word& word )
{
	micro_instruction_Throw() ;
}

// AllocStack
void LContext::instruction_AllocStack( const LCodeBuffer::Word& word )
{
	m_stack->AddPointer( (size_t) word.imm ) ;
}

// FreeStack
void LContext::instruction_FreeStack( const LCodeBuffer::Word& word )
{
	size_t	nFree = (size_t) word.imm ;
	micro_instruction_FreeCount( nFree ) ;
}

// FreeStackLocal
void LContext::instruction_FreeStackLocal( const LCodeBuffer::Word& word )
{
	size_t	sp = m_stack->m_fp + (size_t) word.imop.value.longValue ;
	micro_instruction_FreeFor( sp ) ;
	m_stack->m_sp = sp ;
}

// FreeLocalObj
void LContext::instruction_FreeLocalObj( const LCodeBuffer::Word& word )
{
	LStackBuffer::Word&	obj =
		m_stack->At( m_stack->m_fp + (size_t) word.imop.value.longValue ) ;
	LObject::ReleaseRef( obj.pObject ) ;
	obj.pObject = nullptr ;
}

// PushObjChain
void LContext::instruction_PushObjChain( const LCodeBuffer::Word& word )
{
	m_stack->Push( LValue::MakeLong( m_stack->m_yp ) ) ;
	m_stack->m_yp = m_stack->m_sp - 2 ;
}

// MakeObjChain
void LContext::instruction_MakeObjChain( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	const size_t	iObj = pStack->m_fp + (size_t) word.imop.value.longValue - 1 ;
	pStack->At( iObj + 1 ).longValue = pStack->m_yp ;
	pStack->m_yp = iObj ;
}

// Synchronize
void LContext::instruction_Synchronize( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	const size_t	iObj = pStack->m_fp + (size_t) word.imop.value.longValue ;
	LObject *		pObj = pStack->At( iObj ).pObject ;
	if ( pObj == nullptr )
	{
		pStack->Push( LValue::MakeObjectPtr( nullptr ) ) ;
		pStack->Push( LValue::MakeLong( 0 ) ) ;
		return ;
	}
	pObj->AddRef() ;

	LSyncLockerObj *	pSyncObj =
		new LSyncLockerObj( m_vm.GetObjectClass(), pObj, this ) ;
	if ( !pSyncObj->IsLocked() )
	{
		m_status = statusAwaiting ;
		m_awaiting = [pSyncObj]( std::int64_t timeout )
		{
			return	pSyncObj->WaitSync( timeout ) ;
		} ;
		m_interrupt = true ;
	}

	pStack->Push( LValue::MakeObjectPtr( pSyncObj ) ) ;
	pStack->Push( LValue::MakeLong( pStack->m_yp ) ) ;
	pStack->m_yp = pStack->m_sp - 2 ;
}

// Return
void LContext::instruction_Return( const LCodeBuffer::Word& word )
{
	m_pCodeBuf = reinterpret_cast<LCodeBuffer*>( m_stack->Pop().pVoidPtr ) ;
	m_ip = (size_t) m_stack->Pop().longValue ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	m_stack->NotifyBufferReallocation() ;
	m_interrupt = true ;
}

// SetRetValue
void LContext::instruction_SetRetValue( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	LValue::Primitive&	val = m_stack->BackAt( word.sop1 ) ;

	if ( LType::IsPrimitiveType( type ) )
	{
		m_valReturn = LValue( type, val ) ;
	}
	else
	{
		m_valReturn = LValue( val.pObject ) ;
	}
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
}

// MoveAP
void LContext::instruction_MoveAP( const LCodeBuffer::Word& word )
{
	assert( m_stack->m_sp >= (size_t) word.imm ) ;
	m_stack->m_ap = m_stack->m_sp - word.imm ;
}

// Call
void LContext::instruction_Call( const LCodeBuffer::Word& word )
{
	micro_instruction_Call( word.imop.pFunc ) ;
}

// CallVirtual
void LContext::instruction_CallVirtual( const LCodeBuffer::Word& word )
{
	LClass *	pClass = word.imop.pClass ;
	assert( pClass != nullptr ) ;

	LObject *	pThisObj = m_stack->ArgAt(0).pObject ;
	if ( (pThisObj == nullptr) || (pThisObj->GetClass() == nullptr) )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}

	const LVirtualFuncVector *
		pVirtVec = pThisObj->GetClass()->GetVirtualVectorOf( pClass ) ;
	if ( pVirtVec == nullptr )
	{
		micro_instruction_ThrowNoVirtualVector( pThisObj->GetClass(), pClass ) ;
		return ;
	}

	micro_instruction_Call
		( pVirtVec->GetFunctionAt( (size_t) word.imm ).Ptr() ) ;
}

// CallIndirect
void LContext::instruction_CallIndirect( const LCodeBuffer::Word& word )
{
	LObject *	pFuncObj = m_stack->Pop().pObject ;
	if ( pFuncObj == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	micro_instruction_Call( dynamic_cast<LFunctionObj*>( pFuncObj ) ) ;
}

// ExImmPrefix
void LContext::instruction_ExImmPrefix( const LCodeBuffer::Word& word )
{
	m_eximm = word.imop.value ;
}

// ClearException
void LContext::instruction_ClearException( const LCodeBuffer::Word& word )
{
	m_exception = nullptr ;
}

// LoadRetValue
void LContext::instruction_LoadRetValue( const LCodeBuffer::Word& word )
{
	m_valReturn.AddRef() ;
	m_stack->Push( m_valReturn.Value() ) ;
}

// LoadException
void LContext::instruction_LoadException( const LCodeBuffer::Word& word )
{
	m_stack->Push( LValue::MakeObjectPtr( m_exception.Ptr() ) ) ;
}

// LoadArg
void LContext::instruction_LoadArg( const LCodeBuffer::Word& word )
{
	assert( m_stack->m_ap + word.imm < m_stack->m_fp ) ;
	assert( m_stack->m_ap + word.imm < m_stack->m_sp ) ;
	m_stack->Push( m_stack->ArgAt( (size_t) word.imm ) ) ;
}

// LoadImm
void LContext::instruction_LoadImm( const LCodeBuffer::Word& word )
{
	m_stack->Push( word.imop.value ) ;
}

// LoadLocal
void LContext::instruction_LoadLocal( const LCodeBuffer::Word& word )
{
	size_t	iLocal = m_stack->m_fp + (size_t) word.imop.value.longValue ;

	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type <= LType::typeAllCount ) ;

	m_stack->Push( (s_pfnLoadLocal[type])( m_stack->At(iLocal) ) ) ;
}

// LoadLocalOffset
void LContext::instruction_LoadLocalOffset( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	size_t	iLocal = pStack->m_fp
					+ (size_t) pStack->BackAt(word.sop1).longValue
					+ (size_t) word.imop.value.longValue ;

	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type <= LType::typeAllCount ) ;

	pStack->Push( (s_pfnLoadLocal[type])( pStack->At(iLocal) ) ) ;
}

// LoadStack
void LContext::instruction_LoadStack( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	pStack->Push( pStack->BackAt(word.sop1) ) ;
}

// LoadFetchAddr
void LContext::instruction_LoadFetchAddr( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->BackIndex(word.sop1),
				(ssize_t) word.imop.value.longValue,
				(size_t) word.imm, word.op3 ) ;
	if ( m_throwing )
	{
		return ;
	}
	pStack->Push( micro_instruction_CheckAlignment( pBuf, word.op3 ) ) ;
}

// LoadFetchAddrOffset
void LContext::instruction_LoadFetchAddrOffset( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->BackIndex(word.sop1),
				(ssize_t) pStack->BackAt(word.sop2).longValue
					+ (ssize_t) word.imop.value.longValue,
				(size_t) word.imm, word.op3 ) ;
	if ( m_throwing )
	{
		return ;
	}
	pStack->Push( micro_instruction_CheckAlignment( pBuf, word.op3 ) ) ;
}

// LoadFetchLAddr
void LContext::instruction_LoadFetchLAddr( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->m_fp + word.sop1,
				(ssize_t) pStack->BackAt(word.sop2).longValue
					+ (ssize_t) word.imop.value.longValue,
				(size_t) word.imm, word.op3 ) ;
	if ( m_throwing )
	{
		return ;
	}
	pStack->Push( micro_instruction_CheckAlignment( pBuf, word.op3 ) ) ;
}

// CheckLPtrAlign
void LContext::instruction_CheckLPtrAlign( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->m_fp + (size_t) word.imop.value.longValue,
				(size_t) pStack->At(word.sop1).longValue + word.imm, 0, word.op3 ) ;
	if ( !m_throwing )
	{
		micro_instruction_CheckAlignment( pBuf, word.op3 ) ;
	}
}

// CheckPtrAlign
void LContext::instruction_CheckPtrAlign( const LCodeBuffer::Word& word )
{
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( m_stack->BackIndex( word.sop1 ), word.imm, 0, word.op3 ) ;
	if ( !m_throwing )
	{
		micro_instruction_CheckAlignment( pBuf, word.op3 ) ;
	}
}

// LoadLPtr
void LContext::instruction_LoadLPtr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->m_fp + (size_t) word.imop.value.longValue,
				(size_t) word.imm, LType::s_bytesAligned[type], 1 ) ;
	if ( (pBuf == nullptr) || m_throwing )
	{
		return ;
	}
	pStack->Push( (s_pfnLoadFromPointer[type])( pBuf ) ) ;
}

// LoadLPtrOffset
void LContext::instruction_LoadLPtrOffset( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->m_fp + (size_t) word.imop.value.longValue,
				(size_t) pStack->BackAt(word.sop1).longValue
				+ (size_t) word.imm, LType::s_bytesAligned[type], 1 ) ;
	if ( (pBuf == nullptr) || m_throwing )
	{
		return ;
	}
	pStack->Push( (s_pfnLoadFromPointer[type])( pBuf ) ) ;
}

// LoadByPtr
void LContext::instruction_LoadByPtr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		micro_instruction_FetchAddr
			( pStack->BackIndex( word.sop1 ),
				(size_t) word.imm, LType::s_bytesAligned[type], 1 ) ;
	if ( (pBuf == nullptr) || m_throwing )
	{
		return ;
	}
	pStack->Push( (s_pfnLoadFromPointer[type])( pBuf ) ) ;
}

// LoadByLAddr
void LContext::instruction_LoadByLAddr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		reinterpret_cast<std::uint8_t*>
			( pStack->At( pStack->m_fp
					+ (size_t) word.imop.value.longValue ).pVoidPtr ) ;
	if ( pBuf == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	pStack->Push( (s_pfnLoadFromPointer[type])( pBuf + word.imm ) ) ;
}

// LoadByAddr
void LContext::instruction_LoadByAddr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		reinterpret_cast<std::uint8_t*>
			( pStack->BackAt(word.sop1).pVoidPtr ) ;
	if ( pBuf == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	pStack->Push( (s_pfnLoadFromPointer[type])( pBuf + word.imm ) ) ;
}

// LoadLocalPtr
void LContext::instruction_LoadLocalPtr( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	size_t	si = (pStack->m_fp + (size_t) word.imop.value.longValue)
										* sizeof(LStackBuffer::Word) ;
	if ( si + (size_t) word.imm > pStack->m_sp * sizeof(LStackBuffer::Word) )
	{
		micro_instruction_ThrowIndexOutOfBounds() ;
		return ;
	}
	LPointerObj *	pPtr = new LPointerObj( m_vm.GetPointerClass() ) ;
	pPtr->SetPointer( m_stack, si, (size_t) word.imm ) ;
	pStack->Push( LValue::MakeObjectPtr( pPtr ) ) ;
}

// LoadObjectPtr
void LContext::instruction_LoadObjectPtr( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt( word.sop1 ).pObject ;
	if ( pObj == nullptr )
	{
		m_stack->Push( LValue::MakeObjectPtr( nullptr ) ) ;
		return ;
	}
	m_stack->Push( LValue::MakeObjectPtr( pObj->GetBufferPoiner() ) ) ;
}

// LoadLObjectPtr
void LContext::instruction_LoadLObjectPtr( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	LObject *	pObj =
		pStack->At( pStack->m_fp
					+ (size_t) word.imop.value.longValue ).pObject ;
	if ( pObj == nullptr )
	{
		pStack->Push( LValue::MakeObjectPtr( nullptr ) ) ;
		return ;
	}
	pStack->Push( LValue::MakeObjectPtr( pObj->GetBufferPoiner() ) ) ;
}

// LoadObject
void LContext::instruction_LoadObject( const LCodeBuffer::Word& word )
{
	if ( word.imop.value.pObject != nullptr )
	{
		m_stack->Push
			( LValue::MakeObjectPtr
				( word.imop.value.pObject->CloneObject() ) ) ;
	}
	else
	{
		m_stack->Push( LValue::MakeObjectPtr( nullptr ) ) ;
	}
}

// NewObject
void LContext::instruction_NewObject( const LCodeBuffer::Word& word )
{
	assert( word.imop.pClass != nullptr ) ;
	m_stack->Push( LValue::MakeObjectPtr( word.imop.pClass->CreateInstance() ) ) ;
}

// AllocBuf
void LContext::instruction_AllocBuf( const LCodeBuffer::Word& word )
{
	LPointerObj *	pPtr =
			dynamic_cast<LPointerObj*>( m_stack->BackAt(word.sop1).pObject ) ;
	if ( pPtr == nullptr )
	{
		micro_instruction_ThrowNullException() ;
	}
	const size_t	nBytes = (size_t) m_stack->BackAt(word.sop2).longValue ;

	std::shared_ptr<LArrayBufStorage>
					pBuf = std::make_shared<LArrayBufStorage>( nBytes ) ;

	if ( word.imop.pClass != nullptr )
	{
		LStructureClass *	pStruct =
				dynamic_cast<LStructureClass*>( word.imop.pClass ) ;
		if ( pStruct != nullptr )
		{
			// 構造体の場合は初期値で埋める
			std::shared_ptr<LArrayBufStorage>
							pStructBuf = pStruct->GetProtoArrangemenet().GetBuffer() ;
			const size_t	nStructBytes = pStruct->GetStructBytes() ;
			const std::uint8_t *
							pProtoData = pStructBuf->GetBuffer() ;
			std::uint8_t *	pData = pBuf->GetBuffer() ;
			for ( size_t i = 0; i < nBytes; i += nStructBytes )
			{
				size_t	nCopyBytes = std::min( nBytes - i, nStructBytes ) ;
				memcpy( pData + i, pProtoData, nCopyBytes ) ;
			}
		}
	}
	pPtr->SetPointer( pBuf, 0, nBytes ) ;
}

// RefObject
void LContext::instruction_RefObject( const LCodeBuffer::Word& word )
{
	LObject::AddRef( m_stack->BackAt( word.sop1 ).pObject ) ;
}

// FreeObject
void LContext::instruction_FreeObject( const LCodeBuffer::Word& word )
{
	LObject::ReleaseRef( m_stack->BackAt( word.sop1 ).pObject ) ;
}

// Move
void LContext::instruction_Move( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	pStack->BackAt(word.sop2) = pStack->BackAt(word.sop1) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
}

// StoreLocalImm
void LContext::instruction_StoreLocalImm( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	pStack->At( pStack->m_fp + (size_t) word.imm ) =
						(s_pfnStoreLocal[type])( word.imop.value ) ;
}

// StoreLocal
void LContext::instruction_StoreLocal( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	pStack->At( pStack->m_fp + (size_t) word.imop.value.longValue ) =
					(s_pfnStoreLocal[type])( pStack->BackAt(word.sop1) ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
}

// ExchangeLocal
void LContext::instruction_ExchangeLocal( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	const size_t	iDst = pStack->m_fp + (size_t) word.imop.value.longValue ;
	const size_t	iSrc = pStack->BackIndex( word.sop1 ) ;

	LValue::Primitive	temp = (s_pfnLoadLocal[type])( pStack->At(iDst) ) ;
	pStack->At(iDst) = (s_pfnStoreLocal[type])( pStack->At(iSrc) ) ;
	pStack->At(iSrc) = temp ;

	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
}

// StoreByPtr
void LContext::instruction_StoreByPtr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	std::uint8_t *	pBuf = 
		micro_instruction_FetchAddr
				( m_stack->BackIndex(word.sop1),
					word.imm, LType::s_bytesAligned[type], 1 ) ;
	if ( (pBuf == nullptr) || m_throwing )
	{
		return ;
	}
	(s_pfnStoreIntoPointer[type])( pBuf, m_stack->BackAt(word.sop2) ) ;
}

// StoreByLPtr
void LContext::instruction_StoreByLPtr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	std::uint8_t *	pBuf = 
		micro_instruction_FetchAddr
				( m_stack->m_fp
					+ (size_t) word.imop.value.longValue,
					word.imm, LType::s_bytesAligned[type], 1 ) ;
	if ( (pBuf == nullptr) || m_throwing )
	{
		return ;
	}
	(s_pfnStoreIntoPointer[type])( pBuf, m_stack->BackAt(word.sop2) ) ;
}

// StoreByLAddr
void LContext::instruction_StoreByLAddr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		reinterpret_cast<std::uint8_t*>
			( pStack->At( pStack->m_fp
						+ (size_t) word.imop.value.longValue ).pVoidPtr ) ;
	if ( pBuf == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	(s_pfnStoreIntoPointer[type])( pBuf + word.imm, m_stack->BackAt(word.sop2) ) ;
}

// StoreByAddr
void LContext::instruction_StoreByAddr( const LCodeBuffer::Word& word )
{
	LType::Primitive	type = (LType::Primitive) word.op3 ;
	assert( type < LType::typeAllCount ) ;

	LStackBuffer *	pStack = m_stack.get() ;
	std::uint8_t *	pBuf =
		reinterpret_cast<std::uint8_t*>( pStack->BackAt(word.sop1).pVoidPtr ) ;
	if ( pBuf == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	(s_pfnStoreIntoPointer[type])( pBuf + word.imm, m_stack->BackAt(word.sop2) ) ;
}

// BinaryOperate
void LContext::instruction_BinaryOperate( const LCodeBuffer::Word& word )
{
	LStackBuffer *		pStack = m_stack.get() ;
	LValue::Primitive	valRet = word.imop.pfnOp2
									( pStack->BackAt(word.sop1),
										pStack->BackAt(word.sop2),
										m_eximm.pVoidPtr ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	pStack->Push( valRet ) ;
	m_eximm.pVoidPtr = nullptr ;
}

// UnaryOperate
void LContext::instruction_UnaryOperate( const LCodeBuffer::Word& word )
{
	LStackBuffer *		pStack = m_stack.get() ;
	LValue::Primitive	valRet = word.imop.pfnOp1
									( pStack->BackAt(word.sop1),
										m_eximm.pVoidPtr ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	pStack->Push( valRet ) ;
	m_eximm.pVoidPtr = nullptr ;
}

// CastObject
void LContext::instruction_CastObject( const LCodeBuffer::Word& word )
{
	LStackBuffer *	pStack = m_stack.get() ;
	LObject *		pObj = pStack->BackAt(word.sop1).pObject ;
	if ( (pObj != nullptr) && (word.imop.pClass != nullptr) )
	{
		pObj = pObj->CastClassTo( word.imop.pClass ) ;
	}
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	pStack->Push( LValue::MakeObjectPtr( pObj ) ) ;
}

// Jump
void LContext::instruction_Jump( const LCodeBuffer::Word& word )
{
	m_ip = (size_t) word.imop.value.longValue ;

	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	micro_instruction_VerifyJumped() ;
}

// JumpConditional
void LContext::instruction_JumpConditional( const LCodeBuffer::Word& word )
{
	size_t	nFreeCount = (size_t) word.imm ;
	if ( m_stack->BackAt(word.sop1).longValue )
	{
		m_ip = (size_t) word.imop.value.longValue ;
		nFreeCount += word.sop2 + (size_t) word.op3 * 0x100 ;
	}
	if ( nFreeCount > 0 )
	{
		micro_instruction_FreeCount( nFreeCount ) ;
	}
	micro_instruction_VerifyJumped() ;
}

// JumpNonConditional
void LContext::instruction_JumpNonConditional( const LCodeBuffer::Word& word )
{
	size_t	nFreeCount = (size_t) word.imm ;
	if ( !m_stack->BackAt(word.sop1).longValue )
	{
		m_ip = (size_t) word.imop.value.longValue ;
		nFreeCount += word.sop2 + (size_t) word.op3 * 0x100 ;
	}
	if ( nFreeCount > 0 )
	{
		micro_instruction_FreeCount( nFreeCount ) ;
	}
	micro_instruction_VerifyJumped() ;
}

// GetElement
void LContext::instruction_GetElement( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt(word.sop1).pObject ;
	if ( pObj == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	pObj = pObj->GetElementAt
				( (size_t) m_stack->BackAt(word.sop2).longValue ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	m_stack->Push( LValue::MakeObjectPtr( pObj ) ) ;
}

// GetElementAs
void LContext::instruction_GetElementAs( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt(word.sop1).pObject ;
	LObject *	pStr = m_stack->BackAt(word.sop2).pObject ;
	if ( (pObj == nullptr) || (pStr == nullptr) )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	LString	str ;
	pStr->AsString( str ) ;

	pObj = pObj->GetElementAs( str.c_str() ) ;

	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	m_stack->Push( LValue::MakeObjectPtr( pObj ) ) ;
}

// GetElementName
void LContext::instruction_GetElementName( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt(word.sop1).pObject ;
	if ( pObj == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	LString	name ;
	pObj->GetElementNameAt
			( name, (size_t) m_stack->BackAt(word.sop2).longValue ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
	m_stack->Push( LValue::MakeObjectPtr( new_String( name ) ) ) ;
}

// SetElement
void LContext::instruction_SetElement( const LCodeBuffer::Word& word )
{
	LObject *	pObj1 = m_stack->BackAt(word.sop1).pObject ;
	LObject *	pObj2 = m_stack->BackAt(word.op3).pObject ;
	if ( pObj1 == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	LObject::AddRef( pObj2 ) ;
	LObject::ReleaseRef
		( pObj1->SetElementAt
			( (size_t) m_stack->BackAt(word.sop2).longValue, pObj2 ) ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
}

// SetElementAs
void LContext::instruction_SetElementAs( const LCodeBuffer::Word& word )
{
	LObject *	pObj1 = m_stack->BackAt(word.sop1).pObject ;
	LObject *	pStr = m_stack->BackAt(word.sop2).pObject ;
	LObject *	pObj2 = m_stack->BackAt(word.op3).pObject ;
	if ( (pObj1 == nullptr) || (pStr == nullptr) )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}
	LString	str ;
	pStr->AsString( str ) ;

	LObject::AddRef( pObj2 ) ;
	LObject::ReleaseRef( pObj1->SetElementAs( str.c_str(), pObj2 ) ) ;
	if ( word.imm > 0 )
	{
		micro_instruction_FreeCount( (size_t) word.imm ) ;
	}
}

// ObjectToInt
void LContext::instruction_ObjectToInt( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt(word.sop1).pObject ;
	LLong		val = 0 ;
	if ( pObj != nullptr )
	{
		pObj->AsInteger( val ) ;
	}
	m_stack->BackAt(word.sop2) = LValue::MakeLong( val ) ;
}

// ObjectToFloat
void LContext::instruction_ObjectToFloat( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt(word.sop1).pObject ;
	LDouble		val = 0.0 ;
	if ( pObj != nullptr )
	{
		pObj->AsDouble( val ) ;
	}
	m_stack->BackAt(word.sop2) = LValue::MakeDouble( val ) ;
}

// ObjectToString
void LContext::instruction_ObjectToString( const LCodeBuffer::Word& word )
{
	LObject *	pObj = m_stack->BackAt(word.sop1).pObject ;
	LString		str ;
	if ( pObj != nullptr )
	{
		pObj->AsString( str ) ;
	}
	m_stack->Push( LValue::MakeObjectPtr( new_String( str ) ) ) ;
}

// IntToObject
void LContext::instruction_IntToObject( const LCodeBuffer::Word& word )
{
	m_stack->Push
		( LValue::MakeObjectPtr
			( new_Integer( m_stack->BackAt(word.sop1).longValue ) ) ) ;
}

// FloatToObject
void LContext::instruction_FloatToObject( const LCodeBuffer::Word& word )
{
	m_stack->Push
		( LValue::MakeObjectPtr
			( new_Double( m_stack->BackAt(word.sop1).dblValue ) ) ) ;
}

// IntToFloat
void LContext::instruction_IntToFloat( const LCodeBuffer::Word& word )
{
	m_stack->BackAt(word.sop2) =
		LValue::MakeDouble
			( (LDouble) m_stack->BackAt(word.sop1).longValue ) ;
}

// UintToFloat
void LContext::instruction_UintToFloat( const LCodeBuffer::Word& word )
{
	m_stack->BackAt(word.sop2) =
		LValue::MakeDouble
			( (LDouble) m_stack->BackAt(word.sop1).ulongValue ) ;
}

// FloatToInt
void LContext::instruction_FloatToInt( const LCodeBuffer::Word& word )
{
	m_stack->BackAt(word.sop2) =
		LValue::MakeLong
			( (LLong) m_stack->BackAt(word.sop1).dblValue ) ;
}

// FloatToUint
void LContext::instruction_FloatToUint( const LCodeBuffer::Word& word )
{
	m_stack->BackAt(word.sop2) =
		LValue::MakeUint64
			( (LUint64) m_stack->BackAt(word.sop1).dblValue ) ;
}

// si の位置までスタック上のオブジェクトを開放
void LContext::micro_instruction_FreeFor( ssize_t si )
{
	LStackBuffer *	pStack = m_stack.get() ;
	while ( si <= pStack->m_yp )
	{
		LCodeBuffer::ReleaserDesc *
			pDesc = reinterpret_cast<LCodeBuffer::ReleaserDesc*>
										( pStack->Ptr( pStack->m_yp ) ) ;

		LObject *	pObj = pDesc->object.pObject ;
		pDesc->object.pObject = nullptr ;
		pStack->m_yp = (size_t) pDesc->prev_fp.longValue ;

		LObject::ReleaseRef( pObj ) ;
	}
}

// sp-n の位置までスタック上のオブジェクトを開放
void LContext::micro_instruction_FreeCount( size_t n )
{
	if ( n > 0 )
	{
		micro_instruction_FreeFor( m_stack->m_sp - n ) ;
		m_stack->SubPointer( n ) ;
	}
}

// CallFinally
void LContext::micro_instruction_CallFinally( void )
{
	LStackBuffer *	pStack = m_stack.get() ;
	LCodeBuffer::ExceptionDesc *
			pExceptDesc = reinterpret_cast<LCodeBuffer::ExceptionDesc*>
											( pStack->Ptr( pStack->m_xp ) ) ;
	if ( pExceptDesc->cpFinally.longValue != LCodeBuffer::ExceptNoHandler )
	{
		// finally ハンドラが存在する

		// スタックを例外記述ブロックまで解放する
		micro_instruction_FreeFor( pStack->m_xp ) ;
		pStack->m_sp = pStack->m_xp + LCodeBuffer::ExceptDescSize ;

		// FinallyRetStack をプッシュ
		pStack->Push( LValue::MakeObjectPtr( m_thrown.Detach() ) ) ;
		m_valReturn.AddRef() ;
		pStack->Push( m_valReturn.Value() ) ;
		pStack->Push( LValue::MakeLong( m_valReturn.GetType().GetPrimitive() ) ) ;
		pStack->Push( LValue::MakeLong( m_ip ) ) ;
		pStack->Push( LValue::MakeVoidPtr( m_pCodeBuf ) ) ;

		// finally ハンドラへジャンプ
		m_pCodeBuf = reinterpret_cast<LCodeBuffer*>( pExceptDesc->codeBuf.pVoidPtr ) ;
		m_ip = (size_t) pExceptDesc->cpFinally.longValue ;

		// 例外記述ブロックのハンドラをオミットする
		pExceptDesc->cpHandler.longValue = LCodeBuffer::ExceptNoHandler ;
		pExceptDesc->cpFinally.longValue = LCodeBuffer::ExceptNoHandler ;

		// finally ハンドラ用のフレームポインタ
		pStack->m_fp = (size_t) pExceptDesc->fp.longValue ;

		m_interrupt = true ;
	}
	else
	{
		// finally ハンドラが存在しないので例外記述ブロックを解除する
		micro_instruction_LeaveTry() ;
	}
}

// RetFinally
void LContext::micro_instruction_RetFinally( void )
{
	LStackBuffer *	pStack = m_stack.get() ;

	// CallFinally 呼び出し元
	m_pCodeBuf = reinterpret_cast<LCodeBuffer*>( pStack->Pop().pVoidPtr ) ;
	m_ip = (size_t) pStack->Pop().longValue ;

	// 保存した関数返り値を復帰
	LType::Primitive	typeRet = (LType::Primitive) pStack->Pop().longValue ;
	if ( LType::IsPrimitiveType( typeRet ) )
	{
		m_valReturn = LValue( typeRet, pStack->Pop() ) ;
	}
	else
	{
		m_valReturn = LValue( LObjPtr( pStack->Pop().pObject ) ) ;
	}

	// 保存した threw オブジェクトを取得
	LObject *	pThrew = pStack->Pop().pObject ;

	// 例外記述ブロックを解除する
	micro_instruction_LeaveTry() ;

	// threw オブジェクトがあった場合は throw 処理の途中なので継続
	if ( pThrew != nullptr )
	{
		pStack->Push( LValue::MakeObjectPtr( pThrew ) ) ;

		// throw 命令実行
		m_ip ++ ;
		micro_instruction_Throw() ;
	}
	m_interrupt = true ;
}

// LeaveFinally
void LContext::micro_instruction_LeaveFinally( void )
{
	LStackBuffer *	pStack = m_stack.get() ;

	// CallFinally 呼び出し元は破棄
	pStack->Pop() ;
	pStack->Pop() ;

	// 保存した関数返り値を復帰
	LType::Primitive	typeRet = (LType::Primitive) pStack->Pop().longValue ;
	if ( LType::IsPrimitiveType( typeRet ) )
	{
		m_valReturn = LValue( typeRet, pStack->Pop() ) ;
	}
	else
	{
		m_valReturn = LValue( LObjPtr( pStack->Pop().pObject ) ) ;
	}

	// 保存した threw オブジェクトを破棄
	LObject::ReleaseRef( pStack->Pop().pObject ) ;

	// 例外記述ブロックを解除する
	micro_instruction_LeaveTry() ;
}

// LeaveTry
void LContext::micro_instruction_LeaveTry( void )
{
	LStackBuffer *	pStack = m_stack.get() ;

	micro_instruction_FreeFor( pStack->m_xp ) ;

	LCodeBuffer::ExceptionDesc *
			pExceptDesc = reinterpret_cast<LCodeBuffer::ExceptionDesc*>
											( pStack->Ptr( pStack->m_xp ) ) ;

	pStack->m_sp = pStack->m_xp ;
	pStack->m_fp = (size_t) pExceptDesc->fp.longValue ;
	pStack->m_xp = (size_t) pExceptDesc->prev_xp.longValue ;

	m_thrown = nullptr ;
}

// ジャンプ検証
void LContext::micro_instruction_VerifyJumped( void )
{
	if ( ++ m_jumped >= 0x10000 )
	{
		m_interrupt = true ;
	}
}

// Throw
void LContext::micro_instruction_Throw( void )
{
	LStackBuffer *	pStack = m_stack.get() ;

	m_throwing = true ;
	m_thrown = pStack->Pop().pObject ;
	if ( m_thrown == nullptr )
	{
		m_thrown = m_nullThrown ;
	}
	else
	{
		LExceptionObj *	pException =	
				dynamic_cast<LExceptionObj*>( m_thrown.Ptr() ) ;
		if ( (pException != nullptr)
			&& (pException->m_pCodeBuf == nullptr) )
		{
			pException->m_pCodeBuf = m_pCodeBuf ;
			pException->m_iThrown = m_ip ;
		}
	}

	while ( pStack->m_xp != LStackBuffer::InvalidPtr )
	{
		LCodeBuffer::ExceptionDesc *
			pExceptDesc = reinterpret_cast<LCodeBuffer::ExceptionDesc*>
											( pStack->Ptr( pStack->m_xp ) ) ;
		if ( pExceptDesc->cpHandler.longValue == LCodeBuffer::ExceptNoHandler )
		{
			if ( pExceptDesc->cpFinally.longValue != LCodeBuffer::ExceptNoHandler )
			{
				// finally 句を呼び出す
				m_ip -- ;						// ※ RetFinally で再度 Throw を実行する
				micro_instruction_CallFinally() ;
				return ;
			}
			else
			{
				// ハンドラがないのでひとつ前の例外記述ブロックへ
				pStack->m_xp = (size_t) pExceptDesc->prev_xp.longValue ;
			}
		}
		else
		{
			// 例外処理ハンドラを発見
			break ;
		}
	}

	m_exception = m_thrown.Detach() ;
	if ( m_exception == m_nullThrown )
	{
		m_exception = nullptr ;
	}

	if ( pStack->m_xp != LStackBuffer::InvalidPtr )
	{
		// スタックを例外記述ブロックまで解放する
		micro_instruction_FreeFor( pStack->m_xp ) ;
		pStack->m_sp = pStack->m_xp + LCodeBuffer::ExceptDescSize ;

		// 例外処理ハンドラへジャンプ
		LCodeBuffer::ExceptionDesc *
			pExceptDesc = reinterpret_cast<LCodeBuffer::ExceptionDesc*>
											( pStack->Ptr( pStack->m_xp ) ) ;

		m_pCodeBuf = reinterpret_cast<LCodeBuffer*>( pExceptDesc->codeBuf.pVoidPtr ) ;
		m_ip = (size_t) pExceptDesc->cpHandler.longValue ;

		// 例外処理ハンドラ用のフレームポインタ
		pStack->m_fp = (size_t) pExceptDesc->fp.longValue ;

		// 例外処理ハンドラをオミットする
		pExceptDesc->cpHandler.longValue = LCodeBuffer::ExceptNoHandler ;
	}
	else
	{
		// 処理されない例外
		m_status = statusHalt ;
	}
	m_interrupt = true ;
}

void LContext::micro_instruction_ThrowNullException( void )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs
							( GetExceptionClassName(exceptionNullPointer) ) ;
	m_stack->Push
		( LValue::MakeObjectPtr
			( new LExceptionObj
				( pClass, GetErrorMessage(exceptionNullPointer) ) ) ) ;

	micro_instruction_Throw() ;
}

void LContext::micro_instruction_ThrowIndexOutOfBounds( void )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs
							( GetExceptionClassName(exceptionIndexOutOfBounds) ) ;
	m_stack->Push
		( LValue::MakeObjectPtr
			( new LExceptionObj
				( pClass, GetErrorMessage(exceptionIndexOutOfBounds) ) ) ) ;

	micro_instruction_Throw() ;
}

void LContext::micro_instruction_ThrowPointerOutOfBounds( void )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs
							( GetExceptionClassName(exceptionPointerOutOfBounds) ) ;
	m_stack->Push
		( LValue::MakeObjectPtr
			( new LExceptionObj
				( pClass, GetErrorMessage(exceptionPointerOutOfBounds) ) ) ) ;

	micro_instruction_Throw() ;
}

void LContext::micro_instruction_ThrowAlignmentMismatch( void )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs
							( GetExceptionClassName(exceptionPointerAlignmentMismatch) ) ;
	m_stack->Push
		( LValue::MakeObjectPtr
			( new LExceptionObj
				( pClass, GetErrorMessage(exceptionPointerAlignmentMismatch) ) ) ) ;

	micro_instruction_Throw() ;
}

void LContext::micro_instruction_ThrowUnimplemented( LFunctionObj * pFunc )
{
	LClass *	pClass = m_vm.GetExceptioinClassAs
							( GetExceptionClassName(exceptionUnimplemented) ) ;
	m_stack->Push
		( LValue::MakeObjectPtr
			( new LExceptionObj
				( pClass, (pFunc->GetFullPrintName() + L": "
							+ GetErrorMessage(exceptionUnimplemented)).c_str() ) ) ) ;

	micro_instruction_Throw() ;
}

void LContext::micro_instruction_ThrowNoVirtualVector
			( LClass * pThisClass, LClass * pVirtClass )
{
	LClass *	pExceptClass =
		m_vm.GetExceptioinClassAs
				( GetExceptionClassName(exceptionNoVirtualVector) ) ;
	m_stack->Push
		( LValue::MakeObjectPtr
			( new LExceptionObj
				( pExceptClass, (pThisClass->GetFullClassName()
							+ L" -> " + pVirtClass->GetFullClassName() + L" : "
							+ GetErrorMessage(exceptionNoVirtualVector)).c_str() ) ) ) ;

	micro_instruction_Throw() ;
}

// Call
void LContext::micro_instruction_Call( LFunctionObj * pFunc )
{
	assert( pFunc != nullptr ) ;
	if ( pFunc == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return ;
	}

	// 関数呼び出し
	if ( pFunc->IsNativeFunc() )
	{
		// ネイティブ関数呼び出し
		pFunc->CallNativeFunc( *this ) ;

		m_interrupt |= (m_status != statusRunning) ;
	}
	else if ( pFunc->GetCodeBuffer() != nullptr )
	{
		// キャプチャー・オブジェクトをプッシュ
		for ( size_t i = 0; i < pFunc->GetCaptureObjectCount(); i ++ )
		{
			LValue	valCapture = pFunc->GetCaptureObjectAt( i ) ;
			m_stack->Push( valCapture.Value() ) ;
		}

		// 戻りアドレスをプッシュ
		m_stack->Push( LValue::MakeLong( m_ip ) ) ;
		m_stack->Push( LValue::MakeVoidPtr( m_pCodeBuf ) ) ;

		// 関数アドレスへジャンプ
		m_pCodeBuf = pFunc->GetCodeBuffer().get() ;
		m_ip = pFunc->GetCodePoint() ;

		m_interrupt = true ;
	}
	else
	{
		micro_instruction_ThrowUnimplemented( pFunc ) ;
		return ;
	}
}

// ポインタアドレスを取得
std::uint8_t * LContext::micro_instruction_FetchAddr
			( size_t ptr, ssize_t off, size_t range, std::uint8_t type )
{
	if ( ptr >= m_stack->m_sp )
	{
		micro_instruction_ThrowIndexOutOfBounds() ;
		return	nullptr ;
	}
	LObject *	pObj = m_stack->At(ptr).pObject ;
	if ( pObj == nullptr )
	{
		if ( type != 0 )
		{
			micro_instruction_ThrowNullException() ;
		}
		return	nullptr ;
	}
	LPtr<LPointerObj>	pPtr = pObj->GetBufferPoiner() ;
	if ( pPtr == nullptr )
	{
		micro_instruction_ThrowNullException() ;
		return	nullptr ;
	}
	std::uint8_t *	pBuf = pPtr->GetPointer( off, range ) ;
	if ( pBuf == nullptr )
	{
		micro_instruction_ThrowPointerOutOfBounds() ;
		return	nullptr ;
	}
	return	pBuf ;
}

// ポインタ・アライメント・チェック
LValue::Primitive
	LContext::micro_instruction_CheckAlignment
		( std::uint8_t * ptr, std::uint8_t align )
{
	LValue::Primitive	val = LValue::MakeVoidPtr( ptr ) ;

	if ( (val.longValue & (align - 1)) && (align != 0) )
	{
		micro_instruction_ThrowAlignmentMismatch() ;
	}
	return	val ;
}


// データ丸め関数（ロード・ストア）
//////////////////////////////////////////////////////////////////////////////
const LContext::PFN_MoveStackWord	LContext::s_pfnLoadLocal[LType::typeAllCount] =
{
	&LContext::load_stack_bool,
	&LContext::load_stack_int8,
	&LContext::load_stack_uint8,
	&LContext::load_stack_int16,
	&LContext::load_stack_uint16,
	&LContext::load_stack_int32,
	&LContext::load_stack_uint32,
	&LContext::load_stack_int64,
	&LContext::load_stack_uint64,
	&LContext::load_stack_float,
	&LContext::load_stack_double,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
	&LContext::load_stack_ptr,
} ;

const LContext::PFN_MoveStackWord	LContext::s_pfnStoreLocal[LType::typeAllCount] =
{
	&LContext::store_stack_bool,
	&LContext::store_stack_int8,
	&LContext::store_stack_uint8,
	&LContext::store_stack_int16,
	&LContext::store_stack_uint16,
	&LContext::store_stack_int32,
	&LContext::store_stack_uint32,
	&LContext::store_stack_int64,
	&LContext::store_stack_uint64,
	&LContext::store_stack_float,
	&LContext::store_stack_double,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
	&LContext::store_stack_ptr,
} ;

LValue::Primitive LContext::load_stack_bool( const LValue::Primitive& val )
{
	return	LValue::MakeBool( val.boolValue ) ;
}

LValue::Primitive LContext::load_stack_int8( const LValue::Primitive& val )
{
	return	LValue::MakeLong( *((LInt8*) &val) ) ;
}

LValue::Primitive LContext::load_stack_uint8( const LValue::Primitive& val )
{
	return	LValue::MakeLong( *((LUint8*) &val) ) ;
}

LValue::Primitive LContext::load_stack_int16( const LValue::Primitive& val )
{
	return	LValue::MakeLong( *((LInt16*) &val) ) ;
}

LValue::Primitive LContext::load_stack_uint16( const LValue::Primitive& val )
{
	return	LValue::MakeLong( *((LUint16*) &val) ) ;
}

LValue::Primitive LContext::load_stack_int32( const LValue::Primitive& val )
{
	return	LValue::MakeLong( *((LInt32*) &val) ) ;
}

LValue::Primitive LContext::load_stack_uint32( const LValue::Primitive& val )
{
	return	LValue::MakeLong( *((LUint32*) &val) ) ;
}

LValue::Primitive LContext::load_stack_int64( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::load_stack_uint64( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::load_stack_float( const LValue::Primitive& val )
{
	return	LValue::MakeDouble( val.flValue ) ;
}

LValue::Primitive LContext::load_stack_double( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::load_stack_ptr( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_bool( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_int8( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_uint8( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_int16( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_uint16( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_int32( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_uint32( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_int64( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_uint64( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_float( const LValue::Primitive& val )
{
	LValue::Primitive	st ;
	st.flValue = (LFloat) val.dblValue ;
	return	st ;
}

LValue::Primitive LContext::store_stack_double( const LValue::Primitive& val )
{
	return	val ;
}

LValue::Primitive LContext::store_stack_ptr( const LValue::Primitive& val )
{
	return	val ;
}


// ロード関数
//////////////////////////////////////////////////////////////////////////////
const LContext::PFN_LoadFromPointer	LContext::s_pfnLoadFromPointer[LType::typeAllCount] =
{
	&LContext::load_ptr_bool,
	&LContext::load_ptr_int8,
	&LContext::load_ptr_uint8,
	&LContext::load_ptr_int16,
	&LContext::load_ptr_uint16,
	&LContext::load_ptr_int32,
	&LContext::load_ptr_uint32,
	&LContext::load_ptr_int64,
	&LContext::load_ptr_uint64,
	&LContext::load_ptr_float,
	&LContext::load_ptr_double,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
	&LContext::load_ptr_ptr,
} ;

LValue::Primitive LContext::load_ptr_bool( const void * ptr )
{
	return	LValue::MakeBool( *((LBoolean*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_int8( const void * ptr )
{
	return	LValue::MakeLong( *((LInt8*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_uint8( const void * ptr )
{
	return	LValue::MakeLong( *((LUint8*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_int16( const void * ptr )
{
	return	LValue::MakeLong( *((LInt16*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_uint16( const void * ptr )
{
	return	LValue::MakeLong( *((LUint16*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_int32( const void * ptr )
{
	return	LValue::MakeLong( *((LInt32*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_uint32( const void * ptr )
{
	return	LValue::MakeLong( *((LUint32*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_int64( const void * ptr )
{
	return	LValue::MakeLong( *((LInt64*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_uint64( const void * ptr )
{
	return	LValue::MakeLong( (LLong) *((LUint64*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_float( const void * ptr )
{
	return	LValue::MakeDouble( *((LFloat*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_double( const void * ptr )
{
	return	LValue::MakeDouble( *((LDouble*)ptr) ) ;
}

LValue::Primitive LContext::load_ptr_ptr( const void * ptr )
{
	return	LValue::MakeVoidPtr( *((void**) ptr) ) ;
}


// ストア関数
//////////////////////////////////////////////////////////////////////////////
const LContext::PFN_StoreIntoPointer	LContext::s_pfnStoreIntoPointer[LType::typeAllCount] =
{
	&LContext::store_ptr_bool,
	&LContext::store_ptr_int8,
	&LContext::store_ptr_uint8,
	&LContext::store_ptr_int16,
	&LContext::store_ptr_uint16,
	&LContext::store_ptr_int32,
	&LContext::store_ptr_uint32,
	&LContext::store_ptr_int64,
	&LContext::store_ptr_uint64,
	&LContext::store_ptr_float,
	&LContext::store_ptr_double,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
	&LContext::store_ptr_ptr,
} ;

void LContext::store_ptr_bool( void * ptr, const LValue::Primitive& val )
{
	*((LBoolean*)ptr) = val.boolValue ;
}

void LContext::store_ptr_int8( void * ptr, const LValue::Primitive& val )
{
	*((LInt8*)ptr) = (LInt8) val.longValue ;
}

void LContext::store_ptr_uint8( void * ptr, const LValue::Primitive& val )
{
	*((LUint8*)ptr) = (LUint8) val.longValue ;
}

void LContext::store_ptr_int16( void * ptr, const LValue::Primitive& val )
{
	*((LInt16*)ptr) = (LInt16) val.longValue ;
}

void LContext::store_ptr_uint16( void * ptr, const LValue::Primitive& val )
{
	*((LUint16*)ptr) = (LUint16) val.longValue ;
}

void LContext::store_ptr_int32( void * ptr, const LValue::Primitive& val )
{
	*((LInt32*)ptr) = (LInt32) val.longValue ;
}

void LContext::store_ptr_uint32( void * ptr, const LValue::Primitive& val )
{
	*((LUint32*)ptr) = (LUint32) val.longValue ;
}

void LContext::store_ptr_int64( void * ptr, const LValue::Primitive& val )
{
	*((LInt64*)ptr) = val.longValue ;
}

void LContext::store_ptr_uint64( void * ptr, const LValue::Primitive& val )
{
	*((LUint64*)ptr) = val.longValue ;
}

void LContext::store_ptr_float( void * ptr, const LValue::Primitive& val )
{
	*((LFloat*)ptr) = (LFloat) val.dblValue ;
}

void LContext::store_ptr_double( void * ptr, const LValue::Primitive& val )
{
	*((LDouble*)ptr) = val.dblValue ;
}

void LContext::store_ptr_ptr( void * ptr, const LValue::Primitive& val )
{
	*((void**)ptr) = val.pVoidPtr ;
}


