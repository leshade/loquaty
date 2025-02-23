
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// ポインタ・オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 代入
const LPointerObj& LPointerObj::operator = ( const LPointerObj& ptr )
{
	if ( m_pBuf != ptr.m_pBuf )
	{
		DetachRefChain() ;
		m_pBuf = ptr.m_pBuf ;
		AttachRefChain() ;
	}
	m_pBufPtr = ptr.m_pBufPtr ;
	m_nBufSize = ptr.m_nBufSize ;
	m_iOffset = ptr.m_iOffset ;
	m_iBoundFirst = ptr.m_iBoundFirst ;
	m_iBoundEnd = ptr.m_iBoundEnd ;
	return	*this ;
}


// バッファ参照設定
void LPointerObj::SetPointer
	( std::shared_ptr<LArrayBuffer> pBuf, size_t iOffset, size_t nBounds )
{
	if ( m_pBuf != pBuf )
	{
		DetachRefChain() ;
		m_pBuf = pBuf ;
		AttachRefChain() ;
	}
	if ( pBuf != nullptr )
	{
		m_pBufPtr = pBuf->GetBuffer() ;
		m_nBufSize = pBuf->GetBytes() ;
	}
	else
	{
		m_pBufPtr = nullptr ;
		m_nBufSize = 0 ;
	}
	m_iOffset = iOffset ;
	m_iBoundFirst = iOffset ;
	m_iBoundEnd = std::min( iOffset + nBounds, m_nBufSize ) ;
}

// プリミティブ・ロード（ポインタが無効の場合例外を送出）
LLong LPointerObj::LoadIntegerAt( size_t iOffset, LType::Primitive type ) const
{
	std::uint8_t *	p = GetPointer( iOffset, LType::s_bytesAligned[type] ) ;
	if ( p != nullptr )
	{
		return	(s_pnfLoadAsLong[type])( p ) ;
	}
	else
	{
		LContext::ThrowExceptionError( exceptionPointerOutOfBounds ) ;
		return	0 ;
	}
}

LDouble LPointerObj::LoadDoubleAt( size_t iOffset, LType::Primitive type ) const
{
	std::uint8_t *	p = GetPointer( iOffset, LType::s_bytesAligned[type] ) ;
	if ( p != nullptr )
	{
		return	(s_pnfLoadAsDouble[type])( p ) ;
	}
	else
	{
		LContext::ThrowExceptionError( exceptionPointerOutOfBounds ) ;
		return	0.0 ;
	}
}

const LPointerObj::PFN_LoadPrimitiveAsLong
		LPointerObj::s_pnfLoadAsLong[LType::typePrimitiveCount] =
{
	&LPointerObj::LoadBooleanAsLong,
	&LPointerObj::LoadInt8AsLong,
	&LPointerObj::LoadUint8AsLong,
	&LPointerObj::LoadInt16AsLong,
	&LPointerObj::LoadUint16AsLong,
	&LPointerObj::LoadInt32AsLong,
	&LPointerObj::LoadUint32AsLong,
	&LPointerObj::LoadInt64AsLong,
	&LPointerObj::LoadUint64AsLong,
	&LPointerObj::LoadFloatAsLong,
	&LPointerObj::LoadDoubleAsLong,
} ;

LLong LPointerObj::LoadBooleanAsLong( const std::uint8_t * p )
{
	return	*((LBoolean*) p) ;
}

LLong LPointerObj::LoadInt8AsLong( const std::uint8_t * p )
{
	return	*((LInt8*) p) ;
}

LLong LPointerObj::LoadUint8AsLong( const std::uint8_t * p )
{
	return	*((LUint8*) p) ;
}

LLong LPointerObj::LoadInt16AsLong( const std::uint8_t * p )
{
	return	*((LInt16*) p) ;
}

LLong LPointerObj::LoadUint16AsLong( const std::uint8_t * p )
{
	return	*((LUint16*) p) ;
}

LLong LPointerObj::LoadInt32AsLong( const std::uint8_t * p )
{
	return	*((LInt32*) p) ;
}

LLong LPointerObj::LoadUint32AsLong( const std::uint8_t * p )
{
	return	*((LUint32*) p) ;
}

LLong LPointerObj::LoadInt64AsLong( const std::uint8_t * p )
{
	return	*((LInt64*) p) ;
}

LLong LPointerObj::LoadUint64AsLong( const std::uint8_t * p )
{
	return	(LLong) *((LUint64*) p) ;
}

LLong LPointerObj::LoadFloatAsLong( const std::uint8_t * p )
{
	return	(LLong) *((LFloat*) p) ;
}

LLong LPointerObj::LoadDoubleAsLong( const std::uint8_t * p )
{
	return	(LLong) *((LDouble*) p) ;
}


const LPointerObj::PFN_LoadPrimitiveAsDouble
		LPointerObj::s_pnfLoadAsDouble[LType::typePrimitiveCount] =
{
	&LPointerObj::LoadBooleanAsDouble,
	&LPointerObj::LoadInt8AsDouble,
	&LPointerObj::LoadUint8AsDouble,
	&LPointerObj::LoadInt16AsDouble,
	&LPointerObj::LoadUint16AsDouble,
	&LPointerObj::LoadInt32AsDouble,
	&LPointerObj::LoadUint32AsDouble,
	&LPointerObj::LoadInt64AsDouble,
	&LPointerObj::LoadUint64AsDouble,
	&LPointerObj::LoadFloatAsDouble,
	&LPointerObj::LoadDoubleAsDouble,
} ;

LDouble LPointerObj::LoadBooleanAsDouble( const std::uint8_t * p )
{
	return	(LDouble) ((int) *((LBoolean*) p)) ;
}

LDouble LPointerObj::LoadInt8AsDouble( const std::uint8_t * p )
{
	return	*((LInt8*) p) ;
}

LDouble LPointerObj::LoadUint8AsDouble( const std::uint8_t * p )
{
	return	*((LUint8*) p) ;
}

LDouble LPointerObj::LoadInt16AsDouble( const std::uint8_t * p )
{
	return	*((LInt16*) p) ;
}

LDouble LPointerObj::LoadUint16AsDouble( const std::uint8_t * p )
{
	return	*((LUint16*) p) ;
}

LDouble LPointerObj::LoadInt32AsDouble( const std::uint8_t * p )
{
	return	*((LInt32*) p) ;
}

LDouble LPointerObj::LoadUint32AsDouble( const std::uint8_t * p )
{
	return	*((LUint32*) p) ;
}

LDouble LPointerObj::LoadInt64AsDouble( const std::uint8_t * p )
{
	return	(LDouble) *((LInt64*) p) ;
}

LDouble LPointerObj::LoadUint64AsDouble( const std::uint8_t * p )
{
	return	(LDouble) *((LUint64*) p) ;
}

LDouble LPointerObj::LoadFloatAsDouble( const std::uint8_t * p )
{
	return	*((LFloat*) p) ;
}

LDouble LPointerObj::LoadDoubleAsDouble( const std::uint8_t * p )
{
	return	*((LDouble*) p) ;
}


// プリミティブ・ストア（ポインタが無効の場合例外を送出）
void LPointerObj::StoreIntegerAt
	( size_t iOffset, LType::Primitive type, LLong val ) const
{
	std::uint8_t *	p = GetPointer( iOffset, LType::s_bytesAligned[type] ) ;
	if ( p != nullptr )
	{
		(s_pnfStoreAsLong[type])( p, val ) ;
	}
	else
	{
		LContext::ThrowExceptionError( exceptionPointerOutOfBounds ) ;
	}
}

void LPointerObj::StoreDoubleAt
	( size_t iOffset, LType::Primitive type, LDouble val ) const
{
	std::uint8_t *	p = GetPointer( iOffset, LType::s_bytesAligned[type] ) ;
	if ( p != nullptr )
	{
		(s_pnfStoreAsDouble[type])( p, val ) ;
	}
	else
	{
		LContext::ThrowExceptionError( exceptionPointerOutOfBounds ) ;
	}
}

// 実行時の型情報を使って値をストア（例外は送出しない）
bool LPointerObj::PutInteger( LLong val ) const
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	false ;
	}
	if ( !pPtrClass->GetBufferType().IsPrimitive() )
	{
		return	false ;
	}
	const LType&		typeData = pPtrClass->GetBufferType() ;
	LType::Primitive	type = typeData.GetPrimitive() ;
	std::uint8_t *		ptr = GetPointer( 0, LType::s_bytesAligned[type] ) ;
	if ( ptr == nullptr )
	{
		return	false ;
	}
	(s_pnfStoreAsLong[type])( ptr, val ) ;
	return	true ;
}

bool LPointerObj::PutDouble( LDouble val ) const
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	false ;
	}
	if ( !pPtrClass->GetBufferType().IsPrimitive() )
	{
		return	false ;
	}
	const LType&		typeData = pPtrClass->GetBufferType() ;
	LType::Primitive	type = typeData.GetPrimitive() ;
	std::uint8_t *		ptr = GetPointer( 0, LType::s_bytesAligned[type] ) ;
	if ( ptr == nullptr )
	{
		return	false ;
	}
	(s_pnfStoreAsDouble[type])( ptr, val ) ;
	return	true ;
}

// クラスのメンバやポインタの参照先の構造体に値を設定
bool LPointerObj::PutMembers( const LValue& val )
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	false ;
	}
	return	PutMembers( 0, pPtrClass->GetBufferType(), val ) ;
}

bool LPointerObj::PutMembers
		( size_t iOffset, const LType& type, const LValue& val )
{
	if ( val.GetType().IsArray() )
	{
		// 配列をストア
		LObjPtr	pSrcObj = val.GetObject() ;
		if ( pSrcObj == nullptr )
		{
			return	false ;
		}
		LType	typeElement = type ;
		if ( type.IsDataArray() )
		{
			LDataArrayClass *	pArrayClass = type.GetDataArrayClass() ;
			assert( pArrayClass != nullptr ) ;
			typeElement = pArrayClass->GetElementType() ;
		}
		bool			flagSucceeded = true ;
		const size_t	nDataBytes = typeElement.GetDataBytes() ;
		const size_t	nCount = pSrcObj->GetElementCount() ;
		for ( size_t i = 0; i < nCount; i ++ )
		{
			LObjPtr	pElement( pSrcObj->GetElementAt( i ) ) ;
			if ( pElement != nullptr )
			{
				if ( !PutMembers
					( iOffset + i * nDataBytes,
							typeElement, LValue(pSrcObj) ) )
				{
					flagSucceeded = false ;
				}
			}
		}
		return	flagSucceeded ;
	}
	if ( type.IsPrimitive() )
	{
		// プリミティブ型にストア
		LType::Primitive	primType = type.GetPrimitive() ;
		std::uint8_t *		ptr =
				GetPointer( iOffset, LType::s_bytesAligned[primType] ) ;
		if ( ptr == nullptr )
		{
			return	false ;
		}
		if ( type.IsInteger() )
		{
			(s_pnfStoreAsLong[primType])( ptr, val.AsInteger() ) ;
		}
		else if ( type.IsFloatingPointNumber() )
		{
			(s_pnfStoreAsDouble[primType])( ptr, val.AsDouble() ) ;
		}
		else
		{
			return	false ;
		}
		return	true ;
	}
	if ( type.IsStructure() )
	{
		// 構造体
		LStructureClass *	pStructClass = type.GetStructureClass() ;
		assert( pStructClass != nullptr ) ;
		const LArrangementBuffer&
				arrangeBuf = pStructClass->GetProtoArrangemenet() ;

		LObjPtr	pSrcObj = val.GetObject() ;
		if ( pSrcObj == nullptr )
		{
			return	false ;
		}
		LString			strName ;
		bool			flagSucceeded = true ;
		const size_t	nCount = pSrcObj->GetElementCount() ;
		for ( size_t i = 0; i < nCount; i ++ )
		{
			const wchar_t *	pwszName = pSrcObj->GetElementNameAt( strName, i ) ;
			if ( pwszName == nullptr )
			{
				flagSucceeded = false ;
				continue ;
			}
			LObjPtr	pElement( pSrcObj->GetElementAt( i ) ) ;
			if ( pElement == nullptr )
			{
				continue ;
			}
			LArrangementBuffer::Desc	desc ;
			if ( !arrangeBuf.GetDescAs( desc, pwszName ) )
			{
				flagSucceeded = false ;
				continue ;
			}
			if ( !PutMembers
				( iOffset + desc.m_location,
						desc.m_type, LValue(pElement) ) )
			{
				flagSucceeded = false ;
			}
		}
		return	flagSucceeded ;
	}
	return	false ;
}


const LPointerObj::PFN_StorePrimitiveAsLong
		LPointerObj::s_pnfStoreAsLong[LType::typePrimitiveCount] =
{
	&LPointerObj::StoreBooleanAsLong,
	&LPointerObj::StoreInt8AsLong,
	&LPointerObj::StoreUint8AsLong,
	&LPointerObj::StoreInt16AsLong,
	&LPointerObj::StoreUint16AsLong,
	&LPointerObj::StoreInt32AsLong,
	&LPointerObj::StoreUint32AsLong,
	&LPointerObj::StoreInt64AsLong,
	&LPointerObj::StoreUint64AsLong,
	&LPointerObj::StoreFloatAsLong,
	&LPointerObj::StoreDoubleAsLong,
} ;

void LPointerObj::StoreBooleanAsLong( std::uint8_t * p, LLong val )
{
	*((LBoolean*) p) = (LBoolean) val ;
}

void LPointerObj::StoreInt8AsLong( std::uint8_t * p, LLong val )
{
	*((LInt8*) p) = (LInt8) val ;
}

void LPointerObj::StoreUint8AsLong( std::uint8_t * p, LLong val )
{
	*((LUint8*) p) = (LUint8) val ;
}

void LPointerObj::StoreInt16AsLong( std::uint8_t * p, LLong val )
{
	*((LInt16*) p) = (LInt16) val ;
}

void LPointerObj::StoreUint16AsLong( std::uint8_t * p, LLong val )
{
	*((LUint16*) p) = (LUint16) val ;
}

void LPointerObj::StoreInt32AsLong( std::uint8_t * p, LLong val )
{
	*((LInt32*) p) = (LInt32) val ;
}

void LPointerObj::StoreUint32AsLong( std::uint8_t * p, LLong val )
{
	*((LUint32*) p) = (LUint32) val ;
}

void LPointerObj::StoreInt64AsLong( std::uint8_t * p, LLong val )
{
	*((LInt64*) p) = val ;
}

void LPointerObj::StoreUint64AsLong( std::uint8_t * p, LLong val )
{
	*((LUint64*) p) = (LUint64) val ;
}

void LPointerObj::StoreFloatAsLong( std::uint8_t * p, LLong val )
{
	*((LFloat*) p) = (LFloat) val ;
}

void LPointerObj::StoreDoubleAsLong( std::uint8_t * p, LLong val )
{
	*((LDouble*) p) = (LDouble) val ;
}

const LPointerObj::PFN_StorePrimitiveAsDouble
		LPointerObj::s_pnfStoreAsDouble[LType::typePrimitiveCount] =
{
	&LPointerObj::StoreBooleanAsDouble,
	&LPointerObj::StoreInt8AsDouble,
	&LPointerObj::StoreUint8AsDouble,
	&LPointerObj::StoreInt16AsDouble,
	&LPointerObj::StoreUint16AsDouble,
	&LPointerObj::StoreInt32AsDouble,
	&LPointerObj::StoreUint32AsDouble,
	&LPointerObj::StoreInt64AsDouble,
	&LPointerObj::StoreUint64AsDouble,
	&LPointerObj::StoreFloatAsDouble,
	&LPointerObj::StoreDoubleAsDouble,
} ;

void LPointerObj::StoreBooleanAsDouble( std::uint8_t * p, LDouble val )
{
	*((LBoolean*) p) = (val != 0.0) ;
}

void LPointerObj::StoreInt8AsDouble( std::uint8_t * p, LDouble val )
{
	*((LInt8*) p) = (LInt8) val ;
}

void LPointerObj::StoreUint8AsDouble( std::uint8_t * p, LDouble val )
{
	*((LUint8*) p) = (LUint8) val ;
}

void LPointerObj::StoreInt16AsDouble( std::uint8_t * p, LDouble val )
{
	*((LInt16*) p) = (LInt16) val ;
}

void LPointerObj::StoreUint16AsDouble( std::uint8_t * p, LDouble val )
{
	*((LUint16*) p) = (LUint16) val ;
}

void LPointerObj::StoreInt32AsDouble( std::uint8_t * p, LDouble val )
{
	*((LInt32*) p) = (LInt32) val ;
}

void LPointerObj::StoreUint32AsDouble( std::uint8_t * p, LDouble val )
{
	*((LUint32*) p) = (LUint32) val ;
}

void LPointerObj::StoreInt64AsDouble( std::uint8_t * p, LDouble val )
{
	*((LInt64*) p) = (LInt64) val ;
}

void LPointerObj::StoreUint64AsDouble( std::uint8_t * p, LDouble val )
{
	*((LUint64*) p) = (LUint64) val ;
}

void LPointerObj::StoreFloatAsDouble( std::uint8_t * p, LDouble val )
{
	*((LFloat*) p) = (LFloat) val ;
}

void LPointerObj::StoreDoubleAsDouble( std::uint8_t * p, LDouble val )
{
	*((LDouble*) p) = val ;
}


// バッファアドレス更新通知
void LPointerObj::OnBufferReallocated( void )
{
	if ( m_pBuf != nullptr )
	{
		m_pBufPtr = m_pBuf->GetBuffer() ;
		m_nBufSize = m_pBuf->GetBytes() ;
		m_iBoundEnd = std::min( m_iBoundEnd, m_nBufSize ) ;
	}
}

// 要素数
size_t LPointerObj::GetElementCount( void ) const
{
	return	GetBytes() ;
}

// 保持するバッファへのポインタを返す
LPointerObj * LPointerObj::GetBufferPoiner( void )
{
	AddRef() ;
	return	this ;
}

// 整数値として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LPointerObj::AsInteger( LLong& value ) const
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	false ;
	}
	if ( !pPtrClass->GetBufferType().IsPrimitive() )
	{
		return	false ;
	}
	const LType&		typeData = pPtrClass->GetBufferType() ;
	LType::Primitive	type = typeData.GetPrimitive() ;
	std::uint8_t *		ptr = GetPointer( 0, LType::s_bytesAligned[type] ) ;
	if ( ptr == nullptr )
	{
		return	false ;
	}
	value = (s_pnfLoadAsLong[type])( ptr ) ;
	return	true ;
}

// 浮動小数点として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LPointerObj::AsDouble( LDouble& value ) const
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	false ;
	}
	if ( !pPtrClass->GetBufferType().IsPrimitive() )
	{
		return	false ;
	}
	const LType&		typeData = pPtrClass->GetBufferType() ;
	LType::Primitive	type = typeData.GetPrimitive() ;
	std::uint8_t *		ptr = GetPointer( 0, LType::s_bytesAligned[type] ) ;
	if ( ptr == nullptr )
	{
		return	false ;
	}
	value = (s_pnfLoadAsDouble[type])( ptr ) ;
	return	true ;
}

// 文字列として評価
bool LPointerObj::AsString( LString& str ) const
{
	return	LPointerObj::AsExpression( str ) ;
}

bool LPointerObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	if ( GetPointer() == nullptr )
	{
		str = L"null" ;
		return	true ;
	}
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	false ;
	}
	const LType&	typeData = pPtrClass->GetBufferType() ;
	std::uint8_t *	ptr = GetPointer( 0, typeData.GetDataBytes() ) ;
	if ( ptr == nullptr )
	{
		str = L"null" ;
		return	true ;
	}
	if ( typeData.IsPrimitive() )
	{
		// プリミティブ型
		if ( typeData.IsBoolean() )
		{
			str = (*ptr != 0) ? L"true" : L"false" ;
			return	true ;
		}
		else if ( typeData.IsInteger() )
		{
			ExprIntAsString
				( str, (s_pnfLoadAsLong[typeData.GetPrimitive()])( ptr ) ) ;
			return	true ;
		}
		else
		{
			assert( typeData.IsFloatingPointNumber() ) ;
			str.SetNumberOf( (s_pnfLoadAsDouble[typeData.GetPrimitive()])( ptr ), 14, true ) ;
			return	true ;
		}
	}
	if ( typeData.IsDataArray() )
	{
		// 配列型
		LDataArrayClass *	pArrayClass = typeData.GetDataArrayClass() ;
		assert( pArrayClass != nullptr ) ;

		const LType			typeElement = pArrayClass->GetElementType() ;
		const size_t		nCount = pArrayClass->GetArrayElementCount() ;
		LPtr<LPointerObj>	pNext
			( new LPointerObj
				( m_pClass->VM().
					GetPointerClassAs( typeElement ), m_pBuf, m_iOffset ) ) ;

		const wchar_t *	pwszStarter = L"[ " ;
		const wchar_t *	pwszCloser = L" ]" ;
		const wchar_t *	pwszDelimiter = L", " ;
		if ( flags & expressionForJSON )
		{
			pwszStarter = L"{" ;
			pwszCloser = L"}" ;
			pwszDelimiter = L"," ;
		}

		str = pwszStarter ;
		for ( size_t i = 0; i < nCount; i ++ )
		{
			if ( i > 0 )
			{
				str += pwszDelimiter ;
			}
			LString	strElement ;
			if ( pNext->AsString( strElement ) )
			{
				str += strElement ;
			}
			*pNext += typeElement.GetDataBytes() ;
		}
		str += pwszCloser ;
		return	true ;
	}
	if ( typeData.IsStructure() )
	{
		LStructureClass *	pStructClass = typeData.GetStructureClass() ;
		assert( pStructClass != nullptr ) ;

		const LArrangementBuffer&
					arrange = pStructClass->GetProtoArrangemenet() ;

		std::vector<LString>	names ;
		arrange.GetOrderedNameList( names ) ;

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
		for ( size_t i = 0; i < names.size(); i ++ )
		{
			if ( i > 0 )
			{
				str += pwszDelimiter ;
			}
			LArrangement::Desc	desc ;
			if ( arrange.GetDescAs( desc, names.at(i).c_str() ) )
			{
				if ( flags & expressionForJSON )
				{
					str += L"\"" ;
					str += LStringParser::EncodeStringLiteral
								( names.at(i).c_str(), names.at(i).length() ) ;
					str += L"\"" ;
				}
				else
				{
					str += names.at(i) ;
				}
				str += pwszSeparator ;

				LPtr<LPointerObj>	pElement
					( new LPointerObj
						( m_pClass->VM().GetPointerClassAs( desc.m_type ),
							m_pBuf, m_iOffset + desc.m_location ) ) ;
				LString	strElement ;
				if ( pElement->AsString( strElement ) )
				{
					str += strElement ;
				}
			}
		}
		str += pwszCloser ;
		return	true ;
	}
	return	false ;
}

void LPointerObj::ExprIntAsString( LString& str, LLong value )
{
	str.SetIntegerOf( value ) ;

	if ( value > 0 )
	{
		LString	hex ;
		hex.SetIntegerOf( value, (int) str.GetLength(), 16 ) ;

		if ( EntropyOfNumString( hex ) < EntropyOfNumString( str ) )
		{
			hex.SetIntegerOf( value, 0, 16 ) ;
			str = L"0x" ;
			str += hex ;
		}
	}
}

double LPointerObj::EntropyOfNumString( const LString& str )
{
	double	p[16] ;
	for ( int i = 0; i < 16; i ++ )
	{
		p[i] = 0.0 ;
	}
	size_t	n = 0 ;
	for ( size_t i = 0; i < str.GetLength(); i ++ )
	{
		wchar_t	wch = str.GetAt( i ) ;
		if ( (wch >= L'0') && (wch <= L'9') )
		{
			p[wch - L'0'] += 1.0 ;
			n ++ ;
		}
		else if ( (wch >= L'A') && (wch <= L'F') )
		{
			p[wch - L'A' + 10] += 1.0 ;
			n ++ ;
		}
		else if ( (wch >= L'a') && (wch <= L'f') )
		{
			p[wch - L'a' + 10] += 1.0 ;
			n ++ ;
		}
	}
	if ( n == 0 )
	{
		return	0.0 ;
	}
	for ( int i = 0; i < 16; i ++ )
	{
		p[i] /= (double) n ;
	}
	double	e = 0.0 ;
	for ( size_t i = 0; i < str.GetLength(); i ++ )
	{
		wchar_t	wch = str.GetAt( i ) ;
		if ( (wch >= L'0') && (wch <= L'9') )
		{
			e -= log( p[wch - L'0'] ) ;
		}
		else if ( (wch >= L'A') && (wch <= L'F') )
		{
			e -= log( p[wch - L'A' + 10] ) ;
		}
		else if ( (wch >= L'a') && (wch <= L'f') )
		{
			e -= log( p[wch - L'a' + 10] ) ;
		}
	}
	return	e ;
}

// 型変換（可能なら AddRef されたポインタを返す / 不可能なら nullptr）
LObject * LPointerObj::CastClassTo( LClass * pClass )
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	nullptr ;
	}
	LPointerObj *	pPtrObj = new LPointerObj( pClass ) ;
	*pPtrObj = *this ;
	return	pPtrObj ;
}

// 複製する（要素も全て複製処理する）
LObject * LPointerObj::CloneObject( void ) const
{
	return	new LPointerObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LPointerObj::DuplicateObject( void ) const
{
	return	new LPointerObj( *this ) ;
}

// void copy( Pointer pSrc, long bytes )
void LPointerObj::method_copy( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LPointerObj, pPtr ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, pSrc ) ;
	LQT_VERIFY_NULL_PTR( pSrc ) ;
	LQT_FUNC_ARG_LONG( bytes ) ;

	std::uint8_t *	pDstBuf = pPtr->GetPointer( 0, (size_t) bytes ) ;
	std::uint8_t *	pSrcBuf = pSrc->GetPointer( 0, (size_t) bytes ) ;
	LQT_VERIFY_NULL_PTR( pDstBuf ) ;
	LQT_VERIFY_NULL_PTR( pSrcBuf ) ;

	memmove( pDstBuf, pSrcBuf, (size_t) bytes ) ;

	LQT_RETURN_VOID() ;
}

// void copyTo( Pointer pDst, long bytes )
void LPointerObj::method_copyTo( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LPointerObj, pPtr ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, pDst ) ;
	LQT_VERIFY_NULL_PTR( pDst ) ;
	LQT_FUNC_ARG_LONG( bytes ) ;

	std::uint8_t *	pSrcBuf = pPtr->GetPointer( 0, (size_t) bytes ) ;
	std::uint8_t *	pDstBuf = pDst->GetPointer( 0, (size_t) bytes ) ;
	LQT_VERIFY_NULL_PTR( pSrcBuf ) ;
	LQT_VERIFY_NULL_PTR( pDstBuf ) ;

	memmove( pDstBuf, pSrcBuf, (size_t) bytes ) ;

	LQT_RETURN_VOID() ;
}


