
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 文字列
//////////////////////////////////////////////////////////////////////////////

LString::LString( const std::string& str )
{
	FromString( str ) ;
}

LString::LString( const char * pszStr )
{
	if ( pszStr != nullptr )
	{
		FromString( std::string( pszStr ) ) ;
	}
}

LString::LString( const char * pszStr, size_t nLength )
{
	if ( (pszStr != nullptr) && (nLength > 0) )
	{
		FromString( std::string( pszStr, nLength ) ) ;
	}
}

// 部分文字列抽出（範囲を超えている場合にはクリップ）
LString LString::Middle( size_t iStart, size_t nCount ) const
{
	if ( iStart >= std::wstring::length() )
	{
		return	LString() ;
	}
	if ( iStart + nCount >= std::wstring::length() )
	{
		nCount = std::wstring::length() - iStart ;
	}
	return	LString( std::wstring::c_str() + iStart, nCount ) ;
}

LString LString::Left( size_t nCount ) const
{
	if ( nCount > std::wstring::length() )
	{
		nCount = std::wstring::length() ;
	}
	return	LString( std::wstring::c_str(), nCount ) ;
}

LString LString::Right( size_t nCount ) const
{
	if ( nCount > std::wstring::length() )
	{
		nCount = std::wstring::length() ;
	}
	return	LString( std::wstring::c_str()
						+ (std::wstring::length() - nCount), nCount ) ;
}

// 部分文字列検索
ssize_t LString::Find( const wchar_t * pwszStr, size_t iFrom ) const
{
	if ( (pwszStr == nullptr) || (pwszStr[0] == 0) )
	{
		return	-1 ;
	}
	std::wstring::size_type
		pos = std::wstring::find
				( pwszStr, std::min( iFrom, std::wstring::length() ) ) ;
	if ( pos == std::wstring::npos )
	{
		return	-1 ;
	}
	return	(ssize_t) pos ;
}

// 文字列置き換え
LString LString::Replace( const wchar_t * pwszOld, const wchar_t * pwszNew ) const
{
	assert( pwszOld != nullptr ) ;
	if ( pwszOld == nullptr )
	{
		return	*this ;
	}
	LStringParser	sparsStr = *this ;
	LString			strReplaced ;
	LString			strOld = pwszOld ;
	size_t			iLast = 0 ;
	while ( sparsStr.SeekString( pwszOld ) )
	{
		strReplaced += sparsStr.Middle
			( iLast, sparsStr.GetIndex() - strOld.GetLength() - iLast ) ;
		strReplaced += pwszNew ;
		iLast = sparsStr.GetIndex() ;
	}
	strReplaced += sparsStr.Middle( iLast, sparsStr.GetIndex() - iLast ) ;
	return	strReplaced ;
}

LString LString::Replace
	( std::function<bool(LStringParser&)> willReplace,
		std::function<LString(const LString&)> getReplaced ) const
{
	LStringParser	sparsStr = *this ;
	LString			strReplaced ;
	size_t			iLast = 0 ;
	while ( !sparsStr.IsEndOfString() )
	{
		size_t	iPos = sparsStr.GetIndex() ;
		if ( willReplace( sparsStr ) )
		{
			strReplaced += sparsStr.Middle( iLast, iPos - iLast ) ;
			strReplaced += getReplaced( sparsStr.Middle( iPos, sparsStr.GetIndex() - iPos ) ) ;
			iLast = sparsStr.GetIndex() ;
		}
		else
		{
			sparsStr.NextChar() ;
		}
	}
	strReplaced += sparsStr.Middle( iLast, sparsStr.GetIndex() - iLast ) ;
	return	strReplaced ;
}

// 末端を指定文字数だけ除去する
const LString& LString::ChopRight( size_t nCount )
{
	const size_t	nLength = std::wstring::length() ;
	std::wstring::resize( nLength - std::min( nLength, nCount ) ) ;
	OnModified() ;
	return	*this ;
}

// 末端の空白文字（タブや改行等の制御文字含む）を除去する
const LString& LString::TrimRight( void )
{
	const size_t	nLength = std::wstring::length() ;
	for ( size_t i = 0; i < nLength; i ++ )
	{
		if ( std::wstring::at( nLength - i - 1 ) > L' ' )
		{
			std::wstring::resize( nLength - i ) ;
			OnModified() ;
			return	*this ;
		}
	}
	std::wstring::resize( 0 ) ;
	OnModified() ;
	return	*this ;
}

LString LString::GetTrimmedRight( void ) const
{
	LString	str = *this ;
	str.TrimRight() ;
	return	str ;
}

// 先頭の空白文字（タブや改行等の制御文字含む）を除去する
LString LString::GetTrimmedLeft( void ) const
{
	const size_t	nLength = std::wstring::length() ;
	for ( size_t i = 0; i < nLength; i ++ )
	{
		if ( std::wstring::at(i) > L' ' )
		{
			return	Middle( i, nLength - i ) ;
		}
	}
	return	LString() ;
}

// 先頭と末端の空白文字（タブや改行等の制御文字含む）を除去する
LString LString::GetTrimmed( void ) const
{
	return	GetTrimmedLeft().TrimRight() ;
}

// 小文字アルファベットを大文字に置き換える
const LString& LString::MakeUpper( void )
{
	const size_t	nLength = std::wstring::length() ;
	for ( size_t i = 0; i < nLength; i ++ )
	{
		wchar_t	wch = std::wstring::at(i) ;
		if ( (wch >= L'a') && (wch <= L'z') )
		{
			std::wstring::at(i) = wch - (L'a' - L'A') ;
		}
	}
	return	*this ;
}

LString LString::ToUpper( void ) const
{
	LString	str = *this ;
	return	str.MakeUpper() ;
}

// 大文字アルファベットを小文字に置き換える
const LString& LString::MakeLower( void )
{
	const size_t	nLength = std::wstring::length() ;
	for ( size_t i = 0; i < nLength; i ++ )
	{
		wchar_t	wch = std::wstring::at(i) ;
		if ( (wch >= L'A') && (wch <= L'Z') )
		{
			std::wstring::at(i) = wch + (L'a' - L'A') ;
		}
	}
	return	*this ;
}

LString LString::ToLower( void ) const
{
	LString	str = *this ;
	return	str.MakeLower() ;
}

// 文字順序反転
LString LString::Reverse( void ) const
{
	std::vector<std::uint32_t>	utf32 ;
	ToUTF32( utf32 ) ;

	const size_t	nLength = utf32.size() ;
	std::uint32_t *	pUTF32 = utf32.data() ;
	if ( nLength > 0 )
	{
		for ( size_t i = 0; i < nLength - 1 - i; i ++ )
		{
			const size_t	j = nLength - 1 - i ;
			std::uint32_t	temp = pUTF32[i] ;
			pUTF32[i] = pUTF32[j] ;
			pUTF32[j] = temp ;
		}
	}
	LString	strReversed ;
	strReversed.FromUTF32( utf32 ) ;
	return	strReversed ;
}

// 乗算（繰り返しと反転）
LString LString::operator * ( int nCount ) const
{
	if ( nCount == 0 )
	{
		return	LString() ;
	}
	LString	strUnit ;
	if ( nCount >= 0 )
	{
		strUnit = *this ;
	}
	else
	{
		strUnit = Reverse() ;
		nCount = - nCount ;
	}
	LString	strMulti ;
	int		mask = 1 ;
	for ( ; ; )
	{
		if ( nCount & mask )
		{
			strMulti += strUnit ;
		}
		nCount &= ~mask ;
		if ( nCount == 0 )
		{
			break ;
		}
		mask <<= 1 ;
		strUnit += strUnit ;
	}
	return	strMulti ;
}

// 分割
void LString::Slice( std::vector<LString>& vSliced, const wchar_t * pwszDelimiter ) const
{
	if ( (pwszDelimiter == nullptr) || (pwszDelimiter[0] == 0) )
	{
		vSliced.push_back( *this ) ;
		return ;
	}
	LStringParser	sparsStr = *this ;
	LString			strDelimiter = pwszDelimiter ;
	size_t			iLast = 0 ;
	while ( sparsStr.SeekString( pwszDelimiter ) )
	{
		vSliced.push_back
			( sparsStr.Middle
				( iLast, sparsStr.GetIndex() - strDelimiter.GetLength() - iLast ) ) ;
		iLast = sparsStr.GetIndex() ;
	}
	if ( iLast < sparsStr.GetIndex() )
	{
		vSliced.push_back( sparsStr.Middle( iLast, sparsStr.GetIndex() - iLast ) ) ;
	}
}

// 文字列フォーマット
// ※文字列中の %(var) 形式を func(var) の返り値に置き換える
LString LString::Format
	( const wchar_t * pwszForm,
		std::function<LString(const LString&)> func )
{
	LStringParser	sparsForm = pwszForm ;
	LString			strReplaced ;
	size_t			iLast = 0 ;
	while ( sparsForm.SeekString( L"%(" ) )
	{
		strReplaced += sparsForm.Middle( iLast, sparsForm.GetIndex() - 2 - iLast ) ;
		iLast = sparsForm.GetIndex() ;

		const size_t	iVarPos = sparsForm.GetIndex() ;
		if ( !sparsForm.SeekString( L")" ) )
		{
			break ;
		}
		strReplaced += func( sparsForm.Middle
								( iVarPos, sparsForm.GetIndex() - 1 - iVarPos ) ) ;
		iLast = sparsForm.GetIndex() ;
	}
	strReplaced += sparsForm.Middle( iLast, sparsForm.GetIndex() - iLast ) ;
	return	strReplaced ;
}

// 文字列フォーマット
// ※文字列中の %(index) 形式を listArg[index] に置き換える
LString LString::Format
	( const wchar_t * pwszForm, std::vector<LString>& listArg )
{
	return	Format( pwszForm, [&listArg](const LString& str)
				{
					LLong	index = str.AsInteger() ;
					if ( (index < 0) || ((size_t) index >= listArg.size()) )
					{
						return	LString() ;
					}
					return	listArg.at( (size_t) index ) ;
				} ) ;
}

// 文字列フォーマット
// ※文字列中の %(name) 形式を mapArg[name] に置き換える
LString LString::Format
	( const wchar_t * pwszForm, std::map<LString,LString>& mapArg )
{
	return	Format( pwszForm, [&mapArg](const LString& str)
				{
					auto	iter = mapArg.find( str ) ;
					if ( iter == mapArg.end() )
					{
						return	LString() ;
					}
					return	iter->second ;
				} ) ;
}

// 整数値を文字列にフォーマット
const LString& LString::SetIntegerOf( LLong val, int prec, int radix )
{
	wchar_t			buf[0x80] ;
	int				iDst = 0, col = 1 ;
	std::int64_t	v = val ;
	std::int64_t	r = radix ;
	if ( v < 0 )
	{
		buf[iDst ++] = '-' ;
		v = - v ;
	}
	if ( prec <= 0 )
	{
		std::int64_t	t = r ;			// 桁あふれ判定用
		while ( (v >= r) && (v >= t) )
		{
			t = r ;
			r *= radix ;
			col ++ ;
		}
	}
	else
	{
		if ( prec > 80 )
		{
			prec = 80 ;
		}
		col = prec - iDst ;
	}
	const int	len = iDst + col ;
	buf[len] = 0 ;
	//
	while ( col -- )
	{
		int	n = (int) (v % radix) ;
		if ( n < 10 )
		{
			buf[iDst + col] = (wchar_t) (L'0' + n) ;
		}
		else
		{
			buf[iDst + col] = (wchar_t) (L'A' + n - 10) ;
		}
		v = (v - n) / radix ;
	}
	assert( len < sizeof(buf)/sizeof(buf[0]) ) ;
	*this = buf ;
	return	*this ;
}

LString LString::IntegerOf( LLong val, int prec, int radix )
{
	LString	str ;
	str.SetIntegerOf( val, prec, radix ) ;
	return	str ;
}

// 文字列を整数値として解釈
LLong LString::AsInteger( bool * pHasNumber, int radix ) const
{
	bool	flagNumber = false ;
	bool	flagSign = false ;
	bool	flagNegative = false ;
	LLong	val = 0 ;
	for ( size_t i = 0; i < std::wstring::length(); i ++ )
	{
		wchar_t	wch = std::wstring::at(i) ;
		if ( (wch >= L'0') && (wch <= L'9')
				&& ((int)(wch - L'0') < radix) )
		{
			flagNumber = true ;
			val = val * radix + (wch - L'0') ;
		}
		else if ( (wch >= L'A') && (wch <= L'F')
				&& ((int)(wch - L'A' + 10) < radix) )
		{
			flagNumber = true ;
			val = val * radix + (wch - L'A' + 10) ;
		}
		else if ( (wch >= L'a') && (wch <= L'f')
				&& ((int)(wch - L'a' + 10) < radix) )
		{
			flagNumber = true ;
			val = val * radix + (wch - L'a' + 10) ;
		}
		else if ( !flagNumber && !flagSign )
		{
			if ( wch == L'-' )
			{
				flagSign = true ;
				flagNegative = true ;
			}
			else if ( wch == L'+' )
			{
				flagSign = true ;
				flagNegative = false ;
			}
			else if ( wch > L' ' )
			{
				break ;
			}
		}
		else if ( (wch > L' ') || flagNumber )
		{
			break ;
		}
	}
	if ( pHasNumber != nullptr )
	{
		*pHasNumber = flagNumber ;
	}
	return	flagNegative ? -val : val ;
}

// 浮動小数点値を文字列にフォーマット
// ※数値が大きい、又は小さい場合には指数表記（?.????E+??）を行う
const LString& LString::SetNumberOf( LDouble val, int prec, bool withExp )
{
	wchar_t	buf[0x80] ;
	size_t	iDst = 0 ;
	//
	// 整数部フォーマット
	//
	if ( val < 0 )
	{
		buf[iDst ++] = '-' ;
		val = - val ;
	}
	const int	xprec = prec ;
	int			expNum = 0 ;		// 指数
	if ( prec <= 0 )
	{
		prec = 9 ;			// デフォルトの有効桁
	}
	else if ( prec > 16 )
	{
		prec = 16 ;
	}
	if ( (val != 0.0) && withExp )
	{
		double	e = floor( log10( val ) ) ;
		if ( (e > (xprec/2)) || (e < -(xprec/4)) )
		{
			expNum = (int) e ;
			val *= pow( 10.0, - expNum ) ;
			prec = std::max( prec, 9 ) ;
		}
	}
	std::int64_t	d = 1 ;		// d <= pow( 10, prec )
	for ( int i = 0; i < prec; i ++ )
	{
		d *= 10 ;
	}
	std::int64_t	nValue = ((std::int64_t)(val * d + 0.5)) / d ;
	std::int64_t	nIntPart = nValue ;
	if ( nValue < 0 )
	{
		*this = L"nan" ;
		return	*this ;
	}
	int				col = 1 ;	// 整数部桁数
	std::int64_t	r = 10 ;
	while ( nValue >= r )
	{
		r *= 10 ;
		if ( ++ col >= 19 )
		{
			break ;
		}
	}
	const size_t	iEndOfInt = iDst + col ;
	while ( col -- )
	{
		int	n = (int) (nValue % 10) ;
		buf[iDst + col] = (wchar_t) (L'0' + n) ;
		nValue = (nValue - n) / 10 ;
	}
	iDst = iEndOfInt ;
	//
	// 小数部フォーマット
	//
	buf[iDst ++] = '.' ;
	if ( prec > 0 )
	{
		nValue = (std::int64_t)((val - nIntPart) * d + 0.5) ;
		for ( int i = 0; i < prec; i ++ )
		{
			if ( nValue <= 0 )
			{
				break ;
			}
			d /= 10 ;
			int	n = (int) (nValue / d) ;
			buf[iDst ++] = (wchar_t) (L'0' + n) ;
			nValue -= n * d ;
		}
	}
	//
	// 指数部フォーマット
	//
	if ( expNum != 0 )
	{
		buf[iDst ++] = 'E' ;
		if ( expNum >= 0 )
		{
			buf[iDst ++] = '+' ;
		}
		else
		{
			buf[iDst ++] = '-' ;
			expNum = - expNum ;
		}
		col = 1 ;
		r = 10 ;
		while ( expNum >= r )
		{
			r *= 10 ;
			if ( ++ col >= 8 )
			{
				break ;
			}
		}
		int	i = (int) iDst + col - 1 ;
		iDst += col ;
		while ( col -- )
		{
			int	n = (int) (expNum % 10) ;
			buf[i --] = (wchar_t) (L'0' + n) ;
			expNum = (expNum - n) / 10 ;
		}
	}
	buf[iDst] = 0 ;
	assert( iDst < sizeof(buf)/sizeof(buf[0]) ) ;
	*this = buf ;
	return	*this ;
}

LString LString::NumberOf( LDouble val, int prec, bool withExp )
{
	LString	str ;
	str.SetNumberOf( val, prec, withExp ) ;
	return	str ;
}

// 文字列を浮動小数点値として解釈
LDouble LString::AsNumber( bool * pHasNumber ) const
{
	bool	flagNumber = false ;
	bool	flagSign = false ;
	bool	flagNegative = false ;
	bool	flagDecimal = false ;
	bool	flagExp = false ;
	LDouble	val = 0.0 ;
	LDouble	dec = 0.1 ;
	LDouble	expNum = 0.0 ;
	LDouble	expSign = 1.0 ;
	for ( size_t i = 0; i < std::wstring::size(); i ++ )
	{
		wchar_t	wch = std::wstring::at(i) ;
		if ( (wch >= L'0') && (wch <= L'9') )
		{
			flagNumber = true ;
			if ( flagExp )
			{
				expNum = expNum * 10.0 + (wch - L'0') ;
			}
			else if ( flagDecimal )
			{
				val += dec * (wch - L'0') ;
				dec *= 0.1 ;
			}
			else
			{
				val = val * 10.0 + (wch - L'0') ;
			}
		}
		else if ( flagNumber && !flagDecimal && !flagExp && (wch == L'.') )
		{
			flagDecimal = true ;
		}
		else if ( !flagNumber && !flagSign && !flagExp )
		{
			if ( wch == L'-' )
			{
				flagSign = true ;
				flagNegative = true ;
			}
			else if ( wch == L'+' )
			{
				flagSign = true ;
				flagNegative = false ;
			}
			else if ( wch > L' ' )
			{
				break ;
			}
		}
		else if ( !flagExp && ((wch == L'E') || (wch == L'e')) )
		{
			flagExp = true ;
			if ( (i + 1) < std::wstring::size() )
			{
				if ( std::wstring::at(i + 1) == L'+' )
				{
					expSign = 1.0 ;
					i ++ ;
				}
				else if ( std::wstring::at(i + 1) == L'-' )
				{
					expSign = -1.0 ;
					i ++ ;
				}
			}
		}
		else if ( (wch > L' ') || flagNumber )
		{
			break ;
		}
	}
	if ( pHasNumber != nullptr )
	{
		*pHasNumber = flagNumber ;
	}
	return	(flagNegative ? -val : val) * pow( 10.0, expNum * expSign ) ;
}

// 文字コード変換
std::string& LString::ToString( std::string& str ) const
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	const int	nDstChars =
		::WideCharToMultiByte
			( CP_ACP, 0, std::wstring::data(),
				(int) GetLength(), nullptr, 0, nullptr, nullptr ) ;
	str.resize( (size_t) nDstChars ) ;
	//
	::WideCharToMultiByte
		( CP_ACP, 0, std::wstring::data(), (int) GetLength(),
					str.data(), nDstChars, nullptr, nullptr ) ;
#else
	if ( sizeof(char) == sizeof(std::uint8_t) )
	{
		std::vector<std::uint8_t>	utf8 ;
		ToUTF8( utf8 ) ;
		str.resize( utf8.size() ) ;
		memcpy( str.data(), utf8.data(),
					utf8.size() * sizeof(std::uint8_t) ) ;
	}
	else if ( sizeof(char) == sizeof(wchar_t) )
	{
		str.resize( GetLength() ) ;
		memcpy( str.data(), std::wstring::data(),
					GetLength() * sizeof(wchar_t) ) ;
	}
	else if ( sizeof(char) == sizeof(std::uint16_t) )
	{
		std::vector<std::uint16_t>	utf16 ;
		ToUTF16( utf16 ) ;
		str.resize( utf16.size() ) ;
		memcpy( str.data(), utf16.data(),
					utf16.size() * sizeof(std::uint16_t) ) ;
	}
	else
	{
		assert( sizeof(char) == sizeof(std::uint32_t) ) ;
		std::vector<std::uint32_t>	utf32 ;
		ToUTF32( utf32 ) ;
		str.resize( utf32.size() ) ;
		memcpy( str.data(), utf32.data(),
					utf32.size() * sizeof(std::uint32_t) ) ;
	}
#endif
	return	str ;
}

std::string LString::ToString( void ) const
{
	std::string	str ;
	ToString( str ) ;
	return	str ;
}

void LString::ToUTF8( std::vector<std::uint8_t>& utf8 ) const
{
	std::vector<std::uint32_t>	utf32 ;
	ToUTF32( utf32 ) ;
	UTF32toUTF8( utf32, utf8 ) ;
}

void LString::ToUTF16( std::vector<std::uint16_t>& utf16 ) const
{
	const size_t	nLength = GetLength() ;
	if ( sizeof(wchar_t) == sizeof(std::uint16_t) )
	{
		utf16.resize( nLength ) ;
		memcpy( utf16.data(),
				std::wstring::data(), nLength * sizeof(wchar_t) ) ;
	}
	else
	{
		assert( sizeof(wchar_t) == sizeof(std::uint32_t) ) ;
		std::vector<std::uint32_t>	utf32 ;
		utf32.resize( nLength ) ;
		memcpy( utf32.data(),
				std::wstring::data(), nLength * sizeof(wchar_t) ) ;
		UTF32toUTF16( utf32, utf16 ) ;
	}
}

void LString::ToUTF32( std::vector<std::uint32_t>& utf32 ) const
{
	const size_t	nLength = GetLength() ;
	if ( sizeof(wchar_t) == sizeof(std::uint16_t) )
	{
		std::vector<std::uint16_t>	utf16 ;
		utf16.resize( nLength ) ;
		memcpy( utf16.data(),
				std::wstring::data(), nLength * sizeof(wchar_t) ) ;
		UTF16toUTF32( utf16, utf32 ) ;
	}
	else
	{
		assert( sizeof(wchar_t) == sizeof(std::uint32_t) ) ;
		utf32.resize( nLength ) ;
		memcpy( utf32.data(),
				std::wstring::data(), nLength * sizeof(wchar_t) ) ;
	}
}

void LString::FromString( const std::string& str )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	const int	nDstWChars =
		::MultiByteToWideChar
			( CP_ACP, MB_PRECOMPOSED,
				str.data(), (int) str.size(), nullptr, 0 ) ;
	std::wstring::resize( (size_t) nDstWChars ) ;
	//
	::MultiByteToWideChar
		( CP_ACP, MB_PRECOMPOSED,
			str.data(), (int) str.size(), std::wstring::data(), nDstWChars ) ;
#else
	if ( sizeof(char) == sizeof(std::uint8_t) )
	{
		std::vector<std::uint8_t>	utf8 ;
		utf8.resize( str.size() ) ;
		memcpy( utf8.data(), str.data(),
					str.size() * sizeof(char) ) ;
		FromUTF8( utf8 ) ;
	}
	else if ( sizeof(char) == sizeof(wchar_t) )
	{
		std::wstring::resize( str.size() ) ;
		memcpy( std::wstring::data(), str.data(),
					str.size() * sizeof(wchar_t) ) ;
	}
	else if ( sizeof(char) == sizeof(std::uint16_t) )
	{
		std::vector<std::uint16_t>	utf16 ;
		utf16.resize( str.size() ) ;
		memcpy( utf16.data(), str.data(),
					str.size() * sizeof(char) ) ;
		FromUTF16( utf16 ) ;
	}
	else
	{
		assert( sizeof(char) == sizeof(std::uint32_t) ) ;
		std::vector<std::uint32_t>	utf32 ;
		utf32.resize( str.size() ) ;
		memcpy( utf32.data(), str.data(),
						str.size() * sizeof(char) ) ;
		FromUTF32( utf32 ) ;
	}
#endif

	OnModified() ;
}

void LString::FromUTF8( const std::vector<std::uint8_t>& utf8 )
{
	std::vector<std::uint32_t>	utf32 ;
	UTF8toUTF32( utf8, utf32 ) ;
	FromUTF32( utf32 ) ;
	OnModified() ;
}

void LString::FromUTF16( const std::vector<std::uint16_t>& utf16 )
{
	if ( sizeof(wchar_t) == sizeof(std::uint16_t) )
	{
		std::wstring::resize( utf16.size() ) ;
		memcpy( std::wstring::data(),
				utf16.data(), utf16.size() * sizeof(wchar_t) ) ;
	}
	else
	{
		assert( sizeof(wchar_t) == sizeof(std::uint32_t) ) ;
		std::vector<std::uint32_t>	utf32 ;
		UTF16toUTF32( utf16, utf32 ) ;
		std::wstring::resize( utf32.size() ) ;
		memcpy( std::wstring::data(),
				utf32.data(), utf32.size() * sizeof(wchar_t) ) ;
	}
	OnModified() ;
}

void LString::FromUTF32( const std::vector<std::uint32_t>& utf32 )
{
	if ( sizeof(wchar_t) == sizeof(std::uint16_t) )
	{
		std::vector<std::uint16_t>	utf16 ;
		UTF32toUTF16( utf32, utf16 ) ;
		resize( utf16.size() ) ;
		memcpy( std::wstring::data(),
				utf16.data(), utf16.size() * sizeof(wchar_t) ) ;
	}
	else
	{
		assert( sizeof(wchar_t) == sizeof(std::uint32_t) ) ;
		resize( utf32.size() ) ;
		memcpy( std::wstring::data(),
				utf32.data(), utf32.size() * sizeof(wchar_t) ) ;
	}
	OnModified() ;
}

// サロゲートペア判定
bool LString::IsHighSurrogateCode( uint16_t code )
{
	return	(code >= 0xD800) & (code < 0xDC00) ;
}

bool LString::IsLowSurrogateCode( uint16_t code )
{
	return	(code >= 0xDC00) & (code < 0xE000) ;
}

// utf32 -> utf8
void LString::UTF32toUTF8
	( const std::vector<std::uint32_t>& utf32,
				std::vector<std::uint8_t>& utf8 )
{
	utf8.clear() ;
	utf8.reserve( utf32.size() * 3 ) ;

	for ( size_t i = 0; i < utf32.size(); i ++ )
	{
		std::uint32_t	ucs4 = utf32.at(i) ;
		if ( ucs4 < 0x80 )
		{
			utf8.push_back( (std::uint8_t) ucs4 ) ;
		}
		else
		{
			int				count = 1 ;
			std::int8_t		first_char = -0x40 ;
			std::uint32_t	range = 0x800 ;
			while ( ucs4 >= range )
			{
				count ++ ;
				range <<= 5 ;
				first_char >>= 1 ;
			}
			//
			int	first_width = 6 - count ;
			ucs4 <<= (32 - (count * 6 + first_width)) ;
			//
			utf8.push_back
				( (std::uint8_t) (first_char | (ucs4 >> (32 - first_width))) ) ;
			ucs4 <<= first_width ;
			//
			for ( int i = 0; i < count; i ++ )
			{
				utf8.push_back( (std::uint8_t) (0x80 | (ucs4 >> 26)) ) ;
				ucs4 <<= 6 ;
			}
		}
	}
}

// utf32 -> utf16
void LString::UTF32toUTF16
	( const std::vector<std::uint32_t>& utf32,
				std::vector<std::uint16_t>& utf16 )
{
	utf16.clear() ;
	utf16.reserve( utf32.size() + 0x10 ) ;

	for ( size_t i = 0; i < utf32.size(); i ++ )
	{
		std::uint32_t	ucs4 = utf32.at(i) ;
		if ( ucs4 >= 0x10000 )
		{
			ucs4 -= 0x10000 ;
			//
			utf16.push_back( (std::uint16_t) (0xD800 + (ucs4 >> 10)) ) ;
			utf16.push_back( (std::uint16_t) 0xDC00 + (ucs4 & 0x3FF) ) ;
		}
		else
		{
			utf16.push_back( (std::uint16_t) ucs4 ) ;
		}
	}
}

// utf8 -> utf32
void LString::UTF8toUTF32
	( const std::vector<std::uint8_t>& utf8,
				std::vector<std::uint32_t>& utf32 )
{
	utf32.clear() ;
	utf32.reserve( utf8.size() ) ;

	const size_t			nUTF8Length = utf8.size() ;
	const std::uint8_t *	pUTF8 = utf8.data() ;
	size_t					i = 0 ;

	while ( i < nUTF8Length )
	{
		std::uint32_t	unicode ;
		std::uint8_t	utf8 = pUTF8[i ++] ;
		//
		if ( utf8 & 0x80 )
		{
			std::uint8_t	mask = 0x20 ;
			int				count = 1 ;
			while ( utf8 & mask )
			{
				count ++ ;
				mask >>= 1 ;
				if ( mask == 0 )
				{
					break ;
				}
			}
			//
			unicode = utf8 & ((1 << (6 - count)) - 1) ;
			for ( int j = 0; (j < count) && (i < nUTF8Length); j ++ )
			{
				unicode = (unicode << 6) | (pUTF8[i ++] & 0x3F) ;
			}
		}
		else
		{
			unicode = utf8 ;
		}
		utf32.push_back( unicode ) ;
	}
}

bool LString::IsValidUTF8( const std::vector<std::uint8_t>& utf8 )
{
	const size_t			nUTF8Length = utf8.size() ;
	const std::uint8_t *	pUTF8 = utf8.data() ;
	size_t					i = 0 ;

	while ( i < nUTF8Length )
	{
		std::uint8_t	utf8 = pUTF8[i ++] ;
		if ( utf8 & 0x80 )
		{
			if ( (utf8 & 0xC0) != 0xC0 )
			{
				return	false ;
			}
			std::uint8_t	mask = 0x20 ;
			int				count = 1 ;
			while ( utf8 & mask )
			{
				count ++ ;
				mask >>= 1 ;
				if ( mask == 0 )
				{
					break ;
				}
			}
			for ( int j = 0; j < count; j ++ )
			{
				if ( i >= nUTF8Length )
				{
					return	false ;
				}
				if ( (pUTF8[i ++] & 0xC0) != 0x80 )
				{
					return	false ;
				}
			}
		}
	}
	return	true ;
}

// utf16 -> utf32
void LString::UTF16toUTF32
	( const std::vector<std::uint16_t>& utf16,
				std::vector<std::uint32_t>& utf32 )
{
	utf32.clear() ;
	utf32.reserve( utf16.size() ) ;

	const size_t			nUTF16Length = utf16.size() ;
	const std::uint16_t *	pUTF16 = utf16.data() ;

	for ( size_t i = 0; i < nUTF16Length; i ++ )
	{
		if ( (i + 1 < nUTF16Length)
			&& IsHighSurrogateCode(pUTF16[i])
			&& IsLowSurrogateCode(pUTF16[i + 1]) )
		{
			std::uint32_t	codeHigh = pUTF16[i] - 0xD800 ;
			std::uint32_t	codeLow  = pUTF16[i] - 0xDC00 ;
			utf32.push_back( 0x10000 + (codeHigh << 10) + codeLow ) ;
			i ++ ;
		}
		else
		{
			utf32.push_back( pUTF16[i] ) ;
		}
	}
}

// 文字列を変更した後に呼び出される
void LString::OnModified( void )
{
}


