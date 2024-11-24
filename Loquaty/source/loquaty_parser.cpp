
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 文字列パーサー
/////////////////////////////////////////////////////////////////////////////

// 記号マスク
std::uint32_t	LStringParser::s_maskPunctuation[4] =
{
	0xFFFFFFFF,		// 制御文字
	0x7C00FFFF,		// 空白と !"#$%&'()*+,-./:;<=>
	0x78000000,		// [\]^
	0xF8000001,		// `{|}~ 
} ;

// 特殊記号マスク
std::uint32_t	LStringParser::s_maskSpecialMark[4] =
{
	0x00000000,		//
	0x58001384,		// "'(),;<>
	0x28000000,		// []
	0x28000000,		// {}
} ;

// 特殊記号から始まる２文字以上の演算子
const wchar_t *const	LStringParser::s_pwszSpecialOps[LStringParser::opSpecialCount] =
{
	L">>=",
	L"<<=",
	L">>",
	L"<<",
	L"<=",
	L">=",
} ;


// 文字列を変更した後に呼び出される
void LStringParser::OnModified( void )
{
	SetBounds( 0, SIZE_MAX ) ;
}

// 領域設定
void LStringParser::SetBounds( size_t iFirst, size_t iEnd )
{
	m_iBoundFirst = std::min( iFirst, LString::GetLength() ) ;
	m_iBoundEnd = std::min( iEnd, LString::GetLength() ) ;
	m_index = std::min( std::max( m_index, m_iBoundFirst ), m_iBoundEnd ) ;
}


// 空白文字を読み飛ばす（終端に到達した場合は false を返す）
bool LStringParser::PassSpace( void )
{
	while ( !IsEndOfString() )
	{
		if ( IsCharSpace( LString::at(m_index) ) )
		{
			m_index ++ ;
		}
		else
		{
			return	true ;
		}
	}
	return	false ;
}

// 空白を除いた次の文字が指定のいずれかの文字であるならそれを取得する
// 但し指標はその文字のまま進めない
wchar_t LStringParser::CheckNextChars( const wchar_t * pwszNext )
{
	if ( !PassSpace() )
	{
		return	0 ;
	}
	wchar_t	wch = CurrentChar() ;
	if ( IsCharContained( wch, pwszNext ) )
	{
		return	wch ;
	}
	return	0 ;
}

// 次の文字が指定のいずれかの文字であるならそれを取得し次へ進める
// 何れの文字でもないなら指標を進めず 0 を返す（但し空白は飛ばす）
wchar_t LStringParser::HasNextChars( const wchar_t * pwszNext )
{
	if ( !PassSpace() )
	{
		return	0 ;
	}
	wchar_t	wch = CurrentChar() ;
	if ( IsCharContained( wch, pwszNext ) )
	{
		m_index ++ ;
		return	wch ;
	}
	return	0 ;
}

// 現在の位置から次の文字列が一致するかテストし、一致なら次へ進め true を返す
// そうでないなら指標を進めず false を返す
bool LStringParser::HasNextString( const wchar_t * pwszNext )
{
	assert( pwszNext != nullptr ) ;
	const size_t	iStart = GetIndex() ;
	for ( int i = 0; pwszNext[i] != 0; i ++ )
	{
		if ( NextChar() != pwszNext[i] )
		{
			SeekIndex( iStart ) ;
			return	false ;
		}
	}
	return	true ;
}

// 指定文字列と一致する位置まで指標を進め true を返す
// そうでないなら末尾まで移動して false を返す
bool LStringParser::SeekString( const wchar_t * pwszNext )
{
	assert( pwszNext != nullptr ) ;
	if ( (pwszNext == nullptr) || (pwszNext[0] == 0) )
	{
		return	true ;
	}
	const wchar_t	wchFirst = pwszNext[0] ;
	while ( !IsEndOfString() )
	{
		if ( LString::at(m_index) == wchFirst )
		{
			if ( HasNextString( pwszNext ) )
			{
				return	true ;
			}
		}
		m_index ++ ;
	}
	return	false ;
}

// 現在の位置から次のトークンが一致するかテストし、一致なら次へ進め true を返す
// そうでないなら指標を進めず false を返す
bool LStringParser::HasNextToken( const wchar_t * pwszNext )
{
	const size_t	iStart = GetIndex() ;
	LString			strToken ;
	if ( NextToken( strToken ) != tokenNothing )
	{
		if ( strToken == pwszNext )
		{
			return	true ;
		}
	}
	SeekIndex( iStart ) ;
	return	false ;
}

// １トークンの終端位置まで読み飛ばす
LStringParser::TokenType LStringParser::PassToken( void )
{
	TokenType		tokenType = tokenNothing ;
	
	wchar_t	wch = CurrentChar() ;
	if ( IsPunctuation(wch) )
	{
		if ( IsSpecialMark(wch) )
		{
			// 特殊な演算子と一致しないか判定
			for ( int i = 0; i < opSpecialCount; i ++ )
			{
				const wchar_t *	pwszOp = s_pwszSpecialOps[i] ;
				if ( HasNextString( pwszOp ) )
				{
					// 特殊な演算子
					return	tokenMark ;
				}
			}
			// 1文字だけの記号
			m_index ++ ;
			return	tokenMark ;
		}
		else
		{
			// 記号文字列
			do
			{
				m_index ++ ;
				if ( IsEndOfString() )
				{
					break ;
				}
				wch = CurrentChar() ;
			}
			while ( !IsCharSpace(wch) && !IsSpecialMark(wch) && IsPunctuation(wch) ) ;
		}
		tokenType = tokenMark ;
	}
	else
	{
		// 記号以外の文字列
		tokenType = IsCharNumber(wch) ? tokenNumber : tokenSymbol ;
		do
		{
			m_index ++ ;
			if ( IsEndOfString() )
			{
				break ;
			}
			wch = CurrentChar() ;
		}
		while ( !IsCharSpace(wch) && !IsPunctuation(wch) ) ;
	}
	return	tokenType ;
}

// 次の１トークンを取得して指標を進める
LStringParser::TokenType LStringParser::NextToken( LString& strToken )
{
	if ( !PassSpace() )
	{
		strToken = L"" ;
		return	tokenNothing ;
	}
	const size_t	iStart = GetIndex() ;
	TokenType		tokenType = PassToken() ;
	strToken = LString::Middle( iStart, m_index - iStart ) ;
	return	tokenType ;
}

// 現在の位置から指定記号で区切られている区間の文字列を取得する
// 該当の文字を見つけるか終端に到達するとそこまでの文字列を取得し
// １文字読み進め、区切り文字コードを返す（終端の場合 0）
wchar_t LStringParser::NextStringTerm
	( LString& strTerm, std::function<bool(wchar_t)> funcIsDeli )
{
	const size_t	iStart = GetIndex() ;
	while ( !IsEndOfString() )
	{
		if ( funcIsDeli( CurrentChar() ) )
		{
			strTerm = LString::Middle( iStart, m_index - iStart ) ;
			return	NextChar() ;
		}
		m_index ++ ;
	}
	strTerm = LString::Middle( iStart, m_index - iStart ) ;
	return	0 ;
}

// 現在の位置から空白文字で区切られた文字列
wchar_t LStringParser::NextStringTermBySpace( LString& strTerm )
{
	return	NextStringTerm
		( strTerm, [](wchar_t wch){ return IsCharSpace( wch ) ; } ) ;
}

// 現在の位置から指定のいずれかの文字で区切られた文字列
wchar_t LStringParser::NextStringTermByChars
	( LString& strTerm, const wchar_t * pwszChars )
{
	return	NextStringTerm
		( strTerm, [pwszChars](wchar_t wch)
					{ return IsCharContained( wch, pwszChars ) ; } ) ;
}

// 現在の位置から行末までの文字列
//（strLine に改行は含まず、改行を読み飛ばす。\r\n の場合は２文字読み飛ばす）
wchar_t LStringParser::NextLine( LString& strLine )
{
	wchar_t	wchRet = NextStringTermByChars( strLine, L"\r\n" ) ;
	if ( (wchRet == L'\r') && (CurrentChar() == L'\n') )
	{
		return	NextChar() ;
	}
	return	wchRet ;
}

// 閉じ記号まで読み飛ばす
bool LStringParser::PassEnclosedString
		( wchar_t wchCloser, const wchar_t * pwszEscChars )
{
	while ( !IsEndOfString() )
	{
		wchar_t	wch = LString::at(m_index) ;
		if ( wch == wchCloser )
		{
			return	true ;
		}
		if ( pwszEscChars != nullptr )
		{
			if ( IsCharContained( wch, pwszEscChars ) )
			{
				return	false ;
			}
		}
		if ( (wch == L'\\')
			&& ((wchCloser == L'\'') || (wchCloser == L'\"')) )
		{
			m_index ++ ;
			if ( IsEndOfString() )
			{
				break ;
			}
		}
		m_index ++ ;
	}
	return	false ;
}

// 閉じ記号までの文字列を取得し、閉じ記号を読み飛ばし true を返す
// pwszEscChars 文字列に含まれるいずれかの文字を見つけた場合も
// その文字までの区間の文字列を strEnclosed に取得するが
// その文字は読み飛ばさず false を返す
bool LStringParser::NextEnclosedString
	( LString& strEnclosed, wchar_t wchCloser, const wchar_t * pwszEscChars )
{
	const size_t	iStart = GetIndex() ;
	const bool		flagEnclosed = PassEnclosedString( wchCloser, pwszEscChars ) ;
	strEnclosed = LString::Middle( iStart, m_index - iStart ) ;
	if ( flagEnclosed )
	{
		assert( CurrentChar() == wchCloser ) ;
		m_index ++ ;
	}
	return	flagEnclosed ;
}

// 文字列に文字が含まれるか？
bool LStringParser::IsCharContained( wchar_t wch, const wchar_t * pwszChars )
{
	if ( pwszChars != nullptr )
	{
		for ( int i = 0; pwszChars[i] != 0; i ++ )
		{
			if ( wch == pwszChars[i] )
			{
				return	true ;
			}
		}
	}
	return	false ;
}

// 文字指標に該当する行を探す
LStringParser::LineInfo
	LStringParser::FindLineContainingIndexAt( size_t index, wchar_t wchRet ) const
{
	LineInfo	lninf ;
	lninf.iLine = 0 ;
	lninf.iStart = 0 ;
	lninf.iEnd = 0 ;

	const size_t	nLength = LString::GetLength() ;
	size_t			iLine = 1 ;
	size_t			iStart = 0 ;
	for ( size_t i = 0; i < nLength; i ++ )
	{
		if ( i >= index )
		{
			// 該当行
			lninf.iLine = iLine ;
			lninf.iStart = iStart ;
			lninf.iEnd = nLength ;

			for ( size_t j = i; j < nLength; j ++ )
			{
				if ( LString::at(j) == wchRet )
				{
					// 行の末端
					lninf.iEnd = j + 1 ;
					break ;
				}
			}
			break ;
		}
		if ( LString::at(i) == wchRet )
		{
			// 改行
			iLine ++ ;
			iStart = i + 1 ;
		}
	}
	return	lninf ;
}

// シンボルとして有効な文字列か？
bool LStringParser::IsValidAsSymbol( const wchar_t * pwszStr )
{
	if ( (pwszStr == nullptr) || (pwszStr[0] == 0) )
	{
		return	false ;
	}
	if ( IsCharNumber( pwszStr[0] ) )
	{
		return	false ;
	}
	for ( int i = 0; pwszStr[i] != 0; i ++ )
	{
		if ( IsCharSpace( pwszStr[i] )
			|| IsPunctuation( pwszStr[i] )
			|| IsSpecialMark( pwszStr[i] ) )
		{
			return	false ;
		}
	}
	return	true ;
}

// 文字列リテラルとして特殊文字を \ 記号エンコードする
LString LStringParser::EncodeStringLiteral( const wchar_t * pwszStr, size_t nStrLen )
{
	if ( pwszStr == nullptr )
	{
		return	LString() ;
	}
	if ( nStrLen == 0 )
	{
		while ( pwszStr[nStrLen] != 0 )
		{
			nStrLen ++ ;
		}
	}
	LString		strDst ;
	size_t		iSrcLast = 0 ;
	size_t		iSrc = 0 ;
	while ( iSrc < nStrLen )
	{
		wchar_t	wch = pwszStr[iSrc ++] ;
		if ( (wch < 0x20) || (wch == L'\"') || (wch == L'\'') || (wch == L'\\') )
		{
			strDst += std::wstring( pwszStr + iSrcLast, iSrc - 1 - iSrcLast ) ;
			//
			const wchar_t *	pwchEscCode = L"\a\b\t\n\v\f\r\?\\\"\'\0" ;
			const wchar_t *	pwchEscChar = L"abtnvfr?\\\"\'0" ;
			size_t	i ;
			for ( i = 0; pwchEscCode[i]; i ++ )
			{
				if ( pwchEscCode[i] == wch )
				{
					wchar_t	buf[] =
					{
						L'\\', pwchEscChar[i], 0,
					} ;
					strDst += buf ;
					break ;
				}
			}
			if ( pwchEscCode[i] == 0 )
			{
				if ( wch == 0 )
				{
					strDst += L"\\0" ;
				}
				else
				{
					strDst += L"\\x" ;
					//
					wchar_t	hex[3] ;
					hex[0] = (wch >> 4) & 0x0F ;
					hex[1] = wch & 0x0F ;
					hex[2] = 0 ;
					//
					for ( i = 0; i < 2; i ++ )
					{
						if ( hex[i] < 10 )
						{
							hex[i] += L'0' ;
						}
						else
						{
							hex[i] += L'A' - 10 ;
						}
					}
					strDst += hex ;
				}
			}
			iSrcLast = iSrc ;
		}
	}
	if ( iSrcLast < iSrc )
	{
		strDst += std::wstring( pwszStr + iSrcLast, iSrc - iSrcLast ) ;
	}
	return	strDst ;
}

// 文字列リテラルとして \ 記号エスケープをデコードする
LString LStringParser::DecodeStringLiteral( const wchar_t * pwszStr, size_t nStrLen )
{
	if ( pwszStr == nullptr )
	{
		return	LString() ;
	}
	if ( nStrLen == 0 )
	{
		while ( pwszStr[nStrLen] != 0 )
		{
			nStrLen ++ ;
		}
	}
	LString		strDst ;
	size_t		iSrcLast = 0 ;
	size_t		iSrc = 0 ;
	while ( iSrc < nStrLen )
	{
		wchar_t	wch = pwszStr[iSrc ++] ;
		if ( wch == L'\\' )
		{
			strDst += std::wstring( pwszStr + iSrcLast, iSrc - 1 - iSrcLast ) ;
			//
			wch = pwszStr[iSrc ++] ;
			if ( (wch == L'x') || (wch == L'X') )
			{
				wchar_t	c = 0 ;
				while ( iSrc < nStrLen )
				{
					wch = pwszStr[iSrc] ;
					if ( (wch >= L'0') && (wch <= L'9') )
					{
						c = (c << 4) | (wch - L'0') ;
					}
					else if ( (wch >= L'A') && (wch <= L'F') )
					{
						c = (c << 4) | (wch - L'A' + 10) ;
					}
					else if ( (wch >= L'a') && (wch <= L'f') )
					{
						c = (c << 4) | (wch - L'a' + 10) ;
					}
					else
					{
						break ;
					}
					iSrc ++ ;
				}
				strDst.push_back( c ) ;
			}
			else if ( (wch >= L'0') && (wch < L'8') )
			{
				wchar_t	c = 0 ;
				for ( ; ; )
				{
					c = (c << 3) | (wch - L'0') ;
					wch = pwszStr[iSrc] ;
					if ( (wch == 0) || (wch < L'0') || (wch >= L'8') )
					{
						break ;
					}
					iSrc ++ ;
				}
				strDst.push_back( c ) ;
			}
			else
			{
				const wchar_t *	pwchEscChar = L"abtnvfr?\\\"\'" ;
				const wchar_t *	pwchEscCode = L"\a\b\t\n\v\f\r\?\\\"\'" ;
				size_t	i ;
				for ( i = 0; pwchEscChar[i]; i ++ )
				{
					if ( pwchEscChar[i] == wch )
					{
						strDst.push_back( pwchEscCode[i] ) ;
						break ;
					}
				}
				if ( pwchEscChar[i] == 0 )
				{
					// 処理できないのでそのまま
					strDst.push_back( L'\\' ) ;
					strDst.push_back( wch ) ;
				}
			}
			iSrcLast = iSrc ;
		}
	}
	if ( iSrcLast < iSrc )
	{
		strDst += std::wstring( pwszStr + iSrcLast, iSrc - iSrcLast ) ;
	}
	return	strDst ;
}

// 構文を指定の文字を見つけるまで読み飛ばす
wchar_t LStringParser::PassStatement
	( const wchar_t * pwszClosers, const wchar_t * pwszEscChars )
{
	for ( ; ; )
	{
		if ( !PassSpace() )
		{
			return	0 ;
		}

		// 適合文字判定
		wchar_t	wch = CurrentChar() ;
		if ( IsCharContained( wch, pwszClosers ) )
		{
			m_index ++ ;
			return	wch ;
		}

		// 脱出文字判定
		if ( IsCharContained( wch, pwszEscChars ) )
		{
			return	0 ;
		}

		if ( (wch == L'\"') || (wch == L'\'') )
		{
			m_index ++ ;
			if ( PassEnclosedString( wch, L"\r\n" ) )
			{
				assert( CurrentChar() == wch ) ;
				m_index ++ ;
			}
		}
		else if ( wch == L'(' )
		{
			m_index ++ ;
			PassStatement( L")", L"}" ) ;
		}
		else if ( wch == L'[' )
		{
			m_index ++ ;
			PassStatement( L"]", L"}" ) ;
		}
		else if ( wch == L'{' )
		{
			m_index ++ ;
			PassStatement( L"}", L"" ) ;
		}
		else
		{
			PassToken() ;
		}
	}
}

// 文の末尾まで読み飛ばす
// セミコロンを見つけたらその文字を
// 途中で '{' 括弧を見つけたら '}' 閉じ括弧までを読み飛ばしその文字を返す
wchar_t LStringParser::PassStatementBlock( void )
{
	wchar_t	wch = PassStatement( L";{" ) ;
	if ( wch == L'{' )
	{
		wch = PassStatement( L"}" ) ;
	}
	return	wch ;
}

// 現在の位置から型を解釈（型表現の終端まで移動する）
// ['const']opt ObjectType ['[]'...]
// ['const']opt 'Array' '<' ObjectType '>' ['[]'...]
// ['const']opt 'Map' '<' ObjectType '>' ['[]'...]
// ['const']opt 'Function' '<' <proto> '>' ['[]'...]
// ['const']opt 'Task' '<' ReturnType '>' ['[]'...]
// ['const']opt 'Thread' '<' ReturnType '>' ['[]'...]
// ['const']opt DataType ['['<dim-size>']'...]opt [*]opt
bool LStringParser::NextTypeExpr
	( LType& type, bool mustHave,
		LVirtualMachine& vm,
		const LNamespaceList * pnslNamespace,
		LType::Modifiers accModAdd,
		bool arrayModifiable, bool instantiateGenType )
{
	const size_t		iStart = GetIndex() ;

	// const 修飾
	LType::Modifiers	accMod = accModAdd ;
	if ( HasNextToken( L"const" ) )
	{
		accMod |= LType::modifierConst ;
	}

	// 型名解釈
	LString	strTypeName ;
	if ( NextToken( strTypeName ) != tokenSymbol )
	{
		if ( mustHave )
		{
			LCompiler::Error( errorNothingTypeExpression ) ;
			type = LType() ;
		}
		SeekIndex( iStart ) ;
		return	false ;
	}
	const LType::Primitive
		primitive = LType::AsPrimitive( strTypeName.c_str() ) ;
	if ( LType::IsPrimitiveType( primitive ) )
	{
		type = LType( primitive, accMod ) ;
	}
	else if ( strTypeName == L"void" )
	{
		type = LType( nullptr, accMod ) ;
		if ( HasNextChars( L"*" ) == L'*' )
		{
			type = LType( vm.GetPointerClassAs( type ) ) ;
		}
		return	true ;
	}
	else
	{
		if ( !ParseTypeName
			( type, mustHave, strTypeName.c_str(),
				vm, pnslNamespace, accMod, instantiateGenType ) )
		{
			if ( mustHave )
			{
				LCompiler::Error
					( errorUndefinedClassName, strTypeName.c_str() ) ;
			}
			SeekIndex( iStart ) ;
			return	false ;
		}
	}

	if ( type.CanArrangeOnBuf() )
	{
		// ポインタ型修飾
		if ( HasNextChars( L"*" ) == L'*' )
		{
			type = LType( vm.GetPointerClassAs( type ) ) ;
		}
	}
	if ( type.CanArrangeOnBuf() )
	{
		// プリミティブ / 構造体の配列修飾
		if ( arrayModifiable )
		{
			std::vector<size_t>	dimArray ;
			while ( HasNextChars( L"[" ) == L'[' )
			{
				LLong	nDim =
					LCompiler::EvaluateConstExprAsLong
						( *this, vm, pnslNamespace, L"]" ) ;
				if ( nDim <= 0 )
				{
					if ( mustHave )
					{
						LCompiler::Error
							( errorInvalidDimensionSize,
								nullptr, LString::IntegerOf(nDim).c_str() ) ;
						SeekIndex( iStart ) ;
						return	false ;
					}
					nDim = 1 ;
				}
				else if ( nDim >= 0x10000000 )
				{
					if ( mustHave )
					{
						LCompiler::Warning
							( errorTooHugeDimensionSize,
								LCompiler::warning2,
								nullptr, LString::IntegerOf(nDim).c_str() ) ;
					}
				}
				dimArray.push_back( (size_t) nDim ) ;
				//
				if ( HasNextChars( L"]" ) != L']' )
				{
					if ( mustHave )
					{
						LCompiler::Error( errorMismatchBrackets ) ;
					}
					SeekIndex( iStart ) ;
					return	false ;
				}
			}
			if ( dimArray.size() > 0 )
			{
				type = LType( vm.GetDataArrayClassAs( type, dimArray ) ) ;
			}
		}
		// ポインタ型修飾
		if ( HasNextChars( L"*" ) == L'*' )
		{
			type = LType( vm.GetPointerClassAs( type ) ) ;
		}
		return	true ;
	}
	else if ( type.IsArray() || type.IsMap() )
	{
		// Array<type> 修飾
		// Map<type> 修飾
		if ( HasNextChars( L"<" ) == L'<' )
		{
			LType	typeElement ;
			if ( NextTypeExpr( typeElement, mustHave, vm, pnslNamespace, 0, true, false ) )
			{
				if ( typeElement.CanArrangeOnBuf() )
				{
					if ( mustHave )
					{
						if ( type.IsArray() )
						{
							LCompiler::Error
								( errorInvalidArrayElementType,
									nullptr, typeElement.GetTypeName().c_str() ) ;
						}
						else
						{
							LCompiler::Error
								( errorInvalidMapElementType,
									nullptr, typeElement.GetTypeName().c_str() ) ;
						}
					}
					SeekIndex( iStart ) ;
					return	false ;
				}
				else
				{
					if ( typeElement.IsConst() )
					{
						if ( mustHave )
						{
							LCompiler::Warning
								( errorCannotConstForArrayElement,
									LCompiler::warning1,
									nullptr, typeElement.GetTypeName().c_str() ) ;
						}
						typeElement.SetModifiers( 0 ) ;
					}
					if ( type.IsArray() )
					{
						type = LType( vm.GetArrayClassAs( typeElement.GetClass() ) ) ;
					}
					else
					{
						type = LType( vm.GetMapClassAs( typeElement.GetClass() ) ) ;
					}
				}
				if ( HasNextChars( L">" ) != L'>' )
				{
					if ( mustHave )
					{
						LCompiler::Error( errorMismatchAngleBrackets ) ;
					}
					SeekIndex( iStart ) ;
					return	false ;
				}
			}
			else
			{
				SeekIndex( iStart ) ;
				return	false ;
			}
		}
	}
	else if ( type.IsFunction() )
	{
		// Function<type(arg-list)class> 修飾
		if ( HasNextChars( L"<" ) == L'<' )
		{
			LPrototype	proto ;
			if ( NextPrototype( proto, mustHave, vm, pnslNamespace, false ) )
			{
				type = LType( vm.GetFunctionClassAs
								( std::make_shared<LPrototype>( proto ) ) ) ;
				if ( HasNextChars( L">" ) != L'>' )
				{
					if ( mustHave )
					{
						LCompiler::Error( errorMismatchAngleBrackets ) ;
					}
					SeekIndex( iStart ) ;
					return	false ;
				}
			}
			else if ( PassStatement( L">", L"})]\"\';\n\r" ) != L'>' )
			{
				if ( mustHave )
				{
					LCompiler::Error( errorMismatchAngleBrackets ) ;
				}
				SeekIndex( iStart ) ;
				return	false ;
			}
		}
	}
	else if ( type.IsTask() || type.IsThread() )
	{
		// Task<type> 修飾
		// Thread<type> 修飾
		if ( HasNextChars( L"<" ) == L'<' )
		{
			LType	typeRet ;
			if ( NextTypeExpr( typeRet, mustHave, vm, pnslNamespace, 0, true, false ) )
			{
				if ( type.IsTask() )
				{
					type = LType( vm.GetTaskClassAs( typeRet ) ) ;
				}
				else
				{
					type = LType( vm.GetThreadClassAs( typeRet ) ) ;
				}
				if ( HasNextChars( L">" ) != L'>' )
				{
					if ( mustHave )
					{
						LCompiler::Error( errorMismatchAngleBrackets ) ;
					}
					SeekIndex( iStart ) ;
					return	false ;
				}
			}
			else
			{
				SeekIndex( iStart ) ;
				return	false ;
			}
		}
	}

	// オブジェクトの配列修飾
	if ( arrayModifiable )
	{
		while ( HasNextChars( L"[" ) == L'[' )
		{
			if ( type.CanArrangeOnBuf()
				|| (type.GetClass() == nullptr) )
			{
				if ( mustHave )
				{
					LCompiler::Error
						( errorInvalidArrayElementType,
							nullptr, type.GetTypeName().c_str() ) ;
				}
				SeekIndex( iStart ) ;
				return	false ;
			}
			else
			{
				type = LType( vm.GetArrayClassAs( type.GetClass() ), type.GetModifiers() ) ;
			}
			if ( HasNextChars( L"]" ) != L']' )
			{
				if ( mustHave )
				{
					LCompiler::Error( errorMismatchBrackets ) ;
				}
				SeekIndex( iStart ) ;
				return	false ;
			}
		}
	}
	return	true ;
}

// 型名を解釈（一致する名前の型、又は名前の後ろに引数があるジェネリック型）
bool LStringParser::ParseTypeName
	( LType& type, bool mustHave,
		const wchar_t * pwszName,
		LVirtualMachine& vm,
		const LNamespaceList * pnslNamespace,
		LType::Modifiers accMod, bool instantiateGenType )
{
	// 名前空間／クラス名
	LPtr<LNamespace>	pNamespace ;
	LClass *	pClass = nullptr ;
	if ( pnslNamespace != nullptr )
	{
		pNamespace = pnslNamespace->GetNamespaceAs( pwszName ) ;
	}
	if ( pNamespace == nullptr )
	{
		pNamespace = vm.Global()->GetLocalNamespaceAs( pwszName ) ;
	}
	LString	strName = pwszName ;
	while ( (pNamespace != nullptr)
		&& (HasNextToken( L"." ) || HasNextToken( L"::" )) )
	{
		NextToken( strName ) ;
		pwszName = strName.c_str() ;

		pNamespace = pNamespace->GetLocalNamespaceAs( pwszName ) ;
	}
	if ( pNamespace != nullptr )
	{
		pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
		if ( pClass != nullptr )
		{
			type = LType( pClass, accMod ) ;
			return	true ;
		}
	}

	// 定義された型
	const LType *	pType = nullptr ;
	if ( pNamespace != nullptr )
	{
		pType = pNamespace->GetTypeAs( pwszName ) ;
	}
	else if ( pnslNamespace != nullptr )
	{
		pType = pnslNamespace->GetTypeAs( pwszName ) ;
	}
	if ( pType == nullptr )
	{
		pType = vm.Global()->GetTypeAs( pwszName ) ;
	}
	if ( pType != nullptr )
	{
		type = *pType ;
		type.SetModifiers( type.GetModifiers() | accMod ) ;
		return	true ;
	}

	// ジェネリック型
	const LNamespace::GenericDef *	pGenType = nullptr ;
	if ( pNamespace != nullptr )
	{
		pGenType = pNamespace->GetGenericTypeAs( pwszName ) ;
	}
	else if ( pnslNamespace != nullptr )
	{
		pGenType = pnslNamespace->GetGenericTypeAs( pwszName ) ;
	}
	if ( pGenType == nullptr )
	{
		pGenType = vm.Global()->GetGenericTypeAs( pwszName ) ;
	}
	if ( pGenType == nullptr )
	{
		return	false ;
	}

	// ジェネリック型引数解釈
	LString				strGenTypeName ;
	std::vector<LType>	vecTypes ;

	if ( !ParseGenericArgList
		( vecTypes, strGenTypeName, mustHave, pwszName, vm, pnslNamespace ) )
	{
		return	false ;
	}

	// インスタンス化済みか？
	LPtr<LNamespace>	pInstance ;
	pInstance = pGenType->m_parent->GetClassAs( strGenTypeName.c_str() ) ;
	if ( pInstance != nullptr )
	{
		pClass = dynamic_cast<LClass*>( pInstance.Ptr() ) ;
		if ( pClass != nullptr )
		{
			type = LType( pClass, accMod ) ;
			return	true ;
		}
	}
	if ( !instantiateGenType )
	{
		if ( mustHave )
		{
			LCompiler::Error( errorCannotInstantiateGenType ) ;
		}
		return	false ;
	}

	// インスタンス化
	LCompiler *			pCompiler = LCompiler::GetCurrent() ;
	if ( pCompiler != nullptr )
	{
		pInstance = pGenType->Instantiate( *pCompiler, vecTypes ) ;
	}
	else
	{
		LVirtualMachine&	vm = pGenType->m_parent->GetClass()->VM() ;
		LCompiler			compiler( vm, this ) ;
		LCompiler::Current	current( compiler ) ;

		pInstance = pGenType->Instantiate( compiler, vecTypes ) ;
	}
	if ( pInstance != nullptr )
	{
		pClass = dynamic_cast<LClass*>( pInstance.Ptr() ) ;
		if ( pClass != nullptr )
		{
			type = LType( pClass, accMod ) ;
			return	true ;
		}
	}
	return	false ;
}

// ジェネリック型の引数を解釈
bool LStringParser::ParseGenericArgList
	( std::vector<LType>& vecTypes,
		LString& strGenTypeName,
		bool mustHave, const wchar_t * pwszName,
		LVirtualMachine& vm, const LNamespaceList * pnslNamespace )
{
	// ジェネリック型引数解釈
	if ( HasNextChars( L"<" ) != L'<' )
	{
		if ( mustHave )
		{
			LCompiler::Error( errorNoGenericTypeArgment ) ;
		}
		return	false ;
	}
	strGenTypeName = pwszName ;
	strGenTypeName += L"<" ;

	for ( ; ; )
	{
		LType	type ;
		if ( !NextTypeExpr( type, mustHave, vm, pnslNamespace, 0, true, false ) )
		{
			return	false ;
		}
		vecTypes.push_back( type ) ;

		strGenTypeName += type.GetTypeName() ;

		if ( HasNextChars( L"," ) != L',' )
		{
			if ( HasNextChars( L">" ) != L'>' )
			{
				if ( mustHave )
				{
					LCompiler::Error( errorMismatchAngleBrackets ) ;
				}
				return	false ;
			}
			break ;
		}
		strGenTypeName += L"," ;
	}
	strGenTypeName += L">" ;

	return	true ;
}

// 関数プロトタイプ型表記を解釈
// <ret-type> '(' <arg-type-list> ')' ['['<capture-type-list>']'] [<this-type>]opt
bool LStringParser::NextPrototype
	( LPrototype& proto, bool mustHave,
		LVirtualMachine& vm,
		const LNamespaceList * pnslNamespace, bool instantiateGenType )
{
	bool	flagSuccess = NextTypeExpr
					( proto.ReturnType(), mustHave,
						vm, pnslNamespace, 0, true, instantiateGenType ) ;

	if ( HasNextChars( L"(" ) != L'(' )
	{
		if ( flagSuccess
			|| (PassStatement( L"(", L"})>;\n\r" ) != L'(') )
		{
			if ( mustHave )
			{
				LCompiler::Error
					( errorNothingArgmentList, nullptr, L" \'(\' " ) ;
			}
			return	false ;
		}
	}
	if ( HasNextChars( L")" ) != L')' )
	{
		// 引数型リスト
		for ( ; ; )
		{
			LType	typeArg ;
			if ( !NextTypeExpr
				( typeArg, mustHave, vm, pnslNamespace, 0, true, instantiateGenType ) )
			{
				if ( PassStatement( L")", L"}>;\n\r" ) != L')' )
				{
					return	false ;
				}
				break ;
			}
			proto.ArgListType().push_back( typeArg ) ;
			//
			wchar_t	wch = HasNextChars( L",)" ) ;
			if ( wch == L')' )
			{
				break ;
			}
			if ( wch != L',' )
			{
				if ( mustHave )
				{
					LCompiler::Error( errorNoSeparationArgmentList ) ;
				}
				if ( PassStatement( L")", L"}>;\n\r" ) != L')' )
				{
					return	false ;
				}
				break ;
			}
		}
	}

	const size_t	iEndOfArgList = GetIndex() ;

	// キャプチャーリスト
	if ( HasNextChars( L"[" ) == L'[' )
	{
		if ( HasNextChars( L"]" ) != L']' )
		{
			for ( ; ; )
			{
				LType	typeObj ;
				if ( !NextTypeExpr
					( typeObj, mustHave, vm, pnslNamespace, 0, true, instantiateGenType ) )
				{
					if ( PassStatement( L"]", L"}>;\n\r" ) != L']' )
					{
						return	false ;
					}
					break ;
				}
				proto.AddCaptureObjectType( L"", typeObj ) ;
				//
				wchar_t	wch = HasNextChars( L",]" ) ;
				if ( wch == L']' )
				{
					break ;
				}
				if ( wch != L',' )
				{
					if ( mustHave )
					{
						LCompiler::Error( errorNoSeparationArgmentList ) ;
					}
					if ( PassStatement( L"]", L"}>;\n\r" ) != L']' )
					{
						return	false ;
					}
					break ;
				}
			}
		}
	}

	// this クラス
	LClass *	pThisClass = nullptr ;
	LString		strThisClass ;
	bool		constThis = HasNextToken( L"const" ) ;
	TokenType	tokenType = NextToken( strThisClass ) ;
	if ( tokenType == tokenSymbol )
	{
		if ( pnslNamespace != nullptr )
		{
			pThisClass = pnslNamespace->GetClassAs( strThisClass.c_str() ) ;
		}
		if ( pThisClass == nullptr )
		{
			pThisClass = vm.GetGlobalClassAs( strThisClass.c_str() ) ;
		}
	}
	if ( pThisClass != nullptr )
	{
		proto.SetThisClass( pThisClass, constThis ) ;
	}
	else
	{
		SeekIndex( iEndOfArgList ) ;
	}
	return	true ;
}

// この文字列を読み込んだ元のファイル名を取得する
LString LStringParser::GetFileName( void ) const
{
	return	LString() ;
}

// 開いたディレクトリを取得する
LDirectory * LStringParser::GetFileDirectory( void ) const
{
	return	nullptr ;
}



//////////////////////////////////////////////////////////////////////////////
// StringParser クラス用 native 実装宣言
//////////////////////////////////////////////////////////////////////////////

DECL_LOQUATY_CONSTRUCTOR(StringParser);
DECL_LOQUATY_CONSTRUCTOR_N(StringParser,1);
DECL_LOQUATY_FUNC(StringParser_operator_smov);
DECL_LOQUATY_FUNC(StringParser_operator_smov_1);
DECL_LOQUATY_FUNC(StringParser_toString);
DECL_LOQUATY_FUNC(StringParser_loadTextFile);
DECL_LOQUATY_FUNC(StringParser_readTextFile);
DECL_LOQUATY_FUNC(StringParser_getIndex);
DECL_LOQUATY_FUNC(StringParser_seekIndex);
DECL_LOQUATY_FUNC(StringParser_skipIndex);
DECL_LOQUATY_FUNC(StringParser_isEndOfString);
DECL_LOQUATY_FUNC(StringParser_currentChar);
DECL_LOQUATY_FUNC(StringParser_currentAt);
DECL_LOQUATY_FUNC(StringParser_nextChar);
DECL_LOQUATY_FUNC(StringParser_passSpace);
DECL_LOQUATY_FUNC(StringParser_checkNextChars);
DECL_LOQUATY_FUNC(StringParser_hasNextChars);
DECL_LOQUATY_FUNC(StringParser_hasNextString);
DECL_LOQUATY_FUNC(StringParser_seekString);
DECL_LOQUATY_FUNC(StringParser_hasNextToken);
DECL_LOQUATY_FUNC(StringParser_passToken);
DECL_LOQUATY_FUNC(StringParser_nextToken);
DECL_LOQUATY_FUNC(StringParser_nextStringTermBySpace);
DECL_LOQUATY_FUNC(StringParser_nextStringTermByChars);
DECL_LOQUATY_FUNC(StringParser_nextLine);
DECL_LOQUATY_FUNC(StringParser_passEnclosedString);
DECL_LOQUATY_FUNC(StringParser_nextEnclosedString);
DECL_LOQUATY_FUNC(StringParser_isCharSpace);
DECL_LOQUATY_FUNC(StringParser_findLineContainingIndexAt);
DECL_LOQUATY_FUNC(StringParser_encodeStringLiteral);
DECL_LOQUATY_FUNC(StringParser_decodeStringLiteral);



//////////////////////////////////////////////////////////////////////////////
// StringParser クラス用 native 実装
//////////////////////////////////////////////////////////////////////////////

// StringParser()
IMPL_LOQUATY_CONSTRUCTOR(StringParser)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LStringParser, pThis, () ) ;

	LQT_RETURN_VOID() ;
}

// StringParser( String str ) ;
IMPL_LOQUATY_CONSTRUCTOR_N(StringParser,1)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LStringParser, pThis, () ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	*pThis = str ;

	LQT_RETURN_VOID() ;
}

// StringParser operator := ( StringParser spars ) ;
IMPL_LOQUATY_FUNC(StringParser_operator_smov)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LStringParser, spars ) ;
	LQT_VERIFY_NULL_PTR( spars ) ;

	*pThis = *spars ;

	LQT_RETURN_OBJECT( LQT_ARG_OBJECT(0) ) ;
}

// StringParser operator := ( String str ) ;
IMPL_LOQUATY_FUNC(StringParser_operator_smov_1)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	*pThis = str ;

	LQT_RETURN_OBJECT( LQT_ARG_OBJECT(0) ) ;
}

// String toString() const ;
IMPL_LOQUATY_FUNC(StringParser_toString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_STRING( *pThis ) ;
}

// boolean loadTextFile( String path ) ;
IMPL_LOQUATY_FUNC(StringParser_loadTextFile)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL( LTextFileParser::LoadTextFile( *pThis, path.c_str() ) ) ;
}

// boolean readTextFile( File file ) ;
IMPL_LOQUATY_FUNC(StringParser_readTextFile)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LPureFile, pFile ) ;
	LQT_VERIFY_NULL_PTR( pFile ) ;

	LQT_RETURN_BOOL( LTextFileParser::ReadTextFile( *pThis, pFile ) ) ;
}

// ulong getIndex() const ;
IMPL_LOQUATY_FUNC(StringParser_getIndex)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_LONG( pThis->GetIndex() ) ;
}

// void seekIndex( ulong index ) ;
IMPL_LOQUATY_FUNC(StringParser_seekIndex)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_ULONG( index ) ;

	pThis->SeekIndex( (size_t) index ) ;

	LQT_RETURN_VOID() ;
}

// ulong skipIndex( long offset ) ;
IMPL_LOQUATY_FUNC(StringParser_skipIndex)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_LONG( offset ) ;

	LQT_RETURN_LONG( (LLong) pThis->SkipIndex( (ssize_t) offset ) ) ;
}

// boolean isEndOfString() const ;
IMPL_LOQUATY_FUNC(StringParser_isEndOfString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_BOOL( pThis->IsEndOfString() ) ;
}

// uint currentChar() const ;
IMPL_LOQUATY_FUNC(StringParser_currentChar)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_UINT( pThis->CurrentChar() ) ;
}

// uint currentAt( uint index ) const ;
IMPL_LOQUATY_FUNC(StringParser_currentAt)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_UINT( index ) ;

	LQT_RETURN_UINT( pThis->CurrentAt( index ) ) ;
}

// uint nextChar() ;
IMPL_LOQUATY_FUNC(StringParser_nextChar)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_UINT( pThis->NextChar() ) ;
}

// boolean passSpace() ;
IMPL_LOQUATY_FUNC(StringParser_passSpace)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_BOOL( pThis->PassSpace() ) ;
}

// uint checkNextChars( String strChars ) ;
IMPL_LOQUATY_FUNC(StringParser_checkNextChars)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( strChars ) ;

	LQT_RETURN_UINT( pThis->CheckNextChars( strChars.c_str() ) ) ;
}

// uint hasNextChars( String strChars ) ;
IMPL_LOQUATY_FUNC(StringParser_hasNextChars)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( strChars ) ;

	LQT_RETURN_UINT( pThis->HasNextChars( strChars.c_str() ) ) ;
}

// boolean hasNextString( String strNext ) ;
IMPL_LOQUATY_FUNC(StringParser_hasNextString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( strNext ) ;

	LQT_RETURN_BOOL( pThis->HasNextString( strNext.c_str() ) ) ;
}

// boolean seekString( String strSeek ) ;
IMPL_LOQUATY_FUNC(StringParser_seekString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( strSeek ) ;

	LQT_RETURN_BOOL( pThis->SeekString( strSeek.c_str() ) ) ;
}

// boolean hasNextToken( String strToken ) ;
IMPL_LOQUATY_FUNC(StringParser_hasNextToken)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( strToken ) ;

	LQT_RETURN_BOOL( pThis->HasNextToken( strToken.c_str() ) ) ;
}

// int passToken() ;
IMPL_LOQUATY_FUNC(StringParser_passToken)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LQT_RETURN_INT( pThis->PassToken() ) ;
}

// String nextToken( int* pTokenType = null ) ;
IMPL_LOQUATY_FUNC(StringParser_nextToken)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_POINTER( LInt, pTokenType ) ;

	LString	strToken ;
	LStringParser::TokenType
			type = pThis->NextToken( strToken ) ;

	if ( pTokenType != nullptr )
	{
		*pTokenType = (LInt) type ;
	}
	LQT_RETURN_STRING( strToken ) ;
}

// String nextStringTermBySpace() ;
IMPL_LOQUATY_FUNC(StringParser_nextStringTermBySpace)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;

	LString	strTerm ;
	pThis->NextStringTermBySpace( strTerm ) ;

	LQT_RETURN_STRING( strTerm ) ;
}

// String nextStringTermByChars
//		( String strDeliChars, uint* pDeliChar = null ) ;
IMPL_LOQUATY_FUNC(StringParser_nextStringTermByChars)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_STRING( strDeliChars ) ;
	LQT_FUNC_ARG_POINTER( LUint, pDeliChar ) ;

	LString	strTerm ;
	wchar_t	wchDeli =
		pThis->NextStringTermByChars( strTerm, strDeliChars.c_str() ) ;

	if ( pDeliChar != nullptr )
	{
		*pDeliChar = (LUint) wchDeli ;
	}
	LQT_RETURN_STRING( strTerm ) ;
}

// String nextLine( uint* pRetCode = null ) ;
IMPL_LOQUATY_FUNC(StringParser_nextLine)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_POINTER( LUint, pRetCode ) ;

	LString	strLine ;
	wchar_t	wchRet = pThis->NextLine( strLine ) ;

	if ( pRetCode != nullptr )
	{
		*pRetCode = (LUint) wchRet ;
	}
	LQT_RETURN_STRING( strLine ) ;
}

// boolean passEnclosedString
//		( uint chCloser, String strEscChars = null ) ;
IMPL_LOQUATY_FUNC(StringParser_passEnclosedString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_UINT( chCloser ) ;
	LQT_FUNC_ARG_STRING( strEscChars ) ;

	LQT_RETURN_BOOL( pThis->PassEnclosedString
					( (wchar_t) chCloser, strEscChars.c_str() ) ) ;
}

// String nextEnclosedString
//		( uint chCloser, String strEscChars = null, boolean* pFoundCloser = null ) ;
IMPL_LOQUATY_FUNC(StringParser_nextEnclosedString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_UINT( chCloser ) ;
	LQT_FUNC_ARG_STRING( strEscChars ) ;
	LQT_FUNC_ARG_POINTER( LBoolean, pFoundCloser ) ;

	LString	strEnclosed ;
	bool	fFoundCloser =
				pThis->NextEnclosedString
					( strEnclosed, (wchar_t) chCloser, strEscChars.c_str() ) ;

	if ( pFoundCloser != nullptr )
	{
		*pFoundCloser = fFoundCloser ;
	}
	LQT_RETURN_STRING( strEnclosed ) ;
}

// boolean isCharSpace( uint ch ) ;
IMPL_LOQUATY_FUNC(StringParser_isCharSpace)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_UINT( ch ) ;

	LQT_RETURN_BOOL( LStringParser::IsCharSpace( (wchar_t) ch ) ) ;
}

// LineInfo* findLineContainingIndexAt( ulong index, uint chRet = '\n' ) const ;
IMPL_LOQUATY_FUNC(StringParser_findLineContainingIndexAt)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LStringParser, pThis ) ;
	LQT_FUNC_ARG_ULONG( index ) ;
	LQT_FUNC_ARG_UINT( chRet ) ;

	LStringParser::LineInfo	lninf =
		pThis->FindLineContainingIndexAt( (size_t) index, (wchar_t) chRet ) ;

	LQT_RETURN_POINTER_STRUCT( lninf ) ;
}

// String encodeStringLiteral( String str ) ;
IMPL_LOQUATY_FUNC(StringParser_encodeStringLiteral)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( str ) ;

	LQT_RETURN_STRING
		( LStringParser::EncodeStringLiteral( str.c_str(), str.GetLength() ) ) ;
}

// String decodeStringLiteral( String str ) ;
IMPL_LOQUATY_FUNC(StringParser_decodeStringLiteral)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( str ) ;

	LQT_RETURN_STRING
		( LStringParser::DecodeStringLiteral( str.c_str(), str.GetLength() ) ) ;
}


