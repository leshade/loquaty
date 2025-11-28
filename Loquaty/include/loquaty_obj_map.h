
#ifndef	__LOQUATY_OBJ_MAP_H__
#define	__LOQUATY_OBJ_MAP_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Map オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LMapObj	: public LArrayObj
	{
	public:
		std::map<std::wstring,size_t>	m_map ;
		LSpinLockMutex					m_mtxMap ;	// m_map 操作保護用

	public:
		LMapObj( LClass * pClass )
			: LArrayObj( pClass ) {}
		LMapObj( const LMapObj& obj )
			: LArrayObj( obj ), m_map( obj.m_map ) {}

	public:	// プリミティブな操作の定義
		// 要素取得（存在する場合には AddRef されたポインタを返す）
		virtual LObject * GetElementAs( const wchar_t * name ) const ;
		// 要素検索
		virtual ssize_t FindElementAs( const wchar_t * name ) const ;
		// 要素名取得
		virtual const wchar_t * GetElementNameAt( LString& strName, size_t index ) const ;
		// 要素型情報取得
		virtual LType GetElementTypeAt( size_t index ) const ;
		virtual LType GetElementTypeAs( const wchar_t * name ) const ;
		// 要素設定（pObj には AddRef されたポインタを渡す）
		virtual LObject * SetElementAt( size_t index, LObject * pObj ) ;
		virtual LObject * SetElementAs( const wchar_t * name, LObject * pObj ) ;

		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		// （式表現に近い）文字列に変換
		virtual bool AsExpression( LString& str, std::uint64_t flags = 0 ) const ;

		// 型変換（可能なら AddRef されたポインタを返す / 不可能なら nullptr）
		virtual LObject * CastClassTo( LClass * pClass ) ;
		virtual LObject * CastElementClassTo( LMapClass * pMapClass ) ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		void CloneFrom( const LMapObj& obj ) ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;
		void DuplicateFrom( const LMapObj& obj ) ;

		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

	public:	// Map 固有の操作
		// 要素の削除
		virtual void RemoveElementAt( size_t index ) ;
		virtual void RemoveElementAs( const wchar_t * name ) ;
		virtual void RemoveAll( void ) ;

		// クラスのメンバやポインタの参照先の構造体に値を設定
		virtual bool PutMembers( const LObjPtr& pObj ) ;

	public:
		// uint32 size() const
		static void method_size( LContext& context ) ;
		// boolean has( String key ) const
		static void method_has( LContext& context ) ;
		// int find( String key ) const
		static void method_find( LContext& context ) ;
		// String keyAt( long index ) const
		static void method_keyAt( LContext& context ) ;
		// Object removeAt( long index )
		static void method_removeAt( LContext& context ) ;
		// Object removeAs( String key )
		static void method_removeAs( LContext& context ) ;

	public:
		// := 演算子 : LMapObj*(LMapObj*,LMapObj*)
		// Map operator := ( Map obj )
		static LValue::Primitive operator_smov
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;

}

#endif

