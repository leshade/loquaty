
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Loquaty オブジェクトクラス
//////////////////////////////////////////////////////////////////////////////

// 構築関数名
LString LClass::GetConstructerName( void ) const
{
	ssize_t	iParam = LNamespace::m_name.Find( L"<" ) ;
	if ( iParam > 0 )
	{
		return	LNamespace::m_name.Middle( 0, (size_t) iParam ) ;
	}
	return	LNamespace::m_name ;
}

// pClass へキャスト可能か？（データの変換なしのキャスト）
//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
LClass::ResultInstanceOf
		LClass::TestInstanceOf( LClass * pClass ) const
{
	assert( pClass != nullptr ) ;
	if ( pClass == this )
	{
		return	instanceAvailable ;
	}
	if ( m_pSuperClass != nullptr )
	{
		ResultInstanceOf	rs = m_pSuperClass->TestInstanceOf( pClass ) ;
		if ( rs != instanceUnavailable )
		{
			return	rs ;
		}
	}
	for ( size_t i = 0; i < m_vecImplements.size(); i ++ )
	{
		assert( m_vecImplements.at(i) != nullptr ) ;
		ResultInstanceOf	rs = m_vecImplements.at(i)->TestInstanceOf( pClass ) ;
		if ( rs != instanceUnavailable )
		{
			return	rs ;
		}
	}
	return	instanceUnavailable ;
}

// 複製する（要素も全て複製処理する）
LObject * LClass::CloneObject( void ) const
{
	return	new LClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LClass::DuplicateObject( void ) const
{
	return	new LClass( *this ) ;
}

// ローカルクラス・名前空間以外の全ての要素を解放
void LClass::DisposeAllObjects( void )
{
	LNamespace::DisposeAllObjects() ;

	m_vfvVirtualFuncs.clear() ;
	m_mapVirtualFuncs.clear() ;

	m_pFuncFinalize = nullptr ;

	m_pPrototype = nullptr ;
	m_protoBuffer.ClearAll() ;
	m_nSuperBufSize = 0 ;

	m_vecUnaryOperators.clear() ;
	m_vecBinaryOperators.clear() ;
}

// 仮想関数ベクタ取得
const LVirtualFuncVector *
		LClass::GetVirtualVectorOf( LClass * pClass ) const
{
	if ( pClass == this )
	{
		return	&m_vfvVirtualFuncs ;
	}
	auto	iter = m_mapVirtualFuncs.find( pClass ) ;
	if ( iter != m_mapVirtualFuncs.end() )
	{
		return	&(iter->second) ;
	}
	return	nullptr ;
}

const LVirtualFuncVector& LClass::GetVirtualVector( void ) const
{
	return	m_vfvVirtualFuncs ;
}

// オブジェクト生成（コンストラクタは後で呼び出し元責任で呼び出す）
LObject * LClass::CreateInstance( void )
{
	if ( m_pPrototype != nullptr )
	{
		return	m_pPrototype->CloneObject() ;
	}
	return	new LObject( this ) ;
}

// finalize() を呼び出す
void LClass::InvokeFinalizer( LObject * pObj )
{
	assert( pObj != nullptr ) ;
	if ( (m_pFuncFinalize != nullptr)
		&& m_pFuncFinalize->IsImplemented() )
	{
		// ※ Object::finalize は null な native 関数として設定されているので
		//    明示的にオーバーライドしない限りここは実行されない

		LContext *	pContext = LContext::GetCurrent() ;
		if ( pContext != nullptr )
		{
			pObj->AddRef() ;	// ※この時点で参照カウンタは 0 になっている
			pObj->AddRef() ;	//   １回は valArgs の LValue 用
								//   もう１回は LValue 解放時に再度参照カウンタが
								//    0 にならないようにするため（無限再帰防止）
								// ※この関数を抜けると参照カウンタによらず
								//   無条件に delete されるはずである

			LValue	valArgs[1] ;
			valArgs[0] = LValue( pObj ) ;

			pContext->SyncCallFunction( m_pFuncFinalize, valArgs, 1 ) ;
		}
	}
}

// 親クラス追加
bool LClass::AddSuperClass( LClass * pClass )
{
	assert( pClass != nullptr ) ;
	if ( m_pSuperClass != nullptr )
	{
		LCompiler::Error
			( errorMultipleDerivationClass,
				GetClassName().c_str(), pClass->GetClassName().c_str() ) ;
		return	false ;
	}
	if ( !TestExtendingClass( pClass ) )
	{
		return	false ;
	}
	m_pSuperClass = pClass ;
	//
	// プロトタイプオブジェクト複製
	//
	if ( (m_pPrototype == nullptr)
		&& (pClass->m_pPrototype != nullptr) )
	{
		m_pPrototype = pClass->m_pPrototype->CloneObject() ;
		m_pPrototype->SetClass( this ) ;
		//
		if ( dynamic_cast<LGenericObj*>( m_pPrototype.Ptr() ) != nullptr )
		{
			size_t	nCount = m_pPrototype->GetElementCount() ;
			for ( size_t i = 0; i < nCount; i ++ )
			{
				// private メンバは不可視に変更する
				LType	type = m_pPrototype->GetElementTypeAt( i ) ;
				type.MakePrivateInvisible() ;
				m_pPrototype->SetElementTypeAt( i, type ) ;
			}
		}
	}
	//
	// メンバ変数配置
	//
	AddMemberArrangement
		( ProtoArrangemenet(), pClass->GetProtoArrangemenet() ) ;
	ProtoArrangemenet().MakePrivateInvisible() ;
	//
	m_nSuperBufSize = ProtoArrangemenet().GetAlignedBytes() ;
	//
	// 仮想関数複製
	//
	m_vfvVirtualFuncs = pClass->m_vfvVirtualFuncs ;
	m_mapVirtualFuncs = pClass->m_mapVirtualFuncs ;
	//
	if ( m_mapVirtualFuncs.find( pClass ) == m_mapVirtualFuncs.end() )
	{
		m_mapVirtualFuncs.insert
			( std::make_pair<LClass*,LVirtualFuncVector>
				( (LClass*)pClass, LVirtualFuncVector(pClass->m_vfvVirtualFuncs) ) ) ;
	}
	//
	m_vfvVirtualFuncs.MakePrivateInvisible() ;
	for ( auto iter = m_mapVirtualFuncs.begin(); iter != m_mapVirtualFuncs.end(); iter ++ )
	{
		iter->second.MakePrivateInvisible() ;
	}
	//
	// 演算子定義複製
	//
	m_vecUnaryOperators = pClass->m_vecUnaryOperators ;
	m_vecBinaryOperators = pClass->m_vecBinaryOperators ;

	return	true ;
}

// インターフェース追加
bool LClass::AddImplementClass( LClass * pClass )
{
	if ( !TestExtendingClass( pClass ) )
	{
		return	false ;
	}
	m_vecImplements.push_back( pClass ) ;
	//
	if ( pClass->m_pPrototype != nullptr )
	{
		if ( pClass->m_pPrototype->GetElementCount() > 0 )
		{
			LCompiler::Error
				( errorInterfaceHasNoMemberVariable,
					GetClassName().c_str(), pClass->GetClassName().c_str() ) ;
		}
	}
	if ( pClass->GetProtoArrangemenet().GetAlignedSize() > 0 )
	{
		LCompiler::Error
			( errorInterfaceHasNoMemberVariable,
				GetClassName().c_str(), pClass->GetClassName().c_str() ) ;
	}
	//
	ImplementVirtuals( pClass ) ;

	return	true ;
}

bool LClass::TestExtendingClass( LClass * pClass )
{
	assert( pClass != nullptr ) ;
	if ( pClass->GetVirtualVectorOf( this ) != nullptr )
	{
		LCompiler::Error
			( errorDerivedCirculatingImplement,
				GetClassName().c_str(), pClass->GetClassName().c_str() ) ;
		return	false ;
	}
	if ( !pClass->m_flagCompleted )
	{
		LCompiler::Error
			( errorDerivedFromUndefinedClass,
				GetClassName().c_str(), pClass->GetClassName().c_str() ) ;
		return	false ;
	}
	return	true ;
}

void LClass::ImplementVirtuals( LClass * pClass )
{
	//
	// 仮想関数ベクタを登録
	//
	for ( auto iter = pClass->m_mapVirtualFuncs.begin();
				iter != pClass->m_mapVirtualFuncs.end(); iter ++ )
	{
		LClass *	pInterface = iter->first ;
		if ( m_mapVirtualFuncs.find( pInterface ) == m_mapVirtualFuncs.end() )
		{
			m_mapVirtualFuncs.insert
				( std::make_pair<LClass*,LVirtualFuncVector>
					( (LClass*)pInterface, LVirtualFuncVector(iter->second) ) ) ;
		}
	}
	if ( m_mapVirtualFuncs.find( pClass ) == m_mapVirtualFuncs.end() )
	{
		m_mapVirtualFuncs.insert
			( std::make_pair<LClass*,LVirtualFuncVector>
				( (LClass*)pClass, LVirtualFuncVector(pClass->m_vfvVirtualFuncs) ) ) ;
	}
	//
	// 仮想関数を登録
	//
	std::shared_ptr< std::vector<LString> >
			pVirtNames = pClass->m_vfvVirtualFuncs.GetFuncNameList() ;
	for ( size_t i = 0; i < pVirtNames->size(); i ++ )
	{
		const LString&				strFuncName = pVirtNames->at(i) ;
		const std::vector<size_t> *	pVirtIndices =
			pClass->m_vfvVirtualFuncs.FindFunction( strFuncName.c_str() ) ;
		if ( pVirtIndices == nullptr )
		{
			continue ;
		}
		for ( size_t j = 0; j < pVirtIndices->size(); j ++ )
		{
			LPtr<LFunctionObj>	pInterFunc =
				pClass->m_vfvVirtualFuncs.GetFunctionAt( pVirtIndices->at(j) ) ;
			assert( pInterFunc != nullptr ) ;

			ssize_t	iFoundFunc = m_vfvVirtualFuncs.FindFunction
				( strFuncName.c_str(), pInterFunc->GetArgListType(), nullptr ) ;
			if ( iFoundFunc >= 0 )
			{
				LPtr<LFunctionObj>	pImplFunc =
					m_vfvVirtualFuncs.GetFunctionAt( (size_t) iFoundFunc ) ;
				assert( pImplFunc != nullptr ) ;
				if ( !pImplFunc->IsEqualPrototype( *(pInterFunc->GetPrototype()) ) )
				{
					LCompiler::Warning
						( errorReturnTypeMismatch, LCompiler::warning3,
							strFuncName.c_str(),
							pInterFunc->GetPrototype()->TypeToString().c_str() ) ;
					m_vfvVirtualFuncs.AddFunction
						( strFuncName.c_str(), pInterFunc.Get() ) ;
				}
			}
			else
			{
				m_vfvVirtualFuncs.AddFunction
					( strFuncName.c_str(), pInterFunc.Get() ) ;
			}
		}
	}
}

// 親クラス取得
LClass * LClass::GetSuperClass( void ) const
{
	return	m_pSuperClass ;
}

const std::vector<LClass*>& LClass::GetImplementClasses( void ) const
{
	return	m_vecImplements ;
}

// プロトタイプオブジェクト設定
void LClass::SetPrototypeObject( LObjPtr pPrototype )
{
	m_pPrototype = pPrototype ;
}

// プロトタイプオブジェクト取得
LObjPtr LClass::GetPrototypeObject( void ) const
{
	return	m_pPrototype ;
}

// クラス定義処理（ネイティブな実装）
void LClass::ImplementClass( void )
{
}

// クラス定義完成時処理
void LClass::CompleteClass( void )
{
	// バッファ確保
	m_arrangement.AllocateBuffer() ;
	m_protoBuffer.AllocateBuffer() ;

	LGenericObj *	pGenObj = dynamic_cast<LGenericObj*>( m_pPrototype.Ptr() ) ;
	if ( pGenObj != nullptr )
	{
		pGenObj->AllocateDataBuffer() ;
	}

	// finalize 関数を取得
	LArgumentListType	argListVoid ;
	ssize_t	iFinalizeFunc =
			m_vfvVirtualFuncs.FindFunction
				( L"finalize", argListVoid, nullptr, LType::modifierPrivate ) ;
	if ( iFinalizeFunc >= 0 )
	{
		m_pFuncFinalize =
			m_vfvVirtualFuncs.GetFunctionAt( (size_t) iFinalizeFunc ).Ptr() ;
	}

	m_flagCompleted = true ;
}

// クラス定義済みか？
bool LClass::IsClassCompleted( void ) const
{
	return	m_flagCompleted ;
}

// 抽象クラスか？
bool LClass::IsAbstractClass( void ) const
{
	for ( auto func : m_vfvVirtualFuncs )
	{
		if ( func->GetPrototype()->GetModifiers() & LType::modifierAbstract )
		{
			return	true ;
		}
	}
	return	false ;
}

// 配置情報からメンバ変数を追加
void LClass::AddMemberArrangement
	( LArrangementBuffer& arrangeDst, const LArrangementBuffer& arrangeSrc )
{
	std::map< std::wstring, LArrangement::Desc >
			mapDoubled = arrangeDst.MergeFrom( arrangeSrc ) ;

	for ( auto iter = mapDoubled.begin(); iter != mapDoubled.end(); iter ++ )
	{
		LCompiler::Warning
			( errorDoubleDefinitionOfMember,
				LCompiler::warning1,
				iter->first.c_str(),
				iter->second.m_type.GetTypeName().c_str() ) ;
	}
}

// 仮想関追加、またはオーバーライド
// （返り値は追加された関数インデックスと、オーバーライドされた以前の関数）
std::tuple< size_t, LPtr<LFunctionObj> >
	LClass::OverrideVirtualFunction
		( const wchar_t * pwszName,
			LPtr<LFunctionObj> pFunc, const LPrototype * pAsProto )
{
	pFunc->SetParentNamespace( this ) ;

	std::tuple< size_t, LPtr<LFunctionObj> >
		res = m_vfvVirtualFuncs.OverrideFunction( pwszName, pFunc, pAsProto ) ;

	for ( auto iter = m_mapVirtualFuncs.begin();
				iter != m_mapVirtualFuncs.end(); iter ++ )
	{
		if ( iter->first != this )
		{
			iter->second.OverrideFunction( pwszName, pFunc, pAsProto, true ) ;
		}
	}
	return	res ;
}

// ネイティブ仮想関数群を追加・又はオーバーライド
void LClass::OverrideVirtuals
	( const LClass::NativeFuncDesc * pnfDescs, const LNamespaceList * pnslLocal )
{
	assert( GetClass() != nullptr ) ;

	LNamespaceList	nsList ;
	nsList.AddNamespace( this ) ;
	if ( pnslLocal != nullptr )
	{
		nsList.AddNamespaceList( *pnslLocal ) ;
	}

	LCompiler			compiler( GetClass()->VM() ) ;
	LCompiler::Current	current( compiler ) ;

	while ( pnfDescs->m_pwszFuncName != nullptr )
	{
		std::shared_ptr<LPrototype>	pAsProto =
				GetOverridePrototype( *pnfDescs, compiler, nsList ) ;
		OverrideVirtualFunction
			( pnfDescs->m_pwszFuncName,
				CreateFunctionObj( *pnfDescs, compiler, nsList ), pAsProto.get() ) ;
		pnfDescs ++ ;
	}
}

// ネイティブ静関数群を追加・又はオーバーライド
void LClass::OverloadStaticFunctions
	( const LClass::NativeFuncDesc * pnfDescs, const LNamespaceList * pnslLocal )
{
	assert( GetClass() != nullptr ) ;

	LNamespaceList	nsList ;
	nsList.AddNamespace( this ) ;
	if ( pnslLocal != nullptr )
	{
		nsList.AddNamespaceList( *pnslLocal ) ;
	}

	LCompiler			compiler( GetClass()->VM() ) ;
	LCompiler::Current	current( compiler ) ;

	while ( pnfDescs->m_pwszFuncName != nullptr )
	{
		AddStaticFunctionAs
			( pnfDescs->m_pwszFuncName,
				CreateFunctionObj( *pnfDescs, compiler, nsList ) ) ;
		pnfDescs ++ ;
	}
}

// NativeFuncDesc から LFunctionObj を生成
LPtr<LFunctionObj> LClass::CreateFunctionObj
	( const LClass::NativeFuncDesc& nfDesc,
		LCompiler& compiler, const LNamespaceList& nsList )
{
	LSourceFile		sfModifiers = nfDesc.m_pwszModifiers ;
	LSourceFile		sfRetType = nfDesc.m_pwszRetType ;
	LSourceFile		sfArgList = nfDesc.m_pwszArgList ;

	LType::Modifiers	accMod = compiler.ParseAccessModifiers( sfModifiers ) ;
	LClass *			pThisClass = this ;
	if ( accMod & LType::modifierStatic )
	{
		pThisClass = nullptr ;
	}

	std::shared_ptr<LPrototype>
			pProto = std::make_shared<LPrototype>( pThisClass, accMod ) ;

	LVerify( sfRetType.NextTypeExpr
				( pProto->ReturnType(), true, compiler.VM(), &nsList ) ) ;

	compiler.ParsePrototypeArgmentList
		( sfArgList, *pProto, &nsList, nullptr ) ;
	assert( !compiler.HasErrorOnCurrent() ) ;

	if ( nfDesc.m_pwszComment != nullptr )
	{
		pProto->SetComment( MakeComment( nfDesc.m_pwszComment ) ) ;
	}

	LPtr<LFunctionObj>	pFunc =
		new LFunctionObj( compiler.VM().GetFunctionClass(), pProto ) ;

	pFunc->SetNative( nfDesc.m_pfnNativeFunc, nfDesc.m_callableInConstExpr ) ;
	pFunc->SetName( nfDesc.m_pwszFuncName ) ;

	return	pFunc ;
}

std::shared_ptr<LPrototype> LClass::GetOverridePrototype
	( const LClass::NativeFuncDesc& nfDesc,
		LCompiler& compiler, const LNamespaceList& nsList )
{
	if ( nfDesc.m_pwszAsArgList == nullptr )
	{
		return	nullptr ;
	}
	LSourceFile		sfModifiers = nfDesc.m_pwszModifiers ;
	LSourceFile		sfRetType = nfDesc.m_pwszRetType ;
	LSourceFile		sfArgList = nfDesc.m_pwszAsArgList ;

	LType::Modifiers	accMod = compiler.ParseAccessModifiers( sfModifiers ) ;
	LClass *			pThisClass = this ;
	if ( accMod & LType::modifierStatic )
	{
		pThisClass = nullptr ;
	}

	std::shared_ptr<LPrototype>
			pProto = std::make_shared<LPrototype>( pThisClass, accMod ) ;

	LVerify( sfRetType.NextTypeExpr
			( pProto->ReturnType(), true, compiler.VM(), &nsList ) ) ;

	compiler.ParsePrototypeArgmentList
		( sfArgList, *pProto, &nsList, nullptr ) ;
	assert( !compiler.HasErrorOnCurrent() ) ;

	return	pProto ;
}

// 変数定義を追加
void LClass::AddVariableDefinitions
	( const VariableDesc * pvDescs, const LNamespaceList * pnslLocal )
{
	assert( GetClass() != nullptr ) ;

	LNamespaceList	nsList ;
	nsList.AddNamespace( this ) ;
	if ( pnslLocal != nullptr )
	{
		nsList.AddNamespaceList( *pnslLocal ) ;
	}

	LCompiler			compiler( GetClass()->VM() ) ;
	LCompiler::Current	current( compiler ) ;

	while ( pvDescs->m_pwszName != nullptr )
	{
		AddVariableDefinition( *pvDescs, compiler, nsList ) ;
		pvDescs ++ ;
	}
}

void LClass::AddVariableDefinition
	( const LClass::VariableDesc& varDesc,
		LCompiler& compiler, const LNamespaceList& nsList )
{
	LSourceFile		sfModifiers = varDesc.m_pwszModifiers ;
	LSourceFile		sfType = varDesc.m_pwszType ;

	LType::Modifiers	accMod = compiler.ParseAccessModifiers( sfModifiers ) ;

	LType	typeVar ;
	LVerify( sfType.NextTypeExpr( typeVar, true, compiler.VM(), &nsList ) ) ;

	LType::LComment * pComment = nullptr ;
	if ( varDesc.m_pwszComment != nullptr )
	{
		pComment = MakeComment( varDesc.m_pwszComment ) ;
		typeVar.SetComment( pComment ) ;
	}

	if ( typeVar.CanArrangeOnBuf() )
	{
		// 非オブジェクト型変数
		LArrangementBuffer *	pArrangeBuf = nullptr ;
		if ( accMod & LType::modifierStatic )
		{
			pArrangeBuf = &m_arrangement ;
		}
		else
		{
			pArrangeBuf = &m_protoBuffer ;
		}
		typeVar.SetModifiers( typeVar.GetModifiers() | accMod );
		pArrangeBuf->AddDesc( varDesc.m_pwszName, typeVar, 1 ) ;

		LArrangement::Desc	desc ;
		if ( !pArrangeBuf->GetDescAs( desc, varDesc.m_pwszName ) )
		{
			compiler.OnError
				( errorDoubleDefinitionOfMember, varDesc.m_pwszName ) ;
			return ;
		}
		pArrangeBuf->AllocateBuffer() ;

		if ( varDesc.m_pwszInitExpr != nullptr )
		{
			LSourceFile	sfInitExpr = varDesc.m_pwszInitExpr ;
			std::shared_ptr<LArrayBufStorage>
						pBuf = pArrangeBuf->GetBuffer() ;
			LPtr<LPointerObj>
						pPtr = new LPointerObj
									( compiler.VM().GetPointerClass() ) ;
			pPtr->SetPointer( pBuf, desc.m_location, desc.m_size ) ;

			compiler.ParseDataInitExpression
				( sfInitExpr, desc.m_type, pPtr, &nsList ) ;
		}
		else if ( varDesc.m_pInitValue != nullptr )
		{
			std::shared_ptr<LArrayBufStorage>
						pBuf = pArrangeBuf->GetBuffer() ;
			LPtr<LPointerObj>
						pPtr = new LPointerObj
									( compiler.VM().GetPointerClass() ) ;
			pPtr->SetPointer( pBuf, desc.m_location, desc.m_size ) ;

			if ( typeVar.IsInteger() || typeVar.IsBoolean() )
			{
				pPtr->StoreIntegerAt
					( 0, typeVar.GetPrimitive(), varDesc.m_pInitValue->AsInteger() ) ;
			}
			else if ( typeVar.IsFloatingPointNumber() )
			{
				pPtr->StoreDoubleAt
					( 0, typeVar.GetPrimitive(), varDesc.m_pInitValue->AsDouble() ) ;
			}
		}
	}
	else
	{
		// オブジェクト変数
		LObjPtr	pObj ;
		if ( varDesc.m_pwszInitExpr != nullptr )
		{
			LSourceFile		sfInitExpr = varDesc.m_pwszInitExpr ;
			LExprValuePtr	xvalInit =
				compiler.EvaluateExpression( sfInitExpr, &nsList ) ;
			xvalInit = compiler.EvalCastTypeTo( std::move(xvalInit), typeVar ) ;

			if ( (xvalInit != nullptr)
				&& xvalInit->IsConstExpr() )
			{
				pObj = xvalInit->GetObject() ;
			}
			else
			{
				compiler.OnError( errorUnavailableConstExpr ) ;
			}
		}
		else if ( varDesc.m_pInitValue != nullptr )
		{
			if ( varDesc.m_pInitValue->GetType().IsObject() )
			{
				pObj = varDesc.m_pInitValue->GetObject() ;
			}
		}
		typeVar.SetModifiers( typeVar.GetModifiers() | accMod );

		if ( accMod & LType::modifierStatic )
		{
			DeclareObjectAs( varDesc.m_pwszName, typeVar, pObj ) ;
		}
		else if ( m_pPrototype != nullptr )
		{
			LObject::ReleaseRef
				( m_pPrototype->SetElementAs
						( varDesc.m_pwszName, pObj.Get() ) ) ;
			m_pPrototype->SetElementTypeAs( varDesc.m_pwszName, typeVar ) ;
		}
	}
}

// 二項演算子を追加
void LClass::AddBinaryOperatorDefinitions
	( const LClass::BinaryOperatorDesc * pboDescs, const LNamespaceList * pnslLocal )
{
	assert( GetClass() != nullptr ) ;

	LNamespaceList	nsList ;
	nsList.AddNamespace( this ) ;
	if ( pnslLocal != nullptr )
	{
		nsList.AddNamespaceList( *pnslLocal ) ;
	}

	LCompiler			compiler( GetClass()->VM() ) ;
	LCompiler::Current	current( compiler ) ;

	while ( pboDescs->m_pwszOperator != nullptr )
	{
		AddBinaryOperatorDefinition( *pboDescs, compiler, nsList ) ;
		pboDescs ++ ;
	}
}

void LClass::AddBinaryOperatorDefinition
	( const LClass::BinaryOperatorDesc& boDesc,
		LCompiler& compiler, const LNamespaceList& nsList )
{
	Symbol::BinaryOperatorDef	boDef ;
	boDef.m_opClass = Symbol::operatorBinary ;
	boDef.m_opIndex = compiler.AsOperatorIndex( boDesc.m_pwszOperator ) ;
	assert( boDef.m_opIndex != Symbol::opInvalid ) ;

	boDef.m_pwszComment = boDesc.m_pwszComment ;
	boDef.m_constExpr = boDesc.m_evalInConstExpr ;
	boDef.m_constThis = boDesc.m_constLeft ;

	LSourceFile		sfRetType = boDesc.m_pwszRetType ;
	LSourceFile		sfRightType = boDesc.m_pwszRightType ;

	LVerify( sfRetType.NextTypeExpr
				( boDef.m_typeRet, true, compiler.VM(), &nsList ) ) ;
	LVerify( sfRightType.NextTypeExpr
				( boDef.m_typeRight, true, compiler.VM(), &nsList ) ) ;

	boDef.m_pThisClass = this ;
	boDef.m_pfnOp = boDesc.m_pfnNativeFunc ;
	boDef.m_pInstance = nullptr ;
	boDef.m_pOpFunc = nullptr ;

	LType	typeAsRight = boDef.m_typeRight ;
	if ( boDesc.m_pwszAsRightType != nullptr )
	{
		LSourceFile	sfAsRightType = boDesc.m_pwszAsRightType ;
		LVerify( sfAsRightType.NextTypeExpr
					( typeAsRight, true, compiler.VM(), &nsList ) ) ;
	}

	AddBinaryOperatorDef( boDef, typeAsRight ) ;
}

// 適合単項演算子取得
const Symbol::UnaryOperatorDef *
	LClass::GetMatchUnaryOperator
		( Symbol::OperatorIndex opIndex, LType::AccessModifier accScope ) const
{
	return	FindMatchUnaryOperator
		( opIndex, m_vecUnaryOperators.data(), m_vecUnaryOperators.size(), accScope ) ;
}

const Symbol::UnaryOperatorDef *
		LClass::FindMatchUnaryOperator
			( Symbol::OperatorIndex opIndex,
				const Symbol::UnaryOperatorDef * pList, size_t nCount,
				LType::AccessModifier accScope )
{
	for ( size_t i = 0; i < nCount; i ++ )
	{
		if ( pList[i].m_opIndex == opIndex )
		{
			if ( (pList[i].m_pOpFunc == nullptr)
				|| pList[i].m_pOpFunc->IsEnableAccess( accScope ) )
			{
				return	pList + i ;
			}
		}
	}
	return	nullptr ;
}

// 適合二単項演算子取得
const Symbol::BinaryOperatorDef *
	LClass::GetMatchBinaryOperator
		( Symbol::OperatorIndex opIndex,
			const LType& typeVal2, LType::AccessModifier accScope ) const
{
	const Symbol::BinaryOperatorDef *
		pbopd = FindMatchBinaryOperator
					( opIndex, this, typeVal2,
						m_vecBinaryOperators.data(),
						m_vecBinaryOperators.size(), accScope ) ;
	if ( pbopd == nullptr )
	{
		pbopd = FindMatchBinaryOperator
					( opIndex, nullptr, typeVal2,
						m_vecBinaryOperators.data(),
						m_vecBinaryOperators.size(), accScope ) ;
	}
	return	pbopd ;
}

const Symbol::BinaryOperatorDef *
	LClass::FindMatchBinaryOperator
			( Symbol::OperatorIndex opIndex,
				const LClass * pThisClass, const LType& typeVal2,
				const Symbol::BinaryOperatorDef * pList, size_t nCount,
				LType::AccessModifier accScope )
{
	for ( size_t i = 0; i < nCount; i ++ )
	{
		if ( (pList[i].m_opIndex == opIndex)
			&& typeVal2.CanImplicitCastTo( pList[i].m_typeRight ) )
		{
			if ( (pList[i].m_pOpFunc == nullptr)
				|| pList[i].m_pOpFunc->IsEnableAccess( accScope ) )
			{
				if ( (pThisClass == nullptr)
					|| (pThisClass == pList[i].m_pThisClass) )
				{
					return	pList + i ;
				}
			}
		}
	}
	return	nullptr ;
}

// 単項演算子定義追加
void LClass::AddUnaryOperatorDef
	( Symbol::OperatorIndex opIndex, const LType& typeRet,
		Symbol::PFN_OPERATOR1 pfnOp, void * pInstance,
		bool constExpr, bool constThis, const wchar_t * pwszComment )
{
	Symbol::UnaryOperatorDef	uop ;
	uop.m_opClass = Symbol::operatorUnary ;
	uop.m_opIndex = opIndex ;
	uop.m_pwszComment = pwszComment ;
	uop.m_constExpr = constExpr ;
	uop.m_constThis = constThis ;
	uop.m_typeRet = typeRet ;
	uop.m_pThisClass = this ;
	uop.m_pfnOp = pfnOp ;
	uop.m_pInstance = pInstance ;
	uop.m_pOpFunc = nullptr ;

	AddUnaryOperatorDef( uop ) ;
}

void LClass::AddUnaryOperatorDef
	( const Symbol::UnaryOperatorDef & uopDef )
{
	for ( size_t i = 0; i < m_vecUnaryOperators.size(); i ++ )
	{
		if ( m_vecUnaryOperators.at(i).m_opIndex == uopDef.m_opIndex )
		{
			m_vecUnaryOperators.erase( m_vecUnaryOperators.begin() + i ) ;
			break ;
		}
	}
	m_vecUnaryOperators.push_back( uopDef ) ;
}

void LClass::AddUnaryOperatorDefList
	( const Symbol::UnaryOperatorDef * pList, size_t nCount )
{
	m_vecUnaryOperators.reserve( m_vecUnaryOperators.size() + nCount ) ;

	for ( size_t i = 0; i < nCount; i ++ )
	{
		if ( pList[i].m_opIndex != Symbol::opInvalid )
		{
			break ;
		}
		Symbol::UnaryOperatorDef	uopd = pList[i] ;
		if ( uopd.m_pThisClass == nullptr )
		{
			uopd.m_pThisClass = this ;
		}
		AddUnaryOperatorDef( uopd ) ;
	}
}

// 二項演算子定義追加
void LClass::AddBinaryOperatorDef
	( Symbol::OperatorIndex opIndex, const LType& typeRet,
		const LType& typeRight,
		Symbol::PFN_OPERATOR2 pfnOp, void * pInstance,
		bool constExpr, bool constThis, const wchar_t * pwszComment )
{
	Symbol::BinaryOperatorDef	bop ;
	bop.m_opClass = Symbol::operatorBinary ;
	bop.m_opIndex = opIndex ;
	bop.m_pwszComment = pwszComment ;
	bop.m_constExpr = constExpr ;
	bop.m_constThis = constThis ;
	bop.m_typeRet = typeRet ;
	bop.m_pThisClass = this ;
	bop.m_pfnOp = pfnOp ;
	bop.m_typeRight = typeRight ;
	bop.m_pInstance = pInstance ;
	bop.m_pOpFunc = nullptr ;

	AddBinaryOperatorDef( bop, bop.m_typeRight ) ;
}

void LClass::AddBinaryOperatorDef
	( const Symbol::BinaryOperatorDef & bopDef, const LType& typeAsRight )
{
	for ( size_t i = 0; i < m_vecBinaryOperators.size(); i ++ )
	{
		Symbol::BinaryOperatorDef&	bopd = m_vecBinaryOperators.at(i) ;
		if ( bopd.m_opIndex == bopDef.m_opIndex )
		{
			if ( bopd.m_typeRight == typeAsRight )
			{
				// オーバーライド
				m_vecBinaryOperators.erase( m_vecBinaryOperators.begin() + i ) ;
				break ;
			}
			if ( bopd.m_pThisClass != bopDef.m_pThisClass )
			{
				// 異なるクラス（派生クラス）では派生したクラスを優先する
				// （※同じクラス内では記述した順序が優先される）
				m_vecBinaryOperators.insert
					( m_vecBinaryOperators.begin() + i, bopDef ) ;
				return ;
			}
		}
	}
	m_vecBinaryOperators.push_back( bopDef ) ;
}

void LClass::AddBinaryOperatorDefList
	( const Symbol::BinaryOperatorDef * pList, size_t nCount )
{
	m_vecBinaryOperators.reserve( m_vecBinaryOperators.size() + nCount ) ;

	for ( size_t i = 0; i < nCount; i ++ )
	{
		if ( pList[i].m_opIndex != Symbol::opInvalid )
		{
			break ;
		}
		Symbol::BinaryOperatorDef	bopd = pList[i] ;
		if ( bopd.m_pThisClass == nullptr )
		{
			bopd.m_pThisClass = this ;
		}
		m_vecBinaryOperators.push_back( bopd ) ;
	}
}

// クラスメンバ一括定義
void LClass::AddClassMemberDefinitions
	( const LClass::ClassMemberDesc& desc, const LNamespaceList * pnslLocal )
{
	assert( GetClass() != nullptr ) ;

	LNamespaceList	nsList ;
	nsList.AddNamespace( this ) ;
	if ( pnslLocal != nullptr )
	{
		nsList.AddNamespaceList( *pnslLocal ) ;
	}

	LCompiler			compiler( GetClass()->VM() ) ;
	LCompiler::Current	current( compiler ) ;

	if ( desc.m_pwszComment != nullptr )
	{
		SetSelfComment( desc.m_pwszComment ) ;
	}

	// メンバ変数
	if ( desc.m_pvdVariables != nullptr )
	{
		const VariableDesc *	pvDescs = desc.m_pvdVariables ;
		while ( pvDescs->m_pwszName != nullptr )
		{
			AddVariableDefinition( *pvDescs, compiler, nsList ) ;
			pvDescs ++ ;
		}
	}

	// 仮想関数
	if ( desc.m_pnfdVirtuals != nullptr )
	{
		const NativeFuncDesc *	pnfDescs = desc.m_pnfdVirtuals ;
		while ( pnfDescs->m_pwszFuncName != nullptr )
		{
			std::shared_ptr<LPrototype>	pAsProto =
					GetOverridePrototype( *pnfDescs, compiler, nsList ) ;
			m_vfvVirtualFuncs.OverrideFunction
				( pnfDescs->m_pwszFuncName,
					CreateFunctionObj( *pnfDescs, compiler, nsList ), pAsProto.get() ) ;
			pnfDescs ++ ;
		}
	}

	// 静関数群
	if ( desc.m_pnfdFunctions != nullptr )
	{
		const NativeFuncDesc *	pnfDescs = desc.m_pnfdFunctions ;
		while ( pnfDescs->m_pwszFuncName != nullptr )
		{
			AddStaticFunctionAs
				( pnfDescs->m_pwszFuncName,
					CreateFunctionObj( *pnfDescs, compiler, nsList ) ) ;
			pnfDescs ++ ;
		}
	}

	// 二項演算子
	if ( desc.m_pbodBinaryOps != nullptr )
	{
		const BinaryOperatorDesc *	pboDescs = desc.m_pbodBinaryOps ;
		while ( pboDescs->m_pwszOperator != nullptr )
		{
			AddBinaryOperatorDefinition( *pboDescs, compiler, nsList ) ;
			pboDescs ++ ;
		}
	}
}


// instanceof 演算子 : boolean(LObject*,LClass*)
LValue::Primitive LClass::operator_instanceof
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LObject *	pObj1 = val1.pObject ;
	LObject *	pObj2 = val2.pObject ;
	if ( (pObj1 == nullptr)
		|| (pObj1->GetClass() == nullptr)
		|| (pObj2 == nullptr) )
	{
		return	LValue::MakeBool( false ) ;
	}
	assert( dynamic_cast<LClass*>( pObj2 ) != nullptr ) ;
	return	LValue::MakeBool
				( pObj1->GetClass()->
					IsInstanceOf( dynamic_cast<LClass*>( pObj2 ) ) ) ;
}

// == 演算子 : boolean(LObject*,LObject*)
LValue::Primitive LClass::operator_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.pObject == val2.pObject ) ;
}

// != 演算子 : boolean(LObject*,LObject*)
LValue::Primitive LClass::operator_not_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.pObject != val2.pObject ) ;
}

// sizeof 演算子関数 : long(LObject*)
LValue::Primitive LClass::operator_sizeof
	( LValue::Primitive val1, void * instance )
{
	if ( val1.pObject == nullptr )
	{
		return	LValue::MakeLong(0) ;
	}
	return	LValue::MakeLong( val1.pObject->GetElementCount() ) ;
}


//////////////////////////////////////////////////////////////////////////
// クラス一括実装ヘルパー
//////////////////////////////////////////////////////////////////////////

LBatchClassImplementor::LBatchClassImplementor( LNamespace& nsTarget )
	: m_target( nsTarget )
{
}

// クラス追加
void LBatchClassImplementor::AddClass
		( LPtr<LClass> pClass, LClass * pSuperClass )
{
	m_target.AddClassAs( pClass->GetClassName().c_str(), pClass ) ;
	DeclareClass( pClass.Ptr(), pSuperClass ) ;
}

// 実装のために追加
void LBatchClassImplementor::DeclareClass
		( LClass * pClass, LClass * pSuperClass )
{
	pClass->AddSuperClass( pSuperClass ) ;
	m_classes.push_back( pClass ) ;
}

// 一括実装
void LBatchClassImplementor::Implement( void )
{
	for ( LClass * pClass : m_classes )
	{
		pClass->ImplementClass() ;
		pClass->CompleteClass() ;
	}
	m_classes.clear() ;
}



//////////////////////////////////////////////////////////////////////////////
// Class クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LClassClass::CloneObject( void ) const
{
	return	new LClassClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LClassClass::DuplicateObject( void ) const
{
	return	new LClassClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LClassClass::ImplementClass( void )
{
	OverrideVirtuals( s_Virtuals ) ;
}

const LClass::NativeFuncDesc	LClassClass::s_Virtuals[2] =
{
	{	// public String getName()
		L"getName",
		&LClassClass::method_getName, true,
		L"public", L"String", L"",
		L"クラス名を取得します。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

// String getName()
void LClassClass::method_getName( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LClass, pObj ) ;

	LQT_RETURN_STRING( pObj->GetClassName() ) ;
}



//////////////////////////////////////////////////////////////////////////////
// プリミティブ／構造体配列型
//////////////////////////////////////////////////////////////////////////////

LDataArrayClass::LDataArrayClass
	( LVirtualMachine& vm,
		LClass * pClass, const wchar_t * pwszName,
		const LType& typeData, const std::vector<size_t>& dimArray )
	: LClass( vm, vm.Global(), pClass, pwszName ),
		m_typeData( typeData.GetBaseDataType() ),
		m_dimArray( dimArray ),
		m_pGenPointerType( nullptr )
{
	assert( typeData.IsVoid() || typeData.CanArrangeOnBuf() ) ;
	LDataArrayClass *	pArrayType = typeData.GetDataArrayClass() ;
	if ( pArrayType != nullptr )
	{
		for ( size_t i = 0; i < pArrayType->m_dimArray.size(); i ++ )
		{
			m_dimArray.push_back( pArrayType->m_dimArray.at(i) ) ;
		}
	}
}

LDataArrayClass::LDataArrayClass( const LDataArrayClass& cls )
	: LClass( cls ),
		m_typeData( cls.m_typeData ),
		m_dimArray( cls.m_dimArray ),
		m_pGenPointerType( cls.m_pGenPointerType )
{
}

// 配列装飾されていない型
const LType& LDataArrayClass::GetDataType( void ) const
{
	return	m_typeData ;
}

// １回要素参照した型
LType LDataArrayClass::GetElementType( void ) const
{
	if ( m_dimArray.size() <= 1 )
	{
		return	m_typeData ;
	}
	std::vector<size_t>	dimElement = m_dimArray ;
	dimElement.erase( dimElement.begin() ) ;
	return	LType( m_vm.GetDataArrayClassAs( m_typeData, dimElement ) ) ;
}

// 要素を参照するポインタ型
LClass * LDataArrayClass::GetElementPtrClass( void ) const
{
	return	m_vm.GetPointerClassAs( GetElementType() ) ;
}

// ストライド
size_t LDataArrayClass::GetDataSize( void ) const
{
	size_t	nBytes = m_typeData.GetDataBytes() ;
	for ( size_t i = 0; i < m_dimArray.size(); i ++ )
	{
		nBytes *= m_dimArray.at(i) ;
	}
	return	nBytes ;
}

// アライメント
size_t LDataArrayClass::GetDataAlign( void ) const
{
	return	m_typeData.GetAlignBytes() ;
}

// 配列情報
const std::vector<size_t>& LDataArrayClass::Dimension( void ) const
{
	return	m_dimArray ;
}

// （１回要素参照する際の）配列長
size_t LDataArrayClass::GetArrayElementCount( void ) const
{
	if ( m_dimArray.size() == 0 )
	{
		return	0 ;
	}
	return	m_dimArray.at(0) ;
}

// 複製する（要素も全て複製処理する）
LObject * LDataArrayClass::CloneObject( void ) const
{
	return	new LDataArrayClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LDataArrayClass::DuplicateObject( void ) const
{
	return	new LDataArrayClass( *this ) ;
}

// pClass へキャスト可能か？（データの変換なしのキャスト）
//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
LClass::ResultInstanceOf LDataArrayClass::TestInstanceOf( LClass * pClass ) const
{
	if ( pClass == this )
	{
		return	instanceAvailable ;
	}
	return	instanceUnavailable ;
}



//////////////////////////////////////////////////////////////////////////////
// Pointer クラス
//////////////////////////////////////////////////////////////////////////////

// ポインタ装飾されていない型
const LType& LPointerClass::GetBufferType( void ) const
{
	return	m_typeBuf ;
}

// ポインタ装飾も配列装飾もされていない型
LType LPointerClass::GetDataType( void ) const
{
	LDataArrayClass *	pArrayClass = m_typeBuf.GetDataArrayClass() ;
	if ( pArrayClass != nullptr )
	{
		return	pArrayClass->GetDataType() ;
	}
	return	m_typeBuf ;
}

// 要素ストライド
size_t LPointerClass::GetElementStride( void ) const
{
	return	m_typeBuf.GetDataBytes() ;
}

// 要素アライメント
size_t LPointerClass::GetElementAlign( void ) const
{
	return	m_typeBuf.GetAlignBytes() ;
}

// 複製する（要素も全て複製処理する）
LObject * LPointerClass::CloneObject( void ) const
{
	return	new LPointerClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LPointerClass::DuplicateObject( void ) const
{
	return	new LPointerClass( *this ) ;
}

// pClass へキャスト可能か？（データの変換なしのキャスト）
//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
LClass::ResultInstanceOf LPointerClass::TestInstanceOf( LClass * pClass ) const
{
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( pClass ) ;
	if ( pPtrClass == nullptr )
	{
		return	instanceUnavailable ;
	}
	bool	flagCastable = false ;
	if ( pPtrClass->m_typeBuf.IsVoid() )
	{
		flagCastable = true ;
	}
	else if ( m_typeBuf.IsPrimitive() )
	{
		flagCastable = pPtrClass->m_typeBuf.IsPrimitive()
					&& (m_typeBuf.GetPrimitive()
							== pPtrClass->m_typeBuf.GetPrimitive()) ;
	}
	else if ( m_typeBuf.IsDataArray() )
	{
		flagCastable = pPtrClass->m_typeBuf.IsDataArray()
					&& (m_typeBuf.GetClass() == pPtrClass->m_typeBuf.GetClass()) ;
		if ( !flagCastable && !pPtrClass->m_typeBuf.IsDataArray() )
		{
			flagCastable =
				(m_typeBuf.GetPrimitive() == pPtrClass->m_typeBuf.GetPrimitive())
					&& (m_typeBuf.GetClass() == pPtrClass->m_typeBuf.GetClass()) ;
		}
	}
	else
	{
		LStructureClass *	pSrcStruct = m_typeBuf.GetStructureClass() ;
		LStructureClass *	pDstStruct = pPtrClass->m_typeBuf.GetStructureClass() ;
		if ( (pSrcStruct != nullptr) && (pDstStruct != nullptr) )
		{
			flagCastable =
				(pSrcStruct->TestInstanceOf( pDstStruct ) == instanceAvailable) ;
		}
	}
	if ( flagCastable )
	{
		if ( !m_typeBuf.IsConst()
			|| pPtrClass->m_typeBuf.IsConst() )
		{
			return	instanceAvailable ;
		}
		else
		{
			return	instanceConstCast ;
		}
	}
	return	LClass::TestInstanceOf( pClass ) ;
}

// クラス定義処理（ネイティブな実装）
void LPointerClass::ImplementClass( void )
{
	OverrideVirtuals( s_Virtuals ) ;
}

const LClass::NativeFuncDesc	LPointerClass::s_Virtuals[3] =
{
	{	// public void copy( const void* pSrc, long bytes ) const
		L"copy",
		&LPointerObj::method_copy, false,
		L"public const", L"void", L"const void* pSrc, long bytes",
		L"メモリの内容を複製します。\n"
		L"<param name=\"pSrc\">複製元のポインタ</param>\n"
		L"<param name=\"bytes\">複製するバイト数</param>", nullptr
	},
	{	// public void copyTo( Pointer pDst, long bytes ) const
		L"copyTo",
		&LPointerObj::method_copyTo, false,
		L"public const", L"void", L"Pointer pDst, long bytes",
		L"メモリの内容を複製します。\n"
		L"<param name=\"pDst\">複製先のポインタ</param>\n"
		L"<param name=\"bytes\">複製するバイト数</param>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

// 演算子
const Symbol::UnaryOperatorDef
	LPointerClass::s_UnaryOffsetOperatorDefs[LPointerClass::UnaryOffsetOperatorCount+1] =
{
	{
		Symbol::operatorUnary,
		Symbol::opIncrement,
		nullptr, true, false,
		LType(LType::typeInt64), nullptr,
		&LPointerClass::operator_offset_increment, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opDecrement,
		nullptr, true, false,
		LType(LType::typeInt64), nullptr,
		&LPointerClass::operator_offset_decrement, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opInvalid,
	},
} ;

const Symbol::UnaryOperatorDef
	LPointerClass::s_UnaryPtrOperatorDefs[UnaryPtrOperatorCount+1] =
{
	{
		Symbol::operatorUnary,
		Symbol::opIncrement,
		nullptr, true, true,
		LType(LType::typePointer), nullptr,
		&LPointerClass::operator_ptr_increment, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opDecrement,
		nullptr, true, true,
		LType(LType::typePointer), nullptr,
		&LPointerClass::operator_ptr_decrement, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opInvalid,
	},
} ;

const Symbol::BinaryOperatorDef
	LPointerClass::s_BinaryOffsetOperatorDefs[BinaryOffsetOperatorCount+1] =
{
	{
		Symbol::operatorBinary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LPointerClass::operator_offset_add,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LPointerClass::operator_offset_sub,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_offset_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_offset_not_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqualPtr,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_offset_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqualPtr,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_offset_not_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opInvalid,
	},
} ;

const Symbol::BinaryOperatorDef
	LPointerClass::s_BinaryPtrOperatorDefs[BinaryPtrOperatorCount+1] =
{
	{
		Symbol::operatorBinary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typePointer), nullptr,
		&LPointerClass::operator_ptr_add,
		LType(LType::typeInt64), nullptr, nullptr
	},
	{
		Symbol::operatorBinary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typePointer), nullptr,
		&LPointerClass::operator_ptr_sub,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_ptr_equal,
		LType(LType::typePointer), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_ptr_not_equal,
		LType(LType::typePointer), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqualPtr,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_ptr_equal,
		LType(LType::typePointer), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqualPtr,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LPointerClass::comparator_ptr_not_equal,
		LType(LType::typePointer), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opInvalid,
	},
} ;

// 単項演算子（オフセット/アドレスに対する整数値の操作）
// （※instance はストライド整数値）
LValue::Primitive LPointerClass::operator_offset_increment
	( LValue::Primitive val1, void * instance )
{
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	val1.longValue += prm.longValue ;

	return	val1 ;
}

LValue::Primitive LPointerClass::operator_offset_decrement
	( LValue::Primitive val1, void * instance )
{
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	val1.longValue -= prm.longValue ;

	return	val1 ;
}

// 単項演算子（ポインタ・オブジェクトに対する操作）
// （※instance はストライド整数値）
LValue::Primitive LPointerClass::operator_ptr_increment
	( LValue::Primitive val1, void * instance )
{
	if ( val1.pObject == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LPointerObj *	pPtr = dynamic_cast<LPointerObj*>( val1.pObject ) ;
	if ( pPtr == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	pPtr = new LPointerObj( *pPtr ) ;
	*pPtr += (ssize_t) prm.longValue ;

	return	LValue::MakeObjectPtr( pPtr ) ;
}

LValue::Primitive LPointerClass::operator_ptr_decrement
	( LValue::Primitive val1, void * instance )
{
	if ( val1.pObject == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LPointerObj *	pPtr = dynamic_cast<LPointerObj*>( val1.pObject ) ;
	if ( pPtr == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	pPtr = new LPointerObj( *pPtr ) ;
	*pPtr -= (ssize_t) prm.longValue ;

	return	LValue::MakeObjectPtr( pPtr ) ;
}

// 二項演算子（オフセット/アドレスに対する整数値の操作）
// （※instance はストライド整数値）
LValue::Primitive LPointerClass::operator_offset_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	val1.longValue += val2.longValue * prm.longValue ;

	return	val1 ;
}

LValue::Primitive LPointerClass::operator_offset_sub
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	val1.longValue -= val2.longValue * prm.longValue ;

	return	val1 ;
}

LValue::Primitive LPointerClass::comparator_offset_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue == val2.longValue ) ;
}

LValue::Primitive LPointerClass::comparator_offset_not_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue != val2.longValue ) ;
}

// 二項演算子（ポインタ・オブジェクトに対する操作）
// （※instance はストライド整数値）
LValue::Primitive LPointerClass::operator_ptr_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	if ( val1.pObject == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LPointerObj *	pPtr = dynamic_cast<LPointerObj*>( val1.pObject ) ;
	if ( pPtr == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	pPtr = new LPointerObj( *pPtr ) ;
	*pPtr += (ssize_t) (val2.longValue * prm.longValue) ;

	return	LValue::MakeObjectPtr( pPtr ) ;
}

LValue::Primitive LPointerClass::operator_ptr_sub
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	if ( val1.pObject == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LPointerObj *	pPtr = dynamic_cast<LPointerObj*>( val1.pObject ) ;
	if ( pPtr == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LValue::Primitive	prm ;
	prm.pVoidPtr = instance ;

	pPtr = new LPointerObj( *pPtr ) ;
	*pPtr -= (ssize_t) (val2.longValue * prm.longValue) ;

	return	LValue::MakeObjectPtr( pPtr ) ;
}

LValue::Primitive LPointerClass::comparator_ptr_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	std::uint8_t *	p1 = nullptr ;
	std::uint8_t *	p2 = nullptr ;
	if ( val1.pObject != nullptr )
	{
		LPointerObj *	pPtr1 = dynamic_cast<LPointerObj*>( val1.pObject ) ;
		if ( pPtr1 != nullptr )
		{
			p1 = pPtr1->GetPointer() ;
		}
	}
	if ( val2.pObject != nullptr )
	{
		LPointerObj *	pPtr2 = dynamic_cast<LPointerObj*>( val2.pObject ) ;
		if ( pPtr2 != nullptr )
		{
			p2 = pPtr2->GetPointer() ;
		}
	}
	return	LValue::MakeBool( p1 == p2 ) ;
}

LValue::Primitive LPointerClass::comparator_ptr_not_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	std::uint8_t *	p1 = nullptr ;
	std::uint8_t *	p2 = nullptr ;
	if ( val1.pObject != nullptr )
	{
		LPointerObj *	pPtr1 = dynamic_cast<LPointerObj*>( val1.pObject ) ;
		if ( pPtr1 != nullptr )
		{
			p1 = pPtr1->GetPointer() ;
		}
	}
	if ( val2.pObject != nullptr )
	{
		LPointerObj *	pPtr2 = dynamic_cast<LPointerObj*>( val2.pObject ) ;
		if ( pPtr2 != nullptr )
		{
			p2 = pPtr2->GetPointer() ;
		}
	}
	return	LValue::MakeBool( p1 != p2 ) ;
}



//////////////////////////////////////////////////////////////////////////////
// Integer クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LIntegerClass::CloneObject( void ) const
{
	return	new LIntegerClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LIntegerClass::DuplicateObject( void ) const
{
	return	new LIntegerClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LIntegerClass::ImplementClass( void )
{
	OverrideVirtuals( s_Virtuals ) ;
}

const LClass::NativeFuncDesc	LIntegerClass::s_Virtuals[2] =
{
	{	// public Integer( long val )
		LClass::s_Constructor,
		&LIntegerObj::method_init, true,
		L"public", L"void", L"long val",
		L"Integer オブジェクトを構築します。\n"
		L"<param name=\"val\">初期値</param>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

// long プリミティブ用演算子
const Symbol::UnaryOperatorDef
	LIntegerClass::s_UnaryOperatorDefs[LIntegerClass::UnaryOperatorCount+1] =
{
	{
		Symbol::operatorUnary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_unary_plus, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_unary_minus, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opIncrement,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_increment, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opDecrement,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_decrement, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opBitNot,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_not, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opLogicalNot,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::operator_logical_not, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opInvalid,
	},
} ;

const Symbol::BinaryOperatorDef
	LIntegerClass::s_IntOperatorDefs[LIntegerClass::BinaryOperatorCount+1] =
{
	{
		Symbol::operatorBinary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_add,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_sub,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opMul,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_mul,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opDiv,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_div,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opMod,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_mod,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opBitAnd,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_and,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opBitOr,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_or,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opBitXor,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_xor,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opShiftRight,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_shift_right_a,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opShiftLeft,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_shift_left,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_not_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLessEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_less_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLessThan,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_less_than,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opGraterEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_grater_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opGraterThan,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_grater_than,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLogicalAnd,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_logical_and,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLogicalOr,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_logical_or,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opInvalid,
	},
} ;

const Symbol::BinaryOperatorDef
	LIntegerClass::s_UintOperatorDefs[LIntegerClass::BinaryOperatorCount+1] =
{
	{
		Symbol::operatorBinary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_add,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_sub,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opMul,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_mul,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opDiv,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_div,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opMod,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_mod,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opBitAnd,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_and,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opBitOr,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_or,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opBitXor,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_bit_xor,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opShiftRight,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_shift_right,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opShiftLeft,
		nullptr, true, true,
		LType(LType::typeInt64), nullptr,
		&LIntegerClass::operator_shift_left,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_not_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLessEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_less_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLessThan,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_less_than,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opGraterEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_grater_equal,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opGraterThan,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_grater_than,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLogicalAnd,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_logical_and,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLogicalOr,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LIntegerClass::comparator_logical_or,
		LType(LType::typeInt64), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opInvalid,
	},
} ;

// long プリミティブ用単項演算子
LValue::Primitive LIntegerClass::operator_unary_plus
	( LValue::Primitive val1, void * instance )
{
	return	val1 ;
}

LValue::Primitive LIntegerClass::operator_unary_minus
	( LValue::Primitive val1, void * instance )
{
	return	LValue::MakeLong( - val1.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_increment
	( LValue::Primitive val1, void * instance )
{
	return	LValue::MakeLong( val1.longValue + 1 ) ;
}

LValue::Primitive LIntegerClass::operator_decrement
	( LValue::Primitive val1, void * instance )
{
	return	LValue::MakeLong( val1.longValue - 1 ) ;
}

LValue::Primitive LIntegerClass::operator_bit_not
	( LValue::Primitive val1, void * instance )
{
	return	LValue::MakeLong( ~val1.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_logical_not
	( LValue::Primitive val1, void * instance )
{
	return	LValue::MakeBool( !val1.longValue ) ;
}

// long プリミティブ用二項演算子
LValue::Primitive LIntegerClass::operator_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue + val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_sub
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue - val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_mul
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue * val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_div
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	if ( val2.longValue == 0 )
	{
		LContext::ThrowExceptionError( exceptionZeroDivision ) ;
		return	LValue::MakeLong( 0 ) ;
	}
	return	LValue::MakeLong( val1.longValue / val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_mod
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	if ( val2.longValue == 0 )
	{
		LContext::ThrowExceptionError( exceptionZeroDivision ) ;
		return	LValue::MakeLong( 0 ) ;
	}
	return	LValue::MakeLong( val1.longValue % val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_bit_and
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue & val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_bit_or
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue | val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_bit_xor
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue ^ val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_shift_right
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( (std::uint64_t) val1.longValue >> val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_shift_right_a
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue >> val2.longValue ) ;
}

LValue::Primitive LIntegerClass::operator_shift_left
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeLong( val1.longValue << val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue == val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_not_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue != val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_less_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue <= val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_less_than
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue < val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_grater_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue >= val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_grater_than
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue > val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_logical_and
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue && val2.longValue ) ;
}

LValue::Primitive LIntegerClass::comparator_logical_or
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.longValue || val2.longValue ) ;
}



//////////////////////////////////////////////////////////////////////////
// Double クラス
//////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LDoubleClass::CloneObject( void ) const
{
	return	new LDoubleClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LDoubleClass::DuplicateObject( void ) const
{
	return	new LDoubleClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LDoubleClass::ImplementClass( void )
{
	OverrideVirtuals( s_Virtuals ) ;
}

const LClass::NativeFuncDesc	LDoubleClass::s_Virtuals[2] =
{
	{	// public Double( double val )
		LClass::s_Constructor,
		&LDoubleObj::method_init, true,
		L"public", L"void", L"double val",
		L"Double オブジェクト構築します。\n"
		L"<param name=\"val\">初期値</param>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

// double プリミティブ用演算子
const Symbol::UnaryOperatorDef
	LDoubleClass::s_UnaryOperatorDefs[LDoubleClass::UnaryOperatorCount+1] =
{
	{
		Symbol::operatorUnary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typeDouble), nullptr,
		&LDoubleClass::operator_unary_plus, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typeDouble), nullptr,
		&LDoubleClass::operator_unary_minus, nullptr, nullptr,
	},
	{
		Symbol::operatorUnary,
		Symbol::opInvalid,
	},
} ;

const Symbol::BinaryOperatorDef
	LDoubleClass::s_BinaryOperatorDefs[LDoubleClass::BinaryOperatorCount+1] =
{
	{
		Symbol::operatorBinary,
		Symbol::opAdd,
		nullptr, true, true,
		LType(LType::typeDouble), nullptr,
		&LDoubleClass::operator_add,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opSub,
		nullptr, true, true,
		LType(LType::typeDouble), nullptr,
		&LDoubleClass::operator_sub,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opMul,
		nullptr, true, true,
		LType(LType::typeDouble), nullptr,
		&LDoubleClass::operator_mul,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opDiv,
		nullptr, true, true,
		LType(LType::typeDouble), nullptr,
		&LDoubleClass::operator_div,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LDoubleClass::comparator_equal,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opNotEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LDoubleClass::comparator_not_equal,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLessEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LDoubleClass::comparator_less_equal,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opLessThan,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LDoubleClass::comparator_less_than,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opGraterEqual,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LDoubleClass::comparator_grater_equal,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opGraterThan,
		nullptr, true, true,
		LType(LType::typeBoolean), nullptr,
		&LDoubleClass::comparator_grater_than,
		LType(LType::typeDouble), nullptr, nullptr,
	},
	{
		Symbol::operatorBinary,
		Symbol::opInvalid,
	},
} ;

// double プリミティブ用単項演算子
LValue::Primitive LDoubleClass::operator_unary_plus
	( LValue::Primitive val1, void * instance )
{
	return	val1 ;
}

LValue::Primitive LDoubleClass::operator_unary_minus
	( LValue::Primitive val1, void * instance )
{
	return	LValue::MakeDouble( - val1.dblValue ) ;
}

// double プリミティブ用二項演算子
LValue::Primitive LDoubleClass::operator_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeDouble( val1.dblValue + val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::operator_sub
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeDouble( val1.dblValue - val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::operator_mul
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeDouble( val1.dblValue * val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::operator_div
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeDouble( val1.dblValue / val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::comparator_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.dblValue == val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::comparator_not_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.dblValue != val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::comparator_less_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.dblValue <= val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::comparator_less_than
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.dblValue < val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::comparator_grater_equal
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.dblValue >= val2.dblValue ) ;
}

LValue::Primitive LDoubleClass::comparator_grater_than
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.dblValue > val2.dblValue ) ;
}



//////////////////////////////////////////////////////////////////////////////
// String クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LStringClass::CloneObject( void ) const
{
	return	new LStringClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LStringClass::DuplicateObject( void ) const
{
	return	new LStringClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LStringClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LStringClass::s_Virtuals[22] =
{
	{	// public String( String str )
		LClass::s_Constructor,
		&LStringObj::method_init, true,
		L"public", L"void", L"String str",
		L"文字列を複製して String オブジェクトを構築します。\n"
		L"<param name=\"str\">複製元の文字列</param>", nullptr
	},
	{	// public String( uint8* utf8, long length = -1 )
		LClass::s_Constructor,
		&LStringObj::method_init_utf8, true,
		L"public", L"void", L"uint8* utf8, long length = -1",
		L"utf-8 文字列から String オブジェクトを構築します。\n"
		L"<param name=\"utf8\">utf-8 文字列へのポインタ</param>\n"
		L"<param name=\"length\">utf-8 文字列への長さ。"
		L"マイナス値の場合には utf8 のバッファ終端、又は 0 要素まで</param>", nullptr
	},
	{	// public String( uint16* utf16, long length = -1 )
		LClass::s_Constructor,
		&LStringObj::method_init_utf16, true,
		L"public", L"void", L"uint16* utf16, long length = -1",
		L"utf-16 文字列から String オブジェクトを構築します。\n"
		L"<param name=\"utf16\">utf-16 文字列へのポインタ</param>\n"
		L"<param name=\"length\">utf-16 文字列への長さ。"
		L"マイナス値の場合には utf16 のバッファ終端、又は 0 要素まで</param>", nullptr
	},
	{	// public String( uint32* utf32, long length = -1 )
		LClass::s_Constructor,
		&LStringObj::method_init_utf32, true,
		L"public", L"void", L"uint32* utf32, long length = -1",
		L"utf-32 文字列から String オブジェクトを構築します。\n"
		L"<param name=\"utf32\">utf-32 文字列へのポインタ</param>\n"
		L"<param name=\"length\">utf-32 文字列への長さ。"
		L"マイナス値の場合には utf32 のバッファ終端、又は 0 要素まで</param>", nullptr
	},
	{	// public uint length() const
		L"length",
		&LStringObj::method_length, true,
		L"public const", L"uint", L"",
		L"文字列長を取得します。\n"
		L"文字列長は実際の文字数とは限りません。", nullptr
	},
	{	// public String left( long count ) const
		L"left",
		&LStringObj::method_left, true,
		L"public const", L"String", L"long count",
		L"文字列の左側を取得します。\n"
		L"<param name=\"count\">取り出す文字列長。大きすぎる場合にはクリップされます。</param>", nullptr
	},
	{	// public String right( long count ) const
		L"right",
		&LStringObj::method_right, true,
		L"public const", L"String", L"long count",
		L"文字列の右側を取得します。\n"
		L"<param name=\"count\">取り出す文字列長。大きすぎる場合にはクリップされます。</param>", nullptr
	},
	{	// public String middle( long first, long count = -1 ) const
		L"middle",
		&LStringObj::method_middle, true,
		L"public const", L"String", L"long first, long count = -1",
		L"文字列の任意の部分を取得します。\n"
		L"<param name=\"first\">文字列を取り出す初めの位置</param>\n"
		L"<param name=\"count\">取り出す文字列長。大きすぎる場合にはクリップされます。<br/>\n"
		L"マイナス値の場合には first から終端までを取り出します。</param>", nullptr
	},
	{	// public int find( String str, long first = 0 ) const
		L"find",
		&LStringObj::method_find, true,
		L"public const", L"int", L"String str, int first = 0",
		L"文字列中の str に一致する先頭位置を探します。\n"
		L"<param name=\"str\">一致位置を検索する文字列</param>\n"
		L"<param name=\"first\">検索の開始位置</param>\n"
		L"<return>str に一致する先頭位置。一致箇所が見つからなかった場合には -1</return>", nullptr
	},
	{	// public String upper() const
		L"upper",
		&LStringObj::method_upper, true,
		L"public const", L"String", L"",
		L"半角小文字アルファベットを半角大文字に変換した文字列を取得します。", nullptr
	},
	{	// public String lower() const
		L"lower",
		&LStringObj::method_lower, true,
		L"public const", L"String", L"",
		L"半角大文字アルファベットを半角小文字に変換した文字列を取得します。", nullptr
	},
	{	// public String trim() const
		L"trim",
		&LStringObj::method_trim, true,
		L"public const", L"String", L"",
		L"文字列の先頭と末尾の空白、タブ、改行等を除去した文字列を取得します。", nullptr
	},
	{	// public String chop( long count ) const
		L"chop",
		&LStringObj::method_chop, true,
		L"public const", L"String", L"long count",
		L"文字列の末尾を除去した文字列を取得します。\n"
		L"<param name=\"count\">除去する文字列長。大きすぎる場合にはクリップされます。</param>", nullptr
	},
	{	// public uint charAt( long index ) const
		L"charAt",
		&LStringObj::method_charAt, true,
		L"public const", L"uint", L"long index",
		L"<summary>文字列中の指定位置の文字コードを取得します。<br/>\n"
		L"文字コードは utf-16 か utf-32 で、何れであるかは環境に依存します。</summary>\n"
		L"<param name=\"index\">取得する文字指標</param>\n"
		L"<return>index 位置の文字コード。index が範囲外の場合には 0</return>", nullptr
	},
	{	// public uint backAt( long index ) const
		L"backAt",
		&LStringObj::method_backAt, true,
		L"public const", L"uint", L"long index",
		L"<summary>文字列中の末尾からの指定位置の文字コードを取得します。<br/>\n"
		L"文字コードは utf-16 か utf-32 で、何れであるかは環境に依存します。</summary>\n"
		L"<param name=\"index\">取得する文字列末尾からの文字指標</param>\n"
		L"<return>index 位置の文字コード。index が範囲外の場合には 0</return>", nullptr
	},
	{	// public String replace( Map map ) const
		L"replace",
		&LStringObj::method_replace, true,
		L"public const", L"String", L"Map map",
		L"文字列中の map 要素名 key に一致する部分を"
		L" map[key].toString() に置き換えた文字列を取得します。", nullptr
	},
	{	// public uint8* utf8() const
		L"utf8",
		&LStringObj::method_utf8, true,
		L"public const", L"uint8*", L"",
		L"文字列を utf-8 に変換します。\n"
		L"変換されたutf-8 の文字列長は sizeof で取得してください（末尾が 0 ではありません）", nullptr
	},
	{	// public uint16* utf16() const
		L"utf16",
		&LStringObj::method_utf16, true,
		L"public const", L"uint16*", L"",
		L"文字列を utf-16 に変換します。\n"
		L"変換されたutf-16 の文字列長は sizeof で取得してください（末尾が 0 ではありません）", nullptr
	},
	{	// public uint32* utf32() const
		L"utf32",
		&LStringObj::method_utf32, true,
		L"public const", L"uint32*", L"",
		L"文字列を utf-32 に変換します。\n"
		L"変換されたutf-32 の文字列長は sizeof で取得してください（末尾が 0 ではありません）", nullptr
	},
	{	// public long asInteger( boolean* pHasInteger = null, int radix = 10 ) const
		L"asInteger",
		&LStringObj::method_asInteger, true,
		L"public const", L"long", L"boolean* pHasInteger = null, int radix = 10",
		L"文字列を整数値に変換します。整数値以降にも文字列が続いていたとしても無視されます。\n"
		L"<param name=\"pHasInteger\">文字列の先頭に整数値が含まれていた場合には true を受け取る boolean 変数/param>\n"
		L"<param name=\"radix\">整数値を解釈する進数</param>\n"
		L"<return>変換された整数値。整数が文字列の先頭にない場合には 0</return>", nullptr
	},
	{	// public double asNumber( boolean* pHasNumber = null ) const
		L"asNumber",
		&LStringObj::method_asNumber, true,
		L"public const", L"double", L"boolean* pHasNumber = null",
		L"文字列を数値に変換します。数値以降にも文字列が続いていたとしても無視されます。\n"
		L"<param name=\"pHasNumber\">文字列の先頭に数値が含まれていた場合には true を受け取る boolean 変数/param>\n"
		L"<return>変換された数値。数値が文字列の先頭にない場合には 0.0</return>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::NativeFuncDesc	LStringClass::s_Functions[3] =
{
	{	// public static String integerOf( long val, int prec = 0, int radix = 10 ) const
		L"integerOf",
		&LStringObj::method_integerOf, true,
		L"public static", L"String", L"long val, int prec = 0, int radix = 10",
		L"整数値を文字列に変換します。\n"
		L"<param name=\"val\">整数値</param>\n"
		L"<param name=\"prec\">文字列化する場合の桁数。0 の場合には自動的に調整します。</param>"
		L"<param name=\"radix\">文字列化する場合の進数</param>", nullptr
	},
	{	// public static String numberOf( double val, int prec = 0, boolean exp = false ) const
		L"numberOf",
		&LStringObj::method_numberOf, true,
		L"public static", L"String", L"double val, int prec = 0, boolean exp = false",
		L"数値を文字列に変換します。\n"
		L"<param name=\"prec\">文字列化する場合の小数点以下の桁数。0の場合には自動的に調整します。</param>\n"
		L"<param name=\"exp\">true を指定すると、文字列化する場合に指数表現します。"
		L"但し必ず指数表現されるわけではなく、絶対値が 10<sup>prec/2</sup> に対して大きい"
		L"又は 10<sup>-prec/4</sup> に対して小さい場合に指数表現されます。</param>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::BinaryOperatorDesc	LStringClass::s_BinaryOps[13] =
{
	{	// boolean operator == ( String str )
		L"==", &LStringObj::operator_eq, true, true,
		L"boolean", L"String",
		L"文字列の内容を比較し、一致する場合には true を評価します。",
		L"Object"
	},
	{	// boolean operator != ( String str )
		L"!=", &LStringObj::operator_ne, true, true,
		L"boolean", L"String",
		L"文字列の内容を比較し、一致する場合には false を評価します。",
		L"Object"
	},
	{	// boolean operator < ( String str )
		L"<", &LStringObj::operator_lt, true, true,
		L"boolean", L"String",
		L"文字列の内容を比較し、より小さい場合に true を評価します。", nullptr
	},
	{	// boolean operator <= ( String str )
		L"<=", &LStringObj::operator_le, true, true,
		L"boolean", L"String",
		L"文字列の内容を比較し、一致するかより小さい場合に true を評価します。", nullptr
	},
	{	// boolean operator > ( String str )
		L">", &LStringObj::operator_gt, true, true,
		L"boolean", L"String",
		L"文字列の内容を比較し、より大きい場合に true を評価します。", nullptr
	},
	{	// boolean operator >= ( String str )
		L">=", &LStringObj::operator_ge, true, true,
		L"boolean", L"String",
		L"文字列の内容を比較し、一致するかより大きい場合に true を評価します。", nullptr
	},
	{	// String operator + ( String str )
		L"+", &LStringObj::operator_add, true, true,
		L"String", L"String",
		L"結合した文字列を取得します。", nullptr
	},
	{	// String operator + ( long val )
		L"+", &LStringObj::operator_add_int, true, true,
		L"String", L"long",
		L"整数値を文字列に変換して結合した文字列を取得します。", nullptr,
	},
	{	// String operator + ( double val )
		L"+", &LStringObj::operator_add_num, true, true,
		L"String", L"double",
		L"数値を文字列に変換して結合した文字列を取得します。", nullptr,
	},
	{	// String operator + ( Object obj )
		L"+", &LStringObj::operator_add_obj, true, true,
		L"String", L"Object",
		L"オブジェクトを文字列に変換して結合した文字列を取得します。", nullptr,
	},
	{	// String operator * ( long count )
		L"*", &LStringObj::operator_mul, true, true,
		L"String", L"long",
		L"文字列を n 回繰り返した文字列を取得します。\n"
		L"マイナスの場合には文字列の順序を反転して繰り返します。\n"
		L"※反転の際には utf-32 に変換してから反転されます。", nullptr,
	},
	{	// String[] operator / ( String deli )
		L"/", &LStringObj::operator_div, true, true,
		L"String[]", L"String",
		L"右辺値文字列で分割した文字列配列を取得します。", nullptr,
	},
	{
		nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LStringClass::s_MemberDesc =
{
	LStringClass::s_Virtuals,
	LStringClass::s_Functions,
	nullptr,
	LStringClass::s_BinaryOps,
	L"String は文字列を保持するオブジェクトのクラスです。\n"
	L"String のコンストラクタ以外の全てのメソッドは const であり"
	L" String オブジェクトの内容は不変値です。\n"
	L"内容を変更したい場合には StringBuf オブジェクトを使用してください。",
} ;


//////////////////////////////////////////////////////////////////////////////
// StringBuf クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LStringBufClass::CloneObject( void ) const
{
	return	new LStringBufClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LStringBufClass::DuplicateObject( void ) const
{
	return	new LStringBufClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LStringBufClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LStringBufClass::NativeFuncDesc		LStringBufClass::s_Virtuals[7] =
{
	{	// public StringBuf( String str )
		LClass::s_Constructor,
		&LStringBufObj::method_init, true,
		L"public", L"void", L"String str",
		L"StringBuf オブジェクトを構築します。", nullptr
	},
	{	// public StringBuf set( String str )
		L"set",
		&LStringBufObj::method_set, true,
		L"public", L"StringBuf", L"String str",
		L"StringBuf オブジェクトに文字列を複製します。", nullptr
	},
	{	// public StringBuf setIntegerOf( long val, int prec = 0, int radix = 10 )
		L"setIntegerOf",
		&LStringBufObj::method_setIntegerOf, true,
		L"public", L"StringBuf", L"long val, int prec = 0, int radix = 10",
		L"整数値を文字列に変換して StringBuf オブジェクトに設定します。", nullptr
	},
	{	// public StringBuf setNumberOf( double val, int prec = 0, boolean exp = true )
		L"setNumberOf",
		&LStringBufObj::method_setNumberOf, true,
		L"public", L"StringBuf", L"double val, int prec = 0, boolean exp = true",
		L"数値を文字列に変換して StringBuf オブジェクトに設定します。", nullptr
	},
	{	// public void setAt( long index, uint32 code )
		L"setAt",
		&LStringBufObj::method_setAt, true,
		L"public", L"void", L"long index, uint32 code",
		L"文字列の指定位置の文字コードを置き換えます。\n"
		L"<param name=\"index\">置き換える文字の位置。範囲を超えている場合にはこの関数は何もしません。</param>\n"
		L"<param name=\"code\">置き換える文字コード。"
		L"utf-16 で保持している場合には下位16ビットのみが使用されます。</param>", nullptr
	},
	{	// public void resize( long length )
		L"resize",
		&LStringBufObj::method_resize, true,
		L"public", L"void", L"long length",
		L"StringBuf が保持している文字列長を変更します。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LStringBufClass::BinaryOperatorDesc	LStringBufClass::s_BinaryOps[3] =
{
	{	// StringBuf operator := ( String str )
		L":=", &LStringBufObj::operator_smov, true, false,
		L"StringBuf", L"String",
		L"StringBuf に文字列を複製します。", nullptr,
	},
	{	// StringBuf operator += ( String str )
		L"+=", &LStringBufObj::operator_sadd, true, false,
		L"StringBuf", L"String",
		L"StringBuf が保持する文字列の末尾に文字列を追加します。", nullptr,
	},
	{
		nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LStringBufClass::ClassMemberDesc		LStringBufClass::s_MemberDesc =
{
	LStringBufClass::s_Virtuals,
	nullptr,
	nullptr,
	LStringBufClass::s_BinaryOps,
	L"StringBuf は文字列を保持するオブジェクトのクラスです。\n"
	L"String クラスから派生していますが、StringBuf オブジェクトは文字列の内容を変更することができます。",
} ;




//////////////////////////////////////////////////////////////////////////////
// Array クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LArrayClass::CloneObject( void ) const
{
	return	new LArrayClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LArrayClass::DuplicateObject( void ) const
{
	return	new LArrayClass( *this ) ;
}

// pClass へキャスト可能か？（データの変換なしのキャスト）
//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
LClass::ResultInstanceOf LArrayClass::TestInstanceOf( LClass * pClass ) const
{
	ResultInstanceOf	rs = LClass::TestInstanceOf( pClass ) ;
	if ( rs != instanceUnavailable )
	{
		return	rs ;
	}
	LArrayClass *	pArrayClass = dynamic_cast<LArrayClass*>( pClass ) ;
	if ( pArrayClass == nullptr )
	{
		return	instanceUnavailable ;
	}
	if ( m_pElementType != nullptr )
	{
		if ( pArrayClass->m_pElementType == nullptr )
		{
			return	instanceAvailable ;
		}
		return	m_pElementType->TestInstanceOf( pArrayClass->m_pElementType ) ;
	}
	return	instanceUnavailable ;
}

// クラス定義処理（ネイティブな実装）
void LArrayClass::ImplementClass( void )
{
	if ( m_pElementType == nullptr )
	{
		DefineTypeAs( L"@Type", LType( m_vm.GetObjectClass() ), false ) ;
		DefineTypeAs( L"@ArrayType", LType( m_vm.GetArrayClass() ), false ) ;
	}
	else
	{
		DefineTypeAs( L"@Type", LType( m_pElementType ), false ) ;
		DefineTypeAs( L"@ArrayType", LType( m_vm.GetArrayClassAs( m_pElementType ) ), false ) ;
	}
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc		LArrayClass::s_Virtuals[10] =
{
	{	// public uint length() const
		L"length",
		&LArrayObj::method_length, true,
		L"public const", L"uint", L"",
		L"配列長を取得します。", nullptr
	},
	{	// public uint add( Object obj )
		L"add",
		&LArrayObj::method_add, true,
		L"public", L"uint", L"@Type obj",
		L"配列の末尾に追加します。\n"
		L"<param name=\"obj\">追加するオブジェクト</param>\n"
		L"<return>追加された要素の指標</return>", L"Object obj"
	},
	{	// public uint push( Object obj )
		L"push",
		&LArrayObj::method_add, true,
		L"public", L"uint", L"@Type obj",
		L"配列の末尾に追加します。\n"
		L"<param name=\"obj\">追加するオブジェクト</param>\n"
		L"<return>追加された要素の指標</return>", L"Object obj"
	},
	{	// public Object pop()
		L"pop",
		&LArrayObj::method_pop, true,
		L"public", L"@Type", L"",
		L"配列の末尾から要素を取得し、配列から削除します。", nullptr
	},
	{	// public void insert( long index, Object obj )
		L"insert",
		&LArrayObj::method_insert, true,
		L"public", L"void", L"long index, @Type obj",
		L"要素を挿入します。\n"
		L"<param name=\"index\">挿入する指標</param>"
		L"<param name=\"obj\">挿入する要素</param>",
		L"long index, Object obj"
	},
	{	// void merge( long index, const Array src, long first = 0, long count =-1 )
		L"merge",
		&LArrayObj::method_merge, true,
		L"public", L"void", L"long index, const @ArrayType src, long first = 0, long count =-1",
		L"配列を結合します。\n"
		L"<param name=\"index\">結合する配列を挿入する先頭の指標</param>\n"
		L"<param name=\"src\">結合する複製元配列</param>\n"
		L"<param name=\"first\">複製元の src の先頭要素指標</param>\n"
		L"<param name=\"count\">複製する要素数。マイナスの場合には first 以降のすべての要素が対象になります。</param>",
		L"long index, const Array src, long first = 0, long count =-1"
	},
	{	// public Object remove( long index )
		L"remove",
		&LArrayObj::method_remove, true,
		L"public", L"@Type", L"long index",
		L"要素を削除します。\n"
		L"<param name=\"index\">削除する要素の指標</param>\n"
		L"<return>削除された要素</return>", nullptr
	},
	{	// public int findPtr( Object obj )
		L"findPtr",
		&LArrayObj::method_findPtr, true,
		L"public", L"int", L"@Type obj",
		L"配列に含まれるエンティティの指標を検索します。\n"
		L"<param name=\"obj\">検索するエンティティ</param>\n"
		L"<return>要素の指標。見つからなかった場合には -1</return>", L"Object obj"
	},
	{	// public void clear()
		L"clear",
		&LArrayObj::method_clear, true,
		L"public", L"void", L"",
		L"配列に含まれるすべての要素を削除します。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::BinaryOperatorDesc	LArrayClass::s_BinaryOps[5] =
{
	{	// Array operator := ( Array obj )
		L":=", &LArrayObj::operator_smov, true, false,
		L"@ArrayType", L"@ArrayType",
		L"配列の内容を複製します。\n"
		L"各要素も可能な限り参照の複製ではなく実体を複製します。", L"Array",
	},
	{	// Array operator += ( Object obj )
		L"+=", &LArrayObj::operator_sadd, true, false,
		L"@ArrayType", L"@Type",
		L"配列の末尾に要素を追加します。", L"Object",
	},
	{	// Array operator << ( Object obj )
		L"<<", &LArrayObj::operator_sadd, true, false,
		L"@ArrayType", L"@Type",
		L"配列の末尾に要素を追加します。", L"Object",
	},
	{	// Array operator + ( Object obj ) const
		L"+", &LArrayObj::operator_add, true, true,
		L"@ArrayType", L"@Type",
		L"配列の末尾に要素を追加した新しい配列を返します。", L"Object",
	},
	{
		nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LArrayClass::s_MemberDesc =
{
	LArrayClass::s_Virtuals,
	nullptr,
	nullptr,
	LArrayClass::s_BinaryOps,
	L"配列オブジェクトの基底クラスです。",
} ;



//////////////////////////////////////////////////////////////////////////////
// Map クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LMapClass::CloneObject( void ) const
{
	return	new LMapClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LMapClass::DuplicateObject( void ) const
{
	return	new LMapClass( *this ) ;
}

// pClass へキャスト可能か？（データの変換なしのキャスト）
//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
LClass::ResultInstanceOf LMapClass::TestInstanceOf( LClass * pClass ) const
{
	ResultInstanceOf	rs = LClass::TestInstanceOf( pClass ) ;
	if ( rs != instanceUnavailable )
	{
		return	rs ;
	}
	LMapClass *	pMapClass = dynamic_cast<LMapClass*>( pClass ) ;
	if ( pMapClass == nullptr )
	{
		return	instanceUnavailable ;
	}
	if ( m_pElementType != nullptr )
	{
		if ( pMapClass->m_pElementType == nullptr )
		{
			return	instanceAvailable ;
		}
		return	m_pElementType->TestInstanceOf( pMapClass->m_pElementType ) ;
	}
	return	instanceUnavailable ;
}

// クラス定義処理（ネイティブな実装）
void LMapClass::ImplementClass( void )
{
	if ( m_pElementType == nullptr )
	{
		DefineTypeAs( L"@Type", LType( m_vm.GetObjectClass() ), false ) ;
		DefineTypeAs( L"@MapType", LType( m_vm.GetMapClass() ), false ) ;
	}
	else
	{
		DefineTypeAs( L"@Type", LType( m_pElementType ), false ) ;
		DefineTypeAs( L"@MapType", LType( m_vm.GetMapClassAs( m_pElementType ) ), false ) ;
	}
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc		LMapClass::s_Virtuals[7] =
{
	{	// public uint size() const
		L"size",
		&LMapObj::method_size, true,
		L"public const", L"uint", L"",
		L"辞書配列の要素数を取得します。（計算量 O(1)）", nullptr
	},
	{	// public boolean has( String key ) const
		L"has",
		&LMapObj::method_has, true,
		L"public const", L"boolean", L"String key",
		L"要素が存在するか判定します。（計算量 O(logN)）\n"
		L"<param name=\"key\">存在を判定する要素名</param>", nullptr
	},
	{	// public int find( String key ) const
		L"find",
		&LMapObj::method_find, true,
		L"public const", L"int", L"String key",
		L"要素の整数指標を取得します。（計算量 O(logN)）\n"
		L"<param name=\"key\">指標を取得する要素名</param>\n"
		L"<return>要素の整数指標。要素が存在しない場合 -1</return>", nullptr
	},
	{	// public String keyAt( long index ) const
		L"keyAt",
		&LMapObj::method_keyAt, true,
		L"public const", L"String", L"long index",
		L"整数指標に対応する要素名を取得します。（計算量 O(1)）", nullptr
	},
	{	// public Object removeAt( long index )
		L"removeAt",
		&LMapObj::method_removeAt, true,
		L"public", L"@Type", L"long index",
		L"整数指標に対応する要素を削除します。（計算量 O(N)）\n"
		L"<param name=\"index\">削除する整数指標</param>\n"
		L"<return>削除された要素</return>", nullptr
	},
	{	// public Object removeAs( String key )
		L"removeAs",
		&LMapObj::method_removeAs, true,
		L"public", L"@Type", L"String key",
		L"要素名に対応する要素を削除します。（計算量 O(N+logN)）\n"
		L"<param name=\"key\">削除する要素名</param>\n"
		L"<return>削除された要素</return>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::BinaryOperatorDesc	LMapClass::s_BinaryOps[2] =
{
	{	// Map operator := ( Map obj )
		L":=", &LMapObj::operator_smov, true, false,
		L"@MapType", L"@MapType",
		L"辞書配列の内容を複製します。\n"
		L"各要素も可能な限り参照の複製ではなく実体を複製します。", L"Map",
	},
	{
		nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc		LMapClass::s_MemberDesc =
{
	LMapClass::s_Virtuals,
	nullptr,
	nullptr,
	LMapClass::s_BinaryOps,
	L"辞書配列オブジェクトの基底クラスです。\n"
	L"辞書配列は配列の指標に整数、又は文字列を使用できます。\n"
	L"整数指標は要素が追加された順序が保たれます。",
} ;



//////////////////////////////////////////////////////////////////////////////
// Function クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LFunctionClass::CloneObject( void ) const
{
	return	new LFunctionClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LFunctionClass::DuplicateObject( void ) const
{
	return	new LFunctionClass( *this ) ;
}

// ローカルクラス・名前空間以外の全ての要素を解放
void LFunctionClass::DisposeAllObjects( void )
{
	LClass::DisposeAllObjects() ;

	m_pFuncProto = nullptr ;
}

// pClass へキャスト可能か？（データの変換なしのキャスト）
//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
LClass::ResultInstanceOf LFunctionClass::TestInstanceOf( LClass * pClass ) const
{
	ResultInstanceOf	rs = LClass::TestInstanceOf( pClass ) ;
	if ( rs != instanceUnavailable )
	{
		return	rs ;
	}
	LFunctionClass *	pFuncClass = dynamic_cast<LFunctionClass*>( pClass ) ;
	if ( pFuncClass == nullptr )
	{
		return	instanceUnavailable ;
	}
	if ( pFuncClass->m_pFuncProto == nullptr )
	{
		return	instanceAvailable ;
	}
	if ( m_pFuncProto == nullptr )
	{
		return	instanceUnavailable ;
	}
	if ( m_pFuncProto->IsEqualPrototype( *(pFuncClass->m_pFuncProto) ) )
	{
		return	instanceAvailable ;
	}
	return	instanceUnavailable ;
}

// クラス定義処理（ネイティブな実装）
void LFunctionClass::ImplementClass( void )
{
	if ( m_pFuncProto != nullptr )
	{
		// キャプチャー・オブジェクトを受け取る構築関数
		std::shared_ptr<LPrototype>
					pInitProto = std::make_shared<LPrototype>( this ) ;
		pInitProto->ReturnType() = LType() ;
		pInitProto->ArgListType() = m_pFuncProto->GetCaptureObjListTypes() ;

		LPtr<LFunctionObj>	pFunc =
				new LFunctionObj( m_vm.GetFunctionClass(), pInitProto ) ;
		pFunc->SetNative( &LFunctionClass::method_init, false ) ;

		OverrideVirtualFunction( s_Constructor, pFunc ) ;
	}
}

// Function オブジェクトの構築関数
// （キャプチャーオブジェクトを引数に受け取る）
void LFunctionClass::method_init( LContext& context )
{
	LRuntimeArgList		arglist( context ) ;
	LPtr<LFunctionObj>	pThisObj = arglist.NextObjectAs<LFunctionObj>() ;
	if ( pThisObj == nullptr )
	{
		context.ThrowException
			( GetErrorMessage(exceptionNullPointer), L"NullPointerException" ) ;
		return ;
	}
	std::shared_ptr<LPrototype>	pProto = pThisObj->GetPrototype() ;
	if ( pProto == nullptr )
	{
		return ;
	}
	const LNamedArgumentListType&
				argCaptureObjs = pProto->GetCaptureObjListTypes() ;
	std::vector<LValue>	refObjs ;
	for ( size_t i = 0; i < argCaptureObjs.size(); i ++ )
	{
		const LType&	typeObj = argCaptureObjs.at(i) ;
		if ( typeObj.IsPrimitive() )
		{
			refObjs.push_back
				( LValue( typeObj.GetPrimitive(),
							arglist.NextPrimitive() ) ) ;
		}
		else
		{
			refObjs.push_back
				( LValue( typeObj, arglist.NextObject() ) ) ;
		}
	}
	pThisObj->SetCaptureObjectList( refObjs ) ;
}



//////////////////////////////////////////////////////////////////////////////
// ユーザー定義用 Object クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LGenericObjClass::CloneObject( void ) const
{
	return	new LGenericObjClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LGenericObjClass::DuplicateObject( void ) const
{
	return	new LGenericObjClass( *this ) ;
}

// 基底 Object メソッドの定義処理
void LGenericObjClass::ImpletentPureObjectClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LGenericObjClass::s_Virtuals[8] =
{
	{	// public void finalize()
		L"finalize",
		nullptr /*&LObject::method_finalize*/, false,
		L"private", L"void", L"",
		L"デストラクタです。", nullptr,
	},
	{	// public Class getClass() const
		L"getClass",
		&LObject::method_getClass, true,
		L"public const", L"Class", L"",
		L"オブジェクトのクラスを取得します。", nullptr,
	},
	{	// public String toString() const
		L"toString",
		&LObject::method_toString, true,
		L"public const", L"String", L"",
		L"オブジェクトを文字列に変換します。", nullptr,
	},
	{	// public void notify()
		L"notify",
		&LObject::method_notify, false,
		L"public", L"void", L"",
		L"このオブジェクトに対して wait している１つのスレッドを起床させます。", nullptr,
	},
	{	// public void notifyAll()
		L"notifyAll",
		&LObject::method_notifyAll, false,
		L"public", L"void", L"", 
		L"このオブジェクトに対して wait している全てのスレッドを起床させます。", nullptr,
	},
	{	// public void wait()
		L"wait",
		&LObject::method_wait0, false,
		L"public", L"void", L"",
		L"このオブジェクトへの通知があるまでスレッドを待機状態（休止）にします。\n"
		L"呼び出し時にこのオブジェクトに対して synchronized 状態でなければなりません。", nullptr,
	},
	{	// public void wait( long timeout )
		L"wait",
		&LObject::method_wait1, false,
		L"public", L"void", L"long timeout",
		L"<summary>このオブジェクトへの通知があるかタイムアウトするまでスレッドを待機状態（休止）にします。<br/>"
		L"呼び出し時にこのオブジェクトに対して synchronized 状態でなければなりません。</summary>\n"
		L"<param name=\"timeout\">ミリ秒単位でのタイムアウト時間</param>", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::BinaryOperatorDesc	LGenericObjClass::s_BinaryOps[3] =
{
	{	// boolean operator == ( Object obj )
		L"==", &LGenericObjClass::operator_eq, true, true,
		L"boolean", L"Object",
		L"同一エンティティなら true を評価します。", nullptr,
	},
	{	// boolean operator != ( Object obj )
		L"!=", &LGenericObjClass::operator_ne, true, true,
		L"boolean", L"Object",
		L"同一エンティティなら false を評価します。", nullptr,
	},
	{	nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr },
} ;

const LClass::ClassMemberDesc		LGenericObjClass::s_MemberDesc =
{
	LGenericObjClass::s_Virtuals,
	nullptr, 
	nullptr,
	LGenericObjClass::s_BinaryOps,
	L"全てのオブジェクトの基底クラスです。",
} ;

// boolean operator == ( Object obj )
LValue::Primitive LGenericObjClass::operator_eq
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.pObject == val2.pObject ) ;
}

// boolean operator != ( Object obj )
LValue::Primitive LGenericObjClass::operator_ne
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	return	LValue::MakeBool( val1.pObject != val2.pObject ) ;
}




//////////////////////////////////////////////////////////////////////////////
// NativeObject クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LNativeObjClass::CloneObject( void ) const
{
	return	new LNativeObjClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LNativeObjClass::DuplicateObject( void ) const
{
	return	new LNativeObjClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LNativeObjClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc		LNativeObjClass::s_Virtuals[2] =
{
	{	// public void dispose()
		L"dispose",
		&LNativeObj::method_dispose, false,
		L"public", L"void", L"",
		L"保持しているネイティブオブジェクトを解放します。\n"
		L"別のオブジェクトが同じエンティティを参照している場合、"
		L"実際にネイティブオブジェクトが解放されるとは限りません。", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::BinaryOperatorDesc	LNativeObjClass::s_BinaryOps[3] =
{
	{	// boolean operator == ( NativeObject obj )
		L"==", &LNativeObj::operator_eq, false, true,
		L"boolean", L"NativeObject",
		L"ネイティブオブジェクトが同一エンティティの場合 true を評価します。", nullptr,
	},
	{	// boolean operator != ( NativeObject obj )
		L"!=", &LNativeObj::operator_ne, false, true,
		L"boolean", L"NativeObject",
		L"ネイティブオブジェクトが同一エンティティの場合 false を評価します。", nullptr,
	},
	{	nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr },
} ;

const LClass::ClassMemberDesc		LNativeObjClass::s_MemberDesc =
{
	LNativeObjClass::s_Virtuals,
	nullptr, 
	nullptr,
	LNativeObjClass::s_BinaryOps,
	L"ネイティブオブジェクトを保持するオブジェクトです。",
} ;



//////////////////////////////////////////////////////////////////////////////
// Structure クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LStructureClass::CloneObject( void ) const
{
	return	new LStructureClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LStructureClass::DuplicateObject( void ) const
{
	return	new LStructureClass( *this ) ;
}

// オブジェクト生成（コンストラクタは後で呼び出し元責任で呼び出す）
LObject * LStructureClass::CreateInstance( void )
{
	LPointerObj *	pPtrObj =
			new LPointerObj( GetClass()->VM().GetPointerClass() ) ;

	const size_t	nStructBytes = GetStructBytes() ;
	std::shared_ptr<LArrayBufStorage>
			pBuf = std::make_shared<LArrayBufStorage>( nStructBytes ) ;

	assert( pBuf->GetBytes() >= nStructBytes ) ;
	assert( m_protoBuffer.GetBuffer()->GetBytes() >= nStructBytes ) ;

	memcpy( pBuf->GetBuffer(),
			m_protoBuffer.GetBuffer()->GetBuffer(), nStructBytes ) ;

	pPtrObj->SetPointer( pBuf, 0, nStructBytes ) ;
	return	pPtrObj ;
}

// 親クラス追加
bool LStructureClass::AddSuperClass( LClass * pClass )
{
	assert( GetProtoArrangemenet().GetAlignedBytes() == 0 ) ;
	const size_t	nBaseLoc = GetProtoArrangemenet().GetAlignedBytes() ;

	if ( !LClass::AddSuperClass( pClass ) )
	{
		return	false ;
	}
	m_mapArrangement.insert
		( std::make_pair<LClass*,size_t>
			( (LClass*) pClass, (size_t) nBaseLoc ) ) ;
	return	true ;
}

// インターフェース追加（２つめ以降の struct 派生）
bool LStructureClass::AddImplementClass( LClass * pClass )
{
	if ( !TestExtendingClass( pClass ) )
	{
		return	false ;
	}
	m_vecImplements.push_back( pClass ) ;
	//
	if ( pClass->m_pPrototype != nullptr )
	{
		if ( pClass->m_pPrototype->GetElementCount() > 0 )
		{
			LCompiler::Error
				( errorInterfaceHasNoMemberVariable,
					GetClassName().c_str(), pClass->GetClassName().c_str() ) ;
		}
	}
	//
	// メンバ変数配置
	//
	ProtoArrangemenet().MakeAlignment
			( pClass->GetProtoArrangemenet().GetArrangeAlign() ) ;
	//
	m_mapArrangement.insert
		( std::make_pair<LClass*,size_t>
				( (LClass*) pClass, GetProtoArrangemenet().GetAlignedBytes() ) ) ;
	//
	AddMemberArrangement
		( ProtoArrangemenet(), pClass->GetProtoArrangemenet() ) ;
	ProtoArrangemenet().MakePrivateInvisible() ;
	//
	m_nSuperBufSize = ProtoArrangemenet().GetAlignedBytes() ;
	//
	// 関数実装
	//
	ImplementVirtuals( pClass ) ;

	return	true ;
}

// 親クラスへのキャストの際のオフセット
const size_t * LStructureClass::GetOffsetCastTo( LClass * pClass ) const
{
	auto	iter = m_mapArrangement.find( pClass ) ;
	if ( iter == m_mapArrangement.end() )
	{
		return	nullptr ;
	}
	return	&(iter->second) ;
}


// 二項演算子
LValue::Primitive LStructureClass::operator_store_ptr
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LValue::Primitive	prmBytes ;
	prmBytes.pVoidPtr = instance ;

	std::uint8_t *	p1 = nullptr ;
	std::uint8_t *	p2 = nullptr ;
	if ( val1.pObject != nullptr )
	{
		LPointerObj *	pPtr1 = dynamic_cast<LPointerObj*>( val1.pObject ) ;
		if ( pPtr1 != nullptr )
		{
			p1 = pPtr1->GetPointer( 0, (size_t) prmBytes.longValue ) ;
		}
	}
	if ( val2.pObject != nullptr )
	{
		LPointerObj *	pPtr2 = dynamic_cast<LPointerObj*>( val2.pObject ) ;
		if ( pPtr2 != nullptr )
		{
			p2 = pPtr2->GetPointer( 0, (size_t) prmBytes.longValue ) ;
		}
	}
	if ( (p1 != nullptr) && (p2 != nullptr) )
	{
		memcpy( p1, p2, (size_t) prmBytes.longValue ) ;
	}
	return	prmBytes ;
}

LValue::Primitive LStructureClass::operator_store_fetch_addr
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LValue::Primitive	prmBytes ;
	prmBytes.pVoidPtr = instance ;

	if ( (val1.pVoidPtr != nullptr) && (val2.pVoidPtr != nullptr) )
	{
		memcpy( val1.pVoidPtr, val2.pVoidPtr, (size_t) prmBytes.longValue ) ;
	}
	return	prmBytes ;
}



//////////////////////////////////////////////////////////////////////////////
// Exception クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LExceptionClass::CloneObject( void ) const
{
	return	new LExceptionClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LExceptionClass::DuplicateObject( void ) const
{
	return	new LExceptionClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LExceptionClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc		LExceptionClass::s_Virtuals[2] =
{
	{	// public void Exception( String msg )
		LClass::s_Constructor,
		&LExceptionObj::method_init, false,
		L"public", L"void", L"String msg",
		L"例外オブジェクトを構築します。\n"
		L"<param name=\"msg\">例外エラーメッセージ</param>", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc		LExceptionClass::s_MemberDesc =
{
	LExceptionClass::s_Virtuals,
	nullptr, 
	nullptr,
	nullptr,
	L"例外オブジェクトの基底クラスです。",
} ;



//////////////////////////////////////////////////////////////////////////////
// Task クラス
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LTaskClass::CloneObject( void ) const
{
	return	new LTaskClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LTaskClass::DuplicateObject( void ) const
{
	return	new LTaskClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LTaskClass::ImplementClass( void )
{
	DefineTypeAs( L"@Type", m_typeReturn, false ) ;
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LTaskClass::s_Virtuals[11] =
{
	{	// public boolean begin( Function<Object()> func )
		L"begin",
		&LTaskObj::method_begin, false,
		L"public", L"boolean", L"Function<@Type()> func",
		L"タスクに実行する関数を設定します。\n"
		L"<param name=\"func\">実行する関数。関数の返り値型はジェネリック型によります。</param>", L"Function<void()> func",
	},
	{	// public boolean proceed( long msecTimeout = 0 )
		L"proceed",
		&LTaskObj::method_proceed, false,
		L"public", L"boolean", L"long msecTimeout = 0",
		L"タスクを実行する。\n"
		L"<param name=\"msecTimeout\">ミリ秒単位でのタイムアウト時間。<br/>"
		L"最悪の場合この時間内は処理を中断できないため長い時間を指定できません"
		L"（システムによって大きくない値にクリップされます）。</param>"
		L"<return>タスクの実行が完了すると true が返されます。</return>", nullptr,
	},
	{	// public boolean isFinished() const
		L"isFinished",
		&LTaskObj::method_isFinished, false,
		L"public const", L"boolean", L"",
		L"タスクの実行が完了しているか判定します。", nullptr,
	},
	{	// public boolean waitFor()
		L"waitFor",
		&LTaskObj::method_waitFor0, false,
		L"public", L"boolean", L"",
		L"タスクの実行が完了するまで待機します。\n"
		L"この関数は join の別名です。", nullptr,
	},
	{	// public boolean waitFor( long msecTimeout )
		L"waitFor",
		&LTaskObj::method_waitFor1, false,
		L"public", L"boolean", L"long msecTimeout",
		L"タスクの実行が完了するか、タイムアウトするまで待機します。<br/>\n"
		L"proceed が別のスレッドで実行されていなければ常にタイムアウトすることに注意してください。\n"
		L"<param name=\"msecTimeout\">ミリ秒単位でのタイムアウト時間</param>\n"
		L"<return>タスクの実行が完了すると true が返されます。</return>", nullptr,
	},
	{	// public boolean join()
		L"join",
		&LTaskObj::method_waitFor0, false,
		L"public", L"boolean", L"",
		L"タスクの実行が完了するまで待機します。\n"
		L"proceed が別のスレッドで実行されていなければこの関数は永遠に復帰しません。", nullptr,
	},
	{	// public Object getReturned() const
		L"getReturned",
		&LTaskObj::method_getReturned, false,
		L"public const", L"@Type", L"",
		L"完了したタスク関数の返り値を取得します。\n"
		L"返り値の型はジェネリック型によります。", L"",
	},
	{	// public Object getUnhandledException() const
		L"getUnhandledException",
		&LTaskObj::method_getUnhandledException, false,
		L"public const", L"Object", L"",
		L"タスク関数でキャッチされない例外が発生して終了した場合、"
		L"その例外オブジェクトを取得します。", nullptr,
	},
	{	// public void finish()
		L"finish",
		&LTaskObj::method_finish, false,
		L"public", L"void", L"",
		L"タスクを即座に終了させます。\n"
		L"実行中のタスクが保持していたオブジェクトの参照はすべて解放されます。", nullptr,
	},
	{	// public void throw( Object obj )
		L"throw",
		&LTaskObj::method_throw, false,
		L"public", L"void", L"Object obj",
		L"実行中のタスクに例外を送出します。", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::NativeFuncDesc	LTaskClass::s_Functions[3] =
{
	{	// public static Task getCurrent()
		L"getCurrent",
		&LTaskObj::method_getCurrent, false,
		L"public static", L"Task", L"",
		L"タスク関数内で実行中のタスクを取得します。", nullptr,
	},
	{	// public static void rest()
		L"rest",
		&LTaskObj::method_rest, false,
		L"public static", L"void", L"",
		L"タスク関数内で実行中の proceed 関数を脱出させます。\n"
		L"Thread.sleep(0) と同じ効果を得ます。", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LTaskClass::s_MemberDesc =
{
	LTaskClass::s_Virtuals,
	LTaskClass::s_Functions,
	nullptr,
	nullptr,
	L"軽量スレッドを実行するタスクオブジェクトの基底クラスです。",
} ;



//////////////////////////////////////////////////////////////////////////
// Thread クラス
//////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
LObject * LThreadClass::CloneObject( void ) const
{
	return	new LThreadClass( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LThreadClass::DuplicateObject( void ) const
{
	return	new LThreadClass( *this ) ;
}

// クラス定義処理（ネイティブな実装）
void LThreadClass::ImplementClass( void )
{
	DefineTypeAs( L"@Type", m_typeReturn, false ) ;
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LThreadClass::s_Virtuals[3] =
{
	{	// public boolean begin( Function<Object()> func )
		L"begin",
		&LThreadObj::method_begin, false,
		L"public", L"boolean", L"Function<@Type()> func",
		L"スレッドを開始し関数を実行します。", L"Function<void()> func",
	},
	{	// public Object getReturned() const
		L"getReturned",
		&LThreadObj::method_getReturned, false,
		L"public const", L"@Type", L"",
		L"完了したスレッド関数の返り値を取得します。\n"
		L"返り値の型はジェネリック型によります。", L"",
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::NativeFuncDesc	LThreadClass::s_Functions[3] =
{
	{	// public static Thread getCurrent()
		L"getCurrent",
		&LThreadObj::method_getCurrent, false,
		L"public static", L"Thread", L"",
		L"スレッド関数内で実行中のスレッドを取得します。", nullptr,
	},
	{	// public static void sleep( long msecTimeout )
		L"sleep",
		&LThreadObj::method_sleep, false,
		L"public static", L"void", L"long msecTimeout",
		L"スレッドの実行を一時的に休止します。\n"
		L"<param name=\"msecTimeout\">ミリ秒単位での休止時間</param>", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LThreadClass::s_MemberDesc =
{
	LThreadClass::s_Virtuals,
	LThreadClass::s_Functions,
	nullptr,
	nullptr,
	L"スレッドオブジェクトの基底クラスです。\n"
	L"Thread クラスは Task クラスから派生したクラスで、"
	L"オーバーライドされた begin 関数ではスレッドを作成し、"
	L"実行が完了するまで Task.proceed を繰り返し実行します。",
} ;



