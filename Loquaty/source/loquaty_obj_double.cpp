
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Double オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 整数値として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LDoubleObj::AsInteger( LLong& value ) const
{
	value = (LLong) m_value ;
	return	true ;
}

// 浮動小数点として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LDoubleObj::AsDouble( LDouble& value ) const
{
	value = m_value ;
	return	true ;
}

// 文字列として評価
bool LDoubleObj::AsString( LString& str ) const
{
	str.SetNumberOf( m_value, 14, true ) ;
	return	true ;
}

// 複製する（要素も全て複製処理する）
LObject * LDoubleObj::CloneObject( void ) const
{
	return	new LDoubleObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LDoubleObj::DuplicateObject( void ) const
{
	return	new LDoubleObj( *this ) ;
}

// Double( double val )
void LDoubleObj::method_init( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LDoubleObj, pObj ) ;
	LQT_FUNC_ARG_DOUBLE( val ) ;

	pObj->m_value = val ;

	LQT_RETURN_VOID() ;
}



