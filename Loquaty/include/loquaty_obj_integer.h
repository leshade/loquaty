
#ifndef	__LOQUATY_OBJ_INTEGER_H__
#define	__LOQUATY_OBJ_INTEGER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Integer オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LIntegerObj	: public LObject
	{
	public:
		LLong	m_value ;

	public:
		LIntegerObj( LClass * pClass, LLong valInit = 0 )
			: LObject( pClass ), m_value( valInit ) {}
		LIntegerObj( const LIntegerObj& obj )
			: LObject( obj ), m_value( obj.m_value ) {}

	public:	// プリミティブな操作の定義
		// 整数値として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsInteger( LLong& value ) const ;
		// 浮動小数点として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsDouble( LDouble& value ) const ;
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// Integer( long val )
		static void method_init( LContext& context ) ;

	} ;

}

#endif

