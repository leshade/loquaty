
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// ���[�U�[��`�I�u�W�F�N�g���
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

// �v�f�^���擾
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

// �v�f�̌^���ݒ�
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

// �ێ�����o�b�t�@�ւ̃|�C���^��Ԃ�
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
	m_pPtr = new LPointerObj( pPtrClass ) ;
	m_pPtr->SetPointer( m_pBuf, 0, m_pBuf->size() ) ;
	return	m_pPtr.Get() ;
}

// ������Ƃ��ĕ]��
bool LGenericObj::AsString( LString& str ) const
{
	LString	strElement ;

	str = L"{ " ;
	size_t	nCount = 0 ;
	for ( auto iter : m_map )
	{
		if ( nCount ++ >= 1 )
		{
			str += L", " ;
		}
		str += iter.first ;
		str += L": " ;
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
			str += L", " ;
		}
		str += names.at(i) ;
		str += L": " ;

		LArrangement::Desc	desc ;
		if ( arrange.GetDescAs( desc, names.at(i).c_str() ) )
		{
			LPtr<LPointerObj>	pPtr =
				new LPointerObj
					( m_pClass->VM().GetPointerClassAs( desc.m_type ),
						m_pBuf, desc.m_location ) ;
			if ( pPtr->AsString( strElement ) )
			{
				str += strElement ;
			}
		}
	}

	str += L" }" ;
	return	true ;
}

// ��������i�v�f���S�ĕ�����������j
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

// ��������i�v�f�͎Q�Ƃ���`�ŕ�����������j
LObject * LGenericObj::DuplicateObject( void ) const
{
	LSpinLock	lockMap( m_mtxMap ) ;
	LSpinLock	lockArray( m_mtxArray ) ;
	return	new LGenericObj( *this ) ;
}

// �������\�[�X���������
void LGenericObj::DisposeObject( void )
{
	LMapObj::DisposeObject() ;

	LSpinLock	lockMap( m_mtxMap ) ;
	m_types.clear() ;
	m_pBuf = nullptr ;
}

// �v�f�̍폜
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

// �f�[�^�̔z�u���Ɋ�Â��ăo�b�t�@���m�ۂ���
// �i���Ƀo�b�t�@�������Ă���ꍇ�A�K�v�ɉ����Ċg������j
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


