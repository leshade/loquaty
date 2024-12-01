
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// テキストファイル
//////////////////////////////////////////////////////////////////////////////

// ファイルを読み込む
bool LTextFileParser::LoadTextFile
	( const wchar_t * pwszFile, LDirectory * pDirPath )
{
	if ( LoadTextFile( *this, pwszFile ) )
	{
		pDirPath = nullptr ;
	}
	else if ( (pDirPath == nullptr)
		|| !LoadTextFile( *this, pwszFile, pDirPath ) )
	{
		return	false ;
	}
	SeekIndex( 0 ) ;
	SetBounds( 0, SIZE_MAX ) ;

	LString	strDirPath = LURLSchemer::GetDirectoryOf( pwszFile ) ;
	if ( pDirPath != nullptr )
	{
		m_pDirectory = std::make_shared<LSubDirectory>
							( pDirPath->Duplicate(), strDirPath.c_str() ) ;
	}
	else
	{
		m_pDirectory = std::make_shared<LSubDirectory>
							( nullptr, strDirPath.c_str() ) ;
	}
	m_strFilePath = pwszFile ;
	return	true ;
}

bool LTextFileParser::LoadTextFile
	( LString& strText, const wchar_t * pwszFile, LDirectory * pDirPath )
{
	LFilePtr	file ;
	if ( pDirPath != nullptr )
	{
		file = pDirPath->OpenFile( pwszFile ) ;
	}
	else
	{
		file = LURLSchemer::Open( pwszFile ) ;
	}
	if ( file == nullptr )
	{
		return	false ;
	}
	return	ReadTextFile( strText, file ) ;
}

bool LTextFileParser::ReadTextFile( LString& strText, LFilePtr file )
{
	std::shared_ptr<LArrayBufStorage>
				buf = std::make_shared<LArrayBufStorage>() ;
	LBufferedFile::LoadFileFrom( buf, file ) ;

	if ( (buf->GetBytes() >= 3)
			&& (buf->at(0) == 0xEF)
			&& (buf->at(1) == 0xBB) && (buf->at(2) == 0xBF) )
	{
		// UTF-8
		buf->erase( buf->begin(), buf->begin() + 3 ) ;
		strText.FromUTF8( *buf ) ;
	}
	else if ( (buf->GetBytes() >= 2)
			&& (buf->at(0) == 0xFF) && (buf->at(1) == 0xFE) )
	{
		// UTF-16 LE
		std::vector<std::uint16_t>	utf16 ;
		const size_t	nLength = (buf->GetBytes() - 2)
										/ sizeof(std::uint16_t) ;
		utf16.resize( nLength ) ;
		memcpy( utf16.data(), buf->GetBuffer() + 2,
							nLength * sizeof(std::uint16_t) ) ;

		#ifdef	_LOQUATY_BIG_ENDIAN
		std::uint16_t *	pBuf = utf16.data() ;
		for ( size_t i = 0; i < nLength; i ++ )
		{
			pBuf[i] = (pBuf[i] >> 8) | (pBuf[i] << 8) ;
		}
		#endif

		strText.FromUTF16( utf16 ) ;
	}
	else
	{
		// デフォルト
		if ( !LString::IsValidUTF8( *buf )
			&& (sizeof(char) == sizeof(std::uint8_t)) )
		{
			std::string	str( (const char*) buf->GetBuffer(), buf->GetBytes() ) ;
			strText.FromString( str ) ;
		}
		else
		{
			strText.FromUTF8( *buf ) ;
		}
	}

	return	true ;
}

// この文字列を読み込んだ元のファイル名を取得する
LString LTextFileParser::GetFileName( void ) const
{
	return	m_strFilePath ;
}

// 開いたディレクトリを取得する
LDirectoryPtr LTextFileParser::GetFileDirectory( void ) const
{
	return	m_pDirectory ;
}



//////////////////////////////////////////////////////////////////////////////
// ソースコード
//////////////////////////////////////////////////////////////////////////////

// 空白文字を読み飛ばす（終端に到達した場合は false を返す）
bool LSourceFile::PassSpace( void )
{
	bool	hasComment = false ;
	while ( !IsEndOfString() )
	{
		wchar_t	wch = CurrentChar() ;
		if ( IsCharSpace( wch ) )
		{
			NextChar() ;

			if ( wch == L'\n' )
			{
				if ( !hasComment )
				{
					m_strComment = L"" ;
				}
			}
		}
		else if ( wch == L'/' )
		{
			if ( CurrentAt(1) == L'/' )
			{
				// コメントを読み飛ばす
				size_t	iComment = SkipIndex( 2 ) ;
				while ( CurrentChar() == L'/' )
				{
					iComment = SkipIndex( 1 ) ;
				}
				if ( (CurrentChar() == L' ') || (CurrentChar() == L'\t') )
				{
					iComment = SkipIndex( 1 ) ;
				}
				bool	flagEmptyLine = ((CurrentChar() == L'\r')
										|| (CurrentChar() == L'\n')) ;
				SeekString( L"\n" ) ;

				if ( !flagEmptyLine )
				{
					hasComment = true ;
					m_strComment += Middle( iComment, GetIndex() - iComment ) ;
				}
			}
			else if ( CurrentAt(1) == L'*' )
			{
				/* コメントを読み飛ばす */
				size_t	iComment = SkipIndex( 2 ) ;
				while ( (CurrentChar() == L'*') && (CurrentAt(1) != L'/') )
				{
					iComment = SkipIndex( 1 ) ;
				}
				size_t	iEnd = SeekString( L"*/" )
								? GetIndex() - 2 : GetIndex() ;
				hasComment = true ;

				LString	strComment = Middle( iComment, iEnd - iComment ) ;
				while ( strComment.GetBackAt(0) == L'*' )
				{
					strComment.ChopRight( 1 ) ;
				}
				m_strComment += strComment.GetTrimmed() ;
			}
			else
			{
				m_hasComment = hasComment ;
				return	true ;
			}
		}
		else
		{
			m_hasComment = hasComment ;
			return	true ;
		}
	}
	m_hasComment = hasComment ;
	return	false ;
}



//////////////////////////////////////////////////////////////////////////////
// ソースファイル・プロデューサー
//////////////////////////////////////////////////////////////////////////////

// ロード済みソース取得
LSourceFilePtr LSourceProducer::GetSourceFile( const wchar_t * pwszFile )
{
	LString	strFile = NormalizeFilePath( pwszFile ) ;
	auto	iter = m_mapSources.find( strFile.c_str() ) ;
	if ( iter == m_mapSources.end() )
	{
		return	nullptr ;
	}
	return	iter->second ;
}

// ソースを読み込んで取得
// 既にロード済みならそれを取得
LSourceFilePtr LSourceProducer::LoadSourceFile
		( const wchar_t * pwszFile, LDirectory * pDirPath )
{
	LSourceFilePtr	pSource = GetSourceFile( pwszFile ) ;
	if ( pSource != nullptr )
	{
		return	pSource ;
	}
	pSource = std::make_shared<LSourceFile>() ;
	if ( (pDirPath == nullptr)
		|| !pSource->LoadTextFile( pwszFile, pDirPath ) )
	{
		if ( !pSource->LoadTextFile( pwszFile, &m_dirPath ) )
		{
			return	nullptr ;
		}
	}
	LString	strFile = NormalizeFilePath( pwszFile ) ;
	m_mapSources.insert
		( std::make_pair<std::wstring,LSourceFilePtr>
			( strFile.c_str(), LSourceFilePtr(pSource) ) ) ;

	return	pSource ;
}

// 安全なソースを取得
LSourceFilePtr LSourceProducer::GetSafeSource
		( LStringParser * pParser, bool flagTemporary )
{
	for ( auto iter : m_mapSources )
	{
		if ( iter.second.get() == pParser )
		{
			// ロード済みソース
			return	iter.second ;
		}
	}

	auto	iter = m_mapSafeStock.find( pParser ) ;
	if ( iter != m_mapSafeStock.end() )
	{
		// 既に複製されたソース
		return	iter->second ;
	}

	// 複製
	LSourceFilePtr	pSource = std::make_shared<LSourceFile>( *pParser ) ;
	if ( !flagTemporary )
	{
		m_mapSafeStock.insert
			( std::make_pair<LStringParser*,LSourceFilePtr>
					( (LStringParser*) pParser, LSourceFilePtr( pSource ) ) ) ;
	}
	return	pSource ;
}

// 複製
const LSourceProducer&
	LSourceProducer::operator = ( const LSourceProducer& srcp )
{
	m_mapSources = srcp.m_mapSources ;
	m_mapSafeStock = srcp.m_mapSafeStock ;
	m_dirPath = srcp.m_dirPath ;
	return	*this ;
}

// ファイル名の正規化
LString LSourceProducer::NormalizeFilePath( const wchar_t * pwszFile )
{
	LString	strFile = pwszFile ;
	strFile.MakeLower() ;

	for ( size_t i = 0; i < strFile.GetLength(); i ++ )
	{
		if ( strFile.GetAt(i) == L'\\' )
		{
			strFile.SetAt( i, L'/' ) ;
		}
	}
	return	strFile ;
}



