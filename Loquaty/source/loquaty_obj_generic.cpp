
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// ユーザー定義オブジェクト基底
//////////////////////////////////////////////////////////////////////////////

LGenericObj::LGenericObj( LClass * pClass )
	: LMapObj( pClass )
{
	if ( (pClass != nullptr)
		&& (pClass->GetProtoArrangemenet().GetAlignedBytes() > 0) )
	{
		std::shared_ptr<LArrayBufStorage>
				pBuf = pClass->GetProtoArrangemenet().GetBuffer() ;
		if ( pBuf != nullptr )
		{
			m_pBuf = std::make_shared<LArrayBufStorage>( *pBuf ) ;
		}
	}
}

LGenericObj::LGenericObj( const LGenericObj& obj )
	: LMapObj( obj ),
		m_types( obj.m_types ),
		m_pBuf( (obj.m_pBuf == nullptr) ? nullptr
				: std::make_shared<LArrayBufStorage>( *(obj.m_pBuf) ) )
{
}

// 要素型情報取得
LType LGenericObj::GetElementTypeAt( size_t index ) const
{
	LSpinLock	lockMap( m_mtxMap ) ;
	if ( index < m_types.size() )
	{
		return	m_types.at( index ) ;
	}
	return	LType() ;
}

LType LGenericObj::GetElementTypeAs( const wchar_t * name ) const
{
	ssize_t	index = FindElementAs( name ) ;
	if ( index >= 0 )
	{
		return	GetElementTypeAt( (size_t) index ) ;
	}
	return	LType() ;
}

// 要素の型情報設定
void LGenericObj::SetElementTypeAt( size_t index, const LType& type )
{
	LSpinLock	lockMap( m_mtxMap ) ;
	if ( index >= m_types.size() )
	{
		assert( index < LArrayObj::m_array.size() ) ;
		if ( index >= LArrayObj::m_array.size() )
		{
			return ;
		}
		m_types.resize( index + 1, LType() ) ;
	}
	m_types.at(index) = type ;
}

void LGenericObj::SetElementTypeAs( const wchar_t * name, const LType& type )
{
	ssize_t	index = FindElementAs( name ) ;
	assert( index >= 0 ) ;
	if ( index >= 0 )
	{
		SetElementTypeAt( (size_t) index, type ) ;
	}
}

// 保持するバッファへのポインタを返す
LPointerObj * LGenericObj::GetBufferPoiner( void )
{
	if ( m_pPtr != nullptr )
	{
		return	m_pPtr.Get() ;
	}
	if ( m_pBuf == nullptr )
	{
		return	nullptr ;
	}
	LSpinLock	lockMap( m_mtxMap ) ;
	assert( m_pClass != nullptr ) ;
	LClass *	pPtrClass = m_pClass->VM().GetPointerClass() ;
	m_pPtr.SetPtr( new LPointerObj( pPtrClass ) ) ;
	m_pPtr->SetPointer( m_pBuf, 0, m_pBuf->size() ) ;
	return	m_pPtr.Get() ;
}

// 文字列として評価
bool LGenericObj::AsString( LString& str ) const
{
	return	LGenericObj::AsExpression( str ) ;
}

// （式表現に近い）文字列に変換
bool LGenericObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	LSpinLock	lockMap( m_mtxMap ) ;
	LSpinLock	lockArray( m_mtxArray ) ;


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
	LString	strElement ;

	for ( auto iter : m_map )
	{
		if ( nCount ++ >= 1 )
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
		str += ToExpression( pObj ) ;
	}

	assert( m_pClass != nullptr ) ;
	std::vector<LString>	names ;
	const LArrangementBuffer&
				arrange = m_pClass->GetProtoArrangemenet() ;
	arrange.GetOrderedNameList( names ) ;

	for ( size_t i = 0; i < names.size(); i ++ )
	{
		if ( nCount ++ >= 1 )
		{
			str += pwszDelimiter ;
		}
		if ( !(flags & expressionForJSON)
			&& LStringParser::IsValidAsSymbol( names.at(i).c_str() ) )
		{
			str += names.at(i) ;
		}
		else
		{
			str += L"\"" ;
			str += LStringParser::EncodeStringLiteral
						( names.at(i).c_str(), names.at(i).length() ) ;
			str += L"\"" ;
		}
		str += pwszSeparator ;

		LArrangement::Desc	desc ;
		if ( arrange.GetDescAs( desc, names.at(i).c_str() ) )
		{
			LPtr<LPointerObj>	pPtr
				( new LPointerObj
					( m_pClass->VM().GetPointerClassAs( desc.m_type ),
						m_pBuf, desc.m_location ) ) ;
			if ( pPtr->AsString( strElement ) )
			{
				str += strElement ;
			}
		}
	}

	str += pwszCloser ;
	return	true ;
}

// 複製する（要素も全て複製処理する）
LObject * LGenericObj::CloneObject( void ) const
{
	LGenericObj *	pGenObj = new LGenericObj( m_pClass ) ;
	pGenObj->CloneFrom( *this ) ;
	return	pGenObj ;
}

void LGenericObj::CloneFrom( const LGenericObj& obj )
{
	if ( obj.m_pBuf != nullptr )
	{
		m_pBuf = std::make_shared<LArrayBufStorage>( *(obj.m_pBuf) ) ;
	}

	LSpinLock	lockMap( obj.m_mtxMap ) ;
	LSpinLock	lockArray( obj.m_mtxArray ) ;
	m_types = obj.m_types ;
	LMapObj::CloneFrom( obj ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LGenericObj::DuplicateObject( void ) const
{
	LSpinLock	lockMap( m_mtxMap ) ;
	LSpinLock	lockArray( m_mtxArray ) ;
	return	new LGenericObj( *this ) ;
}

// 内部リソースを解放する
void LGenericObj::DisposeObject( void )
{
	LMapObj::DisposeObject() ;

	LSpinLock	lockMap( m_mtxMap ) ;
	m_types.clear() ;
	m_pBuf = nullptr ;
}

// 要素の削除
void LGenericObj::RemoveElementAt( size_t index )
{
	LMapObj::RemoveElementAt( index ) ;

	LSpinLock	lockMap( m_mtxMap ) ;
	if ( index < m_types.size() )
	{
		m_types.erase( m_types.begin() + index ) ;
	}
}

void LGenericObj::RemoveAll( void )
{
	LMapObj::RemoveAll() ;

	LSpinLock	lockMap( m_mtxMap ) ;
	m_types.clear() ;
}

// クラスのメンバやポインタの参照先の構造体に値を設定
bool LGenericObj::PutMembers( const LObjPtr& pObj )
{
	if ( pObj == nullptr )
	{
		return	false ;
	}
	assert( m_pClass != nullptr ) ;
	const LArrangementBuffer&	arrangeBuf = m_pClass->GetProtoArrangemenet() ;

	LArrangement::Desc	desc ;
	LString				strName ;
	LType				type ;

	bool	flagFaild = false ;
	size_t	nCount = pObj->GetElementCount() ;
	for ( size_t i = 0; i < nCount; i ++ )
	{
		const wchar_t *	pwszName = pObj->GetElementNameAt( strName, i ) ;
		if ( pwszName == nullptr )
		{
			flagFaild = true ;
			continue ;
		}
		LObjPtr			pSrcObj( pObj->GetElementAt( i ) ) ;
		const ssize_t	iElement = FindElementAs( pwszName ) ;
		if ( iElement >= 0 )
		{
			type = GetElementTypeAt( (size_t) iElement ) ;
			if ( type.GetClass() != nullptr )
			{
				pSrcObj.SetPtr( pSrcObj->CastClassTo( type.GetClass() ) ) ;
			}
			LObject::ReleaseRef( SetElementAt( i, pSrcObj.Get() ) ) ;
		}
		else if ( arrangeBuf.GetDescAs( desc, pwszName ) )
		{
			LPtr<LPointerObj>	pPtrObj( GetBufferPoiner() ) ;
			if ( pPtrObj != nullptr )
			{
				if ( !pPtrObj->PutMembers
						( desc.m_location, desc.m_type, LValue(pSrcObj) ) )
				{
					flagFaild = true ;
				}
			}
		}
		else
		{
			flagFaild = true ;
		}
	}
	return	!flagFaild ;
}

// データの配置情報に基づいてバッファを確保する
// （既にバッファを持っている場合、必要に応じて拡張する）
void LGenericObj::AllocateDataBuffer( void )
{
	if ( (m_pClass != nullptr)
		&& (m_pClass->GetProtoArrangemenet().GetAlignedBytes() > 0) )
	{
		const size_t	nBytes = m_pClass->GetProtoArrangemenet().GetAlignedBytes() ;
		std::shared_ptr<LArrayBufStorage>
						pBuf = m_pClass->GetProtoArrangemenet().GetBuffer() ;
		if ( m_pBuf == nullptr )
		{
			if ( pBuf != nullptr )
			{
				m_pBuf = std::make_shared<LArrayBufStorage>( *pBuf ) ;
			}
			else
			{
				m_pBuf = std::make_shared<LArrayBufStorage>( nBytes ) ;
			}
		}
		else if ( m_pBuf->size() < nBytes )
		{
			const size_t	nOldBytes = m_pBuf->size() ;
			m_pBuf->resize( nBytes, 0 ) ;
			//
			if ( pBuf != nullptr )
			{
				memcpy( m_pBuf->GetBuffer() + nOldBytes,
						pBuf->GetBuffer() + nOldBytes,
						nBytes - nOldBytes ) ;
			}
		}
	}
}


