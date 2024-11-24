
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Integer オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 整数値として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LIntegerObj::AsInteger( LLong& value ) const
{
	value = m_value ;
	return	true ;
}

// 浮動小数点として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LIntegerObj::AsDouble( LDouble& value ) const
{
	value = (LDouble) m_value ;
	return	true ;
}

// 文字列として評価
bool LIntegerObj::AsString( LString& str ) const
{
	str.SetIntegerOf( m_value ) ;
	return	true ;
}

// 複製する（要素も全て複製処理する）
LObject * LIntegerObj::CloneObject( void ) const
{
	return	new LIntegerObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LIntegerObj::DuplicateObject( void ) const
{
	return	new LIntegerObj( *this ) ;
}


// Integer( long val )
void LIntegerObj::method_init( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LIntegerObj, pObj ) ;
	LQT_FUNC_ARG_LONG( val ) ;

	pObj->m_value = val ;

	LQT_RETURN_VOID() ;
}

