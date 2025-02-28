
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Map オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 要素取得（存在する場合には AddRef されたポインタを返す）
LObject * LMapObj::GetElementAs( const wchar_t * name ) const
{
	LSpinLock	lock( m_mtxMap ) ;
	auto	iter = m_map.find( name ) ;
	if ( iter == m_map.end() )
	{
		return	nullptr ;
	}
	return	LArrayObj::GetElementAt( iter->second ) ;
}

// 要素検索
ssize_t LMapObj::FindElementAs( const wchar_t * name ) const
{
	LSpinLock	lock( m_mtxMap ) ;
	auto	iter = m_map.find( name ) ;
	if ( iter == m_map.end() )
	{
		return	-1 ;
	}
	return	iter->second ;
}

// 要素名取得
const wchar_t * LMapObj::GetElementNameAt( LString& strName, size_t index ) const
{
	LSpinLock	lock( m_mtxMap ) ;
	for ( auto iter = m_map.begin(); iter != m_map.end(); iter ++ )
	{
		if ( iter->second == index )
		{
			strName = iter->first ;
			return	strName.c_str() ;
		}
	}
	return	nullptr ;
}

// 要素型情報取得
LType LMapObj::GetElementTypeAt( size_t index ) const
{
	if ( m_pClass == nullptr )
	{
		return	LType( LType::typeObject ) ;
	}
	LMapClass *	pMapClass = dynamic_cast<LMapClass*>( m_pClass ) ;
	if ( (pMapClass != nullptr)
		&& (pMapClass->GetElementTypeClass() != nullptr) )
	{
		return	LType( pMapClass->GetElementTypeClass() ) ;
	}
	return	LType( m_pClass->VM().GetObjectClass() ) ;
}

LType LMapObj::GetElementTypeAs( const wchar_t * name ) const
{
	return	LMapObj::GetElementTypeAt(0) ;
}

// 要素設定（pObj には AddRef されたポインタを渡す）
LObject * LMapObj::SetElementAt( size_t index, LObject * pObj )
{
	if ( index >= m_array.size() )
	{
		LContext::ThrowExceptionError( exceptionIndexOutOfBounds ) ;
		LObject::ReleaseRef( pObj ) ;
		return	nullptr ;
	}
	if ( (pObj != nullptr)
		&& (pObj->GetClass() != nullptr)
		&& (m_pClass != nullptr) )
	{
		LMapClass *	pMapClass = dynamic_cast<LMapClass*>( m_pClass ) ;
		if ( (pMapClass != nullptr)
			&& (pMapClass->GetElementTypeClass() != nullptr) )
		{
			if ( !pObj->GetClass()->IsInstanceOf
							( pMapClass->GetElementTypeClass() ) )
			{
				LContext::ThrowExceptionError( exceptionElementTypeMismatch ) ;
			}
		}
	}
	return	LArrayObj::SetElementAt( index, pObj ) ;
}

LObject * LMapObj::SetElementAs( const wchar_t * name, LObject * pObj )
{
	LSpinLock	lockMap( m_mtxMap ) ;
	auto	iter = m_map.find( name ) ;
	if ( iter != m_map.end() )
	{
		return	SetElementAt( iter->second, pObj ) ;
	}
	if ( (pObj != nullptr)
		&& (pObj->GetClass() != nullptr)
		&& (m_pClass != nullptr) )
	{
		LMapClass *	pMapClass = dynamic_cast<LMapClass*>( m_pClass ) ;
		if ( (pMapClass != nullptr)
			&& (pMapClass->GetElementTypeClass() != nullptr) )
		{
			if ( !pObj->GetClass()->IsInstanceOf
							( pMapClass->GetElementTypeClass() ) )
			{
				LContext::ThrowExceptionError( exceptionElementTypeMismatch ) ;
			}
		}
	}
	LSpinLock	lockArray( m_mtxArray ) ;
	size_t	index = m_array.size() ;
	m_array.push_back( LObjPtr( pObj ) ) ;
	m_map.insert( std::make_pair<std::wstring,size_t>( name, (size_t) index ) ) ;
	return	nullptr ;
}

// 文字列として評価
bool LMapObj::AsString( LString& str ) const
{
	return	LMapObj::AsExpression( str ) ;
}

// （式表現に近い）文字列に変換
bool LMapObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	LSpinLock		lockMap( m_mtxMap ) ;
	LSpinLock		lockArray( m_mtxArray ) ;
	LString			strElement ;

	const wchar_t *	pwszStarter = L"{ " ;
	const wchar_t *	pwszCloser = L" }" ;
	const wchar_t *	pwszDelimiter = L", " ;
	const wchar_t *	pwszSeparator = L": " ;
	if ( flags & expressionForJSON )
	{
		pwszStarter = L"{" ;
		pwszCloser = L"}" ;
		pwszDelimiter = L"," ;
		pwszSeparator = L":" ;
	}

	str = pwszStarter ;
	size_t	nCount = 0 ;
	for ( auto iter : m_map )
	{
		if ( nCount >= 1 )
		{
			str += pwszDelimiter ;
		}
		if ( !(flags & expressionForJSON)
			&& LStringParser::IsValidAsSymbol( iter.first.c_str() ) )
		{
			str += iter.first ;
		}
		else
		{
			str += L"\"" ;
			str += LStringParser::EncodeStringLiteral
						( iter.first.c_str(), iter.first.length() ) ;
			str += L"\"" ;
		}
		str += pwszSeparator ;
		//
		LObject *	pObj = nullptr ;
		if ( iter.second < m_array.size() )
		{
			pObj = m_array.at( iter.second ).Ptr() ;
		}
		str += ToExpression( pObj, flags ) ;
		//
		nCount ++ ;
	}
	str += pwszCloser ;
	return	true ;
}

// 型変換（可能なら AddRef されたポインタを返す / 不可能なら nullptr）
LObject * LMapObj::CastClassTo( LClass * pClass )
{
	LObject *	pObj = LObject::CastClassTo( pClass ) ;
	if ( pObj != nullptr )
	{
		return	pObj ;
	}
	LMapClass *	pMapClass = dynamic_cast<LMapClass*>( pClass ) ;
	if ( pMapClass == nullptr )
	{
		return	nullptr ;
	}
	LClass *	pElementType = pMapClass->GetElementTypeClass() ;
	if ( pElementType == nullptr )
	{
		AddRef() ;
		return	this ;
	}
	LMapObj *	pMapObj = new LMapObj( pClass ) ;
	{
		LSpinLock	lockMap( m_mtxMap ) ;
		pMapObj->m_map = m_map ;
	}
	{
		LSpinLock	lockArray( m_mtxArray ) ;
		pMapObj->m_array.resize( m_array.size() ) ;
		for ( size_t i = 0; i < m_array.size(); i ++ )
		{
			pObj = m_array.at(i).Ptr() ;
			if ( pObj != nullptr )
			{
				pObj = pObj->CastClassTo( pElementType ) ;
				if ( pObj == nullptr )
				{
					// 変換できなかった場合は nullptr
					delete	pMapObj ;
					return	nullptr ;
				}
				pMapObj->m_array.at(i).SetPtr( pObj ) ;
			}
		}
	}
	return	pMapObj ;
}

// 複製する（要素も全て複製処理する）
LObject * LMapObj::CloneObject( void ) const
{
	LSpinLock	lockMap( m_mtxMap ) ;
	LSpinLock	lockArray( m_mtxArray ) ;
	LMapObj *	pMapObj = new LMapObj( m_pClass ) ;
	pMapObj->CloneFrom( *this ) ;
	return	pMapObj ;
}

void LMapObj::CloneFrom( const LMapObj& obj )
{
	{
		LSpinLock	lockMap( m_mtxMap ) ;
		m_map = obj.m_map ;
	}
	LArrayObj::CloneFrom( obj ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LMapObj::DuplicateObject( void ) const
{
	LSpinLock	lockMap( m_mtxMap ) ;
	LSpinLock	lockArray( m_mtxArray ) ;
	return	new LMapObj( *this ) ;
}

void LMapObj::DuplicateFrom( const LMapObj& obj )
{
	LArrayObj::DuplicateFrom( obj ) ;

	LSpinLock	lock( m_mtxMap ) ;
	m_map = obj.m_map ;
}

// 内部リソースを解放する
void LMapObj::DisposeObject( void )
{
	LArrayObj::DisposeObject() ;

	LSpinLock	lock( m_mtxMap ) ;
	m_map.clear() ;
}

// 要素の削除
void LMapObj::RemoveElementAt( size_t index )
{
	LArrayObj::RemoveElementAt( index ) ;

	LSpinLock	lock( m_mtxMap ) ;
	auto	iter = m_map.begin() ;
	while ( iter != m_map.end() )
	{
		if ( iter->second > index )
		{
			iter->second -- ;
			iter ++ ;
		}
		else if ( iter->second == index )
		{
			iter = m_map.erase( iter ) ;
		}
		else
		{
			iter ++ ;
		}
	}
}

void LMapObj::RemoveElementAs( const wchar_t * name )
{
	ssize_t	index = FindElementAs( name ) ;
	if ( index < 0 )
	{
		#ifdef	_DEBUG
		LSpinLock	lock( m_mtxMap ) ;
		assert( m_map.find( name ) == m_map.end() ) ;
		#endif
		return ;
	}
	RemoveElementAt( index ) ;
}

void LMapObj::RemoveAll( void )
{
	LArrayObj::RemoveAll() ;

	LSpinLock	lock( m_mtxMap ) ;
	m_map.clear() ;
}

// クラスのメンバやポインタの参照先の構造体に値を設定
bool LMapObj::PutMembers( const LObjPtr& pObj )
{
	LMapClass *	pMapClass = dynamic_cast<LMapClass*>( m_pClass ) ;
	assert( pMapClass != nullptr ) ;
	if ( (pMapClass == nullptr) || (pObj == nullptr) )
	{
		return	false ;
	}
	LObjPtr		pCastObj( pObj->CastClassTo( pMapClass ) ) ;
	LMapObj *	pMapObj = dynamic_cast<LMapObj*>( pCastObj.Ptr() ) ;
	if ( pMapObj == nullptr )
	{
		return	false ;
	}
	CloneFrom( *pMapObj ) ;
	return	true ;
}

// int size() const
void LMapObj::method_size( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LMapObj, pMap ) ;

	LQT_RETURN_LONG( pMap->m_array.size() ) ;
}

// boolean has( String key ) const
void LMapObj::method_has( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LMapObj, pMap ) ;
	LQT_FUNC_ARG_STRING( key ) ;

	LQT_RETURN_BOOL( pMap->FindElementAs( key.c_str() ) >= 0 ) ;
}

// int find( String key ) const
void LMapObj::method_find( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LMapObj, pMap ) ;
	LQT_FUNC_ARG_STRING( key ) ;

	LQT_RETURN_LONG( pMap->FindElementAs( key.c_str() ) ) ;
}

// String keyAt( long index ) const
void LMapObj::method_keyAt( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LMapObj, pMap ) ;
	LQT_FUNC_ARG_LONG( index ) ;

	LString	strName ;
	LQT_RETURN_STRING( pMap->GetElementNameAt( strName, (size_t) index ) ) ;
}

// Object removeAt( long index )
void LMapObj::method_removeAt( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LMapObj, pMap ) ;
	LQT_FUNC_ARG_LONG( index ) ;

	LObjPtr	pObj ;
	if ( (size_t) index < pMap->m_array.size() )
	{
		pObj = pMap->m_array.at( (size_t) index ) ;
		pMap->RemoveElementAt( (size_t) index ) ;
	}
	LQT_RETURN_OBJECT( pObj.Detach() ) ;
}

// Object removeAs( String key )
void LMapObj::method_removeAs( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LMapObj, pMap ) ;
	LQT_FUNC_ARG_STRING( key ) ;

	LObjPtr	pObj ;
	ssize_t	index = pMap->FindElementAs( key.c_str() ) ;
	if ( index >= 0 )
	{
		pObj = pMap->m_array.at( (size_t) index ) ;
		pMap->RemoveElementAt( (size_t) index ) ;
	}
	LQT_RETURN_OBJECT( pObj.Detach() ) ;
}


// := 演算子 : LMapObj*(LMapObj*,LMapObj*)
// Map operator := ( Map obj )
LValue::Primitive LMapObj::operator_smov
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LMapObj *	pMap1 = dynamic_cast<LMapObj*>( val1.pObject ) ;
	if ( pMap1 == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LMapObj *	pMap2 = dynamic_cast<LMapObj*>( val2.pObject ) ;
	if ( pMap2 == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	pMap1->RemoveAll() ;
	pMap1->CloneFrom( *pMap2 ) ;

	pMap1->AddRef() ;
	return	LValue::MakeObjectPtr( pMap1 ) ;
}


