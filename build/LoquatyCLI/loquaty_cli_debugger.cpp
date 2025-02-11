
#include <loquaty.h>
#include <iostream>
#include "loquaty_cli_debugger.h"

using namespace Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Loquaty CLI Debugger
//////////////////////////////////////////////////////////////////////////////

// デバッグ用出力
void LCLIDebugger::OutputTrace( const wchar_t * pwszMessage )
{
	std::string	strMessage = LString(pwszMessage).ToString() ;
	std::cout << strMessage ;
	LTrace( strMessage.c_str() ) ;
}

// 例外発生時
LObjPtr LCLIDebugger::OnThrowException
	( LContext& context, LObjPtr pException )
{
	LString	strType, strMessage ;
	if ( pException != nullptr )
	{
		strType = pException->GetClass()->GetFullClassName() ;
		pException->AsString( strMessage ) ;
	}
	OutputTrace
		( (LString(L"thrown exception: ")
			+ strType + L" \"" + strMessage + L"\"\n\n").c_str() ) ;

	TraceCurrentCode( context ) ;

	DoDialogue( context ) ;

	return	pException ;
}

// ブレークポイント
void LCLIDebugger::OnBreakPoint( LContext& context, BreakReason reason )
{
	if ( reason == reasonBreakPoint )
	{
		OutputTrace( L"stopped at breakpoint.\n\n" ) ;
	}
	TraceCurrentCode( context ) ;

	DoDialogue( context ) ;
}

// 現在の停止位置のコードを表示する
void LCLIDebugger::TraceCurrentCode( LContext& context )
{
	const LCodeBuffer *	pCodeBuf = context.GetCodeBuffer() ;
	if ( (pCodeBuf == nullptr)
		|| (context.GetIP() == LCodeBuffer::InvalidCodePos) )
	{
		return ;
	}
	LSourceFilePtr	pSourceFile = pCodeBuf->GetSourceFile() ;
	size_t			iCodeFirst = (size_t) std::max( context.GetIP() - 1, (ssize_t) 0 ) ;
	size_t			iCodeEnd = (size_t) context.GetIP() + 2 ;
	const LCodeBuffer::DebugSourceInfo *
		pdsi = pCodeBuf->FindDebugSourceInfo( (size_t) context.GetIP() ) ;
	if ( (pSourceFile != nullptr) && (pdsi != nullptr)
		&& (pdsi->m_iCodeFirst <= (size_t) context.GetIP())
		&& ((size_t) context.GetIP() < pdsi->m_iCodeEnd) )
	{
		// ソース位置表示
		LStringParser::LineInfo	linf =
			pSourceFile->FindLineContainingIndexAt( pdsi->m_iSrcFirst )  ;
		LString	strSourceFile = pSourceFile->GetFileName() ;
		if ( !strSourceFile.IsEmpty() && (linf.iLine >= 1) )
		{
			OutputTrace
				( (LString(L"\'")
					+ strSourceFile + L"\' ("
					+ LString::IntegerOf(linf.iLine) + L")\n").c_str() ) ;
		}

		// ソースコード表示
		LString	strSourceLine =
			pSourceFile->Middle
				( pdsi->m_iSrcFirst, pdsi->m_iSrcEnd - pdsi->m_iSrcFirst ) ;
		strSourceLine.TrimRight() ;
		strSourceLine += L"\n" ;
		OutputTrace( strSourceLine.c_str() ) ;

		// コード範囲
		iCodeFirst = pdsi->m_iCodeFirst ;
		iCodeEnd = pdsi->m_iCodeEnd ;
	}

	// コード逆アセンブリ表示
	const std::vector<LCodeBuffer::Word>&	bufCodes = pCodeBuf->GetBuffer() ;
	for ( size_t i = iCodeFirst; (i < iCodeEnd) && (i < bufCodes.size()); i ++ )
	{
		LString	strMnemonic = ((size_t) context.GetIP() == i) ? L">" : L"#" ;
		strMnemonic += LString::IntegerOf( i ) ;
		strMnemonic += L": " ;
		strMnemonic += pCodeBuf->MnemonicOf( bufCodes.at(i), i ) ;
		strMnemonic += L"\n" ;
		OutputTrace( strMnemonic.c_str() ) ;
	}
}

// コマンドライン対話
void LCLIDebugger::DoDialogue( LContext& context )
{
	for ( ; ; )
	{
		OutputTrace( L"\n> " ) ;

		// １行読み取る
		std::string	cmd ;
		for ( ; ; )
		{
			char	buf[0x100] ;
			std::cin.getline( buf, 0x100 ) ;
			cmd += buf ;

			if ( std::cin.fail() )
			{
				std::cin.clear
					( std::cin.rdstate() & ~std::ios_base::failbit ) ;
			}
			else
			{
				break ;
			}
		}

		// コマンドとして処理する
		LStringParser	spars = LString( cmd ) ;
		if ( DoCommandLine( context, spars ) )
		{
			break ;
		}
	}
}

// コマンド処理
bool LCLIDebugger::DoCommandLine( LContext& context, LStringParser& cmdline )
{
	LString	strCmd ;
	cmdline.NextToken( strCmd ) ;
	if ( strCmd == L"g" )
	{
		// go
		ClearStepTrace( context ) ;
		return	true ;
	}
	else if ( strCmd == L"t" )
	{
		// trace (step in)
		SetStepIn( context ) ;
		return	true ;
	}
	else if ( strCmd == L"r" )
	{
		// step in
		DoCommandStepIn( context ) ;
		return	true ;
	}
	else if ( strCmd == L"s" )
	{
		// step over
		DoCommandStepOver( context ) ;
		return	true ;
	}
	else if ( strCmd == L"cl" )
	{
		// code list
		DoCommandCodeList( context ) ;
	}
	else if ( strCmd == L"vl" )
	{
		// local variable list
		DoCommandVariableList( context ) ;
	}
	else if ( strCmd == L"vx" )
	{
		// variable expression
		DoCommandExpression( context, cmdline ) ;
	}
	else if ( strCmd == L"bp" )
	{
		// break point
		DoCommandBreakPoint( context, cmdline ) ;
	}
	else if ( (strCmd == L"?") || (strCmd == L"help") )
	{
		// help
		DoCommandHelp() ;
	}
	else
	{
		// error
		OutputTrace( L"構文エラー\nhelp コマンドで構文を確認してください。\n" ) ;
	}
	return	false ;
}

// ステップイン設定
void LCLIDebugger::DoCommandStepIn( LContext& context )
{
	const LCodeBuffer *	pCodeBuf = context.GetCodeBuffer() ;
	if ( pCodeBuf != nullptr )
	{
		size_t	ipCurStepFirst = (size_t) context.GetIP() ;
		size_t	ipCurStepEnd = ipCurStepFirst ;

		const LCodeBuffer::DebugSourceInfo *
			pdsiCur = pCodeBuf->FindDebugSourceInfo( (size_t) context.GetIP() ) ;
		if ( pdsiCur != nullptr )
		{
			ipCurStepFirst = pdsiCur->m_iCodeFirst ;
			ipCurStepEnd = pdsiCur->m_iCodeEnd - 1 ;
		}

		SetStepIn( context, ipCurStepFirst, ipCurStepEnd ) ;
	}
	else
	{
		SetStepIn( context ) ;
	}
}

// ステップオーバー設定
void LCLIDebugger::DoCommandStepOver( LContext& context )
{
	const LCodeBuffer *	pCodeBuf = context.GetCodeBuffer() ;
	if ( pCodeBuf != nullptr )
	{
		size_t	ipCurStepFirst = (size_t) context.GetIP() ;
		size_t	ipCurStepEnd = ipCurStepFirst ;
		size_t	ipNextStepFirst = ipCurStepEnd + 1 ;
		size_t	ipNextStepEnd = ipNextStepFirst ;

		const LCodeBuffer::DebugSourceInfo *
			pdsiCur = pCodeBuf->FindDebugSourceInfo( (size_t) context.GetIP() ) ;
		if ( pdsiCur != nullptr )
		{
			ipCurStepFirst = pdsiCur->m_iCodeFirst ;
			ipCurStepEnd = pdsiCur->m_iCodeEnd - 1 ;
			ipNextStepFirst = pdsiCur->m_iCodeEnd ;
			ipNextStepEnd = ipNextStepFirst ;

			const LCodeBuffer::DebugSourceInfo *
				pdsiNext = pCodeBuf->FindDebugSourceInfo( pdsiCur->m_iCodeEnd ) ;
			if ( pdsiNext != nullptr )
			{
				ipNextStepFirst = pdsiNext->m_iCodeFirst ;
				ipNextStepEnd = pdsiNext->m_iCodeEnd ;
			}
		}

		SetStepOver
			( context,
				ipCurStepFirst, ipCurStepEnd,
				ipNextStepFirst, ipNextStepEnd ) ;
	}
	else
	{
		SetStepIn( context ) ;
	}
}

// 現在の関数のコードリスト表示
void LCLIDebugger::DoCommandCodeList( LContext& context )
{
	const LCodeBuffer *	pCodeBuf = context.GetCodeBuffer() ;
	if ( pCodeBuf == nullptr )
	{
		return ;
	}

	// 関数名
	LFunctionObj *	pFunc = pCodeBuf->GetFunction() ;
	if ( pFunc != nullptr )
	{
		LString	strFuncProto = pFunc->GetFullPrintName() ;
		strFuncProto += L":" ;

		if ( pFunc->GetPrototype() != nullptr )
		{
			// プロトタイプ
			strFuncProto += pFunc->GetPrototype()->TypeToString() ;
		}
		strFuncProto += L"\n" ;
		OutputTrace( strFuncProto.c_str() ) ;
	}

	// コード
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
			pdbSrcInf = pCodeBuf->FindDebugSourceInfo( i ) ;
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
				OutputTrace( L"\n" ) ;

				LString	strSource =
							pSourceFile->Middle
								( iSrcFirst, pdbSrcInf->m_iSrcEnd - iSrcFirst ) ;
				strSource.TrimRight() ;
				strSource += L"\n\n" ;

				if ( !strFileName.IsEmpty() )
				{
					OutputTrace
						( (strFileName + L" ("
							+ LString::IntegerOf
								( pSourceFile->
									FindLineContainingIndexAt( iSrcFirst ).iLine )
							+ L"):\n").c_str() ) ;
				}
				OutputTrace( strSource.c_str() ) ;
			}
		}
		LString	strMnemonic = L"#" ;
		strMnemonic += LString::IntegerOf( i ) ;
		strMnemonic += L": " ;
		strMnemonic += pCodeBuf->MnemonicOf( bufCodes.at(i), i ) ;
		strMnemonic += L"\n" ;
		OutputTrace( strMnemonic.c_str() ) ;
	}
}

// 現在の関数の位置でのローカル変数リスト表示
void LCLIDebugger::DoCommandVariableList( LContext& context )
{
	const LCodeBuffer *	pCodeBuf = context.GetCodeBuffer() ;
	if ( pCodeBuf == nullptr )
	{
		return ;
	}
	std::vector<LCodeBuffer::DebugLocalVarInfo>	dbgVarInfos ;
	pCodeBuf->GetDebugLocalVarInfos( dbgVarInfos, context.GetIP() ) ;

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
			LValue	valLoc = GetLocalVar( context, pVar, iLoc ) ;
			if ( !valLoc.IsVoid() )
			{
				LString	strName = iter->first ;
				OutputTrace
					( (strName + L": "
						+ pVar->GetType().GetTypeName() + L": ").c_str() ) ;
				//
				LString	strExpr = ToExpression( valLoc ) ;
				strExpr += L"\n" ;
				OutputTrace( strExpr.c_str() ) ;
			}
		}
	}
}

// 式の値を表示
void LCLIDebugger::DoCommandExpression( LContext& context, LStringParser& cmdline )
{
	LValue	value = EvaluateExpr( context, cmdline ) ;
	if ( value.IsVoid() )
	{
		OutputTrace( L"error\n" ) ;
	}
	else
	{
		LString	strExpr = ToExpression( value ) ;
		strExpr += L"\n" ;
		OutputTrace( strExpr.c_str() ) ;
	}
}

// ブレークポイント操作
void LCLIDebugger::DoCommandBreakPoint( LContext& context, LStringParser& cmdline )
{
	if ( !cmdline.PassSpace() )
	{
		// ブレークポイント一覧表示
		if ( m_vBreakPoints.size() == 0 )
		{
			OutputTrace( L"\nno break points.\n" ) ;
		}
		else
		{
			OutputTrace( L"\nbreak point list;\n" ) ;
			for ( size_t i = 0; i < m_vBreakPoints.size(); i ++ )
			{
				LString	strBreakPoint = L"[" ;
				strBreakPoint += LString::IntegerOf( i ) ;
				strBreakPoint += L"]: " ;

				BreakPointPtr	bpp = m_vBreakPoints.at(i) ;
				if ( bpp->m_pCodeBuf != nullptr )
				{
					LSourceFilePtr	pSource = bpp->m_pCodeBuf->GetSourceFile() ;
					if ( pSource != nullptr )
					{
						strBreakPoint += L"\'" ;
						strBreakPoint += pSource->GetFileName() ;
						strBreakPoint += L"\' " ;

						const LCodeBuffer::DebugSourceInfo *
							pdsi = bpp->m_pCodeBuf->FindDebugSourceInfo( bpp->m_ipFirst ) ;
						if ( pdsi != nullptr )
						{
							strBreakPoint += L"(" ;
							strBreakPoint +=
								LString::IntegerOf
									( pSource->FindLineContainingIndexAt
													( pdsi->m_iSrcFirst ).iLine ) ;
							strBreakPoint += L")" ;
						}
					}
				}
				strBreakPoint += L"\n" ;
				OutputTrace( strBreakPoint.c_str() ) ;
			}
		}
	}
	else if ( cmdline.HasNextString( L"-d" ) )
	{
		// ブレークポイント削除
		// bp -d <num>
		LString	strNum ;
		if ( cmdline.NextToken( strNum ) != LStringParser::tokenNumber )
		{
			OutputTrace( L"Syntax error!\n" ) ;
			return ;
		}
		size_t	index = (size_t) strNum.AsInteger() ;
		if ( index >= m_vBreakPoints.size() )
		{
			OutputTrace( L"Out of bounds\n" ) ;
		}
		else
		{
			m_vBreakPoints.erase( m_vBreakPoints.begin() + index ) ;
			OutputTrace( L"Ok.\n" ) ;
		}
	}
	else
	{
		// ブレークポイント追加
		// bp <source>,<num>[,<cond-expr>]
		LString	strSourceFile ;
		LString	strLineNum ;
		LString	strCondExpr ;
		if ( cmdline.NextStringTermByChars( strSourceFile, L"," ) != L',' )
		{
			OutputTrace( L"Syntax error!\n" ) ;
			return ;
		}
		if ( cmdline.NextStringTermByChars( strLineNum, L"," ) == L',' )
		{
			if ( cmdline.PassSpace() )
			{
				strCondExpr =
					cmdline.Middle
						( cmdline.GetIndex(),
							cmdline.GetLength() - cmdline.GetIndex() ) ;
			}
		}
		strSourceFile.TrimRight() ;

		// ソース取得
		LSourceFilePtr	pSource =
			context.VM().SourceProducer().GetSourceFile( strSourceFile.c_str() ) ;
		if ( pSource == nullptr )
		{
			OutputTrace( L"Not found source\n" ) ;
			return ;
		}

		// 行番号 → 文字指標
		LLong	nLineNum = strLineNum.AsInteger() ;
		LLong	nCurLineNum = 1 ;
		size_t	iLineIndex = 0 ;
		for ( size_t i = 0; (i < pSource->GetLength())
								&& (nCurLineNum < nLineNum); i ++ )
		{
			if ( pSource->GetAt(i) == L'\n' )
			{
				nCurLineNum ++ ;
				iLineIndex = i + 1 ;
			}
		}
		while ( iLineIndex < pSource->GetLength() )
		{
			if ( !LStringParser::IsCharSpace( pSource->GetAt(iLineIndex) ) )
			{
				break ;
			}
			iLineIndex ++ ;
		}

		// コード情報取得
		const LSourceFile::DebugCodeInfo *
				pdci = pSource->GetDebugCodeInfoAt( iLineIndex ) ;
		if ( pdci == nullptr )
		{
			OutputTrace( L"No code info at the line\n" ) ;
			return ;
		}
		assert( pdci->pCodeBuf != nullptr ) ;

		const LCodeBuffer::DebugSourceInfo *
				pdsi = pdci->pCodeBuf->FindDebugSourceInfoAtSource( iLineIndex ) ;
		if ( pdsi == nullptr )
		{
			OutputTrace( L"No source info at the line\n" ) ;
			return ;
		}
		AddBreakPoint
			( std::make_shared<BreakPoint>
				( pdci->pCodeBuf,
					pdsi->m_iCodeFirst,
					pdsi->m_iCodeFirst, strCondExpr.c_str() ) ) ;
		OutputTrace( L"Ok.\n" ) ;
	}
}

// コマンド一覧表示
void LCLIDebugger::DoCommandHelp( void )
{
	const wchar_t *	pwszHelp =
		L"command list;\n"
		L"g         : 実行を継続\n"
		L"t         : １命令実行\n"
		L"r         : １行実行（step in）\n"
		L"s         : １行実行（step over）\n"
		L"cl        : 実行中の関数のコードリストを表示\n"
		L"vl        : ローカル変数の一覧表示\n"
		L"vx <expr> : <expr> の値を表示\n"
		L"bp        : ブレークポイントを一覧表示\n"
		L"bp <source>,<line>[,<expr>]\n"
		L"          : ソースファイルの指定行にブレークポイントを設定\n"
		L"bp -d <num>\n"
		L"          : 指定番号（一覧で表示される）のブレークポイントを削除\n"
		L"?         : コマンド一覧を表示\n"
		L"help      : コマンド一覧を表示\n" ;

	OutputTrace( pwszHelp ) ;
}


