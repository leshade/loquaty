
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// String オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 要素数
size_t LStringObj::GetElementCount( void ) const
{
	return	m_string.GetLength() ;
}

// 要素取得
// （返り値は呼び出し側の責任で ReleaseRef）
LObject * LStringObj::GetElementAt( size_t index ) const
{
	if ( index < m_string.GetLength() )
	{
		LIntegerClass *	pIntClass = m_pClass->VM().GetIntegerObjClass() ;
		return	new LIntegerObj( pIntClass, m_string.GetAt(index) ) ;
	}
	return	nullptr ;
}

// 要素型情報取得
LType LStringObj::GetElementTypeAt( size_t index ) const
{
	return	LType( LType::typeUint32 ) ;
}

// 整数値として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LStringObj::AsInteger( LLong& value ) const
{
	bool	hasNumber = false ;
	value = m_string.AsInteger( &hasNumber ) ;
	return	hasNumber ;
}

// 浮動小数点として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LStringObj::AsDouble( LDouble& value ) const
{
	bool	hasNumber = false ;
	value = m_string.AsNumber( &hasNumber ) ;
	return	hasNumber ;
}

// 文字列として評価
bool LStringObj::AsString( LString& str ) const
{
	str = m_string ;
	return	true ;
}

// （式表現に近い）文字列に変換
bool LStringObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	str = L"\"" ;
	str += LStringParser::EncodeStringLiteral
				( m_string.c_str(), m_string.GetLength() ) ;
	str += L"\"" ;
	return	true ;
}

// 複製する（要素も全て複製処理する）
LObject * LStringObj::CloneObject( void ) const
{
	// ※String オブジェクトは静的
	return	LObject::AddRef( const_cast<LStringObj*>( this ) ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LStringObj::DuplicateObject( void ) const
{
	// ※String オブジェクトは静的
	return	LObject::AddRef( const_cast<LStringObj*>( this ) ) ;
}



// String( String str )
void LStringObj::method_init( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	pStrObj->m_string = str ;

	LQT_RETURN_VOID() ;
}

// String( uint8* utf8, long length = -1 )
void LStringObj::method_init_utf8( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, ptrUTF8 ) ;
	LQT_FUNC_ARG_LONG( nLength ) ;

	if ( ptrUTF8 != nullptr )
	{
		size_t	nUTF8Len = ptrUTF8->GetBytes() ;
		if ( nLength < 0 )
		{
			std::uint8_t *	pUTF8 = ptrUTF8->GetPointer() ;
			for ( size_t i = 0; i < nUTF8Len; i ++ )
			{
				if ( pUTF8[i] == 0 )
				{
					nUTF8Len = i ;
					break ;
				}
			}
		}
		else
		{
			nUTF8Len = std::min( nUTF8Len, (size_t) nLength ) ;
		}
		if ( nUTF8Len > 0 )
		{
			std::vector<std::uint8_t>	utf8 ;
			utf8.resize( nUTF8Len ) ;
			memcpy( utf8.data(), ptrUTF8->GetPointer(),
							nUTF8Len * sizeof(std::uint8_t) ) ;
			pStrObj->m_string.FromUTF8( utf8 ) ;
		}
	}

	LQT_RETURN_VOID() ;
}

// String( uint16* utf16, long length = -1 )
void LStringObj::method_init_utf16( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, ptrUTF16 ) ;
	LQT_FUNC_ARG_LONG( nLength ) ;

	if ( ptrUTF16 != nullptr )
	{
		size_t	nUTF16Len = ptrUTF16->GetBytes() / sizeof(std::uint16_t) ;
		if ( nLength < 0 )
		{
			std::uint16_t *	pUTF16 = (std::uint16_t*) ptrUTF16->GetPointer() ;
			for ( size_t i = 0; i < nUTF16Len; i ++ )
			{
				if ( pUTF16[i] == 0 )
				{
					nUTF16Len = i ;
					break ;
				}
			}
		}
		else
		{
			nUTF16Len = std::min( nUTF16Len, (size_t) nLength ) ;
		}
		if ( nUTF16Len > 0 )
		{
			std::vector<std::uint16_t>	utf16 ;
			utf16.resize( nUTF16Len ) ;
			memcpy( utf16.data(), ptrUTF16->GetPointer(),
							nUTF16Len * sizeof(std::uint16_t) ) ;
			pStrObj->m_string.FromUTF16( utf16 ) ;
		}
	}

	LQT_RETURN_VOID() ;
}

// String( uint32* utf32, long length = -1 )
void LStringObj::method_init_utf32( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, ptrUTF32 ) ;
	LQT_FUNC_ARG_LONG( nLength ) ;

	if ( ptrUTF32 != nullptr )
	{
		size_t	nUTF32Len = ptrUTF32->GetBytes() / sizeof(std::uint32_t) ;
		if ( nLength < 0 )
		{
			std::uint32_t *	pUTF32 = (std::uint32_t*) ptrUTF32->GetPointer() ;
			for ( size_t i = 0; i < nUTF32Len; i ++ )
			{
				if ( pUTF32[i] == 0 )
				{
					nUTF32Len = i ;
					break ;
				}
			}
		}
		else
		{
			nUTF32Len = std::min( nUTF32Len, (size_t) nLength ) ;
		}
		if ( nUTF32Len > 0 )
		{
			std::vector<std::uint32_t>	utf32 ;
			utf32.resize( nUTF32Len ) ;
			memcpy( utf32.data(), ptrUTF32->GetPointer(),
							nUTF32Len * sizeof(std::uint32_t) ) ;
			pStrObj->m_string.FromUTF32( utf32 ) ;
		}
	}

	LQT_RETURN_VOID() ;
}

// uint32 length() const
void LStringObj::method_length( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	LQT_RETURN_LONG( pStrObj->m_string.GetLength() ) ;
}

// String left( long count ) const
void LStringObj::method_left( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( nCount ) ;

	LQT_RETURN_STRING( pStrObj->m_string.Left( (size_t) nCount ) ) ;
}

// String right( long count ) const
void LStringObj::method_right( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( nCount ) ;

	LQT_RETURN_STRING( pStrObj->m_string.Right( (size_t) nCount ) ) ;
}

// String middle( long first, long count = -1 ) const
void LStringObj::method_middle( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( iFirst ) ;
	LQT_FUNC_ARG_LONG( nCount ) ;

	if ( nCount < 0 )
	{
		nCount = (LLong) pStrObj->m_string.GetLength() ;
	}

	LQT_RETURN_STRING( pStrObj->m_string.Middle( (size_t) iFirst, (size_t) nCount ) ) ;
}

// int find( String str, int first = 0 ) const
void LStringObj::method_find( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_STRING( strFind ) ;
	LQT_FUNC_ARG_LONG( iFirst ) ;

	LQT_RETURN_LONG( pStrObj->m_string.Find( strFind.c_str(), (size_t) iFirst ) ) ;
}

// String upper() const
void LStringObj::method_upper( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	LQT_RETURN_STRING( pStrObj->m_string.ToUpper() ) ;
}

// String lower() const
void LStringObj::method_lower( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	LQT_RETURN_STRING( pStrObj->m_string.ToLower() ) ;
}

// String trim() const
void LStringObj::method_trim( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	LQT_RETURN_STRING( pStrObj->m_string.GetTrimmed() ) ;
}

// String chop( long count ) const
void LStringObj::method_chop( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( nCount ) ;

	LString	strChopped = pStrObj->m_string ;
	strChopped.ChopRight( (size_t) nCount ) ;

	LQT_RETURN_STRING( strChopped ) ;
}

// uint32 charAt( long index ) const
void LStringObj::method_charAt( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( index ) ;

	LQT_RETURN_INT( pStrObj->m_string.GetAt( (size_t) index ) ) ;
}

// uint32 backAt( long index ) const
void LStringObj::method_backAt( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( index ) ;

	LQT_RETURN_INT( pStrObj->m_string.GetBackAt( (size_t) index ) ) ;
}

// String replace( Map<String> map ) const
void LStringObj::method_replace( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_OBJECT( LMapObj, pMapObj ) ;

	LString	strReplaced ;
	if ( pMapObj != nullptr )
	{
		strReplaced = pStrObj->m_string.Replace
			( [pMapObj]( LStringParser& spars )
			{
				LString	strKey ;
				for ( size_t i = 0; i < pMapObj->GetElementCount(); i ++ )
				{
					if ( spars.HasNextString( pMapObj->GetElementNameAt( strKey, i ) ) )
					{
						return	true ;
					}
				}
				return	false ;
			},
			[pMapObj]( const LString& key )
			{
				LObjPtr	pObj( pMapObj->GetElementAs( key.c_str() ) ) ;
				if ( pObj != nullptr )
				{
					LString	str ;
					if ( pObj->AsString( str ) )
					{
						return	str ;
					}
				}
				return	LString() ;
			} ) ;
	}
	else
	{
		strReplaced = pStrObj->m_string ;
	}

	LQT_RETURN_STRING( strReplaced ) ;
}

// uint8* utf8() const
void LStringObj::method_utf8( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	std::shared_ptr<LArrayBufStorage>
			pBuf = std::make_shared<LArrayBufStorage>() ;
	pStrObj->m_string.ToUTF8( *pBuf ) ;

	LQT_RETURN_POINTER_BUF( pBuf ) ;
}

// uint16* utf16() const
void LStringObj::method_utf16( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	std::vector<std::uint16_t>	utf16 ;
	pStrObj->m_string.ToUTF16( utf16 ) ;

	LQT_RETURN_POINTER( utf16.data(), utf16.size() * sizeof(std::uint16_t) ) ;
}

// uint32* utf32() const
void LStringObj::method_utf32( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;

	std::vector<std::uint32_t>	utf32 ;
	pStrObj->m_string.ToUTF32( utf32 ) ;

	LQT_RETURN_POINTER( utf32.data(), utf32.size() * sizeof(std::uint32_t) ) ;
}

// long asInteger( boolean* pHasInteger = null, int radix = 10 ) const
void LStringObj::method_asInteger( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_POINTER( LBoolean, pHasInteger ) ;
	LQT_FUNC_ARG_INT( radix ) ;

	LQT_RETURN_LONG( pStrObj->m_string.AsInteger( pHasInteger, radix ) ) ;
}

// double asNumber( boolean* pHasNumber = null ) const
void LStringObj::method_asNumber( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringObj, pStrObj ) ;
	LQT_FUNC_ARG_POINTER( LBoolean, pHasNumber ) ;

	LQT_RETURN_DOUBLE( pStrObj->m_string.AsNumber( pHasNumber ) ) ;
}

// static String integerOf( long val, int prec = 0, int radix = 10 ) const
void LStringObj::method_integerOf( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_ARG_LONG( val ) ;
	LQT_FUNC_ARG_INT( prec ) ;
	LQT_FUNC_ARG_INT( radix ) ;

	LQT_RETURN_STRING( LString::IntegerOf( val, prec, radix ) ) ;
}

// static String numberOf( double val, int prec = 0, boolean exp = true ) const
void LStringObj::method_numberOf( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_ARG_DOUBLE( val ) ;
	LQT_FUNC_ARG_INT( prec ) ;
	LQT_FUNC_ARG_BOOL( exp ) ;

	LQT_RETURN_STRING( LString::NumberOf( val, prec, exp ) ) ;
}

// boolean operator == ( String str )
LValue::Primitive LStringObj::operator_eq
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeBool( true ) ;
		}
		return	LValue::MakeBool( pStr2->m_string == L"" ) ;
	}
	if ( pStr2 == nullptr )
	{
		return	LValue::MakeBool( pStr1->m_string == L"" ) ;
	}
	return	LValue::MakeBool( pStr1->m_string == pStr2->m_string ) ;
}

// boolean operator != ( String str )
LValue::Primitive LStringObj::operator_ne
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeBool( false ) ;
		}
		return	LValue::MakeBool( pStr2->m_string != L"" ) ;
	}
	if ( pStr2 == nullptr )
	{
		return	LValue::MakeBool( pStr1->m_string != L"" ) ;
	}
	return	LValue::MakeBool( pStr1->m_string != pStr2->m_string ) ;
}

// boolean operator < ( String str )
LValue::Primitive LStringObj::operator_lt
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeBool( false ) ;
		}
		return	LValue::MakeBool( pStr2->m_string > L"" ) ;
	}
	if ( pStr2 == nullptr )
	{
		return	LValue::MakeBool( pStr1->m_string < L"" ) ;
	}
	return	LValue::MakeBool( pStr1->m_string < pStr2->m_string ) ;
}

// boolean operator <= ( String str )
LValue::Primitive LStringObj::operator_le
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeBool( true ) ;
		}
		return	LValue::MakeBool( pStr2->m_string >= L"" ) ;
	}
	if ( pStr2 == nullptr )
	{
		return	LValue::MakeBool( pStr1->m_string <= L"" ) ;
	}
	return	LValue::MakeBool( pStr1->m_string <= pStr2->m_string ) ;
}

// boolean operator > ( String str )
LValue::Primitive LStringObj::operator_gt
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeBool( true ) ;
		}
		return	LValue::MakeBool( pStr2->m_string < L"" ) ;
	}
	if ( pStr2 == nullptr )
	{
		return	LValue::MakeBool( pStr1->m_string > L"" ) ;
	}
	return	LValue::MakeBool( pStr1->m_string > pStr2->m_string ) ;
}

// boolean operator >= ( String str )
LValue::Primitive LStringObj::operator_ge
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeBool( true ) ;
		}
		return	LValue::MakeBool( pStr2->m_string <= L"" ) ;
	}
	if ( pStr2 == nullptr )
	{
		return	LValue::MakeBool( pStr1->m_string >= L"" ) ;
	}
	return	LValue::MakeBool( pStr1->m_string >= pStr2->m_string ) ;
}

// String operator + ( String str )
LValue::Primitive LStringObj::operator_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;
	if ( pContext == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	LStringObj *	pStr2 = dynamic_cast<LStringObj*>( val2.pObject ) ;
	if ( pStr1 == nullptr )
	{
		if ( pStr2 == nullptr )
		{
			return	LValue::MakeObjectPtr( pContext->new_String( L"" ) ) ;
		}
		return	LValue::MakeObjectPtr( pContext->new_String( pStr2->m_string ) ) ;
	}
	if ( pStr2 == nullptr )
	{
		pStr1->AddRef() ;
		return	LValue::MakeObjectPtr( pStr1 ) ;
	}
	return	LValue::MakeObjectPtr
				( pContext->new_String( pStr1->m_string + pStr2->m_string ) ) ;
}

// String operator + ( long val )
LValue::Primitive LStringObj::operator_add_int
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;
	if ( pContext == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	if ( pStr1 == nullptr )
	{
		return	LValue::MakeObjectPtr
					( pContext->new_String( LString::IntegerOf(val2.longValue) ) ) ;
	}
	return	LValue::MakeObjectPtr
				( pContext->new_String
					( pStr1->m_string + LString::IntegerOf(val2.longValue) ) ) ;
}

// String operator + ( double val )
LValue::Primitive LStringObj::operator_add_num
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;
	if ( pContext == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	if ( pStr1 == nullptr )
	{
		return	LValue::MakeObjectPtr
					( pContext->new_String( LString::NumberOf(val2.dblValue) ) ) ;
	}
	return	LValue::MakeObjectPtr
				( pContext->new_String
					( pStr1->m_string + LString::NumberOf(val2.dblValue) ) ) ;
}

// String operator + ( Object obj )
LValue::Primitive LStringObj::operator_add_obj
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;
	if ( pContext == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LString	strObj ;
	if ( val2.pObject != nullptr )
	{
		val2.pObject->AsString( strObj ) ;
	}
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	if ( pStr1 == nullptr )
	{
		return	LValue::MakeObjectPtr( pContext->new_String( strObj ) ) ;
	}
	return	LValue::MakeObjectPtr
				( pContext->new_String( pStr1->m_string + strObj ) ) ;
}

// String operator * ( long count )
LValue::Primitive LStringObj::operator_mul
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;
	if ( pContext == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	if ( pStr1 == nullptr )
	{
		return	LValue::MakeObjectPtr( pContext->new_String( L"" ) ) ;
	}
	return	LValue::MakeObjectPtr
				( pContext->new_String( pStr1->m_string * (int) val2.longValue ) ) ;
}

// String[] operator / ( String deli )
LValue::Primitive LStringObj::operator_div
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;
	if ( pContext == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LStringObj *	pStr1 = dynamic_cast<LStringObj*>( val1.pObject ) ;
	if ( pStr1 == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LArrayObj *	pArray = pContext->new_Array( pContext->VM().GetStringClass() ) ;

	LString	strDeli ;
	if ( val2.pObject != nullptr )
	{
		val2.pObject->AsString( strDeli ) ;
	}
	std::vector<LString>	vSliced ;
	pStr1->m_string.Slice( vSliced, strDeli.c_str() ) ;

	for ( size_t i = 0; i < vSliced.size(); i ++ )
	{
		pArray->m_array.push_back
			( LObjPtr( pContext->new_String( vSliced.at(i) ) ) ) ;
	}

	return	LValue::MakeObjectPtr( pArray ) ;
}




//////////////////////////////////////////////////////////////////////////
// StringBuf オブジェクト
//////////////////////////////////////////////////////////////////////////

// 要素取得
// （返り値は呼び出し側の責任で ReleaseRef）
LObject * LStringBufObj::GetElementAt( size_t index ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	LStringObj::GetElementAt( index ) ;
}

// 要素設定（pObj には AddRef されたポインタを渡す）
// （以前の要素を返す / ReleaseRef が必要）
LObject * LStringBufObj::SetElementAt( size_t index, LObject * pObj )
{
	LLong	value = 0 ;
	if ( (index < m_string.GetLength())
		&& (pObj != nullptr)
		&& pObj->AsInteger( value ) )
	{
		m_string.at(index) = (wchar_t) value ;
	}
	return	nullptr ;
}

// 整数値として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LStringBufObj::AsInteger( LLong& value ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	LStringObj::AsInteger( value ) ;
}

// 浮動小数点として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LStringBufObj::AsDouble( LDouble& value ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	LStringObj::AsDouble( value ) ;
}

// 文字列として評価
bool LStringBufObj::AsString( LString& str ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	LStringObj::AsString( str ) ;
}

// （式表現に近い）文字列に変換
bool LStringBufObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	LStringObj::AsExpression( str, flags ) ;
}

// プリミティブな操作の定義
// 複製する（要素も全て複製処理する）
LObject * LStringBufObj::CloneObject( void ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	new LStringBufObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LStringBufObj::DuplicateObject( void ) const
{
	LSpinLock	lock( m_mutex ) ;
	return	new LStringBufObj( *this ) ;
}

// StringBuf( String str )
void LStringBufObj::method_init( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringBufObj, pStrObj ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	pStrObj->m_string = str ;

	LQT_RETURN_OBJECT( pStrObj ) ;
}

// StringBuf set( String str )
void LStringBufObj::method_set( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringBufObj, pStrObj ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	LSpinLock	lock( pStrObj->m_mutex ) ;
	pStrObj->m_string = str ;
	pStrObj->AddRef() ;

	LQT_RETURN_OBJECT( pStrObj ) ;
}

// StringBuf setIntegerOf( long val, int prec = 0, int radix = 10 )
void LStringBufObj::method_setIntegerOf( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringBufObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( val ) ;
	LQT_FUNC_ARG_INT( prec ) ;
	LQT_FUNC_ARG_INT( radix ) ;

	LSpinLock	lock( pStrObj->m_mutex ) ;
	pStrObj->m_string.SetIntegerOf( val, prec, radix ) ;
	pStrObj->AddRef() ;

	LQT_RETURN_OBJECT( pStrObj ) ;
}

// StringBuf setNumberOf( double val, int prec = 0, boolean exp = true )
void LStringBufObj::method_setNumberOf( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringBufObj, pStrObj ) ;
	LQT_FUNC_ARG_DOUBLE( val ) ;
	LQT_FUNC_ARG_INT( prec ) ;
	LQT_FUNC_ARG_BOOL( exp ) ;

	LSpinLock	lock( pStrObj->m_mutex ) ;
	pStrObj->m_string.SetNumberOf( val, prec, exp ) ;
	pStrObj->AddRef() ;

	LQT_RETURN_OBJECT( pStrObj ) ;
}

// void setAt( long index, uint32 code )
void LStringBufObj::method_setAt( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringBufObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( index ) ;
	LQT_FUNC_ARG_INT( ch ) ;

	LSpinLock	lock( pStrObj->m_mutex ) ;
	pStrObj->m_string.SetAt( (size_t) index, (wchar_t) ch ) ;

	LQT_RETURN_VOID() ;
}

// void resize( long length )
void LStringBufObj::method_resize( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LStringBufObj, pStrObj ) ;
	LQT_FUNC_ARG_LONG( length ) ;

	LSpinLock	lock( pStrObj->m_mutex ) ;
	pStrObj->m_string.resize( (size_t) length ) ;

	LQT_RETURN_VOID() ;
}

// StringBuf operator := ( String str )
LValue::Primitive LStringBufObj::operator_smov
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringBufObj *	pStrBuf = dynamic_cast<LStringBufObj*>( val1.pObject ) ;
	if ( pStrBuf == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	if ( val2.pObject != nullptr )
	{
		LString	str ;
		if ( val2.pObject->AsString( str ) )
		{
			LSpinLock	lock( pStrBuf->m_mutex ) ;
			pStrBuf->m_string = str ;
		}
		else
		{
			pStrBuf->m_string = L"" ;
		}
	}
	else
	{
		pStrBuf->m_string = L"" ;
	}
	pStrBuf->AddRef() ;
	return	LValue::MakeObjectPtr( pStrBuf ) ;
}

// StringBuf operator += ( String str )
LValue::Primitive LStringBufObj::operator_sadd
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LStringBufObj *	pStrBuf = dynamic_cast<LStringBufObj*>( val1.pObject ) ;
	if ( pStrBuf == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	if ( val2.pObject != nullptr )
	{
		LString	str ;
		if ( val2.pObject->AsString( str ) )
		{
			LSpinLock	lock( pStrBuf->m_mutex ) ;
			pStrBuf->m_string += str ;
		}
	}
	pStrBuf->AddRef() ;
	return	LValue::MakeObjectPtr( pStrBuf ) ;
}

