
#include <loquaty.h>

using namespace	Loquaty ;



//////////////////////////////////////////////////////////////////////////////
// 型と値
//////////////////////////////////////////////////////////////////////////////

LValue::LValue( LObjPtr pObj )
	: m_type( (pObj != nullptr) ? pObj->GetClass() : (LClass*) nullptr ),
		m_pObject( pObj )
{
	m_value.pObject = pObj.Ptr() ;
}

// オブジェクトなら AddRef する
LObject * LValue::AddRef( void ) const
{
	if ( m_pObject != nullptr )
	{
		m_pObject->AddRef() ;
		return	m_pObject.Ptr() ;
	}
	return	nullptr ;
}

// 複製を作成
LValue LValue::Clone( void ) const
{
	if ( m_type.IsObject() )
	{
		if ( m_pObject != nullptr )
		{
			return	LValue( m_type, LObjPtr(m_pObject->CloneObject()) ) ;
		}
	}
	return	LValue( *this ) ;
}

// 値を評価
LBoolean LValue::AsBoolean( void ) const
{
	if ( m_type.IsPrimitive() )
	{
		if ( m_type.IsInteger() )
		{
			return	(m_value.longValue != 0) ;
		}
		else if ( m_type.IsBoolean() )
		{
			return	m_value.boolValue ;
		}
		else
		{
			assert( m_type.IsFloatingPointNumber() ) ;
			return	(m_value.dblValue != 0.0) ;
		}
	}
	else if ( m_pObject != nullptr )
	{
		LPointerObj *	pPtrObj = dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;
		if ( pPtrObj != nullptr )
		{
			LLong	val ;
			if ( pPtrObj->AsInteger( val ) )
			{
				return	(val != 0) ;
			}
			return	false ;
		}
		LIntegerObj *	pIntObj = dynamic_cast<LIntegerObj*>( m_pObject.Ptr() ) ;
		if ( pIntObj != nullptr )
		{
			return	(pIntObj->m_value != 0) ;
		}
		return	true ;
	}
	return	false ;
}

LLong LValue::AsInteger( LLong err ) const
{
	if ( m_type.IsPrimitive() )
	{
		if ( m_type.IsInteger() )
		{
			return	m_value.longValue ;
		}
		else if ( m_type.IsBoolean() )
		{
			return	m_value.boolValue ;
		}
		else
		{
			assert( m_type.IsFloatingPointNumber() ) ;
			return	(LLong) m_value.dblValue ;
		}
	}
	else if ( m_pObject != nullptr )
	{
		LLong	val ;
		if ( m_pObject->AsInteger( val ) )
		{
			return	val ;
		}
	}
	return	err ;
}

LDouble LValue::AsDouble( LDouble err ) const
{
	if ( m_type.IsPrimitive() )
	{
		if ( m_type.IsInteger() )
		{
			return	(LDouble) m_value.longValue ;
		}
		else if ( m_type.IsBoolean() )
		{
			return	m_value.boolValue ;
		}
		else
		{
			assert( m_type.IsFloatingPointNumber() ) ;
			return	m_value.dblValue ;
		}
	}
	else if ( m_pObject != nullptr )
	{
		LDouble	val ;
		if ( m_pObject->AsDouble( val ) )
		{
			return	val ;
		}
	}
	return	err ;
}

LString LValue::AsString( const wchar_t * err ) const
{
	if ( m_type.IsPrimitive() )
	{
		if ( m_type.IsInteger() || m_type.IsBoolean() )
		{
			return	LString::IntegerOf( m_value.longValue ) ;
		}
		else
		{
			assert( m_type.IsFloatingPointNumber() ) ;
			return	LString::NumberOf( m_value.dblValue ) ;
		}
	}
	else if ( m_pObject != nullptr )
	{
		LString	str ;
		if ( m_pObject->AsString( str ) )
		{
			return	str ;
		}
	}
	return	LString( err ) ;
}

// ポインタの参照先やオブジェクトに値を設定
bool LValue::PutInteger( LLong val )
{
	LPointerObj *	pPtr = dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;
	if ( pPtr != nullptr )
	{
		return	pPtr->PutInteger( val ) ;
	}
	LIntegerObj *	pInt = dynamic_cast<LIntegerObj*>( m_pObject.Ptr() ) ;
	if ( pInt != nullptr )
	{
		pInt->m_value = val ;
		return	true ;
	}
	if ( m_type.IsInteger() )
	{
		m_value.longValue = val ;
		return	true ;
	}
	else if ( m_type.IsFloatingPointNumber() )
	{
		m_value.dblValue = (LDouble) val ;
		return	true ;
	}
	return	false ;
}

bool LValue::PutDouble( LDouble val )
{
	LPointerObj *	pPtr = dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;
	if ( pPtr != nullptr )
	{
		return	pPtr->PutDouble( val ) ;
	}
	LDoubleObj *	pDouble = dynamic_cast<LDoubleObj*>( m_pObject.Ptr() ) ;
	if ( pDouble != nullptr )
	{
		pDouble->m_value = val ;
		return	true ;
	}
	if ( m_type.IsInteger() )
	{
		m_value.longValue = (LLong) val ;
		return	true ;
	}
	else if ( m_type.IsFloatingPointNumber() )
	{
		m_value.dblValue = val ;
		return	true ;
	}
	return	false ;
}

bool LValue::PutString( const wchar_t * str )
{
	LStringObj *	pStr = dynamic_cast<LStringObj*>( m_pObject.Ptr() ) ;
	if ( pStr != nullptr )
	{
		pStr->m_string = str ;
		return	true ;
	}
	return	false ;
}

// クラスのメンバやポインタの参照先の構造体に値を設定
// ※AsExpression で文字列化した値を LCompiler::EvaluateConstExpr で
// 　解釈し PutMembers でリストアすることができる
bool LValue::PutMembers( const LValue& val )
{
	if ( m_type.IsPointer() )
	{
		LPointerObj *	pPtrObj = dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;
		if ( pPtrObj != nullptr )
		{
			return	pPtrObj->PutMembers( val ) ;
		}
	}
	else if ( m_type.IsObject() )
	{
		LArrayObj *	pArrayObj = dynamic_cast<LArrayObj*>( m_pObject.Ptr() ) ;
		if ( pArrayObj != nullptr )
		{
			// array / map / generic object...
			return	pArrayObj->PutMembers( val.GetObject() ) ;
		}
		LStringObj *	pStrObj = dynamic_cast<LStringObj*>( m_pObject.Ptr() ) ;
		if ( pStrObj != nullptr )
		{
			pStrObj->m_string = val.AsString() ;
			return	true ;
		}
		LIntegerObj *	pIntObj = dynamic_cast<LIntegerObj*>( m_pObject.Ptr() ) ;
		if ( pIntObj != nullptr )
		{
			pIntObj->m_value = val.AsInteger() ;
			return	true ;
		}
		LDoubleObj *	pNumObj = dynamic_cast<LDoubleObj*>( m_pObject.Ptr() ) ;
		if ( pNumObj != nullptr )
		{
			pNumObj->m_value = val.AsDouble() ;
			return	true ;
		}
	}
	return	false ;
}

// 要素取得
LValue LValue::GetElementAt
	( LVirtualMachine& vm, size_t index, bool flagRef ) const
{
	if ( m_type.IsPointer() )
	{
		// ポインタ
		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;
		if ( pPtrObj == nullptr )
		{
			return	LValue() ;
		}
		LPointerClass *	pPtrClass = m_type.GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		LType	typeElement = pPtrClass->GetBufferType() ;
		if ( typeElement.IsDataArray() )
		{
			typeElement = typeElement.GetRefElementType() ;
		}
		const size_t		nElementBytes = typeElement.GetDataBytes() ;
		LPtr<LPointerObj>	pPtrElement
			( new LPointerObj( vm.GetPointerClassAs( typeElement ) ) ) ;
		pPtrElement->SetPointer
			( pPtrObj->GetArrayBuffer(),
				pPtrObj->GetOffset()
					+ index * nElementBytes,
				pPtrClass->GetBufferType().GetDataBytes() ) ;
		if ( flagRef )
		{
			return	LValue( pPtrElement ) ;
		}
		return	LValue( pPtrElement ).UnboxingData() ;
	}
	else if ( m_type.IsObject() && (m_pObject != nullptr) )
	{
		// オブジェクト要素間接参照
		LType	typeElement = m_pObject->GetElementTypeAt( index ) ;
		LObjPtr	pElement( m_pObject->GetElementAt( index ) ) ;
		if ( (pElement != nullptr) && typeElement.IsPointer() )
		{
			LPointerObj *	pPtrObj =
					dynamic_cast<LPointerObj*>( pElement.Ptr() ) ;
			if ( pPtrObj != nullptr )
			{
				LPtr<LPointerObj>	pPtrMember
						( new LPointerObj( typeElement.GetClass() ) ) ;
				*pPtrMember = *pPtrObj ;
				return	LValue( pPtrMember ) ;
			}
		}
		return	LValue( typeElement, pElement ) ;
	}
	return	LValue() ;		// エラー
}

LValue LValue::GetMemberAs
	( LVirtualMachine& vm, const wchar_t * name, bool flagRef ) const
{
	if ( m_type.IsPointer() )
	{
		// ポインタ
		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;
		if ( pPtrObj == nullptr )
		{
			return	LValue() ;
		}
		LPointerClass *	pPtrClass = m_type.GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		const LType&	typeBuf = pPtrClass->GetBufferType() ;
		if ( typeBuf.IsStructure() )
		{
			// 構造体ポインタのメンバ
			LStructureClass *	pStructClass = typeBuf.GetStructureClass() ;
			assert( pStructClass != nullptr ) ;

			const LArrangementBuffer&
					arrangeBuf = pStructClass->GetProtoArrangemenet() ;
			LArrangement::Desc	desc ;
			if ( arrangeBuf.GetDescAs( desc, name ) )
			{
				LType	typeElement = desc.m_type ;
				if ( typeElement.IsDataArray() )
				{
					typeElement = typeElement.GetRefElementType() ;
				}
				LPtr<LPointerObj>	pPtrMember
					( new LPointerObj( vm.GetPointerClassAs( typeElement ) ) ) ;
				pPtrMember->SetPointer
					( pPtrObj->GetArrayBuffer(),
						pPtrObj->GetOffset() + desc.m_location, desc.m_size ) ;
				if ( flagRef )
				{
					return	LValue( pPtrMember ) ;
				}
				return	LValue( pPtrMember ).UnboxingData() ;
			}
		}
	}
	else if ( m_type.IsObject() )
	{
		// オブジェクト
		if ( m_pObject == nullptr )
		{
			return	LValue() ;
		}
		ssize_t	iElement = m_pObject->FindElementAs( name ) ;
		if ( iElement >= 0 )
		{
			LType	typeElement = m_pObject->GetElementTypeAt( (size_t) iElement ) ;
			LObjPtr	pElement( m_pObject->GetElementAt( (size_t) iElement ) ) ;
			if ( (pElement != nullptr)
				&& typeElement.IsPointer() )
			{
				LPointerObj *	pPtrObj =
						dynamic_cast<LPointerObj*>( pElement.Ptr() ) ;
				if ( pPtrObj != nullptr )
				{
					LPtr<LPointerObj>	pPtrMember
							( new LPointerObj( typeElement.GetClass() ) ) ;
					*pPtrMember = *pPtrObj ;
					return	LValue( pPtrMember ) ;
				}
			}
			return	LValue( typeElement, pElement ) ;
		}
		const LArrangementBuffer&
				arrangeBuf = m_type.GetClass()->GetProtoArrangemenet() ;
		LArrangement::Desc	desc ;
		if ( arrangeBuf.GetDescAs( desc, name ) )
		{
			// クラスメンバ変数へのポインタ
			LPtr<LPointerObj>	pPtrObj( m_pObject->GetBufferPoiner() ) ;
			if ( pPtrObj != nullptr )
			{
				LType	typeElement = desc.m_type ;
				if ( typeElement.IsDataArray() )
				{
					typeElement = typeElement.GetRefElementType() ;
				}
				LPtr<LPointerObj>	pPtrMember
					( new LPointerObj( vm.GetPointerClassAs( typeElement ) ) ) ;
				pPtrMember->SetPointer
					( pPtrObj->GetArrayBuffer(),
						pPtrObj->GetOffset() + desc.m_location, desc.m_size ) ;
				if ( flagRef )
				{
					return	LValue( pPtrMember ) ;
				}
				return	LValue( pPtrMember ).UnboxingData() ;
			}
		}
	}
	return	LValue() ;		// エラー
}

// 要素数取得
size_t LValue::GetElementCount( void ) const
{
	if ( m_type.IsObject() && (m_pObject != nullptr) )
	{
		return	m_pObject->GetElementCount() ;
	}
	return	0 ;
}

// 要素名取得
const wchar_t * LValue::GetElementNameAt
						( LString& strName, size_t index ) const
{
	if ( m_type.IsObject() && (m_pObject != nullptr) )
	{
		return	m_pObject->GetElementNameAt( strName, index ) ;
	}
	return	nullptr ;
}

// ポインタの参照先がプリミティブ型／
// ボックス化されたオブジェクトの場合評価する
LValue LValue::UnboxingData( void ) const
{
	if ( m_type.IsPointer() )
	{
		LPointerClass *	pPtrClass = m_type.GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( m_pObject.Ptr() ) ;

		LType	typeBuf = pPtrClass->GetBufferType() ;
		if ( typeBuf.IsPrimitive() && (pPtrObj != nullptr) )
		{
			// プリミティブ型
			std::uint8_t *	pBuf =
					pPtrObj->GetPointer( 0, typeBuf.GetDataBytes() ) ;
			if ( pBuf != nullptr )
			{
				LType::Primitive	type = typeBuf.GetPrimitive() ;
				if ( typeBuf.IsFloatingPointNumber() )
				{
					return	LValue( type, LValue::MakeDouble
								( (LPointerObj::s_pnfLoadAsDouble[type])( pBuf ) ) ) ;
				}
				else
				{
					return	LValue( type, LValue::MakeLong
								( (LPointerObj::s_pnfLoadAsLong[type])( pBuf ) ) ) ;
				}
			}
		}
	}
	else if ( m_type.IsObject() )
	{
		LIntegerObj *	pIntObj =
				dynamic_cast<LIntegerObj*>( m_pObject.Ptr() ) ;
		if ( pIntObj != nullptr )
		{
			return	LValue( LType::typeInt64,
							LValue::MakeLong( pIntObj->m_value ) ) ;
		}
		LDoubleObj *	pFloatObj =
				dynamic_cast<LDoubleObj*>( m_pObject.Ptr() ) ;
		if ( pFloatObj != nullptr )
		{
			return	LValue( LType::typeDouble,
							LValue::MakeDouble( pFloatObj->m_value ) ) ;
		}
	}
	return	*this ;
}



//////////////////////////////////////////////////////////////////////////
// コンパイル時の数式の評価値
//////////////////////////////////////////////////////////////////////////

// 単体要素の評価値として設定
void LExprValue::SetSingle( ExprOptionType optType )
{
	m_optType = optType ;
	assert( IsSingle() ) ;
}

// 単体要素か？
bool LExprValue::IsSingle( void ) const
{
	return	(m_optType < exprFirstHasOptions) ;
}

// 第二、第三要素の評価値を設定
void LExprValue::SetOption
	( ExprOptionType type,
		std::shared_ptr<LExprValue> val1,
		std::shared_ptr<LExprValue> val2 )
{
	m_optType = type ;
	m_optValue1 = val1 ;
	m_optValue2 = val2 ;
}

void LExprValue::SetOptionPointerOffset
	( std::shared_ptr<LExprValue> valPtr,
		std::shared_ptr<LExprValue> valOffset )
{
	SetOption( exprPtrOffset, valPtr, valOffset ) ;
}

void LExprValue::SetOptionRefPointerOffset
	( std::shared_ptr<LExprValue> valPtr,
		std::shared_ptr<LExprValue> valOffset )
{
	SetOption( exprRefPtrOffset, valPtr, valOffset ) ;
}

void LExprValue::SetOptionRefByIndexOf
	( std::shared_ptr<LExprValue> valObj,
		std::shared_ptr<LExprValue> valIndex )
{
	SetOption( exprRefByIndex, valObj, valIndex ) ;
}

void LExprValue::SetOptionRefByStrOf
	( std::shared_ptr<LExprValue> valObj,
		std::shared_ptr<LExprValue> valStr )
{
	SetOption( exprRefByStr, valObj, valStr ) ;
}

void LExprValue::SetOptionRefCallThisOf
	( std::shared_ptr<LExprValue> valThisObj,
		std::shared_ptr<LExprValue> valFunc )
{
	SetOption( exprRefCallThis, valThisObj, valFunc ) ;
}

// オプション・タイプ
LExprValue::ExprOptionType LExprValue::OptionType( void ) const
{
	return	m_optType ;
}

// 参照型として修飾されているか？
bool LExprValue::IsReference( void ) const
{
	return	IsRefPointer() || IsOnLocal()
			|| IsObjReference() || IsPointerOnLocal() ;
}

// ローカル上のエントリか？
bool LExprValue::IsOnLocal( void ) const
{
	return	m_localVar ;
}

bool LExprValue::IsPointerOnLocal( void ) const
{
	if ( m_optType == exprPtrOffset )
	{
		return	(m_optValue1 != nullptr)
				&& m_optValue1->IsOnLocal() ;
	}
	return	false ;
}

bool LExprValue::IsPtrOffsetOnLocal( void ) const
{
	if ( m_optType == exprPtrOffset )
	{
		return	(m_optValue2 != nullptr)
				&& m_optValue2->IsOnLocal() ;
	}
	return	false ;
}

// 参照ポインタか？（実態はポインタでコンパイル時の型情報が参照）
bool LExprValue::IsRefPointer( void ) const
{
	return	(m_optType == exprRefPointer) || (m_optType == exprRefPtrOffset) ;
}

// ポインタを参照ポインタに変更する
void LExprValue::MakeIntoRefPointer( void )
{
	assert( m_type.IsPointer() ) ;
	assert( (m_optType == exprSingle) || (m_optType == exprPtrOffset) ) ;
	if ( m_optType == exprSingle )
	{
		m_optType = exprRefPointer ;
	}
	else if ( m_optType == exprPtrOffset )
	{
		m_optType = exprRefPtrOffset ;
	}
}

// 参照ポインタをポインタに変更する
void LExprValue::MakeRefIntoPointer( void )
{
	assert( (m_optType == exprRefPointer) || (m_optType == exprRefPtrOffset) ) ;
	if ( m_optType == exprRefPointer )
	{
		m_optType = exprSingle ;
	}
	else if ( m_optType == exprRefPtrOffset )
	{
		m_optType = exprPtrOffset ;
	}
}

// オフセット付きポインタか？
bool LExprValue::IsOffsetPointer( void ) const
{
	return	(m_optType == exprPtrOffset) ;
}

bool LExprValue::IsRefOffsetPointer( void ) const
{
	return	(m_optType == exprRefPtrOffset) ;
}

// fetch_addr ポインタか？
bool LExprValue::IsFetchAddrPointer( void ) const
{
	if ( m_type.IsFetchAddr() )
	{
		return	true ;
	}
	if ( IsOffsetPointer() || IsRefOffsetPointer() )
	{
		if ( m_optValue1 != nullptr )
		{
			return	m_optValue1->IsFetchAddrPointer() ;
		}
	}
	return	false ;
}

// 解放が必要な型か？
bool LExprValue::IsTypeNeededToRelease( void )
{
	return	!m_type.IsPrimitive() && !IsFetchAddrPointer() ;
}

// オブジェクト要素参照か？
bool LExprValue::IsObjReference( void ) const
{
	return	IsRefByIndex() || IsRefByString() ;
}

bool LExprValue::IsRefByIndex( void ) const
{
	return	(m_optType == exprRefByIndex) ;
}

bool LExprValue::IsRefByString( void ) const
{
	return	(m_optType == exprRefByStr) ;
}

// 関数間接呼び出しか？
bool LExprValue::IsRefCallThis( void ) const
{
	return	(m_optType == exprRefCallThis)
			&& (m_optValue1 != nullptr)
			&& (m_optValue2 != nullptr) ;
}

// 定数値判定
bool LExprValue::IsConstExpr( void ) const
{
	return	m_constExpr
			&& ((m_optValue1 == nullptr) || m_optValue1->IsConstExpr())
			&& ((m_optValue2 == nullptr) || m_optValue2->IsConstExpr()) ;
}

bool LExprValue::IsUniqueObject( void ) const
{
	return	m_constExpr && m_uniqueObj ;
}

bool LExprValue::IsFunctionVariation( void ) const
{
	return	(m_optType == exprFuncVar)
			&& (m_pFuncVar != nullptr) ;
}

bool LExprValue::IsOperatorFunction( void ) const
{
	return	(m_optType == exprOperator)
			&& (m_pOperatorDef != nullptr) ;
}

bool LExprValue::IsRefCallFunction( void ) const
{
	return	(m_optType == exprRefCallThis)
			&& (m_optValue1 != nullptr)
			&& (m_optValue2 != nullptr) ;
}

bool LExprValue::IsRefVirtualFunction( void ) const
{
	return	(m_optType == exprRefVirtual)
			&& (m_pVirtClass != nullptr)
			&& (m_pVirtuals != nullptr) ;
}

bool LExprValue::IsNamespace( void ) const
{
	return	(m_optType == exprNamespace)
			&& (m_namespace != nullptr) ;
}

bool LExprValue::IsConstExprClass( void ) const
{
	return	IsSingle() && IsConstExpr()
			&& (GetConstExprClass() != nullptr) ;
}

bool LExprValue::IsConstExprFunction( void ) const
{
	return	IsSingle() && IsConstExpr()
			&& m_type.IsObject()
			&& (m_pObject != nullptr)
			&& (dynamic_cast<LFunctionObj*>( m_pObject.Ptr() ) != nullptr) ;
}

// 定数式評価値取得
LClass * LExprValue::GetConstExprClass( void ) const
{
	if ( m_type.IsObject() && (m_pObject != nullptr) )
	{
		return	dynamic_cast<LClass*>( m_pObject.Ptr() ) ;
	}
	return	nullptr ;
}

LPtr<LFunctionObj> LExprValue::GetConstExprFunction( void ) const
{
	if ( m_type.IsObject() && (m_pObject != nullptr) )
	{
		LFunctionObj *	pFuncObj = dynamic_cast<LFunctionObj*>( m_pObject.Ptr() ) ;
		if ( pFuncObj != nullptr )
		{
			pFuncObj->AddRef() ;
			return	LPtr<LFunctionObj>( pFuncObj );
		}
	}
	return	LPtr<LFunctionObj>() ;
}

// 静的な関数群
const LFunctionVariation * LExprValue::GetFuncVariation( void ) const
{
	assert( IsFunctionVariation() ) ;
	return	m_pFuncVar ;
}

// 仮想関数
LClass * LExprValue::GetVirtFuncClass( void ) const
{
	assert( IsRefVirtualFunction() ) ;
	return	m_pVirtClass ;
}

const std::vector<size_t> * LExprValue::GetVirtFunctions( void ) const
{
	assert( IsRefVirtualFunction() ) ;
	return	m_pVirtuals ;
}

// 演算子関数
const Symbol::OperatorDef * LExprValue::GetOperatorFunction( void ) const
{
	assert( IsOperatorFunction() ) ;
	return	m_pOperatorDef ;
}

// 名前空間
LPtr<LNamespace> LExprValue::GetNamespace( void ) const
{
	assert( IsNamespace() ) ;
	return	m_namespace ;
}

// 第二要素
std::shared_ptr<LExprValue> LExprValue::GetOption1( void ) const
{
	return	m_optValue1 ;
}

std::shared_ptr<LExprValue> LExprValue::GetNonOffsetPointer( void )
{
	if ( m_optValue1 != nullptr )
	{
		return	(m_optValue1->m_optValue1 == nullptr)
					? m_optValue1 : m_optValue1->GetNonOffsetPointer() ;
	}
	return	nullptr ;
}

std::shared_ptr<LExprValue> LExprValue::MoveOption1( void )
{
	return	std::move( m_optValue1 ) ;
}

void LExprValue::SetOption1( std::shared_ptr<LExprValue> pOpt1 )
{
	m_optValue1 = pOpt1 ;
}

// 第三要素
std::shared_ptr<LExprValue> LExprValue::GetOption2( void ) const
{
	return	m_optValue2 ;
}

std::shared_ptr<LExprValue> LExprValue::MoveOption2( void )
{
	return	std::move( m_optValue2 ) ;
}

void LExprValue::SetOption2( std::shared_ptr<LExprValue> pOpt2 )
{
	m_optValue2 = pOpt2 ;
}

// 即値生成
std::shared_ptr<LExprValue> LExprValue::MakeConstExprInt( LLong val )
{
	LExprValuePtr		xval = std::make_shared<LExprValue>() ;
	LType::Primitive	typeInt = LType::typeInt64 ;
	if ( val < 0 )
	{
		if ( val >= -0x80 )
		{
			typeInt = LType::typeInt8 ;
		}
		else if ( val >= -0x8000 )
		{
			typeInt = LType::typeInt16 ;
		}
		else if ( val >= -(LLong)0x80000000UL )
		{
			typeInt = LType::typeInt32 ;
		}
	}
	else
	{
		if ( val <= 0xff )
		{
			typeInt = LType::typeUint8 ;
		}
		else if ( val <= 0xFFFF )
		{
			typeInt = LType::typeUint16 ;
		}
		else if ( val <= 0xFFFFFFFFUL )
		{
			typeInt = LType::typeUint32 ;
		}
	}
	xval->SetConstValue( LValue( typeInt, LValue::MakeLong(val) ) ) ;
	return	xval ;
}

std::shared_ptr<LExprValue> LExprValue::MakeConstExprFloat( LDouble val )
{
	LExprValuePtr	xval = std::make_shared<LExprValue>() ;
	xval->SetConstValue( LValue( LType::typeFloat, LValue::MakeDouble(val) ) ) ;
	return	xval ;
}

std::shared_ptr<LExprValue> LExprValue::MakeConstExprDouble( LDouble val )
{
	LExprValuePtr	xval = std::make_shared<LExprValue>() ;
	xval->SetConstValue( LValue( LType::typeDouble, LValue::MakeDouble(val) ) ) ;
	return	xval ;
}

std::shared_ptr<LExprValue> LExprValue::MakeConstExprObject( LObjPtr pObject )
{
	LExprValuePtr	xval = std::make_shared<LExprValue>() ;
	if ( pObject != nullptr )
	{
		xval->SetConstValue
				( LValue( LType(pObject->GetClass()), pObject ) ) ;
	}
	return	xval ;
}

std::shared_ptr<LExprValue>
	LExprValue::MakeConstExprString( LVirtualMachine& vm, const LString& str )
{
	LExprValuePtr	xval = std::make_shared<LExprValue>() ;
	LClass *	pStringClass = vm.GetStringClass() ;
	xval->SetConstValue
		( LValue( LType(pStringClass),
					LObjPtr( new LStringObj( pStringClass, str ) ) ) ) ;
	return	xval ;
}



//////////////////////////////////////////////////////////////////////////////
// 実行時の型と値の配列（関数の引数や、数式の実行時スタック等）
//////////////////////////////////////////////////////////////////////////////

// プッシュ
LExprValuePtr LExprValueArray::PushBool( LBoolean val )
{
	LExprValuePtr	expr = std::make_shared<LExprValue>() ;
	expr->SetConstValue( LValue( LType::typeBoolean, LValue::MakeBool( val ) ) ) ;
	return	Push( expr ) ;
}

LExprValuePtr LExprValueArray::PushLong( LLong val )
{
	LExprValuePtr	expr = std::make_shared<LExprValue>() ;
	expr->SetConstValue( LValue( LType::typeInt64, LValue::MakeLong( val ) ) ) ;
	return	Push( expr ) ;
}

LExprValuePtr LExprValueArray::PushDouble( LDouble val )
{
	LExprValuePtr	expr = std::make_shared<LExprValue>() ;
	expr->SetConstValue( LValue( LType::typeDouble, LValue::MakeDouble( val ) ) ) ;
	return	Push( expr ) ;
}

LExprValuePtr LExprValueArray::PushObject( const LType& type, LObjPtr pObj )
{
	LExprValuePtr	expr = std::make_shared<LExprValue>() ;
	expr->SetConstValue( LValue( type, pObj ) ) ;
	return	Push( expr ) ;
}

LExprValuePtr LExprValueArray::PushRuntimeType( const LType& type, bool responsible )
{
	LExprValuePtr	expr = std::make_shared<LExprValue>() ;
	expr->SetType( type, false ) ;
	return	Push( expr ) ;
}

// 比較
bool LExprValueArray::IsEqual( const LExprValueArray& xvalArray ) const
{
	if ( std::vector<LExprValuePtr>::size() != xvalArray.size() )
	{
		return	false ;
	}
	for ( size_t i = 0; i < std::vector<LExprValuePtr>::size(); i ++ )
	{
		if ( std::vector<LExprValuePtr>::at(i) != xvalArray.at(i) )
		{
			return	false ;
		}
	}
	return	true ;
}

// 要素検索
ssize_t LExprValueArray::Find( LExprValuePtr expr ) const
{
	for ( size_t i = 0; i < std::vector<LExprValuePtr>::size(); i ++ )
	{
		if ( std::vector<LExprValuePtr>::at(i) == expr )
		{
			return	(ssize_t) i ;
		}
	}
	return	-1 ;
}

ssize_t LExprValueArray::FindBack( LExprValuePtr expr ) const
{
	ssize_t	i = Find( expr ) ;
	if ( i < 0 )
	{
		return	-1 ;
	}
	return	(ssize_t) BackIndex( (size_t) i ) ;
}

// 解放可能なスタック数を数える
size_t LExprValueArray::CountBackTemporaries( void )
{
	const size_t	nLength = GetLength() ;
	size_t	nGarbage ;
	do
	{
		nGarbage = 0 ;
		for ( size_t i = 0; i < nLength; i ++ )
		{
			if ( std::vector<LExprValuePtr>::at(i).use_count() == 1 )
			{
				std::vector<LExprValuePtr>::at(i) = nullptr ;
				nGarbage ++ ;
			}
		}
	}
	while ( nGarbage != 0 ) ;

	for ( size_t i = 0; i < nLength; i ++ )
	{
		if ( std::vector<LExprValuePtr>::at(BackIndex(i)) != nullptr )
		{
			return	i ;
		}
	}
	return	nLength ;
}

// スタックを開放する
void LExprValueArray::FreeStack( size_t nCount )
{
	if ( nCount <= GetLength() )
	{
		std::vector<LExprValuePtr>::resize( GetLength() - nCount ) ;
	}
	else
	{
		std::vector<LExprValuePtr>::clear() ;
	}
}



//////////////////////////////////////////////////////////////////////////////
// 実行時のフレームバッファの型と値の配置
//////////////////////////////////////////////////////////////////////////////

// 使用領域サイズを切り詰める
void LLocalVarArray::TruncateUsedSize( size_t nUsed )
{
	m_nUsed = std::min( nUsed, m_nUsed ) ;

	std::shared_ptr<LLocalVarArray>	pParent = m_pParent ;
	size_t	nLocals = m_nUsed ;
	while ( pParent != nullptr )
	{
		nLocals += pParent->m_nUsed ;
		pParent = pParent->m_pParent ;
	}

	auto	iterNames = m_mapNames.begin() ;
	while ( iterNames != m_mapNames.end() )
	{
		LLocalVarPtr	pVar = GetLocalVarAt( iterNames->second ) ;
		if ( (pVar != nullptr) && (pVar->GetLocation() >= nLocals) )
		{
			iterNames = m_mapNames.erase( iterNames ) ;
		}
		else
		{
			iterNames ++ ;
		}
	}

	auto	iterVars = m_mapVars.begin() ;
	while ( iterVars != m_mapVars.end() )
	{
		LLocalVarPtr	pVar = iterVars->second ;
		if ( (pVar != nullptr) && (pVar->GetLocation() >= nLocals) )
		{
			iterVars = m_mapVars.erase( iterVars ) ;
		}
		else
		{
			iterVars ++ ;
		}
	}
}

// 変数追加
LLocalVarPtr LLocalVarArray::AddLocalVar
	( const wchar_t * pwszName, const LType& type )
{
	LLocalVarPtr	pVar = AllocLocalVar( type ) ;

	SetLocalVarNameAs( pwszName, pVar ) ;
	return	pVar ;
}

LLocalVarPtr LLocalVarArray::AllocLocalVar( const LType& type )
{
	const size_t	nLocals = m_nUsed ;
	size_t			iLoc = nLocals ;
	std::shared_ptr<LLocalVarArray>	pParent = m_pParent ;
	while ( pParent != nullptr )
	{
		iLoc += pParent->m_nUsed ;
		pParent = pParent->m_pParent ;
	}

	LLocalVarPtr	pVar ;
	size_t			nStorages = 1 ;
	if ( type.IsDataArray() || type.IsStructure() )
	{
		// データ配列や構造体
		nStorages = (type.GetDataBytes() + sizeof(LValue::Primitive) - 1)
											/ sizeof(LValue::Primitive) ;
		if ( nStorages == 0 )
		{
			nStorages = 1 ;
		}
		pVar = std::make_shared<LLocalVar>
					( type, LLocalVar::allocLocalArray, iLoc ) ;
	}
	else if ( !type.IsNeededToRelease() )
	{
		// プリミティブ型、fetch_addr ポインタ
		pVar = std::make_shared<LLocalVar>
					( type, LLocalVar::allocPrimitive, iLoc ) ;
	}
	else if ( type.IsPointer() )
	{
		// ポインタ
		LLocalVarPtr	pVarPtr =
				std::make_shared<LLocalVar>
					( type, LLocalVar::allocPointer, iLoc ) ;
		LLocalVarPtr	pVarIndex =
				std::make_shared<LLocalVar>
					( LType(LType::typeInt64),
							LLocalVar::allocPrimitive, iLoc + 2 ) ;
		m_mapVars.insert
			( std::make_pair<size_t,LLocalVarPtr>
							( iLoc + 2, LLocalVarPtr(pVarIndex) ) ) ;
		pVar = pVarPtr ;
		nStorages = 3 ;
	}
	else
	{
		// オブジェクト
		assert( type.IsObject() ) ;

		pVar = std::make_shared<LLocalVar>
					( type, LLocalVar::allocObject, iLoc ) ;
		nStorages = 2 ;
	}
	m_nUsed += nStorages ;

	m_mapVars.insert
		( std::make_pair<size_t,LLocalVarPtr>
							( (size_t) iLoc, LLocalVarPtr(pVar) ) ) ;
	return	pVar ;
}

LLocalVarPtr LLocalVarArray::PutLocalVar( size_t iLoc, const LType& type )
{
	std::shared_ptr<LLocalVarArray>	pParent = m_pParent ;
	while ( pParent != nullptr )
	{
		iLoc += pParent->m_nUsed ;
		pParent = pParent->m_pParent ;
	}

	LLocalVarPtr	pVar ;
	if ( !type.IsNeededToRelease() )
	{
		// プリミティブ型、fetch_addr ポインタ
		pVar = std::make_shared<LLocalVar>
					( type, LLocalVar::allocPrimitive, iLoc ) ;
	}
	else
	{
		// オブジェクト
		assert( type.IsObject() ) ;
		pVar = std::make_shared<LLocalVar>
					( type, LLocalVar::allocObject, iLoc ) ;
	}

	m_mapVars.insert
		( std::make_pair<size_t,LLocalVarPtr>
							( (size_t) iLoc, LLocalVarPtr(pVar) ) ) ;
	return	pVar ;
}

void LLocalVarArray::SetLocalVarNameAs
	( const wchar_t * pwszName, LLocalVarPtr  pVar )
{
	assert( pVar != nullptr ) ;
	m_mapNames.insert
		( std::make_pair<std::wstring,size_t>( pwszName, pVar->GetLocation() ) ) ;
}

// 変数取得（この配列のみ）
LLocalVarPtr LLocalVarArray::GetLocalVarAs( const wchar_t * pwszName ) const
{
	auto	iterIndex = m_mapNames.find( pwszName ) ;
	if ( iterIndex == m_mapNames.end() )
	{
		return	nullptr ;
	}
	return	GetLocalVarAt( iterIndex->second ) ;
}

LLocalVarPtr LLocalVarArray::GetLocalVarAt( size_t iLoc ) const
{
	auto	iterVar = m_mapVars.find( iLoc ) ;
	if ( iterVar == m_mapVars.end() )
	{
		return	nullptr ;
	}
	return	iterVar->second ;
}

// 変数名取得
const std::wstring * LLocalVarArray::GetLocalVarNameAt( size_t iLoc ) const
{
	for ( auto iter = m_mapNames.begin(); iter != m_mapNames.end(); iter ++ )
	{
		if ( iter->second == iLoc )
		{
			return	&(iter->first) ;
		}
	}
	return	nullptr ;
}

// 変数を検索（この配列のみ）
ssize_t LLocalVarArray::FindLocalVar( LExprValuePtr xval ) const
{
	for ( auto iter : m_mapVars )
	{
		if ( iter.second == xval )
		{
			return	(ssize_t) iter.first ;
		}
	}
	return	-1 ;
}

// fetch_addr 修飾された変数が存在するか？（この配列のみ）
bool LLocalVarArray::AreThereAnyFetchAddr( void ) const
{
	for ( auto iter : m_mapVars )
	{
		if ( iter.second->GetType().IsFetchAddr() )
		{
			return	true ;
		}
	}
	return	false ;
}

// 適合するか？（この配列のみ）
bool LLocalVarArray::DoesMatchWith
	( const LLocalVarArray& lvaFrom, bool flagEqueal ) const
{
	if ( m_nUsed > lvaFrom.m_nUsed )
	{
		return	false ;
	}
	if ( flagEqueal )
	{
		if ( m_nUsed != lvaFrom.m_nUsed )
		{
			return	false ;
		}
	}
	for ( auto iter : m_mapVars )
	{
		auto	i = lvaFrom.m_mapVars.find( iter.first ) ;
		if ( i == lvaFrom.m_mapVars.end() )
		{
			return	false ;
		}
		if ( iter.second != i->second )
		{
			return	false ;
		}
	}
	return	true ;
}

// 結合
const LLocalVarArray&
	LLocalVarArray::operator += ( const LLocalVarArray& lva )
{
	m_nUsed += lva.m_nUsed ;

	for ( auto iter : lva.m_mapVars )
	{
		m_mapVars.insert
			( std::make_pair<size_t,LLocalVarPtr>
				( (size_t) iter.first, LLocalVarPtr( iter.second ) ) ) ;
	}

	for ( auto iter : lva.m_mapNames )
	{
		m_mapNames.insert
			( std::make_pair<std::wstring,size_t>
				( iter.first.c_str(), (size_t) iter.second ) ) ;
	}

	return	*this ;
}



