
#ifndef	__LOQUATY_ARRAY_BUFFER_H__
#define	__LOQUATY_ARRAY_BUFFER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 抽象バッファ
	//////////////////////////////////////////////////////////////////////////

	class	LArrayBuffer	: public Object
	{
	private:
		LPointerObj *		m_pChainFirst ;
		LSpinLockMutex		m_mutex ;

	public:
		LArrayBuffer( void )
			: m_pChainFirst( nullptr ) {}

	public:
		// バッファへのポインタ取得
		virtual uint8_t * GetBuffer( void ) = 0 ;
		// バッファサイズ取得
		virtual size_t GetBytes( void ) const = 0 ;

	public:
		// バッファアドレスやサイズの変更を参照ポインタに通知する
		void NotifyBufferReallocation( void ) ;
		// 参照チェーンに追加する
		void AttachRefChain( LPointerObj * pPtr ) ;
		// 参照チェーンから取り外す
		void DetachRefChain( LPointerObj * pPtr ) ;

	} ;



	//////////////////////////////////////////////////////////////////////////
	// バッファ
	//////////////////////////////////////////////////////////////////////////

	class	LArrayBufStorage	: public LArrayBuffer,
									public std::vector<std::uint8_t>
	{
	public:
		LArrayBufStorage( void ) {}
		LArrayBufStorage( size_t nBytes )
			: std::vector<std::uint8_t>( nBytes, 0 ) {}
		LArrayBufStorage( const LArrayBufStorage& buf )
			: std::vector<std::uint8_t>( buf ) {}
		LArrayBufStorage( const std::vector<std::uint8_t>& buf )
			: std::vector<std::uint8_t>( buf ) {}
		LArrayBufStorage( const std::uint8_t * buf, size_t nBytes )
			: std::vector<std::uint8_t>( buf, buf + nBytes ) {}

	public:
		// バッファへのポインタ取得
		virtual uint8_t * GetBuffer( void )
		{
			return	std::vector<std::uint8_t>::data() ;
		}
		// バッファサイズ取得
		virtual size_t GetBytes( void ) const
		{
			return	std::vector<std::uint8_t>::size() ;
		}
		size_t GetCapacityBytes( void ) const
		{
			return	std::vector<std::uint8_t>::capacity() ;
		}
		// バッファサイズを変更
		void ResizeBuffer( size_t nBytes )
		{
			std::vector<std::uint8_t>::resize( nBytes, 0) ;
			NotifyBufferReallocation() ;
		}
		void ReserveBuffer( size_t nBytes )
		{
			uint8_t *	p = std::vector<std::uint8_t>::data() ;
			std::vector<std::uint8_t>::reserve( nBytes ) ;
			if ( p != std::vector<std::uint8_t>::data() )
			{
				NotifyBufferReallocation() ;
			}
		}
		// バッファの追加結合
		void AppendBuffer( const std::vector<std::uint8_t>& bufAppend )
		{
			const size_t	nBaseSize = GetBytes() ;
			std::vector<std::uint8_t>::resize( nBaseSize + bufAppend.size() ) ;
			memcpy( GetBuffer() + nBaseSize,
						bufAppend.data(), bufAppend.size() ) ;
			NotifyBufferReallocation() ;
		}

	} ;



	//////////////////////////////////////////////////////////////////////////
	// 別名バッファ
	//////////////////////////////////////////////////////////////////////////

	class	LArrayBufAlias	: public LArrayBuffer
	{
	public:
		uint8_t *	m_pBuf ;
		size_t		m_nSize ;

	public:
		LArrayBufAlias( uint8_t * pBuf = nullptr, size_t nSize = 0 )
			: m_pBuf( pBuf ), m_nSize( nSize ) {}
		LArrayBufAlias( const LArrayBufAlias& buf )
			: m_pBuf( buf.m_pBuf ), m_nSize( buf.m_nSize ) {}

	public:
		// バッファへのポインタ取得
		virtual uint8_t * GetBuffer( void )
		{
			return	m_pBuf ;
		}
		// バッファサイズ取得
		virtual size_t GetBytes( void ) const
		{
			return	m_nSize ;
		}

	} ;

}

#endif


