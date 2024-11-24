
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// NativeObject オブジェクト
//////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Object> LNativeObj::GetNativeObject( LObject * pObj )
{
	if ( pObj == nullptr )
	{
		return	nullptr ;
	}
	LNativeObj *	pNativeObj = dynamic_cast<LNativeObj*>( pObj ) ;
	if ( pNativeObj == nullptr )
	{
		return	nullptr ;
	}
	return	pNativeObj->m_pNativeObj ;
}

void LNativeObj::SetNativeObject( LObject * pObj, std::shared_ptr<Object> pNativeObj )
{
	if ( pObj == nullptr )
	{
		return ;
	}
	LNativeObj *	pNObj = dynamic_cast<LNativeObj*>( pObj ) ;
	if ( pNObj == nullptr )
	{
		return ;
	}
	pNObj->SetNative( pNativeObj ) ;
}


// 複製する（要素も全て複製処理する）
// （返り値は呼び出し側の責任で ReleaseRef）
LObject * LNativeObj::CloneObject( void ) const
{
	LNativeObj *	pGenObj = new LNativeObj( m_pClass ) ;
	pGenObj->CloneFrom( *this ) ;
	pGenObj->m_pNativeObj = m_pNativeObj ;
	return	pGenObj ;
}

// 複製する（要素は参照する形で複製処理する）
// （返り値は呼び出し側の責任で ReleaseRef）
LObject * LNativeObj::DuplicateObject( void ) const
{
	return	new LNativeObj( *this ) ;
}

// 内部リソースを解放する
void LNativeObj::DisposeObject( void )
{
	LGenericObj::DisposeObject() ;

	m_pNativeObj = nullptr ;
}


// void dispose()
void LNativeObj::method_dispose( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	pNObj->m_pNativeObj = nullptr ;

	LQT_RETURN_VOID() ;
}

// boolean operator == ( NativeObject obj )
LValue::Primitive LNativeObj::operator_eq
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LNativeObj *	pNObj1 = dynamic_cast<LNativeObj*>( val1.pObject ) ;
	LNativeObj *	pNObj2 = dynamic_cast<LNativeObj*>( val2.pObject ) ;
	if ( pNObj1 == nullptr )
	{
		if ( pNObj2 == nullptr )
		{
			return	LValue::MakeBool( true ) ;
		}
		return	LValue::MakeBool( pNObj2->m_pNativeObj == nullptr ) ;
	}
	if ( pNObj2 == nullptr )
	{
		return	LValue::MakeBool( pNObj1->m_pNativeObj == nullptr ) ;
	}
	return	LValue::MakeBool( pNObj1->m_pNativeObj == pNObj2->m_pNativeObj ) ;
}

// boolean operator != ( NativeObject obj )
LValue::Primitive LNativeObj::operator_ne
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LNativeObj *	pNObj1 = dynamic_cast<LNativeObj*>( val1.pObject ) ;
	LNativeObj *	pNObj2 = dynamic_cast<LNativeObj*>( val2.pObject ) ;
	if ( pNObj1 == nullptr )
	{
		if ( pNObj2 == nullptr )
		{
			return	LValue::MakeBool( false ) ;
		}
		return	LValue::MakeBool( pNObj2->m_pNativeObj != nullptr ) ;
	}
	if ( pNObj2 == nullptr )
	{
		return	LValue::MakeBool( pNObj1->m_pNativeObj != nullptr ) ;
	}
	return	LValue::MakeBool( pNObj1->m_pNativeObj != pNObj2->m_pNativeObj ) ;
}


