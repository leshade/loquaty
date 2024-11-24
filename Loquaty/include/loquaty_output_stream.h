
#ifndef	__LOQUATY_OUTPUT_STREAM_H__
#define	__LOQUATY_OUTPUT_STREAM_H__	1

namespace	Loquaty
{

	//////////////////////////////////////////////////////////////////////////
	// 出力ストリーム
	//////////////////////////////////////////////////////////////////////////

	class	LOutputStream	: public Object
	{
	protected:
		LFilePtr	m_file ;

	public:
		LOutputStream( LFilePtr file )
			: m_file( file ) { }

		// 文字列出力
		virtual void WriteString( const LString& str ) ;

		LOutputStream& operator << ( const LString& str ) ;
		LOutputStream& operator << ( const wchar_t * str ) ;
		LOutputStream& operator << ( int num ) ;
		LOutputStream& operator << ( unsigned int num ) ;
		LOutputStream& operator << ( long num ) ;
		LOutputStream& operator << ( double num ) ;

		// ファイル取得
		LFilePtr GetFile( void ) const
		{
			return	m_file ;
		}
	} ;


	//////////////////////////////////////////////////////////////////////////
	// デフォルト文字コード出力ストリーム
	//////////////////////////////////////////////////////////////////////////

	class	LOutputStringStream	: public LOutputStream
	{
	public:
		LOutputStringStream( LFilePtr file )
			: LOutputStream( file ) { }

		// 文字列出力
		virtual void WriteString( const LString& str ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// UTF-8 出力ストリーム
	//////////////////////////////////////////////////////////////////////////

	class	LOutputUTF8Stream	: public LOutputStream
	{
	public:
		LOutputUTF8Stream( LFilePtr file )
			: LOutputStream( file ) { }

		// 文字列出力
		virtual void WriteString( const LString& str ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// UTF-16 出力ストリーム
	//////////////////////////////////////////////////////////////////////////

	class	LOutputUTF16Stream	: public LOutputStream
	{
	public:
		LOutputUTF16Stream( LFilePtr file )
			: LOutputStream( file ) { }

		// 文字列出力
		virtual void WriteString( const LString& str ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// UTF-32 出力ストリーム
	//////////////////////////////////////////////////////////////////////////

	class	LOutputUTF32Stream	: public LOutputStream
	{
	public:
		LOutputUTF32Stream( LFilePtr file )
			: LOutputStream( file ) { }

		// 文字列出力
		virtual void WriteString( const LString& str ) ;

	} ;

}

#endif

