
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Array オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 要素数
size_t LArrayObj::GetElementCount( void ) const
{
	return	m_array.size() ;
}

// 要素取得（存在する場合には AddRef されたポインタを返す）
LObject * LArrayObj::GetElementAt( size_t index ) const
{
	LSpinLock	lock( m_mtxArray ) ;
	if ( index >= m_array.size() )
	{
		return	nullptr ;
	}
	return	m_array.at(index).Get() ;
}

// 要素設定（pObj には AddRef されたポインタを渡す）
LObject * LArrayObj::SetElementAt( size_t index, LObject * pObj )
{
	LSpinLock	lock( m_mtxArray ) ;
	if ( index >= m_array.size() )
	{
		if ( index >= m_bounds )
		{
			LContext::ThrowExceptionError( exceptionIndexOutOfBounds ) ;
			LObject::ReleaseRef( pObj ) ;
			return	nullptr ;
		}
		m_array.resize( index + 1, nullptr ) ;
	}
	LObject *	pOldObj = m_array.at( index ).Get() ;
	if ( (pObj != nullptr)
		&& (pObj->GetClass() != nullptr)
		&& (m_pClass != nullptr) )
	{
		LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( m_pClass ) ;
		if ( (pArrayClass != nullptr)
			&& (pArrayClass->GetElementTypeClass() != nullptr) )
		{
			if ( !pObj->GetClass()->IsInstanceOf
							( pArrayClass->GetElementTypeClass() ) )
			{
				LContext::ThrowExceptionError( exceptionElementTypeMismatch ) ;
			}
		}
	}
	m_array.at( index ) = pObj ;
	return	pOldObj ;
}

// 要素型情報取得
LType LArrayObj::GetElementTypeAt( size_t index ) const
{
	if ( m_pClass == nullptr )
	{
		return	LType( LType::typeObject ) ;
	}
	LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( m_pClass ) ;
	if ( (pArrayClass != nullptr)
		&& (pArrayClass->GetElementTypeClass() != nullptr) )
	{
		return	LType( pArrayClass->GetElementTypeClass() ) ;
	}
	return	LType( m_pClass->VM().GetObjectClass() ) ;
}

// 文字列として評価
bool LArrayObj::AsString( LString& str ) const
{
	return	LArrayObj::AsExpression( str ) ;
}

// （式表現に近い）文字列に変換
bool LArrayObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	LSpinLock	lock( m_mtxArray ) ;
	LString	strElement ;
	str = L"[ " ;
	for ( size_t i = 0; i < m_array.size(); i ++ )
	{
		if ( i >= 1 )
		{
			str += L", " ;
		}
		str += ToExpression( m_array.at(i).Ptr(), flags ) ;
	}
	str += L" ]" ;
	return	true ;
}

// 型変換（可能なら AddRef されたポインタを返す / 不可能なら nullptr）
LObject * LArrayObj::CastClassTo( LClass * pClass )
{
	LObject *	pObj = LObject::CastClassTo( pClass ) ;
	if ( pObj != nullptr )
	{
		return	pObj ;
	}
	LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( pClass ) ;
	if ( pArrayClass == nullptr )
	{
		return	nullptr ;
	}
	LClass *	pElementType = pArrayClass->GetElementTypeClass() ;
	if ( pElementType == nullptr )
	{
		AddRef() ;
		return	this ;
	}
	LSpinLock	lock( m_mtxArray ) ;
	LArrayObj *	pArrayObj = new LArrayObj( pClass ) ;
	pArrayObj->m_array.resize( m_array.size(), nullptr ) ;
	for ( size_t i = 0; i < m_array.size(); i ++ )
	{
		pObj = m_array.at(i).Ptr() ;
		if ( pObj != nullptr )
		{
			pObj = pObj->CastClassTo( pElementType ) ;
			if ( pObj == nullptr )
			{
				// 変換できなかった場合は nullptr
				delete	pArrayObj ;
				return	nullptr ;
			}
			pArrayObj->m_array.at(i) = pObj ;
		}
	}
	return	pArrayObj ;
}

// 複製する（要素も全て複製処理する）
LObject * LArrayObj::CloneObject( void ) const
{
	LSpinLock	lock( m_mtxArray ) ;
	LArrayObj *	pArrayObj = new LArrayObj( m_pClass, m_bounds ) ;
	pArrayObj->CloneFrom( *this ) ;
	return	pArrayObj ;
}

void LArrayObj::CloneFrom( const LArrayObj& obj )
{
	LSpinLock	lock( m_mtxArray ) ;
	m_array.resize( obj.m_array.size(), nullptr ) ;

	for ( size_t i = 0; i < obj.m_array.size(); i ++ )
	{
		LObject *	pObj = obj.m_array.at(i).Ptr() ;
		if ( pObj != nullptr )
		{
			m_array.at(i) = pObj->CloneObject() ;
		}
	}
}

// 複製する（要素は参照する形で複製処理する）
LObject * LArrayObj::DuplicateObject( void ) const
{
	LSpinLock	lock( m_mtxArray ) ;
	return	new LArrayObj( *this ) ;
}

void LArrayObj::DuplicateFrom( const LArrayObj& obj )
{
	LSpinLock	lock( m_mtxArray ) ;
	m_array = obj.m_array ;
	m_bounds = obj.m_bounds ;
}

// 内部リソースを解放する
void LArrayObj::DisposeObject( void )
{
	RemoveAll() ;

	LObject::DisposeObject() ;
}

// 要素の削除
void LArrayObj::RemoveElementAt( size_t index )
{
	LSpinLock	lock( m_mtxArray ) ;
	if ( index < m_array.size() )
	{
		m_array.erase( m_array.begin() + index ) ;
	}
}

void LArrayObj::RemoveAll( void )
{
	LSpinLock	lock( m_mtxArray ) ;
	m_array.clear() ;
}

// uint32 length() const
void LArrayObj::method_length( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;

	LQT_RETURN_LONG( pArray->m_array.size() ) ;
}

// uint32 add( Object obj )
// uint32 push( Object obj )
void LArrayObj::method_add( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;
	LQT_FUNC_ARG_OBJECT( LObject, pObj ) ;

	LSpinLock	lock( pArray->m_mtxArray ) ;
	const size_t	index = pArray->m_array.size() ;
	if ( pObj != nullptr )
	{
		LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( pArray->m_pClass ) ;
		if ( (pArrayClass != nullptr)
			&& (pArrayClass->GetElementTypeClass() != nullptr) )
		{
			pObj = pObj->CastClassTo( pArrayClass->GetElementTypeClass() ) ;
			if ( pObj == nullptr )
			{
				_context.ThrowException( exceptionElementTypeMismatch ) ;
				return ;
			}
		}
	}
	pArray->m_array.push_back( pObj ) ;

	LQT_RETURN_LONG( index ) ;
}

// Object pop()
void LArrayObj::method_pop( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;

	LObjPtr		pObj ;
	LSpinLock	lock( pArray->m_mtxArray ) ;
	if ( pArray->m_array.size() > 0 )
	{
		pObj = pArray->m_array.back() ;
		pArray->m_array.pop_back() ;
	}
	LQT_RETURN_OBJECT( pObj.Detach() ) ;
}

// void insert( long index, Object obj )
void LArrayObj::method_insert( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;
	LQT_FUNC_ARG_LONG( index ) ;
	LQT_FUNC_ARG_OBJECT( LObject, pObj ) ;

	if ( pObj != nullptr )
	{
		LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( pArray->m_pClass ) ;
		if ( (pArrayClass != nullptr)
			&& (pArrayClass->GetElementTypeClass() != nullptr) )
		{
			pObj = pObj->CastClassTo( pArrayClass->GetElementTypeClass() ) ;
			if ( pObj == nullptr )
			{
				_context.ThrowException( exceptionElementTypeMismatch ) ;
				return ;
			}
		}
	}
	LSpinLock	lock( pArray->m_mtxArray ) ;
	if ( (size_t) index > pArray->m_array.size() )
	{
		index = (size_t) pArray->m_array.size() ;
	}
	pArray->m_array.insert
		( pArray->m_array.begin() + (size_t) index, pObj ) ;

	LQT_RETURN_VOID() ;
}

// void merge( long index, const Array src, long first = 0, long count =-1 )
void LArrayObj::method_merge( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;
	LQT_FUNC_ARG_LONG( index ) ;
	LQT_FUNC_ARG_OBJECT( LArrayObj, pSrcArray ) ;
	LQT_FUNC_ARG_LONG( first ) ;
	LQT_FUNC_ARG_LONG( count ) ;

	if ( pSrcArray != nullptr )
	{
		LSpinLock	lock( pArray->m_mtxArray ) ;
		LSpinLock	lockSrc( pSrcArray->m_mtxArray ) ;

		size_t	iInsert = std::min( (size_t) index, pArray->m_array.size() ) ;
		size_t	iSrc = std::min( (size_t) first, pSrcArray->m_array.size() ) ;
		size_t	nCount = pSrcArray->m_array.size() - iSrc ;
		if ( count >= 0 )
		{
			nCount = std::min( (size_t) count, nCount ) ;
		}
		for ( size_t i = 0; i < nCount; i ++ )
		{
			pArray->m_array.insert
				( pArray->m_array.begin() + (iInsert + i),
							pSrcArray->m_array.at( iSrc + i ) ) ;
		}
	}

	LQT_RETURN_VOID() ;
}

// Object remove( long index )
void LArrayObj::method_remove( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;
	LQT_FUNC_ARG_LONG( index ) ;

	LObjPtr		pObj ;
	if ( (size_t) index < pArray->m_array.size() )
	{
		pObj = pArray->m_array.at( (size_t) index ) ;
		pArray->RemoveElementAt( (size_t) index ) ;
	}
	LQT_RETURN_OBJECT( pObj.Detach() ) ;
}

// int32 findPtr( Object obj ) const
void LArrayObj::method_findPtr( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;
	LQT_FUNC_ARG_OBJECT( LObject, pObj ) ;

	LSpinLock	lock( pArray->m_mtxArray ) ;
	for ( size_t i = 0; i < pArray->m_array.size(); i ++ )
	{
		if ( pArray->m_array.at(i) == pObj )
		{
			LQT_RETURN_INT( (LInt) i ) ;
		}
	}

	LQT_RETURN_INT( -1 ) ;
}

// void clear()
void LArrayObj::method_clear( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LArrayObj, pArray ) ;

	pArray->RemoveAll() ;

	LQT_RETURN_VOID() ;
}

// := 演算子 : LArrayObj*(LArrayObj*,LArrayObj*)
// Array operator := ( Array obj )
LValue::Primitive LArrayObj::operator_smov
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LArrayObj *	pArray1 = dynamic_cast<LArrayObj*>( val1.pObject ) ;
	if ( pArray1 == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LArrayObj *	pArray2 = dynamic_cast<LArrayObj*>( val2.pObject ) ;
	if ( pArray2 == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	pArray1->RemoveAll() ;
	pArray1->CloneFrom( *pArray2 ) ;

	pArray1->AddRef() ;
	return	LValue::MakeObjectPtr( pArray1 ) ;
}

// +=, << 演算子 : LArrayObj*(LArrayObj*,LObject*)
// Array operator << ( Object obj )
LValue::Primitive LArrayObj::operator_sadd
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LArrayObj *	pArray = dynamic_cast<LArrayObj*>( val1.pObject ) ;
	if ( pArray == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LObjPtr	pObj = val2.pObject ;
	if ( pObj != nullptr )
	{
		pObj->AddRef() ;

		LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( pArray->m_pClass ) ;
		if ( (pArrayClass != nullptr)
			&& (pArrayClass->GetElementTypeClass() != nullptr) )
		{
			pObj = pObj->CastClassTo( pArrayClass->GetElementTypeClass() ) ;
			if ( pObj == nullptr )
			{
				LContext::ThrowExceptionError( exceptionElementTypeMismatch ) ;
				return	LValue::MakeObjectPtr( nullptr ) ;
			}
		}
	}
	pArray->m_array.push_back( pObj ) ;

	pArray->AddRef() ;
	return	LValue::MakeObjectPtr( pArray ) ;
}

// + 演算子 : LArrayObj*(LArrayObj*,LObject*)
// Array operator + ( Object obj )
LValue::Primitive LArrayObj::operator_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LArrayObj *	pArray = dynamic_cast<LArrayObj*>( val1.pObject ) ;
	if ( pArray == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LObjPtr	pObj = val2.pObject ;
	if ( pObj != nullptr )
	{
		pObj->AddRef() ;

		LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( pArray->m_pClass ) ;
		if ( (pArrayClass != nullptr)
			&& (pArrayClass->GetElementTypeClass() != nullptr) )
		{
			pObj = pObj->CastClassTo( pArrayClass->GetElementTypeClass() ) ;
			if ( pObj == nullptr )
			{
				LContext::ThrowExceptionError( exceptionElementTypeMismatch ) ;
				return	LValue::MakeObjectPtr( nullptr ) ;
			}
		}
	}
	LArrayObj *	pDupArray = new LArrayObj( *pArray ) ;
	pDupArray->m_array.push_back( pObj ) ;

	return	LValue::MakeObjectPtr( pDupArray ) ;
}

