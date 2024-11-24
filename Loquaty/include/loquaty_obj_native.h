
#ifndef	__LOQUATY_OBJ_NATIVE_H__
#define	__LOQUATY_OBJ_NATIVE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// NativeObject オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LNativeObj	: public LGenericObj
	{
	public:
		std::shared_ptr<Object>	m_pNativeObj ;

	public:
		LNativeObj( LClass * pClass )
			: LGenericObj( pClass ) {}
		LNativeObj( const LNativeObj& obj )
			: LGenericObj( obj ), m_pNativeObj( obj.m_pNativeObj ) {}

		static std::shared_ptr<Object> GetNativeObject( LObject * pObj ) ;
		static void SetNativeObject( LObject * pObj, std::shared_ptr<Object> pNativeObj ) ;

		void SetNative( std::shared_ptr<Object> pObj )
		{
			m_pNativeObj = pObj ;
		}

		template <class T> std::shared_ptr<T> Get( void )
		{
			return	std::dynamic_pointer_cast<T>( m_pNativeObj ) ;
		}
		template <class T> static std::shared_ptr<T> GetNative( LObject * pObj )
		{
			return	std::dynamic_pointer_cast<T>( GetNativeObject(pObj) ) ;
		}

	public:
		// 複製する（要素も全て複製処理する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * DuplicateObject( void ) const ;

		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

	public:
		// void dispose()
		static void method_dispose( LContext& context ) ;

	public:
		// boolean operator == ( NativeObject obj )
		static LValue::Primitive operator_eq
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator != ( NativeObject obj )
		static LValue::Primitive operator_ne
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;

}

#endif


