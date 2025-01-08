
#include <loquaty.h>

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

#else
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#endif

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 抽象ファイル
//////////////////////////////////////////////////////////////////////////////

// シーク可能か？
bool LPureFile::IsSeekable( void ) const
{
	return	(GetLength() >= 0) ;
}



//////////////////////////////////////////////////////////////////////////////
// 抽象ディレクトリ
//////////////////////////////////////////////////////////////////////////////

// ファイルが存在しているか？
bool LDirectory::IsExisting( const wchar_t * pwszPath )
{
	State	state ;
	return	QueryFileState( state, pwszPath ) ;
}

// ディレクトリか？
bool LDirectory::IsDirectory( const wchar_t * pwszPath )
{
	State	state ;
	if ( QueryFileState( state, pwszPath ) )
	{
		return	(state.fields & stateHasAttribute)
				&& (state.attributes & attrDirectory) ;
	}
	return	false ;
}



//////////////////////////////////////////////////////////////////////////////
// 複数ディレクトリ（検索パス）
//////////////////////////////////////////////////////////////////////////////

// ファイルを開く
LFilePtr LDirectories::OpenFile( const wchar_t * pwszPath, long nOpenFlags )
{
	for ( auto dir : m_dirs )
	{
		LFilePtr	file = dir->OpenFile( pwszPath, nOpenFlags ) ;
		if ( file != nullptr )
		{
			return	file ;
		}
	}
	return	nullptr ;
}

// 可能なら同等のディレクトリを複製する
std::shared_ptr<LDirectory> LDirectories::Duplicate( void )
{
	return	std::make_shared<LDirectories>( *this ) ;
}

// ファイル情報取得
bool LDirectories::QueryFileState( State& state, const wchar_t * pwszPath )
{
	for ( auto dir : m_dirs )
	{
		if ( dir->QueryFileState( state, pwszPath ) )
		{
			return	true ;
		}
	}
	return	false ;
}

// ファイル（サブディレクトリ含む）列挙
// ※ files へは以前のデータを削除せずに追加
void LDirectories::ListFiles
	( std::vector<LString>& files, const wchar_t * pwszSubDirPath )
{
	for ( auto dir : m_dirs )
	{
		dir->ListFiles( files, pwszSubDirPath ) ;
	}
}

// ファイル削除
bool LDirectories::DeleteFile( const wchar_t * pwszPath )
{
	for ( auto dir : m_dirs )
	{
		if ( dir->DeleteFile( pwszPath ) )
		{
			return	true ;
		}
	}
	return	false ;
}

// ファイル名変更
bool LDirectories::RenameFile
	( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath )
{
	for ( auto dir : m_dirs )
	{
		if ( dir->RenameFile( pwszOldPath, pwszNewPath ) )
		{
			return	true ;
		}
	}
	return	false ;
}

// サブディレクトリ作成
bool LDirectories::CreateDirectory( const wchar_t * pwszPath )
{
	for ( auto dir : m_dirs )
	{
		if ( dir->CreateDirectory( pwszPath ) )
		{
			return	true ;
		}
	}
	return	false ;
}

// サブディレクトリ削除
bool LDirectories::DeleteDirectory( const wchar_t * pwszPath )
{
	for ( auto dir : m_dirs )
	{
		if ( dir->DeleteDirectory( pwszPath ) )
		{
			return	true ;
		}
	}
	return	false ;
}

// 候補ディレクトリを追加する
void LDirectories::AddDirectory( LDirectoryPtr dir )
{
	m_dirs.push_back( dir ) ;
}

// 候補ディレクトリを全て削除する
void LDirectories::ClearDirectories( void )
{
	m_dirs.clear() ;
}

// 複製する
const LDirectories& LDirectories::operator = ( const LDirectories& dirs )
{
	m_dirs = dirs.m_dirs ;
	return	*this ;
}



//////////////////////////////////////////////////////////////////////////////
// 複数ディレクトリ（サブパス接続）
//////////////////////////////////////////////////////////////////////////////

// ファイルを開く
LFilePtr LDirectorySchemer::OpenFile( const wchar_t * pwszPath, long nOpenFlags )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return	nullptr ;
	}
	return	iter->second->OpenFile( pwszPath + pos, nOpenFlags ) ;
}

// 可能なら同等のディレクトリを複製する
std::shared_ptr<LDirectory> LDirectorySchemer::Duplicate( void )
{
	return	std::make_shared<LDirectorySchemer>( *this ) ;
}

// ファイル情報取得
bool LDirectorySchemer::QueryFileState( State& state, const wchar_t * pwszPath )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return	false ;
	}
	return	iter->second->QueryFileState( state, pwszPath + pos ) ;
}

// ファイル（サブディレクトリ含む）列挙
// ※ files へは以前のデータを削除せずに追加
void LDirectorySchemer::ListFiles
	( std::vector<LString>& files, const wchar_t * pwszSubDirPath )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszSubDirPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return ;
	}
	iter->second->ListFiles( files, pwszSubDirPath + pos ) ;
}

// ファイル削除
bool LDirectorySchemer::DeleteFile( const wchar_t * pwszPath )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return	false ;
	}
	return	iter->second->DeleteFile( pwszPath + pos ) ;
}

// ファイル名変更
bool LDirectorySchemer::RenameFile
	( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszOldPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return	false ;
	}
	if ( LString( pwszOldPath, pos ) != LString( pwszNewPath ).Left( pos ) )
	{
		return	false ;
	}
	return	iter->second->RenameFile( pwszOldPath + pos, pwszNewPath + pos ) ;
}

// サブディレクトリ作成
bool LDirectorySchemer::CreateDirectory( const wchar_t * pwszPath )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return	false ;
	}
	return	iter->second->CreateDirectory( pwszPath + pos ) ;
}

// サブディレクトリ削除
bool LDirectorySchemer::DeleteDirectory( const wchar_t * pwszPath )
{
	size_t	pos = 0 ;
	LString	strDir = LURLSchemer::ParseNextDirectory( pwszPath, pos ) ;

	auto	iter = m_dirs.find( strDir ) ;
	if ( iter == m_dirs.end() )
	{
		return	false ;
	}
	return	iter->second->DeleteDirectory( pwszPath + pos ) ;
}

// 候補ディレクトリを追加する
void LDirectorySchemer::AddDirectory( const wchar_t * name, LDirectoryPtr dir )
{
	m_dirs.insert
		( std::make_pair<std::wstring,LDirectoryPtr>
							( name, LDirectoryPtr(dir) ) ) ;
}

// 候補ディレクトリを追加／上書きする
void LDirectorySchemer::SetDirectory( const wchar_t * name, LDirectoryPtr dir )
{
	auto	iter = m_dirs.find( name ) ;
	if ( iter != m_dirs.end() )
	{
		iter->second = dir ;
	}
	else
	{
		AddDirectory( name, dir ) ;
	}
}

// 候補ディレクトリを全て削除する
void LDirectorySchemer::ClearDirectories( void )
{
	m_dirs.clear() ;
}



//////////////////////////////////////////////////////////////////////////////
// URL スキーマ
//////////////////////////////////////////////////////////////////////////////

LOQUATY_DLL_DECL
( LURLSchemer	LURLSchemer::s_schemer ) ;

LURLSchemer::PFUNC_OpenProc	LURLSchemer::s_pfnOpen = &LURLSchemer::OpenProc ;


LURLSchemer::LURLSchemer( void )
{
	LDirectoryPtr	pFileDir = std::make_shared<LFileDirectory>() ;

	AddDirectory( L"", pFileDir ) ;		// そのまま（但しドライブレターを含まない）
	AddDirectory( L"file", pFileDir ) ;	// file://～
}

LURLSchemer::LURLSchemer( const LURLSchemer& schemer )
	: LDirectorySchemer( schemer )
{
}

// ファイルを開く
LFilePtr LURLSchemer::OpenFile( const wchar_t * pwszPath, long nOpenFlags )
{
	size_t	posScheme = 0 ;
	LString	strScheme = ParseSchemeName( pwszPath, posScheme ) ;

	auto	iter = m_dirs.find( strScheme ) ;
	if ( iter == m_dirs.end() )
	{
		return	nullptr ;
	}
	return	iter->second->OpenFile( pwszPath + posScheme, nOpenFlags ) ;
}

// 可能なら同等のディレクトリを複製する
std::shared_ptr<LDirectory> LURLSchemer::Duplicate( void )
{
	if ( &s_schemer == this )
	{
		return	std::make_shared<LSubDirectory>() ;
	}
	else
	{
		return	std::make_shared<LURLSchemer>( *this ) ;
	}
}

// ファイルを開く
LFilePtr LURLSchemer::Open( const wchar_t * pwszPath, long nOpenFlags )
{
	return	s_pfnOpen( pwszPath, nOpenFlags ) ;
}

LFilePtr LURLSchemer::OpenProc( const wchar_t * pwszPath, long nOpenFlags )
{
	return	s_schemer.OpenFile( pwszPath, nOpenFlags ) ;
}

// スキームを取得
// スキーム名がない場合は空白文字列を返し pos=0
LString LURLSchemer::ParseSchemeName( const wchar_t * path, size_t& pos )
{
	if ( path == nullptr )
	{
		pos = 0 ;
		return	LString() ;
	}
	size_t	i = 0 ;
	while ( path[i] != 0 )
	{
		if ( path[i] == L':' )
		{
			pos = i + 1 ;
			return	LString( path, i ) ;
		}
		i ++ ;
	}
	pos = 0 ;
	return	LString() ;
}

// pos 位置から次のディレクトリ区切りを探してディレクトリ名を取得
// pos には次のディレクトリ・ファイル名の先頭位置を返す
// ディレクトリ区切りがない場合には pos は進めず空白文字列を返す
LString LURLSchemer::ParseNextDirectory( const wchar_t * path, size_t& pos )
{
	size_t	iLastPos = pos ;
	ssize_t	iNextDir = FindDirectoryPathOf( path, pos ) ;
	if ( iNextDir >= 0 )
	{
		pos = (size_t) iNextDir + 1 ;
		return	LString( path + iLastPos, (size_t) iNextDir - iLastPos ) ;
	}
	return	LString() ;
}

ssize_t LURLSchemer::FindDirectoryPathOf( const wchar_t * path, size_t pos )
{
	if ( path == nullptr )
	{
		return	-1 ;
	}
	size_t	i = pos ;
	while ( path[i] != 0 )
	{
		if ( (path[i] == L'/') || (path[i] == L'\\') )
		{
			return	(ssize_t) i ;
		}
		i ++ ;
	}
	return	-1 ;
}

// ファイル名以外のディレクトリパスを取得する
LString LURLSchemer::GetDirectoryOf( const wchar_t * path )
{
	size_t	iFile = FindFileName( path ) ;
	if ( iFile == 0 )
	{
		return	LString() ;
	}
	return	LString( path, iFile - 1 ) ;
}

// ディレクトリパスを除いたファイル名を取得する
LString LURLSchemer::GetFileNameOf( const wchar_t * path )
{
	return	LString( path + FindFileName(path) ) ;
}

size_t LURLSchemer::FindFileName( const wchar_t * path )
{
	size_t	iFile = 0 ;
	ssize_t	pos = 0 ;
	for ( ; ; )
	{
		iFile = (size_t) pos ;
		pos = FindDirectoryPathOf( path, (size_t) pos ) ;
		if ( pos < 0 )
		{
			break ;
		}
		pos ++ ;
	}
	return	iFile ;
}

// ディレクトリパスを結合する
// （sub の先頭が '/' 又は '\' で始まる場合、
// 　又はスキームを含む場合は sub をそのまま返す）
LString LURLSchemer::SubPath
	( const wchar_t * dir, const wchar_t * sub, wchar_t sep )
{
	if ( (dir == nullptr) || (dir[0] == 0) )
	{
		return	LString( sub ) ;
	}
	if ( (sub == nullptr) || (sub[0] == 0) )
	{
		return	LString( dir ) ;
	}
	if ( (sub != nullptr)
		&& ((sub[0] == sep) || (sub[0] == L'/') || (sub[0] == L'\\')) )
	{
		return	LString( sub ) ;
	}
	size_t	pos = 0 ;
	LString	strScheme = ParseSchemeName( sub, pos ) ;
	if ( !strScheme.IsEmpty() )
	{
		return	LString( sub ) ;
	}
	LString	strPath = dir ;
	for ( ; ; )
	{
		wchar_t	wchBack = strPath.GetBackAt(0) ;
		if ( (wchBack == sep) || (wchBack == L'/') || (wchBack == '\\') )
		{
			strPath.ChopRight( 1 ) ;
		}
		else
		{
			break ;
		}
	}
	wchar_t	szSep[2] = { sep, 0 } ;
	strPath += szSep ;
	return	strPath + sub ;
}



//////////////////////////////////////////////////////////////////////////////
// サブディレクトリ
//////////////////////////////////////////////////////////////////////////////

// ファイルを開く
LFilePtr LSubDirectory::OpenFile
	( const wchar_t * pwszPath, long nOpenFlags )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->OpenFile( MakePath(pwszPath).c_str(), nOpenFlags ) ;
}

// 可能なら同等のディレクトリを複製する
std::shared_ptr<LDirectory> LSubDirectory::Duplicate( void )
{
	return	std::make_shared<LSubDirectory>( *this ) ;
}

// ファイル情報取得
bool LSubDirectory::QueryFileState( State& state, const wchar_t * pwszPath )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->QueryFileState( state, MakePath(pwszPath).c_str() ) ;
}

// ファイル（サブディレクトリ含む）列挙
// ※ files へは以前のデータを削除せずに追加
void LSubDirectory::ListFiles
	( std::vector<LString>& files, const wchar_t * pwszSubDirPath )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->ListFiles( files, MakePath(pwszSubDirPath).c_str() ) ;
}

// ファイル削除
bool LSubDirectory::DeleteFile( const wchar_t * pwszPath )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->DeleteFile( MakePath(pwszPath).c_str() ) ;
}

// ファイル名変更
bool LSubDirectory::RenameFile
	( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->RenameFile
				( MakePath(pwszOldPath).c_str(),
					MakePath(pwszNewPath).c_str() ) ;
}

// サブディレクトリ作成
bool LSubDirectory::CreateDirectory( const wchar_t * pwszPath )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->CreateDirectory( MakePath(pwszPath).c_str() ) ;
}

// サブディレクトリ削除
bool LSubDirectory::DeleteDirectory( const wchar_t * pwszPath )
{
	LDirectory*	pDir = (m_dir != nullptr) ? m_dir.get() : &LURLSchemer::s_schemer ;
	return	pDir->DeleteDirectory( MakePath(pwszPath).c_str() ) ;
}

// 修飾されたパスを取得する
LString LSubDirectory::MakePath( const wchar_t * pwszPath )
{
	return	LURLSchemer::SubPath( m_path.c_str(), pwszPath ) ;
}


#if	defined(_LOQUATY_USES_CPP_FILESYSTEM)

//////////////////////////////////////////////////////////////////////////////
// C++ 標準のファイルシステム
//////////////////////////////////////////////////////////////////////////////

// ファイルを開く
LFilePtr LCppStdFileDirectory::OpenFile( const wchar_t * pwszPath, long nOpenFlags )
{
	std::unique_ptr<std::fstream>
				fs = OpenFileStream( pwszPath, nOpenFlags ) ;
	if ( fs == nullptr )
	{
		return	nullptr ;
	}
	return	std::make_shared<LCppStdFile>( std::move(fs), nOpenFlags, this, pwszPath ) ;
}

std::unique_ptr<std::fstream>
	LCppStdFileDirectory::OpenFileStream( const wchar_t * pwszPath, long nOpenFlags )
{
	LString	strPath = pwszPath ;
	try
	{
		if ( nOpenFlags & modeCreateDirsFlag )
		{
			LString	strDir = LURLSchemer::GetDirectoryOf( strPath.c_str() ) ;
			if ( !strDir.IsEmpty() )
			{
				std::filesystem::create_directories( strDir.c_str() ) ;
			}
		}
		std::ios_base::openmode	mode = std::ios_base::binary ;
		if ( nOpenFlags & modeReadFlag )
		{
			mode |= std::ios_base::in ;
		}
		if ( nOpenFlags & modeWriteFlag )
		{
			mode |= std::ios_base::out ;
		}
		if ( nOpenFlags & modeCreateFlag )
		{
			mode |= std::ios_base::trunc ;
		}
		else
		{
			mode |= std::ios_base::in ;	// ※書き込みモードで開くときにファイルを切り詰めない
		}
		std::unique_ptr<std::fstream>	ofs =
				std::make_unique<std::fstream>( strPath.c_str(), mode ) ;
		if ( !ofs->is_open() )
		{
			return	nullptr ;
		}
		return	std::move(ofs) ;
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::OpenOutputStream : %s\n", e.what() ) ;
	}
	return	nullptr ;
}

// 可能なら同等のディレクトリを複製する
std::shared_ptr<LDirectory> LCppStdFileDirectory::Duplicate( void )
{
	return	std::make_shared<LCppStdFileDirectory>( *this ) ;
}

// ファイル情報取得
bool LCppStdFileDirectory::QueryFileState( State& state, const wchar_t * pwszPath )
{
	LString	strPath = pwszPath ;
	state.fields = 0 ;
	try
	{
		std::filesystem::file_status
				status = std::filesystem::status( strPath.c_str() ) ;
		state.attributes = 0 ;
		switch ( status.type() )
		{
		case	std::filesystem::file_type::regular:
		default:
			break ;
		case	std::filesystem::file_type::directory:
		case	std::filesystem::file_type::symlink:
			state.attributes |= attrDirectory ;
			break ;
		case	std::filesystem::file_type::none:
		case	std::filesystem::file_type::not_found:
			return	false ;
		}
		state.attributes |=
			(std::uint32_t) status.permissions() & permissionMask ;
		state.fields |= stateHasAttribute ;

		state.fileSizeInBytes = std::filesystem::file_size( strPath.c_str() ) ;
		state.fields |= stateHasFileSize ;

		 std::filesystem::file_time_type
				file_time = std::filesystem::last_write_time( strPath.c_str() ) ;
		 std::time_t	time =
			 std::chrono::duration_cast<std::chrono::seconds>
							( file_time.time_since_epoch() ).count() ;
		 // ※ time_since_epoch はファイルシステムの時刻とは一致するとは限らない

		std::tm		tmLocal ;
		localtime_s( &tmLocal, &time ) ;

		state.dateModified.SetTm( &tmLocal ) ;
		state.attributes |= stateHasModifiedDate ;

		return	true ;
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::QueryFileState : %s\n", e.what() ) ;
	}
	return	false ;
}

// ファイル（サブディレクトリ含む）列挙
// ※ files へは以前のデータを削除せずに追加
void LCppStdFileDirectory::ListFiles
	( std::vector<LString>& files, const wchar_t * pwszSubDirPath )
{
	LString	strPath = pwszSubDirPath ;
	for ( auto x : std::filesystem::directory_iterator( strPath.c_str() ) )
	{
		const std::filesystem::path&	pathFile = x.path() ;
		if ( std::filesystem::exists( pathFile ) )
		{
			files.push_back
				( LURLSchemer::GetFileNameOf( LString(pathFile).c_str() ) ) ;
		}
	}
}

// ファイル削除
bool LCppStdFileDirectory::DeleteFile( const wchar_t * pwszPath )
{
	LString	strPath = pwszPath ;
	try
	{
		if ( std::filesystem::exists( strPath.c_str() ) )
		{
			std::filesystem::remove( strPath.c_str() ) ;
			return	std::filesystem::exists( strPath.c_str() ) ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::DeleteFile : %s\n", e.what() ) ;
	}
	return	false ;
}

// ファイル名変更
bool LCppStdFileDirectory::RenameFile
	( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath )
{
	LString	strOldPath = pwszOldPath ;
	LString	strNewPath = pwszNewPath ;
	try
	{
		std::filesystem::rename( strOldPath.c_str(), strNewPath.c_str() ) ;
		return	true ;
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::RenameFile : %s\n", e.what() ) ;
	}
	return	false ;
}

// サブディレクトリ作成
bool LCppStdFileDirectory::CreateDirectory( const wchar_t * pwszPath )
{
	LString	strPath = pwszPath ;
	try
	{
		return	std::filesystem::create_directories( strPath.c_str() ) ;
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::CreateDirectory : %s\n", e.what() ) ;
	}
	return	false ;
}

// サブディレクトリ削除
bool LCppStdFileDirectory::DeleteDirectory( const wchar_t * pwszPath )
{
	LString	strPath = pwszPath ;
	try
	{
		if ( std::filesystem::exists( strPath.c_str() ) )
		{
			std::filesystem::remove( strPath.c_str() ) ;
			return	std::filesystem::exists( strPath.c_str() ) ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::DeleteDirectory : %s\n", e.what() ) ;
	}
	return	false ;
}

// ファイルサイズを変更する
bool LCppStdFileDirectory::Turncate( const wchar_t * pwszPath, std::uint64_t nBytes )
{
	LString	strPath = pwszPath ;
	try
	{
		std::filesystem::resize_file( strPath.c_str(), nBytes ) ;
		return	true ;
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFileDirectory::Turncate : %s\n", e.what() ) ;
	}
	return	false ;
}



//////////////////////////////////////////////////////////////////////////////
// C++ 標準のファイル
//////////////////////////////////////////////////////////////////////////////

LCppStdFile::LCppStdFile
	( std::unique_ptr<std::fstream> fs,
		long nOpenFlags, LCppStdFileDirectory * dir, const wchar_t * pwszPath )
	: m_fs( std::move(fs) ),
		m_dir( dir ), m_path( pwszPath ), m_nOpenFlags( nOpenFlags )
{
}

// ファイルから読み込む
size_t LCppStdFile::Read( void * buf, size_t bytes )
{
	assert( m_nOpenFlags & LDirectory::modeReadFlag ) ;
	assert( m_fs != nullptr ) ;
	try
	{
		if ( m_fs != nullptr )
		{
			m_fs->peek() ;
			return	m_fs->readsome( reinterpret_cast<char*>( buf ), bytes ) ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::Read : %s\n", e.what() ) ;
	}
	return	0 ;
}

// ファイルへ読み込む
size_t LCppStdFile::Write( const void * buf, size_t bytes )
{
	assert( m_nOpenFlags & LDirectory::modeWriteFlag ) ;
	assert( m_fs != nullptr ) ;
	try
	{
		if ( m_fs != nullptr )
		{
			m_fs->write( reinterpret_cast<const char*>( buf ), bytes ) ;
			if ( m_fs->good() )
			{
				return	bytes ;
			}
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::Write : %s\n", e.what() ) ;
	}
	return	0 ;
}

// 書き出しを確定する
void LCppStdFile::Flush( void )
{
	assert( m_fs != nullptr ) ;
	try
	{
		if ( m_fs != nullptr )
		{
			m_fs->flush() ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::Flush : %s\n", e.what() ) ;
	}
}

// シークする
void LCppStdFile::Seek( std::int64_t pos )
{
	try
	{
		if ( m_fs != nullptr )
		{
			m_fs->seekp( (std::streampos) pos ) ;
			m_fs->seekg( (std::streampos) pos ) ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::Seek : %s\n", e.what() ) ;
	}
}

void LCppStdFile::Skip( std::int64_t bytes )
{
	try
	{
		if ( m_fs != nullptr )
		{
			m_fs->seekg( (std::streampos) bytes, std::ios_base::cur ) ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::Skip : %s\n", e.what() ) ;
	}
}

// 現在の位置を取得する (ストリームの場合は -1)
std::int64_t LCppStdFile::GetPosition( void ) const
{
	try
	{
		if ( m_fs != nullptr )
		{
			std::int64_t	pos = m_fs->tellg() ;
			if ( m_fs->good() )
			{
				return	pos ;
			}
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::GetPosition : %s\n", e.what() ) ;
	}
	return	-1 ;
}

// ファイル全長 (ストリームの場合は -1)
std::int64_t LCppStdFile::GetLength( void ) const
{
	try
	{
		if ( m_fs != nullptr )
		{
			std::int64_t	pos = m_fs->tellg() ;
			if ( m_fs->good() )
			{
				m_fs->seekg( 0, std::ios_base::end ) ;
				if ( m_fs->good() )
				{
					std::int64_t	len = m_fs->tellg() ;

					m_fs->seekg( pos ) ;
					if ( m_fs->good() )
					{
						return	len ;
					}
				}
			}
			return	-1 ;
		}
	}
	catch ( const std::exception& e )
	{
		LTrace( "exception at LCppStdFile::GetLength : %s\n", e.what() ) ;
	}
	return	-1 ;
}

// 現在の位置にファイルを切り詰める
void LCppStdFile::Truncate( void )
{
	assert( m_nOpenFlags & LDirectory::modeWriteFlag ) ;
	if ( (m_fs != nullptr)
		&& (m_dir != nullptr)
		&& !m_path.IsEmpty()
		&& (m_nOpenFlags & LDirectory::modeWriteFlag) )
	{
		std::int64_t	pos = GetPosition() ;
		m_fs = nullptr ;

		m_dir->Turncate( m_path.c_str(), pos ) ;

		m_fs = m_dir->OpenFileStream
			( m_path.c_str(),
				m_nOpenFlags & ~(LDirectory::modeCreateFlag
								| LDirectory::modeCreateDirsFlag) ) ;
	}
}

// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
LString LCppStdFile::GetFilePath( void ) const
{
	return	m_path ;
}

// ディレクトリを取得する
//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
LDirectory * LCppStdFile::GetDirectory( void )
{
	return	m_dir ;
}

#endif


//////////////////////////////////////////////////////////////////////////////
// POSIX/Windows 標準のファイルシステム
//////////////////////////////////////////////////////////////////////////////

// ファイルを開く
LFilePtr LStdFileDirectory::OpenFile
	( const wchar_t * pwszPath, long nOpenFlags )
{
	std::shared_ptr<LStdFile>	file = std::make_shared<LStdFile>( this ) ;
	if ( file->Open( pwszPath, nOpenFlags ) )
	{
		return	file ;
	}
	return	nullptr ;
}

// 可能なら同等のディレクトリを複製する
std::shared_ptr<LDirectory> LStdFileDirectory::Duplicate( void )
{
	return	std::make_shared<LStdFileDirectory>( *this ) ;
}

// ファイル情報取得
bool LStdFileDirectory::QueryFileState( State& state, const wchar_t * pwszPath )
{
	return	LStdFile::QueryFileState( state, pwszPath ) ;
}

// ファイル（サブディレクトリ含む）列挙
// ※ files へは以前のデータを削除せずに追加
void LStdFileDirectory::ListFiles
	( std::vector<LString>& files, const wchar_t * pwszSubDirPath )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	LString	strFiles = LURLSchemer::SubPath( pwszSubDirPath, L"*.*" ) ;

	WIN32_FIND_DATAW	wfd ;
	HANDLE	hFind = ::FindFirstFileW( strFiles.c_str(), &wfd ) ;
	if ( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			files.push_back( LString( wfd.cFileName ) ) ;
		}
		while ( ::FindNextFileW( hFind, &wfd ) ) ;
		::FindClose( hFind ) ;
	}

#else
	LString	strDirPath = pwszSubDirPath ;
	strDirPath = strDirPath.Replace( L"\\", L"/" ) ;

	DIR *	dir = opendir( strDirPath.ToString().c_str() ) ;
	if ( dir != nullptr )
	{
		for ( ; ; )
		{
			dirent*	dent = readdir( dir ) ;
			if ( dent != nullptr )
			{
				files.push_back( LString( dent->d_name ) ) ;
			}
			else
			{
				break ;
			}
		}
		closedir( dir ) ;
	}
#endif
}

// ファイル削除
bool LStdFileDirectory::DeleteFile( const wchar_t * pwszPath )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	return	::DeleteFileW( pwszPath ) ;

#else
	LString	strPath = pwszPath ;
	strPath = strPath.Replace( L"\\", L"/" ) ;

	return	(remove( strPath.ToString().c_str() ) == 0) ;
#endif
}

// ファイル名変更
bool LStdFileDirectory::RenameFile
	( const wchar_t * pwszOldPath, const wchar_t * pwszNewPath )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	return	::MoveFileW( pwszOldPath, pwszNewPath ) ;

#else
	LString	strOldPath = pwszOldPath ;
	LString	strNewPath = pwszNewPath ;
	strOldPath = strOldPath.Replace( L"\\", L"/" ) ;
	strNewPath = strNewPath.Replace( L"\\", L"/" ) ;

	return	(rename( strOldPath.ToString().c_str(),
						strNewPath.ToString().c_str() ) == 0) ;
#endif
}

// サブディレクトリ作成
bool LStdFileDirectory::CreateDirectory( const wchar_t * pwszPath )
{
	return	LStdFile::CreateDirectories( pwszPath ) ;
}

// サブディレクトリ削除
bool LStdFileDirectory::DeleteDirectory( const wchar_t * pwszPath )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	return	::RemoveDirectoryW( pwszPath ) ;

#else
	LString	strPath = pwszPath ;
	strPath = strPath.Replace( L"\\", L"/" ) ;

	return	(rmdir( strPath.ToString().c_str() ) == 0) ;
#endif
}



//////////////////////////////////////////////////////////////////////////////
// POSIX/Windows 標準のファイル
//////////////////////////////////////////////////////////////////////////////

LStdFile::LStdFile( LDirectory * dir )
	: m_file( InvalidFile ),
		m_nOpenFlags( 0 ), m_dir( dir )
{
}

LStdFile::~LStdFile( void )
{
	if ( m_file != InvalidFile )
	{
		Close() ;
	}
}

// ファイルを開く
bool LStdFile::Open( const wchar_t * pwszPath, long nOpenFlags )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	// フラグを変換
	DWORD	dwAccess = 0 ;
	DWORD	dwShareMode = 0 ;
	DWORD	dwCreate = OPEN_EXISTING ;
	if ( nOpenFlags & LDirectory::modeCreateFlag )
	{
		dwCreate = CREATE_ALWAYS ;
	}
	if ( nOpenFlags & LDirectory::modeReadFlag )
	{
		dwAccess |= GENERIC_READ | FILE_SHARE_READ ;
	}
	if ( nOpenFlags & LDirectory::modeWriteFlag )
	{
		dwAccess |= GENERIC_WRITE | FILE_SHARE_WRITE ;
	}

	// ファイルを開く
	m_path = pwszPath ;
	m_nOpenFlags = nOpenFlags ;

	if ( (nOpenFlags & (LDirectory::modeCreateFlag | LDirectory::modeCreateDirsFlag))
					== (LDirectory::modeCreateFlag | LDirectory::modeCreateDirsFlag) )
	{
		CreateDirectories
			( LURLSchemer::GetDirectoryOf( m_path.c_str() ).c_str() ) ;
	}

	UINT	nErrorMode = ::SetErrorMode( SEM_FAILCRITICALERRORS ) ;

	m_file = ::CreateFileW
		( pwszPath, dwAccess, dwShareMode,
			nullptr, dwCreate, FILE_ATTRIBUTE_NORMAL, nullptr ) ;

	::SetErrorMode( nErrorMode ) ;

	if ( m_file == InvalidFile )
	{
		return	false ;
	}

#else
	int			flags = 0 ;
	mode_t		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;
	long int	nPermission =
					(nOpenFlags & LDirectory::modePermissionMask) ;
	if ( nPermission != 0 )
	{
		mode = (mode_t) nPermission ;
	}
	if ( nOpenFlags & LDirectory::modeCreateFlag )
	{
		flags |= O_CREAT ;
	}
	if ( (nOpenFlags & LDirectory::modeReadWrite) == LDirectory::modeReadWrite )
	{
		flags |= O_RDWR ;
	}
	else if ( nOpenFlags & LDirectory::modeWriteFlag )
	{
		flags |= O_WRONLY ;
	}
	else
	{
		flags |= O_RDONLY ;
	}
	m_path = pwszPath ;
	m_path = m_path.Replace( L"\\", L"/" ) ;
	m_nOpenFlags = nOpenFlags ;
	//
	if ( (nOpenFlags & (LDirectory::modeCreateFlag | LDirectory::modeCreateDirsFlag))
					== (LDirectory::modeCreateFlag | LDirectory::modeCreateDirsFlag) )
	{
		CreateDirectories
			( LURLSchemer::GetDirectoryOf( m_path.c_str() ).c_str() ) ;
	}
	m_file = open( m_path.ToString().c_str(), flags, mode ) ;
	if ( m_file == InvalidFile )
	{
		// ファイルのオープンに失敗
		LTrace( "failed to open \'%s\' %03X (error=%d, %s)\n",
				m_path.ToString().c_str(), mode, errno, strerror(errno) ) ;
		return	false ;
	}
	if ( nOpenFlags & LDirectory::modeCreateFlag )
	{
		// ※ Android では以前のファイルは残ったままになるので
		ftruncate( m_file, 0 ) ;
	}
#endif
	return	true ;
}

// ファイルを閉じる
void LStdFile::Close( void )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		::CloseHandle( m_file ) ;
		m_file = InvalidFile ;
	}

#else
	if ( m_file != InvalidFile )
	{
		close( m_file ) ;
		m_file = InvalidFile ;
	}
#endif
}

// ファイルから読み込む
size_t LStdFile::Read( void * buf, size_t bytes )
{
	assert( m_file != InvalidFile ) ;
	assert( m_nOpenFlags & LDirectory::modeReadFlag ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		DWORD	dwReadBytes = 0 ;
		::ReadFile( m_file, buf, (DWORD) bytes, &dwReadBytes, nullptr ) ;
		return	dwReadBytes ;
	}

#else
	if ( m_file != InvalidFile )
	{
		return	read( m_file, buf, bytes ) ;
	}
#endif
	return	0 ;
}

// ファイルへ読み込む
size_t LStdFile::Write( const void * buf, size_t bytes )
{
	assert( m_file != InvalidFile ) ;
	assert( m_nOpenFlags & LDirectory::modeWriteFlag ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		DWORD	dwWrittenBytes = 0 ;
		::WriteFile( m_file, buf, (DWORD) bytes, &dwWrittenBytes, nullptr ) ;
		return	dwWrittenBytes ;
	}

#else
	if ( m_file != InvalidFile )
	{
		return	write( m_file, buf, bytes ) ;
	}
#endif
	return	0 ;
}

// 書き出しを確定する
void LStdFile::Flush( void )
{
}

// シークする
void LStdFile::Seek( std::int64_t pos )
{
	assert( m_file != InvalidFile ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		LONG	nHigh = (LONG) (pos >> 32) ;
		DWORD	dwHigh = 0 ;

		SetLastError( NO_ERROR ) ;
		::SetFilePointer( m_file, (DWORD) pos, &nHigh, FILE_BEGIN ) ;
	}

#else
	if ( m_file != InvalidFile )
	{
		lseek64( m_file, pos, SEEK_SET ) ;
	}
#endif
}

void LStdFile::Skip( std::int64_t bytes )
{
	assert( m_file != InvalidFile ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		LONG	nHigh = (LONG) (bytes >> 32) ;
		DWORD	dwHigh = 0 ;

		SetLastError( NO_ERROR ) ;
		::SetFilePointer( m_file, (DWORD) bytes, &nHigh, FILE_CURRENT ) ;
	}

#else
	if ( m_file != InvalidFile )
	{
		lseek64( m_file, bytes, SEEK_CUR ) ;
	}
#endif
}

// 現在の位置を取得する (ストリームの場合は -1)
std::int64_t LStdFile::GetPosition( void ) const
{
	assert( m_file != InvalidFile ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		LONG	nHigh = 0 ;
		DWORD	dwNewPos ;
		SetLastError( NO_ERROR ) ;
		dwNewPos = ::SetFilePointer( m_file, 0, &nHigh, FILE_CURRENT ) ;
		if ( (dwNewPos == INVALID_SET_FILE_POINTER)
						&& GetLastError() != NO_ERROR )
		{
			nHigh = 0 ;
			dwNewPos = 0 ;
		}
		return	(((int64_t) nHigh) << 32) | dwNewPos ;
	}

#else
	if ( m_file != InvalidFile )
	{
		off64_t	offCur = lseek64( m_file, 0, SEEK_CUR ) ;
		if ( offCur != -1 )
		{
			return	offCur ;
		}
	}
#endif
	return	0 ;
}

// ファイル全長 (ストリームの場合は -1)
std::int64_t LStdFile::GetLength( void ) const
{
	assert( m_file != InvalidFile ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		SetLastError( NO_ERROR ) ;
		DWORD	dwHigh = 0 ;
		DWORD	dwSize = ::GetFileSize( m_file, &dwHigh ) ;
		if ( (dwSize == (DWORD) -1) && (GetLastError() != NO_ERROR) )
		{
			dwSize = 0 ;
		}
		return	(((int64_t) dwHigh) << 32) | dwSize ;
	}

#else
	if ( m_file != InvalidFile )
	{
		off64_t	offCur = lseek64( m_file, 0, SEEK_CUR ) ;
		if ( offCur != -1 )
		{
			off64_t	offEnd = lseek64( m_file, 0, SEEK_END ) ;
			lseek64( m_file, offCur, SEEK_SET ) ;
			//
			if ( offEnd != -1 )
			{
				return	offEnd ;
			}
		}
	}
#endif
	return	0 ;
}

// 現在の位置にファイルを切り詰める
void LStdFile::Truncate( void )
{
	assert( m_file != InvalidFile ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( m_file != InvalidFile )
	{
		::SetEndOfFile( m_file ) ;
	}

#else
	if ( m_file != InvalidFile )
	{
		ftruncate( m_file, GetPosition() ) ;
	}
#endif
}

// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
LString LStdFile::GetFilePath( void ) const
{
	return	m_path ;
}

// ディレクトリを取得する
//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
LDirectory * LStdFile::GetDirectory( void )
{
	return	m_dir ;
}

// ファイルが存在するか？
bool LStdFile::IsFileExisting( const wchar_t * pwszPath )
{
	LDirectory::State	state ;
	return	LStdFile::QueryFileState( state, pwszPath ) ;
}

// ファイル情報取得
bool LStdFile::QueryFileState
		( LDirectory::State& state, const wchar_t * pwszPath )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	WIN32_FIND_DATAW	wfd ;
	HANDLE	hFind = ::FindFirstFileW( pwszPath, &wfd ) ;
	if ( hFind == INVALID_HANDLE_VALUE )
	{
		return	false ;
	}
	state.fields = LDirectory::stateHasAttribute
					| LDirectory::stateHasFileSize ;
	state.attributes = LDirectory::permissionRUSR
							| LDirectory::permissionWUSR
							| LDirectory::permissionXUSR ;
	if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		state.attributes |= LDirectory::attrDirectory ;
	}
	if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )
	{
		state.attributes |= LDirectory::attrHidden ;
	}
	state.fileSizeInBytes =
		wfd.nFileSizeLow
			| (((std::uint64_t)wfd.nFileSizeHigh) << 32) ;

	FILETIME	ftLocalTime ;
	SYSTEMTIME	stLocalTime ;
	if ( ::FileTimeToLocalFileTime
				( &wfd.ftLastWriteTime, &ftLocalTime )
		&& ::FileTimeToSystemTime( &ftLocalTime, &stLocalTime ) )
	{
		state.fields |= LDirectory::stateHasModifiedDate ;
		state.dateModified.year = stLocalTime.wYear ;
		state.dateModified.month = (LUint8) stLocalTime.wMonth ;
		state.dateModified.day = (LUint8) stLocalTime.wDay ;
		state.dateModified.week = (LUint8) stLocalTime.wDayOfWeek ;
		state.dateModified.hour = (LUint8) stLocalTime.wHour ;
		state.dateModified.minute = (LUint8) stLocalTime.wMinute ;
		state.dateModified.second = (LUint8) stLocalTime.wSecond ;
		state.dateModified.msec = stLocalTime.wMilliseconds ;
		state.dateModified.usec = 0 ;
	}
	if ( ::FileTimeToLocalFileTime
				( &wfd.ftCreationTime, &ftLocalTime )
		&& ::FileTimeToSystemTime( &ftLocalTime, &stLocalTime ) )
	{
		state.fields |= LDirectory::stateHasCreatedDate ;
		state.dateCreated.year = stLocalTime.wYear ;
		state.dateCreated.month = (LUint8) stLocalTime.wMonth ;
		state.dateCreated.day = (LUint8) stLocalTime.wDay ;
		state.dateCreated.week = (LUint8) stLocalTime.wDayOfWeek ;
		state.dateCreated.hour = (LUint8) stLocalTime.wHour ;
		state.dateCreated.minute = (LUint8) stLocalTime.wMinute ;
		state.dateCreated.second = (LUint8) stLocalTime.wSecond ;
		state.dateCreated.msec = stLocalTime.wMilliseconds ;
		state.dateCreated.usec = 0 ;
	}
	if ( ::FileTimeToLocalFileTime
				( &wfd.ftLastAccessTime, &ftLocalTime )
		&& ::FileTimeToSystemTime( &ftLocalTime, &stLocalTime ) )
	{
		state.fields |= LDirectory::stateHasAccessedDate ;
		state.dateAccessed.year = stLocalTime.wYear ;
		state.dateAccessed.month = (LUint8) stLocalTime.wMonth ;
		state.dateAccessed.day = (LUint8) stLocalTime.wDay ;
		state.dateAccessed.week = (LUint8) stLocalTime.wDayOfWeek ;
		state.dateAccessed.hour = (LUint8) stLocalTime.wHour ;
		state.dateAccessed.minute = (LUint8) stLocalTime.wMinute ;
		state.dateAccessed.second = (LUint8) stLocalTime.wSecond ;
		state.dateAccessed.msec = stLocalTime.wMilliseconds ;
		state.dateAccessed.usec = 0 ;
	}
	::FindClose( hFind ) ;
	return	true ;

#else
	LString	strPath = pwszPath ;
	strPath = strPath.Replace( L"\\", L"/" ) ;

	struct stat	st ;
	if ( stat( strPath.ToString().c_str(), &st ) != 0 )
	{
		return	false ;
	}
	state.fields = LDirectory::stateHasAttribute
					| LDirectory::stateHasFileSize ;
	state.attributes = 0 ;
	if ( S_ISDIR(st.st_mode) )
	{
		state.attributes |= LDirectory::attrDirectory ;
	}
	if ( st.st_mode & S_IRUSR )
	{
		state.attributes |= LDirectory::permissionRUSR ;
	}
	if ( st.st_mode & S_IWUSR )
	{
		state.attributes |= LDirectory::permissionWUSR ;
	}
	if ( st.st_mode & S_IXUSR )
	{
		state.attributes |= LDirectory::permissionXUSR ;
	}
	if ( st.st_mode & S_IRGRP )
	{
		state.attributes |= LDirectory::permissionRGRP ;
	}
	if ( st.st_mode & S_IWGRP )
	{
		state.attributes |= LDirectory::permissionWGRP ;
	}
	if ( st.st_mode & S_IXGRP )
	{
		state.attributes |= LDirectory::permissionXGRP ;
	}
	if ( st.st_mode & S_IROTH )
	{
		state.attributes |= LDirectory::permissionROTH ;
	}
	if ( st.st_mode & S_IWOTH )
	{
		state.attributes |= LDirectory::permissionWOTH ;
	}
	if ( st.st_mode & S_IXOTH )
	{
		state.attributes |= LDirectory::permissionXOTH ;
	}
	state.fileSizeInBytes = st.st_size ;
	//
	tm		tmLocal ;
	tm *	ptmLocal ;
	ptmLocal = localtime_r
		( (const time_t*) &st.st_mtime, &tmLocal ) ;
	if ( ptmLocal != nullptr )
	{
		state.fields |= LDirectory::stateHasModifiedDate ;
		state.dateModified.SetTm( ptmLocal ) ;
	}
	ptmLocal = localtime_r
		( (const time_t*) &st.st_ctime, &tmLocal ) ;
	if ( ptmLocal != nullptr )
	{
		state.fields |= LDirectory::stateHasCreatedDate ;
		state.dateCreated.SetTm( ptmLocal ) ;
	}
	ptmLocal = localtime_r
		( (const time_t*) &st.st_atime, &tmLocal ) ;
	if ( ptmLocal != nullptr )
	{
		state.fields |= LDirectory::stateHasAccessedDate ;
		state.dateAccessed.SetTm( ptmLocal ) ;
	}
	return	true ;
#endif
}

// ディレクトリ作成
bool LStdFile::CreateDirectories( const wchar_t * pwszDirPath )
{
	size_t	pos = 0 ;
	ssize_t	iDir = -1 ;
	for ( ; ; )
	{
		iDir = LURLSchemer::FindDirectoryPathOf( pwszDirPath, pos ) ;
		if ( iDir < 0 )
		{
			break ;
		}
		if ( (iDir >= 1)
			&& (pwszDirPath[iDir - 1] != L':')
			&& (pwszDirPath[iDir - 1] != L'\\')
			&& (pwszDirPath[iDir - 1] != L'/') )
		{
			LString	strDir( pwszDirPath, iDir ) ;
			if ( !IsFileExisting( strDir.c_str() ) )
			{
				LStdFile::CreateDirectory( strDir.c_str() ) ;
			}
		}
		pos = iDir + 1 ;
	}
	return	LStdFile::CreateDirectory( pwszDirPath ) ;
}

bool LStdFile::CreateDirectory( const wchar_t * pwszDirPath )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	return	::CreateDirectoryW( pwszDirPath, nullptr ) ;

#else
	mode_t	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ;
	int		err = mkdir( LString(pwszDirPath).ToString().c_str(), mode ) ;
	return	(err == 0) ;

#endif
}



//////////////////////////////////////////////////////////////////////////////
// バッファされたファイル
//////////////////////////////////////////////////////////////////////////////

LBufferedFile::LBufferedFile
	( long nOpenFlags,
		std::shared_ptr<LArrayBufStorage> buf,
		LDirectory * dir, const wchar_t * pwszPath,
		LFilePtr fileWriteBack )
	: m_buf( buf ), m_pos( 0 ), m_written( false ),
		m_dir( dir ), m_path( pwszPath ),
		m_fileWriteBack( fileWriteBack ),
		m_nOpenFlags( nOpenFlags )
{
	if ( m_buf == nullptr )
	{
		m_buf = std::make_shared<LArrayBufStorage>() ;
	}
}

LBufferedFile::~LBufferedFile( void )
{
	if ( m_written && (m_fileWriteBack != nullptr) )
	{
		m_fileWriteBack->Seek( 0 ) ;
		m_fileWriteBack->Write( m_buf->GetBuffer(), m_buf->GetBytes() ) ;
		m_fileWriteBack->Truncate() ;
	}
}

// バッファにファイルから読み込む
void LBufferedFile::LoadFileFrom( LFilePtr file )
{
	LoadFileFrom( m_buf, file ) ;
}

void LBufferedFile::LoadFileFrom
	( std::shared_ptr<LArrayBufStorage> buf, LFilePtr file )
{
	assert( file != nullptr ) ;
	if ( file == nullptr )
	{
		return ;
	}
	std::int64_t	nFileLen = file->GetLength() ;
	size_t			nBufBytes = (size_t) nFileLen ;
	if ( nFileLen < 0 )
	{
		nBufBytes = 0x400 ;
	}
	buf->resize( nBufBytes ) ;

	bool	flagEOF = false ;
	size_t	nTotalReadBytes = 0 ;
	while ( !flagEOF )
	{
		while ( nTotalReadBytes < nBufBytes )
		{
			size_t	nReadBytes =
				file->Read( buf->GetBuffer() + nTotalReadBytes,
							nBufBytes - nTotalReadBytes ) ;
			if ( nReadBytes == 0 )
			{
				// ファイルの終端
				flagEOF = true ;
				break ;
			}
			nTotalReadBytes += nReadBytes ;
		}
		if ( !flagEOF && (nFileLen < 0) )
		{
			// ストリームの場合はバッファを拡張する
			nBufBytes *= 2 ;
			buf->ResizeBuffer( nBufBytes ) ;
		}
		else
		{
			break ;
		}
	}
	buf->ResizeBuffer( nTotalReadBytes ) ;
}

// ファイルから読み込む
size_t LBufferedFile::Read( void * buf, size_t bytes )
{
	assert( m_nOpenFlags & LDirectory::modeReadFlag ) ;
	if ( (m_nOpenFlags & LDirectory::modeReadFlag)
		&& (m_pos < m_buf->GetBytes()) )
	{
		bytes = std::min( m_buf->GetBytes() - m_pos, bytes ) ;
		memcpy( buf, m_buf->GetBuffer() + m_pos, bytes ) ;
		m_pos += bytes ;
		return	bytes ;
	}
	return	0 ;
}

// ファイルへ読み込む
size_t LBufferedFile::Write( const void * buf, size_t bytes )
{
	assert( m_nOpenFlags & LDirectory::modeWriteFlag ) ;
	if ( m_nOpenFlags & LDirectory::modeWriteFlag )
	{
		if ( m_pos + bytes > m_buf->GetCapacityBytes() )
		{
			// バッファ許容量延長
			size_t	nCapBytes = m_buf->GetCapacityBytes() ;
			while ( nCapBytes >= m_pos + bytes )
			{
				nCapBytes *= 2 ;
			}
			m_buf->ReserveBuffer( nCapBytes ) ;
		}
		if ( m_pos + bytes > m_buf->GetBytes() )
		{
			// サイズ拡張
			m_buf->ResizeBuffer( m_pos + bytes ) ;
		}

		bytes = std::min( m_buf->GetBytes() - m_pos, bytes ) ;
		memcpy( m_buf->GetBuffer() + m_pos, buf, bytes ) ;
		m_pos += bytes ;
		m_written = true ;
		return	bytes ;
	}
	return	0 ;
}

// 書き出しを確定する
void LBufferedFile::Flush( void )
{
}

// シークする
void LBufferedFile::Seek( std::int64_t pos )
{
	m_pos = (size_t) pos ;
}

void LBufferedFile::Skip( std::int64_t bytes )
{
	if ( (bytes >= 0) || ((size_t) -bytes <= m_pos) )
	{
		m_pos += (ssize_t) bytes ;
	}
	else
	{
		m_pos = 0 ;
	}
}

// シーク可能か？
bool LBufferedFile::IsSeekable( void ) const
{
	return	true ;
}

// 現在の位置を取得する (ストリームの場合は -1)
std::int64_t LBufferedFile::GetPosition( void ) const
{
	return	m_pos ;
}

// ファイル全長 (ストリームの場合は -1)
std::int64_t LBufferedFile::GetLength( void ) const
{
	return	m_buf->GetBytes() ;
}

// 現在の位置にファイルを切り詰める
void LBufferedFile::Truncate( void )
{
	m_buf->ResizeBuffer( m_pos ) ;
}

// ファイルパスを取得する（大抵は開いたときのファイルパスで絶対パスではない）
LString LBufferedFile::GetFilePath( void ) const
{
	return	m_path ;
}

// ディレクトリを取得する
//（可能なら GetFilePath() で取得したパスで同じファイルが開けるようにする）
LDirectory * LBufferedFile::GetDirectory( void )
{
	return	m_dir ;
}
