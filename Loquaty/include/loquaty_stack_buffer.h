
#ifndef	__LOQUATY_STACK_BUFFER_H__
#define	__LOQUATY_STACK_BUFFER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 実行スタック
	// ※実際の CPU の場合 SP は後ろから前に移動するが、
	//   LStackBuffer では 0 から始まり後ろへ移動することに注意
	//////////////////////////////////////////////////////////////////////////

	class	LStackBuffer	: public LArrayBufStorage
	{
	public:
		typedef	LValue::Primitive	Word ;

		static constexpr const size_t	InvalidPtr	= -1 ;

	private:
		size_t	m_length ;	// バッファの要素数
		Word *	m_ptr ;		// バッファのポインタ

		size_t	m_sp ;		// スタックポインタ
		size_t	m_ap ;		// 引数ポインタ
		size_t	m_fp ;		// フレームポインタ
							// m_fp[-1]:以前のフレームポインタ
		ssize_t	m_xp ;		// 構造化例外ポインタ : see LCodeBuffer::ExceptionDesc
		ssize_t	m_yp ;		// ガベージ処理用ポインタ : see LCodeBuffer::ReleaserDesc

		friend class LContext ;

	public:
		LStackBuffer( size_t nInitSize = 0x100 )
			: LArrayBufStorage( nInitSize * sizeof(Word) ),
				m_length(nInitSize), m_ptr(nullptr),
				m_sp(0), m_ap(0), m_fp(0), m_xp(InvalidPtr), m_yp(InvalidPtr)
		{
			m_ptr = reinterpret_cast<Word*>( LArrayBufStorage::data() ) ;
		}

		// 有効なバッファサイズ（Word 単位）
		size_t GetLength( void ) const
		{
			assert( m_length * sizeof(Word) == LArrayBufStorage::size() ) ;
			return	m_length ;
		}
		// バッファサイズ取得（Byte 単位）（LArrayBuffer オーバーライド）
		virtual size_t GetBytes( void ) const ;
		// 有効なバッファサイズ拡張
		void ExpandBuffer( size_t nReqLength ) ;

		// プッシュ
		void Push( const Word& word )
		{
			if ( m_sp >= m_length )
			{
				ExpandBuffer( m_sp + 1 ) ;
				assert( m_sp < GetLength() ) ;
			}
			m_ptr[m_sp ++] = word ;
		}

		// ポップ
		Word Pop( void )
		{
			if ( m_sp > 0 )
			{
				return	m_ptr[-- m_sp] ;
			}
			else
			{
				return	LValue::MakeObjectPtr( nullptr ) ;
			}
		}

		// 直接参照
		Word& At( size_t index )
		{
			assert( index < m_sp ) ;
			return	m_ptr[index] ;
		}
		Word& BackAt( size_t index )
		{
			assert( index < m_sp ) ;
			return	m_ptr[m_sp - index - 1] ;
		}
		Word* Ptr( size_t index )
		{
			assert( index < m_sp ) ;
			return	m_ptr + index ;
		}
		const Word& GetAt( size_t index ) const
		{
			return	m_ptr[index] ;
		}
		const Word* ConstPtr( size_t index ) const
		{
			assert( index < m_sp ) ;
			return	m_ptr + index ;
		}

		// スタックポインタ
		size_t sp( void ) const
		{
			return	m_sp ;
		}
		size_t sp( ssize_t off ) const
		{
			assert( (off <= 0) && ((size_t)-off >= m_sp) ) ;
			return	m_sp + off ;
		}
		size_t BackIndex( size_t sop ) const
		{
			assert( sop < m_sp ) ;
			return	m_sp - sop - 1 ;
		}

		// スタックポインタ加算
		void AddPointer( size_t off )
		{
			if ( m_sp + off > m_length )
			{
				ExpandBuffer( m_sp + off ) ;
			}
			for ( size_t i = 0; i < off; i ++ )
			{
				m_ptr[m_sp + i].longValue = 0 ;
			}
			m_sp += off ;
		}

		// スタックポインタ減算
		void SubPointer( size_t off )
		{
			m_sp = (off <= m_sp) ? (m_sp - off) : 0 ;
		}

		// 引数ポインタ
		size_t ap( size_t index ) const
		{
			assert( m_ap + index < m_sp ) ;
			assert( m_ap + index < m_fp ) ;
			return	m_ap + index ;
		}
		void SetAP( size_t index )
		{
			m_ap = index ;
		}
		Word ArgAt( size_t index ) const
		{
			assert( m_ap + index < m_sp ) ;
			if ( m_ap + index < m_sp )
			{
				return	GetAt( m_ap + index ) ;
			}
			return	LValue::MakeObjectPtr( nullptr ) ;
		}

		// フレームポインタ
		size_t fp( void ) const
		{
			return	m_fp ;
		}
		size_t fp( ssize_t off ) const
		{
			assert( ((ssize_t)m_fp + off >= 0) && (m_fp + off <= m_sp) ) ;
			return	m_fp + off ;
		}
		void SetFP( size_t index )
		{
			m_fp = index ;
		}

		// 構造化例外ポインタ
		size_t xp( void ) const
		{
			return	m_xp ;
		}
		void SetXP( size_t index )
		{
			m_xp = index ;
		}

		// ガベージ用ポインタ
		size_t yp( void ) const
		{
			return	m_yp ;
		}
		void SetYP( size_t index )
		{
			m_yp = index ;
		}

	} ;

}

#endif

