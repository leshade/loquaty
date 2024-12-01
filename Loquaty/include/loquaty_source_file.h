
#ifndef	__LOQUATY_SOURCE_FILE_H__
#define	__LOQUATY_SOURCE_FILE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// テキストファイル
	//////////////////////////////////////////////////////////////////////////

	class	LTextFileParser : public LStringParser
	{
	protected:
		LDirectoryPtr	m_pDirectory ;
		LString			m_strFilePath ;

	public:
		LTextFileParser( void ) {}
		LTextFileParser( const LTextFileParser& src )
			: LStringParser( src ) {}
		LTextFileParser( const LStringParser& spars )
			: LStringParser( spars ),
				m_pDirectory( spars.GetFileDirectory() ),
				m_strFilePath( spars.GetFileName() ) {}
		LTextFileParser( const LString& str )
			: LStringParser( str ) {}
		LTextFileParser( const wchar_t * pwszText )
			: LStringParser( pwszText ) {}

		// ファイルを読み込む
		bool LoadTextFile
			( const wchar_t * pwszFile, LDirectory * pDirPath = nullptr ) ;
	
		static bool LoadTextFile
			( LString& strText,
				const wchar_t * pwszFile, LDirectory * pDirPath = nullptr ) ;
		static bool ReadTextFile( LString& strText, LFilePtr file ) ;

		// この文字列を読み込んだ元のファイル名を取得する
		virtual LString GetFileName( void ) const ;
		// 開いたディレクトリを取得する
		virtual LDirectoryPtr GetFileDirectory( void ) const ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// ソースファイル
	//////////////////////////////////////////////////////////////////////////

	class	LSourceFile : public LTextFileParser
	{
	protected:
		bool		m_hasComment ;
		LString		m_strComment ;

	public:
		LSourceFile( void )
			: m_hasComment( false ) {}
		LSourceFile( const LSourceFile& src )
			: LTextFileParser( src ), m_hasComment( src.m_hasComment ) {}
		LSourceFile( const LString& str )
			: LTextFileParser( str ), m_hasComment( false ) {}
		LSourceFile( const wchar_t * pwszText )
			: LTextFileParser( pwszText ), m_hasComment( false ) {}

		// 空白文字を読み飛ばす（終端に到達した場合は false を返す）
		virtual bool PassSpace( void ) ;

		// 直前のコメントを取得する
		const LString& GetCommentBefore( void ) const
		{
			return	m_strComment ;
		}
	} ;

	typedef	std::shared_ptr<LSourceFile>	LSourceFilePtr ;


	//////////////////////////////////////////////////////////////////////////
	// ソースファイル・プロデューサー
	//////////////////////////////////////////////////////////////////////////

	class	LSourceProducer	: public Object
	{
	protected:
		std::map<std::wstring,LSourceFilePtr>	m_mapSources ;
		std::map<LStringParser*,LSourceFilePtr>	m_mapSafeStock ;
		LDirectories							m_dirPath ;

	public:
		// ロード済みソース取得
		LSourceFilePtr GetSourceFile( const wchar_t * pwszFile ) ;
		// ソースを読み込んで取得
		// 既にロード済みならそれを取得
		LSourceFilePtr LoadSourceFile
			( const wchar_t * pwszFile, LDirectory * pDirPath = nullptr ) ;

		// 安全なソースを取得
		// pParser がロード済みのソースならそれを取得
		// そうでないなら複製する
		LSourceFilePtr GetSafeSource
			( LStringParser * pParser, bool flagTemporary = true ) ;

		// 複製
		const LSourceProducer& operator = ( const LSourceProducer& srcp ) ;

		// ファイルをロードするためのパス
		LDirectories& DirectoryPath( void )
		{
			return	m_dirPath ;
		}

	protected:
		// ファイル名の正規化
		static LString NormalizeFilePath( const wchar_t * pwszFile ) ;

	} ;


}

#endif


