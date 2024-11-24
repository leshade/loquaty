
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 抽象バッファ
//////////////////////////////////////////////////////////////////////////////

// バッファアドレスやサイズの変更を参照ポインタに通知する
void LArrayBuffer::NotifyBufferReallocation( void )
{
	LSpinLock	lock( m_mutex ) ;

	LPointerObj *	pChain = m_pChainFirst ;
	while ( pChain != nullptr )
	{
		pChain->OnBufferReallocated() ;
		pChain = pChain->m_pChainNext ;
	}
}

// 参照チェーンに追加する
void LArrayBuffer::AttachRefChain( LPointerObj * pPtr )
{
	assert( pPtr->m_pChainPrev == nullptr ) ;
	assert( pPtr->m_pChainNext == nullptr ) ;

	LSpinLock	lock( m_mutex ) ;

	pPtr->m_pChainPrev = nullptr ;
	pPtr->m_pChainNext = m_pChainFirst ;

	if ( m_pChainFirst != nullptr )
	{
		m_pChainFirst->m_pChainPrev = pPtr ;
	}

	m_pChainFirst = pPtr ;
}

// 参照チェーンから取り外す
void LArrayBuffer::DetachRefChain( LPointerObj * pPtr )
{
	LSpinLock	lock( m_mutex ) ;

	LPointerObj *const	pPrev = pPtr->m_pChainPrev ;
	LPointerObj *const	pNext  = pPtr->m_pChainNext ;

	if ( pPrev != nullptr )
	{
		pPrev->m_pChainNext = pNext ;
		if ( pNext != nullptr )
		{
			pNext->m_pChainPrev = pPrev ;
		}
	}
	else
	{
		assert( m_pChainFirst == pPtr ) ;
		m_pChainFirst = pNext ;
		if ( pNext != nullptr )
		{
			pNext->m_pChainPrev = nullptr ;
		}
	}
	pPtr->m_pChainPrev = nullptr ;
	pPtr->m_pChainNext = nullptr ;
}


