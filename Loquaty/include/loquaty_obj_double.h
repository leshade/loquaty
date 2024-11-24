
#ifndef	__LOQUATY_OBJ_DOUBLE_H__
#define	__LOQUATY_OBJ_DOUBLE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Double オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LDoubleObj	: public LObject
	{
	public:
		LDouble	m_value ;

	public:
		LDoubleObj( LClass * pClass, LDouble valInit = 0.0 )
			: LObject( pClass ), m_value( valInit ) {}
		LDoubleObj( const LDoubleObj& obj )
			: LObject( obj ), m_value( obj.m_value ) {}

	public:
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
		// Double( double val )
		static void method_init( LContext& context ) ;

	} ;

}

#endif

