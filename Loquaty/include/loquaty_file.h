
#ifndef	__LOQUATY_FILE_H__
#define	__LOQUATY_FILE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 抽象ファイル
	//////////////////////////////////////////////////////////////////////////

	class	LPureFile : public Object
	{
	public:
		// ファイルから読み込む
		virtual size_t Read( void * buf, size_t bytes ) = 0 ;

		// ファイルへ読み込む
		virtual size_t Write( const void * buf, size_t bytes ) = 0 ;

		// 書き出しを確定する
		virtual void Flush( void ) = 0 ;

		// シークする
		virtual void Seek( std::int64_t pos ) = 0 ;
		virtual void Skip( std::int64_t bytes ) = 0 ;

		// シーク可能か？
		virtual bool IsSeekable( void ) const ;

		// 現在の位置を取得する (ストリームの場合は -1)
		virtual std::int64_t GetPosition( void ) const = 0 ;

		// ファイル全長 (ストリームの場合は -1)
		virtual std::int64_t GetLength( void ) const = 0 ;

		// 現在の位置にファイルを切り詰める
		virtual void Truncate( void ) = 0 ;

		// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
		virtual LString GetFilePath( void ) const = 0 ;

		// ディレクトリを取得する
		//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
		virtual LDirectory * GetDirectory( void ) = 0 ;
	} ;

	typedef	std::shared_ptr<LPureFile>	LFilePtr ;


	//////////////////////////////////////////////////////////////////////////
	// 抽象ディレクトリ
	//////////////////////////////////////////////////////////////////////////

	class	LDirectory	: public Object
	{
	public:
		// ファイルを開くフラグ
		enum	OpenFlags
		{
			modeReadFlag		= 0x00001000,	// 読み込みように開く
			modeWriteFlag		= 0x00002000,	// 書き込み用に開く
			modeStreamingFlag	= 0x00004000,	// ストリーミング明示
												// ※デフォルトでストリーミングを一旦メモリ上にバッファしてから
												//   返すような実装の場合に、バッファせずに開くよう指示
												// ex.アーカイバの圧縮（復号）ストリームや Android の assets ファイル
			modeCreateFlag		= 0x00008000,	// ファイルを作成する
			modeCreateDirsFlag	= 0x00010000,	// ディレクトリが存在しない場合、ディレクトリも作成する

			modeXOTH			= 000000001,
			modeWOTH			= 000000002,
			modeROTH			= 000000004,
			modeRWXO			= 000000007,
			modeXGRP			= 000000010,
			modeWGRP			= 000000020,
			modeRGRP			= 000000040,
			modeRWXG			= 000000070,
			modeXUSR			= 000000100,
			modeWUSR			= 000000200,
			modeRUSR			= 000000400,
			modeRWXU			= 000000700,
			modePermissionMask	= 000000777,

			modeRead			= modeReadFlag,
			modeWrite			= modeWriteFlag,
			modeReadWrite		= modeReadFlag | modeWriteFlag,

			modeCreate			= modeWriteFlag | modeCreateFlag | modeCreateDirsFlag,
		} ;

		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) = 0 ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) = 0 ;

		// ファイル情報取得
		enum	StateField
		{
			stateHasAttribute		= 0x0001,
			stateHasFileSize		= 0x0002,
			stateHasModifiedDate	= 0x0004,
			stateHasCreatedDate		= 0x0008,
			stateHasAccessedDate	= 0x0010,
		} ;
		enum	FileAttribute
		{
			permissionXOTH	= 000000001,
			permissionWOTH	= 000000002,
			permissionROTH	= 000000004,
			permissionRWXO	= 000000007,
			permissionXGRP	= 000000010,
			permissionWGRP	= 000000020,
			permissionRGRP	= 000000040,
			permissionRWXG	= 000000070,
			permissionXUSR	= 000000100,
			permissionWUSR	= 000000200,
			permissionRUSR	= 000000400,
			permissionRWXU	= 000000700,
			permissionMask	= 000000777,
			attrDirectory	= 0x0010000,
			attrHidden		= 0x0020000,
		} ;
		struct	State
		{
			std::uint32_t	fields ;			// enum StateField
			std::uint32_t	attributes ;		// enum FileAttribute
			std::uint64_t	fileSizeInBytes ;	// ファイルサイズ
			LDateTime		dateModified ;		// 最終更新日時
			LDateTime		dateCreated ;		// 作成日時
			LDateTime		dateAccessed ;		// 最終アクセス日時
		} ;
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) = 0 ;

		// ファイルが存在しているか？
		virtual bool IsExisting( const wchar_t * pwszPath ) ;

		// ディレクトリか？
		virtual bool IsDirectory( const wchar_t * pwszPath ) ;

		// ファイル名（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		// ※ ファイル名にはディレクトリパスを含まない
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) = 0 ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) = 0 ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) = 0 ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) = 0 ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) = 0 ;

	} ;

	typedef	std::shared_ptr<LDirectory>	LDirectoryPtr ;


	//////////////////////////////////////////////////////////////////////////
	// 複数ディレクトリ（検索パス）
	//////////////////////////////////////////////////////////////////////////

	class	LDirectories	: public LDirectory
	{
	protected:
		std::vector<LDirectoryPtr>	m_dirs ;

	public:
		LDirectories( void ) {}
		LDirectories( const LDirectories& dirs )
			: m_dirs( dirs.m_dirs ) {}

	public:
		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) ;

		// ファイル情報取得
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) ;

		// ファイル（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) ;

	public:
		// 候補ディレクトリを追加する
		void AddDirectory( LDirectoryPtr dir ) ;
		// 候補ディレクトリを削除する
		bool DetachDirectory( LDirectoryPtr dir ) ;
		// 候補ディレクトリを全て削除する
		void ClearDirectories( void ) ;

		// 複製する
		const LDirectories& operator = ( const LDirectories& dirs ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 複数ディレクトリ（サブパス接続）
	//////////////////////////////////////////////////////////////////////////

	class	LDirectorySchemer	: public LDirectory
	{
	protected:
		std::map<std::wstring,LDirectoryPtr>	m_dirs ;

	public:
		LDirectorySchemer( void ) {}
		LDirectorySchemer( const LDirectorySchemer& dirs )
			: m_dirs( dirs.m_dirs ) {}

	public:
		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) ;

		// ファイル情報取得
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) ;

		// ファイル（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) ;

	public:
		// 候補ディレクトリを追加する
		void AddDirectory( const wchar_t * name, LDirectoryPtr dir ) ;
		// 候補ディレクトリを追加／上書きする
		void SetDirectory( const wchar_t * name, LDirectoryPtr dir ) ;
		// 候補ディレクトリを全て削除する
		void ClearDirectories( void ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// URL スキーマ
	//////////////////////////////////////////////////////////////////////////

	class	LURLSchemer	: public LDirectorySchemer
	{
	public:
		LOQUATY_DLL_EXPORT
		static LURLSchemer	s_schemer ;

	public:
		LURLSchemer( void ) ;
		LURLSchemer( const LURLSchemer& schemer ) ;

		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) ;

		// ファイル情報取得
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) ;

		// ファイル（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) ;

	public:
		// ファイルを開く
		static LFilePtr Open( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

		// LURLSchemer::Open フック用関数ポインタ
		typedef	LFilePtr (*PFUNC_OpenProc)( const wchar_t * pwszPath, long nOpenFlags ) ;
		static PFUNC_OpenProc s_pfnOpen ;

		static LFilePtr OpenProc( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

	public:
		// スキームを取得しスキームに対するパスの先頭位置を pos に返す
		// （':' の後に '//' が存在する場合読み飛ばす）
		// スキーム名がない場合は空白文字列を返し pos に 0 を返す
		static LString ParseSchemeName( const wchar_t * path, size_t& pos ) ;

		// pos 位置から次のディレクトリ区切りを探してディレクトリ名を取得
		// pos には次のディレクトリ・ファイル名の先頭位置を返す
		// ディレクトリ区切りがない場合には pos は進めず空白文字列を返す
		static LString ParseNextDirectory( const wchar_t * path, size_t& pos ) ;
		static ssize_t FindDirectoryPathOf( const wchar_t * path, size_t pos ) ;

		// ファイル名以外のディレクトリパスを取得する
		static LString GetDirectoryOf( const wchar_t * path ) ;

		// ディレクトリパスを除いたファイル名を取得する
		static LString GetFileNameOf( const wchar_t * path ) ;
		static size_t FindFileName( const wchar_t * path ) ;

		// ディレクトリパスを結合する
		// （sub の先頭が '/' 又は '\' で始まる場合、
		// 　又はスキームを含む場合は sub をそのまま返す）
		static LString SubPath
			( const wchar_t * dir, const wchar_t * sub, wchar_t sep = L'/' ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// サブディレクトリ
	//////////////////////////////////////////////////////////////////////////

	class	LSubDirectory	: public LDirectory
	{
	protected:
		LString			m_path ;
		LDirectoryPtr	m_dir ;

	public:
		LSubDirectory
			( LDirectoryPtr dir = nullptr, const wchar_t * path = nullptr )
			: m_dir( dir ), m_path( path ) { }
		LSubDirectory( const LSubDirectory& dir )
			: m_dir( dir.m_dir ), m_path( dir.m_path ) { }

		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) ;

		// ファイル情報取得
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) ;

		// ファイル（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) ;

	public:
		// 修飾されたパスを取得する
		LString MakePath( const wchar_t * pwszPath ) ;

	} ;


#if	defined(_LOQUATY_USES_CPP_FILESYSTEM)

	//////////////////////////////////////////////////////////////////////////
	// C++ 標準のファイルシステム
	//////////////////////////////////////////////////////////////////////////

	class	LCppStdFileDirectory	: public LDirectory
	{
	public:
		LCppStdFileDirectory( void ) {}
		LCppStdFileDirectory( const LCppStdFileDirectory& dir ) {}

	public:
		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;
		std::unique_ptr<std::fstream>
				OpenFileStream( const wchar_t * pwszPath, long nOpenFlags ) ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) ;

		// ファイル情報取得
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) ;

		// ファイル（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) ;

	public:
		// ファイルサイズを変更する
		bool Turncate( const wchar_t * pwszPath, std::uint64_t nBytes ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// C++ 標準のファイル
	//////////////////////////////////////////////////////////////////////////

	class	LCppStdFile	: public LPureFile
	{
	protected:
		std::unique_ptr<std::fstream>	m_fs ;
		LCppStdFileDirectory *			m_dir ;
		LString							m_path ;
		long							m_nOpenFlags ;

	public:
		LCppStdFile( std::unique_ptr<std::fstream> fs,
				long nOpenFlags,
				LCppStdFileDirectory * dir = nullptr,
				const wchar_t * pwszPath = nullptr ) ;

		// ファイルから読み込む
		virtual size_t Read( void * buf, size_t bytes ) ;

		// ファイルへ読み込む
		virtual size_t Write( const void * buf, size_t bytes ) ;

		// 書き出しを確定する
		virtual void Flush( void ) ;

		// シークする
		virtual void Seek( std::int64_t pos ) ;
		virtual void Skip( std::int64_t bytes ) ;

		// 現在の位置を取得する (ストリームの場合は -1)
		virtual std::int64_t GetPosition( void ) const ;

		// ファイル全長 (ストリームの場合は -1)
		virtual std::int64_t GetLength( void ) const ;

		// 現在の位置にファイルを切り詰める
		virtual void Truncate( void ) ;

		// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
		virtual LString GetFilePath( void ) const ;

		// ディレクトリを取得する
		//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
		virtual LDirectory * GetDirectory( void ) ;

	} ;

#endif


	//////////////////////////////////////////////////////////////////////////
	// POSIX / Windows 標準のファイルシステム
	//////////////////////////////////////////////////////////////////////////

	class	LStdFileDirectory	: public LDirectory
	{
	public:
		LStdFileDirectory( void ) {}
		LStdFileDirectory( const LStdFileDirectory& dir ) {}

	public:
		// ファイルを開く
		virtual LFilePtr OpenFile
			( const wchar_t * pwszPath, long nOpenFlags = modeRead ) ;

		// 可能なら同等のディレクトリを複製する
		virtual std::shared_ptr<LDirectory> Duplicate( void ) ;

		// ファイル情報取得
		virtual bool QueryFileState( State& state, const wchar_t * pwszPath ) ;

		// ファイル（サブディレクトリ含む）列挙
		// ※ files へは以前のデータを削除せずに追加
		virtual void ListFiles
			( std::vector<LString>& files,
					const wchar_t * pwszSubDirPath = nullptr ) ;

		// ファイル削除
		virtual bool DeleteFile( const wchar_t * pwszPath ) ;

		// ファイル名変更
		virtual bool RenameFile
			( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath ) ;

		// サブディレクトリ作成
		virtual bool CreateDirectory( const wchar_t * pwszPath ) ;

		// サブディレクトリ削除
		virtual bool DeleteDirectory( const wchar_t * pwszPath ) ;

	} ;

	#if	defined(_LOQUATY_USES_CPP_FILESYSTEM)
		typedef	LCppStdFileDirectory	LFileDirectory ;
	#else
		typedef	LStdFileDirectory	LFileDirectory ;
	#endif

	//////////////////////////////////////////////////////////////////////////
	// POSIX / Windows 標準のファイル
	//////////////////////////////////////////////////////////////////////////

	class	LStdFile	: public LPureFile
	{
	public:
	#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
		typedef	HANDLE					file_t ;
		static constexpr const file_t	InvalidFile = INVALID_HANDLE_VALUE ;
	#else
		typedef	int						file_t ;
		static constexpr const file_t	InvalidFile = -1 ;
	#endif

	protected:
		file_t			m_file ;
		LString			m_path ;
		long			m_nOpenFlags ;
		LDirectory *	m_dir ;

	public:
		LStdFile( LDirectory * dir = nullptr ) ;
		virtual ~LStdFile( void ) ;

		// ファイルを開く
		bool Open( const wchar_t * pwszPath, long nOpenFlags ) ;

		// ファイルを閉じる
		void Close( void ) ;

		// ファイルから読み込む
		virtual size_t Read( void * buf, size_t bytes ) ;

		// ファイルへ読み込む
		virtual size_t Write( const void * buf, size_t bytes ) ;

		// 書き出しを確定する
		virtual void Flush( void ) ;

		// シークする
		virtual void Seek( std::int64_t pos ) ;
		virtual void Skip( std::int64_t bytes ) ;

		// 現在の位置を取得する (ストリームの場合は -1)
		virtual std::int64_t GetPosition( void ) const ;

		// ファイル全長 (ストリームの場合は -1)
		virtual std::int64_t GetLength( void ) const ;

		// 現在の位置にファイルを切り詰める
		virtual void Truncate( void ) ;

		// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
		virtual LString GetFilePath( void ) const ;

		// ディレクトリを取得する
		//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
		virtual LDirectory * GetDirectory( void ) ;

	public:
		// ファイルが存在するか？
		static bool IsFileExisting( const wchar_t * pwszPath ) ;
		// ファイル情報取得
		static bool QueryFileState
				( LDirectory::State& state, const wchar_t * pwszPath ) ;
		// ディレクトリ作成
		static bool CreateDirectories( const wchar_t * pwszDirPath ) ;
		static bool CreateDirectory( const wchar_t * pwszDirPath ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// バッファされたファイル
	//////////////////////////////////////////////////////////////////////////

	class	LBufferedFile	: public LPureFile
	{
	protected:
		std::shared_ptr<LArrayBufStorage>	m_buf ;
		size_t								m_pos ;
		bool								m_written ;

		LDirectory *						m_dir ;
		LString								m_path ;
		LFilePtr							m_fileWriteBack ;
		long								m_nOpenFlags ;

	public:
		LBufferedFile
			( long nOpenFlags,
				std::shared_ptr<LArrayBufStorage> buf = nullptr,
				LDirectory * dir = nullptr,
				const wchar_t * pwszPath = nullptr,
				LFilePtr fileWriteBack = nullptr ) ;
		virtual ~LBufferedFile( void ) ;

		// バッファにファイルから読み込む
		void LoadFileFrom( LFilePtr file ) ;
		static void LoadFileFrom
			( std::shared_ptr<LArrayBufStorage> buf, LFilePtr file ) ;

	public:
		// ファイルから読み込む
		virtual size_t Read( void * buf, size_t bytes ) ;

		// ファイルへ読み込む
		virtual size_t Write( const void * buf, size_t bytes ) ;

		// 書き出しを確定する
		virtual void Flush( void ) ;

		// シークする
		virtual void Seek( std::int64_t pos ) ;
		virtual void Skip( std::int64_t bytes ) ;

		// シーク可能か？
		virtual bool IsSeekable( void ) const ;

		// 現在の位置を取得する (ストリームの場合は -1)
		virtual std::int64_t GetPosition( void ) const ;

		// ファイル全長 (ストリームの場合は -1)
		virtual std::int64_t GetLength( void ) const ;

		// 現在の位置にファイルを切り詰める
		virtual void Truncate( void ) ;

		// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
		virtual LString GetFilePath( void ) const ;

		// ディレクトリを取得する
		//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
		virtual LDirectory * GetDirectory( void ) ;

	} ;


}

#endif


