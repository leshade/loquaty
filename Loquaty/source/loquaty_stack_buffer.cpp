
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////
// 実行スタック
//////////////////////////////////////////////////////////////////////////

// バッファサイズ取得（Byte 単位）（LArrayBuffer オーバーライド）
size_t LStackBuffer::GetBytes( void ) const
{
	return	m_sp * sizeof(Word) ;
}

// 有効なバッファサイズ拡張
void LStackBuffer::ExpandBuffer( size_t nReqLength )
{
	size_t	nLen = GetLength() ;
	if ( nReqLength > nLen )
	{
		if ( nLen < 0x100 )
		{
			nLen = 0x100 ;
		}
		while ( nLen < nReqLength )
		{
			nLen *= 2 ;
		}
		LArrayBufStorage::resize( nLen * sizeof(Word), 0 ) ;

		m_length = nLen ;
		m_ptr = reinterpret_cast<Word*>( LArrayBufStorage::data() ) ;

		NotifyBufferReallocation() ;
	}
}

