
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Loquaty クラス
//////////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LLoquatyClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LLoquatyClass::s_Virtuals[8] =
{
	{	// public Loquaty( Loquaty ref = null )
		LClass::s_Constructor,
		&LLoquatyClass::method_init, false,
		L"public", L"void", L"Loquaty ref = null",
		L"Loquaty 仮想マシンを構築します。\n"
		L"<param name=\"ref\">参照元の仮想マシン。null の場合には新規に作成します。</param>"
		L"<desc>別の仮想マシン間では相互に干渉することはありませんが、"
		L"ref を指定するとクラスを共有するため、"
		L"グローバル変数やクラスの static メンバを介して干渉することが可能となります。<br/>"
		L"しかし ref を指定して仮想マシンを作成する主な利点は初期化の高速化です。</desc>", nullptr
	},
	{	// public void release()
		L"release",
		&LLoquatyClass::method_release, false,
		L"public", L"void", L"",
		L"Loquaty 仮想マシン内の全てのスレッドを終了して全てのオブジェクトを解放します。", nullptr
	},
	{	// public int includeScript( String file, StringBuf errmsg = null )
		L"includeScript",
		&LLoquatyClass::method_includeScript, false,
		L"public", L"int", L"String file, StringBuf errmsg = null",
		L"仮想マシンにスクリプトを追加します。\n"
		L"<param name=\"file\">インクルードするスクリプトファイル名</param>\n"
		L"<param name=\"errmsg\">コンパイル時に出力された警告やエラーメッセージを受け取る StringBuf オブジェクト</param>\n"
		L"<return>発生したエラー数</return>", nullptr
	},
	{	// public Function<long()> compileAsInteger( String expr, StringBuf errmsg = null ) const
		L"compileAsInteger",
		&LLoquatyClass::method_compileAsInteger, false,
		L"public const", L"Function<long()>", L"String expr, StringBuf errmsg = null",
		L"式を表す文字列を long 型を評価する関数としてコンパイルします。\n"
		L"<param name=\"expr\">コンパイルする式を格納した文字列</param>\n"
		L"<param name=\"errmsg\">コンパイル時に出力された警告やエラーメッセージを受け取る StringBuf オブジェクト</param>\n"
		L"<return>コンパイルされた関数。エラーが発生した場合には null</return>", nullptr
	},
	{	// public Function<double()> compileAsNumber( String expr, StringBuf errmsg = null ) const
		L"compileAsNumber",
		&LLoquatyClass::method_compileAsNumber, false,
		L"public const", L"Function<double()>", L"String expr, StringBuf errmsg = null",
		L"式を表す文字列を double 型を評価する関数としてコンパイルします。\n"
		L"<param name=\"expr\">コンパイルする式を格納した文字列</param>\n"
		L"<param name=\"errmsg\">コンパイル時に出力された警告やエラーメッセージを受け取る StringBuf オブジェクト</param>\n"
		L"<return>コンパイルされた関数。エラーが発生した場合には null</return>", nullptr
	},
	{	// public Function<Object()> compileAsObject( String expr, StringBuf errmsg = null ) const
		L"compileAsObject",
		&LLoquatyClass::method_compileAsObject, false,
		L"public const", L"Function<Object()>", L"String expr, StringBuf errmsg = null",
		L"式を表す文字列を Object 型を評価する関数としてコンパイルします。\n"
		L"<param name=\"expr\">コンパイルする式を格納した文字列</param>\n"
		L"<param name=\"errmsg\">コンパイル時に出力された警告やエラーメッセージを受け取る StringBuf オブジェクト</param>\n"
		L"<return>コンパイルされた関数。エラーが発生した場合には null</return>\n"
		L"<desc>※getCurrent() で取得される仮想マシン自身、"
		L"あるいはそれを参照する仮想マシン以外からの Object は、"
		L"この仮想マシンとはクラスの互換性がないため"
		L"スクリプト上で利用することはできません</desc>", nullptr
	},
	{	// public Function<void()> compileAsVoid( String statement, StringBuf errmsg = null ) const
		L"compileAsVoid",
		&LLoquatyClass::method_compileAsVoid, false,
		L"public const", L"Function<void()>", L"String statement, StringBuf errmsg = null",
		L"文をコンパイルします。\n"
		L"<param name=\"statement\">コンパイルする文を格納した文字列</param>\n"
		L"<param name=\"errmsg\">コンパイル時に出力された警告やエラーメッセージを受け取る StringBuf オブジェクト</param>\n"
		L"<return>コンパイルされた関数。エラーが発生した場合には null</return>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::NativeFuncDesc	LLoquatyClass::s_Functions[5] =
{
	{	// public static Loquaty getCurrent()
		L"getCurrent",
		&LLoquatyClass::method_getCurrent, false,
		L"public static", L"Loquaty", L"",
		L"実行している仮想マシンを取得します。", nullptr
	},
	{	// public String express( Object obj, ulong flags = 0 )
		L"express",
		&LLoquatyClass::method_express, false,
		L"public static", L"String", L"Object obj, ulong flags = 0",
		L"オブジェクトを文字列変換しますが、出来るだけ Loquaty の式として評価できる形式にします。\n"
		L"null を受け取った場合には &quot;null&quot; が返されます。\n"
		L"この関数は簡易的な JSON エンコーダーとして利用できるかもしれません。\n"
		L"<param name=\"obj\">文字列化したいオブジェクト</param>\n"
		L"<param name=\"flags\">文字列化フラグ。"
		L"JSON エンコーダーとして利用するには expressJSON を指定します。</param>\n"
		L"<return>式として文字列化された String オブジェクト</return>", nullptr
	},
	{	// public Object evalConstExpr( String expr )
		L"evalConstExpr",
		&LLoquatyClass::method_evalConstExpr, false,
		L"public static", L"Object", L"String expr",
		L"定数式を表現した文字列を評価してオブジェクトとして返します。\n"
		L"この関数は簡易的な JSON デコーダーとして利用できるかもしれません。"
		L"<param name=\"expr\">文字列化したいオブジェクト</param>\n"
		L"<return>定数式として評価された Object オブジェクト。"
		L"結果が数値の場合には Integer オブジェクトか Double オブジェクトにボックス化されます。</return>", nullptr
	},
	{	// public static void traceLocalVars()
		L"traceLocalVars",
		&LLoquatyClass::method_traceLocalVars, false,
		L"public static", L"void", L"",
		L"現在のスタック上のローカル変数の内容を標準出力へダンプします。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::VariableDesc	LLoquatyClass::s_VariableDesc[2] =
{
	{
		L"expressJSON", L"public static const", L"uint64", L"1",
		nullptr, L"express 関数で JSON の互換性を優先します。",
	},
	{
		nullptr, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LLoquatyClass::s_MemberDesc =
{
	LLoquatyClass::s_Virtuals,
	LLoquatyClass::s_Functions,
	LLoquatyClass::s_VariableDesc,
	nullptr,
	L"Loquaty 仮想マシンのクラスです。"
} ;

// public Loquaty( Loquaty ref = null )
void LLoquatyClass::method_init( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;
	LQT_FUNC_ARG_OBJECT( LVirtualMachine, pRefVM ) ;

	if ( pRefVM != nullptr )
	{
		pThis->InitializeRef( *pRefVM ) ;
	}
	else
	{
		pThis->Initialize() ;
		pThis->SetClass( _context.VM().GetGlobalClassAs( L"Loquaty" ) ) ;
		pThis->SourceProducer().DirectoryPath()
					= _context.VM().SourceProducer().DirectoryPath() ;
	}
	LQT_RETURN_VOID() ;
}

// public void release()
void LLoquatyClass::method_release( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;

	pThis->Release() ;

	LQT_RETURN_VOID() ;
}

// public int includeScript( String file, StringBuf errmsg = null )
void LLoquatyClass::method_includeScript( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;
	LQT_FUNC_ARG_STRING( file ) ;
	LQT_FUNC_ARG_OBJECT( LStringBufObj, errmsg ) ;

	Compiler			compiler( *pThis, errmsg.Ptr() ) ;
	LCompiler::Current	current( compiler ) ;

	compiler.IncludeScript( file.c_str() ) ;

	LQT_RETURN_LONG( compiler.GetErrorCount() ) ;
}

// public Function<long()> compileAsInt( String expr, StringBuf errmsg = null )
void LLoquatyClass::method_compileAsInteger( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;
	LQT_FUNC_ARG_STRING( expr ) ;
	LQT_FUNC_ARG_OBJECT( LStringBufObj, errmsg ) ;

	LQT_RETURN_OBJECT
		( CompileFunc( _context.VM(), *pThis, expr,
						errmsg.Ptr(), LType( LType::typeInt64 ) ) ) ;
}

// public Function<double()> compileAsDouble( String expr, StringBuf errmsg = null )
void LLoquatyClass::method_compileAsNumber( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;
	LQT_FUNC_ARG_STRING( expr ) ;
	LQT_FUNC_ARG_OBJECT( LStringBufObj, errmsg ) ;

	LQT_RETURN_OBJECT
		( CompileFunc( _context.VM(), *pThis, expr,
						errmsg.Ptr(), LType( LType::typeDouble ) ) ) ;
}

// public Function<Object()> compileAsObject( String expr, StringBuf errmsg = null )
void LLoquatyClass::method_compileAsObject( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;
	LQT_FUNC_ARG_STRING( expr ) ;
	LQT_FUNC_ARG_OBJECT( LStringBufObj, errmsg ) ;

	LQT_RETURN_OBJECT
		( CompileFunc( _context.VM(), *pThis, expr,
						errmsg.Ptr(), LType( _context.VM().GetObjectClass() ) ) ) ;
}

// public Function<void()> compileAsVoid( String expr, StringBuf errmsg = null ) const
void LLoquatyClass::method_compileAsVoid( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LVirtualMachine, pThis ) ;
	LQT_FUNC_ARG_STRING( expr ) ;
	LQT_FUNC_ARG_OBJECT( LStringBufObj, errmsg ) ;

	LQT_RETURN_OBJECT
		( CompileFunc( _context.VM(), *pThis, expr,
							errmsg.Ptr(), LType() ) ) ;
}

LPtr<LFunctionObj> LLoquatyClass::CompileFunc
	( LVirtualMachine& vmMaster, LVirtualMachine& vm,
		const LString& strExpr, LStringBufObj * pErrMsg, const LType& typeRet )
{
	return	CompileFunc
		( vmMaster, vm, strExpr,
			((pErrMsg != nullptr) ? &(pErrMsg->m_string) : nullptr), typeRet ) ;
}

LPtr<LFunctionObj>
	LLoquatyClass::CompileFunc
		( LVirtualMachine& vmMaster,
			LVirtualMachine& vm,
			const LString& strExpr,
			LString * pErrMsg, const LType& typeRet )
{
	LString	strStatement ;
	if ( !typeRet.IsVoid() )
	{
		strStatement = L"return " ;
		strStatement += strExpr ;
		strStatement += L" ;" ;
	}
	else
	{
		strStatement = strExpr ;
	}
	LSourceFile	source = strStatement ;

	std::shared_ptr<LPrototype>		proto = std::make_shared<LPrototype>() ;
	proto->ReturnType() = typeRet ;

	std::shared_ptr<LCodeBuffer>	codeBuf = std::make_shared<LCodeBuffer>() ;

	Compiler			compiler( vm, pErrMsg ) ;
	LCompiler::Current	current( compiler ) ;

	LCompiler::ContextPtr
		ctx = compiler.BeginFunctionBlock( proto, codeBuf ) ;

	compiler.ParseStatementList( source ) ;

	compiler.EndFunctionBlock( ctx ) ;

	if ( compiler.GetErrorCount() > 0 )
	{
		return	nullptr ;
	}
	LPtr<LFunctionObj>	pFunc =
			new LFunctionObj( vmMaster.GetFunctionClassAs( proto ), proto ) ;
	pFunc->SetFuncCode( codeBuf, 0 ) ;
	return	pFunc ;
}

// 文字列出力
void LLoquatyClass::Compiler::PrintString( const LString& str )
{
	LCompiler::PrintString( str ) ;

	if ( m_pErrMsg != nullptr )
	{
		*m_pErrMsg += str ;
	}
}

// public static Loquaty getCurrent()
void LLoquatyClass::method_getCurrent( LContext& _context )
{
	LObjPtr	pVM = LObject::AddRef( &_context.VM() ) ;
	LQT_RETURN_OBJECT( pVM ) ;
}

// public String express( Object obj, ulong flags = 0 ) const ;
void LLoquatyClass::method_express( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_OBJECT( LObject, obj ) ;
	LQT_FUNC_ARG_ULONG( flags ) ;

	LQT_RETURN_STRING( LObject::ToExpression( obj.Ptr(), flags ) ) ;
}

// public Object evalConstExpr( String expr ) const ;
void LLoquatyClass::method_evalConstExpr( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( expr ) ;

	LSourceFile	source = expr ;

	LValue	valEval =
		LCompiler::EvaluateConstExpr( source, _context.VM() ) ;

	if ( valEval.GetType().IsPrimitive() )
	{
		if ( valEval.GetType().IsFloatingPointNumber() )
		{
			LQT_RETURN_OBJECT( _context.new_Double( valEval.AsDouble() ) ) ;
		}
		else
		{
			LQT_RETURN_OBJECT( _context.new_Integer( valEval.AsInteger() ) ) ;
		}
	}
	else
	{
		LQT_RETURN_OBJECT( valEval.GetObject() ) ;
	}
}

// public static void traceLocalVars()
void LLoquatyClass::method_traceLocalVars( LContext& _context )
{
	const LCodeBuffer *	pCodeBuf = _context.GetCodeBuffer() ;
	if ( pCodeBuf != nullptr )
	{
		const LCodeBuffer::DebugSourceInfo*
			pdsi = pCodeBuf->FindDebugSourceInfo( _context.GetIP() ) ;
		LSourceFilePtr	pSource = pCodeBuf->GetSourceFile() ;
		if ( (pdsi != nullptr)
			&& (pSource != nullptr)
			&& !pSource->GetFileName().IsEmpty() )
		{
			LStringParser::LineInfo
				linf = pSource->FindLineContainingIndexAt( pdsi->m_iSrcFirst ) ;
			printf( "\'%s\' at line %d\n",
					pSource->GetFileName().ToString().c_str(), (int) linf.iLine ) ;
		}

		std::vector<LCodeBuffer::DebugLocalVarInfo>	dbgVarInfos ;
		pCodeBuf->GetDebugLocalVarInfos( dbgVarInfos, _context.GetIP() ) ;

		for ( LCodeBuffer::DebugLocalVarInfo& dlvi : dbgVarInfos )
		{
			const std::map<std::wstring,size_t>&
				mapNames = dlvi.m_varInfo->GetLocalNameList() ;
			for ( auto iter = mapNames.begin(); iter != mapNames.end(); iter ++ )
			{
				const size_t	iLoc = iter->second ;
				LLocalVarPtr	pVar = dlvi.m_varInfo->GetLocalVarAt( iLoc ) ;
				if ( pVar == nullptr )
				{
					continue ;
				}
				LValue	valLoc = LDebugger::GetLocalVar( _context, pVar, iLoc ) ;
				if ( !valLoc.IsVoid() )
				{
					LString	strName = iter->first ;
					printf( "%s: %s: ",
							strName.ToString().c_str(),
							pVar->GetType().GetTypeName().ToString().c_str() ) ;
					//
					LString	strExpr = LDebugger::ToExpression( valLoc ) ;
					puts( strExpr.ToString().c_str() ) ;
				}
			}
		}
	}

	LQT_RETURN_VOID() ;
}

