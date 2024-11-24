
#ifndef	__LOQUATY_STRING_H__
#define	__LOQUATY_STRING_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 文字列
	//////////////////////////////////////////////////////////////////////////

	class	LString	: public Object, public std::wstring
	{
	public:
		LString( void ) {}
		LString( const std::wstring& wstr )
			: std::wstring( wstr ) {}
		LString( const wchar_t * pwszStr )
			: std::wstring( pwszStr != nullptr ? pwszStr : L"" ) {}
		LString( const wchar_t * pwszStr, size_t nLength )
			: std::wstring( pwszStr != nullptr ? pwszStr : L"", nLength ) {}
		LString( const std::string& str ) ;
		LString( const char * pszStr ) ;
		LString( const char * pszStr, size_t nLength ) ;

		// 代入
		const LString& operator = ( const LString& str )
		{
			std::wstring::operator = ( str ) ;
			OnModified() ;
			return	*this ;
		}
		const LString& operator = ( const std::wstring& str )
		{
			std::wstring::operator = ( str ) ;
			OnModified() ;
			return	*this ;
		}
		const LString& operator = ( const wchar_t * pwszStr )
		{
			std::wstring::operator = ( pwszStr != nullptr ? pwszStr : L"" ) ;
			OnModified() ;
			return	*this ;
		}
		const LString& operator += ( const LString& str )
		{
			std::wstring::operator += ( str ) ;
			OnModified() ;
			return	*this ;
		}
		const LString& operator += ( const std::wstring& str )
		{
			std::wstring::operator += ( str ) ;
			OnModified() ;
			return	*this ;
		}
		const LString& operator += ( const wchar_t * pwszStr )
		{
			std::wstring::operator += ( pwszStr != nullptr ? pwszStr : L"" ) ;
			OnModified() ;
			return	*this ;
		}
		const LString& operator += ( wchar_t wChar )
		{
			std::wstring::operator += ( wChar ) ;
			OnModified() ;
			return	*this ;
		}

		// 文字列長
		size_t GetLength( void ) const
		{
			return	std::wstring::length() ;
		}

		// 空か？
		bool IsEmpty( void ) const
		{
			return	std::wstring::empty() ;
		}

		// 文字取得（範囲外は 0）
		wchar_t GetAt( size_t index ) const
		{
			return	(index < std::wstring::length()) ? std::wstring::at(index) : 0 ;
		}
		wchar_t GetBackAt( size_t index ) const
		{
			return	(index < std::wstring::length())
						? std::wstring::at(std::wstring::length() - index - 1) : 0 ;
		}

		// 文字設定
		void SetAt( size_t index, wchar_t wch )
		{
			if ( index < std::wstring::length() )
			{
				std::wstring::at(index) = wch ;
			}
		}

		// 部分文字列抽出（範囲を超えている場合にはクリップ）
		LString Middle( size_t iStart, size_t nCount ) const ;
		LString Left( size_t nCount ) const ;
		LString Right( size_t nCount ) const ;

		// 部分文字列検索
		ssize_t Find( const wchar_t * pwszStr, size_t iFrom = 0 ) const ;

		// 末端を指定文字数だけ除去する
		const LString& ChopRight( size_t nCount ) ;

		// 末端の空白文字（タブや改行等の制御文字含む）を除去する
		const LString& TrimRight( void ) ;
		LString GetTrimmedRight( void ) const ;

		// 先頭の空白文字（タブや改行等の制御文字含む）を除去する
		LString GetTrimmedLeft( void ) const ;

		// 先頭と末端の空白文字（タブや改行等の制御文字含む）を除去する
		LString GetTrimmed( void ) const ;

		// 小文字アルファベットを大文字に置き換える
		const LString& MakeUpper( void ) ;
		LString ToUpper( void ) const ;

		// 大文字アルファベットを小文字に置き換える
		const LString& MakeLower( void ) ;
		LString ToLower( void ) const ;

		// 文字順序反転
		LString Reverse( void ) const ;

		// 置き換えた文字列を生成
		LString Replace( const wchar_t * pwszOld, const wchar_t * pwszNew ) const ;
		LString Replace( std::function<bool(LStringParser&)> willReplace,
						std::function<LString(const LString&)> getReplaced ) const ;

		// 乗算（繰り返しと反転）
		LString operator * ( int nCount ) const ;

		// 分割
		void Slice( std::vector<LString>& vSliced, const wchar_t * pwszDelimiter ) const ;

		// 文字列フォーマット
		// ※文字列中の %(var) 形式を func(var) の返り値に置き換える
		static LString Format
			( const wchar_t * pwszForm,
				std::function<LString(const LString&)> func ) ;

		// 文字列フォーマット
		// ※文字列中の %(index) 形式を listArg[index] に置き換える
		static LString Format
			( const wchar_t * pwszForm, std::vector<LString>& listArg ) ;

		// 文字列フォーマット
		// ※文字列中の %(name) 形式を mapArg[name] に置き換える
		static LString Format
			( const wchar_t * pwszForm, std::map<LString,LString>& mapArg ) ;

		// 整数値を文字列にフォーマット
		const LString& SetIntegerOf( LLong val, int prec = 0, int radix = 10 ) ;
		static LString IntegerOf( LLong val, int prec = 0, int radix = 10 ) ;

		// 文字列を整数値として解釈
		LLong AsInteger( bool * pHasNumber = nullptr, int radix = 10 ) const ;

		// 浮動小数点値を文字列にフォーマット
		// ※withExp 且つ数値が大きい又は小さい場合には指数表記（?.????E+??）を行う
		const LString& SetNumberOf( LDouble val, int prec = 0, bool withExp = false ) ;
		static LString NumberOf( LDouble val, int prec = 0, bool withExp = false ) ;

		// 文字列を浮動小数点値として解釈
		LDouble AsNumber( bool * pHasNumber = nullptr ) const ;

		// 文字コード変換
		std::string& ToString( std::string& str ) const ;
		std::string ToString( void ) const ;
		void ToUTF8( std::vector<std::uint8_t>& utf8 ) const ;
		void ToUTF16( std::vector<std::uint16_t>& utf16 ) const ;
		void ToUTF32( std::vector<std::uint32_t>& utf32 ) const ;
		void FromString( const std::string& str ) ;
		void FromUTF8( const std::vector<std::uint8_t>& utf8 ) ;
		void FromUTF16( const std::vector<std::uint16_t>& utf16 ) ;
		void FromUTF32( const std::vector<std::uint32_t>& utf32 ) ;

		// サロゲートペア判定
		static bool IsHighSurrogateCode( uint16_t code ) ;
		static bool IsLowSurrogateCode( uint16_t code ) ;

		// utf32 -> utf8
		static void UTF32toUTF8
			( const std::vector<std::uint32_t>& utf32,
						std::vector<std::uint8_t>& utf8 ) ;

		// utf32 -> utf16
		static void UTF32toUTF16
			( const std::vector<std::uint32_t>& utf32,
						std::vector<std::uint16_t>& utf16 ) ;

		// utf8 -> utf32
		static void UTF8toUTF32
			( const std::vector<std::uint8_t>& utf8,
						std::vector<std::uint32_t>& utf32 ) ;
		static bool IsValidUTF8( const std::vector<std::uint8_t>& utf8 ) ;

		// utf16 -> utf32
		static void UTF16toUTF32
			( const std::vector<std::uint16_t>& utf16,
						std::vector<std::uint32_t>& utf32 ) ;

	protected:
		// ※文字列を変更した後に呼び出される
		virtual void OnModified( void ) ;

	} ;

}

#endif

