
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// コンパイラ
//////////////////////////////////////////////////////////////////////////////

// スレッドに設定された LCompiler
thread_local LCompiler *	LCompiler::t_pCurrent = nullptr ;


// 入れ子空間
//////////////////////////////////////////////////////////////////////////////

LLocalVarArrayPtr LCompiler::CodeNest::MakeFrame( void )
{
	if ( m_frame == nullptr )
	{
		LLocalVarArrayPtr	pPrevFrame ;
		CodeNestPtr			pPrev = m_prev ;
		while ( pPrev != nullptr )
		{
			if ( pPrev->m_frame != nullptr )
			{
				pPrevFrame = pPrev->m_frame ;
				break ;
			}
			pPrev = pPrev->m_prev ;
		}
		m_frame = std::make_shared<LLocalVarArray>( pPrevFrame ) ;
	}
	return	m_frame ;
}


// ジャンプ命令用ポインタ情報
//////////////////////////////////////////////////////////////////////////////
LCompiler::CodePoint::CodePoint( const LCompiler::CodePoint& cp )
	: m_src( cp.m_src ),
		m_pos( cp.m_pos ),
		m_nest( cp.m_nest ),
		m_locals( cp.m_locals ),
		m_stack( cp.m_stack )
{
}

const LCompiler::CodePoint&
	LCompiler::CodePoint::operator = ( const LCompiler::CodePoint& cp )
{
	m_src = cp.m_src ;
	m_pos = cp.m_pos ;
	m_nest = cp.m_nest ;
	m_locals = cp.m_locals ;
	m_stack = cp.m_stack ;
	return	*this ;
}

void LCompiler::CodePoint::Release( void )
{
	m_nest = nullptr ;		// ※m_nest の解放は循環参照のため必須
	m_locals.clear() ;
}


// デバッグ用ローカル変数配置情報
//////////////////////////////////////////////////////////////////////////////
LCompiler::DebugLocalVarInfo::DebugLocalVarInfo
		( const LCompiler::DebugLocalVarInfo& dlvi )
	: m_varInfo( dlvi.m_varInfo ),
		m_cpFirst( dlvi.m_cpFirst ),
		m_cpEnd( dlvi.m_cpEnd )
{
}

const LCompiler::DebugLocalVarInfo&
	LCompiler::DebugLocalVarInfo::operator =
					( const LCompiler::DebugLocalVarInfo& dlvi )
{
	m_varInfo = dlvi.m_varInfo ;
	m_cpFirst = dlvi.m_cpFirst ;
	m_cpEnd = dlvi.m_cpEnd ;
	return	*this ;
}


// ジャンプ命令情報
//////////////////////////////////////////////////////////////////////////////
LCompiler::JumpPoint::JumpPoint( const LCompiler::JumpPoint& jp )
	: m_cpJump( jp.m_cpJump ),
		m_cpDst( jp.m_cpDst )
{
}

LCompiler::JumpPoint::JumpPoint
	( LCompiler::CodePointPtr cpJump, LCompiler::CodePointPtr cpDst )
	: m_cpJump( cpJump ),
		m_cpDst( cpDst )
{
}

const LCompiler::JumpPoint&
	LCompiler::JumpPoint::operator = (const LCompiler::JumpPoint& jp )
{
	m_cpJump = jp.m_cpJump ;
	m_cpDst = jp.m_cpDst ;
	return	*this ;
}



// 前方参照ジャンプ命令情報
//////////////////////////////////////////////////////////////////////////////
LCompiler::ForwardJumpPoint::ForwardJumpPoint
		( const LCompiler::ForwardJumpPoint& fjp )
	: m_cpJump( fjp.m_cpJump ),
		m_strDstLabel( fjp.m_strDstLabel )
{
}

LCompiler::ForwardJumpPoint::ForwardJumpPoint
		( LCompiler::CodePointPtr cpJump, const wchar_t * pwszLabel )
	: m_cpJump( cpJump ),
		m_strDstLabel( pwszLabel )
{
}

const LCompiler::ForwardJumpPoint&
	LCompiler::ForwardJumpPoint::operator = (const ForwardJumpPoint& fjp )
{
	m_cpJump = fjp.m_cpJump ;
	m_strDstLabel = fjp.m_strDstLabel ;
	return	*this ;
}



// ラベル情報
//////////////////////////////////////////////////////////////////////////////
LCompiler::LabelEntry::LabelEntry( const LCompiler::LabelEntry& label )
	: m_cpLabel( label.m_cpLabel )
{
}

const LCompiler::LabelEntry&
	LCompiler::LabelEntry::operator = ( const LCompiler::LabelEntry& label )
{
	m_cpLabel = label.m_cpLabel ;
	return	*this ;
}



// fetch_addr ローカル変数情報
//////////////////////////////////////////////////////////////////////////////
LCompiler::FetchAddrVar::FetchAddrVar( const LCompiler::FetchAddrVar& fav )
	: m_cpFetch( fav.m_cpFetch ),
		m_pVar( fav.m_pVar )
{
}

LCompiler::FetchAddrVar::FetchAddrVar
		( LCompiler::CodePointPtr cpFetch, LLocalVarPtr pVar )
	: m_cpFetch( cpFetch ),
		m_pVar( pVar )
{
}

const LCompiler::FetchAddrVar&
	LCompiler::FetchAddrVar::operator = ( const LCompiler::FetchAddrVar& fav )
{
	m_cpFetch = fav.m_cpFetch ;
	m_pVar = fav.m_pVar ;
	return	*this ;
}



// コンパイル文脈情報
//////////////////////////////////////////////////////////////////////////////

LCompiler::Context::Context( void )
	: m_codeBarrier( false ),
		m_iBeginFunc( 0 ), m_iCurStCode( 0 )
{
}

void LCompiler::Context::Release( void )
{
	for ( CodePointPtr cp : m_codePoints )
	{
		cp->Release() ;
	}
}



// LCompiler の構築と消滅
//////////////////////////////////////////////////////////////////////////////
LCompiler::LCompiler( LVirtualMachine& vm, LStringParser * src )
	: m_vm( vm ), m_pSource( src ),
		m_warningLevel( warning3 ),
		m_nErrors( 0 ), m_nWarnings( 0 ), m_hasCurError( errorNothing )
{
	using Symbol::s_ReservedWordDescs ;
	assert( sizeof(s_ReservedWordDescs)/sizeof(s_ReservedWordDescs[0]) == Symbol::rwiReservedWordCount ) ;
	for ( size_t i = 0; i < Symbol::rwiReservedWordCount; i ++ )
	{
		assert( s_ReservedWordDescs[i].rwIndex == i ) ;
		m_mapReservedWordDescs.insert
			( std::make_pair<std::wstring,const Symbol::ReservedWordDesc*>
				( s_ReservedWordDescs[i].pwszName, &(s_ReservedWordDescs[i]) ) ) ;
	}

	using Symbol::s_OperatorDescs ;
	assert( sizeof(s_OperatorDescs)/sizeof(s_OperatorDescs[0]) == Symbol::opOperatorCount ) ;
	for ( size_t i = 0; i < Symbol::opOperatorCount; i ++ )
	{
		assert( s_OperatorDescs[i].opIndex == i ) ;
		m_mapOperatorDescs.insert
			( std::make_pair<std::wstring,const Symbol::OperatorDesc*>
				( s_OperatorDescs[i].pwszName, &(s_OperatorDescs[i]) ) ) ;
	}

	assert( s_pfnParseStatement[Symbol::rwiImport] == &LCompiler::ParseStatement_import ) ;
	assert( s_pfnParseStatement[Symbol::rwiInclude] == &LCompiler::ParseStatement_incllude ) ;
	assert( s_pfnParseStatement[Symbol::rwiError] == &LCompiler::ParseStatement_error ) ;
	assert( s_pfnParseStatement[Symbol::rwiTodo] == &LCompiler::ParseStatement_todo ) ;
	assert( s_pfnParseStatement[Symbol::rwiClass] == &LCompiler::ParseStatement_class ) ;
	assert( s_pfnParseStatement[Symbol::rwiTypeDef] == &LCompiler::ParseStatement_typedef ) ;
	assert( s_pfnParseStatement[Symbol::rwiFor] == &LCompiler::ParseStatement_for ) ;
	assert( s_pfnParseStatement[Symbol::fwiForever] == &LCompiler::ParseStatement_forever ) ;
	assert( s_pfnParseStatement[Symbol::rwiWhile] == &LCompiler::ParseStatement_while ) ;
	assert( s_pfnParseStatement[Symbol::rwiDo] == &LCompiler::ParseStatement_do ) ;
	assert( s_pfnParseStatement[Symbol::rwiIf] == &LCompiler::ParseStatement_if ) ;
	assert( s_pfnParseStatement[Symbol::rwiSwitch] == &LCompiler::ParseStatement_switch ) ;
	assert( s_pfnParseStatement[Symbol::rwiCase] == &LCompiler::ParseStatement_case ) ;
	assert( s_pfnParseStatement[Symbol::rwiDefault] == &LCompiler::ParseStatement_default ) ;
	assert( s_pfnParseStatement[Symbol::rwiBreak] == &LCompiler::ParseStatement_break ) ;
	assert( s_pfnParseStatement[Symbol::rwiContinue] == &LCompiler::ParseStatement_continue ) ;
	assert( s_pfnParseStatement[Symbol::rwiGoto] == &LCompiler::ParseStatement_goto ) ;
	assert( s_pfnParseStatement[Symbol::rwiTry] == &LCompiler::ParseStatement_try ) ;
	assert( s_pfnParseStatement[Symbol::rwiThrow] == &LCompiler::ParseStatement_throw ) ;
	assert( s_pfnParseStatement[Symbol::rwiReturn] == &LCompiler::ParseStatement_return ) ;
	assert( s_pfnParseStatement[Symbol::rwiWith] == &LCompiler::ParseStatement_with ) ;
	assert( s_pfnParseStatement[Symbol::rwiOperator] == &LCompiler::ParseStatement_expr ) ;
	assert( s_pfnParseStatement[Symbol::rwiSynchronized] == &LCompiler::ParseStatement_synchronized ) ;
	assert( s_pfnParseStatement[Symbol::rwiStatic] == &LCompiler::ParseStatement_definition ) ;
	assert( s_pfnParseStatement[Symbol::rwiConst] == &LCompiler::ParseStatement_def_expr ) ;
	assert( s_pfnParseStatement[Symbol::rwiFetchAddr] == &LCompiler::ParseStatement_definition ) ;
	assert( s_pfnParseStatement[Symbol::rwiVoid] == &LCompiler::ParseStatement_def_expr ) ;
	assert( s_pfnParseStatement[Symbol::rwiUint64] == &LCompiler::ParseStatement_def_expr ) ;
	assert( s_pfnParseStatement[Symbol::rwiThis] == &LCompiler::ParseStatement_expr ) ;
	assert( s_pfnParseStatement[Symbol::rwiTrue] == &LCompiler::ParseStatement_expr ) ;
}

LCompiler::~LCompiler( void )
{
	if ( t_pCurrent == this )
	{
		t_pCurrent = nullptr ;
	}
}


//////////////////////////////////////////////////////////////////////////////
// コンパイル
//////////////////////////////////////////////////////////////////////////////

// ソースファイルをコンパイルする
void LCompiler::DoCompile( LStringParser * pSource )
{
	Current				current( *this ) ;

	LContext			context( m_vm ) ;
	LContext::Current	curCtx( context ) ;

	LStringParser *	pLastSrc = m_pSource ;
	m_pSource = pSource ;
	m_iSrcStatement = 0 ;

	ContextPtr	ctx = BeginNamespaceBlock( m_vm.Global() ) ;

	ParseStatementList( *pSource ) ;

	EndNamespaceBlock( ctx ) ;

	m_pSource = pLastSrc ;
}

void LCompiler::IncludeScript( const wchar_t * pwszFileName, LDirectory * pDirPath )
{
	LSourceFilePtr	pSource =
		m_vm.SourceProducer().GetSourceFile( pwszFileName ) ;
	if ( pSource != nullptr )
	{
		// 既にインクルード済み
		return ;
	}
	pSource = m_vm.SourceProducer().LoadSourceFile( pwszFileName, pDirPath ) ;
	if ( pSource == nullptr )
	{
		// 読み込めない
		OnError( errorNotFoundSourceFile, pwszFileName ) ;
		return ;
	}

	// パッケージ
	std::shared_ptr<LPackage::Current>	curPackage ;

	LPackage *	pCurPackage = LPackage::GetCurrent() ;
	if ( (pCurPackage == nullptr)
		|| ((pCurPackage->GetType() != LPackage::typeImportingModule)
			&& (pCurPackage->GetType() != LPackage::typeIncludingScript)) )
	{
		LPackagePtr	pPackage =
			std::make_shared<LPackage>
				( LPackage::typeIncludingScript, pwszFileName ) ;
		m_vm.AddPackage( pPackage ) ;

		curPackage = std::make_shared<LPackage::Current>( pPackage.get() ) ;
	}

	// コンパイル
	DoCompile( pSource.get() ) ;
}



///////////////////////////////////////////////////////////////////////////////
// エラー
//////////////////////////////////////////////////////////////////////////////

// 現在のステートメントでエラーが発生したか？
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::HasErrorOnCurrent( void ) const
{
	return	(m_hasCurError != errorNothing) ;
}

// 現在のステートメントのエラーフラグをクリア
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ClearErrorOnCurrent( void )
{
	m_hasCurError = errorNothing ;
}

// エラー出力
//////////////////////////////////////////////////////////////////////////////
void LCompiler::OnError
	( ErrorMessageIndex err,
		const wchar_t * opt1, const wchar_t * opt2 )
{
	if ( !HasErrorOnCurrent() )
	{
		PrintString( FormatErrorLineString( L"error", err, opt1, opt2 ) ) ;
		m_nErrors ++ ;
		m_hasCurError = err ;
	}
}

void LCompiler::Error
	( ErrorMessageIndex err,
		const wchar_t * opt1, const wchar_t * opt2 )
{
	if ( t_pCurrent != nullptr )
	{
		t_pCurrent->ClearErrorOnCurrent() ;
		t_pCurrent->OnError( err, opt1, opt2 ) ;
	}
	else
	{
		LContext::ThrowExceptionError
			( FormatErrorString( L"error", err, opt1, opt2 ).c_str(), nullptr ) ;
	}
}

// 警告出力
//////////////////////////////////////////////////////////////////////////////
void LCompiler::OnWarning
	( ErrorMessageIndex err, int nLevel,
		const wchar_t * opt1, const wchar_t * opt2 )
{
	if ( !HasErrorOnCurrent() && (nLevel <= m_warningLevel) )
	{
		PrintString( FormatErrorLineString( L"warning", err, opt1, opt2 ) ) ;
		m_nWarnings ++ ;
	}
}

void LCompiler::Warning
	( ErrorMessageIndex err, int nLevel,
		const wchar_t * opt1, const wchar_t * opt2 )
{
	if ( t_pCurrent != nullptr )
	{
		t_pCurrent->OnWarning( err, nLevel, opt1, opt2 ) ;
	}
}

// エラー文字列フォーマット
//////////////////////////////////////////////////////////////////////////////
LString LCompiler::FormatErrorLineString
	( const wchar_t * pwszType,
		ErrorMessageIndex err,
		const wchar_t * opt1, const wchar_t * opt2 )
{
	LString	strCurFileLine = GetCurrentFileLine() ;
	if ( !strCurFileLine.IsEmpty() )
	{
		strCurFileLine += L":" ;
	}
	return	strCurFileLine + FormatErrorString( pwszType, err, opt1, opt2 ) + L"\n" ;
}

LString LCompiler::FormatErrorString
	( const wchar_t * pwszType,
		ErrorMessageIndex err,
		const wchar_t * opt1, const wchar_t * opt2 )
{
	LString	strErr ;
	if ( pwszType != nullptr )
	{
		strErr += pwszType ;
		strErr += L": " ;
	}

	LString	strErrMsg = GetErrorMessage( err ) ;

	std::vector<LString>	opts ;
	opts.push_back( LString(opt1) ) ;
	opts.push_back( LString(opt2) ) ;

	LString	strReplaced = LString::Format( strErrMsg.c_str(), opts ) ;

	if ( (opt1 != nullptr) && (opt1[0] != 0) )
	{
		if ( strErrMsg.Find( L"%(0)" ) < 0 )
		{
			strErr += L"\'" ;
			strErr += opt1 ;
			strErr += L"\' : " ;
		}
	}
	if ( (opt2 != nullptr) && (opt2[0] != 0) )
	{
		if ( strErrMsg.Find( L"%(1)" ) < 0 )
		{
			strReplaced += L" (" ;
			strReplaced += opt2 ;
			strReplaced += L")" ;
		}
	}
	strErr += strReplaced ;

	return	strErr ;
}

// 現在のソースと行番号・桁
//////////////////////////////////////////////////////////////////////////////
LString LCompiler::GetCurrentFileLine( void ) const
{
	if ( m_pSource == nullptr )
	{
		return	LString() ;
	}
	LString	strFile = LURLSchemer::GetFileNameOf
							( m_pSource->GetFileName().c_str() ) ;

	LStringParser::LineInfo	lninf =
		m_pSource->FindLineContainingIndexAt( m_pSource->GetIndex() ) ;
	if ( lninf.iStart != 0 )
	{
		strFile += L"(" ;
		strFile += LString::IntegerOf( lninf.iLine ) ;

		if ( m_pSource->GetIndex() >= lninf.iStart )
		{
			strFile += L":" ;
			strFile += LString::IntegerOf
							( m_pSource->GetIndex() - lninf.iStart ) ;
		}
		strFile += L")" ;
	}

	return	strFile ;
}

// 文字列出力
//////////////////////////////////////////////////////////////////////////////
void LCompiler::PrintString( const LString& str )
{
	std::string	msg ;
	LTrace( "%s\n", str.ToString(msg).c_str() ) ;
}

