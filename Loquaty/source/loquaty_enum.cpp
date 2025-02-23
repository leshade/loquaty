
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Loquaty 列挙型
//////////////////////////////////////////////////////////////////////////////

// 列挙子に適合する値か？
//////////////////////////////////////////////////////////////////////////////
bool LEnumerativeClass::IsConformableValue( const LValue& value ) const
{
	if ( m_typeElement.IsInteger() )
	{
		if ( value.GetType().IsInteger() )
		{
			return	(value.AsInteger() & ~m_maskEnum) == 0 ;
		}
	}
	else if ( m_typeElement.IsFloatingPointNumber() )
	{
		if ( value.GetType().IsFloatingPointNumber() )
		{
			LDouble	fpValue = value.AsDouble() ;
			for ( size_t i = 0; i < GetElementCount(); i ++ )
			{
				LObjPtr	pElement( GetElementAt( i ) ) ;
				LDouble	dblElement ;
				if ( (pElement != nullptr) && pElement->AsDouble( dblElement ) )
				{
					if ( fpValue == dblElement )
					{
						return	true ;
					}
				}
			}
		}
	}
	else if ( m_typeElement.IsString() )
	{
		if ( value.GetType().IsString() )
		{
			LString	strValue = value.AsString() ;
			for ( size_t i = 0; i < GetElementCount(); i ++ )
			{
				LObjPtr	pElement( GetElementAt( i ) ) ;
				LString	str ;
				if ( (pElement != nullptr) && pElement->AsString( str ) )
				{
					if ( strValue == str )
					{
						return	true ;
					}
				}
			}
		}
	}
	return	false ;
}

// 複製する（要素も全て複製処理する）
LObject * LEnumerativeClass::CloneObject( void ) const
{
	return	new LEnumerativeClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LEnumerativeClass::DuplicateObject( void ) const
{
	return	new LEnumerativeClass( *this ) ;
}


