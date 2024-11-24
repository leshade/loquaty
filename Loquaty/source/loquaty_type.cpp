
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 基本的な型情報
//////////////////////////////////////////////////////////////////////////////

// メモリ配置上の整列サイズ
const size_t	LType::s_bytesAligned[LType::typeAllCount]  =
{
	sizeof(LBoolean),					// typeBoolean
	sizeof(LInt8), sizeof(LUint8),		// typeInt8, typeUint8
	sizeof(LInt16), sizeof(LUint16),	// typeInt16, typeUint16
	sizeof(LInt32), sizeof(LUint32),	// typeInt32, typeUint32
	sizeof(LInt64),	sizeof(LUint64),	// typeInt64, typeUint64
	sizeof(LFloat), sizeof(LDouble),	// typeFloat, typeDouble
	sizeof(LObject*),					// typeObject
	sizeof(LNativeObj*),				// typeNativeObject
	sizeof(LObject*),					// typeDataArray
	sizeof(LObject*),					// typeStructure
	sizeof(LPointerObj*),				// typePointer
	sizeof(LIntegerObj*),				// typeIntegerObject
	sizeof(LDoubleObj*),				// typeDoubleObject
	sizeof(LStringObj*),				// typeString
	sizeof(LStringBufObj*),				// typeStringBuf
	sizeof(LArrayObj*),					// typeArray
	sizeof(LMapObj*),					// typeMap
	sizeof(LFunctionObj*),				// typeFunction
} ;

// プリミティブ型名
const wchar_t *	LType::s_pwszPrimitiveTypeName[LType::typePrimitiveCount] =
{
	L"boolean",
	L"byte",
	L"uint8",
	L"short",
	L"uint16",
	L"int",
	L"uint",
	L"long",
	L"ulong",
	L"float",
	L"double",
} ;

struct	PrimitiveTypeNamePair
{
	const wchar_t *		name ;
	LType::Primitive	type ;
} ;
static const PrimitiveTypeNamePair	s_PrimitiveTypeNamePairs[] =
{
	{	L"boolean",	LType::typeBoolean	},
	{	L"byte",	LType::typeInt8		},
	{	L"ubyte",	LType::typeUint8	},
	{	L"short",	LType::typeInt16	},
	{	L"ushort",	LType::typeUint16	},
	{	L"int",		LType::typeInt32	},
	{	L"uint",	LType::typeUint32	},
	{	L"long",	LType::typeInt64	},
	{	L"ulong",	LType::typeUint64	},
	{	L"float",	LType::typeFloat	},
	{	L"double",	LType::typeDouble	},
	{	L"uint8",	LType::typeUint8	},
	{	L"int8",	LType::typeInt8		},
	{	L"int16",	LType::typeInt16	},
	{	L"uint16",	LType::typeUint16	},
	{	L"int32",	LType::typeInt32	},
	{	L"uint32",	LType::typeUint32	},
	{	L"int64",	LType::typeInt64	},
	{	L"uint64",	LType::typeUint64	},
	{	nullptr,	LType::typeObject	},
} ;

// プリミティブ型名判定（一致しない場合は typeObject を返却）
LType::Primitive LType::AsPrimitive( const wchar_t * name )
{
	if ( name == nullptr )
	{
		return	typeObject ;
	}
	for ( size_t iType = 0;
			s_PrimitiveTypeNamePairs[iType].name != nullptr; iType ++ )
	{
		const wchar_t *	type = s_PrimitiveTypeNamePairs[iType].name ;
		size_t	i = 0 ;
		while ( type[i] == name[i] )
		{
			if ( type[i] == 0 )
			{
				return	s_PrimitiveTypeNamePairs[iType].type ;
			}
			i ++ ;
		}
	}
	return	typeObject ;
}

// プリミティブ型の合成
LType::Primitive LType::MaxPrimitiveOf
	( LType::Primitive type1, LType::Primitive type2 )
{
	if ( type1 == type2 )
	{
		return	type1 ;
	}
	if ( IsFloatingPointPrimitive(type1)
		|| IsFloatingPointPrimitive(type2) )
	{
		return	typeDouble ;
	}
	assert( !IsFloatingPointPrimitive(type1) ) ;
	assert( !IsFloatingPointPrimitive(type2) ) ;
	if ( type1 < type2 )
	{
		LType::Primitive	t = type2 ;
		type2 = type1 ;
		type1 = t ;
	}
	assert( type1 > type2 ) ;
	if ( type1 == typeUint32 )
	{
		if ( type2 == typeInt32 )
		{
			return	typeInt64 ;
		}
	}
	else if ( type1 == typeUint16 )
	{
		if ( type2 == typeInt16 )
		{
			return	typeInt32 ;
		}
	}
	else if ( type1 == typeUint8 )
	{
		if ( type2 == typeInt8 )
		{
			return	typeInt16 ;
		}
	}
	return	type1 ;
}

// Object 型判定
bool LType::IsArray( void ) const
{
	return	(m_type == typeArray)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LArrayClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsMap( void ) const
{
	return	(m_type == typeMap)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LMapClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsFunction( void ) const
{
	return	(m_type == typeFunction)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LFunctionClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsStructure( void ) const
{
	return	(m_type == typeStructure)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LStructureClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsStructurePtr( void ) const
{
	LPointerClass *	pPtrClass = GetPointerClass() ;
	if ( pPtrClass != nullptr )
	{
		LStructureClass *
			pStructClass = pPtrClass->GetBufferType().GetStructureClass() ;
		return	(pStructClass != nullptr) ;
	}
	return	false ;
}

bool LType::IsDataArray( void ) const
{
	return	(m_type == typeDataArray)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LDataArrayClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsPointer( void ) const
{
	return	(m_type == typePointer)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LPointerClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsFetchAddr( void ) const
{
	return	IsObject()
			&& (m_accMod & LType::modifierFetchAddr) ;
}

bool LType::IsIntegerObj( void ) const
{
	return	(m_type == typeIntegerObj)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LIntegerClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsDoubleObj( void ) const
{
	return	(m_type == typeDoubleObj)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LDoubleClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsString( void ) const
{
	return	(m_type == typeString)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LStringClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsStringBuf( void ) const
{
	return	(m_type == typeStringBuf)
			|| ((m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LStringBufClass*>( m_pClass ) != nullptr)) ;
}

bool LType::IsTask( void ) const
{
	return	(m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LTaskClass*>( m_pClass ) != nullptr) ;
}

bool LType::IsThread( void ) const
{
	return	(m_type == typeObject)
				&& (m_pClass != nullptr)
				&& (dynamic_cast<LThreadClass*>( m_pClass ) != nullptr) ;
}

// バッファ配列可能な型か？（ポインタ型にできるか？）
bool LType::CanArrangeOnBuf( void ) const
{
	return	IsPrimitive() || IsStructure() || IsDataArray() ;
}

// 実行時にオブジェクト表現される型か？
bool LType::IsRuntimeObject( void ) const
{
	return	IsObject() && !IsDataArray() && !IsFetchAddr() ;
}

// 解放が必要な型か？
bool LType::IsNeededToRelease( void ) const
{
	return	IsObject() && !IsFetchAddr() ;
}

// 型情報取得
LClass * LType::GetClass( void ) const
{
	if ( IsObjectType( m_type ) )
	{
		if ( m_pClass != nullptr )
		{
			return	m_pClass ;
		}
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetObjectClass() ;
	}
	return	nullptr ;
}

LArrayClass * LType::GetArrayClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LArrayClass*>( m_pClass ) ;
	}
	else if ( m_type == typeArray )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetArrayClass() ;
	}
	return	nullptr ;
}

LMapClass * LType::GetMapClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LMapClass*>( m_pClass ) ;
	}
	else if ( m_type == typeMap )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetMapClass() ;
	}
	return	nullptr ;
}

LFunctionClass * LType::GetFunctionClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LFunctionClass*>( m_pClass ) ;
	}
	else if ( m_type == typeFunction )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetFunctionClass() ;
	}
	return	nullptr ;
}

LTaskClass * LType::GetTaskClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LTaskClass*>( m_pClass ) ;
	}
	return	nullptr ;
}

LThreadClass * LType::GetThreadClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LThreadClass*>( m_pClass ) ;
	}
	return	nullptr ;
}

LStructureClass * LType::GetStructureClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LStructureClass*>( m_pClass ) ;
	}
	else if ( m_type == typeStructure )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetStructureClass() ;
	}
	return	nullptr ;
}

LStructureClass * LType::GetPtrStructureClass( void ) const
{
	LPointerClass *	pPtrClass = GetPointerClass() ;
	if ( pPtrClass != nullptr )
	{
		return	pPtrClass->GetBufferType().GetStructureClass() ;
	}
	return	nullptr ;
}

LDataArrayClass * LType::GetDataArrayClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LDataArrayClass*>( m_pClass ) ;
	}
	else if ( m_type == typeDataArray )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetDataArrayClass() ;
	}
	return	nullptr ;
}

LPointerClass * LType::GetPointerClass( void ) const
{
	if ( (m_type == typeObject) && (m_pClass != nullptr) )
	{
		return	dynamic_cast<LPointerClass*>( m_pClass ) ;
	}
	else if ( m_type == typePointer )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? nullptr : pVM->GetPointerClass() ;
	}
	return	nullptr ;
}

// ポインタ型かデータ配列型で装飾されていないベースの型を取得する
LType LType::GetBaseDataType( void ) const
{
	LPointerClass *	pPtrClass = GetPointerClass() ;
	if ( pPtrClass != nullptr )
	{
		return	pPtrClass->GetDataType() ;
	}
	else
	{
		LDataArrayClass *	pArrayClass = GetDataArrayClass() ;
		if ( pArrayClass != nullptr )
		{
			return	pArrayClass->GetDataType() ;
		}
	}
	return	*this ;
}

// ポインタ型の場合、ポインタ装飾を外した型を取得する
// データ配列型の場合、１回要素参照した型を取得する
// 構造体型の場合、そのままを取得する
// Map, Array の場合は要素型を取得する
// String, StringBuf の場合は Integer 型を取得する
// それ以外の場合は void を取得する
LType LType::GetRefElementType( void ) const
{
	LPointerClass *	pPtrClass = GetPointerClass() ;
	if ( pPtrClass != nullptr )
	{
		return	pPtrClass->GetBufferType() ;
	}
	LDataArrayClass *	pDataArrayClass = GetDataArrayClass() ;
	if ( pDataArrayClass != nullptr )
	{
		return	pDataArrayClass->GetElementType() ;
	}
	if ( IsStructure() )
	{
		return	*this ;
	}
	LMapClass *	pMapClass = GetMapClass() ;
	if ( pMapClass != nullptr )
	{
		LClass *	pElementType = pMapClass->GetElementTypeClass() ;
		if ( pElementType != nullptr )
		{
			return	LType( pElementType ) ;
		}
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? LType(typeObject)
									: LType(pVM->GetObjectClass()) ;
	}
	LArrayClass *	pArrayClass = GetArrayClass() ;
	if ( pArrayClass != nullptr )
	{
		LClass *	pElementType = pArrayClass->GetElementTypeClass() ;
		if ( pElementType != nullptr )
		{
			return	LType( pElementType ) ;
		}
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? LType(typeObject)
									: LType(pVM->GetObjectClass()) ;
	}
	if ( IsString() || IsStringBuf() )
	{
		LVirtualMachine *	pVM = LVirtualMachine::GetCurrentVM() ;
		return	(pVM == nullptr) ? LType(typeIntegerObj)
									: LType(pVM->GetIntegerObjClass()) ;
	}
	return	LType() ;
}

// 配置の際のアライメント
size_t LType::GetAlignBytes( void ) const
{
	if ( !IsPrimitive() )
	{
		LStructureClass *	pStructClass = GetStructureClass() ;
		if ( pStructClass != nullptr )
		{
			return	pStructClass->GetProtoArrangemenet().GetArrangeAlign() ;
		}
		LDataArrayClass *	pArrayClass = GetDataArrayClass() ;
		if ( pArrayClass != nullptr )
		{
			return	pArrayClass->GetDataAlign() ;
		}
	}
	return	s_bytesAligned[m_type] ;
}

// 配置の際のデータサイズ
size_t LType::GetDataBytes( void ) const
{
	if ( !IsPrimitive() )
	{
		LStructureClass *	pStructClass = GetStructureClass() ;
		if ( pStructClass != nullptr )
		{
			return	pStructClass->GetStructBytes() ;
		}
		LDataArrayClass *	pArrayClass = GetDataArrayClass() ;
		if ( pArrayClass != nullptr )
		{
			return	pArrayClass->GetDataSize() ;
		}
	}
	return	s_bytesAligned[m_type] ;
}

// 型名取得
LString LType::GetTypeName( void ) const
{
	if ( IsPrimitive() )
	{
		assert( (size_t) m_type < typePrimitiveCount ) ;
		if ( IsConst() )
		{
			LString	strType = L"const " ;
			strType += s_pwszPrimitiveTypeName[m_type] ;
			return	strType ;
		}
		else
		{
			return	s_pwszPrimitiveTypeName[m_type] ;
		}
	}
	if ( m_pClass != nullptr )
	{
		LString	strType ;
		if ( m_accMod & LType::modifierFetchAddr )
		{
			strType += L"fetch_addr " ;
		}
		if ( IsPointer() || IsDataArray() )
		{
			strType += m_pClass->GetFullClassName() ;
			if ( IsConst() )
			{
				strType += L"const" ;
			}
			return	strType ;
		}
		if ( IsConst() )
		{
			strType += L"const " ;
			strType += m_pClass->GetFullClassName() ;
			return	strType ;
		}
		else
		{
			return	strType + m_pClass->GetFullClassName() ;
		}
	}
	return	IsConst() ? L"const void" : L"void" ;
}

// コメントデータ
LType::LComment * LType::GetComment( void ) const
{
	return	m_pComment ;
}

void LType::SetComment( LComment * pComment )
{
	m_pComment = pComment ;
}

// 暗黙のキャスト可能か？（データの変換を含む）
LType::CastMethod LType::CanCastTo( const LType& type ) const
{
	if ( IsEqual( type ) )
	{
		return	castableJust ;
	}
	if ( IsPrimitive() )
	{
		if ( type.IsPrimitive() )
		{
			if ( IsInteger() )
			{
				if ( type.IsInteger() )
				{
					if ( GetAlignBytes() == type.GetAlignBytes() )
					{
						if ( IsUnsignedInteger() == type.IsUnsignedInteger() )
						{
							return	castablePrecision ;
						}
						// int -> int : 符号有無変換
						return	castablePrecision ; //castableConvertNum ;
					}
					else if ( GetAlignBytes() < type.GetAlignBytes() )
					{
						// int -> int : 大きい整数へ拡張
						return	castablePrecision ;
					}
					else
					{
						// int -> int : 小さい整数へ切り詰め
						return	castableConvertNum ;
					}
				}
				else if ( type.IsFloatingPointNumber() )
				{
					if ( GetAlignBytes() < type.GetAlignBytes() )
					{
						// int -> double : 大きい精度の浮動小数点へ拡張
						return	castablePrecision ;
					}
					else
					{
						// int -> float : 浮動小数点へ丸め
						return	castableConvertNum ;
					}
				}
				else if ( IsBoolean() )
				{
					// int -> boolean
					return	castableConvertNum ;
				}
			}
			else if ( IsFloatingPointNumber() )
			{
				if ( type.IsInteger() )
				{
					// double -> int : 整数への丸め
					return	castableConvertNum ;
				}
				else if ( type.IsFloatingPointNumber() )
				{
					if ( GetAlignBytes() <= type.GetAlignBytes() )
					{
						// float -> double : 大きい精度へ拡張
						return	castablePrecision ;
					}
					else
					{
						// double -> float : 小さい精度へ丸め
						return	castableConvertNum ;
					}
				}
			}
			else if ( IsBoolean() )
			{
				if ( type.IsBoolean() || type.IsInteger() )
				{
					// bool -> { bool | intXX }
					return	castablePrecision ;
				}
			}
		}
		else
		{
			if ( type.IsPointer() )
			{
				LPointerClass *	pPtrClass = type.GetPointerClass() ;
				assert( pPtrClass != nullptr ) ;
				if ( pPtrClass->GetBufferType().IsVoid()
					|| (pPtrClass->GetBufferType().IsPrimitive()
						&& (pPtrClass->GetBufferType().GetPrimitive()
													== GetPrimitive())) )
				{
					// primitive -> [const] primitive*  ※変換元が参照型でないなら不可
					if ( !IsConst() || pPtrClass->GetBufferType().IsConst() )
					{
						return	castableDataToPtr ;
					}
					else
					{
						return	castableConstDataToPtr ;
					}
				}
			}
			else if ( IsInteger()  || IsBoolean() )
			{
				if ( type.IsIntegerObj() )
				{
					// int -> Integer : ボックス化
					return	castableBoxing ;
				}
				else if ( type.IsString() )
				{
					// int -> String : 文字列化
					return	castableNumToStr ;
				}
			}
			else if ( IsFloatingPointNumber() )
			{
				if ( type.IsDoubleObj() )
				{
					// double -> Double : ボックス化
					return	castableBoxing ;
				}
				else if ( type.IsString() )
				{
					// double -> String : 文字列化
					return	castableNumToStr ;
				}
			}
		}
	}
	else if ( !IsVoid() )
	{
		if ( type.IsPrimitive() )
		{
			if ( type.IsInteger() )
			{
				if ( IsIntegerObj() )
				{
					// Integer -> int : アンボックス化
					return	castableUnboxing ;
				}
				else if ( IsString() || IsStringBuf() )
				{
					// String -> int : 数値化
					return	castableStrToNum ;
				}
			}
			else if ( type.IsFloatingPointNumber() )
			{
				if ( IsDoubleObj() )
				{
					// Double -> double : アンボックス化
					return	castableUnboxing ;
				}
				else if ( IsString() || IsStringBuf() )
				{
					// String -> double : 数値化
					return	castableStrToNum ;
				}
			}
			else if ( type.IsBoolean() )
			{
				if ( !type.IsDataArray() && !type.IsStructure() )
				{
					// {Object | Pointer} -> boolean
					return	castableConvertNum ;
				}
			}
		}
		else if ( (m_pClass != nullptr) && (type.m_pClass != nullptr) )
		{
			if ( IsDataArray() )
			{
				LType	typePtr( GetDataArrayClass()->GetElementPtrClass() ) ;
				if ( typePtr.CanImplicitCastTo( type ) < castableExplicitly )
				{
					// type[] -> type* : 配列のポインタ化
					return	castableArrayToPtr ;
				}
				typePtr = LType( m_pClass->VM().GetPointerClassAs
									( GetDataArrayClass()->GetDataType() ) ) ;
				if ( typePtr.CanImplicitCastTo( type ) < castableExplicitly )
				{
					// type[] -> type* : 配列のポインタ化
					return	castableArrayToPtr ;
				}
				return	castImpossible ;
			}
			if ( IsStructure() && type.IsStructurePtr() )
			{
				if ( m_pClass->IsInstanceOf( type.GetPtrStructureClass() ) )
				{
					// struct -> struct* : 構造体のポインタ化
					if ( !IsConst() || type.GetPointerClass()->GetBufferType().IsConst() )
					{
						return	castableDataToPtr ;
					}
					else
					{
						return	castableConstDataToPtr ;
					}
				}
			}
			LClass::ResultInstanceOf	rs = m_pClass->TestInstanceOf( type.m_pClass ) ;
			if ( rs == LClass::instanceAvailable )
			{
				if ( !IsConst() || type.IsConst() )
				{
					return	IsPointer() ? castableUpCastPtr : castableUpCast ;
				}
				else
				{
					return	IsPointer() ? castableConstCastPtr : castableConstCast ;
				}
			}
			else if ( rs == LClass::instanceConstCast )
			{
				return	IsPointer() ? castableConstCastPtr : castableConstCast ;
			}
			else
			{
				rs = type.m_pClass->TestInstanceOf( m_pClass ) ;
				if ( rs == LClass::instanceAvailable )
				{
					if ( !IsConst() || type.IsConst() )
					{
						return	IsPointer() ? castableDownCastPtr : castableDownCast ;
					}
					else
					{
						return	IsPointer() ? castableConstCastPtr : castableConstCast ;
					}
				}
				else if ( rs == LClass::instanceConstCast )
				{
					return	IsPointer() ? castableConstCastPtr : castableConstCast ;
				}
				else if ( rs == LClass::instanceStrangerPtr )
				{
					assert( IsPointer() && type.IsPointer() ) ;
					return	castableCastStrangerPtr ;
				}
				else if ( type.IsString() )
				{
					// Object -> String : 文字列化
					return	castableObjToStr ;
				}
				else if ( !CanArrangeOnBuf() && !type.CanArrangeOnBuf()
								&& !IsPointer() && !type.IsPointer() )
				{
					return	castableCrossCast ;
				}
				else if ( IsRuntimeObject()
						&& type.IsPointer()
						&& type.GetPointerClass()->GetBufferType().IsVoid() )
				{
					// Object -> void*
					return	castableObjectToPtr ;
				}
			}
		}
	}
	else
	{
		// void (null)
		assert( IsVoid() ) ;
		if ( !type.IsPrimitive() )
		{
			return	castableJust ;
		}
	}
	return	castImpossible ;
}

// 暗黙のキャスト可能か？（データの変換を含む）
LType::CastMethod LType::CanImplicitCastTo( const LType& type ) const
{
	CastMethod	cm = CanCastTo( type ) ;
	if ( cm >= castableExplicitly )
	{
		return	castImpossible ;
	}
	return	cm ;
}

// 明示的なキャスト可能か？（データの変換を含む）
LType::CastMethod LType::CanExplicitCastTo( const LType& type ) const
{
	return	CanCastTo( type ) ;
}


