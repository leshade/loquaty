
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Loquaty 仮想マシン
//////////////////////////////////////////////////////////////////////////////

LOQUATY_DLL_DECL
( LVirtualMachine *	LVirtualMachine::s_pCurrent = nullptr ) ;

LVirtualMachine::LVirtualMachine( void )
	: m_refGlobal( false ),
		m_pFirstThread( nullptr ), m_pDebugger( nullptr )
{
	assert( classBasicCount == sizeof(m_pBasicClass)/sizeof(m_pBasicClass[0]) ) ;
	for ( int i = 0; i < classBasicCount; i ++ )
	{
		m_pBasicClass[i] = nullptr ;
	}
	assert( LType::typePrimitiveCount
			== sizeof(m_pPrimitivePtrClass)/sizeof(m_pPrimitivePtrClass[0]) ) ;
	for ( int i = 0; i < LType::typePrimitiveCount; i ++ )
	{
		m_pPrimitivePtrClass[i] = nullptr ;
	}
	assert( LType::typePrimitiveCount
			== sizeof(m_pPrimitiveCPtrClass)/sizeof(m_pPrimitiveCPtrClass[0]) ) ;
	for ( int i = 0; i < LType::typePrimitiveCount; i ++ )
	{
		m_pPrimitiveCPtrClass[i] = nullptr ;
	}
	m_pVoidPointerClass = nullptr ;
	m_pConstVoidPointerClass = nullptr ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	m_producer.AddProducer
		( std::make_shared<LPluginModuleProducer>( L"" ) ) ;
	m_producer.AddProducer
		( std::make_shared<LPluginModuleProducer>( L"plugins" ) ) ;
#endif
}

LVirtualMachine::~LVirtualMachine( void )
{
	if ( s_pCurrent == this )
	{
		s_pCurrent = nullptr ;
	}
	Release() ;
}

// 初期化
void LVirtualMachine::Initialize( void )
{
	LPackagePtr	pPackage =
		std::make_shared<LPackage>( LPackage::typeSystemDefault, L"system" ) ;
	AddPackage( pPackage ) ;

	LPackage::Current	current( pPackage.get() ) ;

	// 基本クラス
	LClassClass *	pClassClass = new LClassClass( *this, nullptr, L"Class" ) ;
	pClassClass->SetClass( pClassClass ) ;
	m_pBasicClass[classClass] = pClassClass ;

	LClass *	pNamespaceClass =
				new LClass( *this, LPtr<LNamespace>(), pClassClass, L"Namespace" ) ;
	m_pBasicClass[classNamespace] = pNamespaceClass ;

	LGenericObjClass *	pObjectClass =
				new LGenericObjClass( *this, LPtr<LNamespace>(), pClassClass, L"Object" ) ;
	m_pBasicClass[classObject] = pObjectClass ;

	LNativeObjClass *	pNativeObjClass =
				new LNativeObjClass( *this, LPtr<LNamespace>(), pClassClass, L"NativeObject" ) ;
	m_pBasicClass[classNativeObject] = pNativeObjClass ;

	LStructureClass *	pStructClass =
				new LStructureClass( *this, LPtr<LNamespace>(), pClassClass, L"Structure" ) ;
	m_pBasicClass[classStructure] = pStructClass ;

	LType						typeVoid ;
	const std::vector<size_t>	dimArray ;
	LDataArrayClass *	pDataArrayClass =
				new LDataArrayClass( *this, pClassClass, L"DataArray", typeVoid, dimArray ) ;
	m_pBasicClass[classDataArray] = pDataArrayClass ;

	LPointerClass *	pPointerClass =
				new LPointerClass( *this, pClassClass, L"Pointer", typeVoid ) ;
	m_pBasicClass[classPointer] = pPointerClass ;

	LIntegerClass *	pIntegerClass =
				new LIntegerClass( *this, pClassClass, L"Integer" ) ;
	m_pBasicClass[classInteger] = pIntegerClass ;

	LDoubleClass *	pDoubleClass =
				new LDoubleClass( *this, pClassClass, L"Double" ) ;
	m_pBasicClass[classDouble] = pDoubleClass ;

	LArrayClass *	pArrayClass =
				new LArrayClass( *this, pClassClass, L"Array" ) ;
	m_pBasicClass[classArray] = pArrayClass ;

	LMapClass *	pMapClass =
				new LMapClass( *this, pClassClass, L"Map" ) ;
	m_pBasicClass[classMap] = pMapClass ;

	LStringClass *	pStringClass =
				new LStringClass( *this, pClassClass, L"String" ) ;
	m_pBasicClass[classString] = pStringClass ;

	LStringBufClass *	pStringBufClass =
				new LStringBufClass( *this, pClassClass, L"StringBuf" ) ;
	m_pBasicClass[classStringBuf] = pStringBufClass ;

	LFunctionClass *	pFunctionClass =
				new LFunctionClass( *this, pClassClass, L"Function" ) ;
	m_pBasicClass[classFunction] = pFunctionClass ;

	LExceptionClass *	pExceptionClass =
				new LExceptionClass( *this, LPtr<LNamespace>(), pClassClass, L"Exception" ) ;
	m_pBasicClass[classException] = pExceptionClass ;

	LTaskClass *	pTaskClass =
				new LTaskClass( *this, LPtr<LNamespace>(), pClassClass, L"Task", LType() ) ;
	m_pBasicClass[classTask] = pTaskClass ;

	LThreadClass *	pThreadClass =
				new LThreadClass( *this, LPtr<LNamespace>(), pClassClass, L"Thread", LType() ) ;
	m_pBasicClass[classThread] = pThreadClass ;

	// 大域空間
	m_pGlobal = new LNamespace( *this, LPtr<LNamespace>(), pNamespaceClass ) ;
	m_refGlobal = false ;

	// まず基本クラスを宣言だけする
	for ( int i = 0; i < classBasicCount; i ++ )
	{
		assert( m_pBasicClass[i] != nullptr ) ;
		m_pGlobal->AddClassAs
			( m_pBasicClass[i]->GetClassName().c_str(),
				LPtr<LClass>( m_pBasicClass[i] ) ) ;
	}

	// プリミティブ型のポインタ型を宣言
	struct
	{
		LType::Primitive	type ;
		LType::Modifiers	mod ;
		LClass *			pClass ;
	}	s_PrimitiveTyps[] =
	{
		{ LType::typeBoolean, 0 },
		{ LType::typeBoolean, LType::modifierConst },
		{ LType::typeInt8, 0 },
		{ LType::typeInt8, LType::modifierConst },
		{ LType::typeUint8, 0 },
		{ LType::typeUint8, LType::modifierConst },
		{ LType::typeInt16, 0 },
		{ LType::typeInt16, LType::modifierConst },
		{ LType::typeUint16, 0 },
		{ LType::typeUint16, LType::modifierConst },
		{ LType::typeInt32, 0 },
		{ LType::typeInt32, LType::modifierConst },
		{ LType::typeUint32, 0 },
		{ LType::typeUint32, LType::modifierConst },
		{ LType::typeInt64, 0 },
		{ LType::typeInt64, LType::modifierConst },
		{ LType::typeUint64, 0 },
		{ LType::typeUint64, LType::modifierConst },
		{ LType::typeFloat, 0 },
		{ LType::typeFloat, LType::modifierConst },
		{ LType::typeDouble, 0 },
		{ LType::typeDouble, LType::modifierConst },
		{ LType::typeObject, 0 },
	} ;
	for ( int i = 0; s_PrimitiveTyps[i].type != LType::typeObject; i ++ )
	{
		LType	typeData( s_PrimitiveTyps[i].type, s_PrimitiveTyps[i].mod ) ;
		LString	strPtrTypeName = typeData.GetTypeName() + L"*" ;
		LPtr<LClass>
				pClass( new LPointerClass
						( *this, m_pBasicClass[classClass],
							strPtrTypeName.c_str(), typeData ) ) ;

		m_pGlobal->AddClassAs( strPtrTypeName.c_str(), pClass ) ;

		if ( s_PrimitiveTyps[i].mod == 0 )
		{
			m_pPrimitivePtrClass[s_PrimitiveTyps[i].type] = pClass.Ptr() ;
		}
		else
		{
			m_pPrimitiveCPtrClass[s_PrimitiveTyps[i].type] = pClass.Ptr() ;
		}
		s_PrimitiveTyps[i].pClass = pClass.Ptr() ;
	}
	m_pVoidPointerClass = new LPointerClass
						( *this, m_pBasicClass[classClass],
							L"void*", LType( nullptr, 0 ) ) ;
	m_pGlobal->AddClassAs
			( L"void*", LPtr<LClass>( m_pVoidPointerClass ) ) ;

	m_pConstVoidPointerClass = new LPointerClass
						( *this, m_pBasicClass[classClass],
							L"const void*", LType( nullptr, LType::modifierConst ) ) ;
	m_pGlobal->AddClassAs
			( L"const void*", LPtr<LClass>( m_pConstVoidPointerClass ) ) ;

	// 基底クラスの定義処理
	pObjectClass->ImpletentPureObjectClass() ;
	pObjectClass->CompleteClass() ;

	// クラスの派生
	LBatchClassImplementor	bci( *m_pGlobal ) ;
	bci.DeclareClass( pClassClass, pObjectClass ) ;
	bci.DeclareClass( pNamespaceClass, pObjectClass ) ;
	bci.DeclareClass( pNativeObjClass, pObjectClass ) ;
	bci.DeclareClass( pStructClass, pObjectClass ) ;
	bci.DeclareClass( pDataArrayClass, pObjectClass ) ;
	bci.DeclareClass( pPointerClass, pObjectClass ) ;
	bci.DeclareClass( pIntegerClass, pObjectClass ) ;
	bci.DeclareClass( pDoubleClass, pObjectClass ) ;
	bci.DeclareClass( pArrayClass, pObjectClass ) ;
	bci.DeclareClass( pMapClass, pObjectClass ) ;
	bci.DeclareClass( pFunctionClass, pObjectClass ) ;
	bci.DeclareClass( pExceptionClass, pObjectClass ) ;
	bci.DeclareClass( pTaskClass, pObjectClass ) ;
	bci.Implement() ;

	bci.DeclareClass( pStringClass, pObjectClass ) ;
	bci.Implement() ;

	bci.DeclareClass( pStringBufClass, pStringClass ) ;
	bci.Implement() ;

	bci.DeclareClass( pThreadClass, pTaskClass ) ;
	bci.Implement() ;

	// プリミティブ型のポインタ型定義
	for ( int i = 0; s_PrimitiveTyps[i].type != LType::typeObject; i ++ )
	{
		LClass *	pClass = s_PrimitiveTyps[i].pClass ;
		pClass->AddSuperClass( m_pBasicClass[classPointer] ) ;
		pClass->ImplementClass() ;
		pClass->CompleteClass() ;
		pClass->m_flagGenInstance = true ;
	}
	m_pVoidPointerClass->AddSuperClass( m_pBasicClass[classPointer] ) ;
	m_pVoidPointerClass->ImplementClass() ;
	m_pVoidPointerClass->CompleteClass() ;
	m_pVoidPointerClass->m_flagGenInstance = true ;

	m_pConstVoidPointerClass->AddSuperClass( m_pBasicClass[classPointer] ) ;
	m_pConstVoidPointerClass->ImplementClass() ;
	m_pConstVoidPointerClass->CompleteClass() ;
	m_pConstVoidPointerClass->m_flagGenInstance = true ;

	// リリース順序の制御のため
	// ※本当は LObject の m_pClass を AddRef, ReleaseRef で管理すればいいのだが
	//   さすがにそれはハイコストかと思って
	for ( int i = 0; i < classBasicCount; i ++ )
	{
		m_pBasicClass[i]->AddRef() ;
	}
	pClassClass->AddRef() ;			// ※一番最後にリリースするため


	// ネイティブ関数
	AddNativeFuncDefinitions() ;


	// 追加の標準クラス
	bci.AddClass( LPtr<LClass>( new LConsoleClass
					( *this, pClassClass, L"Console" ) ), pObjectClass ) ;
	bci.AddClass( LPtr<LClass>( new LDateTimeStructure
					( *this, pClassClass, L"DateTime" ) ), pStructClass ) ;
	bci.AddClass( LPtr<LClass>( new LClockClass
					( *this, pClassClass, L"Clock" ) ), pObjectClass ) ;
	bci.AddClass( LPtr<LClass>( new LMathClass
					( *this, pClassClass, L"Math" ) ), pObjectClass ) ;
	bci.AddClass( LPtr<LClass>( new LFileClass
					( *this, pClassClass, L"File" ) ), pObjectClass ) ;
	bci.AddClass( LPtr<LClass>( new LOutputStreamClass
					( *this, pClassClass, L"OutputStream" ) ), pObjectClass ) ;

	LPtr<LClass>
		pLoquatyClass
			( new LLoquatyClass( *this, pClassClass, L"Loquaty" ) ) ;
	bci.AddClass( pLoquatyClass, pObjectClass ) ;

	bci.Implement() ;

	SetClass( pLoquatyClass.Ptr() ) ;
}

void LVirtualMachine::InitializeRef( LVirtualMachine& vmRef )
{
	assert( vmRef.m_pGlobal != nullptr ) ;
	if ( vmRef.m_pGlobal == nullptr )
	{
		return ;
	}
	m_pGlobal = new LNamespace( *(vmRef.m_pGlobal) ) ;
	m_refGlobal = true ;

	for ( int i = 0; i < classBasicCount; i ++ )
	{
		assert( vmRef.m_pBasicClass[i] != nullptr ) ;
		m_pBasicClass[i] = vmRef.m_pBasicClass[i] ;
		m_pBasicClass[i]->AddRef() ;
	}
	m_pBasicClass[classClass]->AddRef() ;

	for ( int i = 0; i < LType::typePrimitiveCount; i ++ )
	{
		m_pPrimitivePtrClass[i] = vmRef.m_pPrimitivePtrClass[i] ;
		m_pPrimitiveCPtrClass[i] = vmRef.m_pPrimitiveCPtrClass[i] ;
	}
	m_pVoidPointerClass = vmRef.m_pVoidPointerClass ;
	m_pConstVoidPointerClass = vmRef.m_pConstVoidPointerClass ;

	m_producer = vmRef.m_producer ;
	m_sources = vmRef.m_sources ;

	m_mapSolvedFuncs = vmRef.m_mapSolvedFuncs ;
	m_mapNativeFuncs = vmRef.m_mapNativeFuncs ;

	SetClass( vmRef.GetClass() ) ;
}

// 解放
void LVirtualMachine::Release( void )
{
	TerminateAllThreads() ;
	SetClass( nullptr ) ;

	m_mapSolvedFuncs.clear() ;
	m_mapNativeFuncs.clear() ;

	if ( m_pGlobal != nullptr )
	{
		// 一通りの解放
		if ( !m_refGlobal )
		{
			m_pGlobal->DisposeAllObjects() ;
		}
		m_pGlobal = nullptr ;
		m_refGlobal = false ;

		// クラスの最終的なリリース
		for ( int i = 0; i < classBasicCount; i ++ )
		{
			m_pBasicClass[i]->ReleaseRef() ;
		}
		m_pBasicClass[classClass]->ReleaseRef() ;
	}
}

// グローバルに設定された LVirtualMachine
LVirtualMachine * LVirtualMachine::GetCurrentVM( void )
{
	if ( s_pCurrent != nullptr )
	{
		return	s_pCurrent ;
	}
	LCompiler *	pCompiler = LCompiler::GetCurrent() ;
	if ( pCompiler != nullptr )
	{
		return	&(pCompiler->VM()) ;
	}
	LContext *	pContext = LContext::GetCurrent() ;
	if ( pContext != nullptr )
	{
		return	&(pContext->VM()) ;
	}
	return	nullptr ;
}

void LVirtualMachine::SetCurrentVM( void )
{
	s_pCurrent = this ;
}

// グローバル空間
LPtr<LNamespace> LVirtualMachine::Global( void ) const
{
	return	m_pGlobal ;
}

// モジュール・プロデューサー
LModuleProducer& LVirtualMachine::ModuleProducer( void )
{
	return	m_producer ;
}

// ソース・プロデューサー
LSourceProducer& LVirtualMachine::SourceProducer( void )
{
	return	m_sources ;
}

// パッケージ追加
void LVirtualMachine::AddPackage( LPackagePtr package )
{
	m_packages.push_back( package ) ;
}

// パッケージ・リスト
const std::vector<LPackagePtr>&
				LVirtualMachine::GetPackageList( void ) const
{
	return	m_packages ;
}

// 基本クラス取得
LClass * LVirtualMachine::GetBasicClass
			( LVirtualMachine::BasicClassIndex clsIndex ) const
{
	return	m_pBasicClass[clsIndex] ;
}

LClassClass * LVirtualMachine::GetClassClass( void ) const
{
	assert( dynamic_cast<LClassClass*>(m_pBasicClass[classClass]) != nullptr ) ;
	return	static_cast<LClassClass*>(m_pBasicClass[classClass]) ;
}

LClass * LVirtualMachine::GetNamespaceClass( void ) const
{
	return	m_pBasicClass[classNamespace] ;
}

LGenericObjClass * LVirtualMachine::GetObjectClass( void ) const
{
	assert( dynamic_cast<LGenericObjClass*>(m_pBasicClass[classObject]) != nullptr ) ;
	return	static_cast<LGenericObjClass*>(m_pBasicClass[classObject]) ;
}

LNativeObjClass * LVirtualMachine::GetNativeObjClass( void ) const
{
	assert( dynamic_cast<LNativeObjClass*>(m_pBasicClass[classNativeObject]) != nullptr ) ;
	return	static_cast<LNativeObjClass*>(m_pBasicClass[classNativeObject]) ;
}

LStructureClass * LVirtualMachine::GetStructureClass( void ) const
{
	assert( dynamic_cast<LStructureClass*>(m_pBasicClass[classStructure]) != nullptr ) ;
	return	static_cast<LStructureClass*>(m_pBasicClass[classStructure]) ;
}

LDataArrayClass * LVirtualMachine::GetDataArrayClass( void ) const
{
	assert( dynamic_cast<LDataArrayClass*>(m_pBasicClass[classDataArray]) != nullptr ) ;
	return	static_cast<LDataArrayClass*>(m_pBasicClass[classDataArray]) ;
}

LPointerClass * LVirtualMachine::GetPointerClass( void ) const
{
	assert( dynamic_cast<LPointerClass*>(m_pBasicClass[classPointer]) != nullptr ) ;
	return	static_cast<LPointerClass*>(m_pBasicClass[classPointer]) ;
}

LIntegerClass * LVirtualMachine::GetIntegerObjClass( void ) const
{
	assert( dynamic_cast<LIntegerClass*>(m_pBasicClass[classInteger]) != nullptr ) ;
	return	static_cast<LIntegerClass*>(m_pBasicClass[classInteger]) ;
}

LDoubleClass * LVirtualMachine::GetDoubleObjClass( void ) const
{
	assert( dynamic_cast<LDoubleClass*>(m_pBasicClass[classDouble]) != nullptr ) ;
	return	static_cast<LDoubleClass*>(m_pBasicClass[classDouble]) ;
}

LStringClass * LVirtualMachine::GetStringClass( void ) const
{
	assert( dynamic_cast<LStringClass*>(m_pBasicClass[classString]) != nullptr ) ;
	return	static_cast<LStringClass*>(m_pBasicClass[classString]) ;
}

LStringBufClass * LVirtualMachine::GetStringBufClass( void ) const
{
	assert( dynamic_cast<LStringBufClass*>(m_pBasicClass[classStringBuf]) != nullptr ) ;
	return	static_cast<LStringBufClass*>(m_pBasicClass[classStringBuf]) ;
}

LArrayClass * LVirtualMachine::GetArrayClass( void ) const
{
	assert( dynamic_cast<LArrayClass*>(m_pBasicClass[classArray]) != nullptr ) ;
	return	static_cast<LArrayClass*>(m_pBasicClass[classArray]) ;
}

LMapClass * LVirtualMachine::GetMapClass( void ) const
{
	assert( dynamic_cast<LMapClass*>(m_pBasicClass[classMap]) != nullptr ) ;
	return	static_cast<LMapClass*>(m_pBasicClass[classMap]) ;
}

LFunctionClass * LVirtualMachine::GetFunctionClass( void ) const
{
	assert( dynamic_cast<LFunctionClass*>(m_pBasicClass[classFunction]) != nullptr ) ;
	return	static_cast<LFunctionClass*>(m_pBasicClass[classFunction]) ;
}

LExceptionClass * LVirtualMachine::GetExceptionClass( void ) const
{
	assert( dynamic_cast<LExceptionClass*>(m_pBasicClass[classException]) != nullptr ) ;
	return	static_cast<LExceptionClass*>(m_pBasicClass[classException]) ;
}

LTaskClass * LVirtualMachine::GetTaskClass( void ) const
{
	assert( dynamic_cast<LTaskClass*>(m_pBasicClass[classTask]) != nullptr ) ;
	return	static_cast<LTaskClass*>(m_pBasicClass[classTask]) ;
}

LThreadClass * LVirtualMachine::GetThreadClass( void ) const
{
	assert( dynamic_cast<LThreadClass*>(m_pBasicClass[classThread]) != nullptr ) ;
	return	static_cast<LThreadClass*>(m_pBasicClass[classThread]) ;
}

// クラス取得（大域空間のみ）
LClass * LVirtualMachine::GetGlobalClassAs( const wchar_t * pwszName ) const
{
	return	m_pGlobal->GetLocalClassAs( pwszName ) ;
}

// クラス取得（. 記号で階層化された名前）
LClass * LVirtualMachine::GetClassPathAs( const wchar_t * pwszName ) const
{
	LPtr<LNamespace>	pNamespace = m_pGlobal ;
	size_t	iLast = 0 ;
	for ( size_t i = 0; pwszName[i] != 0; i ++ )
	{
		if ( pwszName[i] == L'.' )
		{
			LString	strName( pwszName + iLast, i - iLast ) ;
			pNamespace = pNamespace->GetLocalNamespaceAs( strName.c_str() ) ;
			if ( pNamespace == nullptr )
			{
				return	nullptr ;
			}
			iLast = i + 1 ;
		}
	}
	return	pNamespace->GetLocalClassAs( pwszName + iLast ) ;
}

// 配列ジェネリック型取得
LClass * LVirtualMachine::GetArrayClassAs( LClass * pElementType )
{
	if ( (pElementType == nullptr)
		|| (pElementType == m_pBasicClass[classObject]) )
	{
		return	m_pBasicClass[classArray] ;
	}
	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = pElementType->m_pGenArrayType ;
	if ( pClass == nullptr )
	{
		LString	strFullElementTypeName = pElementType->GetFullClassName() ;
		LString	strGenClassName = strFullElementTypeName + L"[]" ;
		pClass = m_pGlobal->GetLocalClassAs( strGenClassName.c_str() ) ;
		if ( pClass == nullptr )
		{
			pClass = new LArrayClass
						( *this, m_pBasicClass[classClass],
							strGenClassName.c_str(), pElementType ) ;
			pElementType->m_pGenArrayType = pClass ;

			if ( !m_pGlobal->AddClassAs
					( strGenClassName.c_str(), LPtr<LClass>( pClass ) ) )
			{
				LCompiler::Error( errorCannotAddClass, strGenClassName.c_str() ) ;
			}
			pClass->AddSuperClass( m_pBasicClass[classArray] ) ;
			pClass->ImplementClass() ;
			pClass->CompleteClass() ;
			pClass->m_flagGenInstance = true ;
		}
		pElementType->m_pGenArrayType = pClass ;
	}
	return	pClass ;
}

// 辞書配列ジェネリック型取得
LClass * LVirtualMachine::GetMapClassAs( LClass * pElementType )
{
	if ( (pElementType == nullptr)
		|| (pElementType == m_pBasicClass[classObject]) )
	{
		return	m_pBasicClass[classMap] ;
	}
	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = pElementType->m_pGenMapType ;
	if ( pClass == nullptr )
	{
		LString	strFullElementTypeName = pElementType->GetFullClassName() ;
		LString	strGenClassName = L"Map<" + strFullElementTypeName + L">" ;
		pClass = m_pGlobal->GetLocalClassAs( strGenClassName.c_str() ) ;
		if ( pClass == nullptr )
		{
			pClass = new LMapClass
						( *this, m_pBasicClass[classClass],
							strGenClassName.c_str(), pElementType ) ;
			pElementType->m_pGenMapType = pClass ;

			if ( !m_pGlobal->AddClassAs
				( strGenClassName.c_str(), LPtr<LClass>( pClass ) ) )
			{
				LCompiler::Error( errorCannotAddClass, strGenClassName.c_str() ) ;
			}
			pClass->AddSuperClass( m_pBasicClass[classMap] ) ;
			pClass->ImplementClass() ;
			pClass->CompleteClass() ;
			pClass->m_flagGenInstance = true ;
		}
		pElementType->m_pGenMapType = pClass ;
	}
	return	pClass ;
}

// データ配列型取得
LClass * LVirtualMachine::GetDataArrayClassAs
		( const LType& typeData, const std::vector<size_t>& dimArray )
{
	assert( typeData.CanArrangeOnBuf() ) ;
	LString		strArrayTypeName = typeData.GetTypeName() ;
	for ( size_t i = 0; i < dimArray.size(); i ++ )
	{
		strArrayTypeName += L"[" ;
		strArrayTypeName += LString::IntegerOf( dimArray.at(i) ) ;
		strArrayTypeName += L"]" ;
	}
	LClass *	pClass = m_pGlobal->GetLocalClassAs( strArrayTypeName.c_str() ) ;
	if ( pClass == nullptr )
	{
		pClass = new LDataArrayClass
					( *this, m_pBasicClass[classClass],
						strArrayTypeName.c_str(), typeData, dimArray ) ;

		if ( !m_pGlobal->AddClassAs
				( strArrayTypeName.c_str(), LPtr<LClass>( pClass ) ) )
		{
			LCompiler::Error( errorCannotAddClass, strArrayTypeName.c_str() ) ;
		}
		pClass->AddSuperClass( m_pBasicClass[classDataArray] ) ;
		pClass->ImplementClass() ;
		pClass->CompleteClass() ;
		pClass->m_flagGenInstance = true ;
	}
	return	pClass ;
}

// ポインタ型取得
LClass * LVirtualMachine::GetPointerClassAs( const LType& typeData )
{
	if ( typeData.IsVoid() )
	{
		return	typeData.IsConst()
					? m_pConstVoidPointerClass : m_pVoidPointerClass ;
	}
	if ( typeData.IsPrimitive() )
	{
		return	typeData.IsConst()
					? m_pPrimitiveCPtrClass[typeData.GetPrimitive()]
					: m_pPrimitivePtrClass[typeData.GetPrimitive()] ;
	}
	LStructureClass *	pStruct = typeData.GetStructureClass() ;
	LDataArrayClass *	pArray = nullptr ;
	if ( pStruct == nullptr )
	{
		pArray = typeData.GetDataArrayClass() ;
		if ( pArray == nullptr )
		{
			return	nullptr ;
		}
	}
	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = nullptr ;
	if ( pStruct != nullptr )
	{
		pClass = typeData.IsConst()
					? pStruct->m_pGenConstPtrType
					: pStruct->m_pGenPointerType ;
	}
	else
	{
		assert( pArray != nullptr ) ;
		pClass = pArray->m_pGenPointerType ;
	}
	if ( pClass == nullptr )
	{
		LString	strPtrTypeName = typeData.GetTypeName() + L"*" ;
		pClass = m_pGlobal->GetLocalClassAs( strPtrTypeName.c_str() ) ;
		if ( pClass == nullptr )
		{
			pClass = new LPointerClass
						( *this, m_pBasicClass[classClass],
							strPtrTypeName.c_str(), typeData ) ;

			if ( !m_pGlobal->AddClassAs
				( strPtrTypeName.c_str(), LPtr<LClass>( pClass ) ) )
			{
				LCompiler::Error( errorCannotAddClass, strPtrTypeName.c_str() ) ;
			}
			pClass->AddSuperClass( m_pBasicClass[classPointer] ) ;
			pClass->ImplementClass() ;
			pClass->CompleteClass() ;
			pClass->m_flagGenInstance = true ;
		}
		if ( pStruct != nullptr )
		{
			if ( typeData.IsConst() )
			{
				pStruct->m_pGenConstPtrType = pClass ;
			}
			else
			{
				pStruct->m_pGenPointerType = pClass ;
			}
		}
		else
		{
			assert( pArray != nullptr ) ;
			pArray->m_pGenPointerType = pClass ;
		}
	}
	return	pClass ;
}

// 例外型取得
LClass * LVirtualMachine::GetExceptioinClassAs( const wchar_t * pwszClassName )
{
	if ( (pwszClassName == nullptr) || (pwszClassName[0] == 0) )
	{
		return	GetExceptionClass() ;
	}
	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = m_pGlobal->GetLocalClassAs( pwszClassName ) ;
	if ( pClass != nullptr )
	{
		return	pClass ;
	}
	pClass = new LExceptionClass
			( *this, m_pGlobal,
				m_pBasicClass[classClass], pwszClassName ) ;

	if ( !m_pGlobal->AddClassAs( pwszClassName, LPtr<LClass>( pClass ) ) )
	{
		LCompiler::Error( errorCannotAddClass, pwszClassName ) ;
	}
	pClass->AddSuperClass( m_pBasicClass[classException] ) ;
	pClass->ImplementClass() ;
	pClass->CompleteClass() ;
	return	pClass ;
}

// 関数型取得
LClass * LVirtualMachine::GetFunctionClassAs
	( std::shared_ptr<LPrototype> pProto, LFunctionClass * pProtoClass )
{
	assert( pProto != nullptr ) ;
	LString	strFuncTypeName = L"Function<" ;
	strFuncTypeName += pProto->TypeToString() ;
	strFuncTypeName += L">" ;

	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = m_pGlobal->GetLocalClassAs( strFuncTypeName.c_str() ) ;
	if ( pClass == nullptr )
	{
		pClass = new LFunctionClass
					( *this, m_pBasicClass[classClass],
						strFuncTypeName.c_str(), pProto ) ;

		if ( !m_pGlobal->AddClassAs
				( strFuncTypeName.c_str(), LPtr<LClass>( pClass ) ) )
		{
			LCompiler::Error( errorCannotAddClass, strFuncTypeName.c_str() ) ;
		}
		if ( pProtoClass != nullptr )
		{
			pClass->AddSuperClass( pProtoClass ) ;
		}
		else
		{
			pClass->AddSuperClass( m_pBasicClass[classFunction] ) ;
		}
		pClass->ImplementClass() ;
		pClass->CompleteClass() ;
		pClass->m_flagGenInstance = true ;
	}
	return	pClass ;
}

// タスク型取得
LClass * LVirtualMachine::GetTaskClassAs( const LType& typeRet )
{
	if ( typeRet.IsVoid() )
	{
		return	m_pBasicClass[classTask] ;
	}
	LString	strClassName = L"Task<" ;
	strClassName += typeRet.GetTypeName() ;
	strClassName += L">" ;

	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = m_pGlobal->GetLocalClassAs( strClassName.c_str() ) ;
	if ( pClass == nullptr )
	{
		pClass = new LTaskClass
					( *this, m_pGlobal, m_pBasicClass[classClass],
						strClassName.c_str(), typeRet ) ;

		if ( !m_pGlobal->AddClassAs
				( strClassName.c_str(), LPtr<LClass>( pClass ) ) )
		{
			LCompiler::Error( errorCannotAddClass, strClassName.c_str() ) ;
		}
		pClass->AddSuperClass( m_pBasicClass[classTask] ) ;
		pClass->ImplementClass() ;
		pClass->CompleteClass() ;
		pClass->m_flagGenInstance = true ;
	}
	return	pClass ;
}

// スレッド型取得
LClass * LVirtualMachine::GetThreadClassAs( const LType& typeRet )
{
	if ( typeRet.IsVoid() )
	{
		return	m_pBasicClass[classThread] ;
	}
	LString	strClassName = L"Thread<" ;
	strClassName += typeRet.GetTypeName() ;
	strClassName += L">" ;

	auto	lock = m_pGlobal->GetLock() ;

	LClass *	pClass = m_pGlobal->GetLocalClassAs( strClassName.c_str() ) ;
	if ( pClass == nullptr )
	{
		pClass = new LThreadClass
					( *this, m_pGlobal, m_pBasicClass[classClass],
						strClassName.c_str(), typeRet ) ;

		if ( !m_pGlobal->AddClassAs
				( strClassName.c_str(), LPtr<LClass>( pClass ) ) )
		{
			LCompiler::Error( errorCannotAddClass, strClassName.c_str() ) ;
		}
		pClass->AddSuperClass( m_pBasicClass[classThread] ) ;
		pClass->ImplementClass() ;
		pClass->CompleteClass() ;
		pClass->m_flagGenInstance = true ;
	}
	return	pClass ;
}

// 式／文を関数としてコンパイルする
LPtr<LFunctionObj>
	LVirtualMachine::CompileAsFunc
		( const wchar_t * pwszExpr,
			LString* pErrMsg, const LType& typeRet )
{
	LString	strExpr = pwszExpr ;
	return	LLoquatyClass::CompileFunc
				( *this, *this, strExpr, pErrMsg, typeRet ) ;
}

// スレッドをリストに追加
void LVirtualMachine::AttachThread( LThreadObj * pThread )
{
	assert( pThread != nullptr ) ;

	std::unique_lock<std::mutex>	lock( m_mutexThreads ) ;

	assert( pThread->m_pListPrev == nullptr ) ;
	assert( pThread->m_pListNext == nullptr ) ;
	pThread->AddRef() ;

	pThread->m_pListPrev = nullptr ;
	pThread->m_pListNext = m_pFirstThread ;

	if ( m_pFirstThread != nullptr )
	{
		m_pFirstThread->m_pListPrev = pThread ;
	}

	m_pFirstThread = pThread ;
}

// スレッドをリストに追加
void LVirtualMachine::DetachThread( LThreadObj * pThread )
{
	assert( pThread != nullptr ) ;

	std::unique_lock<std::mutex>	lock( m_mutexThreads ) ;

	LThreadObj *const	pPrev = pThread->m_pListPrev ;
	LThreadObj *const	pNext  = pThread->m_pListNext ;

	if ( pPrev != nullptr )
	{
		pPrev->m_pListNext = pNext ;
		if ( pNext != nullptr )
		{
			pNext->m_pListPrev = pPrev ;
		}
	}
	else
	{
		assert( m_pFirstThread == pThread ) ;
		m_pFirstThread = pNext ;
		if ( pNext != nullptr )
		{
			pNext->m_pListPrev = nullptr ;
		}
	}
	pThread->m_pListPrev = nullptr ;
	pThread->m_pListNext = nullptr ;

	pThread->ReleaseRef() ;
}

// スレッドリストを取得
void LVirtualMachine::GetThreadList( std::vector< LPtr<LThreadObj> >& listThreads )
{
	std::unique_lock<std::mutex>	lock( m_mutexThreads ) ;

	LThreadObj *	pThread = m_pFirstThread ;

	while ( pThread != nullptr )
	{
		pThread->AddRef() ;
		listThreads.push_back( LPtr<LThreadObj>( pThread ) ) ;

		pThread = pThread->m_pListNext ;
	}
}

// 実行中のスレッドを強制終了する
void LVirtualMachine::TerminateAllThreads( void )
{
	m_mutexThreads.lock() ;
	while ( m_pFirstThread != nullptr )
	{
		LThreadObj *	pThread = m_pFirstThread ;
		pThread->AddRef() ;
		m_mutexThreads.unlock() ;

		pThread->ForceTermination() ;
		pThread->WaitForThread() ;
		pThread->ReleaseRef() ;

		m_mutexThreads.lock() ;
	}
	m_mutexThreads.unlock() ;
}

// デバッガー設定
void LVirtualMachine::AttachDebugger( LDebugger * pDebugger )
{
	m_pDebugger = pDebugger ;
}

// デバッガー取得
LDebugger * LVirtualMachine::GetDebugger( void ) const
{
	return	m_pDebugger ;
}

// ネイティブ関数定義追加
void LVirtualMachine::AddNativeFuncDefinitions( void )
{
	AddNativeFuncDefinitions( s_pnfdFirstDesc ) ;
}

void LVirtualMachine::AddNativeFuncDefinitions( const NativeFuncDesc * pFuncDescs )
{
	auto	lock = m_pGlobal->GetLock() ;

	while ( pFuncDescs != nullptr )
	{
		LString	strFuncName = GetActualFunctionName( pFuncDescs->pwszFuncName ) ;

		AddNativeFuncDefinition( strFuncName.c_str(), pFuncDescs->pfnNativeProc ) ;

		pFuncDescs = pFuncDescs->pnfdNext ;
	}
}

void LVirtualMachine::AddNativeFuncDefinitions( const NativeFuncDeclList * pFuncDeclList )
{
	auto	lock = m_pGlobal->GetLock() ;

	while ( pFuncDeclList != nullptr )
	{
		const NativeFuncDesc *	pnfdDesc = pFuncDeclList->pnfdDesc ;
		LString	strFuncName = GetActualFunctionName( pnfdDesc->pwszFuncName ) ;

		AddNativeFuncDefinition( strFuncName.c_str(), pnfdDesc->pfnNativeProc ) ;

		pFuncDeclList = pFuncDeclList->pNext ;
	}
}

void LVirtualMachine::AddNativeFuncDefinition
	( const wchar_t * pwszFullFuncName, PFN_NativeProc pfnNativeProc )
{
	auto	iFunc = m_mapNativeFuncs.find( pwszFullFuncName ) ;
	if ( iFunc != m_mapNativeFuncs.end() )
	{
		// ネイティブ関数を上書き
		iFunc->second = pfnNativeProc ;
	}
	else
	{
		// ネイティブ関数を登録
		m_mapNativeFuncs.insert
			( std::make_pair<std::wstring,PFN_NativeProc>
				( pwszFullFuncName, (PFN_NativeProc) pfnNativeProc ) ) ;
	}

	auto	iterSolved = m_mapSolvedFuncs.find( pwszFullFuncName ) ;
	if ( iterSolved != m_mapSolvedFuncs.end() )
	{
		for ( LPtr<LFunctionObj> pFunc : iterSolved->second )
		{
			// 関数を設定／上書き
			pFunc->SetNative( pfnNativeProc ) ;
		}
//		m_mapSolvedFuncs.erase( iterSolved ) ;
	}
}

// ネイティブ関数の実装解決
void LVirtualMachine::SolveNativeFunction( LPtr<LFunctionObj> pFunc )
{
	auto	lock = m_pGlobal->GetLock() ;

	LString	strFuncName = pFunc->GetFullName() ;
	if ( pFunc->GetVariationIndex() > 0 )
	{
		strFuncName += L"." ;
		strFuncName += LString::IntegerOf( pFunc->GetVariationIndex() ) ;
	}

	auto	iFunc = m_mapNativeFuncs.find( strFuncName.c_str() ) ;
	if ( iFunc != m_mapNativeFuncs.end() )
	{
		// 登録済みのネイティブ関数
		pFunc->SetNative( iFunc->second ) ;
	}

	auto	iterSolved = m_mapSolvedFuncs.find( strFuncName.c_str() ) ;
	if ( iterSolved != m_mapSolvedFuncs.end() )
	{
		// 解決リストに追加
		iterSolved->second.push_back( pFunc ) ;
	}
	else
	{
		// 解決リストを新規作成
		m_mapSolvedFuncs.insert
			( std::make_pair
				< std::wstring,
					std::vector< LPtr<LFunctionObj> > >
				( strFuncName.c_str(),
					std::vector< LPtr<LFunctionObj> >{ pFunc } ) ) ;
	}
}

// IMPL_LOQUATY_FUNC マクロで定義した関数名を実際の関数名に変換する
// ※ '_' を '.' に置き換え。
//    但し先頭が '_' の場合は '__' を '.' に置き換え '_' はそのまま
LString LVirtualMachine::GetActualFunctionName( const wchar_t * pwszFuncName )
{
	assert( pwszFuncName != nullptr ) ;
	LString	strFuncName ;
	if ( pwszFuncName[0] == L'_' )
	{
		strFuncName = pwszFuncName + 1 ;
		strFuncName = strFuncName.Replace( L"__", L"." ) ;
	}
	else
	{
		strFuncName = pwszFuncName ;
		strFuncName = strFuncName.Replace( L"_", L"." ) ;
	}
	ssize_t	iOperator = strFuncName.Find( L"operator." ) ;
	if ( iOperator >= 0 )
	{
		LString	strOp = strFuncName.Right
							( strFuncName.GetLength()
									- ((size_t) iOperator + 9) ) ;
		LString	strVarNum ;
		ssize_t	iVarPt = strOp.Find( L"." ) ;
		if ( iVarPt >= 0 )
		{
			strVarNum = strOp.Middle( (size_t) iVarPt, strOp.GetLength() - iVarPt ) ;
			strOp = strOp.Left( (size_t) iVarPt ) ;
		}
		for ( size_t i = 0; i < Symbol::opOperatorCount; i ++ )
		{
			if ( (Symbol::s_OperatorDescs[i].pwszFuncName != nullptr)
				&& (strOp == Symbol::s_OperatorDescs[i].pwszFuncName) )
			{
				strFuncName = strFuncName.Left( (size_t) iOperator ) ;
				strFuncName += L"operator " ;
				strFuncName += Symbol::s_OperatorDescs[i].pwszName ;
				if ( Symbol::s_OperatorDescs[i].pwszPairName != nullptr )
				{
					strFuncName += Symbol::s_OperatorDescs[i].pwszPairName ;
				}
				strFuncName += strVarNum ;
				break ;
			}
		}
	}
	return	strFuncName ;
}

// s_pnfdFirstDesc チェーンに pnfdDesc を追加（挿入）
const NativeFuncDesc *
	LVirtualMachine::AddNativeFuncDesc( const NativeFuncDesc * pnfdDesc )
{
	const NativeFuncDesc *	pnfdNext = s_pnfdFirstDesc ;
	s_pnfdFirstDesc = pnfdDesc ;
	return	pnfdNext ;
}

const NativeFuncDesc * LVirtualMachine::s_pnfdFirstDesc = nullptr ;


// 複製する（要素も全て複製処理する）
LObject * LVirtualMachine::CloneObject( void ) const
{
	return	new LVirtualMachine ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LVirtualMachine::DuplicateObject( void ) const
{
	return	new LVirtualMachine ;
}


