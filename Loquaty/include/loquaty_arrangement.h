
#ifndef	__LOQUATY_ARRANGEMENT_H__
#define	__LOQUATY_ARRANGEMENT_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// フラットなバッファにおけるローカル名前空間情報
	//////////////////////////////////////////////////////////////////////////

	class	LArrangement	: public Object
	{
	public:
		class	Desc
		{
		public:
			LType				m_type ;		// 型
			size_t				m_elements ;	// 配列数
			size_t				m_location ;	// バイト単位での位置
			size_t				m_size ;		// バイト単位での確保サイズ

		public:
			Desc( void )
				: m_elements( 1 ), m_location( 0 ), m_size( 1 ) {}
			Desc( const LType& type, size_t elements = 1 )
				: m_type( type ), m_elements( elements ),
					m_location( 0 ), m_size( 1 ) {}
			Desc( const Desc& desc )
				: m_type( desc.m_type ), m_elements( desc.m_elements ),
					m_location( desc.m_location ),
					m_size( desc.m_size ) {}
			const Desc& operator = ( const Desc& desc )
			{
				m_type = desc.m_type ;
				m_elements = desc.m_elements ;
				m_location = desc.m_location ;
				m_size = desc.m_size ;
				return	*this ;
			}
		} ;

	protected:
		std::map<std::wstring, Desc>	m_mapNames ;

		size_t	m_nAlign ;		// ベース整列幅
								//（Stack の場合は sizeof(LValue::Primitive), 構造体は 1）
		size_t	m_nMaxAlign ;	// 要素の最大アライメント

		size_t	m_nAllocated ;	// 確保済みサイズ

	public:
		LArrangement( size_t nAlign )
			: m_nAlign( nAlign ), m_nMaxAlign( nAlign ), m_nAllocated( 0 ) { }
		LArrangement( const LArrangement& arrange )
			: m_mapNames( arrange.m_mapNames ),
				m_nAlign( arrange.m_nAlign ),
				m_nMaxAlign( arrange.m_nMaxAlign ),
				m_nAllocated( arrange.m_nAllocated ) { }

		const LArrangement& operator = ( const LArrangement& arrange )
		{
			m_mapNames = arrange.m_mapNames ;
			m_nAlign = arrange.m_nAlign ;
			m_nMaxAlign = arrange.m_nMaxAlign ;
			m_nAllocated = arrange.m_nAllocated ;
			return	*this ;
		}

		// すべて削除
		void ClearAll( void )
		{
			m_mapNames.clear() ;
			m_nAllocated = 0 ;
		}

		// 整列した確保済みサイズを取得
		size_t GetAlignedSize( void ) const
		{
			return	(m_nAllocated + m_nAlign - 1) / m_nAlign ;
		}
		size_t GetAlignedBytes( void ) const
		{
			return	CalcAlign( GetAlignedSize() * m_nAlign, GetArrangeAlign() ) ;
		}

		// 配置アライメント取得
		size_t GetArrangeAlign( void ) const
		{
			return	m_nMaxAlign ;
		}

		// 次に確保する位置をアライメントする
		void MakeAlignment( size_t nAlign )
		{
			m_nAllocated = CalcAlign( m_nAllocated, nAlign ) ;
		}
		static size_t CalcAlign( size_t nLoc, size_t nAlign )
		{
			assert( nAlign != 0 ) ;
			return	((nLoc + nAlign - 1) / nAlign) * nAlign ;
		}

		// 新しい要素を追加配置
		void AddDesc( const wchar_t * name, const LType& type, size_t elements = 1 )
		{
			Desc			desc( type, elements ) ;
			const size_t	nAlign = type.GetAlignBytes() ;
			const size_t	nBytes = type.GetDataBytes() ;
			MakeAlignment( std::max( nAlign, m_nAlign ) ) ;
			//
			desc.m_location = m_nAllocated ;
			desc.m_size = desc.m_elements * nBytes ;
			//
			m_nMaxAlign = std::max( m_nMaxAlign, nAlign ) ;
			m_nAllocated = desc.m_location + desc.m_size ;
			//
			m_mapNames.insert
				( std::make_pair<std::wstring,Desc>( name, Desc(desc) ) ) ;
		}

		// 要素の有無を確認
		bool IsExistingAs( const wchar_t * name ) const
		{
			return	(m_mapNames.find( name ) != m_mapNames.end()) ;
		}

		// 要素を取得
		bool GetDescAs( Desc& desc, const wchar_t * name ) const
		{
			auto	iter = m_mapNames.find( name ) ;
			if ( iter == m_mapNames.end() )
			{
				return	false ;
			}
			desc = iter->second ;
			return	true ;
		}

		// 要素名リストを取得
		void GetOrderedNameList( std::vector<LString>& names ) const
		{
			size_t	iNext = 0 ;
			for ( ; ; )
			{
				const std::wstring *	pName = nullptr ;
				const Desc *			pDesc = nullptr ;
				size_t					iMin = m_nAllocated ;
				for ( auto iter = m_mapNames.begin(); iter != m_mapNames.end(); iter ++ )
				{
					if ( (iter->second.m_location < iMin)
						&& (iNext <= iter->second.m_location) )
					{
						iMin = iter->second.m_location ;
						pName = &(iter->first) ;
						pDesc = &(iter->second) ;
					}
				}
				if ( pName == nullptr )
				{
					break ;
				}
				names.push_back( *pName ) ;
				iNext = pDesc->m_location + pDesc->m_size ;
			}
		}

		// マージ
		std::map< std::wstring, Desc >
				MergeFrom( const LArrangement& arrange )
		{
			std::map< std::wstring, Desc >	mapDoubled ;
			MakeAlignment( arrange.GetArrangeAlign() ) ;
			const size_t	nBaseLoc = m_nAllocated ;
			for ( auto iter = arrange.m_mapNames.begin();
						iter != arrange.m_mapNames.end(); iter ++ )
			{
				Desc	desc = iter->second ;
				desc.m_location += nBaseLoc ;
				//
				auto	iExist = m_mapNames.find( iter->first ) ;
				if ( iExist == m_mapNames.end() )
				{
					m_mapNames.insert
						( std::make_pair<std::wstring,Desc>
							( iter->first.c_str(), Desc(desc) ) ) ;
				}
				else
				{
					mapDoubled.insert
						( std::make_pair<std::wstring,Desc>
							( iter->first.c_str(), Desc(iExist->second) ) ) ;
					iExist->second = desc ;
				}
			}
			m_nMaxAlign = std::max( m_nMaxAlign, arrange.m_nMaxAlign ) ;
			m_nAllocated += arrange.GetAlignedBytes() ;
			return	mapDoubled ;
		}

		// private 要素を不可視に変更する
		void MakePrivateInvisible( void )
		{
			for ( auto iter = m_mapNames.begin(); iter != m_mapNames.end(); iter ++ )
			{
				iter->second.m_type.MakePrivateInvisible() ;
			}
		}

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 構造情報付きバッファ
	//////////////////////////////////////////////////////////////////////////

	class	LArrangementBuffer	: public LArrangement
	{
	public:
		std::shared_ptr<LArrayBufStorage>	m_pBuf ;

	public:
		LArrangementBuffer( size_t nAlign )
			: LArrangement( nAlign ) {}
		LArrangementBuffer( const LArrangementBuffer& arrangeBuf )
			: LArrangement( arrangeBuf ), m_pBuf( arrangeBuf.m_pBuf ) {}
		LArrangementBuffer
			( const LArrangement& arrange,
				std::shared_ptr<LArrayBufStorage> pBuf = nullptr )
			: LArrangement( arrange ), m_pBuf( pBuf ) {}

	public:
		// バッファを取得する（バッファは必要に応じて拡張される）
		std::shared_ptr<LArrayBufStorage> GetBuffer( void )
		{
			if ( m_pBuf == nullptr )
			{
				m_pBuf = std::make_shared<LArrayBufStorage>( GetAlignedBytes() ) ;
			}
			else if ( m_pBuf->GetBytes() < GetAlignedBytes() )
			{
				m_pBuf->resize( GetAlignedBytes(), 0 ) ;
			}
			return	m_pBuf ;
		}

		// バッファを取得する
		std::shared_ptr<LArrayBufStorage> GetBuffer( void ) const
		{
			return	m_pBuf ;
		}

		// バッファを確保する
		// （既にバッファを持っている場合、必要に応じて拡張する）
		void AllocateBuffer( void )
		{
			GetBuffer() ;
		}

		// すべて削除
		void ClearAll( void )
		{
			LArrangement::ClearAll() ;
			m_pBuf = nullptr ;
		}

		// マージ
		std::map< std::wstring, Desc >
				MergeFrom( const LArrangementBuffer& arrange )
		{
			MakeAlignment( arrange.GetArrangeAlign() ) ;
			//
			std::shared_ptr<LArrayBufStorage>	pBuf = GetBuffer() ;
			std::shared_ptr<LArrayBufStorage>	pBufFrom = arrange.GetBuffer() ;
			if ( pBufFrom != nullptr )
			{
				assert( pBuf->GetBytes() == GetAlignedBytes() ) ;
				assert( pBufFrom->GetBytes() <= arrange.GetAlignedBytes() ) ;
				pBuf->AppendBuffer( *pBufFrom ) ;
			}
			return	LArrangement::MergeFrom( arrange ) ;
		}

	} ;

}

#endif

