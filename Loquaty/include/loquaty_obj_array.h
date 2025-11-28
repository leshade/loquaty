
#ifndef	__LOQUATY_OBJ_ARRAY_H__
#define	__LOQUATY_OBJ_ARRAY_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Array オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LArrayObj	: public LObject
	{
	public:
		std::vector<LObjPtr>	m_array ;
		size_t					m_bounds ;

		LSpinLockMutex			m_mtxArray ;	// m_array 操作保護用

	public:
		LArrayObj( LClass * pClass, size_t bounds = SIZE_MAX )
			: LObject( pClass ), m_bounds( bounds ) {}
		LArrayObj( const LArrayObj& obj )
			: LObject( obj ), m_array( obj.m_array ), m_bounds( obj.m_bounds ) {}
		virtual ~LArrayObj( void ) {}

	public:
		// 要素数
		virtual size_t GetElementCount( void ) const ;
		// 要素取得（存在する場合には AddRef されたポインタを返す）
		virtual LObject * GetElementAt( size_t index ) const ;
		// 要素設定（pObj には AddRef されたポインタを渡す）
		virtual LObject * SetElementAt( size_t index, LObject * pObj ) ;
		// 要素型情報取得
		virtual LType GetElementTypeAt( size_t index ) const ;

		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		// （式表現に近い）文字列に変換
		virtual bool AsExpression( LString& str, std::uint64_t flags = 0 ) const ;

		// 型変換（可能なら AddRef されたポインタを返す / 不可能なら nullptr）
		virtual LObject * CastClassTo( LClass * pClass ) ;
		virtual LObject * CastElementClassTo( LArrayClass * pArrayClass ) ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		void CloneFrom( const LArrayObj& obj ) ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;
		void DuplicateFrom( const LArrayObj& obj ) ;

		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

	public:	// Array 固有の操作
		// 要素の削除
		virtual void RemoveElementAt( size_t index ) ;
		virtual void RemoveAll( void ) ;

		// クラスのメンバやポインタの参照先の構造体に値を設定
		virtual bool PutMembers( const LObjPtr& pObj ) ;

		// スピンロック（配列操作保護）
		const LSpinLockMutex& ArrayMutex( void ) const
		{
			return	m_mtxArray ;
		}

	public:
		// uint32 length() const
		static void method_length( LContext& context ) ;
		// uint32 add( Object obj )
		// uint32 push( Object obj )
		static void method_add( LContext& context ) ;
		// Object pop()
		static void method_pop( LContext& context ) ;
		// void insert( long index, Object obj )
		static void method_insert( LContext& context ) ;
		// void merge( long index, const Array src, long first = 0, long count =-1 )
		static void method_merge( LContext& context ) ;
		// Object remove( long index )
		static void method_remove( LContext& context ) ;
		// int32 findPtr( Object obj ) const
		static void method_findPtr( LContext& context ) ;
		// void clear()
		static void method_clear( LContext& context ) ;

	public:
		// := 演算子 : LArrayObj*(LArrayObj*,LArrayObj*)
		// Array operator := ( Array obj )
		static LValue::Primitive operator_smov
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// +=, << 演算子 : LArrayObj*(LArrayObj*,LObject*)
		// Array operator << ( Object obj )
		static LValue::Primitive operator_sadd
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// + 演算子 : LArrayObj*(LArrayObj*,LObject*)
		// Array operator + ( Object obj )
		static LValue::Primitive operator_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;

}

#endif

