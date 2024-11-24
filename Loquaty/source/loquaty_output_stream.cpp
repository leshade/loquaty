
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////
// 出力ストリーム
//////////////////////////////////////////////////////////////////////////

// 文字列出力
void LOutputStream::WriteString( const LString& lstr )
{
	std::string	sstr ;
	lstr.ToString( sstr ) ;

	m_file->Write( sstr.data(), sstr.length() * sizeof(char) ) ;
}

LOutputStream& LOutputStream::operator << ( const LString& str )
{
	WriteString( str ) ;
	return	*this ;
}

LOutputStream& LOutputStream::operator << ( const wchar_t * str )
{
	WriteString( LString(str) ) ;
	return	*this ;
}

LOutputStream& LOutputStream::operator << ( int num )
{
	WriteString( LString::IntegerOf( num ) ) ;
	return	*this ;
}

LOutputStream& LOutputStream::operator << ( unsigned int num )
{
	WriteString( LString::IntegerOf( num ) ) ;
	return	*this ;
}

LOutputStream& LOutputStream::operator << ( long num )
{
	WriteString( LString::IntegerOf( num ) ) ;
	return	*this ;
}

LOutputStream& LOutputStream::operator << ( double num )
{
	WriteString( LString::NumberOf( num ) ) ;
	return	*this ;
}



//////////////////////////////////////////////////////////////////////////
// デフォルト文字コード出力ストリーム
//////////////////////////////////////////////////////////////////////////

// 文字列出力
void LOutputStringStream::WriteString( const LString& str )
{
	std::string	sstr ;
	str.ToString( sstr ) ;

	m_file->Write( sstr.data(), sstr.size() * sizeof(char) ) ;
}



//////////////////////////////////////////////////////////////////////////
// UTF-8 出力ストリーム
//////////////////////////////////////////////////////////////////////////

// 文字列出力
void LOutputUTF8Stream::WriteString( const LString& str )
{
	std::vector<std::uint8_t>	utf8 ;
	str.ToUTF8( utf8 ) ;

	m_file->Write( utf8.data(), utf8.size() * sizeof(std::uint8_t) ) ;
}



//////////////////////////////////////////////////////////////////////////
// UTF-16 出力ストリーム
//////////////////////////////////////////////////////////////////////////

// 文字列出力
void LOutputUTF16Stream::WriteString( const LString& str )
{
	std::vector<std::uint16_t>	utf16 ;
	str.ToUTF16( utf16 ) ;

	m_file->Write( utf16.data(), utf16.size() * sizeof(std::uint16_t) ) ;
}



//////////////////////////////////////////////////////////////////////////
// UTF-32 出力ストリーム
//////////////////////////////////////////////////////////////////////////

// 文字列出力
void LOutputUTF32Stream::WriteString( const LString& str )
{
	std::vector<std::uint32_t>	utf32 ;
	str.ToUTF32( utf32 ) ;

	m_file->Write( utf32.data(), utf32.size() * sizeof(std::uint32_t) ) ;
}



