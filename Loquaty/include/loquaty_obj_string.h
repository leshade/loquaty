
#ifndef	__LOQUATY_OBJ_STRING_H__
#define	__LOQUATY_OBJ_STRING_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// String オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LStringObj	: public LObject
	{
	public:
		LString	m_string ;

	public:
		LStringObj( LClass * pClass, const LString& strInit )
			: LObject( pClass ), m_string( strInit ) {}
		LStringObj( LClass * pClass, const wchar_t * pwszInit = nullptr )
			: LObject( pClass ), m_string( pwszInit ) {}
		LStringObj( const LStringObj& obj )
			: LObject( obj ), m_string( obj.m_string ) {}

	public:	// プリミティブな操作の定義
		// 要素数
		virtual size_t GetElementCount( void ) const ;
		// 要素取得
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * GetElementAt( size_t index ) const ;
		// 要素型情報取得
		virtual LType GetElementTypeAt( size_t index ) const ;

		// 整数値として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsInteger( LLong& value ) const ;
		// 浮動小数点として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsDouble( LDouble& value ) const ;
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		// （式表現に近い）文字列に変換
		virtual bool AsExpression( LString& str, std::uint64_t flags = 0 ) const ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// String( String str )
		static void method_init( LContext& context ) ;
		// String( uint8* utf8, long length = -1 )
		static void method_init_utf8( LContext& context ) ;
		// String( uint16* utf16, long length = -1 )
		static void method_init_utf16( LContext& context ) ;
		// String( uint32* utf32, long length = -1 )
		static void method_init_utf32( LContext& context ) ;
		// uint32 length() const
		static void method_length( LContext& context ) ;
		// String left( long count ) const
		static void method_left( LContext& context ) ;
		// String right( long count ) const
		static void method_right( LContext& context ) ;
		// String middle( long first, long count = -1 ) const
		static void method_middle( LContext& context ) ;
		// int find( String str, int first = 0 ) const
		static void method_find( LContext& context ) ;
		// String upper() const
		static void method_upper( LContext& context ) ;
		// String lower() const
		static void method_lower( LContext& context ) ;
		// String trim() const
		static void method_trim( LContext& context ) ;
		// String chop( long count ) const
		static void method_chop( LContext& context ) ;
		// uint32 charAt( long index ) const
		static void method_charAt( LContext& context ) ;
		// uint32 backAt( long index ) const
		static void method_backAt( LContext& context ) ;
		// String replace( Map map ) const
		static void method_replace( LContext& context ) ;
		// uint8* utf8() const
		static void method_utf8( LContext& context ) ;
		// uint16* utf16() const
		static void method_utf16( LContext& context ) ;
		// uint32* utf32() const
		static void method_utf32( LContext& context ) ;
		// long asInteger( boolean* pHasInteger = null, int radix = 10 ) const
		static void method_asInteger( LContext& context ) ;
		// double asNumber( boolean* pHasNumber = null ) const
		static void method_asNumber( LContext& context ) ;

		// static String integerOf( long val, int prec = 0, int radix = 10 ) const
		static void method_integerOf( LContext& context ) ;
		// static String numberOf( double val, int prec = 0, boolean exp = true ) const
		static void method_numberOf( LContext& context ) ;

	public:
		// boolean operator == ( String str )
		static LValue::Primitive operator_eq
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator != ( String str )
		static LValue::Primitive operator_ne
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator < ( String str )
		static LValue::Primitive operator_lt
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator <= ( String str )
		static LValue::Primitive operator_le
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator > ( String str )
		static LValue::Primitive operator_gt
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator >= ( String str )
		static LValue::Primitive operator_ge
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// String operator + ( String str )
		static LValue::Primitive operator_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// String operator + ( long val )
		static LValue::Primitive operator_add_int
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// String operator + ( double val )
		static LValue::Primitive operator_add_num
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// String operator + ( Object obj )
		static LValue::Primitive operator_add_obj
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// String operator * ( long count )
		static LValue::Primitive operator_mul
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// String[] operator / ( String deli )
		static LValue::Primitive operator_div
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// StringBuf オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LStringBufObj	: public LStringObj
	{
	public:
		LSpinLockMutex		m_mutex ;

	public:
		LStringBufObj( LClass * pClass, const LString& strInit )
			: LStringObj( pClass, strInit ) {}
		LStringBufObj( LClass * pClass, const wchar_t * pwszInit = nullptr )
			: LStringObj( pClass, pwszInit ) {}
		LStringBufObj( const LStringBufObj& obj )
			: LStringObj( obj ) {}

	public:	// プリミティブな操作の定義
		// 要素取得
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * GetElementAt( size_t index ) const ;
		// 要素設定（pObj には AddRef されたポインタを渡す）
		// （以前の要素を返す / ReleaseRef が必要）
		virtual LObject * SetElementAt( size_t index, LObject * pObj ) ;

		// 整数値として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsInteger( LLong& value ) const ;
		// 浮動小数点として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsDouble( LDouble& value ) const ;
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		// （式表現に近い）文字列に変換
		virtual bool AsExpression( LString& str, std::uint64_t flags = 0 ) const ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// StringBuf( String str )
		static void method_init( LContext& context ) ;
		// StringBuf set( String str )
		static void method_set( LContext& context ) ;
		// StringBuf setIntegerOf( long val, int prec = 0, int radix = 10 )
		static void method_setIntegerOf( LContext& context ) ;
		// StringBuf setNumberOf( double val, int prec = 0, boolean exp = true )
		static void method_setNumberOf( LContext& context ) ;
		// void setAt( long index, uint32 code )
		static void method_setAt( LContext& context ) ;
		// void resize( long length )
		static void method_resize( LContext& context ) ;

	public:
		// StringBuf operator := ( String str )
		static LValue::Primitive operator_smov
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// StringBuf operator += ( String str )
		static LValue::Primitive operator_sadd
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
	} ;


}

#endif

