
#ifndef	__LOQUATY_OBJ_POINTER_H__
#define	__LOQUATY_OBJ_POINTER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// ポインタ・オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LPointerObj	: public LObject
	{
	private:
		LPointerObj *	m_pChainPrev ;
		LPointerObj *	m_pChainNext ;

		friend class LArrayBuffer ;

	protected:
		std::shared_ptr<LArrayBuffer>	m_pBuf ;
		uint8_t *						m_pBufPtr ;
		size_t							m_nBufSize ;
		size_t							m_iOffset ;
		size_t							m_iBoundFirst ;
		size_t							m_iBoundEnd ;

	public:
		LPointerObj( LClass * pClass )
			: LObject( pClass ),
				m_pChainPrev( nullptr ), m_pChainNext( nullptr ),
				m_pBufPtr( nullptr ), m_nBufSize( 0 ),
				m_iOffset( 0 ), m_iBoundFirst( 0 ), m_iBoundEnd( 0 ) {}
		LPointerObj( LClass * pClass,
						std::shared_ptr<LArrayBuffer> buf, size_t iOffset = 0 )
			: LObject( pClass ),
				m_pChainPrev( nullptr ), m_pChainNext( nullptr ),
				m_pBuf( buf ), m_pBufPtr( nullptr ), m_nBufSize( 0 ),
				m_iOffset( iOffset ), m_iBoundFirst( 0 ), m_iBoundEnd( SIZE_MAX )
		{
			AttachRefChain() ;
			OnBufferReallocated() ;
		}
		LPointerObj( const LPointerObj& ptr )
			: LObject( ptr ),
				m_pChainPrev( nullptr ), m_pChainNext( nullptr ),
				m_pBuf( ptr.m_pBuf ),
				m_pBufPtr( ptr.m_pBufPtr ), m_nBufSize( ptr.m_nBufSize ),
				m_iOffset( ptr.m_iOffset ),
				m_iBoundFirst( ptr.m_iBoundFirst ), m_iBoundEnd( ptr.m_iBoundEnd )
		{
			AttachRefChain() ;
		}
		virtual ~LPointerObj( void )
		{
			DetachRefChain() ;
		}

		// 代入
		const LPointerObj& operator = ( const LPointerObj& ptr ) ;

		// ポインタ操作
		const LPointerObj& operator += ( ssize_t iOffset )
		{
			m_iOffset += iOffset ;
			return	*this ;
		}
		const LPointerObj& operator -= ( ssize_t iOffset )
		{
			m_iOffset -= iOffset ;
			return	*this ;
		}
		LPointerObj operator + ( ssize_t iOffset )
		{
			LPointerObj	ptr( *this ) ;
			ptr += iOffset ;
			return	ptr ;
		}
		LPointerObj operator - ( ssize_t iOffset )
		{
			LPointerObj	ptr( *this ) ;
			ptr -= iOffset ;
			return	ptr ;
		}

		// バッファ参照設定
		void SetPointer
			( std::shared_ptr<LArrayBuffer> pBuf, size_t iOffset, size_t nBounds ) ;

		// ポインタ取得
		std::uint8_t * GetPointer( ssize_t iOffset = 0, size_t nRangeBytes = 0 ) const
		{
			iOffset += (ssize_t) m_iOffset ;
			if ( (m_pBufPtr != nullptr)
				&& ((size_t) iOffset >= m_iBoundFirst)
				&& ((size_t) (iOffset + nRangeBytes) <= m_iBoundEnd) )
			{
				assert( m_pBufPtr != nullptr ) ;
				assert( m_pBufPtr == m_pBuf->GetBuffer() ) ;
				assert( m_nBufSize <= m_pBuf->GetBytes() ) ;
				assert( (size_t) (iOffset + nRangeBytes) <= m_nBufSize ) ;
				return	m_pBufPtr + iOffset ;
			}
			return	nullptr ;
		}
		template <class T> T * Ptr( void ) const
		{
			return	reinterpret_cast<T*>( GetPointer( 0, sizeof(T) ) ) ;
		}

		// オフセット値取得
		size_t GetOffset( void ) const
		{
			return	m_iOffset ;
		}

		// オフセットから末端までのバイト数
		size_t GetBytes( void ) const
		{
			return	(m_iOffset <= m_iBoundEnd) ? (m_iBoundEnd - m_iOffset) : 0 ;
		}

		// アライメント・チェック
		bool CheckAlignment( ssize_t iOffset, size_t nAlign ) const
		{
			assert( (nAlign == 1) || (nAlign == 2) || (nAlign == 4) || (nAlign == 8) || (nAlign == 16) ) ;
			return	((m_iOffset + iOffset) & (nAlign - 1)) == 0 ;
		}

		// プリミティブ・ロード（ポインタが無効の場合例外を送出）
		LLong LoadIntegerAt( size_t iOffset, LType::Primitive type ) const ;
		LDouble LoadDoubleAt( size_t iOffset, LType::Primitive type ) const ;

		// プリミティブ・ストア（ポインタが無効の場合例外を送出）
		void StoreIntegerAt
			( size_t iOffset, LType::Primitive type, LLong val ) const ;
		void StoreDoubleAt
			( size_t iOffset, LType::Primitive type, LDouble val ) const ;

		// 実行時の型情報を使って値をストア（例外は送出しない）
		bool PutInteger( LLong val ) const ;
		bool PutDouble( LDouble val ) const ;

		// クラスのメンバやポインタの参照先の構造体に値を設定
		bool PutMembers( const LValue& val ) ;
		bool PutMembers( size_t iOffset, const LType& type, const LValue& val ) ;

	public:
		// プリミティブ・ロード関数
		typedef LLong (*PFN_LoadPrimitiveAsLong)( const std::uint8_t * p ) ;
		typedef LDouble (*PFN_LoadPrimitiveAsDouble)( const std::uint8_t * p ) ;

		static const PFN_LoadPrimitiveAsLong
							s_pnfLoadAsLong[LType::typePrimitiveCount] ;
		static const PFN_LoadPrimitiveAsDouble
							s_pnfLoadAsDouble[LType::typePrimitiveCount] ;

		// プリミティブ・ストア関数
		typedef void (*PFN_StorePrimitiveAsLong)( std::uint8_t * p, LLong val ) ;
		typedef void (*PFN_StorePrimitiveAsDouble)( std::uint8_t * p, LDouble val ) ;

		static const PFN_StorePrimitiveAsLong
							s_pnfStoreAsLong[LType::typePrimitiveCount] ;
		static const PFN_StorePrimitiveAsDouble
							s_pnfStoreAsDouble[LType::typePrimitiveCount] ;

	public:
		static LLong LoadBooleanAsLong( const std::uint8_t * p ) ;
		static LLong LoadInt8AsLong( const std::uint8_t * p ) ;
		static LLong LoadUint8AsLong( const std::uint8_t * p ) ;
		static LLong LoadInt16AsLong( const std::uint8_t * p ) ;
		static LLong LoadUint16AsLong( const std::uint8_t * p ) ;
		static LLong LoadInt32AsLong( const std::uint8_t * p ) ;
		static LLong LoadUint32AsLong( const std::uint8_t * p ) ;
		static LLong LoadInt64AsLong( const std::uint8_t * p ) ;
		static LLong LoadUint64AsLong( const std::uint8_t * p ) ;
		static LLong LoadFloatAsLong( const std::uint8_t * p ) ;
		static LLong LoadDoubleAsLong( const std::uint8_t * p ) ;

		static LDouble LoadBooleanAsDouble( const std::uint8_t * p ) ;
		static LDouble LoadInt8AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadUint8AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadInt16AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadUint16AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadInt32AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadUint32AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadInt64AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadUint64AsDouble( const std::uint8_t * p ) ;
		static LDouble LoadFloatAsDouble( const std::uint8_t * p ) ;
		static LDouble LoadDoubleAsDouble( const std::uint8_t * p ) ;

		static void StoreBooleanAsLong( std::uint8_t * p, LLong val ) ;
		static void StoreInt8AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreUint8AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreInt16AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreUint16AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreInt32AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreUint32AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreInt64AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreUint64AsLong( std::uint8_t * p, LLong val ) ;
		static void StoreFloatAsLong( std::uint8_t * p, LLong val ) ;
		static void StoreDoubleAsLong( std::uint8_t * p, LLong val ) ;

		static void StoreBooleanAsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreInt8AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreUint8AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreInt16AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreUint16AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreInt32AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreUint32AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreInt64AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreUint64AsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreFloatAsDouble( std::uint8_t * p, LDouble val ) ;
		static void StoreDoubleAsDouble( std::uint8_t * p, LDouble val ) ;

	protected:
		// バッファ参照バックリンク設定
		void AttachRefChain( void )
		{
			if ( m_pBuf != nullptr )
			{
				m_pBuf->AttachRefChain( this ) ;
			}
		}
		// バッファ参照バックリンク解除
		void DetachRefChain( void )
		{
			if ( m_pBuf != nullptr )
			{
				m_pBuf->DetachRefChain( this ) ;
			}
		}

	public:
		// バッファアドレス更新通知
		virtual void OnBufferReallocated( void ) ;

	public:
		// 要素数
		virtual size_t GetElementCount( void ) const ;

		// 保持するバッファへのポインタを返す
		virtual LPointerObj * GetBufferPoiner( void ) ;

		// 整数値として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsInteger( LLong& value ) const ;
		// 浮動小数点として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsDouble( LDouble& value ) const ;
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		virtual bool AsExpression( LString& str, std::uint64_t flags = 0 ) const ;
		static void ExprIntAsString( LString& str, LLong value ) ;
		static double EntropyOfNumString( const LString& str ) ;

		// 型変換（可能なら AddRef されたポインタを返す / 不可能なら nullptr）
		virtual LObject * CastClassTo( LClass * pClass ) ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// void copy( Pointer pSrc, long bytes )
		static void method_copy( LContext& context ) ;
		// void copyTo( Pointer pDst, long bytes )
		static void method_copyTo( LContext& context ) ;

	} ;

}

#endif


