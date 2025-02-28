
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Task オブジェクト
//////////////////////////////////////////////////////////////////////////////

LTaskObj::LTaskObj( LClass * pClass )
	: LGenericObj( pClass ),
		m_context( pClass->VM() ),
		m_flagAsync( false ), m_flagFinished( false )
{
}

LTaskObj::LTaskObj( const LTaskObj& obj )
	: LGenericObj( obj ),
		m_context( obj.GetClass()->VM() ),
		m_flagAsync( false ), m_flagFinished( false )
{
}

// 複製する（要素も全て複製処理する）
LObject * LTaskObj::CloneObject( void ) const
{
	return	new LTaskObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LTaskObj::DuplicateObject( void ) const
{
	return	new LTaskObj( *this ) ;
}

// 関数実行（同期）
std::tuple<LValue,LObjPtr>
	LTaskObj::SyncCallFunction
		( LFunctionObj * pFunc,
			const LValue * pArgValues, size_t nArgCount )
{
	LTaskObj *	pPrevTask = GetCurrent() ;
	SetCurrent( this ) ;

	std::tuple<LValue,LObjPtr>	res =
		m_context.SyncCallFunction( pFunc, pArgValues, nArgCount ) ;

	SetCurrent( pPrevTask ) ;
	return	res ;
}

std::tuple<LValue,LObjPtr>
	LTaskObj::SyncCallFunctionAs
		( LObjPtr pThisObj,
			const wchar_t * pwszFuncName,
			const LValue * pArgValues, size_t nArgCount )
{
	std::vector<LValue>	argValues ;
	LPtr<LFunctionObj>	pFunc =
		GetFunctionAs( argValues, pThisObj,
						pwszFuncName, pArgValues, nArgCount ) ;
	if ( pFunc == nullptr )
	{
		return	std::make_tuple<LValue,LObjPtr>
			( LValue(), LObjPtr(m_context.new_Exception( exceptionUnimplemented)) ) ;
	}

	return	SyncCallFunction
				( pFunc.Ptr(), argValues.data(), argValues.size() ) ;
}

// 関数取得
LPtr<LFunctionObj> LTaskObj::GetFunctionAs
		( std::vector<LValue>& argValues,
			const LObjPtr& pThisObj,
			const wchar_t * pwszFuncName,
			const LValue * pArgValues, size_t nArgCount ) const
{
	if ( pThisObj != nullptr )
	{
		argValues.push_back( LValue( pThisObj ) ) ;
	}

	LArgumentListType	argListType ;
	for ( size_t i = 0; i < nArgCount; i ++ )
	{
		argListType.push_back( pArgValues[i].GetType() ) ;
		argValues.push_back( pArgValues[i] ) ;
	}

	LPtr<LFunctionObj>	pFunc ;
	if ( pThisObj != nullptr )
	{
		LClass *	pClass = pThisObj->GetClass() ;
		assert( pClass != nullptr ) ;
		const ssize_t	iFunc =
			pClass->GetVirtualVector().FindCallableFunction
								( pwszFuncName, argListType, pClass ) ;
		if ( iFunc < 0 )
		{
			return	LPtr<LFunctionObj>() ;
		}
		return	pClass->GetVirtualVector().GetFunctionAt( (size_t) iFunc ) ;
	}
	else
	{
		const LFunctionVariation *	pFuncVar =
			m_context.VM().Global()->GetLocalStaticFunctionsAs( pwszFuncName ) ;
		if ( pFuncVar == nullptr )
		{
			return	LPtr<LFunctionObj>() ;
		}
		return	pFuncVar->GetCallableFunction( argListType ) ;
	}
}

// 軽量スレッドとして実行を開始する
bool LTaskObj::BeginAsync
	( const LPtr<LFunctionObj>& pFunc, const LValue * pArgValues, size_t nArgCount )
{
	assert( !m_context.IsRunning() ) ;
	m_flagAsync = true ;
	m_flagFinished = false ;
	m_pFunc = pFunc ;

	LTaskObj *	pPrevTask = GetCurrent() ;
	SetCurrent( this ) ;

	if ( m_context.AsyncCallFunction
			( m_state, pFunc.Ptr(), pArgValues, nArgCount ) )
	{
		OnFinished() ;
	}

	SetCurrent( pPrevTask ) ;
	return	m_flagFinished ;
}

bool LTaskObj::BeginAsyncAs
	( const LObjPtr& pThisObj,
		const wchar_t * pwszFuncName,
		const LValue * pArgValues, size_t nArgCount )
{
	std::vector<LValue>	argValues ;
	LPtr<LFunctionObj>	pFunc =
		GetFunctionAs( argValues, pThisObj,
						pwszFuncName, pArgValues, nArgCount ) ;
	if ( pFunc == nullptr )
	{
		m_state.exception.SetPtr( m_context.new_Exception( exceptionNullPointer ) ) ;
		return	true ;
	}
	return	BeginAsync( pFunc, argValues.data(), argValues.size() ) ;
}

// 軽量スレッドとして実行を継続する
bool LTaskObj::AsyncProceed( std::int64_t msecTimeout )
{
	if ( !m_flagFinished )
	{
		LTaskObj *	pPrevTask = GetCurrent() ;
		SetCurrent( this ) ;

		if ( m_context.AsyncProceed( m_state, msecTimeout ) )
		{
			OnFinished() ;
		}
		SetCurrent( pPrevTask ) ;
	}
	return	m_flagFinished ;
}

// 完了時の処理
void LTaskObj::OnFinished( void )
{
	m_flagFinished = true ;
	m_cvFinished.notify_all() ;
}

// タスクは実行中か？
bool LTaskObj::IsRunning( void ) const
{
	return	m_context.IsRunning() ;
}

// スレッドは完了したか？
bool LTaskObj::IsFinished( void ) const
{
	return	m_flagFinished ;
}

// スレッド完了を待つ
bool LTaskObj::WaitForThread( std::int64_t msecTimeout )
{
	std::unique_lock<std::mutex>	lock( m_mutex ) ;
	if ( !m_flagFinished )
	{
		if ( msecTimeout < 0 )
		{
			m_cvFinished.wait( lock, [this](){ return m_flagFinished ; } ) ;
		}
		else
		{
			m_cvFinished.wait_for( lock, std::chrono::milliseconds(msecTimeout) ) ;
		}
	}
	return	m_flagFinished ;
}

// 返り値取得
const LValue& LTaskObj::GetReturnedValue( void ) const
{
	assert( m_flagFinished ) ;
	return	m_state.valRet ;
}

// ハンドルされなかった例外取得
LObjPtr LTaskObj::GetUnhandledException( void ) const
{
	assert( m_flagFinished ) ;
	return	m_state.exception ;
}

// 例外を送出する
void LTaskObj::ThrowException( LObjPtr pObj )
{
	m_context.ThrowException( pObj ) ;
}

// 即座に強制終了させる
//（通常スレッドで別スレッドから呼び出した場合には
//  完全なスレッドの終了は WaitForThread() を呼び出す必要がある）
void LTaskObj::ForceTermination( void )
{
	if ( !m_flagFinished )
	{
		m_context.SetInterruptSignal( LContext::interruptAbort ) ;

		if ( m_flagAsync )
		{
			AsyncProceed( 0 ) ;
		}
	}
}

// 一時的に実行を停止する
std::unique_lock<std::recursive_mutex> LTaskObj::LockRunning( void )
{
	m_context.SetInterruptSignal( LContext::interruptBreak ) ;

	return	m_context.LockRunning() ;
}

#if !defined(_DLL_IMPORT_LOQUATY)

static thread_local LTaskObj *	t_pCurrent = nullptr ;

// 現在のスレッドを取得する
LOQUATY_DLL_DECL(LTaskObj *) LTaskObj::GetCurrent( void )
{
	return	t_pCurrent ;
}

// 現在のスレッドを設定する
LOQUATY_DLL_DECL(void) LTaskObj::SetCurrent( LTaskObj * pCurrent )
{
	t_pCurrent = pCurrent ;
}

#endif

// boolean begin( Function<Object()> func )
void LTaskObj::method_begin( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;
	LQT_FUNC_ARG_OBJECT( LFunctionObj, pFunc ) ;
	LQT_VERIFY_NULL_PTR( pFunc ) ;

	LQT_RETURN_BOOL( pTaskObj->BeginAsync( pFunc ) ) ;
}

// boolean proceed( long timeout = 0 )
void LTaskObj::method_proceed( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;
	LQT_FUNC_ARG_LONG( msecTimeout ) ;

	LQT_RETURN_BOOL
		( pTaskObj->AsyncProceed
			( std::min( msecTimeout, (std::int64_t) 100 ) ) ) ;
}

// boolean isFinished() const
void LTaskObj::method_isFinished( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;

	LQT_RETURN_BOOL( pTaskObj->IsFinished() ) ;
}

// boolean waitFor()
void LTaskObj::method_waitFor0( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;

	if ( pTaskObj->IsFinished() )
	{
		LQT_RETURN_BOOL( true ) ;
	}
	std::shared_ptr<LTimeoutRemaining>
			remaining = std::make_shared<LTimeoutRemaining>( -1 ) ;
	_context.SetAwaiting
		( [&_context,pTaskObj,remaining]( std::int64_t msecTimeout )
		{
			if ( remaining->IsFinished() )
			{
				return	true ;
			}
			if ( pTaskObj->WaitForThread( remaining->Remaining( msecTimeout ) ) )
			{
				remaining->Finish() ;
				_context.SetReturnValue
					( LValue(LType::typeBoolean, LValue::MakeBool(true) ) ) ;
				return	true ;
			}
			return	false ;
		} ) ;

	LQT_RETURN_BOOL( false ) ;
}

// boolean waitFor( long msecTimeout )
void LTaskObj::method_waitFor1( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;
	LQT_FUNC_ARG_LONG( msecTimeout ) ;

	if ( pTaskObj->IsFinished() )
	{
		LQT_RETURN_BOOL( true ) ;
	}

	std::shared_ptr<LTimeoutRemaining>
			remaining = std::make_shared<LTimeoutRemaining>( msecTimeout ) ;
	_context.SetAwaiting
		( [&_context,pTaskObj,remaining]( std::int64_t msecTimeout )
		{
			if ( remaining->IsFinished() )
			{
				return	true ;
			}
			if ( pTaskObj->WaitForThread( remaining->Remaining( msecTimeout ) ) )
			{
				remaining->Finish() ;
				_context.SetReturnValue
					( LValue(LType::typeBoolean, LValue::MakeBool(true) ) ) ;
				return	true ;
			}
			return	remaining->IsTimeout() ;
		} ) ;

	LQT_RETURN_BOOL( false ) ;
}

// Object getReturned() const
void LTaskObj::method_getReturned( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;

	LTaskClass *	pTaskClass =
			dynamic_cast<LTaskClass*>( pTaskObj->GetClass() ) ;
	if ( (pTaskClass != nullptr)
		&& pTaskClass->GetReturnType().IsObject() )
	{
		const LValue&	valRet = pTaskObj->GetReturnedValue() ;
		if ( valRet.GetType().IsPrimitive() )
		{
			if ( valRet.GetType().IsFloatingPointNumber() )
			{
				_context.SetReturnValue
					( LValue( LObjPtr
						( _context.new_Double( valRet.Value().dblValue ) ) ) ) ;
			}
			else
			{
				_context.SetReturnValue
					( LValue( LObjPtr
						( _context.new_Integer( valRet.Value().longValue ) ) ) ) ;
			}
		}
		else
		{
			_context.SetReturnValue( pTaskObj->GetReturnedValue() ) ;
		}
	}
	else
	{
		_context.SetReturnValue( pTaskObj->GetReturnedValue() ) ;
	}
}

// Object getUnhandledException() const
void LTaskObj::method_getUnhandledException( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;

	LQT_RETURN_OBJECT( pTaskObj->GetUnhandledException() ) ;
}

// void finish()
void LTaskObj::method_finish( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;

	pTaskObj->ForceTermination() ;

	LQT_RETURN_VOID() ;
}

// void throw( Object obj )
void LTaskObj::method_throw( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;
	LQT_FUNC_ARG_OBJECT( LObject, pObj ) ;

	pTaskObj->ThrowException( pObj ) ;

	LQT_RETURN_VOID() ;
}

// static Task getCurrent()
void LTaskObj::method_getCurrent( LContext& _context )
{
	LTaskObj *	pTaskObj = GetCurrent() ;

	LObject::AddRef( pTaskObj ) ;

	LQT_RETURN_OBJECT( LObjPtr( pTaskObj ) ) ;
}

// static void rest() １回休み
void LTaskObj::method_rest( LContext& _context )
{
	_context.SetInterruptSignal( LContext::interruptBreak ) ;
}




//////////////////////////////////////////////////////////////////////////////
// Thread オブジェクト
//////////////////////////////////////////////////////////////////////////////

LThreadObj::LThreadObj( LClass * pClass )
	: LTaskObj( pClass ),
		m_pListPrev( nullptr ), m_pListNext( nullptr )
{
}

LThreadObj::LThreadObj( const LThreadObj& obj )
	: LTaskObj( obj ),
		m_pListPrev( nullptr ), m_pListNext( nullptr )
{
}

LThreadObj::~LThreadObj( void )
{
	if ( m_thread != nullptr )
	{
		m_thread->join() ;
	}
}

// 複製する（要素も全て複製処理する）
LObject * LThreadObj::CloneObject( void ) const
{
	return	new LThreadObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LThreadObj::DuplicateObject( void ) const
{
	return	new LThreadObj( *this ) ;
}

// スレッド開始
bool LThreadObj::BeginThread( LPtr<LFunctionObj> pFunc )
{
	assert( m_thread == nullptr ) ;
	assert( !m_context.IsRunning() ) ;

	if ( BeginAsync( pFunc ) )
	{
		return	true ;
	}
	m_flagAsync = false ;
	m_pClass->VM().AttachThread( this ) ;

	m_thread = std::make_unique<std::thread>( [this]()
		{
			while ( !AsyncProceed( 10 ) )
			{
			}
		} ) ;

	return	false ;
}

// 完了時の処理
void LThreadObj::OnFinished( void )
{
	if ( !m_flagAsync )
	{
		m_pClass->VM().DetachThread( this ) ;
	}
	LTaskObj::OnFinished() ;
}

// boolean begin( Function<Object()> func )
void LThreadObj::method_begin( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LThreadObj, pThreadObj ) ;
	LQT_FUNC_ARG_OBJECT( LFunctionObj, pFunc ) ;
	LQT_VERIFY_NULL_PTR( pFunc ) ;

	LQT_RETURN_BOOL( pThreadObj->BeginThread( pFunc ) ) ;
}

// Object getReturned() const
void LThreadObj::method_getReturned( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LTaskObj, pTaskObj ) ;

	LThreadClass *	pThreadClass =
			dynamic_cast<LThreadClass*>( pTaskObj->GetClass() ) ;
	if ( (pThreadClass != nullptr)
		&& pThreadClass->GetReturnType().IsObject() )
	{
		const LValue&	valRet = pTaskObj->GetReturnedValue() ;
		if ( valRet.GetType().IsPrimitive() )
		{
			if ( valRet.GetType().IsFloatingPointNumber() )
			{
				_context.SetReturnValue
					( LValue( LObjPtr
						( _context.new_Double( valRet.Value().dblValue ) ) ) ) ;
			}
			else
			{
				_context.SetReturnValue
					( LValue( LObjPtr
						( _context.new_Integer( valRet.Value().longValue ) ) ) ) ;
			}
		}
		else
		{
			_context.SetReturnValue( pTaskObj->GetReturnedValue() ) ;
		}
	}
	else
	{
		_context.SetReturnValue( pTaskObj->GetReturnedValue() ) ;
	}
}

// static Thread getCurrent()
void LThreadObj::method_getCurrent( LContext& _context )
{
	LThreadObj *	pThreadObj =
			dynamic_cast<LThreadObj*>( GetCurrent() ) ;

	LObject::AddRef( pThreadObj ) ;

	LQT_RETURN_OBJECT( LObjPtr( pThreadObj ) ) ;
}

// static void sleep( long msecTimeout )
void LThreadObj::method_sleep( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_ARG_LONG( msecTimeout ) ;

	std::shared_ptr<LTimeoutRemaining>
			remaining = std::make_shared<LTimeoutRemaining>( msecTimeout ) ;
	_context.SetAwaiting
		( [&_context,remaining]( std::int64_t msecTimeout )
		{
			msecTimeout = remaining->Remaining( msecTimeout ) ;
			if ( msecTimeout > 0 )
			{
				std::mutex						mutex ;
				std::condition_variable			cv ;
				std::unique_lock<std::mutex>	lock( mutex ) ;
				cv.wait_for( lock, std::chrono::milliseconds(msecTimeout) ) ;
			}
			return	remaining->IsTimeout() ;
		} ) ;

	LQT_RETURN_VOID() ;
}


