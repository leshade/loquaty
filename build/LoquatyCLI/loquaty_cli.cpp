
#include "loquaty_cli.h"


// エントリポイント
//////////////////////////////////////////////////////////////////////////////

int main( int argc, char* argv[], char* envp[] )
{
#if	defined(_MSC_VER) && defined(_DEBUG)
	// 終了時にメモリリークをレポートするように設定する
	int	dbgFlags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) ;
	dbgFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ;
	_CrtSetDbgFlag( dbgFlags ) ;

	// ※ 終了時にメモリリークがレポートされた場合
	//    レポートに表示される { } 括弧内の番号で
	//    ブレークして発生個所を見つける
	//_CrtSetBreakAlloc( 17886 ) ;
#endif

	LoquatyApp	app ;
	int	nExit = app.ParseCmdLine( argc, argv, envp ) ;
	if ( nExit == 0 )
	{
		nExit = app.Run() ;
	}
	return	nExit ;
}



//////////////////////////////////////////////////////////////////////////////
// コマンドライン・アプリケーション
//////////////////////////////////////////////////////////////////////////////

LoquatyApp::LoquatyApp( void )
	: m_verb( verbNo ),
		m_optNologo( false ),
		m_warnLevel( LCompiler::warning3 )
{
	m_vm = new LVirtualMachine ;
}

LoquatyApp::~LoquatyApp( void )
{
}

// コマンドライン解釈
int LoquatyApp::ParseCmdLine( int argc, char* argv[], char* envp[] )
{
	int	nExit = 0 ;
	int	iArg ;
	for ( iArg = 1; iArg < argc; iArg ++ )
	{
		if ( argv[iArg][0] == '/' )
		{
			std::string	strArg = argv[iArg] ;
			if ( strArg == "/nologo" )
			{
				m_optNologo = true ;
			}
			else if ( strArg == "/x" )
			{
				m_verb = verbRun ;
				m_optNologo = true ;
			}
			else if ( (strArg == "/help") || (strArg == "/?") )
			{
				m_verb = verbHelp ;
			}
			else if ( (iArg + 1 < argc) && (strArg == "/I") )
			{
				m_vm->SourceProducer().DirectoryPath().
					AddDirectory
						( std::make_shared<LSubDirectory>
							( std::make_shared<LFileDirectory>(),
								LString( argv[++ iArg] ).c_str() ) ) ;
			}
			else if ( (iArg + 1 < argc)
					&& ((strArg == "/dump_func") || (strArg == "/DF")) )
			{
				m_strDumpFunc = LString( argv[++ iArg] ) ;
				m_verb = verbDumpFunc ;
			}
			else if ( (iArg + 1 < argc)
					&& ((strArg == "/doc_class") || (strArg == "/DC")) )
			{
				m_strMakeTarget = LString( argv[++ iArg] ) ;
				m_verb = verbMakeDocClass ;
			}
			else if ( (iArg + 1 < argc)
					&& ((strArg == "/doc_package") || (strArg == "/DP")) )
			{
				m_strMakeTarget = LString( argv[++ iArg] ) ;
				m_verb = verbMakeDocPackage ;
			}
			else if ( (strArg == "/doc_all") || (strArg == "/DA") )
			{
				m_verb = verbMakeDocAll ;
			}
			else if ( (iArg + 1 < argc)
					&& ((strArg == "/cpp_stub") || (strArg == "/CS")) )
			{
				m_strMakeTarget = LString( argv[++ iArg] ) ;
				m_verb = verbMakeCppStub ;
			}
			else if ( (iArg + 1 < argc)
					&& ((strArg == "/cpp_stubs") || (strArg == "/CSs")) )
			{
				m_strMakeTarget = LString( argv[++ iArg] ) ;
				m_verb = verbMakeCppStubs ;
			}
			else if ( (iArg + 1 < argc) && (strArg == "/out") )
			{
				m_strMakeOutput = LString( argv[++ iArg] ) ;
			}
			else if ( (strArg.length() >= 3)
					&& (strArg.substr(0,2) == "/W") )
			{
				m_warnLevel = atoi( argv[iArg] + 2 ) ;
			}
			else
			{
				printf( "Error : %s\n", argv[iArg] ) ;
				nExit ++ ;
				break ;
			}
		}
		else
		{
			m_strSourceFile = LString( argv[iArg] ) ;
			if ( m_verb == verbNo )
			{
				m_verb = verbRun ;
				m_optNologo = true ;
			}
			break ;
		}
	}
	while ( ++ iArg < argc )
	{
		m_argsScript.push_back( LString( argv[iArg] ) ) ;
	}
	if ( m_verb == verbNo )
	{
		m_verb = verbHelp ;
	}
	if ( m_verb != verbHelp )
	{
		if ( m_strSourceFile.IsEmpty() )
		{
			printf( "Error : ソースファイルが指定されていません\n" ) ;
			nExit ++ ;
		}
		AddEnvIncludePath( envp ) ;
	}
	return	nExit ;
}

void LoquatyApp::AddEnvIncludePath( char* envp[] )
{
	LString	strEnvName = L"LOQUATY_INCLUDE_PATH=" ;
	for ( int i = 0; envp[i] != nullptr; i ++ )
	{
		LString	strEnv = envp[i] ;
		if ( strEnv.Left( strEnvName.GetLength() ) == strEnvName )
		{
			std::vector<LString>	vPath ;
			LString	strPath = envp[i] + strEnvName.GetLength() ;
			strPath.Slice( vPath, L";" ) ;
			for ( size_t j = 0; j < vPath.size(); j ++ )
			{
				if ( !vPath.at(j).IsEmpty() )
				{
					m_vm->SourceProducer().DirectoryPath().
						AddDirectory
							( std::make_shared<LSubDirectory>
								( std::make_shared<LFileDirectory>(),
										vPath.at(j).c_str() ) ) ;
				}
			}
			break ;
		}
	}
}

// 実行
int LoquatyApp::Run( void )
{
	if ( !m_optNologo )
	{
		PrintLogo() ;
	}
	int	nExit = 0 ;
	switch ( m_verb )
	{
	case	verbRun:
		nExit = RunMain() ;
		break ;

	case	verbDumpFunc:
		nExit = DumpFunction() ;
		break ;

	case	verbMakeDocClass:
		nExit = MakeDocClass() ;
		break ;

	case	verbMakeDocPackage:
		nExit = MakeDocIndexInPackage() ;
		break ;

	case	verbMakeDocAll:
		nExit = MakeDocAllPackages() ;
		break ;

	case	verbMakeCppStub:
		nExit = MakeNativeFuncStubClass() ;
		break ;

	case	verbMakeCppStubs:
		nExit = MakeNativeFuncStubClassInPackage() ;
		break ;

	case	verbHelp:
	default:
		PrintHelp() ;
		break ;
	}
	return	nExit ;
}

// ロゴ表示
void LoquatyApp::PrintLogo( void )
{
	static const char	szLogo[] =
		"Loquaty vestion %s\n"
		"Copyright (c) Leshade Entis.\n\n" ;

	printf( szLogo, VersionString ) ;
}

// ヘルプ表示
void LoquatyApp::PrintHelp( void )
{
	static const char	szUsage[] =
		"usage: loquaty [options] <file> [<arg>...]\n"
		"\n"
		"options;\n"
		"/nologo    : ロゴ表示抑制\n"
		"/help      : 書式表示 (/?)\n"
		"/I <include-path>\n"
		"           : インクルードパスを追加します\n"
		"            （複数追加する場合は複数の /I で記述）。\n"
		"            ※デフォルトのパスは環境変数 LOQUATY_INCLUDE_PATH に設定\n"
		"              複数定義する場合はセミコロン(;)で結合\n"
		"/W0        : 全ての警告出力を抑制\n"
		"/W1        : 重大な警告（レベル１）のみ抑制\n"
		"/W2        : レベル１と２の警告を出力\n"
		"/W3        : レベル１～３の警告を出力（デフォルト）\n"
		"/W4        : 全ての警告と情報を出力\n"
		"/dump_func <func-name>\n"
		"           : 関数のニーモニックダンプを出力 (/DF)\n"
		"/doc_class <class-name>\n"
		"           : クラスをドキュメント化して出力 (/DC)\n"
		"/doc_package <package-name>\n"
		"           : パッケージ（スクリプトファイル／モジュール単位）を\n"
		"             ドキュメント化して出力 (/DP)\n"
		"/doc_all   : 全てのクラスをドキュメント化して出力 (/DA)\n"
		"/cpp_stub <class-name>\n"
		"           : クラスに含まれる native 関数の C++ 用スタブコードを出力 (/CS)\n"
		"             ※クラスや構造体に対応する C++ 側の名前は任意であるため\n"
		"               便宜的に一定の規則で命名して出力します。\n"
		"/cpp_stubs <package-name>\n"
		"           : パッケージに含まれる native 関数の C++ 用スタブコードを出力 (/CSs)\n"
		"/out <output-directory>\n"
		"           : ドキュメント化／C++スタブを出力するディレクトリ\n"
		"\n"
		"arguments;\n"
		"file       : スクリプトファイル\n"
		"arg        : スクリプトへ渡す引数\n" ;

	puts( szUsage ) ;
}

// ソースを読み込んでコンパイルする
int LoquatyApp::LoadSource( void )
{
	m_vm->Initialize() ;

	LSourceFilePtr	pSource =
		m_vm->SourceProducer().LoadSourceFile( m_strSourceFile.c_str() ) ;
	if ( pSource == nullptr )
	{
		printf( "ファイルを開けませんでした : \'%s\'\n",
					m_strSourceFile.ToString().c_str() ) ;
		return	1 ;
	}

	LPackagePtr	pPackage =
		std::make_shared<LPackage>
			( LPackage::typeSourceFile,
				LURLSchemer::GetFileNameOf( m_strSourceFile.c_str() ).c_str() ) ;
	m_vm->AddPackage( pPackage ) ;

	LPackage::Current	current( pPackage.get() ) ;

	CLICompiler	compiler( *m_vm ) ;
	compiler.SetWarningLevel( m_warnLevel ) ;
	compiler.DoCompile( pSource.get() ) ;

	if ( compiler.GetErrorCount() > 0 )
	{
		printf( "\n%d errors\n", (int) compiler.GetErrorCount() ) ;
		return	(int) compiler.GetErrorCount() ;
	}
	return	0 ;
}

// main 関数を実行する
int LoquatyApp::RunMain( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}

	// 引数配列
	LPtr<LThreadObj>	pThread = new LThreadObj( m_vm->GetThreadClass() ) ;

	std::vector<LValue>	args ;
	LPtr<LArrayObj>	pArg =
		pThread->Context().new_Array( m_vm->GetStringClass() ) ;
	args.push_back( LValue( pArg ) ) ;

	for ( size_t i = 0; i < m_argsScript.size(); i ++ )
	{
		pArg->m_array.push_back
			( LObjPtr( pThread->Context().new_String( m_argsScript.at(i) ) ) ) ;
	}

	// 関数を呼び出す
	auto [valRet, pExcept] =
		pThread->SyncCallFunctionAs
			( nullptr, L"main", args.data(), args.size() ) ;

	if ( pExcept != nullptr )
	{
		std::string	strClass ;
		if ( pExcept->GetClass() != nullptr )
		{
			strClass = pExcept->GetClass()->GetClassName().ToString() ;
		}
		LString	strExcept ;
		pExcept->AsString( strExcept ) ;

		printf( "[%s]: %s\n",
				strClass.c_str(), strExcept.ToString().c_str() ) ;

		LExceptionObj *	pExObj = dynamic_cast<LExceptionObj*>( pExcept.Ptr() ) ;
		if ( (pExObj != nullptr)
			&& (pExObj->m_pCodeBuf != nullptr)
			&& (pExObj->m_pCodeBuf->GetSourceFile() != nullptr) )
		{
			LSourceFilePtr	pSource = pExObj->m_pCodeBuf->GetSourceFile() ;
			const LCodeBuffer::DebugSourceInfo*
				pdsi = pExObj->m_pCodeBuf->FindDebufSourceInfo( pExObj->m_iThrown ) ;
			if ( pdsi != nullptr )
			{
				LStringParser::LineInfo	linf =
					pSource->FindLineContainingIndexAt( pdsi->m_iSrcFirst ) ;

				printf( "thrown in \'%s\' at line %d.\n",
						pSource->GetFileName().ToString().c_str(), (int) linf.iLine ) ;
			}
		}
	}

	nExit = (int) valRet.AsInteger() ;
	return	nExit ;
}

// 関数をダンプする
int LoquatyApp::DumpFunction( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}

	// 関数を取得する
	CLICompiler	compiler( *m_vm ) ;
	LSourceFile		srcFunc = m_strDumpFunc ;
	LExprValuePtr	xvalFunc = compiler.EvaluateExpression( srcFunc ) ;
	if ( (xvalFunc == nullptr)
		|| (!xvalFunc->IsFunctionVariation()
			&& !xvalFunc->IsRefVirtualFunction()
			&& !xvalFunc->IsConstExprFunction()) )
	{
		printf( "\n関数が見つかりません : %s\n",
					m_strDumpFunc.ToString().c_str() ) ;
		return	1 ;
	}

	if ( xvalFunc->IsFunctionVariation() )
	{
		const LFunctionVariation *	pFuncVar = xvalFunc->GetFuncVariation() ;
		assert( pFuncVar != nullptr ) ;

		for ( size_t i = 0; i < pFuncVar->size(); i ++ )
		{
			nExit += DumpFunction( pFuncVar->at(i) ) ;
		}
	}
	else if ( xvalFunc->IsRefVirtualFunction() )
	{
		LClass *	pThisClass = xvalFunc->GetVirtFuncClass() ;
		assert( pThisClass != nullptr ) ;

		const std::vector<size_t> *
					pVirtFuncs = xvalFunc->GetVirtFunctions() ;
		assert( pVirtFuncs != nullptr ) ;

		const LVirtualFuncVector&
					vecVirtFunc = pThisClass->GetVirtualVector() ;

		for ( size_t i = 0; i < pVirtFuncs->size(); i ++ )
		{
			nExit += DumpFunction( vecVirtFunc.at( pVirtFuncs->at(i) ) ) ;
		}
	}
	else if ( xvalFunc->IsConstExprFunction() )
	{
		nExit += DumpFunction( xvalFunc->GetConstExprFunction() ) ;
	}
	return	nExit ;
}

int LoquatyApp::DumpFunction( LPtr<LFunctionObj> pFunc )
{
	if ( pFunc == nullptr )
	{
		puts( "\n関数が見つかりません" ) ;
		return	1 ;
	}

	// 関数名
	printf( "%s:", pFunc->GetFullPrintName().ToString().c_str() ) ;

	if ( pFunc->GetPrototype() != nullptr )
	{
		// プロトタイプ
		printf( " %s", pFunc->GetPrototype()->
						TypeToString().ToString().c_str() ) ;
	}
	printf( "\n\n" ) ;

	// コード
	std::shared_ptr<LCodeBuffer>	pCodeBuf = pFunc->GetCodeBuffer() ;
	if ( pCodeBuf == nullptr )
	{
		return	0 ;
	}

	const std::vector<LCodeBuffer::Word>&	bufCodes = pCodeBuf->GetBuffer() ;
	const LCodeBuffer::DebugSourceInfo *	pdbSrcInf = nullptr ;

	LSourceFilePtr	pSourceFile = pCodeBuf->GetSourceFile() ;
	LString			strFileName ;
	if ( pSourceFile != nullptr )
	{
		strFileName = pSourceFile->GetFileName() ;
	}

	for ( size_t i = 0; i < bufCodes.size(); i ++ )
	{
		if ( (pdbSrcInf != nullptr) && (i >= pdbSrcInf->m_iCodeEnd) )
		{
			pdbSrcInf = nullptr ;
		}
		if ( pdbSrcInf == nullptr )
		{
			pdbSrcInf = pCodeBuf->FindDebufSourceInfo( i ) ;
			if ( (pdbSrcInf != nullptr)
				&& (pSourceFile != nullptr) )
			{
				size_t	iSrcFirst = pdbSrcInf->m_iSrcFirst ;
				while ( iSrcFirst > 0 )
				{
					wchar_t	wch = pSourceFile->GetAt( iSrcFirst - 1 ) ;
					if ( (wch != L' ') && (wch != L'\t') )
					{
						break ;
					}
					iSrcFirst -- ;
				}
				LString	strSource =
					pSourceFile->Middle
						( iSrcFirst, pdbSrcInf->m_iSrcEnd - iSrcFirst ) ;
				strSource.TrimRight() ;

				printf( "\n%s (%d):\n%s\n\n",
						strFileName.ToString().c_str(),
						(int) pSourceFile->FindLineContainingIndexAt( iSrcFirst ).iLine,
						strSource.ToString().c_str() ) ;
			}
		}
		printf( "#%d: %s\n", (int) i,
			pCodeBuf->MnemonicOf
				( bufCodes.at(i), i ).ToString().c_str() ) ;
	}
	puts( "" ) ;

	return	0 ;
}


// クラスを文書化する
//////////////////////////////////////////////////////////////////////////////
int LoquatyApp::MakeDocClass( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}
	LSourceFile	source = m_strMakeTarget ;

	// クラス取得
	LType	type ;
	if ( !source.NextTypeExpr( type, false, *m_vm, nullptr )
		|| (type.GetClass() == nullptr) )
	{
		printf( "\n\'%s\' クラスが見つかりません\n",
					m_strMakeTarget.ToString().c_str() ) ;
		return	1 ;
	}

	// css ファイルを出力
	LString	strCssFile =
		LURLSchemer::SubPath
			( m_strMakeOutput.c_str(),
				(MakeClassDocFileDir
					( type.GetClass(), (LPackage*) nullptr )
										+ L"specifications.css").c_str() ) ;
	WriteCssFile( strCssFile.c_str() ) ;

	// クラスを文書化して出力
	std::shared_ptr<LOutputStream>
			pStream = OpenClassDocFile( type.GetClass() ) ;
	if ( pStream == nullptr )
	{
		return	1 ;
	}
	return	MakeDocClass( *pStream, type.GetClass() ) ;
}

// クラスの出力ファイル名を生成する
LString LoquatyApp::MakeClassDocFileName( LClass * pClass, LClass * pFromClass )
{
	LString	strClassName = pClass->GetFullClassName() ;
	LString	strClassFile = MakeTypeFileName( strClassName ) ;

	return	MakeClassDocFileDir( pClass, pFromClass ) + strClassFile ;
}

LString LoquatyApp::MakeClassDocFileName( LClass * pClass, LPackage * pFromPackage )
{
	LString	strClassName = pClass->GetFullClassName() ;
	LString	strClassFile = MakeTypeFileName( strClassName ) ;

	return	MakeClassDocFileDir( pClass, pFromPackage ) + strClassFile ;
}

LString LoquatyApp::MakeTypeFileName( const LString& strTypeName )
{
	return	strTypeName.Replace
			( []( LStringParser& spars )
				{
					if ( LStringParser::IsCharContained
						( spars.CurrentChar(), L"+*/:<> " ) )
					{
						spars.NextChar() ;
						return	true ;
					}
					return	false ;
				},
				[]( const LString& str )
				{
					static const wchar_t *	s_pwszChars[] =
					{
						L"+", L"*", L"/", L":", L"<", L">", L" ", nullptr
					} ;
					static const wchar_t *	s_pwszReplace[] =
					{
						L"＋", L"＊", L"／", L"_", L"{", L"}", L"_", nullptr
					} ;
					for ( int i = 0; s_pwszChars[i] != nullptr; i ++ )
					{
						if ( str == s_pwszChars[i] )
						{
							return	LString( s_pwszReplace[i] ) ;
						}
					}
					return	str ;
				} ) ;
}

LString LoquatyApp::MakeClassDocFileDir( LClass * pClass, LClass * pFromClass )
{
	return	MakeClassDocFileDir
				( pClass, (pFromClass != nullptr)
							? pFromClass->GetPackage() : nullptr ) ;
}

LString LoquatyApp::MakeClassDocFileDir( LClass * pClass, LPackage * pFromPackage )
{
	LPackage *	pPackage = pClass->GetPackage() ;
	if ( pPackage != nullptr )
	{
		if ( pFromPackage == nullptr )
		{
			return	pPackage->GetName() + L"/" ;
		}
		else if ( pFromPackage != pPackage )
		{
			return	LString( L"../" ) + pPackage->GetName() + L"/" ;
		}
	}
	else if ( pFromPackage != nullptr )
	{
		return	LString( L"../" ) ;
	}
	return	LString() ;
}

// 出力ファイルを開く
std::shared_ptr<LOutputStream> LoquatyApp::OpenClassDocFile( LClass * pClass )
{
	return	OpenDocFile
		( (MakeClassDocFileName( pClass, (LPackage*) nullptr ) + L".xhtml").c_str() ) ;
}

std::shared_ptr<LOutputStream> LoquatyApp::OpenDocFile( const wchar_t * pwszFileName )
{
	// 出力ファイルを開く
	LString		strClassFile =
		LURLSchemer::SubPath( m_strMakeOutput.c_str(), pwszFileName ) ;
	LFilePtr	file =
		LURLSchemer::Open( strClassFile.c_str(), LDirectory::modeCreate ) ;

	if ( file == nullptr )
	{
		printf( "ファイルを開けませんでした : \'%s\'\n",
						strClassFile.ToString().c_str() ) ;
		return	nullptr ;
	}
	printf( "%s...\n", strClassFile.ToString().c_str() ) ;

	return	std::make_shared<LOutputUTF8Stream>( file ) ;
}

// 型定義を文書化する
int LoquatyApp::MakeDocTypeDef
	( LOutputStream& strm,
		const wchar_t * pwszName, const LType type, LPackage * pPackage )
{
	LString	strXTypeName = LXMLDocParser::EncodeXMLString( pwszName ) ;

	// ヘッダ・見出し
	strm << L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
			L"<!DOCTYPE html\r\n"
			L"[\r\n"
			L"	<!ENTITY nbsp \"&#160;\">\r\n"
			L"]>\r\n"
			L"<?xml-stylesheet type=\"text/css\" href=\"specifications.css\"?>\r\n"
			L"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"ja\" lang=\"ja\">\r\n"
			L"<head>\r\n"
			L"	<title>" << strXTypeName << L"</title>\r\n"
			L"</head>\r\n"
			L"<body>\r\n"
			L"<div class=\"chapter\">" << strXTypeName << L"</div>\r\n\r\n" ;

	// 定義概要
	strm << L"<div class=\"headline\">Type Summary</div>\r\n" ;
	strm << L"<div class=\"usage\">typedef "
			<< strXTypeName << L" = "
			<< LXMLDocParser::EncodeXMLString( type.GetTypeName().c_str() )
			<< L"</div>\r\n" ;

	LType::LComment *	pComment = type.GetComment() ;
	if ( (pComment != nullptr) && HasCommentSummary( *pComment ) )
	{
		MakeDocXMLSummary( strm, *pComment ) ;
		MakeDocXMLDescription( strm, *pComment ) ;
	}
	strm << L"<br/>\r\n\r\n" ;

	// ジェネリック型の場合にはクラスを文書化する
	int	nExit = 0 ;
	if ( (type.GetModifiers() == 0)
		&& type.IsObject()
		&& (type.GetClass() != nullptr)
		&& type.GetClass()->IsGenericInstance() )
	{
		nExit = MakeDocClassDefs( strm, type.GetClass(), pPackage ) ;
	}
	else
	{
		strm << L"<br/><br/>\r\n"
				L"</body>\r\n"
				L"</html>\r\n" ;
	}
	return	nExit ;
}

// クラスを文書化する
int LoquatyApp::MakeDocClass( LOutputStream& strm, LClass * pClass )
{
	LString	strClassName = pClass->GetFullClassName() ;
	LString	strXClassName = LXMLDocParser::EncodeXMLString( strClassName.c_str() ) ;

	// ヘッダ・見出し
	strm << L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
			L"<!DOCTYPE html\r\n"
			L"[\r\n"
			L"	<!ENTITY nbsp \"&#160;\">\r\n"
			L"]>\r\n"
			L"<?xml-stylesheet type=\"text/css\" href=\"specifications.css\"?>\r\n"
			L"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"ja\" lang=\"ja\">\r\n"
			L"<head>\r\n"
			L"	<title>" << strXClassName << L"</title>\r\n"
			L"</head>\r\n"
			L"<body>\r\n"
			L"<div class=\"chapter\">" << strXClassName << L"</div>\r\n\r\n" ;

	return	MakeDocClassDefs( strm, pClass, pClass->GetPackage() ) ;
}

int LoquatyApp::MakeDocClassDefs
	( LOutputStream& strm, LClass * pClass, LPackage * pPackage )
{
	// クラス概要
	MakeDocClassSummary( strm, pClass ) ;

	// パッケージ
	if ( (pPackage != nullptr)
		&& (pPackage->GetType() != LPackage::typeSystemDefault) )
	{
		strm << L"<div class=\"headline\">Package</div>\r\n"
				L"<div class=\"usage\">\r\n" ;
		switch ( pPackage->GetType() )
		{
		case	LPackage::typeImportingModule:
			strm << L"@import &nbsp;" ;
			break ;
		case	LPackage::typeIncludingScript:
			strm << L"@include &nbsp;" ;
			break ;
		}
		strm << pPackage->GetName() << L"\r\n" ;
		strm << L"</div>\r\n<br/>\r\n\r\n" ;
	}

	// クラス派生ツリー
	strm << L"<div class=\"headline\">Super Classes</div>\r\n"
			L"<div class=\"indent1\"><a href=\"index.xhtml\">&lt;index&gt;</a></div>\r\n" ;

	strm << MakeDocSuperClass( strm, pClass, pPackage ) ;

	if ( dynamic_cast<LStructureClass*>( pClass ) != nullptr )
	{
		const std::vector<LClass*>&	vImplements = pClass->GetImplementClasses() ;
		for ( size_t i = 0; i < vImplements.size(); i ++ )
		{
			strm << MakeDocSuperClass
					( strm, vImplements.at(i), pPackage ) ;
		}
	}
	strm << L"<br/>\r\n\r\n" ;

	// 実装クラスリスト
	const std::vector<LClass*>&	vImplements = pClass->GetImplementClasses() ;
	if ( (vImplements.size() > 0)
		&& (dynamic_cast<LStructureClass*>( pClass ) == nullptr) )
	{
		strm << L"<div class=\"headline\">Implement Classes</div>\r\n" ;
		for ( size_t i = 0; i < vImplements.size(); i ++ )
		{
			LString	strImplName = vImplements.at(i)->GetFullClassName() ;
			LString	strXImplName = LXMLDocParser::EncodeXMLString( strImplName.c_str() ) ;
			strm << L"<div class=\"indent1\"><a href=\""
					<< LXMLDocParser::EncodeXMLString
							( MakeClassDocFileName( vImplements.at(i), pClass ).c_str() )
					<< L".xhtml\">" << strXImplName << L"</a></div>\r\n" ;
		}
		strm << L"<br/>\r\n\r\n" ;
	}

	// サブクラス
	if ( pClass->GetNamespaceList().size() > 0 )
	{
		strm << L"<div class=\"headline\">Classes</div>\r\n" ;
		for ( auto iter : pClass->GetNamespaceList() )
		{
			LClass *	pSubClass = dynamic_cast<LClass*>( iter.second.Ptr() ) ;
			LString		strSubName = pSubClass->GetFullClassName() ;
			LString		strXSubName = LXMLDocParser::EncodeXMLString( strSubName.c_str() ) ;
			strm << L"<div class=\"indent2\"><a href=\""
					<< LXMLDocParser::EncodeXMLString
							( MakeClassDocFileName( pSubClass, pClass ).c_str() )
					<< L".xhtml\">" << strXSubName << L"</a></div>\r\n" ;
		}
		strm << L"<br/>\r\n\r\n" ;
	}

	// 静的メンバ変数リスト
	if ( (pClass->GetElementCount() > 0)
		|| (pClass->GetStaticArrangement().GetAlignedBytes() > 0) )
	{
		strm << L"<div class=\"headline\">Static Members</div>\r\n"
				L"<div class=\"indent2\">\r\n" ;
		pClass->AddRef() ;
		MakeDocVariableList
			( strm, L"static_", LObjPtr(pClass), pClass->GetStaticArrangement() ) ;
		strm << L"</div><br/>\r\n\r\n" ;
	}

	// メンバ変数リスト
	if ( ((pClass->GetPrototypeObject() != nullptr)
			&& (pClass->GetPrototypeObject()->GetElementCount() > 0))
		|| (pClass->GetProtoArrangemenet().GetAlignedBytes() > 0) )
	{
		strm << L"<div class=\"headline\">Members</div>\r\n"
				L"<div class=\"indent2\">\r\n" ;
		MakeDocVariableList
			( strm, L"member_", pClass->GetPrototypeObject(),
								pClass->GetProtoArrangemenet() ) ;
		strm << L"</div><br/>\r\n\r\n" ;
	}

	// 静的関数リスト
	if ( pClass->GetStaticFunctionList().size() > 0 )
	{
		strm << L"<div class=\"headline\">Static Functions</div>\r\n"
				L"<div class=\"indent2\">\r\n" ;
		for ( auto iter : pClass->GetStaticFunctionList() )
		{
			strm << L"	<a href=\"#func_"
					<< LXMLDocParser::EncodeXMLString( iter.first.c_str() ) << L"\">"
					<< LXMLDocParser::EncodeXMLString( iter.first.c_str() )
					<< L"</a><br/>\r\n" ;
		}
		strm << L"</div><br/>\r\n\r\n" ;
	}

	// 仮想関数リスト
	const LVirtualFuncVector&	vecVirtuals = pClass->GetVirtualVector() ;
	std::set<LString>			setVirtualNames ;
	for ( size_t i = 0; i < vecVirtuals.size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc = vecVirtuals.GetFunctionAt( i ) ;
		if ( (pFunc == nullptr)
			|| (pFunc->GetPrototype() == nullptr)
			|| (pFunc->GetPrototype()->GetThisClass() != pClass)
			|| (pFunc->GetName().Left(9) == L"operator ") )
		{
			continue ;
		}
		setVirtualNames.insert( pFunc->GetName() ) ;
	}
	if ( setVirtualNames.size() > 0 )
	{
		strm << L"<div class=\"headline\">Methods</div>\r\n"
				L"<div class=\"indent2\">\r\n" ;
		for ( auto iter : setVirtualNames )
		{
			LString	strFuncName = iter ;
			if ( strFuncName == LClass::s_Constructor )
			{
				strFuncName = pClass->GetClassName() ;
			}
			strm << L"	<a href=\"#virtual_"
					<< LXMLDocParser::EncodeXMLString( strFuncName.c_str() ) << L"\">"
					<< LXMLDocParser::EncodeXMLString( strFuncName.c_str() )
					<< L"</a><br/>\r\n" ;
		}
		strm << L"</div><br/>\r\n\r\n" ;
	}

	// 演算子リスト
	const std::vector<Symbol::BinaryOperatorDef>&
						vBinOperators = pClass->GetBinaryOperators() ;
	std::set<Symbol::OperatorIndex>	setOperators ;
	if ( vBinOperators.size() > 0 )
	{
		for ( size_t i = 0; i < vBinOperators.size(); i ++ )
		{
			if ( vBinOperators.at(i).m_pThisClass != pClass )
			{
				continue ;
			}
			setOperators.insert( vBinOperators.at(i).m_opIndex ) ;
		}
	}
	if ( setOperators.size() > 0 )
	{
		strm << L"<div class=\"headline\">Operators</div>\r\n"
				L"<div class=\"indent2\">\r\n" ;
		for ( auto i = setOperators.begin(); i != setOperators.end(); i ++ )
		{
			Symbol::OperatorIndex	opIndex = *i ;
			LString	strOperator = Symbol::s_OperatorDescs[opIndex].pwszName ;
			if ( Symbol::s_OperatorDescs[opIndex].pwszPairName != nullptr )
			{
				strOperator += Symbol::s_OperatorDescs[opIndex].pwszPairName ;
			}
			strm << L"	<a href=\"#operator_"
					<< Symbol::s_OperatorDescs[opIndex].pwszFuncName
					<< L"\">operator "
					<< LXMLDocParser::EncodeXMLString( strOperator.c_str() )
					<< L"</a><br/>\r\n" ;
			setOperators.insert( opIndex ) ;
		}
		strm << L"</div><br/>\r\n\r\n" ;
	}

	// 静的メンバ変数
	if ( (pClass->GetElementCount() > 0)
		|| (pClass->GetStaticArrangement().GetAlignedBytes() > 0) )
	{
		strm << L"<div class=\"chapter\">Static Members</div>\r\n" ;
		pClass->AddRef() ;
		MakeDocVariableDesc
			( strm, L"static_", LObjPtr(pClass), pClass->GetStaticArrangement() ) ;
		strm << L"\r\n\r\n" ;
	}

	// メンバ変数
	if ( ((pClass->GetPrototypeObject() != nullptr)
			&& (pClass->GetPrototypeObject()->GetElementCount() > 0))
		|| (pClass->GetProtoArrangemenet().GetAlignedBytes() > 0) )
	{
		strm << L"<div class=\"chapter\">Members</div>\r\n" ;
		MakeDocVariableDesc
			( strm, L"member_", pClass->GetPrototypeObject(),
								pClass->GetProtoArrangemenet() ) ;
		strm << L"\r\n\r\n" ;
	}

	// 静的関数リスト
	if ( pClass->GetStaticFunctionList().size() > 0 )
	{
		strm << L"<div class=\"chapter\">Static Functions</div>\r\n" ;
		for ( auto iter : pClass->GetStaticFunctionList() )
		{
			strm << L"\r\n<a name=\"func_"
				<< LXMLDocParser::EncodeXMLString( iter.first.c_str() ) << L"\"/>\r\n" ;
			for ( size_t i = 0; i < iter.second.size(); i ++ )
			{
				MakeDocFunctionDesc( strm, iter.second.at(i) ) ;
			}
		}
		strm << L"\r\n\r\n" ;
	}

	// 仮想関数リスト
	if ( setVirtualNames.size() > 0 )
	{
		strm << L"<div class=\"chapter\">Methods</div>\r\n" ;
		for ( auto iter : setVirtualNames )
		{
			LString	strFuncName = iter ;
			if ( strFuncName == LClass::s_Constructor )
			{
				strFuncName = pClass->GetClassName() ;
			}
			strm << L"\r\n<a name=\"virtual_"
				<< LXMLDocParser::EncodeXMLString( strFuncName.c_str() ) << L"\"/>\r\n" ;
			const std::vector<size_t> *
				pVirtFuncs = vecVirtuals.FindFunction( iter.c_str() ) ;
			if ( pVirtFuncs == nullptr )
			{
				continue ;
			}
			for ( size_t i = 0; i < pVirtFuncs->size(); i ++ )
			{
				LPtr<LFunctionObj>	pFunc =
						vecVirtuals.GetFunctionAt( pVirtFuncs->at(i) ) ;
				if ( (pFunc == nullptr)
					|| (pFunc->GetPrototype() == nullptr)
					|| (pFunc->GetPrototype()->GetThisClass() != pClass) )
				{
					continue ;
				}
				MakeDocFunctionDesc( strm, pFunc ) ;
			}
		}
		strm << L"\r\n\r\n" ;
	}

	// 演算子リスト
	if ( setOperators.size() > 0 )
	{
		std::set<Symbol::OperatorIndex>	setPrintedOperators ;
		strm << L"<div class=\"chapter\">Operators</div>\r\n" ;
		for ( size_t i = 0; i < vBinOperators.size(); i ++ )
		{
			if ( vBinOperators.at(i).m_pThisClass != pClass )
			{
				continue ;
			}
			Symbol::OperatorIndex	opIndex = vBinOperators.at(i).m_opIndex ;
			if ( setPrintedOperators.count( opIndex ) == 0 )
			{
				strm << L"\r\n<a name=\"operator_"
						<< Symbol::s_OperatorDescs[opIndex].pwszFuncName
						<< L"\"/>\r\n" ;
				setPrintedOperators.insert( opIndex ) ;
			}
			MakeDocBinaryOperatorDesc( strm, vBinOperators.at(i) ) ;
		}
	}

	strm << L"<br/><br/>\r\n"
			L"</body>\r\n"
			L"</html>\r\n" ;
	return	0 ;
}

// クラス概要
void LoquatyApp::MakeDocClassSummary( LOutputStream& strm, LClass * pClass )
{
	strm << L"<div class=\"headline\">Summary</div>\r\n" ;
	strm << L"<div class=\"usage\">" ;
	if ( dynamic_cast<LStructureClass*>( pClass ) != nullptr )
	{
		strm << L"struct "
				<< LXMLDocParser::EncodeXMLString
						( pClass->GetName().c_str() ) << L"<br/>\r\n" ;
		strm << L"{<br/>\r\n" ;

		const LArrangementBuffer&	arrange = pClass->GetProtoArrangemenet() ;
		std::vector<LString>		names ;
		arrange.GetOrderedNameList( names ) ;
		for ( size_t i = 0; i < names.size(); i ++ )
		{
			LArrangement::Desc	desc ;
			if ( arrange.GetDescAs( desc, names.at(i).c_str() ) )
			{
				strm << L"&nbsp; &nbsp; "
					<< LXMLDocParser::EncodeXMLString
							( desc.m_type.GetTypeName().c_str() )
					<< L"&nbsp; &nbsp; " << names.at(i) << L" ;<br/>\r\n" ;
			}
		}

		strm << L"}" ;
	}
	else if ( dynamic_cast<LClass*>( pClass ) != nullptr )
	{
		strm << L"class "
				<< LXMLDocParser::EncodeXMLString
						( pClass->GetName().c_str() ) << L" ;" ;
	}
	else
	{
		strm << L"namespace "
				<< LXMLDocParser::EncodeXMLString( pClass->GetName().c_str() ) ;
	}
	strm << L"</div>\r\n" ;

	LType::LComment *	pComment = pClass->GetSelfComment() ;
	if ( (pComment != nullptr) && HasCommentSummary( *pComment ) )
	{
		MakeDocXMLSummary( strm, *pComment ) ;
		MakeDocXMLDescription( strm, *pComment ) ;
	}

	strm << L"<br/>\r\n\r\n" ;
}

// 親クラスツリー
LString LoquatyApp::MakeDocSuperClass
	( LOutputStream& strm, LClass * pClass, LPackage * pFromPackage )
{
	LClass *	pSuperClass = pClass->GetSuperClass() ;
	LString		strCloser ;
	if ( pSuperClass != nullptr )
	{
		strCloser = MakeDocSuperClass( strm, pSuperClass, pFromPackage ) ;
	}

	LString	strClassName = pClass->GetFullClassName() ;
	LString	strXClassName = LXMLDocParser::EncodeXMLString( strClassName.c_str() ) ;

	strm << L"<div class=\"indent1\"><a href=\""
			<< LXMLDocParser::EncodeXMLString
				( MakeClassDocFileName( pClass, pFromPackage ).c_str() )
			<< L".xhtml\">" << strXClassName << L"</a>\r\n" ;

	if ( m_mapDocClass.find(pClass) == m_mapDocClass.end() )
	{
		std::shared_ptr<LOutputStream>	pStream = OpenClassDocFile( pClass ) ;
		if ( pStream != nullptr )
		{
			m_mapDocClass.insert
				( std::make_pair<LClass*,LString>
					( (LClass*) pClass, pStream->GetFile()->GetFilePath() ) ) ;

			MakeDocClass( *pStream, pClass ) ;
		}
	}
	return	LString( L"</div>" ) + strCloser ;
}

// 変数リスト
void LoquatyApp::MakeDocVariableList
	( LOutputStream& strm, const wchar_t * pwszBase,
			LObjPtr pVar, const LArrangementBuffer& arrange )
{
	if ( pVar != nullptr )
	{
		for ( size_t i = 0; i < pVar->GetElementCount(); i ++ )
		{
			LObjPtr	pElement = pVar->GetElementAt( i ) ;
			LString	strName ;
			pVar->GetElementNameAt( strName, i ) ;

			if ( !strName.IsEmpty() )
			{
				strm << L"	<a href=\"#" << pwszBase << strName << L"\">"
						<< LXMLDocParser::EncodeXMLString( strName.c_str() )
						<< L"</a><br/>\r\n" ;
			}
		}
	}

	std::vector<LString>	names ;
	arrange.GetOrderedNameList( names ) ;

	for ( size_t i = 0; i < names.size(); i ++ )
	{
		strm << L"	<a href=\"#" << pwszBase
				<< LXMLDocParser::EncodeXMLString( names.at(i).c_str() ) << L"\">"
				<< LXMLDocParser::EncodeXMLString( names.at(i).c_str() )
				<< L"</a><br/>\r\n" ;
	}
}

// 変数説明
void LoquatyApp::MakeDocVariableDesc
	( LOutputStream& strm, const wchar_t * pwszBase,
		LObjPtr pVar, const LArrangementBuffer& arrange )
{
	if ( pVar != nullptr )
	{
		for ( size_t i = 0; i < pVar->GetElementCount(); i ++ )
		{
			LObjPtr	pElement = pVar->GetElementAt( i ) ;
			LString	strName ;
			pVar->GetElementNameAt( strName, i ) ;

			if ( !strName.IsEmpty() )
			{
				strm << L"<a name=\"" << pwszBase
					<< LXMLDocParser::EncodeXMLString( strName.c_str() ) << L"\"/>\r\n" ;

				LType	typeVar = pVar->GetElementTypeAt( i ) ;
				MakeDocVariableDesc
						( strm, strName.c_str(), typeVar, pElement ) ;
			}
		}
	}

	std::vector<LString>	names ;
	arrange.GetOrderedNameList( names ) ;

	for ( size_t i = 0; i < names.size(); i ++ )
	{
		LArrangement::Desc	desc ;
		if ( arrange.GetDescAs( desc, names.at(i).c_str() ) )
		{
			strm << L"<a name=\"" << pwszBase
				<< LXMLDocParser::EncodeXMLString( names.at(i).c_str() ) << L"\"/>\r\n" ;

			LPtr<LPointerObj>	pPtr =
				new LPointerObj( m_vm->GetPointerClassAs( desc.m_type ) ) ;
			pPtr->SetPointer( arrange.GetBuffer(), desc.m_location, desc.m_size ) ;

			MakeDocVariableDesc
				( strm, names.at(i).c_str(), desc.m_type, pPtr ) ;
		}
	}
}

void LoquatyApp::MakeDocVariableDesc
	( LOutputStream& strm, const wchar_t * pwszName,
		const LType& typeVar, LObjPtr pVarInit )
{
	strm << L"<div class=\"headline\">"
		<< LXMLDocParser::EncodeXMLString( pwszName ) << L"</div>\r\n" ;
	strm << L"<div class=\"usage\">"
			<< MakeTypeModifiers
					( typeVar.GetModifiers() & ~LType::modifierConst )
			<< L" " << LXMLDocParser::EncodeXMLString
						( typeVar.GetTypeName().c_str() ) << L"&#9; " << pwszName ;

	if ( pVarInit != nullptr )
	{
		LString	strExpr = LObject::ToExpression( pVarInit.Ptr() ) ;
		if ( !strExpr.IsEmpty() )
		{
			strm << L" = " << strExpr ;
		}
	}
	strm << L"</div>\r\n" ;

	LType::LComment *	pComment = typeVar.GetComment() ;
	if ( (pComment != nullptr) && HasCommentSummary(*pComment) )
	{
		MakeDocXMLSummary( strm, *pComment ) ;
		MakeDocXMLDescription( strm, *pComment ) ;
	}

	strm << L"<br/>\r\n" ;
}

// アクセス修飾子
LString LoquatyApp::MakeTypeModifiers( LType::Modifiers modifiers )
{
	LString	strModifiers ;
	if ( modifiers & LType::modifierDeprecated )
	{
		strModifiers += L"@deprecated<br/>\r\n" ;
	}
	switch ( modifiers & LType::accessMask )
	{
	case	LType::modifierPublic:
		strModifiers += L"public" ;
		break ;
	case	LType::modifierProtected:
		strModifiers += L"protected" ;
		break ;
	case	LType::modifierPrivate:
	case	LType::modifierPrivateInvisible:
		strModifiers += L"private" ;
		break ;
	}
	if ( modifiers & LType::modifierStatic )
	{
		strModifiers += L" static" ;
	}
	if ( modifiers & LType::modifierAbstract )
	{
		strModifiers += L" abstract" ;
	}
	if ( modifiers & LType::modifierNative )
	{
		strModifiers += L" native" ;
	}
	if ( modifiers & LType::modifierSynchronized )
	{
		strModifiers += L" synchronized" ;
	}
	if ( modifiers & LType::modifierConst )
	{
		strModifiers += L" const" ;
	}
	return	strModifiers ;
}

// 関数説明
void LoquatyApp::MakeDocFunctionDesc
	( LOutputStream& strm, LPtr<LFunctionObj> pFunc )
{
	std::shared_ptr<LPrototype>	pProto = pFunc->GetPrototype() ;
	assert( pProto != nullptr ) ;

	strm << L"<div class=\"headline\">"
		<< LXMLDocParser::EncodeXMLString
				( pFunc->GetPrintName().c_str() ) << L"</div>\r\n" ;
	strm << L"<div class=\"usage\">"
			<< MakeTypeModifiers
					( pProto->GetModifiers() & ~LType::modifierConst ) ;
	if ( pFunc->GetName() != LClass::s_Constructor )
	{
		strm << L" " << LXMLDocParser::EncodeXMLString
							( pProto->GetReturnType().GetTypeName().c_str() ) ;
	}
	strm << L" " << LXMLDocParser::EncodeXMLString
						( pFunc->GetPrintName().c_str() ) << L"(" ;

	const LNamedArgumentListType&	argList = pProto->GetArgListType() ;
	const std::vector<LValue>&		argDefaults = pProto->GetDefaultArgList() ;
	for ( size_t i = 0; i < argList.size(); i ++ )
	{
		if ( i > 0 )
		{
			strm << L"," ;
		}
		strm << L" " << LXMLDocParser::EncodeXMLString
							( argList.at(i).GetTypeName().c_str() )
				<< L" " << argList.GetArgNameAt(i) ;

		const LValue&	valDef = argDefaults.at(i) ;
		if ( !valDef.IsVoid() )
		{
			if ( valDef.GetType().IsObject() )
			{
				LString	str = LObject::ToExpression( valDef.GetObject().Ptr() ) ;
				if ( !str.IsEmpty() )
				{
					strm << L" = " << str ;
				}
			}
			else if ( valDef.GetType().IsBoolean() )
			{
				if ( valDef.AsInteger() )
				{
					strm << L" = true" ;
				}
				else
				{
					strm << L" = false" ;
				}
			}
			else if ( valDef.GetType().IsFloatingPointNumber() )
			{
				strm << L" = " << LString::NumberOf( valDef.AsDouble() ) ;
			}
			else
			{
				LString	str ;
				LPointerObj::ExprIntAsString( str, valDef.AsInteger() ) ;
				strm << L" = " << str ;
			}
		}
	}

	strm << L" )" << MakeTypeModifiers
						( pProto->GetModifiers() & LType::modifierConst )
			<< L"</div>\r\n" ;

	LType::LComment *	pComment = pProto->GetComment() ;
	if ( pComment != nullptr )
	{
		MakeDocXMLSummary( strm, *pComment ) ;
		MakeDocXMLParams( strm, *pComment ) ;
		MakeDocXMLDescription( strm, *pComment ) ;
	}

	strm << L"<br/>\r\n" ;
}

// 演算子
void LoquatyApp::MakeDocBinaryOperatorDesc
	( LOutputStream& strm, const Symbol::BinaryOperatorDef& bodef )
{
	if ( bodef.m_pOpFunc != nullptr )
	{
		bodef.m_pOpFunc->AddRef() ;
		MakeDocFunctionDesc( strm, LPtr<LFunctionObj>( bodef.m_pOpFunc ) ) ;
		return ;
	}

	const Symbol::OperatorIndex	opIndex = bodef.m_opIndex ;

	LString	strOperator = Symbol::s_OperatorDescs[opIndex].pwszName ;
	if ( Symbol::s_OperatorDescs[opIndex].pwszPairName != nullptr )
	{
		strOperator += Symbol::s_OperatorDescs[opIndex].pwszPairName ;
	}

	strm << L"<div class=\"headline\">operator "
			<< LXMLDocParser::EncodeXMLString( strOperator.c_str() ) << L"</div>\r\n" ;
	strm << L"<div class=\"usage\">"
			<< LXMLDocParser::EncodeXMLString
							( bodef.m_typeRet.GetTypeName().c_str() ) ;
	strm << L" operator "
			<< LXMLDocParser::EncodeXMLString( strOperator.c_str() ) << L" (" ;
	strm << L" " << LXMLDocParser::EncodeXMLString
						( bodef.m_typeRight.GetTypeName().c_str() ) << " )" ;
	if ( bodef.m_constThis )
	{
		strm << L" const" ;
	}
	strm << L"</div>\r\n" ;

	if ( bodef.m_pwszComment != nullptr )
	{
		LType::LComment	comment = bodef.m_pwszComment ;
		MakeDocXMLSummary( strm, comment ) ;
		MakeDocXMLParams( strm, comment ) ;
		MakeDocXMLDescription( strm, comment ) ;
	}

	strm << L"<br/>\r\n" ;
}

// コメントを解釈する
// タグが存在しない場合テキストを変換して <summary> に変換
void LoquatyApp::MakeComment( LType::LComment& comment )
{
	if ( comment.m_xmlDoc == nullptr )
	{
		LXMLDocParser	xmlParser = comment ;
		comment.m_xmlDoc = xmlParser.ParseDocument() ;
		if ( (comment.m_xmlDoc->GetElementCount() == 1)
			&& (comment.m_xmlDoc->FindElement( LXMLDocument::typeText ) == 0) )
		{
			xmlParser = comment.Replace( L"\r\n", L"\n" ).
								Replace( L"\n", L"<br/>\r\n" ) ;
			LXMLDocPtr	pSummary = xmlParser.ParseDocument() ;
			pSummary->SetTag( L"summary" ) ;

			comment.m_xmlDoc = std::make_shared<LXMLDocument>() ;
			comment.m_xmlDoc->AddElement( pSummary ) ;
		}
	}
}

// コメントに <summary> が存在するか？
bool LoquatyApp::HasCommentSummary( LType::LComment& comment )
{
	MakeComment( comment ) ;
	if ( comment.m_xmlDoc->GetTagAs( L"summary" ) != nullptr )
	{
		return	true ;
	}
	return	(comment.m_xmlDoc->GetElementCount() >= 2)
			&& (comment.m_xmlDoc->FindElement( LXMLDocument::typeText ) == 0) ;
}

// コメントの <summary> タグ内、あるいは先頭のテキスト要素を出力
// <summary>概要</summary>
void LoquatyApp::MakeDocXMLSummary
		( LOutputStream& strm, LType::LComment& comment )
{
	MakeComment( comment ) ;

	LXMLDocPtr	pSummary = comment.m_xmlDoc->GetTagAs( L"summary" ) ;
	if ( pSummary != nullptr )
	{
		LXMLDocParser	xmlParser ;
		strm << L"<div class=\"normal\">\r\n" ;
		xmlParser.SerializeElements( strm, *pSummary ) ;
		strm << L"</div>\r\n" ;
	}
	else if ( (comment.m_xmlDoc->GetElementCount() >= 2)
		&& (comment.m_xmlDoc->FindElement( LXMLDocument::typeText ) == 0) )
	{
		LXMLDocParser	xmlParser ;
		strm << L"<div class=\"normal\">\r\n" ;
		xmlParser.Serialize( strm, *(comment.m_xmlDoc->GetElementAt(0)) ) ;
		strm << L"</div>\r\n" ;
	}
}

// <param name="引数名">引数の説明</param>
// <return>返り値の説明</return>
void LoquatyApp::MakeDocXMLParams
		( LOutputStream& strm, LType::LComment& comment )
{
	MakeComment( comment ) ;

	if ( (comment.m_xmlDoc->GetTagAs(L"param") == nullptr)
		&& (comment.m_xmlDoc->GetTagAs(L"return") == nullptr) )
	{
		return ;
	}
	strm << L"<div class=\"notes_parameter\">\r\n" ;

	LXMLDocParser	xmlParser ;
	for ( size_t i = 0; i < comment.m_xmlDoc->GetElementCount(); i ++ )
	{
		LXMLDocPtr	pTag = comment.m_xmlDoc->GetElementAt( i ) ;
		if ( (pTag == nullptr)
			|| (pTag->GetType() != LXMLDocument::typeTag)
			|| (pTag->GetTag() != L"param")
			|| !pTag->HasAttribute( L"name" ) )
		{
			continue ;
		}
		strm << L"<div class=\"param_name\">"
				<< LXMLDocParser::EncodeXMLString
					( pTag->GetAttrString( L"name" ).c_str() ) << L"</div>\r\n" ;
		strm << L"<div class=\"param_desc\">\r\n" ;
		xmlParser.SerializeElements( strm, *pTag ) ;
		strm << L"</div>\r\n" ;
	}

	LXMLDocPtr	pReturn = comment.m_xmlDoc->GetTagAs( L"return" ) ;
	if ( pReturn != nullptr )
	{
		strm << L"<div class=\"param_name\">Return Value</div>\r\n" ;
		strm << L"<div class=\"param_desc\">\r\n" ;
		xmlParser.SerializeElements( strm, *pReturn ) ;
		strm << L"</div>\r\n" ;
	}

	strm << L"</div>\r\n" ;
}

// <desc>関数の詳細説明</desc>
void LoquatyApp::MakeDocXMLDescription
		( LOutputStream& strm, LType::LComment& comment )
{
	MakeComment( comment ) ;

	LXMLDocPtr	pDesc = comment.m_xmlDoc->GetTagAs( L"desc" ) ;
	if ( pDesc != nullptr )
	{
		LXMLDocParser	xmlParser ;
		strm << L"<div class=\"indent1\">\r\n" ;
		xmlParser.SerializeElements( strm, *pDesc ) ;
		strm << L"</div><br/>\r\n" ;
	}
}

// css ファイルを出力する
//////////////////////////////////////////////////////////////////////////////
bool LoquatyApp::WriteCssFile( const wchar_t * pwszFilePath )
{
	LFilePtr	file =
		LURLSchemer::Open( pwszFilePath, LDirectory::modeCreate ) ;

	if ( file == nullptr )
	{
		printf( "ファイルを開けませんでした : \'%s\'\n",
					LString(pwszFilePath).ToString().c_str() ) ;
		return	false ;
	}

	LOutputUTF8Stream	strm( file ) ;

	static const wchar_t *	s_pwszCss =
	L"\r\n"
	L"/* タイトル */\r\n"
	L"div.title0\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\ttext-align: left;\r\n"
	L"\ttext-decoration: underline;\r\n"
	L"\tfont-size: 300%;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"\tpadding: 8px 8px 8px 8px;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 章題 */\r\n"
	L"div.chapter\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\ttext-align: left;\r\n"
	L"\ttext-decoration: underline;\r\n"
	L"\tfont-size: 200%;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: italic;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"\tmargin-top: 1em;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* ヘッドライン */\r\n"
	L"div.headline\r\n"
	L"{\r\n"
	L"\tcolor: #ffffff;\r\n"
	L"\tbackground-color: #000000;\r\n"
	L"\ttext-align: left;\r\n"
	L"\tfont-size: 120%;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: italic;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"\tmargin: 0.5em 4px 0.5em 4px;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 項目見出し */\r\n"
	L"div.term_name\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tbackground-color: #E0E0E0;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"\tmargin: 8px 4px 8px 4px;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 本文 */\r\n"
	L"div.normal\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tmargin-left: 1em;\r\n"
	L"\tmargin-right: 1em;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 本文インデント1 */\r\n"
	L"div.indent1\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tmargin-left: 2em;\r\n"
	L"\tmargin-right: 2em;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 本文インデント2 */\r\n"
	L"div.indent2\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tmargin-left: 4em;\r\n"
	L"\tmargin-right: 4em;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 注記インデント1 */\r\n"
	L"div.notes_indent1\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tborder: solid 1px #C0C0C0;\r\n"
	L"\tmargin-left: 2em;\r\n"
	L"\tmargin-right: 2em;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"\tfont-size: 85%;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 書式 */\r\n"
	L"div.usage\r\n"
	L"{\r\n"
	L"\tborder: double 3px #808080;\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tbackground-color: #E8E8E8;\r\n"
	L"\tfont-size: 100%;\r\n"
	L"\tfont-family: monospace, serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"\tmargin: 1em 1em 1em 1em;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* パラメータ */\r\n"
	L"div.notes_parameter\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tborder: solid 1px #C0C0C0;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"\tfont-size: 100%;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"\tmargin: 1em 2em 1.5em 2em;\r\n"
	L"}\r\n"
	L"\r\n"
	L"div.param_name\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tbackground-color: #F0F0F0;\r\n"
	L"\tpadding: 2px 4px 2px 8px;\r\n"
	L"\tmargin: 8px 4px 8px 4px;\r\n"
	L"\tfont-size: 95%;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-style: italic;\r\n"
	L"}\r\n"
	L"\r\n"
	L"div.param_desc\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tmargin-left: 2em;\r\n"
	L"\tmargin-right: 2em;\r\n"
	L"\tmargin-bottom: 1em;\r\n"
	L"\tfont-family: serif;\r\n"
	L"\tfont-size: 85%;\r\n"
	L"\tfont-style: normal;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* コード */\r\n"
	L"div.code_quote\r\n"
	L"{\r\n"
	L"\tcolor: #000000;\r\n"
	L"\tbackground-color: #E8E8E8;\r\n"
	L"\tfont-size: 90%;\r\n"
	L"\tfont-family: monospace, serif;\r\n"
	L"\tfont-style: normal;\r\n"
	L"\tpadding: 4px 4px 4px 4px;\r\n"
	L"\tmargin: 1em 4em 1em 4em;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 斜体 */\r\n"
	L"italic\r\n"
	L"{\r\n"
	L"\tfont-style: italic;\r\n"
	L"}\r\n"
	L"\r\n"
	L"/* 太字 */\r\n"
	L"bold\r\n"
	L"{\r\n"
	L"\tfont-weight: bold;\r\n"
	L"}\r\n" ;

	strm << s_pwszCss ;
	return	true ;
}

// パッケージに含まれるクラスのインデックスを文書化する
//////////////////////////////////////////////////////////////////////////////
int LoquatyApp::MakeDocIndexInPackage( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}

	// パッケージ取得
	LPackagePtr	pPackage = nullptr ;
	for ( auto p : m_vm->GetPackageList() )
	{
		if ( p->GetName() == m_strMakeTarget )
		{
			pPackage = p ;
			break ;
		}
	}
	if ( pPackage == nullptr )
	{
		printf( "\n\'%s\' パッケージが見つかりません\n",
					m_strMakeTarget.ToString().c_str() ) ;
		return	1 ;
	}

	return	MakeDocIndexInPackage( pPackage ) ;
}

int LoquatyApp::MakeDocIndexInPackage( LPackagePtr pPackage )
{
	int	nExitCode = 0 ;

	// css ファイルを出力
	LString	strBaseDir =
		LURLSchemer::SubPath
			( m_strMakeOutput.c_str(), pPackage->GetName().c_str() ) ;
	WriteCssFile( (strBaseDir + L"/specifications.css").c_str() ) ;

	// インデックスファイルを開く
	LString		strIndexFile =
		LURLSchemer::SubPath( strBaseDir.c_str(), L"index.xhtml" ) ;

	LFilePtr	file =
		LURLSchemer::Open( strIndexFile.c_str(), LDirectory::modeCreate ) ;

	LOutputUTF8Stream	strm( file ) ;

	printf( "%s...\n", strIndexFile.ToString().c_str() ) ;

	// ヘッダ・見出し
	strm << L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
			L"<!DOCTYPE html\r\n"
			L"[\r\n"
			L"	<!ENTITY nbsp \"&#160;\">\r\n"
			L"]>\r\n"
			L"<?xml-stylesheet type=\"text/css\" href=\"specifications.css\"?>\r\n"
			L"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"ja\" lang=\"ja\">\r\n"
			L"<head>\r\n"
			L"	<title>" << pPackage->GetName() << L"</title>\r\n"
			L"</head>\r\n"
			L"<body>\r\n"
			L"<div class=\"chapter\">" << pPackage->GetName() << L"</div>\r\n\r\n" ;

	// クラス一覧
	if ( CoundClassesInPackage( pPackage ) > 0 )
	{
		strm << L"<div class=\"headline\">Classes</div>\r\n"
				<< L"<div class=\"indent2\">"
				<< L"<a href=\"../index.xhtml\">&lt;..&gt;</a></div>\r\n" ;

		nExitCode += MakeDocClassesInPackage( strm, pPackage, false ) ;

		strm << L"<br/>\r\n" ;
	}

	// 型定義一覧
	if ( CoundTypesInPackage( pPackage ) > 0 )
	{
		strm << L"<div class=\"headline\">Types</div>\r\n" ;

		nExitCode += MakeDocTypesInPackage( strm, pPackage, false ) ;

		strm << L"<br/>\r\n" ;
	}

	strm << L"</body>\r\n"
			L"</html>\r\n" ;
	return	nExitCode ;
}

int LoquatyApp::MakeDocClassesInPackage
	( LOutputStream& strm, LPackagePtr pPackage, bool flagRootIndex )
{
	int	nExitCode = 0 ;

	std::map<LString,LClass*>	mapClasses ;
	for ( LClass * pClass : pPackage->GetClasses() )
	{
		if ( pClass->IsGenericInstance() )
		{
			continue ;
		}
		mapClasses.insert
			( std::make_pair<LString,LClass*>
				( pClass->GetFullClassName(), (LClass*) pClass ) ) ;
	}
	for ( auto iter = mapClasses.begin(); iter != mapClasses.end(); iter ++ )
	{
		LString		strName = iter->first ;
		LClass *	pClass = iter->second ;
		LString		strXName = LXMLDocParser::EncodeXMLString( strName.c_str() ) ;
		strm << L"<div class=\"indent2\"><a href=\""
				<< LXMLDocParser::EncodeXMLString
						( MakeClassDocFileName
							( pClass, flagRootIndex ? nullptr : pClass ).c_str() )
				<< L".xhtml\">" << strXName << L"</a></div>\r\n" ;

		// クラスを文書化して出力
		if ( m_mapDocClass.find(pClass) == m_mapDocClass.end() )
		{
			std::shared_ptr<LOutputStream>	pStream = OpenClassDocFile( pClass ) ;
			if ( pStream != nullptr )
			{
				m_mapDocClass.insert
					( std::make_pair<LClass*,LString>
						( (LClass*) pClass, pStream->GetFile()->GetFilePath() ) ) ;

				nExitCode += MakeDocClass( *pStream, pClass ) ;
			}
			else
			{
				nExitCode ++ ;
			}
		}
	}
	return	nExitCode ;
}

int LoquatyApp::MakeDocTypesInPackage
	( LOutputStream& strm, LPackagePtr pPackage, bool flagRootIndex )
{
	int	nExitCode = 0 ;

	for ( size_t i = 0; i < pPackage->GetTypes().size(); i ++ )
	{
		const LPackage::TypeDefDesc& tdd = pPackage->GetTypes().at(i) ;
		if ( tdd.m_strName.GetAt(0) == L'@' )
		{
			continue ;
		}
		const LType *	pType = tdd.m_pNamespace->GetTypeAs( tdd.m_strName.c_str() ) ;
		assert( pType != nullptr ) ;
		LString	strTypeName ;
		if ( tdd.m_pNamespace->GetName().IsEmpty() )
		{
			strTypeName = tdd.m_strName ;
		}
		else
		{
			strTypeName = tdd.m_pNamespace->GetName() + L"." + tdd.m_strName ;
		}
		LString	strFileName ;
		if ( flagRootIndex )
		{
			strFileName = pPackage->GetName() + L"/" ;
		}
		strFileName += MakeTypeFileName(strTypeName) + L".xhtml" ;

		strm << L"<div class=\"indent2\"><a href=\""
				<< LXMLDocParser::EncodeXMLString( strFileName.c_str() ) << L"\">"
				<< LXMLDocParser::EncodeXMLString( strTypeName.c_str() )
				<< L"</a></div>\r\n" ;

		// 型定義を文書化して出力
		if ( !flagRootIndex )
		{
			strFileName = pPackage->GetName() + L"/" + strFileName ;
		}
		std::shared_ptr<LOutputStream>
					pStream = OpenDocFile( strFileName.c_str() ) ;
		if ( pStream != nullptr )
		{
			nExitCode += MakeDocTypeDef
				( *pStream, strTypeName.c_str(), *pType, pPackage.get() ) ;
		}
		else
		{
			nExitCode ++ ;
		}
	}
	return	nExitCode ;
}

// 全てのパッケージのクラスを文書化する
int LoquatyApp::MakeDocAllPackages( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}

	// css ファイルを出力
	WriteCssFile( (m_strMakeOutput + L"/specifications.css").c_str() ) ;

	// インデックスファイルを開く
	LString		strIndexFile =
		LURLSchemer::SubPath( m_strMakeOutput.c_str(), L"index.xhtml" ) ;

	LFilePtr	file =
		LURLSchemer::Open( strIndexFile.c_str(), LDirectory::modeCreate ) ;

	LOutputUTF8Stream	strm( file ) ;

	// ヘッダ・見出し
	strm << L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
			L"<!DOCTYPE html\r\n"
			L"[\r\n"
			L"	<!ENTITY nbsp \"&#160;\">\r\n"
			L"]>\r\n"
			L"<?xml-stylesheet type=\"text/css\" href=\"specifications.css\"?>\r\n"
			L"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"ja\" lang=\"ja\">\r\n"
			L"<head>\r\n"
			L"	<title>Packages</title>\r\n"
			L"</head>\r\n"
			L"<body>\r\n"
			L"<div class=\"chapter\">Packages</div>\r\n\r\n" ;

	for ( auto p : m_vm->GetPackageList() )
	{
		if ( CoundClassesInPackage(p) + CoundTypesInPackage(p) == 0 )
		{
			continue ;
		}
		strm << L"<div class=\"headline\">" << p->GetName() << L"</div>\r\n" ;

		nExit += MakeDocClassesInPackage( strm, p, true ) ;
		nExit += MakeDocTypesInPackage( strm, p, true ) ;

		strm << L"<br/>\r\n\r\n" ;

		// パッケージに含まれるクラスを文書化
		MakeDocIndexInPackage( p ) ;
	}

	strm << L"</body>\r\n"
			L"</html>\r\n" ;

	return	nExit ;
}

size_t LoquatyApp::CoundClassesInPackage( LPackagePtr pPackage ) const
{
	size_t	nNoGenClasses = 0 ;
	for ( LClass * pClass : pPackage->GetClasses() )
	{
		if ( !pClass->IsGenericInstance() )
		{
			nNoGenClasses ++ ;
			break ;
		}
	}
	return	nNoGenClasses ;
}

size_t LoquatyApp::CoundTypesInPackage( LPackagePtr pPackage ) const
{
	size_t	nTypeCount = 0 ;
	for ( size_t i = 0; i < pPackage->GetTypes().size(); i ++ )
	{
		const LPackage::TypeDefDesc& tdd = pPackage->GetTypes().at(i) ;
		if ( tdd.m_strName.GetAt(0) == L'@' )
		{
			continue ;
		}
		nTypeCount ++ ;
	}
	return	nTypeCount ;
}

// クラスの native 関数宣言・実装のテンプレートを出力する
//////////////////////////////////////////////////////////////////////////////
int LoquatyApp::MakeNativeFuncStubClass( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}
	LSourceFile	source = m_strMakeTarget ;

	// クラス取得
	LType	type ;
	if ( !source.NextTypeExpr( type, false, *m_vm, nullptr )
		|| (type.GetClass() == nullptr) )
	{
		printf( "\n\'%s\' クラスが見つかりません\n",
					m_strMakeTarget.ToString().c_str() ) ;
		return	1 ;
	}
	return	MakeNativeFuncStubClass( type.GetClass() ) ;
}

int LoquatyApp::MakeNativeFuncStubClass( LClass * pClass )
{
	// 出力ファイルを開く
	LString		strHeaderFile =
		LURLSchemer::SubPath
			( m_strMakeOutput.c_str(),
				(MakeClassDocFileName( pClass, pClass ) + L".h").c_str() ) ;
	LString		strCppFile =
		LURLSchemer::SubPath
			( m_strMakeOutput.c_str(),
				(MakeClassDocFileName( pClass, pClass ) + L".cpp").c_str() ) ;

	LFilePtr	fileHdr =
		LURLSchemer::Open( strHeaderFile.c_str(), LDirectory::modeCreate ) ;
	if ( fileHdr == nullptr )
	{
		printf( "ファイルを開けませんでした : \'%s\'\n",
						strHeaderFile.ToString().c_str() ) ;
		return	1 ;
	}
	LFilePtr	fileCpp =
		LURLSchemer::Open( strCppFile.c_str(), LDirectory::modeCreate ) ;
	if ( fileCpp == nullptr )
	{
		printf( "ファイルを開けませんでした : \'%s\'\n",
						strCppFile.ToString().c_str() ) ;
		return	1 ;
	}
	printf( "%s...\n", strHeaderFile.ToString().c_str() ) ;
	printf( "%s...\n", strCppFile.ToString().c_str() ) ;

	LString	strCppClass = m_strCppClass ;
	if ( strCppClass.IsEmpty() )
	{
		strCppClass = MakeCppClassName( pClass ) ;
	}
	LOutputUTF8Stream	osHeader( fileHdr ) ;
	LOutputUTF8Stream	osCpp( fileCpp ) ;
	osHeader << L"\r\n\r\n" ;
	osCpp << L"\r\n"
			<< L"#include <loquaty.h>\r\n"
			<< L"\r\n"
			<< L"using namespace Loquaty ;\r\n"
			<< L"\r\n\r\n" ;

	int	nExit = MakeNativeFuncStubClass
					( osHeader, osCpp, pClass, strCppClass.c_str() ) ;

	osHeader << L"\r\n\r\n" ;
	osCpp << L"\r\n\r\n" ;
	return	nExit ;
}

int LoquatyApp::MakeNativeFuncStubClass
	( LOutputStream& osHeader, LOutputStream& osCpp,
			LClass * pClass, const wchar_t * pwszCppClass )
{
	int	nExit = 0 ;
	for ( auto iter : pClass->GetStaticFunctionList() )
	{
		for ( size_t i = 0; i < iter.second.size(); i ++ )
		{
			LPtr<LFunctionObj>	pFunc = iter.second.at(i) ;
			if ( (pFunc != nullptr)
				&& (pFunc->GetPrototype() != nullptr)
				&& (pFunc->GetPrototype()->GetModifiers() & LType::modifierNative) )
			{
				nExit += MakeNativeFunctionStub
							( osHeader, osCpp, pFunc, pwszCppClass ) ;
			}
		}
	}

	// 仮想関数リスト
	const LVirtualFuncVector&	vecVirtuals = pClass->GetVirtualVector() ;
	for ( size_t i = 0; i < vecVirtuals.size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc = vecVirtuals.GetFunctionAt( i ) ;
		if ( (pFunc != nullptr)
			&& (pFunc->GetThisClass() == pClass)
			&& (pFunc->GetPrototype() != nullptr)
			&& (pFunc->GetPrototype()->GetModifiers() & LType::modifierNative) )
		{
			nExit += MakeNativeFunctionStub
						( osHeader, osCpp, pFunc, pwszCppClass ) ;
		}
	}

	return	nExit ;
}

int LoquatyApp::MakeNativeFunctionStub
	( LOutputStream& osHeader, LOutputStream& osCpp,
		LPtr<LFunctionObj> pFunc, const wchar_t * pwszCppClass )
{
	int			nExit = 0 ;
	LClass *	pThisClass = nullptr ;
	if ( pFunc->GetName() == LClass::s_Constructor )
	{
		//
		// コンストラクタ
		//
		if ( pFunc->GetThisClass() == nullptr )
		{
			printf( "コンストラクタにクラス情報がありません。\n" ) ;
			return	1 ;
		}
		LClass *	pClass = pFunc->GetThisClass() ;
		const bool	flagNativeObjClass = IsNativeClass( pClass ) ;
		pThisClass = pClass ;

		// プロトタイプをコメントに出力
		osCpp << L"// " << pFunc->GetThisClass()->GetClassName() ;
		OutputStubFuncArgList( osCpp, pFunc ) ;
		osCpp << L"\r\n" ;

		// 関数の宣言と実装文の冒頭を出力
		LString	strStubClassName =
			MakeStubFunctionName
				( pFunc->GetThisClass()->GetFullClassName(), nExit ) ;
		if ( pFunc->GetVariationIndex() == 0 )
		{
			osHeader << L"DECL_LOQUATY_CONSTRUCTOR(" << strStubClassName << L");\r\n" ;
			osCpp << L"IMPL_LOQUATY_CONSTRUCTOR(" << strStubClassName << L")\r\n" ;
		}
		else
		{
			osHeader << L"DECL_LOQUATY_CONSTRUCTOR_N(" << strStubClassName << L","
						<< (unsigned int) pFunc->GetVariationIndex() << L");\r\n" ;
			osCpp << L"IMPL_LOQUATY_CONSTRUCTOR_N(" << strStubClassName << L","
						<< (unsigned int) pFunc->GetVariationIndex() << L")\r\n" ;
		}
		osCpp << L"{\r\n"
				<< L"	LQT_FUNC_ARG_LIST ;\r\n" ;
		if ( flagNativeObjClass )
		{
			osCpp << L"	LQT_FUNC_THIS_INIT_NOBJ( "
						<< pwszCppClass << L", pThis, () ) ;\r\n" ;
		}
		else
		{
			osCpp << L"	LQT_FUNC_THIS_OBJ( " << pwszCppClass << L", pThis ) ;\r\n" ;
		}
	}
	else
	{
		//
		// コンストラクタ以外の関数
		//
		LString	strFullFuncName = pFunc->GetFullName() ;
		LString	strFuncName = pFunc->GetName() ;
		if ( strFuncName.Left(9) == L"operator " )
		{
			// operator 関数
			if ( pFunc->GetParentNamespace() != nullptr )
			{
				strFullFuncName = pFunc->GetParentNamespace()->GetFullName() ;
				strFullFuncName += L".operator." ;
			}
			else
			{
				strFullFuncName = L"operator." ;
			}
			LString	strOp = strFuncName.Middle( 9, strFuncName.GetLength() ) ;
			for ( int i = 0; i < Symbol::opOperatorCount; i ++ )
			{
				if ( Symbol::s_OperatorDescs[i].pwszPairName == nullptr )
				{
					if ( strOp == Symbol::s_OperatorDescs[i].pwszName )
					{
						strFullFuncName += Symbol::s_OperatorDescs[i].pwszFuncName ;
						break ;
					}
				}
				else
				{
					LString	strOpName = Symbol::s_OperatorDescs[i].pwszName ;
					strOpName += Symbol::s_OperatorDescs[i].pwszPairName ;
					if ( strOp == strOpName )
					{
						strFullFuncName += Symbol::s_OperatorDescs[i].pwszFuncName ;
						break ;
					}
				}
			}
		}
		if ( pFunc->GetVariationIndex() > 0 )
		{
			strFullFuncName += L"." ;
			strFullFuncName += LString::IntegerOf( pFunc->GetVariationIndex() ) ;
		}
		LString	strStubFuncName = MakeStubFunctionName( strFullFuncName, nExit ) ;

		// プロトタイプをコメントに出力
		osCpp << L"// " << pFunc->GetReturnType().GetTypeName()
				<< L" " << pFunc->GetPrintName() ;
		OutputStubFuncArgList( osCpp, pFunc ) ;
		osCpp << L"\r\n" ;

		// 関数の宣言と実装文の冒頭を出力
		osHeader << L"DECL_LOQUATY_FUNC(" << strStubFuncName << L");\r\n" ;
		osCpp << L"IMPL_LOQUATY_FUNC(" << strStubFuncName << L")\r\n" ;
		osCpp << L"{\r\n"
				<< L"	LQT_FUNC_ARG_LIST ;\r\n" ;

		if ( !(pFunc->GetPrototype()->GetModifiers() & LType::modifierStatic)
			&& (pFunc->GetThisClass() != nullptr) )
		{
			LClass *	pClass = pFunc->GetThisClass() ;
			const bool	flagNativeObjClass = IsNativeClass( pClass ) ;
			if ( flagNativeObjClass )
			{
				osCpp << L"	LQT_FUNC_THIS_NOBJ( " << pwszCppClass << L", pThis ) ;\r\n" ;
			}
			else
			{
				osCpp << L"	LQT_FUNC_THIS_OBJ( " << pwszCppClass << L", pThis ) ;\r\n" ;
			}
			pThisClass = pClass ;
		}
	}
	//
	// 関数の引数を受け取る
	//
	std::shared_ptr<LPrototype>		pProto = pFunc->GetPrototype() ;
	assert( pProto != nullptr ) ;

	const LNamedArgumentListType&	argList = pProto->GetArgListType() ;
	const std::vector<LValue>&		argDefaults = pProto->GetDefaultArgList() ;
	LString							strRetValueName = L"valRet" ;
	for ( size_t i = 0; i < argList.size(); i ++ )
	{
		LType	typeArg = argList.at(i) ;
		LString	strArgName = argList.GetArgNameAt(i) ;
		if ( typeArg.IsObject() )
		{
			if ( typeArg.IsPointer() )
			{
				LPointerClass *	pPtrClass = typeArg.GetPointerClass() ;
				assert( pPtrClass != nullptr ) ;

				LType	typeData = pPtrClass->GetDataType() ;
				if ( typeData.IsStructure() )
				{
					osCpp << L"	LQT_FUNC_ARG_STRUCT( "
							<< MakeCppClassName(typeData.GetClass())
							<< L", " << strArgName << L" ) ;\r\n" ;
				}
				else
				{
					osCpp << L"	LQT_FUNC_ARG_POINTER( "
							<< GetPrimitiveTypeName(typeData.GetPrimitive())
							<< L", " << strArgName << L" ) ;\r\n" ;
				}
				osCpp << L"	LQT_VERIFY_NULL_PTR( " << strArgName << L" ) ;\r\n" ;
			}
			else if ( typeArg.IsString() )
			{
				osCpp << L"	LQT_FUNC_ARG_STRING( " << strArgName << L" ) ;\r\n" ;
			}
			else
			{
				LClass *	pArgClass = typeArg.GetClass() ;
				assert( pArgClass != nullptr ) ;

				if ( IsNativeClass( pArgClass ) )
				{
					osCpp << L"	LQT_FUNC_ARG_NOBJ( "
							<< MakeCppClassName(pArgClass)
							<< L", " << strArgName << L" ) ;\r\n" ;
				}
				else
				{
					osCpp << L"	LQT_FUNC_ARG_OBJECT( "
							<< MakeCppClassName(pArgClass)
							<< L", " << strArgName << L" ) ;\r\n" ;
				}
				osCpp << L"	LQT_VERIFY_NULL_PTR( " << strArgName << L" ) ;\r\n" ;
			}
		}
		else
		{
			switch ( typeArg.GetPrimitive() )
			{
			case	LType::typeBoolean:
				osCpp << L"	LQT_FUNC_ARG_BOOL( " << strArgName << L" ) ;\r\n" ;
				break ;

			case	LType::typeInt8:
			case	LType::typeInt16:
			case	LType::typeInt32:
				osCpp << L"	LQT_FUNC_ARG_INT( " << strArgName << L" ) ;\r\n" ;
				break ;

			case	LType::typeUint8:
			case	LType::typeUint16:
			case	LType::typeUint32:
				osCpp << L"	LQT_FUNC_ARG_UINT( " << strArgName << L" ) ;\r\n" ;
				break ;

			default:
			case	LType::typeInt64:
				osCpp << L"	LQT_FUNC_ARG_LONG( " << strArgName << L" ) ;\r\n" ;
				break ;

			case	LType::typeUint64:
				osCpp << L"	LQT_FUNC_ARG_ULONG( " << strArgName << L" ) ;\r\n" ;
				break ;

			case	LType::typeFloat:
				osCpp << L"	LQT_FUNC_ARG_FLOAT( " << strArgName << L" ) ;\r\n" ;
				break ;

			case	LType::typeDouble:
				osCpp << L"	LQT_FUNC_ARG_DOUBLE( " << strArgName << L" ) ;\r\n" ;
				break ;
			}
		}
	}
	osCpp << L"\r\n" ;

	// 返り値変数
	const LType	typeRet = pProto->GetReturnType() ;
	if ( !typeRet.IsVoid() )
	{
		osCpp << L"	" ;
		if ( typeRet.IsObject() )
		{
			if ( typeRet.IsPointer() )
			{
				LPointerClass *	pPtrClass = typeRet.GetPointerClass() ;
				assert( pPtrClass != nullptr ) ;

				LType	typeData = pPtrClass->GetDataType() ;
				if ( typeData.IsStructure() )
				{
					osCpp << MakeCppClassName(typeData.GetClass())
							<< L"\t" << strRetValueName << L" ;\r\n" ;
				}
				else
				{
					osCpp << GetPrimitiveTypeName(typeData.GetPrimitive())
							<< L"\t" << strRetValueName << L" ;\r\n" ;
				}
			}
			else if ( typeRet.IsString() )
			{
				osCpp << L"LString	" << strRetValueName << L" ;\r\n" ;
			}
			else
			{
				LClass *	pRetClass = typeRet.GetClass() ;
				assert( pRetClass != nullptr ) ;
				if ( IsNativeClass( pRetClass ) )
				{
					osCpp << L"LPtr<LNativeObj>	" << strRetValueName
							<< L" = new LNativeObj( LQT_GET_CLASS("
							<< pRetClass->GetFullClassName() << L") ) ;\r\n" ;
				}
				else
				{
					osCpp << L"LObjPtr	" << strRetValueName << L" ;\r\n" ;
				}
			}
		}
		else
		{
			osCpp << GetPrimitiveTypeName( typeRet.GetPrimitive() )
					<< L"\t" << strRetValueName << L" ;\r\n" ;
		}
	}

	if ( pFunc->GetName() == LClass::s_Constructor )
	{
		osCpp << L"	// pThis->Initialize() ;\r\n\r\n" ;
	}
	else
	{
		osCpp << L"	// " ;
		if ( !typeRet.IsVoid() )
		{
			osCpp << strRetValueName << L" = " ;
		}
		if ( pThisClass != nullptr )
		{
			osCpp << L"pThis->" ;
		}
		osCpp << pFunc->GetPrintName() << L"(...) ;\r\n" ;
		osCpp << L"\r\n" ;
	}

	// 返り値
	if ( !typeRet.IsVoid() )
	{
		if ( typeRet.IsObject() )
		{
			if ( typeRet.IsPointer() )
			{
				osCpp << L"	LQT_RETURN_POINTER_STRUCT( "
							<< strRetValueName << L" ) ;\r\n" ;
			}
			else if ( typeRet.IsString() )
			{
				osCpp << L"	LQT_RETURN_STRING( "
							<< strRetValueName << L" ) ;\r\n" ;
			}
			else
			{
				LClass *	pRetClass = typeRet.GetClass() ;
				assert( pRetClass != nullptr ) ;
				if ( IsNativeClass( pRetClass ) )
				{
					osCpp << L"	// ToDo: set std::shared_ptr<"
								<< MakeCppClassName(pRetClass) << L"> to return\r\n" ;
					osCpp << L"	" << strRetValueName
							<< L"->SetNative( std::make_shared<"
								<< MakeCppClassName(pRetClass) << L">() ) ;\r\n\r\n" ;
				}
				osCpp << L"	LQT_RETURN_OBJECT( " << strRetValueName << L" ) ;\r\n" ;
			}
		}
		else
		{
			switch ( typeRet.GetPrimitive() )
			{
			case	LType::typeBoolean:
				osCpp << L"	LQT_RETURN_BOOL( " << strRetValueName << L" ) ;\r\n" ;
				break ;

			case	LType::typeInt8:
			case	LType::typeInt16:
			case	LType::typeInt32:
				osCpp << L"	LQT_RETURN_INT( " << strRetValueName << L" ) ;\r\n" ;
				break ;

			case	LType::typeUint8:
			case	LType::typeUint16:
			case	LType::typeUint32:
				osCpp << L"	LQT_RETURN_UINT( " << strRetValueName << L" ) ;\r\n" ;
				break ;

			default:
			case	LType::typeInt64:
				osCpp << L"	LQT_RETURN_LONG( " << strRetValueName << L" ) ;\r\n" ;
				break ;

			case	LType::typeUint64:
				osCpp << L"	LQT_RETURN_ULONG( " << strRetValueName << L" ) ;\r\n" ;
				break ;

			case	LType::typeFloat:
				osCpp << L"	LQT_RETURN_FLOAT( " << strRetValueName << L" ) ;\r\n" ;
				break ;

			case	LType::typeDouble:
				osCpp << L"	LQT_RETURN_DOUBLE( " << strRetValueName << L" ) ;\r\n" ;
				break ;
			}
		}
	}
	else
	{
		osCpp << L"	LQT_RETURN_VOID() ;\r\n" ;
	}
	osCpp << L"}\r\n\r\n" ;
	return	nExit ;
}

void LoquatyApp::OutputStubFuncArgList
	( LOutputStream& osCpp, LPtr<LFunctionObj> pFunc )
{
	std::shared_ptr<LPrototype>		pProto = pFunc->GetPrototype() ;
	assert( pProto != nullptr ) ;

	osCpp << L"(" ;

	const LNamedArgumentListType&	argList = pProto->GetArgListType() ;
	const std::vector<LValue>&		argDefaults = pProto->GetDefaultArgList() ;
	for ( size_t i = 0; i < argList.size(); i ++ )
	{
		if ( i > 0 )
		{
			osCpp << L"," ;
		}
		osCpp << L" " << argList.at(i).GetTypeName()
						<< L" " << argList.GetArgNameAt(i) ;
	}
	osCpp << L" )" ;

	if ( pProto->GetModifiers() & LType::modifierConst )
	{
		osCpp << L" const" ;
	}
}

// パッケージに含まれるクラスの native 関数の宣言・実装のテンプレートを出力する
int LoquatyApp::MakeNativeFuncStubClassInPackage( void )
{
	int	nExit = LoadSource() ;
	if ( nExit != 0 )
	{
		return	nExit ;
	}

	// パッケージ取得
	LPackagePtr	pPackage = nullptr ;
	for ( auto p : m_vm->GetPackageList() )
	{
		if ( p->GetName() == m_strMakeTarget )
		{
			pPackage = p ;
			break ;
		}
	}
	if ( pPackage == nullptr )
	{
		printf( "\n\'%s\' パッケージが見つかりません\n",
					m_strMakeTarget.ToString().c_str() ) ;
		return	1 ;
	}

	return	MakeNativeFuncStubClassInPackage( pPackage ) ;
}

int LoquatyApp::MakeNativeFuncStubClassInPackage( LPackagePtr pPackage )
{
	int	nExit = 0 ;
	for ( LClass * pClass : pPackage->GetClasses() )
	{
		if ( HasClassNativeFunction( pClass ) )
		{
			nExit += MakeNativeFuncStubClass( pClass ) ;
		}
	}
	return	nExit ;
}

bool LoquatyApp::HasClassNativeFunction( LClass * pClass )
{
	for ( auto iter : pClass->GetStaticFunctionList() )
	{
		for ( size_t i = 0; i < iter.second.size(); i ++ )
		{
			LPtr<LFunctionObj>	pFunc = iter.second.at(i) ;
			if ( (pFunc != nullptr)
				&& (pFunc->GetPrototype() != nullptr)
				&& (pFunc->GetPrototype()->GetModifiers() & LType::modifierNative) )
			{
				return	true ;
			}
		}
	}
	const LVirtualFuncVector&	vecVirtuals = pClass->GetVirtualVector() ;
	for ( size_t i = 0; i < vecVirtuals.size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc = vecVirtuals.GetFunctionAt( i ) ;
		if ( (pFunc != nullptr)
			&& (pFunc->GetThisClass() == pClass)
			&& (pFunc->GetPrototype() != nullptr)
			&& (pFunc->GetPrototype()->GetModifiers() & LType::modifierNative) )
		{
			return	true ;
		}
	}
	return	false ;
}

bool LoquatyApp::IsNativeClass( LClass * pClass )
{
	if ( pClass == nullptr )
	{
		return	false ;
	}
	if ( dynamic_cast<LNativeObjClass*>( pClass ) != nullptr )
	{
		return	true ;
	}
	return	IsNativeClass( pClass->GetSuperClass() ) ;
}

LString LoquatyApp::GetPrimitiveTypeName( LType::Primitive type )
{
	LString	strTypeName ;
	switch ( type )
	{
	case	LType::typeBoolean:
		strTypeName = L"LBoolean" ;
		break ;

	case	LType::typeInt8:
		strTypeName = L"LInt8" ;
		break ;

	case	LType::typeInt16:
		strTypeName = L"LInt16" ;
		break ;

	case	LType::typeInt32:
		strTypeName = L"LInt32" ;
		break ;

	case	LType::typeUint8:
		strTypeName = L"LUint8" ;
		break ;

	case	LType::typeUint16:
		strTypeName = L"LUint16" ;
		break ;

	case	LType::typeUint32:
		strTypeName = L"LUint32" ;
		break ;

	case	LType::typeInt64:
		strTypeName = L"LInt64" ;
		break ;

	case	LType::typeUint64:
		strTypeName = L"LUint64" ;
		break ;

	case	LType::typeFloat:
		strTypeName = L"LFloat" ;
		break ;

	case	LType::typeDouble:
		strTypeName = L"LDouble" ;
		break ;

	default:
		strTypeName = L"void" ;
		break ;
	}
	return	strTypeName ;
}

LString LoquatyApp::MakeStubFunctionName( const LString& strFuncName, int& nExit )
{
	if ( strFuncName.Find( L"_" ) < 0 )
	{
		return	strFuncName.Replace( L".", L"_" ) ;
	}
	if ( strFuncName.Find( L"__" ) >= 0 )
	{
		printf( "%s: 正常に処理できないクラス／関数名です。\n",
					strFuncName.ToString().c_str() ) ;
		nExit ++ ;
	}
	return	LString( L"_" ) + strFuncName.Replace( L".", L"__" ) ;
}

LString LoquatyApp::MakeCppClassName( LClass * pClass )
{
	if ( (dynamic_cast<LGenericObjClass*>( pClass ) == nullptr)
		&& (dynamic_cast<LStructureClass*>( pClass ) == nullptr) )
	{
		if ( dynamic_cast<LPointerClass*>( pClass ) != nullptr )
		{
			return	LString(L"LPointerObj") ;
		}
		if ( dynamic_cast<LIntegerClass*>( pClass ) != nullptr )
		{
			return	LString(L"LIntegerObj") ;
		}
		if ( dynamic_cast<LDoubleClass*>( pClass ) != nullptr )
		{
			return	LString(L"LDoubleObj") ;
		}
		if ( dynamic_cast<LStringClass*>( pClass ) != nullptr )
		{
			return	LString(L"LStringObj") ;
		}
		if ( dynamic_cast<LStringBufClass*>( pClass ) != nullptr )
		{
			return	LString(L"LStringBufObj") ;
		}
		if ( dynamic_cast<LArrayClass*>( pClass ) != nullptr )
		{
			return	LString(L"LArrayObj") ;
		}
		if ( dynamic_cast<LMapClass*>( pClass ) != nullptr )
		{
			return	LString(L"LMapObj") ;
		}
		if ( dynamic_cast<LFunctionClass*>( pClass ) != nullptr )
		{
			return	LString(L"LFunctionObj") ;
		}
		if ( dynamic_cast<LExceptionClass*>( pClass ) != nullptr )
		{
			return	LString(L"LExceptionObj") ;
		}
		if ( dynamic_cast<LTaskClass*>( pClass ) != nullptr )
		{
			return	LString(L"LTaskObj") ;
		}
		if ( dynamic_cast<LThreadClass*>( pClass ) != nullptr )
		{
			return	LString(L"LThreadObj") ;
		}
	}
	LPtr<LNamespace>	pParent = pClass->GetParentNamespace() ;
	if ( (pParent != nullptr) && !pParent->GetName().IsEmpty() )
	{
		return	MakeCppNamespaceName( pParent.Ptr() ) + L"_" + pClass->GetName() ;
	}
	else
	{
		return	LString(L"L") + pClass->GetName() ;
	}
}

LString LoquatyApp::MakeCppNamespaceName( LNamespace * pNamespace )
{
	LPtr<LNamespace>	pParent = pNamespace->GetParentNamespace() ;
	if ( (pParent != nullptr) && !pParent->GetName().IsEmpty() )
	{
		return	MakeCppNamespaceName( pParent.Ptr() ) + L"_" + pNamespace->GetName() ;
	}
	else
	{
		return	LString(L"L") + pNamespace->GetName() ;
	}
}



//////////////////////////////////////////////////////////////////////////////
// エラーを標準出力するコンパイラ
//////////////////////////////////////////////////////////////////////////////

// 文字列出力
void CLICompiler::PrintString( const LString& str )
{
	LCompiler::PrintString( str ) ;

	std::string	msg ;
	printf( "%s", str.ToString(msg).c_str() ) ;
}

