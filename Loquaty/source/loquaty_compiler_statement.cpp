
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// コンパイラ・文解釈
//////////////////////////////////////////////////////////////////////////////

// 文解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatementList
	( LStringParser& sparsSrc,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	while ( sparsSrc.PassSpace() )
	{
		// 終端判定
		if ( LStringParser::IsCharContained
					( sparsSrc.CurrentChar(), pwszEscChars ) )
		{
			break ;
		}

		// １文解釈
		ParseOneStatement( sparsSrc, pnslLocal ) ;
		ClearErrorOnCurrent() ;
	}
}

void LCompiler::ParseOneStatement
	( LStringParser& sparsSrc, const LNamespaceList * pnslLocal )
{
	assert( (m_ctx == nullptr) || (m_ctx->m_xvaStack.GetLength() == 0) ) ;

	if ( !sparsSrc.PassSpace()
		|| (sparsSrc.HasNextChars( L";" ) == L';') )
	{
		// 空の文
		return ;
	}

	OnBeginStatement( sparsSrc ) ;

	if ( (m_ctx != nullptr)
		&& (sparsSrc.HasNextChars( L"{" ) == L'{') )
	{
		// 複文
		CodeNestPtr	pNest = PushNest( CodeNest::ctrlStatementList ) ;

		ParseStatementList( sparsSrc, pnslLocal, L"}" ) ;

		if ( sparsSrc.HasNextChars( L"}" ) != L'}' )
		{
			OnError( errorMismatchBraces ) ;
		}
		PopNest( pNest ) ;
		return ;
	}

	LString		strToken ;
	LStringParser::TokenType
				typeToken = sparsSrc.NextToken( strToken ) ;

	Symbol::ReservedWordIndex
				rwIndex = AsReservedWordIndex( strToken.c_str() ) ;
	if ( rwIndex != Symbol::rwiInvalid )
	{
		// 予約語から始まる文
		(this->*s_pfnParseStatement[rwIndex])( sparsSrc, rwIndex, pnslLocal ) ;
		ExprCodeFreeTempStack() ;
		OnEndStatement( sparsSrc, rwIndex ) ;
		return ;
	}

	if ( (typeToken == LStringParser::tokenSymbol)
		&& sparsSrc.HasNextToken( L":" ) )
	{
		// ラベル
		PutLabel( strToken ) ;
		OnEndStatement( sparsSrc, rwIndex ) ;
		return ;
	}

	// 型名から始まる場合は宣言文、そうでないなら式文
	sparsSrc.SeekIndex( m_iSrcStatement ) ;
	ParseDefinitionOrExpression( sparsSrc, pnslLocal ) ;
	ExprCodeFreeTempStack() ;
	OnEndStatement( sparsSrc, rwIndex ) ;

	assert( (m_ctx == nullptr) || (m_ctx->m_xvaStack.GetLength() == 0) ) ;
}

// 文開始処理
///////////////////////////////////////////////////////////////////////////////
void LCompiler::OnBeginStatement( LStringParser& sparsSrc )
{
	ClearErrorOnCurrent() ;

	m_iSrcStatement = sparsSrc.GetIndex() ;
	if ( (m_ctx != nullptr) && (m_ctx->m_codeBuf != nullptr) )
	{
		m_ctx->m_iCurStCode = m_ctx->m_codeBuf->m_buffer.size() ;
	}

	LSourceFile*	pSource = dynamic_cast<LSourceFile*>( &sparsSrc ) ;
	if ( pSource != nullptr )
	{
		m_commentBefore = pSource->GetCommentBefore() ;
	}
	else
	{
		m_commentBefore = L"" ;
	}
}

// 文終了時処理
///////////////////////////////////////////////////////////////////////////////
void LCompiler::OnEndStatement
	( LStringParser& sparsSrc, Symbol::ReservedWordIndex rwIndex )
{
	ExprCodeFreeTempStack() ;
	if ( (m_ctx != nullptr) && (m_ctx->m_xvaStack.size() > 0) )
	{
		OnError( errorAssertMismatchStack ) ;
	}
	if ( rwIndex != Symbol::rwiReturn )
	{
		if ( (m_ctx != nullptr) && (m_ctx->m_codeBuf != nullptr) )
		{
			if ( m_ctx->m_iCurStCode < m_ctx->m_codeBuf->m_buffer.size() )
			{
				if ( m_ctx->m_curNest != nullptr )
				{
					m_ctx->m_curNest->m_returned = false ;
				}
			}
		}
	}
	if ( (m_ctx != nullptr) && (m_ctx->m_codeBuf != nullptr) )
	{
		if ( m_ctx->m_iCurStCode < m_ctx->m_codeBuf->m_buffer.size() )
		{
			LCodeBuffer::DebugSourceInfo	dbsi ;
			dbsi.m_iSrcFirst = m_iSrcStatement ;
			dbsi.m_iSrcEnd = sparsSrc.GetIndex() ;
			dbsi.m_iCodeFirst = m_ctx->m_iCurStCode ;
			dbsi.m_iCodeEnd = m_ctx->m_codeBuf->m_buffer.size() ;
			//
			m_ctx->m_codeBuf->AddDebugSourceInfo( dbsi ) ;
		}
	}
}

// 関数ブロック開始
///////////////////////////////////////////////////////////////////////////////
LCompiler::ContextPtr LCompiler::BeginFunctionBlock
	( std::shared_ptr<LPrototype> proto,
		std::shared_ptr<LCodeBuffer> codeBuf,
		const wchar_t * pwszFuncName, CodeNest::ControlType typeBaseNest )
{
	ContextPtr	ctx = std::make_shared<Context>() ;
	ctx->m_prev = m_ctx ;
	m_ctx = ctx ;

	m_ctx->m_name = pwszFuncName ;
	m_ctx->m_proto = proto ;
	m_ctx->m_codeBuf = codeBuf ;

	if ( (codeBuf->m_pSourceFile == nullptr) && (m_pSource != nullptr) )
	{
		m_ctx->m_codeBuf->AttachSourceFile
			( m_vm.SourceProducer().GetSafeSource( m_pSource ) ) ;
	}

	CodeNestPtr			pNest = PushNest( typeBaseNest ) ;
	LLocalVarArrayPtr	frame = pNest->MakeFrame() ;

	// this オブジェクト／ポインタ情報
	std::vector<LLocalVarPtr>	args ;
	LLocalVarPtr				pThis ;
	if ( proto->GetThisClass() != nullptr )
	{
		LType	typeThis =
			GetLocalTypeOf( LType(proto->GetThisClass()), proto->IsConstThis() ) ;
		pThis = frame->AddLocalVar( L"this", typeThis ) ;

		pThis->SetReadOnly() ;
		if ( pThis->GetAllocType() == LLocalVar::allocPointer )
		{
			LLocalVarPtr	pVarOffset =
				frame->GetLocalVarAt( pThis->GetLocation() + 2 ) ;
			if ( pVarOffset != nullptr )
			{
				pVarOffset->SetReadOnly() ;
			}
		}

		args.push_back( pThis ) ;
	}

	// 引数情報をフレーム上に追加
	for ( size_t i = 0; i < proto->GetArgListType().size(); i ++ )
	{
		LType	typeArg =
			GetLocalTypeOf( proto->GetArgListType().GetArgTypeAt(i) ) ;
		args.push_back
			( frame->AddLocalVar
				( proto->GetArgListType().GetArgNameAt(i), typeArg ) ) ;
	}

	// キャプチャーオブジェクト情報をフレーム上に追加
	for ( size_t i = 0; i < proto->GetCaptureObjectCount(); i ++ )
	{
		LType	typeArg =
			GetLocalTypeOf( proto->GetCaptureObjListTypes().GetArgTypeAt(i) ) ;
		args.push_back
			( frame->AddLocalVar
				( proto->GetCaptureObjListTypes().GetArgNameAt(i), typeArg ) ) ;
	}

	// ローカルフレームを確保する命令生成
	AddCode( LCodeBuffer::codeEnterFunc, 0, 0, 0, frame->GetUsedSize() ) ;

	for ( size_t i = 0; i < args.size(); i ++ )
	{
		LCodeBuffer::ImmediateOperand	immop ;
		LLocalVarPtr		arg = args.at(i) ;
		LType::Primitive	type = arg->GetType().GetPrimitive() ;

		AddCode( LCodeBuffer::codeLoadArg, 0, 0, 0, i ) ;

		if ( arg->HasDestructChain() )
		{
			AddCode( LCodeBuffer::codeRefObject, 0 ) ;
		}

		immop.value.longValue = arg->GetLocation() ;
		AddCode( LCodeBuffer::codeStoreLocal, 0, 0, type, 1, &immop ) ;

		ExprCodeTreatLocalObjectChain( arg ) ;
	}

	// synchronized 実行
	if ( (pThis != nullptr)
		&& (proto->GetModifiers() & LType::modifierSynchronized) )
	{
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = pThis->GetLocation() ;
		AddCode( LCodeBuffer::codeSynchronize, 0, 0, 0, 0, &immop ) ;

		frame->AddUsedSize( 2 ) ;
	}

	ctx->m_iBeginFunc = codeBuf->GetCodeSize() ;

	return	ctx ;
}

// 関数ブロック終了
///////////////////////////////////////////////////////////////////////////////
void LCompiler::EndFunctionBlock( ContextPtr ctx )
{
	assert( ctx == m_ctx ) ;
	assert( m_ctx->m_curNest != nullptr ) ;
	assert( m_ctx->m_curNest->m_prev == nullptr ) ;

	// 関数 return 文の存在判定
	if ( (m_ctx != nullptr)
		&& (m_ctx->m_curNest != nullptr)
		&& !(m_ctx->m_curNest->m_returned) )
	{
		if ( !m_ctx->m_proto->GetReturnType().IsVoid() )
		{
			OnError( errorNothingReturn, m_ctx->m_name.c_str() ) ;
		}
	}

	// ラベル参照解決
	for ( auto fjp : m_ctx->m_jumpForwards )
	{
		auto	iLabel = m_ctx->m_mapLabels.find( fjp.m_strDstLabel.c_str() ) ;
		if ( iLabel == m_ctx->m_mapLabels.end() )
		{
			m_iSrcStatement = fjp.m_cpJump->m_src ;
			OnError( errorNotFoundLabel, fjp.m_strDstLabel.c_str() ) ;
		}
		else
		{
			FixJumpDestination( fjp.m_cpJump, iLabel->second.m_cpLabel ) ;
		}
	}

	// fetch_addr 反映
	for ( auto fav : m_ctx->m_fetchVars )
	{
		LCodeBuffer::Word *	pWord = GetCodeAt( fav.m_cpFetch ) ;
		if ( pWord != nullptr )
		{
			assert( (pWord->code == LCodeBuffer::codeLoadFetchAddr)
					|| (pWord->code == LCodeBuffer::codeLoadFetchAddrOffset)
					|| (pWord->code == LCodeBuffer::codeLoadFetchLAddr) ) ;
			if ( (pWord->code == LCodeBuffer::codeLoadFetchAddr)
					|| (pWord->code == LCodeBuffer::codeLoadFetchAddrOffset)
					|| (pWord->code == LCodeBuffer::codeLoadFetchLAddr) )
			{
				if ( fav.m_pVar->GetMaxFetchPointerRange() > 0x7FFFFFFF )
				{
					OnError( errorTooHugeToRefFetchAddr ) ;
				}
				pWord->imm = (std::int32_t) fav.m_pVar->GetMaxFetchPointerRange() ;
			}
		}
	}
	PopNest( m_ctx->m_curNest ) ;
	assert( m_ctx->m_curNest == nullptr ) ;

	// return
	AddCode( LCodeBuffer::codeLeaveFunc ) ;
	AddCode( LCodeBuffer::codeReturn,
				0, 0, 0, m_ctx->m_proto->GetCaptureObjectCount() ) ;

	// デバッグ情報
	assert( m_ctx->m_codeBuf != nullptr ) ;
	for ( const DebugLocalVarInfo& dlvi : m_ctx->m_dbgVarInfos )
	{
		m_ctx->m_codeBuf->AddDebugLocalVarInfo
			( dlvi.m_varInfo, dlvi.m_cpFirst->m_pos, dlvi.m_cpEnd->m_pos ) ;
	}

	m_ctx = ctx->m_prev ;
	ctx->Release() ;
}

// 名前空間ブロック開始
///////////////////////////////////////////////////////////////////////////////
LCompiler::ContextPtr
	LCompiler::BeginNamespaceBlock
		( LPtr<LNamespace> pNamespace, CodeNest::ControlType typeBaseNest )
{
	// void(void) 関数
	std::shared_ptr<LPrototype>	proto =
		std::make_shared<LPrototype>
			( nullptr, LType::modifierPublic | LType::modifierConst ) ;

	ContextPtr	ctx =
		BeginFunctionBlock
			( proto, std::make_shared<LCodeBuffer>(), nullptr, typeBaseNest ) ;

	assert( ctx->m_curNest != nullptr ) ;
	assert( ctx->m_curNest->m_type == typeBaseNest ) ;

	if ( pNamespace != nullptr )
	{
		m_ctx->m_name = pNamespace->GetName() ;
		ctx->m_curNest->m_namespace = pNamespace ;
	}

	return	ctx ;
}

// 名前空間ブロック終了
///////////////////////////////////////////////////////////////////////////////
void LCompiler::EndNamespaceBlock( LCompiler::ContextPtr ctx )
{
	std::shared_ptr<LPrototype>		proto = ctx->m_proto ;
	std::shared_ptr<LCodeBuffer>	codeBuf = ctx->m_codeBuf ;
	const bool	hasInitCode = (codeBuf->GetCodeSize() != ctx->m_iBeginFunc) ;

	EndFunctionBlock( ctx ) ;

	if ( hasInitCode )
	{
		// 初期化コード実行関数
		LPtr<LFunctionObj>	pInitFunc =
				new LFunctionObj( m_vm.GetFunctionClass(), proto ) ;
		pInitFunc->SetFuncCode( codeBuf, 0 ) ;

		// 初期化コード実行
		LPtr<LThreadObj>	pThread = new LThreadObj( m_vm.GetThreadClass() ) ;
		auto [valRet, pExcept] =
			pThread->SyncCallFunction( pInitFunc.Ptr(), nullptr, 0 ) ;

		OutputExceptionAsError( pExcept ) ;
	}
}

// コード生成を要求可能な入れ子状態を要求（そうでない場合エラーを出力し false）
///////////////////////////////////////////////////////////////////////////////
bool LCompiler::MustBeRuntimeCode( void )
{
	if ( (m_ctx == nullptr)
		|| (m_ctx->m_codeBuf == nullptr)
		|| (m_ctx->m_curNest == nullptr) )
	{
		OnError( errorUnavailableCodeBlock ) ;
		return	false ;
	}
	return	true ;
}

// 名前空間内でなければならない（そうでない場合エラーを出力し false）
///////////////////////////////////////////////////////////////////////////////
bool LCompiler::MustBeInNamespace( void )
{
	if ( GetCurrentNamespace() == nullptr )
	{
		OnError( errorSyntaxErrorInInvalidScope ) ;
		return	false ;
	}
	return	true ;
}

// 名前空間を取得
///////////////////////////////////////////////////////////////////////////////
LPtr<LNamespace> LCompiler::GetCurrentNamespace( void ) const
{
	if ( (m_ctx != nullptr)
		&& (m_ctx->m_curNest != nullptr) )
	{
		return	m_ctx->m_curNest->m_namespace ;
	}
	return	nullptr ;
}

// 文末のセミコロンを読み飛ばす
// セミコロンがない場合、エラーを出力しセミコロンまで読み飛ばす
///////////////////////////////////////////////////////////////////////////////
void LCompiler::HasSemicolonForEndOfStatement( LStringParser& sparsSrc )
{
	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		return ;
	}
	OnError( errorNotFoundSemicolonEnding ) ;
	sparsSrc.PassStatement( L";", L"}" ) ;
}

// 入れ子情報追加
///////////////////////////////////////////////////////////////////////////////
LCompiler::CodeNestPtr LCompiler::PushNest( CodeNest::ControlType type )
{
	CodeNestPtr	pNest = std::make_shared<CodeNest>( type ) ;
	if ( m_ctx == nullptr )
	{
		OnError( errorNotExistCodeContext ) ;
		return	pNest ;
	}
	pNest->m_prev = m_ctx->m_curNest ;
	m_ctx->m_curNest = pNest ;

	if ( m_ctx->m_codeBuf != nullptr )
	{
		pNest->m_cpVarBegin = GetCurrentCodePointer( true ) ;
	}
	return	pNest ;
}

// 入れ子情報削除
///////////////////////////////////////////////////////////////////////////////
LCompiler::CodeNestPtr LCompiler::PopNest( CodeNestPtr pNest )
{
	if ( m_ctx == nullptr )
	{
		OnError( errorNotExistCodeContext ) ;
		return	nullptr ;
	}

	// ローカルスタックの解放
	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	if ( (pNest->m_frame != nullptr)
		&& (pNest->m_frame->GetUsedSize() > 0) )
	{
		AddCode( LCodeBuffer::codeFreeStack,
					0, 0, 0, pNest->m_frame->GetUsedSize() ) ;
	}

	// リターンパスの反映
	assert( m_ctx->m_curNest == pNest ) ;
	if ( pNest->m_prev != nullptr )
	{
		pNest->m_prev->m_returned = pNest->m_returned ;
	}

	// 変数遅延初期化
	for ( auto pDelay : pNest->m_vecDelayInitVars )
	{
		ParseDelayInitVar( *pDelay ) ;
	}

	// 関数遅延実装
	for ( auto pDelay : pNest->m_vecDelayImplements )
	{
		ParseDelayFuncImplement( *pDelay ) ;
	}

	// 入れ子解除
	m_ctx->m_curNest = pNest->m_prev ;

	// break ジャンプ先確定
	CodePointPtr	cpBreakPos ;
	if ( pNest->m_vecBreaks.size() > 0 )
	{
		cpBreakPos = GetCurrentCodePointer( true ) ;
		for ( auto cpBreak : pNest->m_vecBreaks )
		{
			FixJumpDestination( cpBreak, cpBreakPos ) ;
		}
	}

	// continue ジャンプ先確定
	if ( pNest->m_vecContinues.size() > 0 )
	{
		assert( pNest->m_cpContinue != nullptr ) ;
		CodePointPtr	cpContinuePos = pNest->m_cpContinue ;
		if ( cpContinuePos == nullptr )
		{
			OnError( errorAssertJumpCode ) ;
			cpContinuePos = GetCurrentCodePointer( true ) ;
		}
		for ( auto cpContinue : pNest->m_vecContinues )
		{
			FixJumpDestination( cpContinue, cpContinuePos ) ;
		}
	}

	// デバッグ情報
	if ( (m_ctx->m_codeBuf != nullptr)
		&& (pNest->m_frame != nullptr)
		&& (pNest->m_frame->GetUsedSize() > 0) )
	{
		if ( cpBreakPos == nullptr )
		{
			cpBreakPos = GetCurrentCodePointer( true ) ;
		}

		DebugLocalVarInfo	dlvi ;
		dlvi.m_varInfo = pNest->m_frame ;
		dlvi.m_cpFirst = pNest->m_cpVarBegin ;
		dlvi.m_cpEnd = cpBreakPos ;

		dlvi.m_varInfo->DetachParent() ;

		m_ctx->m_dbgVarInfos.push_back( dlvi ) ;
	}

	return	m_ctx->m_curNest ;
}

// ラベルを設置する
///////////////////////////////////////////////////////////////////////////////
void LCompiler::PutLabel( const LString& strLabel )
{
	if ( m_ctx == nullptr )
	{
		OnError( errorNotExistCodeContext ) ;
		return ;
	}
	if ( m_ctx->m_curNest != nullptr )
	{
		m_ctx->m_curNest->m_returned = false ;
	}
	auto	iter = m_ctx->m_mapLabels.find( strLabel.c_str() ) ;
	if ( iter != m_ctx->m_mapLabels.end() )
	{
		OnError( errorDoubleDefinitionOfLabel, strLabel.c_str() ) ;
		return ;
	}
	m_ctx->m_mapLabels.insert
		( std::make_pair<std::wstring,LabelEntry>
			( strLabel.c_str(), LabelEntry( GetCurrentCodePointer( true ) ) ) ) ;
}

// 現在の入れ子でのローカル変数の使用枠を取得する
//////////////////////////////////////////////////////////////////////////////
size_t LCompiler::GetLocalNestVarUsedSize( void )
{
	if ( (m_ctx != nullptr)
		&& (m_ctx->m_curNest != nullptr)
		&& (m_ctx->m_curNest->m_frame != nullptr) )
	{
		return	m_ctx->m_curNest->m_frame->GetUsedSize() ;
	}
	return	0 ;
}

// ローカル変数で定義されたサイズだけスタックを確保する命令を出力する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeAllocLocalFrame( size_t nLastUsed )
{
	if ( !MustBeRuntimeCode() )
	{
		return ;
	}
	size_t	nCurUsed = GetLocalNestVarUsedSize() ;
	assert( nCurUsed >= nLastUsed ) ;
	if ( nCurUsed > nLastUsed )
	{
		AddCode( LCodeBuffer::codeAllocStack, 0, 0, 0, nCurUsed - nLastUsed ) ;
	}
}

// 必要であればローカル変数に yp チェーンを構築するコードを出力する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeTreatLocalObjectChain( LLocalVarPtr pVar )
{
	if ( (pVar != nullptr) && pVar->HasDestructChain() )
	{
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = pVar->GetLocation() + 1 ;

		AddCode( LCodeBuffer::codeMakeObjChain, 0, 0, 0, 0, &immop ) ;
	}
}

// ローカル変数を定義する
//（スタックを確保し、必要であれば yp チェーンを構築力する）
//////////////////////////////////////////////////////////////////////////////
LLocalVarPtr LCompiler::ExprCodeDeclareLocalVar
		( const wchar_t * pwszName, const LType& typeVar )
{
	ExprCodeFreeTempStack() ;
	assert( (m_ctx != nullptr) && (m_ctx->m_xvaStack.size() == 0) ) ;

	const size_t	nLastUsed = GetLocalNestVarUsedSize() ;

	LLocalVarPtr	pVar = DeclareLocalVar( pwszName, typeVar ) ;
	ExprCodeAllocLocalFrame( nLastUsed ) ;

	ExprCodeTreatLocalObjectChain( pVar ) ;

	if ( typeVar.IsStructure() || typeVar.IsDataArray() )
	{
		OnWarning( warningAllocateBufferOnLocal, warning4,
					pwszName, typeVar.GetTypeName().c_str() ) ;
	}

	return	pVar ;
}

// ローカル変数を定義する（情報を登録するだけ）
//////////////////////////////////////////////////////////////////////////////
LLocalVarPtr LCompiler::DeclareLocalVar
		( const wchar_t * pwszName, const LType& typeVar )
{
	assert( m_ctx != nullptr ) ;

	CodeNestPtr	pNest = m_ctx->m_curNest ;
	if ( pNest == nullptr )
	{
		return	nullptr ;
	}
	if ( !VerifyLocalVarDeclarationAs( pwszName, typeVar ) )
	{
		return	nullptr ;
	}
	LLocalVarArrayPtr	frame = pNest->MakeFrame() ;
	return	frame->AddLocalVar( pwszName, typeVar ) ;
}

// 現在の一時スタックをローカルフレームに移し替えて
// ローカル変数に変換する
//////////////////////////////////////////////////////////////////////////////
LLocalVarPtr LCompiler::MakeTempStackIntoLocalVar( LExprValuePtr xval )
{
	assert( m_ctx != nullptr ) ;

	CodeNestPtr	pNest = m_ctx->m_curNest ;
	assert( pNest != nullptr ) ;
	if ( pNest == nullptr )
	{
		return	nullptr ;
	}
	LLocalVarArrayPtr	frame = pNest->MakeFrame() ;
	size_t	nLastUsed = frame->GetUsedSize() ;

	xval = ExprMakeOnStack( std::move(xval) ) ;
	if ( xval->IsFetchAddrPointer() )
	{
		OnError( errorCannotUseFetchAddr ) ;
	}
	if ( !VerifyLocalVarDeclarationTypeAs( nullptr, xval->GetType() ) )
	{
		return	nullptr ;
	}

	ssize_t	iStack = m_ctx->m_xvaStack.Find( xval ) ;
	assert( iStack >= 0 ) ;

	LLocalVarPtr	pVar ;
	frame->AddUsedSize( m_ctx->m_xvaStack.size() ) ;
	if ( iStack >= 0 )
	{
		pVar = frame->PutLocalVar( nLastUsed + iStack, xval->GetType() ) ;
	}
	m_ctx->m_xvaStack.clear() ;

	return	pVar ;
}

LLocalVarPtr LCompiler::MakeTempStackIntoLocalVarAs
				( const wchar_t * pwszName, LExprValuePtr xval )
{
	LLocalVarPtr	pVar = MakeTempStackIntoLocalVar( std::move(xval) ) ;
	if ( pVar == nullptr )
	{
		return	nullptr ;
	}
	assert( m_ctx != nullptr ) ;

	if ( !VerifyLocalVarDeclarationAs( pwszName, pVar->GetType() ) )
	{
		return	nullptr ;
	}
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	assert( pNest != nullptr ) ;

	LLocalVarArrayPtr	frame = pNest->MakeFrame() ;
	frame->SetLocalVarNameAs( pwszName, pVar ) ;

	return	pVar ;
}

// ローカル変数定義妥当性検証
///////////////////////////////////////////////////////////////////////////////
bool LCompiler::VerifyLocalVarDeclarationAs
			( const wchar_t * pwszName, const LType& typeVar )
{
	if ( !MustBeRuntimeCode() )
	{
		return	false ;
	}
	assert( m_ctx != nullptr ) ;

	CodeNestPtr	pNest = m_ctx->m_curNest ;
	assert( pNest != nullptr ) ;

	LLocalVarArrayPtr	frame = pNest->MakeFrame() ;
	if ( frame->GetLocalVarAs( pwszName ) != nullptr )
	{
		OnError( errorDoubleDefinitionOfVar, pwszName ) ;
		return	false ;
	}
	else
	{
		CodeNestPtr	pPrev = pNest->m_prev ;
		while ( pPrev != nullptr )
		{
			if ( (pPrev->m_frame != nullptr)
				&& (pPrev->m_frame->GetLocalVarAs( pwszName ) != nullptr) )
			{
				OnWarning( warningSameNameInDifferentScope, warning3, pwszName ) ;
				break ;
			}
			pPrev = pPrev->m_prev ;
		}
	}
	return	VerifyLocalVarDeclarationTypeAs( pwszName, typeVar ) ;
}

bool LCompiler::VerifyLocalVarDeclarationTypeAs
			( const wchar_t * pwszName, const LType& typeVar )
{
	if ( typeVar.GetModifiers()
			& (LType::accessMask | LType::modifierStatic
				| LType::modifierAbstract | LType::modifierNative
				| LType::modifierSynchronized | LType::modifierOverride) )
	{
		OnWarning( warningInvalidModifierForLocalVar, warning3,
					pwszName, typeVar.GetTypeName().c_str() ) ;
	}
	if ( typeVar.IsFetchAddr() && !typeVar.IsPointer() )
	{
		OnError( errorCannotFetchAddr,
					pwszName, typeVar.GetTypeName().c_str() ) ;
	}
	return	true ;
}

// 有効なユーザー定義名か？
///////////////////////////////////////////////////////////////////////////////
bool LCompiler::IsValidUserName( const wchar_t * pwszName ) const
{
	if ( (pwszName == nullptr) || (pwszName[0] == 0) )
	{
		return	false ;
	}
	if ( LSourceFile::IsCharSpace( pwszName[0] )
		|| LSourceFile::IsCharNumber( pwszName[0] )
		|| LSourceFile::IsPunctuation( pwszName[0] )
		|| LSourceFile::IsSpecialMark( pwszName[0] ) )
	{
		return	false ;
	}
	if ( AsReservedWordIndex( pwszName ) != Symbol::rwiInvalid )
	{
		return	false ;
	}
	if ( AsOperatorIndex( pwszName ) != Symbol::opInvalid )
	{
		return	false ;
	}
	return	true ;
}

// アクセス修飾子フラグを解釈
///////////////////////////////////////////////////////////////////////////////
LType::Modifiers LCompiler::ParseAccessModifiers
	( LStringParser& sparsExpr,
		LType::Modifiers accModAdd, LType::Modifiers accModExclusions )
{
	LString	strToken ;
	while ( sparsExpr.PassSpace() )
	{
		const size_t	iSaveIndex = sparsExpr.GetIndex() ;

		sparsExpr.NextToken( strToken ) ;

		Symbol::ReservedWordIndex
				rwIndex = AsReservedWordIndex( strToken.c_str() ) ;
		if ( rwIndex != Symbol::rwiInvalid )
		{
			LType::Modifiers	accMod = GetModifierOfReservedWord( rwIndex )
														& ~accModExclusions ;
			if ( accMod != 0 )
			{
				accModAdd |= accMod ;
				continue ;
			}
		}
		sparsExpr.SeekIndex( iSaveIndex ) ;
		break ;
	}
	return	accModAdd ;
}

LType::Modifiers LCompiler::GetModifierOfReservedWord
	( Symbol::ReservedWordIndex rwIndex )
{
	if ( Symbol::IsAccessModifier( rwIndex ) )
	{
		return	(LType::Modifiers) GetReservedWordDesc(rwIndex).mapValue ;
	}
	return	0 ;
}

// 関数プロトタイプ引数リストを解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParsePrototypeArgmentList
	( LStringParser& sparsExpr, LPrototype& proto,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	while ( sparsExpr.PassSpace() )
	{
		if ( LStringParser::IsCharContained
				( sparsExpr.CurrentChar(), pwszEscChars ) )
		{
			// 脱出
			break ;
		}

		const size_t	iArg = proto.GetArgListType().size() ;
		if ( iArg > 0 )
		{
			// 2つ目以降の引数は , で区切る
			if ( sparsExpr.HasNextChars( L"," ) != L',' )
			{
				OnError( errorNoSeparationArgmentList ) ;
				return ;
			}
		}

		// type-expr [name [= default-value-expr]]
		LType	typeArg ;
		if ( !sparsExpr.NextTypeExpr( typeArg, true, m_vm, pnslLocal ) )
		{
			return ;
		}
		if ( typeArg.IsVoid() )
		{
			continue ;
		}
		typeArg = GetArgumentTypeOf( typeArg ) ;

		proto.ArgListType().SetArgTypeAt( iArg, typeArg ) ;

		sparsExpr.PassSpace() ;
		if ( LStringParser::IsCharContained
				( sparsExpr.CurrentChar(), pwszEscChars ) )
		{
			proto.ArgListType().SetArgNameAt( iArg, L"" ) ;
			proto.DefaultArgList().push_back( LValue() ) ;
			break ;
		}

		LString		strName ;
		sparsExpr.NextToken( strName ) ;
		if ( !IsValidUserName( strName.c_str() ) )
		{
			OnError( errorConnotAsUserSymbol_opt1, strName.c_str() ) ;
			return ;
		}
		proto.ArgListType().SetArgNameAt( iArg, strName.c_str() ) ;

		if ( sparsExpr.HasNextChars( L"=" ) == L'=' )
		{
			LExprValuePtr	xvalDef =
				EvaluateExpression
					( sparsExpr, pnslLocal, pwszEscChars, Symbol::priorityList ) ;
			xvalDef = EvalCastTypeTo( std::move(xvalDef), typeArg ) ;
			if ( HasErrorOnCurrent() )
			{
				return ;
			}
			if ( (xvalDef == nullptr)
				|| !xvalDef->IsConstExpr() )
			{
				OnError( errorUnavailableConstExpr ) ;
				return ;
			}
			proto.DefaultArgList().push_back( *xvalDef ) ;
		}
		else
		{
			proto.DefaultArgList().push_back( LValue() ) ;
		}
		assert( proto.GetDefaultArgList().size() == proto.GetArgListType().size() ) ;
	}
}

// 非オブジェクト変数の初期値を解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseDataInitExpression
	( LStringParser& sparsExpr,
		const LType& typeData, LPtr<LPointerObj> pPtr,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	assert( typeData.CanArrangeOnBuf() ) ;

	if ( typeData.IsPrimitive() )
	{
		// プリミティブデータ
		LExprValuePtr	xval =
			EvaluateExpression( sparsExpr, pnslLocal, pwszEscChars ) ;
		xval = EvalCastTypeTo( std::move(xval), typeData ) ;
		if ( !xval->IsConstExpr() )
		{
			OnError( errorUnavailableConstExpr ) ;
		}
		if ( HasErrorOnCurrent() )
		{
			return ;
		}
		if ( typeData.IsFloatingPointNumber() )
		{
			pPtr->StoreDoubleAt( 0, typeData.GetPrimitive(), xval->AsDouble() ) ;
		}
		else
		{
			pPtr->StoreIntegerAt( 0, typeData.GetPrimitive(), xval->AsInteger() ) ;
		}
		return ;
	}

	if ( typeData.IsDataArray() )
	{
		// 配列 : [ expr, expr, ... ]
		if ( sparsExpr.HasNextChars( L"[" ) != L'[' )
		{
			OnError( errorInitExpressionError ) ;
			sparsExpr.PassStatement( L"", pwszEscChars ) ;
			return ;
		}
		LDataArrayClass *	pArrayClass = typeData.GetDataArrayClass() ;
		assert( pArrayClass != nullptr ) ;

		LPtr<LPointerObj>	pNext = new LPointerObj( *pPtr );
		const size_t		nLength = pArrayClass->GetArrayElementCount() ;
		const LType			typeElement = pArrayClass->GetElementType() ;
		size_t				iElement = 0 ;
		for ( ; ; )
		{
			if ( sparsExpr.HasNextChars( L"]" ) == L']' )
			{
				break ;
			}
			if ( iElement > 0 )
			{
				if ( sparsExpr.HasNextChars( L"," ) != L',' )
				{
					OnError( errorNoSeparationArrayList ) ;
					sparsExpr.PassStatement( L"]", L";" ) ;
					return ;
				}
				if ( sparsExpr.HasNextChars( L"]" ) == L']' )
				{
					break ;
				}
			}
			if ( iElement >= nLength )
			{
				OnError( errorInitExprErrorTooMuchElement ) ;
				sparsExpr.PassStatement( L"]", L";" ) ;
				break ;
			}
			ParseDataInitExpression
				( sparsExpr, typeElement,
					new LPointerObj( *pNext ), pnslLocal, L",]" ) ;
			if ( HasErrorOnCurrent() )
			{
				sparsExpr.PassStatement( L"]", L";" ) ;
				return ;
			}
			iElement ++ ;
			*pNext += typeElement.GetDataBytes() ;
		}
		return ;
	}

	if ( typeData.IsStructure() )
	{
		// 構造体 : { expr, expr, ... }
		if ( sparsExpr.HasNextChars( L"{" ) != L'{' )
		{
			OnError( errorInitExpressionError ) ;
			sparsExpr.PassStatement( L"", pwszEscChars ) ;
			return ;
		}
		LStructureClass *	pStructClass = typeData.GetStructureClass() ;
		assert( pStructClass != nullptr ) ;

		const LArrangement&		arrange = pStructClass->ProtoArrangemenet() ;
		std::vector<LString>	vMemberNames ;
		arrange.GetOrderedNameList( vMemberNames ) ;

		for ( size_t i = 0; true; i ++ )
		{
			if ( sparsExpr.HasNextChars( L"}" ) == L'}' )
			{
				break ;
			}
			if ( i > 0 )
			{
				if ( sparsExpr.HasNextChars( L"," ) != L',' )
				{
					OnError( errorNoSeparationArrayList ) ;
					sparsExpr.PassStatement( L"}", L";" ) ;
					return ;
				}
				if ( sparsExpr.HasNextChars( L"}" ) == L'}' )
				{
					return ;
				}
			}
			if ( i >= vMemberNames.size() )
			{
				OnError( errorInitExprErrorTooMuchElement ) ;
				sparsExpr.PassStatement( L"}", L";" ) ;
				break ;
			}
			LArrangement::Desc	desc ;
			if ( !arrange.GetDescAs( desc, vMemberNames.at(i).c_str() ) )
			{
				assert( arrange.GetDescAs( desc, vMemberNames.at(i).c_str() ) ) ;
				continue ;
			}
			LPtr<LPointerObj>	pMember = new LPointerObj( *pPtr ) ;
			*pMember += desc.m_location ;

			ParseDataInitExpression
				( sparsExpr, desc.m_type, pMember, pnslLocal, L",}" ) ;
			if ( HasErrorOnCurrent() )
			{
				sparsExpr.PassStatement( L"}", L";" ) ;
				return ;
			}
		}
		return ;
	}

	OnError( errorInitExpressionError ) ;
}

// 予約語解釈
///////////////////////////////////////////////////////////////////////////////
const LCompiler::PFN_ParseStatement
		LCompiler::s_pfnParseStatement[Symbol::rwiReservedWordCount] =
{
	&LCompiler::ParseStatement_import,			// @import
	&LCompiler::ParseStatement_incllude,		// @include
	&LCompiler::ParseStatement_error,			// @error
	&LCompiler::ParseStatement_todo,			// @todo
	&LCompiler::ParseStatement_class,			// class
	&LCompiler::ParseStatement_class,			// struct
	&LCompiler::ParseStatement_class,			// namespace
	&LCompiler::ParseStatement_typedef,			// typedef
	&LCompiler::ParseStatement_expr,			// function
	&LCompiler::ParseStatement_syntax_error,	// extends
	&LCompiler::ParseStatement_syntax_error,	// implements
	&LCompiler::ParseStatement_for,				// for
	&LCompiler::ParseStatement_syntax_error,	// in
	&LCompiler::ParseStatement_forever,			// forever
	&LCompiler::ParseStatement_while,			// while
	&LCompiler::ParseStatement_do,				// do
	&LCompiler::ParseStatement_if,				// if
	&LCompiler::ParseStatement_syntax_error,	// else
	&LCompiler::ParseStatement_switch,			// switch
	&LCompiler::ParseStatement_case,			// case
	&LCompiler::ParseStatement_default,			// default
	&LCompiler::ParseStatement_break,			// break
	&LCompiler::ParseStatement_continue,		// continue
	&LCompiler::ParseStatement_goto,			// goto
	&LCompiler::ParseStatement_try,				// try
	&LCompiler::ParseStatement_syntax_error,	// catch
	&LCompiler::ParseStatement_syntax_error,	// finally
	&LCompiler::ParseStatement_throw,			// throw
	&LCompiler::ParseStatement_return,			// return
	&LCompiler::ParseStatement_with,			// with
	&LCompiler::ParseStatement_expr,			// operator
	&LCompiler::ParseStatement_synchronized,	// synchronized
	&LCompiler::ParseStatement_definition,		// static
	&LCompiler::ParseStatement_definition,		// abstract
	&LCompiler::ParseStatement_definition,		// native
	&LCompiler::ParseStatement_def_expr,		// const
	&LCompiler::ParseStatement_definition,		// fetch_addr
	&LCompiler::ParseStatement_definition,		// public
	&LCompiler::ParseStatement_definition,		// protected
	&LCompiler::ParseStatement_definition,		// private
	&LCompiler::ParseStatement_definition,		// @override
	&LCompiler::ParseStatement_definition,		// @deprecated
	&LCompiler::ParseStatement_definition_auto,	// auto
	&LCompiler::ParseStatement_def_expr,		// void
	&LCompiler::ParseStatement_def_expr,		// boolean
	&LCompiler::ParseStatement_def_expr,		// byte
	&LCompiler::ParseStatement_def_expr,		// ubyte
	&LCompiler::ParseStatement_def_expr,		// short
	&LCompiler::ParseStatement_def_expr,		// ushort
	&LCompiler::ParseStatement_def_expr,		// int
	&LCompiler::ParseStatement_def_expr,		// uint
	&LCompiler::ParseStatement_def_expr,		// long
	&LCompiler::ParseStatement_def_expr,		// ulong
	&LCompiler::ParseStatement_def_expr,		// float
	&LCompiler::ParseStatement_def_expr,		// double
	&LCompiler::ParseStatement_def_expr,		// int8
	&LCompiler::ParseStatement_def_expr,		// uint8
	&LCompiler::ParseStatement_def_expr,		// int16
	&LCompiler::ParseStatement_def_expr,		// uint16
	&LCompiler::ParseStatement_def_expr,		// int32
	&LCompiler::ParseStatement_def_expr,		// uint32
	&LCompiler::ParseStatement_def_expr,		// int64
	&LCompiler::ParseStatement_def_expr,		// uint64
	&LCompiler::ParseStatement_expr,			// this
	&LCompiler::ParseStatement_expr,			// super
	&LCompiler::ParseStatement_expr,			// global
	&LCompiler::ParseStatement_expr,			// null
	&LCompiler::ParseStatement_expr,			// false
	&LCompiler::ParseStatement_expr,			// true
} ;

// @import module-name
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_import
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	LStringParser	sparsLine ;
	sparsSrc.NextLine( sparsLine ) ;

	LString	strModuleName ;
	sparsLine.PassSpace() ;
	if ( sparsLine.HasNextChars( L"\"" ) == L'\"' )
	{
		if ( !sparsLine.NextStringTermByChars( strModuleName, L"\"" ) )
		{
			OnError( errorMismatchDoubleQuotation ) ;
		}
	}
	else
	{
		sparsLine.NextStringTermBySpace( strModuleName ) ;
	}
	if ( strModuleName.IsEmpty() )
	{
		OnError( errorSyntaxError, GetReservedWordDesc(rwIndex).pwszName ) ;
		return ;
	}
	LModulePtr	pModule = m_vm.ModuleProducer().
							GetModule( strModuleName.c_str() ) ;
	if ( pModule != nullptr )
	{
		// 既にインポート済み
		return ;
	}
	pModule = m_vm.ModuleProducer().ProduceModule( strModuleName.c_str() ) ;
	if ( pModule != nullptr )
	{
		LPackagePtr	pPackage =
			std::make_shared<LPackage>
				( LPackage::typeImportingModule, strModuleName.c_str() ) ;
		m_vm.AddPackage( pPackage ) ;

		LPackage::Current	current( pPackage.get() ) ;

		pModule->ImportTo( m_vm ) ;
	}
	else
	{
		OnError( errorNotFoundModule, strModuleName.c_str() ) ;
	}

	if ( sparsLine.PassSpace() )
	{
		OnError( errorExtraDescription ) ;
	}
}

// @include source-file
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_incllude
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	LStringParser	sparsLine ;
	sparsSrc.NextLine( sparsLine ) ;

	LString	strSourceName ;
	sparsLine.PassSpace() ;
	if ( sparsLine.HasNextChars( L"\"" ) == L'\"' )
	{
		if ( !sparsLine.NextStringTermByChars( strSourceName, L"\"" ) )
		{
			OnError( errorMismatchDoubleQuotation ) ;
		}
	}
	else
	{
		sparsLine.NextStringTermBySpace( strSourceName ) ;
	}
	if ( strSourceName.IsEmpty() )
	{
		OnError( errorSyntaxError, GetReservedWordDesc(rwIndex).pwszName ) ;
		return ;
	}

	// インクルード
	IncludeScript( strSourceName.c_str(), sparsSrc.GetFileDirectory() ) ;
}

// @error ...
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_error
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	LStringParser	sparsLine ;
	sparsSrc.NextLine( sparsLine ) ;

	LString	strMsg ;
	if ( sparsLine.PassSpace() )
	{
		strMsg = sparsLine.Middle
					( sparsLine.GetIndex(),
						sparsLine.GetLength() - sparsLine.GetIndex() ) ;
	}
	OnError( errorUserMessage_opt1, strMsg.c_str() ) ;
}

// @todo ...
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_todo
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	LString	strLine ;
	sparsSrc.NextLine( strLine ) ;

	OnWarning( errorUserMessage_opt1, warning3, strLine.c_str() ) ;
}

// class, struct, namespace
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_class
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeInNamespace() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	assert( pNamespace != nullptr ) ;

	// class Type1 :: Type2 :: ... DefName
	for ( ; ; )
	{
		LString	strName ;
		if ( sparsSrc.NextToken( strName ) == LStringParser::tokenNothing )
		{
			OnError( errorSyntaxError ) ;
			return ;
		}

		LPtr<LNamespace>	pSub = pNamespace->GetLocalNamespaceAs( strName.c_str() ) ;
		if ( pSub == nullptr )
		{
			if ( !IsValidUserName( strName.c_str() ) )
			{
				OnError( errorConnotAsUserSymbol_opt1, strName.c_str() ) ;
				if ( strName != L";" )
				{
					sparsSrc.PassStatement
						( (strName == L"{") ? L"};" : L";" ) ;
				}
				return ;
			}
			if ( sparsSrc.PassSpace()
				&& (sparsSrc.CurrentChar() == L'<') )
			{
				if ( IsInstantiatingGenericType( sparsSrc ) )
				{
					// ジェネリック型のインスタンス化
					ParseInstantiatingGenericType
						( sparsSrc, pNamespace, strName.c_str(), rwIndex, pnslLocal ) ;
					return ;
				}
				// ジェネリック型の宣言
				ParseDeclarationGenericType
					( sparsSrc, pNamespace, strName.c_str(), rwIndex, pnslLocal ) ;
				return ;
			}
			switch ( rwIndex )
			{
			case	Symbol::rwiClass:
			default:
				pSub = new LGenericObjClass
						( m_vm, pNamespace, m_vm.GetClassClass(), strName.c_str() ) ;
				break ;

			case	Symbol::rwiStruct:
				pSub = new LStructureClass
						( m_vm, pNamespace, m_vm.GetClassClass(), strName.c_str() ) ;
				break ;

			case	Symbol::rwiNamespace:
				pSub = new LNamespace
						( m_vm, pNamespace, m_vm.GetNamespaceClass(), strName.c_str() ) ;
				break ;
			}
			pNamespace->AddNamespace( strName.c_str(), pSub ) ;
			//
			if ( !m_commentBefore.IsEmpty() )
			{
				pSub->SetSelfComment( m_commentBefore.c_str() ) ;
				m_commentBefore = L"" ;
			}
		}
		pNamespace = pSub ;

		if ( sparsSrc.HasNextToken( L"::" ) )
		{
			continue ;
		}
		else
		{
			break ;
		}
	}
	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		// 宣言だけ
		return ;
	}
	if ( !m_commentBefore.IsEmpty() )
	{
		pNamespace->SetSelfComment( m_commentBefore.c_str() ) ;
		m_commentBefore = L"" ;
	}

	// 実装
	ParseImplementationClass( sparsSrc, pNamespace, rwIndex, pnslLocal ) ;
}

// class, struct, namespace 実装（派生から）
void LCompiler::ParseImplementationClass
	( LStringParser& sparsSrc, LPtr<LNamespace> pNamespace,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	CodeNest::ControlType	typeNest ;
	if ( rwIndex == Symbol::rwiClass )
	{
		// クラス
		LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
		if ( pClass == nullptr )
		{
			OnError( errorIsNotClassName, pNamespace->GetName().c_str() ) ;
			sparsSrc.PassStatementBlock() ;
			return ;
		}
		if ( pClass->IsClassCompleted() )
		{
			OnError( errorDoubleDefinitionOfClass, pClass->GetClassName().c_str() ) ;
			sparsSrc.PassStatementBlock() ;
			return ;
		}
		typeNest = CodeNest::ctrlClass ;
	}
	else if ( rwIndex == Symbol::rwiStruct )
	{
		// 構造体
		LStructureClass *
				pStruct = dynamic_cast<LStructureClass*>( pNamespace.Ptr() ) ;
		if ( pStruct == nullptr )
		{
			OnError( errorIsNotStructName, pNamespace->GetName().c_str() ) ;
			sparsSrc.PassStatementBlock() ;
			return ;
		}
		if ( pStruct->IsClassCompleted() )
		{
			OnError( errorDoubleDefinitionOfClass, pStruct->GetClassName().c_str() ) ;
			sparsSrc.PassStatementBlock() ;
			return ;
		}
		typeNest = CodeNest::ctrlStruct ;
	}
	else
	{
		// 名前空間
		if ( dynamic_cast<LClass*>( pNamespace.Ptr() ) != nullptr )
		{
			OnError( errorIsNotNamespace, pNamespace->GetName().c_str() ) ;
			sparsSrc.PassStatementBlock() ;
			return ;
		}
		typeNest = CodeNest::ctrlNamespace ;
	}

	CodeNestPtr	pNest = PushNest( typeNest ) ;
	NestPopper	popSpace( *this, pNest ) ;
	pNest->m_namespace = pNamespace ;

	if ( rwIndex == Symbol::rwiClass )
	{
		// class name [extends super-class] [implements class2 [, class3 ...]]
		LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
		assert( pClass != nullptr ) ;

		LClass *	pSuperClass = m_vm.GetObjectClass() ;
		if ( sparsSrc.HasNextToken
				( GetReservedWordDesc
					(Symbol::rwiExtends).pwszName /*L"extends"*/ ) )
		{
			// extends ...
			LPtr<LNamespace>	pParent =
				ParseNamespaceExpr( sparsSrc, Symbol::rwiClass, pnslLocal ) ;
			if ( pParent == nullptr )
			{
				sparsSrc.PassStatementBlock() ;
				return ;
			}
			pSuperClass = dynamic_cast<LClass*>( pParent.Ptr() ) ;
			if ( (pSuperClass == nullptr)
				|| (dynamic_cast<LStructureClass*>( pSuperClass ) != nullptr) )
			{
				OnError( errorIsNotClassName, pParent->GetName().c_str() ) ;
				sparsSrc.PassStatementBlock() ;
				return ;
			}
			if ( dynamic_cast<LGenericObj*>
					( pSuperClass->GetPrototypeObject().Ptr() ) == nullptr )
			{
				OnError( errorNonDerivableClass, pParent->GetName().c_str() ) ;
				sparsSrc.PassStatementBlock() ;
				return ;
			}
		}
		if ( pSuperClass->GetPrototypeObject() != nullptr )
		{
			pClass->SetPrototypeObject
				( pSuperClass->GetPrototypeObject()->CloneObject() ) ;
			pClass->GetPrototypeObject()->SetClass( pClass ) ;
		}
		if ( !pClass->AddSuperClass( pSuperClass ) )
		{
			sparsSrc.PassStatementBlock() ;
			return ;
		}
		if ( sparsSrc.HasNextToken
				( GetReservedWordDesc
					(Symbol::rwiImplements).pwszName /*L"implements"*/ ) )
		{
			// implements ...
			do
			{
				LPtr<LNamespace>	pParent =
					ParseNamespaceExpr( sparsSrc, Symbol::rwiClass, pnslLocal ) ;
				if ( pParent == nullptr )
				{
					sparsSrc.PassStatementBlock() ;
					return ;
				}
				pSuperClass = dynamic_cast<LClass*>( pParent.Ptr() ) ;
				if ( (pSuperClass == nullptr)
					|| (dynamic_cast<LStructureClass*>( pSuperClass ) != nullptr) )
				{
					OnError( errorIsNotClassName, pParent->GetName().c_str() ) ;
					sparsSrc.PassStatementBlock() ;
					return ;
				}
				if ( dynamic_cast<LGenericObj*>
						( pSuperClass->GetPrototypeObject().Ptr() ) == nullptr )
				{
					OnError( errorNonDerivableClass, pParent->GetName().c_str() ) ;
					sparsSrc.PassStatementBlock() ;
					return ;
				}
				if ( !pClass->AddImplementClass( pSuperClass ) )
				{
					sparsSrc.PassStatementBlock() ;
					return ;
				}
			}
			while ( sparsSrc.HasNextChars( L"," ) == L',' ) ;
		}
		typeNest = CodeNest::ctrlClass ;
	}
	else if ( rwIndex == Symbol::rwiStruct )
	{
		// struct name [extends super-class [, ...]]
		LStructureClass *
				pStruct = dynamic_cast<LStructureClass*>( pNamespace.Ptr() ) ;
		assert( pStruct != nullptr ) ;

		if ( sparsSrc.HasNextToken( L"extends" ) )
		{
			// extends ...
			size_t	i = 0 ;
			do
			{
				LPtr<LNamespace>	pParent =
					ParseNamespaceExpr( sparsSrc, Symbol::rwiClass, pnslLocal ) ;
				if ( pParent == nullptr )
				{
					sparsSrc.PassStatementBlock() ;
					return ;
				}
				LStructureClass *	pSuperClass =
							dynamic_cast<LStructureClass*>( pParent.Ptr() ) ;
				if ( pSuperClass == nullptr )
				{
					OnError( errorIsNotStructName, pParent->GetName().c_str() ) ;
					sparsSrc.PassStatementBlock() ;
					return ;
				}
				if ( (i == 0)
						? !pStruct->AddSuperClass( pSuperClass )
						: !pStruct->AddImplementClass( pSuperClass ) )
				{
					sparsSrc.PassStatementBlock() ;
					return ;
				}
				i ++ ;
			}
			while ( sparsSrc.HasNextChars( L"," ) == L',' ) ;
		}
		else
		{
			pStruct->AddSuperClass( m_vm.GetStructureClass() ) ;
		}
		typeNest = CodeNest::ctrlStruct ;
	}

	if ( sparsSrc.HasNextChars( L"{" ) != L'{' )
	{
		OnError( errorNotFoundSyntax_opt1, L"{" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}

	LNamespaceList	nslNamespace ;
	nslNamespace.AddNamespace( pNamespace.Ptr() ) ;
	if ( pnslLocal != nullptr )
	{
		nslNamespace.AddNamespaceList( *pnslLocal ) ;
	}
	ParseStatementList( sparsSrc, &nslNamespace, L"}" ) ;

	if ( sparsSrc.HasNextChars( L"}" ) != L'}' )
	{
		OnError( errorMismatchBraces ) ;
		sparsSrc.PassStatement( L"}" ) ;
	}

	popSpace.Pop() ;

	if ( (rwIndex == Symbol::rwiClass)
			|| (rwIndex == Symbol::rwiStruct) )
	{
		LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
		assert( pClass != nullptr ) ;
		pClass->CompleteClass() ;
	}
}

// 派生元クラスを解釈
LPtr<LNamespace> LCompiler::ParseNamespaceExpr
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	// Type1 :: Type2 :: ... DefName
	size_t	iStart = sparsSrc.GetIndex() ;

	LString	strName ;
	if ( sparsSrc.NextToken( strName ) == LStringParser::tokenNothing )
	{
		OnError( errorSyntaxError ) ;
		return	nullptr ;
	}

	LPtr<LNamespace>	pNamespace ;
	LExprValuePtr		xval = GetExprSymbolAs
									( strName.c_str(), &sparsSrc, pnslLocal ) ;
	if ( xval != nullptr )
	{
		if ( xval->IsNamespace() )
		{
			pNamespace = xval->GetNamespace() ;
		}
		else if ( xval->IsConstExprClass() )
		{
			LClass *	pClass = xval->GetConstExprClass() ;
			assert( pClass != nullptr ) ;
			pClass->AddRef() ;
			pNamespace = pClass ;
		}
	}
	while ( (pNamespace != nullptr)
			&& (sparsSrc.HasNextToken( L"::" )
				|| sparsSrc.HasNextToken( L"." )) )
	{
		if ( sparsSrc.NextToken( strName ) == LStringParser::tokenNothing )
		{
			OnError( errorSyntaxError ) ;
			return	nullptr ;
		}
		pNamespace = pNamespace->GetLocalNamespaceAs( strName.c_str() ) ;
	}
	if ( pNamespace == nullptr )
	{
		switch ( rwIndex )
		{
		case	Symbol::rwiClass:
			OnError( errorIsNotClassName, strName.c_str() ) ;
			break ;
		case	Symbol::rwiStruct:
			OnError( errorIsNotStructName, strName.c_str() ) ;
			break ;
		default:
			OnError( errorIsNotNamespace, strName.c_str() ) ;
			break ;
		}
		sparsSrc.SeekIndex( iStart ) ;
		return	nullptr ;
	}
	return	pNamespace ;
}

// ジェネリック型のインスタンス化書式か？
bool LCompiler::IsInstantiatingGenericType( LStringParser& sparsSrc )
{
	const size_t	iSaveIndex = sparsSrc.GetIndex() ;
	bool			isInstantiating = false ;
	if ( (sparsSrc.HasNextChars( L"<" ) == L'<')
		&& (sparsSrc.PassStatement( L">", L"};" ) == L'>')
		&& (sparsSrc.HasNextChars( L";" ) == L';') )
	{
		isInstantiating = true ;
	}
	sparsSrc.SeekIndex( iSaveIndex ) ;
	return	isInstantiating ;
}

// ジェネリック型のインスタンス化
void LCompiler::ParseInstantiatingGenericType
	( LStringParser& sparsSrc,
		LPtr<LNamespace> pNamespace, const wchar_t * pwszName,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	LType	type ;
	if ( !sparsSrc.ParseTypeName
		( type, true, pwszName, m_vm, pnslLocal, 0, true ) )
	{
		if ( !HasErrorOnCurrent() )
		{
			OnError( errorNotGenericType, pwszName ) ;
		}
	}
	if ( sparsSrc.HasNextChars( L";" ) != L';' )
	{
		OnError( errorNotFoundSemicolonEnding ) ;
		sparsSrc.PassStatement( L";", L"{}" ) ;
	}
}

// ジェネリック型の宣言
void LCompiler::ParseDeclarationGenericType
	( LStringParser& sparsSrc,
		LPtr<LNamespace> pNamespace, const wchar_t * pwszName,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( pNamespace->GetGenericTypeAs( pwszName ) != nullptr )
	{
		OnError( errorDoubleDefinitionOfClass, pwszName ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}

	LNamespace::GenericDef	gendef ;
	gendef.m_parent = pNamespace ;
	gendef.m_name = pwszName ;
	gendef.m_kind = rwIndex ;
	gendef.m_source = m_vm.SourceProducer().GetSafeSource( &sparsSrc ) ;
	gendef.m_srcFirst = sparsSrc.GetIndex() ;

	// ジェネリック型引数解釈
	if ( sparsSrc.HasNextChars( L"<" ) == L'<' )
	{
		for ( ; ; )
		{
			LString	strArgName ;
			sparsSrc.NextToken( strArgName ) ;

			if ( !IsValidUserName( strArgName.c_str() ) )
			{
				OnError( errorConnotAsUserSymbol_opt1, strArgName.c_str() ) ;
				sparsSrc.PassStatementBlock() ;
				return ;
			}
			gendef.m_params.push_back( strArgName ) ;

			wchar_t	wch = sparsSrc.HasNextChars( L",>" ) ;
			if ( wch != L',' )
			{
				if ( wch != L'>' )
				{
					OnError( errorNoSeparationArgmentList ) ;
					sparsSrc.PassStatementBlock() ;
				}
				break ;
			}
		}
	}
	else
	{
		OnError( errorNotFoundSyntax_opt1, L"<" ) ;
	}

	// ブロックの終端
	sparsSrc.PassStatementBlock() ;
	gendef.m_srcEnd = sparsSrc.GetIndex() ;

	// 登録
	pNamespace->DefineGenericType( pwszName, gendef ) ;
}

// typedef
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_typedef
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeInNamespace() )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	assert( pNamespace != nullptr ) ;

	// typdef name = type-expr ;
	LString	strTypeName ;
	sparsSrc.NextToken( strTypeName ) ;

	if ( !IsValidUserName( strTypeName.c_str() ) )
	{
		OnError( errorConnotAsUserSymbol_opt1, strTypeName.c_str() ) ;
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}

	if ( sparsSrc.HasNextChars( L"=" ) != L'=' )
	{
		OnError( errorNotFoundSyntax_opt1, L"=" ) ;
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}

	LType	typeDef ;
	sparsSrc.NextTypeExpr( typeDef, true, m_vm, pnslLocal ) ;

	HasSemicolonForEndOfStatement( sparsSrc ) ;

	// 定義追加
	if ( pNamespace->GetTypeAs( strTypeName.c_str() ) != nullptr )
	{
		OnError( errorDoubleDefinitionOfType, strTypeName.c_str() ) ;
	}
	else
	{
		pNamespace->DefineTypeAs( strTypeName.c_str(), typeDef ) ;
	}
}

// for
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_for
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	// for ( init-expr; cond-expr; iter-expr )
	// for ( decl-var-statement cond-expr; iter-expr )
	// for ( iter-var-name in expr )
	// for ( var-name : expr )

	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	CodeNestPtr	pNestFor = PushNest( CodeNest::ctrlFor ) ;
	NestPopper	popFor( *this, pNestFor ) ;

	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNotFoundSyntax_opt1, L"(" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	const size_t	iStartParams = sparsSrc.GetIndex() ;

	// 反復変数名、初期化式の判定
	LString	strVarName ;
	sparsSrc.NextToken( strVarName ) ;

	LType	typeIterVar ;
	if ( sparsSrc.HasNextChars( L":" ) == L':' )
	{
		// for ( var-name : expr )
		ParseStatement_for_autovar
			( sparsSrc, pNestFor, strVarName.c_str(), pnslLocal ) ;
	}
	else  if ( sparsSrc.HasNextToken
			( GetReservedWordDesc(Symbol::fwiIn).pwszName /*L"in"*/ ) )
	{
		// for ( iter-var-name in expr )
		ParseStatement_for_autoiter
			( sparsSrc, pNestFor, strVarName.c_str(), pnslLocal ) ;
	}
	else
	{
		// for ( init-expr; cond-expr; iter-expr )
		// for ( decl-var-statement cond-expr; iter-expr )
		sparsSrc.SeekIndex( iStartParams ) ;
		ParseStatement_for_params( sparsSrc, pNestFor, pnslLocal ) ;
	}
	if ( HasErrorOnCurrent() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ExprCodeFreeTempStack() ;

	// for 反復実体
	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		OnWarning( warningIntendedBlankStatement, warning3 ) ;
	}
	else
	{
		ParseOneStatement( sparsSrc, pnslLocal ) ;
	}

	// 繰り返し
	ExprCodeJump( pNestFor->m_cpContinue ) ;

	pNestFor->m_returned = false ;
	popFor.Pop() ;
}

void LCompiler::ParseStatement_for_autovar
	( LStringParser& sparsSrc,
		LCompiler::CodeNestPtr pNestFor, const wchar_t * pwszVarName,
		const LNamespaceList * pnslLocal )
{
	// for ( var-name : expr )

	// ループ対象オブジェクト
	LExprValuePtr	xval = EvaluateExpression( sparsSrc, pnslLocal, L")" ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}
	LLocalVarPtr	pVarObj = MakeTempStackIntoLocalVar( std::move(xval) ) ;

	// ループ変数を確保する
	LLocalVarPtr	pVarIndex =
			ExprCodeDeclareLocalVar( L"@for.i", LType(LType::typeInt64) ) ;
	if ( pVarIndex == nullptr )
	{
		return ;
	}
	ExprCodeStore( pVarIndex, LExprValue::MakeConstExprInt(0) ) ;

	LLocalVarPtr	pVarElement =
			ExprCodeDeclareLocalVar
				( pwszVarName, pVarObj->GetType().GetRefElementType() ) ;
	if ( pVarElement == nullptr )
	{
		return ;
	}
	CodePointPtr	cpJumpToCond = ExprCodeJump() ;

	// イテレーターを進める
	CodePointPtr	cpContinue = GetCurrentCodePointer( true ) ;
	pNestFor->m_cpContinue = cpContinue ;

	ExprCodeStore( pVarIndex,
		EvalUnaryOperator( Symbol::opIncrement, false, pVarIndex ) ) ;

	// 反復条件比較
	FixJumpDestination( cpJumpToCond, GetCurrentCodePointer( true ) ) ;

	LExprValuePtr	xvalCond =
		EvalBinaryOperator
			( pVarIndex, Symbol::opLessThan, ExprSizeOfExpr( pVarObj ) ) ;
	CodePointPtr	cpJumpToBreak =
		ExprCodeJumpNonIf( std::move(xvalCond) ) ;

	pNestFor->m_vecBreaks.push_back( cpJumpToBreak ) ;

	// 要素取得
	ExprCodeStore( pVarElement,
					EvalOperatorElement( pVarObj, pVarIndex ) ) ;

	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;
}

void LCompiler::ParseStatement_for_autoiter
	( LStringParser& sparsSrc,
		LCompiler::CodeNestPtr pNestFor, const wchar_t * pwszVarName,
		const LNamespaceList * pnslLocal )
{
	// for ( iter-var-name in expr )

	// ループ対象オブジェクト
	LExprValuePtr	xval = EvaluateExpression( sparsSrc, pnslLocal, L")" ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}
	LLocalVarPtr	pVarObj = MakeTempStackIntoLocalVar( std::move(xval) ) ;

	// ループ変数を確保する
	LLocalVarPtr	pVarIndex ;
	LLocalVarPtr	pVarKey ;
	if ( pVarObj->GetType().IsMap() )
	{
		pVarKey = ExprCodeDeclareLocalVar
						( pwszVarName, LType(m_vm.GetStringClass()) ) ;
		if ( pVarKey == nullptr )
		{
			return ;
		}
		pVarIndex = ExprCodeDeclareLocalVar
						( L"@for.i", LType(LType::typeInt64) ) ;
	}
	else
	{
		pVarIndex = ExprCodeDeclareLocalVar
						( pwszVarName, LType(LType::typeInt64) ) ;
	}
	if ( pVarIndex == nullptr )
	{
		return ;
	}
	ExprCodeStore( pVarIndex, LExprValue::MakeConstExprInt(0) ) ;

	CodePointPtr	cpJumpToCond = ExprCodeJump() ;

	// イテレーターを進める
	CodePointPtr	cpContinue = GetCurrentCodePointer( true ) ;
	pNestFor->m_cpContinue = cpContinue ;

	ExprCodeStore( pVarIndex,
		EvalUnaryOperator
			( Symbol::opIncrement, false, pVarIndex ) ) ;

	// 反復条件比較
	FixJumpDestination( cpJumpToCond, GetCurrentCodePointer( true ) ) ;

	LExprValuePtr	xvalCond =
		EvalBinaryOperator
			( pVarIndex, Symbol::opLessThan, ExprSizeOfExpr( pVarObj ) ) ;
	CodePointPtr	cpJumpToBreak =
		ExprCodeJumpNonIf( std::move(xvalCond) ) ;

	pNestFor->m_vecBreaks.push_back( cpJumpToBreak ) ;

	// 要素名取得
	if ( pVarKey != nullptr )
	{
		// 以前の要素名 String オブジェクトを解放しておく
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = pVarKey->GetLocation() ;

		AddCode( LCodeBuffer::codeFreeLocalObj, 0, 0, 0, 0, &immop ) ;

		// 要素名を取得する
		LExprValuePtr	xvalObj = ExprMakeOnStack( pVarObj ) ;
		LExprValuePtr	xvalIndex = ExprMakeOnStack( pVarIndex ) ;

		const size_t	iStackObj = GetBackIndexOnStack( xvalObj ) ;
		const size_t	iStackIndex = GetBackIndexOnStack( xvalIndex ) ;
		assert( iStackObj >= 0 ) ;
		assert( iStackIndex >= 0 ) ;

		xvalObj = nullptr ;
		xvalIndex = nullptr ;

		const size_t	nFreeCount = CountFreeTempStack() ;
		PopExprValueOnStacks( nFreeCount ) ;

		AddCode( LCodeBuffer::codeGetElementName, iStackObj, iStackIndex, 0, 0 ) ;

		// 要素名をストアする
		AddCode( LCodeBuffer::codeStoreLocal,
						0, 0, LType::typeObject, nFreeCount + 1, &immop ) ;
	}
	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;
}

void LCompiler::ParseStatement_for_params
	( LStringParser& sparsSrc,
		LCompiler::CodeNestPtr pNestFor,
		const LNamespaceList * pnslLocal )
{
	// for ( init-expr; cond-expr; iter-expr )
	// for ( decl-var-statement cond-expr; iter-expr )

	// 初期化式・又は変数宣言式
	ParseDefinitionOrExpression( sparsSrc, pnslLocal ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}
	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	// 継続条件式
	CodePointPtr	cpCondPos = GetCurrentCodePointer( true ) ;
	if ( sparsSrc.HasNextChars( L";" ) != L';' )
	{
		LExprValuePtr	xvalCond =
			EvalAsBoolean
				( EvaluateExpression( sparsSrc, pnslLocal, L";" ) ) ;
		if ( HasErrorOnCurrent() )
		{
			return ;
		}
		if ( sparsSrc.HasNextChars( L";" ) != L';' )
		{
			OnError( errorNotFoundSyntax_opt1, L";" ) ;
			return ;
		}
		if ( xvalCond->IsConstExpr() )
		{
			pNestFor->m_forever = (xvalCond->AsInteger() != 0) ;
		}
		CodePointPtr	cpJumpToBreak =
							ExprCodeJumpNonIf( std::move(xvalCond) ) ;
		pNestFor->m_vecBreaks.push_back( cpJumpToBreak ) ;
	}
	else
	{
		// 無条件ループ
		pNestFor->m_forever = true ;
	}
	CodePointPtr	cpJumpIn = ExprCodeJump() ;

	// 反復式
	sparsSrc.PassSpace() ;
	if ( sparsSrc.CurrentChar() != L')' )
	{
		CodePointPtr	cpContinue = GetCurrentCodePointer( true ) ;
		pNestFor->m_cpContinue = cpContinue ;

		EvaluateExpression( sparsSrc, pnslLocal, L")" ) ;
		ExprCodeFreeTempStack() ;
		assert( m_ctx->m_xvaStack.size() == 0 ) ;

		if ( !pNestFor->m_forever )
		{
			ExprCodeJump( cpCondPos ) ;
		}
	}
	else
	{
		pNestFor->m_cpContinue = cpCondPos ;
	}

	FixJumpDestination( cpJumpIn, GetCurrentCodePointer( true ) ) ;
}

// forever
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_forever
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	CodeNestPtr	pNestFor = PushNest( CodeNest::ctrlFor ) ;
	NestPopper	popFor( *this, pNestFor ) ;

	pNestFor->m_cpContinue = GetCurrentCodePointer( true ) ;
	pNestFor->m_forever = true ;

	// forever 反復実体
	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		OnWarning( warningIntendedBlankStatement, warning3 ) ;
	}
	else
	{
		ParseOneStatement( sparsSrc, pnslLocal ) ;
	}

	// 繰り返し
	ExprCodeJump( pNestFor->m_cpContinue ) ;

	pNestFor->m_returned = false ;
	popFor.Pop() ;
}

// while
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_while
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	CodeNestPtr	pNestWhile = PushNest( CodeNest::ctrlWhile ) ;
	NestPopper	popWhile( *this, pNestWhile ) ;

	pNestWhile->m_cpContinue = GetCurrentCodePointer( true ) ;

	// 反復条件
	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNotFoundSyntax_opt1, L"(" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	LExprValuePtr	xvalCond =
		EvalAsBoolean
			( EvaluateExpression( sparsSrc, pnslLocal, L")" ) ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}
	if ( xvalCond->IsConstExpr() )
	{
		pNestWhile->m_forever = (xvalCond->AsInteger() != 0) ;
	}
	CodePointPtr	cpJumpToBreak =
						ExprCodeJumpNonIf( std::move(xvalCond) ) ;
	pNestWhile->m_vecBreaks.push_back( cpJumpToBreak ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ExprCodeFreeTempStack() ;

	// while 反復実体
	ParseOneStatement( sparsSrc, pnslLocal ) ;

	// 繰り返し
	ExprCodeJump( pNestWhile->m_cpContinue ) ;

	pNestWhile->m_returned = false ;
	popWhile.Pop() ;
}

// do
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_do
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	CodeNestPtr	pNestDo = PushNest( CodeNest::ctrlDo ) ;
	NestPopper	popDo( *this, pNestDo ) ;

	CodePointPtr	cpJumpTop =GetCurrentCodePointer( true ) ;

	// do 反復実体
	if ( sparsSrc.HasNextChars( L"{" ) != L'{' )
	{
		OnError( errorNotFoundSyntax_opt1, L"{" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ParseStatementList( sparsSrc, pnslLocal, L"}" ) ;

	if ( sparsSrc.HasNextChars( L"}" ) != L'}' )
	{
		OnError( errorMismatchBraces ) ;
		return ;
	}

	// 反復条件
	pNestDo->m_cpContinue = GetCurrentCodePointer( true ) ;

	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		// 反復しない
	}
	else if ( sparsSrc.HasNextToken
			( GetReservedWordDesc(Symbol::rwiWhile).pwszName /*L"while"*/ ) )
	{
		// 反復条件
		LExprValuePtr	xvalCond =
			EvalAsBoolean
				( EvaluateExpression( sparsSrc, pnslLocal, L";" ) ) ;
		if ( HasErrorOnCurrent() )
		{
			return ;
		}
		if ( xvalCond->IsConstExpr() )
		{
			pNestDo->m_forever = (xvalCond->AsInteger() != 0) ;
		}
		ExprCodeJumpIf( std::move(xvalCond), cpJumpTop ) ;

		HasSemicolonForEndOfStatement( sparsSrc ) ;
	}
	else
	{
		OnError( errorNotFoundSyntax_opt1,
				GetReservedWordDesc(Symbol::rwiWhile).pwszName ) ;
	}

	pNestDo->m_returned = false ;
	popDo.Pop() ;
}

// if
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_if
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	CodeNestPtr	pNestIf = PushNest( CodeNest::ctrlIf ) ;
	NestPopper	popIf( *this, pNestIf ) ;

	// 条件式
	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNotFoundSyntax_opt1, L"(" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	LExprValuePtr	xvalCond =
		EvalAsBoolean
			( EvaluateExpression( sparsSrc, pnslLocal, L")" ) ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}
	CodePointPtr	cpJumpToElse =
						ExprCodeJumpNonIf( std::move(xvalCond) ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ExprCodeFreeTempStack() ;

	// if ブロック
	ParseOneStatement( sparsSrc, pnslLocal ) ;

	if ( sparsSrc.HasNextToken
			( GetReservedWordDesc(Symbol::rwiElse).pwszName /*L"else"*/ ) )
	{
		// else ブロック
		CodePointPtr	cpJumpToEndIf = ExprCodeJump() ;
		CodePointPtr	cpElse = GetCurrentCodePointer( true ) ;
		FixJumpDestination( cpJumpToElse, cpElse ) ;
		cpJumpToElse = cpJumpToEndIf ;

		bool flagIfReturned = pNestIf->m_returned ;
		pNestIf->m_returned = false ;

		ParseOneStatement( sparsSrc, pnslLocal ) ;

		pNestIf->m_returned = (pNestIf->m_returned && flagIfReturned) ;
	}
	else
	{
		pNestIf->m_returned = false ;
	}

	// endif
	CodePointPtr	cpEndIf = GetCurrentCodePointer( true ) ;
	FixJumpDestination( cpJumpToElse, cpEndIf ) ;
}

// switch
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_switch
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	CodeNestPtr	pNestSwitch = PushNest( CodeNest::ctrlSwitch ) ;
	NestPopper	popSwitch( *this, pNestSwitch ) ;

	// switch 分岐値
	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNotFoundSyntax_opt1, L"(" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	LExprValuePtr	xval =
			EvaluateExpression( sparsSrc, pnslLocal, L")" ) ;
	if ( HasErrorOnCurrent() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	pNestSwitch->m_var =MakeTempStackIntoLocalVar( std::move(xval) ) ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	pNestSwitch->m_cpJumpNext = ExprCodeJump() ;

	// switch 実体
	if ( sparsSrc.HasNextChars( L"{" ) != L'{' )
	{
		OnError( errorNotFoundSyntax_opt1, L"{" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ParseStatementList( sparsSrc, pnslLocal, L"}" ) ;

	if ( sparsSrc.HasNextChars( L"}" ) != L'}' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}

	// switch 終端処理
	CodePointPtr	cpJumpToBreak = ExprCodeJump() ;
	pNestSwitch->m_vecBreaks.push_back( cpJumpToBreak ) ;

	if ( pNestSwitch->m_cpDefault != nullptr )
	{
		// default へジャンプ
		FixJumpDestination
			( pNestSwitch->m_cpJumpNext, pNestSwitch->m_cpDefault ) ;
	}
	else
	{
		pNestSwitch->m_vecBreaks.push_back( pNestSwitch->m_cpJumpNext ) ;
	}

	popSwitch.Pop() ;
}

// case
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_case
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L":", L";}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;
	m_ctx->m_curNest->m_returned = false ;

	CodeNestPtr	pNestSwitch = m_ctx->m_curNest ;
	if ( pNestSwitch->m_type != CodeNest::ctrlSwitch )
	{
		OnError( errorAssertCaseLabelInSwitchBlock ) ;
		sparsSrc.PassStatement( L":", L";}" ) ;
		return ;
	}
	assert( pNestSwitch->m_cpJumpNext != nullptr ) ;
	FixJumpDestination
		( pNestSwitch->m_cpJumpNext, GetCurrentCodePointer( true ) ) ;

	// case 値評価
	assert( pNestSwitch->m_var != nullptr ) ;

	LExprValuePtr	xvalCase = EvaluateExpression( sparsSrc, pnslLocal, L":" ) ;
	LExprValuePtr	xvalCond = 
		EvalBinaryOperator
			( std::move(xvalCase), Symbol::opEqual, pNestSwitch->m_var ) ;

	pNestSwitch->m_cpJumpNext = ExprCodeJumpNonIf( std::move(xvalCond) ) ;

	if ( sparsSrc.HasNextChars( L":" ) != L':' )
	{
		OnError( errorNotFoundSyntax_opt1, L":" ) ;
	}
	if ( HasErrorOnCurrent() )
	{
		sparsSrc.PassStatement( L":", L";}" ) ;
		return ;
	}

	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;
}

// default
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_default
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L":", L";}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;
	m_ctx->m_curNest->m_returned = false ;

	CodeNestPtr	pNestSwitch = m_ctx->m_curNest ;
	if ( pNestSwitch->m_type != CodeNest::ctrlSwitch )
	{
		OnError( errorAssertDefaultInSwitchBlock ) ;
		sparsSrc.PassStatement( L":", L";}" ) ;
		return ;
	}
	if ( pNestSwitch->m_cpDefault != nullptr )
	{
		OnError( errorDoubleDefinitionOfLabel, L"default" ) ;
	}
	else
	{
		pNestSwitch->m_cpDefault = GetCurrentCodePointer( true ) ;
	}
	if ( sparsSrc.HasNextChars( L":" ) != L':' )
	{
		OnError( errorNotFoundSyntax_opt1, L":" ) ;
		sparsSrc.PassStatement( L":", L";}" ) ;
	}
}

// break
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_break
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;

	if ( sparsSrc.HasNextChars( L";" ) != L';' )
	{
		ParseStatement_goto( sparsSrc, rwIndex, pnslLocal ) ;
		return ;
	}

	// break するブロックを探す
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( (pNest->m_type == CodeNest::ctrlWhile)
			|| (pNest->m_type == CodeNest::ctrlDo)
			|| (pNest->m_type == CodeNest::ctrlFor)
			|| (pNest->m_type == CodeNest::ctrlSwitch) )
		{
			break ;
		}
		pNest = pNest->m_prev ;
	}
	if ( pNest == nullptr )
	{
		OnError( errorCannotBreakBlock ) ;
	}
	else
	{
		pNest->m_vecBreaks.push_back( ExprCodeJump() ) ;
	}
}

// continue
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_continue
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;

	// continue するブロックを探す
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( (pNest->m_type == CodeNest::ctrlWhile)
			|| (pNest->m_type == CodeNest::ctrlDo)
			|| (pNest->m_type == CodeNest::ctrlFor) )
		{
			break ;
		}
		pNest = pNest->m_prev ;
	}
	if ( pNest == nullptr )
	{
		OnError( errorCannotContinueBlock ) ;
	}
	else
	{
		if ( pNest->m_cpContinue != nullptr )
		{
			ExprCodeJump( pNest->m_cpContinue ) ;
		}
		else
		{
			pNest->m_vecContinues.push_back( ExprCodeJump() ) ;
		}
	}

	HasSemicolonForEndOfStatement( sparsSrc ) ;
}

// goto
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_goto
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;

	LString	strLabelName ;
	sparsSrc.NextToken( strLabelName ) ;
	if ( !IsValidUserName( strLabelName.c_str() ) )
	{
		OnError( errorConnotAsUserSymbol_opt1, strLabelName.c_str() ) ;
		if ( strLabelName != L";" )
		{
			sparsSrc.PassStatement( L";", L"}" ) ;
		}
		return ;
	}
	HasSemicolonForEndOfStatement( sparsSrc ) ;

	auto	iter = m_ctx->m_mapLabels.find( strLabelName.c_str() ) ;
	if ( iter != m_ctx->m_mapLabels.end() )
	{
		ExprCodeJump( iter->second.m_cpLabel ) ;
	}
	else
	{
		m_ctx->m_jumpForwards.push_back
			( ForwardJumpPoint( ExprCodeJump(), strLabelName.c_str() ) ) ;
	}
}

// try
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_try
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	CodeNestPtr	pNestTry = PushNest( CodeNest::ctrlTry ) ;
	NestPopper	popTry( *this, pNestTry ) ;

	// 構造化例外構造をスタック上に構築する
	LCodeBuffer::ImmediateOperand	immop ;
	immop.value.longValue = LCodeBuffer::ExceptNoHandler ;

	assert( LCodeBuffer::ExceptDescHandler == 0 ) ;
	CodePointPtr	cpPushExcept = GetCurrentCodePointer( false ) ;
	AddCode( LCodeBuffer::codeLoadImm, 0, 0, LType::typeInt64, 0, &immop ) ;

	assert( LCodeBuffer::ExceptDescFinally == 1 ) ;
	CodePointPtr	cpPushFinally = GetCurrentCodePointer( false ) ;
	AddCode( LCodeBuffer::codeLoadImm, 0, 0, LType::typeInt64, 0, &immop ) ;

	AddCode( LCodeBuffer::codeEnterTry ) ;

	pNestTry->MakeFrame()->AddUsedSize( LCodeBuffer::ExceptDescSize ) ;
	pNestTry->m_cpPushExcept = cpPushExcept ;
	pNestTry->m_cpPushFinally = cpPushFinally ;

	// try ブロック
	ParseOneStatement( sparsSrc, pnslLocal ) ;

	pNestTry->m_vecBreaks.push_back( ExprCodeJump() ) ;

	// catch ブロック
	CodePointPtr	cpJumpToNextCatch ;
	while ( sparsSrc.HasNextToken
			( GetReservedWordDesc(Symbol::rwiCatch).pwszName /*L"catch"*/ ) )
	{
		// catch チェーン
		CodePointPtr	cpExceptHandler = GetCurrentCodePointer( true ) ;
		if ( cpJumpToNextCatch != nullptr )
		{
			FixJumpDestination( cpJumpToNextCatch, cpExceptHandler ) ;
			cpJumpToNextCatch = nullptr ;
		}
		else
		{
			LCodeBuffer::Word *	pWord = GetCodeAt( cpPushExcept ) ;
			assert( pWord != nullptr ) ;
			pWord->imop.value.longValue = cpExceptHandler->m_pos ;
			m_ctx->m_jumpPoints.push_back
					( JumpPoint( cpPushExcept, cpExceptHandler ) ) ;
		}
		pNestTry->m_frame->TruncateUsedSize( LCodeBuffer::ExceptDescSize ) ;

		CodeNestPtr	pNestCatch = PushNest( CodeNest::ctrlCatch ) ;
		NestPopper	popCatch( *this, pNestCatch ) ;

		if ( sparsSrc.HasNextChars( L"(" ) == L'(' )
		{
			// catch 条件判定
			LType	typeCatch ;
			sparsSrc.NextTypeExpr( typeCatch, true, m_vm, pnslLocal ) ;

			if ( !HasErrorOnCurrent() )
			{
				typeCatch = GetLocalTypeOf( typeCatch ) ;
				if ( !typeCatch.IsRuntimeObject() )
				{
					OnError( errorCannotCatchType,
							typeCatch.GetTypeName().c_str() ) ;
				}
			}
			if ( !HasErrorOnCurrent() )
			{
				// (type) except
				LCodeBuffer::ImmediateOperand	immop ;
				immop.pClass = typeCatch.GetClass() ;

				AddCode( LCodeBuffer::codeLoadException ) ;
				AddCode( LCodeBuffer::codeCastObject, 0, 0, 0, 1, &immop ) ;

				LExprValuePtr	xvalCatched =
					std::make_shared<LExprValue>( typeCatch, nullptr, false, false )  ;
				PushExprValueOnStack( xvalCatched ) ;

				// キャスト結果が null かテスト
				immop.value.longValue = 0 ;
				AddCode( LCodeBuffer::codeLoadImm, 0, 0, LType::typeObject, 0, &immop ) ;

				immop.pfnOp2 = &LClass::operator_equal ;
				AddCode( LCodeBuffer::codeBinaryOperate,
									1, 0, Symbol::opEqual, 1, &immop ) ;

				cpJumpToNextCatch = GetCurrentCodePointer( false ) ;
				immop.value.longValue = 0 ;
				AddCode( LCodeBuffer::codeJumpConditional, 0, 0, 0, 1, &immop ) ;

				ExprTreatObjectChain( xvalCatched ) ;

				AddCode( LCodeBuffer::codeClearException ) ;

				// 変数名
				LString	strCatchedName ;
				if ( sparsSrc.HasNextChars( L")" ) != L')' )
				{
					sparsSrc.NextToken( strCatchedName ) ;

					if ( sparsSrc.HasNextChars( L")" ) != L')' )
					{
						OnError( errorMismatchParentheses ) ;
						sparsSrc.PassStatement( L")", L"{}" ) ;
					}
					else if ( !IsValidUserName( strCatchedName.c_str() ) )
					{
						OnError( errorConnotAsUserSymbol_opt1,
									strCatchedName.c_str() ) ;
					}
				}

				// 変数定義
				if ( strCatchedName.IsEmpty() )
				{
					MakeTempStackIntoLocalVar( std::move(xvalCatched) ) ;
				}
				else
				{
					MakeTempStackIntoLocalVarAs
						( strCatchedName.c_str(), std::move(xvalCatched) ) ;
				}
			}
			else
			{
				sparsSrc.PassStatement( L")", L"{}" ) ;
			}
		}
		else
		{
			AddCode( LCodeBuffer::codeClearException ) ;
		}

		// catch ブロック
		bool flagTryReturned = pNestTry->m_returned ;
		ParseOneStatement( sparsSrc, pnslLocal ) ;

		pNestTry->m_vecBreaks.push_back( ExprCodeJump() ) ;

		popCatch.Pop() ;
		pNestTry->m_returned = (pNestTry->m_returned && flagTryReturned) ;
	}

	// 例外がハンドルされなかった場合
	if ( cpJumpToNextCatch != nullptr )
	{
		FixJumpDestination( cpJumpToNextCatch, GetCurrentCodePointer( true ) ) ;
		cpJumpToNextCatch = nullptr ;

		AddCode( LCodeBuffer::codeLoadException ) ;
		AddCode( LCodeBuffer::codeRefObject ) ;
		AddCode( LCodeBuffer::codeThrow ) ;
	}

	// finally ブロック
	if ( sparsSrc.HasNextToken
		( GetReservedWordDesc(Symbol::rwiFinally).pwszName /*L"finally"*/ ) )
	{
		// finally ハンドラ設定
		CodePointPtr		cpFinallyHandler = GetCurrentCodePointer( true ) ;
		LCodeBuffer::Word *	pWord = GetCodeAt( cpPushFinally ) ;
		assert( pWord != nullptr ) ;
		pWord->imop.value.longValue = cpFinallyHandler->m_pos ;
		m_ctx->m_jumpPoints.push_back
					( JumpPoint( cpPushFinally, cpFinallyHandler ) ) ;

		size_t	nTryUsedSize = pNestTry->m_frame->GetUsedSize() ;
		if ( nTryUsedSize > LCodeBuffer::ExceptDescSize
									+ LCodeBuffer::FinallyRetStackSize )
		{
			pNestTry->m_frame->TruncateUsedSize
				( LCodeBuffer::ExceptDescSize + LCodeBuffer::FinallyRetStackSize ) ;
		}
		else
		{
			pNestTry->m_frame->AddUsedSize
				( LCodeBuffer::ExceptDescSize
						+ LCodeBuffer::FinallyRetStackSize - nTryUsedSize ) ;
		}

		// finally ブロック
		CodeNestPtr	pNestFinally = PushNest( CodeNest::ctrlFinally ) ;
		NestPopper	popFinally( *this, pNestFinally ) ;
		bool flagTryReturned = pNestTry->m_returned ;

		ParseOneStatement( sparsSrc, pnslLocal ) ;

		// finally ブロック離脱
		ExprCodeFreeTempStack() ;
		assert( m_ctx->m_xvaStack.size() == 0 ) ;

		if ( pNestFinally->m_frame != nullptr )
		{
			AddCode( LCodeBuffer::codeFreeStack,
						0, 0, 0, pNestFinally->m_frame->GetUsedSize() ) ;
		}
		AddCode( LCodeBuffer::codeRetFinally ) ;

		pNestTry->m_vecBreaks.push_back( ExprCodeJump() ) ;
		popFinally.Pop() ;
		pNestTry->m_returned = flagTryReturned ;
	}
	popTry.Pop() ;
}

// throw
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_throw
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;
	m_ctx->m_curNest->m_returned = true ;

	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		// throw null;
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.pObject = nullptr ;
		AddCode( LCodeBuffer::codeLoadImm, 0, 0, LType::typeObject, 0, &immop ) ;
		AddCode( LCodeBuffer::codeThrow ) ;
		return ;
	}

	LExprValuePtr	xval =
			EvaluateExpression( sparsSrc, pnslLocal, L";" ) ;

	HasSemicolonForEndOfStatement( sparsSrc ) ;

	xval = ExprPushClone( ExprPrepareToPushClone( std::move(xval), true ), true ) ;
	if ( !xval->GetType().IsObject() )
	{
		OnError( errorCannotThrowType,
					xval->GetType().GetTypeName().c_str() ) ;
	}
	AddCode( LCodeBuffer::codeThrow ) ;
}

// return
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_return
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	assert( m_ctx != nullptr ) ;
	assert( m_ctx->m_curNest != nullptr ) ;
	m_ctx->m_curNest->m_returned = true ;

	if ( !m_ctx->m_proto->GetReturnType().IsVoid() )
	{
		// 返り値評価
		LExprValuePtr	xval =
				EvaluateExpression( sparsSrc, pnslLocal, L";" ) ;
		xval = EvalCastTypeTo( std::move(xval),
								m_ctx->m_proto->GetReturnType() ) ;
		xval = ExprMakeOnStack( std::move(xval) ) ;

		if ( xval->IsFetchAddrPointer() )
		{
			OnError( errorCannotFetchAddrToRet ) ;
		}

		if ( !HasErrorOnCurrent() )
		{
			// 返り値設定
			const ssize_t	iStack = GetBackIndexOnStack( xval ) ;
			assert( iStack >= 0 ) ;
			if ( xval->IsTypeNeededToRelease() )
			{
				AddCode( LCodeBuffer::codeRefObject, iStack ) ;
			}

			LType::Primitive	type = xval->GetType().GetPrimitive() ;
			xval =nullptr ;

			const size_t	nFreeCount = CountFreeTempStack() ;
			PopExprValueOnStacks( nFreeCount ) ;

			AddCode( LCodeBuffer::codeSetRetValue,
							iStack, 0, type, nFreeCount ) ;
		}
		HasSemicolonForEndOfStatement( sparsSrc ) ;
	}
	else if ( sparsSrc.HasNextChars( L";" ) != L';' )
	{
		OnError( errorNotFoundSemicolonEndingAfterReturn ) ;
		sparsSrc.PassStatement( L";", L"}" ) ;
	}

	ExprCodeFreeTempStack() ;

	// try ブロック離脱
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( (pNest->m_type == CodeNest::ctrlTry)
			|| (pNest->m_type == CodeNest::ctrlTry) )
		{
			AddCode( LCodeBuffer::codeCallFinally ) ;
			if ( pNest->m_type == CodeNest::ctrlCatch )
			{
				pNest = pNest->m_prev ;
				assert( pNest->m_type == CodeNest::ctrlTry ) ;
			}
		}
		else if ( pNest->m_type == CodeNest::ctrlFinally )
		{
			AddCode( LCodeBuffer::codeLeaveFinally ) ;
			pNest = pNest->m_prev ;
			assert( pNest->m_type == CodeNest::ctrlTry ) ;
		}
		pNest = pNest->m_prev ;
	}
	AddCode( LCodeBuffer::codeLeaveFunc ) ;
	AddCode( LCodeBuffer::codeReturn,
				0, 0, 0, m_ctx->m_proto->GetCaptureObjectCount() ) ;
}

// with
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_with
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	CodeNestPtr	pNestWith = PushNest( CodeNest::ctrlWith ) ;
	NestPopper	popWith( *this, pNestWith ) ;

	// with ( expr )
	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNotFoundSyntax_opt1, L"(" ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	LExprValuePtr	xvalWith =
			EvaluateExpression( sparsSrc, pnslLocal, L")" ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}

	// ローカル変数として確保
	if ( xvalWith->IsConstExpr() )
	{
		pNestWith->m_xvalObj = xvalWith ;
	}
	else
	{
		xvalWith = ExprMakeOnStack( std::move(xvalWith) ) ;
		pNestWith->m_xvalObj =
				MakeTempStackIntoLocalVar( std::move(xvalWith) ) ;
	}

	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	// with ブロック
	ParseOneStatement( sparsSrc, pnslLocal ) ;

	popWith.Pop() ;
}

// synchronized
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_synchronized
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		ParseDeclarationAndDefinition
			( sparsSrc, LType::modifierSynchronized, pnslLocal ) ;
		return ;
	}
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	ExprCodeFreeTempStack() ;
	assert( m_ctx->m_xvaStack.size() == 0 ) ;

	CodeNestPtr	pNestSync = PushNest( CodeNest::ctrlSynchronized ) ;
	NestPopper	popSync( *this, pNestSync ) ;

	// synchronized ( expr )
	LExprValuePtr	xvalSync =
			EvaluateExpression( sparsSrc, pnslLocal, L")" ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	xvalSync = ExprMakeOnStack( std::move(xvalSync) ) ;

	LLocalVarPtr	pSyncVar =
			MakeTempStackIntoLocalVar( std::move(xvalSync) ) ;
	pNestSync->m_xvalObj = pSyncVar ;

	if ( !pNestSync->m_xvalObj->GetType().IsRuntimeObject() )
	{
		OnError( errorCannotSynchronizeType,
				pNestSync->m_xvalObj->GetType().GetTypeName().c_str() ) ;
	}
	else
	{
		// sync 実行
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = pSyncVar->GetLocation() ;
		AddCode( LCodeBuffer::codeSynchronize, 0, 0, 0, 0, &immop ) ;

		pNestSync->MakeFrame()->AddUsedSize( 2 ) ;
	}

	// synchronized ブロック
	ParseOneStatement( sparsSrc, pnslLocal ) ;

	popSync.Pop() ;
}

// 宣言文か式文 : const, void, boolean, byte, short, char, int, long,
//					float, double, int8, uint8, int16, uint16,
//					int32, uint32, int64
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_def_expr
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	sparsSrc.SeekIndex( m_iSrcStatement ) ;
	ParseDefinitionOrExpression( sparsSrc, pnslLocal ) ;
}

// 宣言文 : static, abstract, native, fetch_addr,
//			public, protected, private, @override, @deprecated,
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_definition
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	ParseDeclarationAndDefinition
		( sparsSrc, GetReservedWordDesc(rwIndex).mapValue, pnslLocal ) ;
}

// 宣言文 : auto
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_definition_auto
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	ParseDefinitionAsAutoVar( sparsSrc, 0, pnslLocal ) ;
}

// 式文 : function, operator,
//			this, super, global, null, false, true
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_expr
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	sparsSrc.SeekIndex( m_iSrcStatement ) ;

	EvaluateExpression( sparsSrc, pnslLocal, L";" ) ;

	ExprCodeFreeTempStack() ;
	HasSemicolonForEndOfStatement( sparsSrc );
}

// 構文エラー : extends, implements, in, else, catch, finally,
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseStatement_syntax_error
	( LStringParser& sparsSrc,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	OnError( errorSyntaxError, GetReservedWordDesc(rwIndex).pwszName ) ;
}

// 宣言文か式文
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseDefinitionOrExpression
	( LStringParser& sparsSrc, const LNamespaceList * pnslLocal )
{
	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		return ;
	}
	const size_t	iStartExpr = sparsSrc.GetIndex() ;

	if ( ParseConstructer( sparsSrc, 0, pnslLocal ) )
	{
		return ;
	}

	LType	type ;
	if ( sparsSrc.NextTypeExpr( type, false, m_vm, pnslLocal ) )
	{
		const size_t	iSaveIndex = sparsSrc.GetIndex() ;
		LString			strNext ;
		sparsSrc.NextToken( strNext ) ;

		if ( (AsOperatorIndex(strNext.c_str()) == Symbol::opInvalid)
			&& (AsReservedWordIndex(strNext.c_str()) == Symbol::rwiInvalid) )
		{
			// 宣言・定義文
			sparsSrc.SeekIndex( iSaveIndex ) ;
			ParseDeclarationAndDefinition( sparsSrc, 0, type, pnslLocal ) ;
			return ;
		}
	}

	// 式文
	sparsSrc.SeekIndex( iStartExpr ) ;

	EvaluateExpression( sparsSrc, pnslLocal, L";" ) ;

	ExprCodeFreeTempStack() ;
	HasSemicolonForEndOfStatement( sparsSrc );
}

// 変数又は関数の宣言・定義文
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseDeclarationAndDefinition
	( LStringParser& sparsSrc,
		LType::Modifiers accModLeft, const LNamespaceList * pnslLocal )
{
	accModLeft = ParseAccessModifiers
					( sparsSrc, accModLeft, LType::modifierConst ) ;

	if ( ParseConstructer( sparsSrc, accModLeft, pnslLocal ) )
	{
		return ;
	}

	LType	type ;
	if ( !sparsSrc.NextTypeExpr( type, true, m_vm, pnslLocal ) )
	{
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	type.SetModifiers( type.GetModifiers() ) ;

	ParseDeclarationAndDefinition( sparsSrc, accModLeft, type, pnslLocal ) ;
}

void LCompiler::ParseDeclarationAndDefinition
	( LStringParser& sparsSrc, LType::Modifiers accModLeft,
		const LType& type, const LNamespaceList * pnslLocal )
{
	LType	typeVar = type ;
	typeVar.SetModifiers
		( DefaultAccessModifiers( type.GetModifiers() | accModLeft ) ) ;

	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	if ( pNamespace != nullptr )
	{
		typeVar.SetComment( pNamespace->MakeComment( m_commentBefore.c_str() ) ) ;
	}

	for ( ; ; )
	{
		// 変数名
		LString	strVarName ;
		sparsSrc.NextToken( strVarName ) ;
		if ( !IsValidUserName( strVarName.c_str() ) )
		{
			if ( strVarName == GetReservedWordDesc(Symbol::rwiOperator).pwszName )
			{
				// operator ? ()
				ParseOperatorFuncDeclaration( sparsSrc, accModLeft, type, pnslLocal ) ;
				return ;
			}
			else
			{
				OnError( errorConnotAsUserSymbol_opt1, strVarName.c_str() ) ;
				sparsSrc.PassStatement( L";", L"}" ) ;
				return ;
			}
		}

		if ( sparsSrc.HasNextChars( L"(" ) == L'(' )
		{
			if ( GetCurrentNamespace() != nullptr )
			{
				// メンバ関数
				ParseMemberFunction
					( sparsSrc, accModLeft, type, strVarName.c_str(), pnslLocal ) ;
				return ;
			}
			else
			{
				// ローカル変数の構築
				ParseLocalVariableDefinition
					( typeVar, strVarName.c_str(), sparsSrc, pnslLocal, true ) ;
			}
		}
		else
		{
			if ( GetCurrentNamespace() != nullptr )
			{
				// メンバ変数
				ParseMemberVariable
					( sparsSrc, GetCurrentNamespace(), 
						typeVar, strVarName.c_str(), pnslLocal, L",;" ) ;
			}
			else
			{
				// ローカル変数の初期化
				ParseLocalVariableDefinition
					( typeVar, strVarName.c_str(), sparsSrc, pnslLocal, false ) ;
			}
		}
		if ( HasErrorOnCurrent() )
		{
			return ;
		}
		wchar_t	wchNext = sparsSrc.HasNextChars( L",;" ) ;
		if ( wchNext == L';' )
		{
			break ;
		}
		if ( wchNext != L',' )
		{
			OnError( errorNotFoundSyntax_opt1, L"," ) ;
			sparsSrc.PassStatement( L";", L"}" ) ;
			return ;
		}
		typeVar.SetComment( nullptr ) ;
		m_commentBefore = L"" ;

		ExprCodeFreeTempStack() ;
	}
}

// operator 関数宣言・定義
void LCompiler::ParseOperatorFuncDeclaration
	( LStringParser& sparsSrc, LType::Modifiers accModFunc,
		const LType& typeRet, const LNamespaceList * pnslLocal )
{
	// type operator ? ( arg-list )
	LString	strOp ;
	sparsSrc.NextToken( strOp ) ;

	Symbol::OperatorIndex	opIndex = AsOperatorIndex( strOp.c_str() ) ;
	if ( opIndex == Symbol::opInvalid )
	{
		OnError( errorSyntaxError ) ;
		sparsSrc.PassStatement( L";}", L"{" ) ;
		return ;
	}
	const Symbol::OperatorDesc&	opDesc = GetOperatorDesc( opIndex ) ;
	if ( opDesc.pwszPairName != nullptr )
	{
		LString	strOpPair ;
		sparsSrc.NextToken( strOpPair ) ;

		if ( strOpPair != opDesc.pwszPairName )
		{
			OnError( errorNotFoundSyntax_opt1, opDesc.pwszPairName ) ;
			sparsSrc.PassStatement( L";}", L"{" ) ;
			return ;
		}
	}

	// operator 関数名
	strOp = GetReservedWordDesc(Symbol::rwiOperator).pwszName ;
	strOp += L" " ;
	strOp += opDesc.pwszName ;
	if ( opDesc.pwszPairName != nullptr )
	{
		strOp += opDesc.pwszPairName ;
	}

	// 関数を定義する名前空間・クラス
	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	if ( pNamespace == nullptr )
	{
		OnError( errorUnavailableDefineFunc, strOp.c_str() ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	LClass *	pClass = nullptr ;
	if ( !(accModFunc & LType::modifierStatic) )
	{
		pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
	}

	// 引数解釈
	if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNothingArgmentList, strOp.c_str() ) ;
		sparsSrc.PassStatement( L";}", L"{" ) ;
		return ;
	}
	std::shared_ptr<LPrototype>	pProto =
			std::make_shared<LPrototype>( pClass, accModFunc ) ;
	pProto->SetComment( pNamespace->MakeComment( m_commentBefore.c_str() ) ) ;
	m_commentBefore = L"" ;

	ParsePrototypeArgmentList( sparsSrc, *pProto, pnslLocal, L")" ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatement( L")", L"{}" ) ;
	}

	// 返り値型設定
	pProto->ReturnType() = GetArgumentTypeOf( typeRet ) ;

	// 関数の後置修飾
	ParsePostFunctionModifiers( sparsSrc, pProto ) ;

	// 関数登録
	LPtr<LFunctionObj>	pFunc =
			new LFunctionObj( m_vm.GetFunctionClassAs( pProto ), pProto ) ;
	if ( pClass != nullptr )
	{
		auto [iVirtFunc, pOverrided] =
			pClass->OverrideVirtualFunction( strOp.c_str(), pFunc ) ;
		if ( (pOverrided == nullptr)
			&& (accModFunc & LType::modifierOverride) )
		{
			OnWarning( warningFunctionNotOverrided, warning2, strOp.c_str() ) ;
		}
		if ( (pProto->GetArgListType().size() == 0)
			&& (opDesc.priorityUnary != Symbol::priorityNo) )
		{
			// 単項演算子登録
			Symbol::UnaryOperatorDef	uopd ;
			uopd.m_opClass = Symbol::operatorUnary ;
			uopd.m_opIndex = opIndex ;
			uopd.m_pwszComment = nullptr ;
			uopd.m_constExpr = false ;
			uopd.m_constThis = pProto->IsConstThis() ;
			uopd.m_typeRet = typeRet ;
			uopd.m_pThisClass = pClass ;
			uopd.m_pfnOp = nullptr ;
			uopd.m_pInstance = nullptr ;
			uopd.m_pOpFunc = pFunc.Ptr() ;
			pClass->AddUnaryOperatorDef( uopd ) ;
		}
		else if ( (pProto->GetArgListType().size() == 1)
				&& (opDesc.priorityBinary != Symbol::priorityNo) )
		{
			// 二項演算子登録
			Symbol::BinaryOperatorDef	bopd ;
			bopd.m_opClass = Symbol::operatorBinary ;
			bopd.m_opIndex = opIndex ;
			bopd.m_pwszComment = nullptr ;
			bopd.m_constExpr = false ;
			bopd.m_constThis = pProto->IsConstThis() ;
			bopd.m_typeRet = typeRet ;
			bopd.m_pThisClass = pClass ;
			bopd.m_pfnOp = nullptr ;
			bopd.m_typeRight = pProto->GetArgListType().GetArgTypeAt(0) ;
			bopd.m_pInstance = nullptr ;
			bopd.m_pOpFunc = pFunc.Ptr() ;
			pClass->AddBinaryOperatorDef( bopd, bopd.m_typeRight ) ;
		}
	}
	else
	{
		LPtr<LFunctionObj>	pOverrided =
				pNamespace->AddStaticFunctionAs( strOp.c_str(), pFunc ) ;
		if ( (pOverrided == nullptr)
			&& (accModFunc & LType::modifierOverride) )
		{
			OnWarning( warningFunctionNotOverrided, warning2, strOp.c_str() ) ;
		}
	}

	// 関数実装解釈
	ParseFunctionImplementation( sparsSrc, pFunc, pnslLocal ) ;
}

// auto 変数定義
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseDefinitionAsAutoVar
	( LStringParser& sparsSrc,
		LType::Modifiers accModLeft, const LNamespaceList * pnslLocal )
{
	// 変数名
	LString	strVarName ;
	sparsSrc.NextToken( strVarName ) ;
	if ( !IsValidUserName( strVarName.c_str() ) )
	{
		OnError( errorConnotAsUserSymbol_opt1, strVarName.c_str() ) ;
		if ( strVarName!= L";" )
		{
			sparsSrc.PassStatement( L";", L"}" ) ;
		}
		return ;
	}

	// 初期値式
	if ( sparsSrc.HasNextChars( L"=" ) != L'=' )
	{
		OnError( errorNoInitExprForAutoVar, strVarName.c_str() ) ;
		sparsSrc.PassStatement( L";", L"}" ) ;
		return ;
	}
	LExprValuePtr	xvalInit = EvaluateExpression( sparsSrc, pnslLocal, L";" ) ;
	xvalInit = EvalMakeInstance( std::move(xvalInit) ) ;

	HasSemicolonForEndOfStatement( sparsSrc ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}

	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	if ( pNamespace != nullptr )
	{
		// namespace か class, struct メンバ
		if ( !xvalInit->IsConstExpr() )
		{
			OnError( errorNotConstExprInitExpr, strVarName.c_str() ) ;
			return ;
		}
		LType	typeVar = xvalInit->GetType() ;
		typeVar.SetModifiers
			( (typeVar.GetModifiers() & ~LType::accessMask) | accModLeft ) ;
		typeVar = DefaultAccessModifierTypeOf( typeVar ) ;
		typeVar.SetComment( pNamespace->MakeComment( m_commentBefore.c_str() ) ) ;
		m_commentBefore = L"" ;

		DefineMemberVariable
			( pNamespace, typeVar, strVarName.c_str(), xvalInit ) ;
	}
	else
	{
		// ローカル変数
		if ( !MustBeRuntimeCode() )
		{
			return ;
		}
		MakeTempStackIntoLocalVarAs
			( strVarName.c_str(), std::move(xvalInit) ) ;
	}
}

// namespace, class, struct メンバ修飾子
///////////////////////////////////////////////////////////////////////////////
LType LCompiler::DefaultAccessModifierTypeOf( const LType& type ) const
{
	LType	typeDef = type ;
	typeDef.SetModifiers( DefaultAccessModifiers( type.GetModifiers() ) ) ;
	return	typeDef ;
}

LType::Modifiers LCompiler::DefaultAccessModifiers( LType::Modifiers accMod ) const
{
	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	if ( pNamespace != nullptr )
	{
		LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
		if ( pClass != nullptr )
		{
			if ( (accMod & LType::accessMask) == LType::modifierDefault )
			{
				accMod &= ~LType::accessMask ;
				if ( dynamic_cast<LStructureClass*>( pClass ) != nullptr )
				{
					accMod |= LType::modifierPublic ;
				}
				else
				{
					accMod |= LType::modifierPrivate ;
				}
			}
		}
		else
		{
			if ( (accMod & LType::accessMask) == LType::modifierDefault )
			{
				accMod = (accMod & ~LType::accessMask) | LType::modifierPublic ;
			}
			accMod |= LType::modifierStatic ;
		}
	}
	return	accMod ;
}

// メンバ変数定義
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseMemberVariable
	( LStringParser& sparsSrc,
		LPtr<LNamespace> pNamespace,
		const LType& typeVar, const wchar_t * pwszName,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	assert( pNamespace != nullptr ) ;
	LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;

	LObjPtr					pRefObj ;
	LArrangementBuffer *	pArrange = nullptr ;

	if ( typeVar.IsModifiered( LType::modifierStatic ) || (pClass == nullptr) )
	{
		// static 変数
		if ( typeVar.CanArrangeOnBuf() )
		{
			// 非オブジェクト変数
			DefineVariableOnArrangeBuf
				( pNamespace->StaticArrangement(), typeVar, pwszName, nullptr ) ;
			pArrange = &(pNamespace->StaticArrangement()) ;
		}
		else
		{
			// オブジェクト変数
			DefineStaticVariable( pNamespace, typeVar, pwszName, nullptr ) ;
			pRefObj = pNamespace ;
		}
	}
	else
	{
		// メンバ変数
		if ( typeVar.CanArrangeOnBuf() )
		{
			// 非オブジェクト変数
			DefineVariableOnArrangeBuf
				( pClass->ProtoArrangemenet(), typeVar, pwszName, nullptr ) ;
			pArrange = &(pClass->ProtoArrangemenet()) ;
		}
		else
		{
			// オブジェクト変数
			if ( dynamic_cast<LStructureClass*>( pClass ) != nullptr )
			{
				OnError( errorObjectCannotBePlacedInStruct,
						pwszName, typeVar.GetTypeName().c_str() ) ;
			}
			DefineObjectMemberVariable
				( pClass->GetPrototypeObject(), typeVar, pwszName, nullptr ) ;
			pRefObj = pClass->GetPrototypeObject() ;
		}
	}

	if ( sparsSrc.HasNextChars( L"=" ) == L'=' )
	{
		// 初期値式
		const size_t	iStartInitExpr = sparsSrc.GetIndex() ;
		sparsSrc.PassStatement( L"", pwszEscChars ) ;

		LParserSegment	ps( &sparsSrc, iStartInitExpr, sparsSrc.GetIndex() ) ;
//		m_ctx->m_curNest->m_vecDelayInitVars.push_back
//			( std::make_shared<DelayInitVar>
//					( ps, pRefObj, pArrange, pwszName, pnslLocal ) ) ;

		DelayInitVar	diVar( ps, pRefObj, pArrange, pwszName, pnslLocal ) ;
		ParseDelayInitVar( diVar ) ;
	}
}

void LCompiler::DefineMemberVariable
	( LPtr<LNamespace> pNamespace,
		const LType& typeVar,
		const wchar_t * pwszName, LExprValuePtr xvalInit )
{
	assert( pNamespace != nullptr ) ;
	LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;

	if ( typeVar.IsModifiered( LType::modifierStatic ) )
	{
		// static 変数
		if ( typeVar.CanArrangeOnBuf() )
		{
			// 非オブジェクト変数
			DefineVariableOnArrangeBuf
				( pNamespace->StaticArrangement(), typeVar, pwszName, xvalInit ) ;
		}
		else
		{
			// オブジェクト変数
			DefineStaticVariable
				( pNamespace, typeVar, pwszName, xvalInit ) ;
		}
	}
	else
	{
		// メンバ変数
		assert( pClass != nullptr ) ;
		if ( typeVar.CanArrangeOnBuf() )
		{
			// 非オブジェクト変数
			DefineVariableOnArrangeBuf
				( pClass->ProtoArrangemenet(), typeVar, pwszName, xvalInit ) ;
		}
		else
		{
			// オブジェクト変数
			if ( dynamic_cast<LStructureClass*>( pClass ) != nullptr )
			{
				OnError( errorObjectCannotBePlacedInStruct,
						pwszName, typeVar.GetTypeName().c_str() ) ;
			}
			DefineObjectMemberVariable
				( pClass->GetPrototypeObject(), typeVar, pwszName, xvalInit ) ;
		}
	}
}

void LCompiler::DefineVariableOnArrangeBuf
	( LArrangementBuffer& arrangeBuf,
		const LType& typeVar,
		const wchar_t * pwszName, LExprValuePtr xvalInit )
{
	if ( arrangeBuf.IsExistingAs( pwszName ) )
	{
		OnError( errorDoubleDefinitionOfMember, pwszName ) ;
		return ;
	}
	if ( typeVar.IsFetchAddr() )
	{
		OnWarning( warningInvalidFetchAddr, warning1, pwszName ) ;
	}
	if ( !typeVar.CanArrangeOnBuf() )
	{
		OnError( errorCannotArrangeObject, pwszName ) ;
	}
	arrangeBuf.AddDesc( pwszName, typeVar ) ;

	LArrangement::Desc	desc ;
	if ( !arrangeBuf.GetDescAs( desc, pwszName ) )
	{
		OnError( errorDoubleDefinitionOfMember, pwszName ) ;
		return ;
	}
	arrangeBuf.AllocateBuffer() ;

	if ( desc.m_type.IsStructure() )
	{
		LStructureClass *	pStructClass = desc.m_type.GetStructureClass() ;
		assert( pStructClass != nullptr ) ;
		if ( pStructClass != nullptr )
		{
			const LArrangementBuffer&
					protoBuf = pStructClass->GetProtoArrangemenet();
			assert( arrangeBuf.GetBuffer()->GetBytes() >= desc.m_location + desc.m_size ) ;
			assert( protoBuf.GetAlignedBytes() == pStructClass->GetStructBytes() ) ;
			const size_t	nStructBytes = pStructClass->GetStructBytes() ;
			uint8_t *		pBuf = arrangeBuf.GetBuffer()->GetBuffer()
														+ desc.m_location ;
			for ( size_t i = 0; i < desc.m_size; i += nStructBytes )
			{
				size_t	nBytes = std::min( (desc.m_size - i), nStructBytes ) ;
				memcpy( pBuf + i, protoBuf.GetBuffer()->GetBuffer(), nBytes ) ;
			}
		}
	}

	if ( xvalInit != nullptr )
	{
		if ( !typeVar.IsPrimitive() )
		{
			OnError( errorUnavailableAutoType,
					pwszName, typeVar.GetTypeName().c_str() ) ;
			return ;
		}
		xvalInit = EvalCastTypeTo( std::move(xvalInit), typeVar ) ;
		if ( !xvalInit->IsConstExpr() )
		{
			OnError( errorNotConstExprInitExpr, pwszName ) ;
		}

		LPtr<LPointerObj>	pPtr = new LPointerObj( m_vm.GetPointerClass() ) ;
		pPtr->SetPointer( arrangeBuf.GetBuffer(), desc.m_location, desc.m_size ) ;

		if ( typeVar.IsFloatingPointNumber() )
		{
			pPtr->StoreDoubleAt
				( 0, typeVar.GetPrimitive(), xvalInit->AsDouble() ) ;
		}
		else
		{
			pPtr->StoreIntegerAt
				( 0, typeVar.GetPrimitive(), xvalInit->AsInteger() ) ;
		}
	}
}

void LCompiler::DefineStaticVariable
	( LPtr<LNamespace> pNamespace,
		const LType& typeVar,
		const wchar_t * pwszName, LExprValuePtr xvalInit )
{
	if ( typeVar.IsFetchAddr() )
	{
		OnWarning( warningInvalidFetchAddr, warning1, pwszName ) ;
	}
	if ( typeVar.CanArrangeOnBuf() )
	{
		OnError( errorCannotPutDataOnObject, pwszName ) ;
	}
	LObjPtr	pInitObj = GetObjectOfInitExpr( typeVar, pwszName, xvalInit ) ;
	if ( pNamespace->GetLocalObjectTypeAs( pwszName ) != nullptr )
	{
		OnError( errorDoubleDefinitionOfMember, pwszName ) ;
		return ;
	}
	pNamespace->DeclareObjectAs( pwszName, typeVar, pInitObj ) ;
}

void LCompiler::DefineObjectMemberVariable
	( LObjPtr pProto, const LType& typeVar,
		const wchar_t * pwszName, LExprValuePtr xvalInit )
{
	if ( pProto == nullptr )
	{
		OnError( errorCannotDefineVariable, pwszName ) ;
		return ;
	}
	if ( typeVar.IsFetchAddr() )
	{
		OnWarning( warningInvalidFetchAddr, warning1, pwszName ) ;
	}
	if ( typeVar.CanArrangeOnBuf() )
	{
		OnError( errorCannotPutDataOnObject, pwszName ) ;
	}
	LObjPtr	pInitObj = GetObjectOfInitExpr( typeVar, pwszName, xvalInit ) ;
	if ( !pProto->GetElementTypeAs( pwszName ).IsVoid() )
	{
		OnError( errorDoubleDefinitionOfMember, pwszName ) ;
		return ;
	}
	LObject::ReleaseRef
		( pProto->SetElementAs( pwszName, pInitObj.Get() ) ) ;
	pProto->SetElementTypeAs( pwszName, typeVar ) ;
}

LObjPtr LCompiler::GetObjectOfInitExpr
	( const LType& typeVar,
		const wchar_t * pwszName, LExprValuePtr xvalInit )
{
	LObjPtr	pInitObj ;
	if ( xvalInit != nullptr )
	{
		xvalInit = EvalMakeInstance
					( EvalCastTypeTo( std::move(xvalInit), typeVar ) ) ;
		if ( xvalInit != nullptr )
		{
			if ( !xvalInit->IsConstExpr() )
			{
				OnError( errorNotConstExprInitExpr, pwszName ) ;
			}
			pInitObj = xvalInit->GetObject() ;
		}
	}
	return	pInitObj ;
}

// メンバ変数初期値遅延実装
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseDelayInitVar( const LCompiler::DelayInitVar& delayInit )
{
	assert( delayInit.m_pParser != nullptr ) ;

	LStringParser&				sparsExpr = *(delayInit.m_pParser) ;
	LStringParser::StateSaver	saver( sparsExpr ) ;

	sparsExpr.SetBounds( delayInit.m_iFirst, delayInit.m_iEnd ) ;
	sparsExpr.SeekIndex( delayInit.m_iFirst ) ;

	if ( delayInit.m_pArrange != nullptr )
	{
		LArrangement::Desc	desc ;
		assert( delayInit.m_pArrange->GetDescAs( desc, delayInit.m_strName.c_str() ) ) ;
		if ( delayInit.m_pArrange->GetDescAs( desc, delayInit.m_strName.c_str() ) )
		{
			LPtr<LPointerObj>	pPtr = new LPointerObj( m_vm.GetPointerClass() ) ;
			pPtr->SetPointer
				( delayInit.m_pArrange->GetBuffer(),
								desc.m_location, desc.m_size ) ;

			ParseDataInitExpression
				( sparsExpr, desc.m_type, pPtr, delayInit.m_pnslLocal ) ;
		}
	}
	else if ( delayInit.m_pRefObj != nullptr )
	{
		LType	typeVar = delayInit.m_pRefObj->
							GetElementTypeAs( delayInit.m_strName.c_str() ) ;
		assert( !typeVar.IsVoid() ) ;

		LExprValuePtr	xval =
			EvaluateExpression( sparsExpr, delayInit.m_pnslLocal ) ;
		xval = EvalMakeInstance( std::move(xval) ) ;
		xval = EvalCastTypeTo( std::move(xval), typeVar ) ;

		if ( xval == nullptr )
		{
			OnError( errorInitExpressionError,
						delayInit.m_strName.c_str() ) ;
			return ;
		}
		if ( xval->IsConstExpr() )
		{
			LObject::ReleaseRef
				( delayInit.m_pRefObj->SetElementAs
					( delayInit.m_strName.c_str(), xval->GetObject().Get() ) ) ;
		}
		else
		{
			OnWarning( warningNotConstExprInitExpr, warning4,
						delayInit.m_strName.c_str() ) ;

			LExprValuePtr	xvalObj =
				std::make_shared<LExprValue>
					( LType(delayInit.m_pRefObj->GetClass()),
									delayInit.m_pRefObj, true, true ) ;
			LExprValuePtr	xvalRef =
				std::make_shared<LExprValue>( typeVar, nullptr, false, false ) ;
			xvalRef->SetOptionRefByStrOf
				( xvalObj, ExprLoadImmString( delayInit.m_strName ) ) ;
			ExprCodeStore( std::move(xvalRef), std::move(xval) ) ;
		}
	}
}

// ローカル変数定義
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseLocalVariableDefinition
	( const LType& typeVar,
		const wchar_t * pwszName, LStringParser& sparsSrc,
		const LNamespaceList * pnslLocal, bool flagConstructerParam )
{
	assert( GetCurrentNamespace() == nullptr ) ;
	if ( !MustBeRuntimeCode() )
	{
		sparsSrc.PassStatement
			( (flagConstructerParam ? L")" : L""), L";}" ) ;
		return ;
	}

	// ローカル変数定義
	LLocalVarPtr	pVar = ExprCodeDeclareLocalVar( pwszName, typeVar ) ;
	if ( pVar == nullptr )
	{
		sparsSrc.PassStatement
			( (flagConstructerParam ? L")" : L""), L";}" ) ;
		return ;
	}

	if ( flagConstructerParam )
	{
		// type name( arg-list )
		if ( !typeVar.IsStructure() )
		{
			// 構造体ではないので初期値式として解釈
			ParseLocalVariableInitExpr
				( sparsSrc, pVar, pnslLocal, L")" ) ;
			if ( sparsSrc.HasNextChars( L")" ) != L')' )
			{
				OnError( errorMismatchParentheses ) ;
				sparsSrc.PassStatement( L")", L"};" ) ;
			}
			return ;
		}

		LStructureClass *	pStructClass = typeVar.GetStructureClass() ;
		assert( pStructClass != nullptr ) ;

		// 引数解釈
		std::vector<LExprValuePtr>	xvalList ;
		if ( !ParseExprList( sparsSrc, xvalList, pnslLocal, L")};" ) )
		{
			assert( HasErrorOnCurrent() ) ;
			sparsSrc.PassStatement( L")", L"};" ) ;
			return ;
		}
		if ( sparsSrc.HasNextChars( L")" ) != L')' )
		{
			OnError( errorMismatchParentheses ) ;
			sparsSrc.PassStatement( L")", L"};" ) ;
			return ;
		}
		LArgumentListType	argListType ;
		GetExprListTypes( argListType, xvalList ) ;

		// 構造体の構築関数取得
		ssize_t	iInitFunc =
			pStructClass->GetVirtualVector().FindCallableFunction
				( LClass::s_Constructor, argListType,
					pStructClass, GetAccessibleScope(pStructClass) ) ;
		if ( iInitFunc < 0 )
		{
			OnError( errorNotFoundMatchConstructor,
						pStructClass->GetFullClassName().c_str(),
						argListType.ToString().c_str() ) ;
			return ;
		}

		// 構築関数呼び出し
		LPtr<LFunctionObj>	pInitFunc =
			pStructClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
		assert( pInitFunc != nullptr ) ;

		ExprCallFunction( pVar, pInitFunc, xvalList ) ;
		return ;
	}

	if ( sparsSrc.HasNextChars( L"=" ) != L'=' )
	{
		if ( typeVar.IsFetchAddr() )
		{
			OnError( errorFetchAddrVarMustHaveInitExpr, pwszName ) ;
		}
		return ;
	}

	// 初期値解釈
	ParseLocalVariableInitExpr
		( sparsSrc, pVar, pnslLocal, L",;" ) ;
}

// ローカル変数の初期値式を評価
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseLocalVariableInitExpr
	( LStringParser& sparsSrc, LExprValuePtr xvalVar,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	if ( xvalVar->GetType().IsDataArray() )
	{
		// 配列 : [ expr, expr, ... ]
		if ( sparsSrc.HasNextChars( L"[" ) != L'[' )
		{
			OnError( errorInitExpressionError ) ;
			sparsSrc.PassStatement( L"", pwszEscChars ) ;
			return ;
		}
		LDataArrayClass *	pArrayClass = xvalVar->GetType().GetDataArrayClass() ;
		assert( pArrayClass != nullptr ) ;

		LExprValuePtr		xvalPtrBase = MakeRefPointerToPointer( xvalVar ) ;
		const size_t		nLength = pArrayClass->GetArrayElementCount() ;
		const LType			typeElement = pArrayClass->GetElementType() ;
		size_t				iElement = 0 ;
		for ( ; ; )
		{
			if ( sparsSrc.HasNextChars( L"]" ) == L']' )
			{
				break ;
			}
			if ( iElement > 0 )
			{
				if ( sparsSrc.HasNextChars( L"," ) != L',' )
				{
					OnError( errorNoSeparationArrayList ) ;
					sparsSrc.PassStatement( L"]", L";" ) ;
					return ;
				}
				if ( sparsSrc.HasNextChars( L"]" ) == L']' )
				{
					break ;
				}
			}
			if ( iElement >= nLength )
			{
				OnError( errorInitExprErrorTooMuchElement ) ;
				sparsSrc.PassStatement( L"]", L";" ) ;
				break ;
			}
			LExprValuePtr	xvalPtrElement =
				std::make_shared<LExprValue>( typeElement, nullptr, false, false ) ;
			xvalPtrElement->SetOptionRefPointerOffset
				( xvalPtrBase, LExprValue::MakeConstExprInt
								( typeElement.GetDataBytes() * iElement ) ) ;
			ParseLocalVariableInitExpr
					( sparsSrc, xvalPtrElement, pnslLocal, L",]" ) ;
			if ( HasErrorOnCurrent() )
			{
				sparsSrc.PassStatement( L"]", L";" ) ;
				return ;
			}
			iElement ++ ;
		}
		return ;
	}
	if ( xvalVar->GetType().IsStructure() )
	{
		// 構造体 : { expr, expr, ... }
		if ( sparsSrc.HasNextChars( L"{" ) != L'{' )
		{
			OnError( errorInitExpressionError ) ;
			sparsSrc.PassStatement( L"", pwszEscChars ) ;
			return ;
		}
		LStructureClass *	pStructClass = xvalVar->GetType().GetStructureClass() ;
		assert( pStructClass != nullptr ) ;

		LExprValuePtr			xvalPtrBase = MakeRefPointerToPointer( xvalVar ) ;

		const LArrangement&		arrange = pStructClass->ProtoArrangemenet() ;
		std::vector<LString>	vMemberNames ;
		arrange.GetOrderedNameList( vMemberNames ) ;

		for ( size_t i = 0; i < vMemberNames.size(); i ++ )
		{
			if ( sparsSrc.HasNextChars( L"}" ) == L'}' )
			{
				break ;
			}
			if ( i > 0 )
			{
				if ( sparsSrc.HasNextChars( L"," ) != L',' )
				{
					OnError( errorNoSeparationArrayList ) ;
					sparsSrc.PassStatement( L"}", L";" ) ;
					return ;
				}
				if ( sparsSrc.HasNextChars( L"}" ) == L'}' )
				{
					break ;
				}
			}
			if ( i >= vMemberNames.size() )
			{
				OnError( errorInitExprErrorTooMuchElement ) ;
				sparsSrc.PassStatement( L"}", L";" ) ;
				break ;
			}
			LArrangement::Desc	desc ;
			if ( !arrange.GetDescAs( desc, vMemberNames.at(i).c_str() ) )
			{
				assert( arrange.GetDescAs( desc, vMemberNames.at(i).c_str() ) ) ;
				continue ;
			}
			LExprValuePtr	xvalPtrMember =
				std::make_shared<LExprValue>( desc.m_type, nullptr, false, false ) ;
			xvalPtrMember->SetOptionRefPointerOffset
				( xvalPtrBase, LExprValue::MakeConstExprInt( desc.m_location ) ) ;
			ParseLocalVariableInitExpr
				( sparsSrc, xvalPtrMember, pnslLocal, L",}" ) ;
			if ( HasErrorOnCurrent() )
			{
				sparsSrc.PassStatement( L"}", L";" ) ;
				return ;
			}
		}
		return ;
	}

	LExprValuePtr xval =
		EvaluateExpression( sparsSrc, pnslLocal, pwszEscChars ) ;
	xval = EvalCastTypeTo( std::move(xval), xvalVar->GetType() ) ;

	if ( HasErrorOnCurrent() )
	{
		sparsSrc.PassStatement( L"", pwszEscChars ) ;
		return ;
	}

	if ( xvalVar->GetType().IsFetchAddr() )
	{
		// ポインタをフェッチする
		assert( !xval->IsFetchAddrPointer() ) ;
		LLocalVarPtr	pAssumeVar ;
		ssize_t	iVar = GetLocalVarIndex( xvalVar ) ;
		if ( iVar >= 0 )
		{
			pAssumeVar = GetLocalVarAt( (size_t) iVar ) ;
		}
		else
		{
			OnError( errorCannotUseFetchAddr ) ;
		}
		xval = ExprFetchPointerAddr( std::move(xval), 0, 0, pAssumeVar ) ;
	}

	// 初期値を設定
	if ( xvalVar->GetType().IsConst() && xval->IsConstExpr() )
	{
		xvalVar->SetConstValue( *xval ) ;
	}
	ExprCodeStore( xvalVar, std::move(xval), true ) ;
	ExprCodeFreeTempStack() ;
}

// コンストラクタ定義判定・解釈
// コンストラクタなら関数を解釈し次へ進めたうえで true を返す
///////////////////////////////////////////////////////////////////////////////
bool LCompiler::ParseConstructer
	( LStringParser& sparsSrc,
		LType::Modifiers accModLeft, const LNamespaceList * pnslLocal )
{
	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	if ( pNamespace == nullptr )
	{
		return	false ;
	}
	LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
	if ( pClass == nullptr )
	{
		return	false ;
	}

	// ClassName( ... )
	const size_t	iSaveIndex = sparsSrc.GetIndex() ;
	if ( !sparsSrc.HasNextToken( pClass->GetConstructerName().c_str() )
		|| (sparsSrc.HasNextChars( L"(" ) != L'(') )
	{
		sparsSrc.SeekIndex( iSaveIndex ) ;
		return	false ;
	}

	// 引数リスト解釈
	std::shared_ptr<LPrototype>	pProto =
			std::make_shared<LPrototype>
				( pClass, DefaultAccessModifiers(accModLeft) ) ;
	pProto->SetComment( pNamespace->MakeComment( m_commentBefore.c_str() ) ) ;
	m_commentBefore = L"" ;

	ParsePrototypeArgmentList( sparsSrc, *pProto, pnslLocal, L")" ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatement( L")", L":{}" ) ;
	}

	// 返り値型は void
	pProto->ReturnType() = LType() ;

	// 関数の後置修飾
	ParsePostFunctionModifiers( sparsSrc, pProto ) ;

	// 関数登録
	LPtr<LFunctionObj>	pFunc =
			new LFunctionObj( m_vm.GetFunctionClassAs( pProto ), pProto ) ;
	pClass->OverrideVirtualFunction( LClass::s_Constructor, pFunc ) ;

	// 関数実装解釈
	ParseFunctionImplementation( sparsSrc, pFunc, pnslLocal ) ;

	return	true ;
}

// メンバ関数解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseMemberFunction
	( LStringParser& sparsSrc,
		LType::Modifiers accModFunc, const LType& typeRet,
		const wchar_t * pwszFuncName, const LNamespaceList * pnslLocal )
{
	LPtr<LNamespace>	pNamespace = GetCurrentNamespace() ;
	if ( pNamespace == nullptr )
	{
		OnError( errorUnavailableDefineFunc, pwszFuncName ) ;
		sparsSrc.PassStatementBlock() ;
		return ;
	}
	accModFunc = DefaultAccessModifiers(accModFunc) ;

	if ( accModFunc & LType::modifierFetchAddr )
	{
		OnWarning
			( warningInvalidModifierForFunction, warning3,
				pwszFuncName,
				GetReservedWordDesc(Symbol::rwiFetchAddr).pwszName ) ;
	}

	LClass *	pClass = nullptr ;
	if ( !(accModFunc & LType::modifierStatic) )
	{
		pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
	}

	// 引数リスト解釈
	std::shared_ptr<LPrototype>	pProto =
			std::make_shared<LPrototype>( pClass, accModFunc ) ;
	pProto->SetComment( pNamespace->MakeComment( m_commentBefore.c_str() ) ) ;
	m_commentBefore = L"" ;

	ParsePrototypeArgmentList( sparsSrc, *pProto, pnslLocal, L")" ) ;

	if ( sparsSrc.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		sparsSrc.PassStatement( L")", L":{}" ) ;
	}

	// 返り値型設定
	pProto->ReturnType() = GetArgumentTypeOf( typeRet ) ;

	// 関数の後置修飾
	ParsePostFunctionModifiers( sparsSrc, pProto ) ;

	// 関数登録
	LPtr<LFunctionObj>	pFunc =
			new LFunctionObj( m_vm.GetFunctionClassAs( pProto ), pProto ) ;
	if ( pClass != nullptr )
	{
		auto [iVirtFunc, pOverrided] =
			pClass->OverrideVirtualFunction( pwszFuncName, pFunc ) ;
		if ( (pOverrided == nullptr)
			&& (accModFunc & LType::modifierOverride) )
		{
			OnWarning( warningFunctionNotOverrided, warning2, pwszFuncName ) ;
		}
	}
	else
	{
		LPtr<LFunctionObj>	pOverrided =
				pNamespace->AddStaticFunctionAs( pwszFuncName, pFunc ) ;
		if ( (pOverrided == nullptr)
			&& (accModFunc & LType::modifierOverride) )
		{
			OnWarning( warningFunctionNotOverrided, warning2, pwszFuncName ) ;
		}
	}

	// 関数実装解釈
	ParseFunctionImplementation( sparsSrc, pFunc, pnslLocal ) ;
}

// 関数後置修飾解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParsePostFunctionModifiers
	( LStringParser& sparsSrc, std::shared_ptr<LPrototype> pProto )
{
	if ( sparsSrc.HasNextToken( L"const" ) )
	{
		pProto->SetModifiers( pProto->GetModifiers() | LType::modifierConst ) ;
	}
}

// 関数実装解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseFunctionImplementation
	( LStringParser& sparsSrc,
		LPtr<LFunctionObj> pFunc, const LNamespaceList * pnslLocal )
{
	if ( sparsSrc.HasNextChars( L";" ) == L';' )
	{
		if ( pFunc->GetPrototype()->GetModifiers() & LType::modifierNative )
		{
			m_vm.SolveNativeFunction( pFunc ) ;
		}
		else if ( !(pFunc->GetPrototype()->GetModifiers() & LType::modifierAbstract) )
		{
			OnError( errorNoStatementsOfFunction,
						pFunc->GetPrintName().c_str() ) ;
		}
		return ;
	}
	else
	{
		if ( pFunc->GetPrototype()->GetModifiers() & LType::modifierAbstract )
		{
			OnError( errorFunctionWithImplementation_opt2,
						pFunc->GetPrintName().c_str(),
						GetReservedWordDesc(Symbol::rwiAbstract).pwszName ) ;
		}
		if ( pFunc->GetPrototype()->GetModifiers() & LType::modifierNative )
		{
			OnError( errorFunctionWithImplementation_opt2,
						pFunc->GetPrintName().c_str(),
						GetReservedWordDesc(Symbol::rwiNative).pwszName ) ;
		}
	}

	// 遅延実装
	// ※ソースファイル単位での前方参照まではOK
	const size_t	iStartImplements = sparsSrc.GetIndex() ;
	sparsSrc.PassStatementBlock() ;

	LParserSegment	ps( &sparsSrc, iStartImplements, sparsSrc.GetIndex() ) ;
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest->m_prev != nullptr )	// ソースファイルのルートに遅延実装を設定する
	{
		pNest = pNest->m_prev ;
	}
	LNamespaceList	nslTemp ;
	if ( pnslLocal != nullptr )
	{
		nslTemp.AddNamespaceList( *pnslLocal ) ;
	}
	pNest->m_vecDelayImplements.push_back
		( std::make_shared<DelayImplement>( ps, pFunc, nslTemp ) ) ;

}

// 関数遅延実装
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseDelayFuncImplement( const LCompiler::DelayImplement& delayImpl )
{
	assert( delayImpl.m_pParser != nullptr ) ;

	LStringParser&				sparsSrc = *(delayImpl.m_pParser) ;
	LStringParser::StateSaver	saver( sparsSrc ) ;

	sparsSrc.SetBounds( delayImpl.m_iFirst, delayImpl.m_iEnd ) ;
	sparsSrc.SeekIndex( delayImpl.m_iFirst ) ;

	// 関数ブロック開始
	std::shared_ptr<LCodeBuffer>	codeBuf = std::make_shared<LCodeBuffer>() ;
	delayImpl.m_pFunc->SetFuncCode( codeBuf, 0 ) ;
	codeBuf->AttachSourceFile
		( m_vm.SourceProducer().GetSafeSource( delayImpl.m_pParser ) ) ;

	ContextPtr	ctxFunc =
		BeginFunctionBlock
			( delayImpl.m_pFunc->GetPrototype(), codeBuf ) ;

	// 構築関数・初期化リスト解釈
	if ( sparsSrc.HasNextChars( L":" ) == L':' )
	{
		ParseInitListOfConstructer
			( sparsSrc, delayImpl.m_pFunc, &(delayImpl.m_nslLocal) ) ;
	}
	else if ( delayImpl.m_pFunc->GetName() == LClass::s_Constructor )
	{
		std::set<LClass*>	setEmpty ;
		ExprCodeCallDefaultConstructer( delayImpl.m_pFunc, setEmpty ) ;
	}

	if ( sparsSrc.HasNextChars( L"{" ) != L'{' )
	{
		OnError( errorNoStatementsOfFunction,
					delayImpl.m_pFunc->GetPrintName().c_str() ) ;
	}
	else
	{
		// 関数実装
		LNamespaceList	nslNamespace ;
		AddScopeToNamespaceList( nslNamespace, &(delayImpl.m_nslLocal) ) ;

		ParseStatementList( sparsSrc, &nslNamespace, L"}" ) ;

		if ( sparsSrc.HasNextChars( L"}" ) != L'}' )
		{
			OnError( errorMismatchBraces ) ;
		}
	}

	// 関数ブロック終了
	EndFunctionBlock( ctxFunc ) ;
}

// 構築関数初期化リスト解釈
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseInitListOfConstructer
	( LStringParser& sparsSrc,
		LPtr<LFunctionObj> pFunc, const LNamespaceList * pnslLocal )
{
	std::set<std::wstring>		setInitMember ;
	std::set<LClass*>			setInitClasses ;
	std::shared_ptr<LPrototype>	pProto = pFunc->GetPrototype() ;

	LClass *	pClass = pProto->GetThisClass() ;
	if ( (pClass == nullptr)
		|| (pFunc->GetName() != LClass::s_Constructor) )
	{
		OnError( errorSyntaxError ) ;
		sparsSrc.PassStatement( L"", L"{" ) ;
		return ;
	}

	while ( sparsSrc.PassSpace() )
	{
		if ( sparsSrc.CurrentChar() == L'{' )
		{
			break ;
		}
		LString	strName ;
		if ( sparsSrc.NextToken( strName ) != LStringParser::tokenSymbol )
		{
			OnError( errorSyntaxError ) ;
			sparsSrc.PassStatement( L"", L"{" ) ;
			return ;
		}
		if ( setInitMember.count( strName.c_str() ) > 0 )
		{
			OnError( errorDoubleInitialized, strName.c_str() ) ;
		}
		else
		{
			setInitMember.insert( strName ) ;
		}

		LClass *		pInitClass = nullptr ;
		LExprValuePtr	xvalInit ;
		do
		{
			// 親クラス名か？
			LClass *	pSuperClass = pClass->GetSuperClass() ;
			if ( (pSuperClass != nullptr)
				&& (pSuperClass->GetClassName() == strName) )
			{
				pInitClass = pSuperClass ;
				setInitClasses.insert( pSuperClass ) ;
				xvalInit = GetThisObject() ;
				break ;
			}

			// 実装クラス名か？
			const std::vector<LClass*>&
					vImplClasses = pClass->GetImplementClasses() ;
			for ( LClass* pImplClass : vImplClasses )
			{
				if ( pImplClass->GetClassName() == strName )
				{
					pInitClass = pImplClass ;
					xvalInit = GetThisObject() ;
					break ;
				}
			}
			if ( pInitClass != nullptr )
			{
				setInitClasses.insert( pSuperClass ) ;
				break ;
			}

			// メンバ変数名か？
			LArrangement::Desc	desc ;
			if ( pClass->GetProtoArrangemenet().GetDescAs( desc, strName.c_str() )
				&& (desc.m_location >= pClass->GetProtoSuperArrangeBufBytes())
				&& desc.m_type.IsStructure() )
			{
				pInitClass = desc.m_type.GetClass() ;
				xvalInit = GetExprVarMember( GetThisObject(), strName.c_str() ) ;
			}
		}
		while ( false ) ;

		if ( (pInitClass == nullptr) || (xvalInit == nullptr) )
		{
			OnError( errorUnavailableInitVarName, strName.c_str() ) ;
			sparsSrc.PassStatement( L"", L"{" ) ;
			return ;
		}

		// 初期化引数解釈
		std::vector<LExprValuePtr>	xvalList ;
		if ( sparsSrc.HasNextChars( L"(" ) != L'(' )
		{
			OnError( errorNotFoundSyntax_opt1, L"(" ) ;
			sparsSrc.PassStatement( L")", L"{" ) ;
			return ;
		}
		if ( !ParseExprList( sparsSrc, xvalList, pnslLocal, L"){}" ) )
		{
			assert( HasErrorOnCurrent() ) ;
			sparsSrc.PassStatement( L")", L"{" ) ;
			return ;
		}
		if ( sparsSrc.HasNextChars( L")" ) != L')' )
		{
			OnError( errorMismatchParentheses ) ;
			sparsSrc.PassStatement( L")", L"{" ) ;
			return ;
		}
		LArgumentListType	argListType ;
		GetExprListTypes( argListType, xvalList ) ;

		// 構築関数取得
		ssize_t	iInitFunc =
			pInitClass->GetVirtualVector().FindCallableFunction
				( LClass::s_Constructor, argListType,
					pInitClass, GetAccessibleScope(pInitClass) ) ;
		if ( iInitFunc < 0 )
		{
			OnError( errorNotFoundMatchConstructor,
						pInitClass->GetFullClassName().c_str(),
						argListType.ToString().c_str() ) ;
			return ;
		}

		// 構築関数呼び出し
		LPtr<LFunctionObj>	pInitFunc =
			pInitClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
		assert( pInitFunc != nullptr ) ;

		ExprCallFunction( xvalInit, pInitFunc, xvalList ) ;

		ExprCodeFreeTempStack() ;

		// 次のリスト
		if ( sparsSrc.HasNextChars( L"," ) == L',' )
		{
			continue ;
		}
	}

	// 呼び出されていない親クラスの構築関数呼び出し
	ExprCodeCallDefaultConstructer( pFunc, setInitClasses ) ;
}

// 親クラスのデフォルトの構築関数呼び出し
///////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeCallDefaultConstructer
	( LPtr<LFunctionObj> pFunc,
		const std::set<LClass*>& setCalledInit )
{
	LClass *	pClass = pFunc->GetPrototype()->GetThisClass() ;
	if ( pClass == nullptr )
	{
		return ;
	}

	// 呼び出すべき親クラスを列挙
	std::vector<LClass*>	vInitClasses ;

	LClass *	pSuperClass = pClass->GetSuperClass() ;
	if ( (pSuperClass != nullptr)
		&& (setCalledInit.count( pSuperClass ) == 0) )
	{
		vInitClasses.push_back( pSuperClass ) ;
	}

	const std::vector<LClass*>&
			vImplClasses = pClass->GetImplementClasses() ;
	for ( LClass* pImplClass : vImplClasses )
	{
		if ( setCalledInit.count( pImplClass ) == 0 )
		{
			vInitClasses.push_back( pImplClass ) ;
		}
	}

	for ( LClass * pInitClass : vInitClasses )
	{
		// 構築関数取得
		LArgumentListType	argListType ;
		ssize_t	iInitFunc =
			pInitClass->GetVirtualVector().FindCallableFunction
				( LClass::s_Constructor, argListType,
					pInitClass, GetAccessibleScope(pInitClass) ) ;
		if ( iInitFunc >= 0 )
		{
			// 構築関数呼び出し
			LPtr<LFunctionObj>	pInitFunc =
				pInitClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
			assert( pInitFunc != nullptr ) ;

			LExprValuePtr	xvalInit = GetThisObject() ;
			std::vector<LExprValuePtr>	xvalList ;
			ExprCallFunction( xvalInit, pInitFunc, xvalList ) ;

			ExprCodeFreeTempStack() ;
		}
	}
}

