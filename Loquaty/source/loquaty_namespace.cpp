
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 名前空間
//////////////////////////////////////////////////////////////////////////

// ジェネリック型定義情報
LNamespace::GenericDef::GenericDef( void )
	: m_kind( Symbol::rwiNamespace ),
		m_srcFirst( 0 ), m_srcEnd( SIZE_MAX )
{
}

LNamespace::GenericDef::GenericDef( const GenericDef& gendef )
	: m_parent( gendef.m_parent ),
		m_params( gendef.m_params ),
		m_name( gendef.m_name ),
		m_kind( gendef.m_kind ),
		m_source( gendef.m_source ),
		m_srcFirst( gendef.m_srcFirst ),
		m_srcEnd( gendef.m_srcEnd )
{
}

// インスタンス化する
LPtr<LNamespace> LNamespace::GenericDef::Instantiate
	( LCompiler& compiler, const std::vector<LType>& arg ) const
{
	// インスタンス化した型名
	LString	strGenTypeName = m_name ;
	strGenTypeName += L"<" ;
	for ( size_t i = 0; i < arg.size(); i ++ )
	{
		if ( i > 0 )
		{
			strGenTypeName += L"," ;
		}
		strGenTypeName += arg.at(i).GetTypeName() ;
	}
	strGenTypeName += L">" ;

	// 定義済み判定
	assert( m_parent != nullptr ) ;
	LClass *	pGenClass = m_parent->GetClassAs( strGenTypeName.c_str() ) ;
	if ( pGenClass != nullptr )
	{
		return	pGenClass ;
	}

	if ( arg.size() != m_params.size() )
	{
		compiler.OnError( errorMismatchGenericTypeArgment ) ;
		return	nullptr ;
	}

	// インスタンス生成
	LPtr<LNamespace>	pInstance ;
	switch ( m_kind )
	{
	case	Symbol::rwiClass:
	default:
		pInstance = new LGenericObjClass
				( compiler.VM(), m_parent,
					compiler.VM().GetClassClass(), strGenTypeName.c_str() ) ;
		break ;

	case	Symbol::rwiStruct:
		pInstance = new LStructureClass
				( compiler.VM(), m_parent,
					compiler.VM().GetClassClass(), strGenTypeName.c_str() ) ;
		break ;

	case	Symbol::rwiNamespace:
		pInstance = new LNamespace
				( compiler.VM(), m_parent,
					compiler.VM().GetNamespaceClass(), strGenTypeName.c_str() ) ;
		break ;
	}
	m_parent->AddNamespace( strGenTypeName.c_str(), pInstance ) ;

	// 引数を設定
	for ( size_t i = 0; i < arg.size(); i ++ )
	{
		pInstance->DefineTypeAs( m_params.at(i).c_str(), arg.at(i) ) ;
	}

	// 実装
	LCompiler::ContextPtr	ctx = compiler.BeginNamespaceBlock( m_parent ) ;
	{
		LStringParser&				sparsSrc = *m_source ;
		LStringParser::StateSaver	saver( sparsSrc ) ;	

		sparsSrc.SetBounds( m_srcFirst, m_srcEnd ) ;
		sparsSrc.SeekIndex( m_srcFirst ) ;

		LNamespaceList	nsl ;
		nsl.AddNamespace( pInstance.Ptr() ) ;
		nsl.AddNamespace( m_parent.Ptr() ) ;

		compiler.ParseImplementationClass( sparsSrc, pInstance, m_kind, &nsl ) ;
	}
	compiler.EndNamespaceBlock( ctx ) ;

	LClass *	pClass = dynamic_cast<LClass*>( pInstance.Ptr() ) ;
	if ( pClass != nullptr )
	{
		pClass->SetGenericInstanceFlag() ;
	}

	return	pInstance ;
}


// 名前空間
LNamespace::LNamespace
	( LVirtualMachine& vm,
		LPtr<LNamespace> pParent,
		LClass * pClass, const wchar_t * pwszName )
	: LGenericObj( pClass ),
		m_vm( vm ), m_parent( pParent ),
		m_name( pwszName ),
		m_arrangement( 1 )
{
}

LNamespace::LNamespace( const LNamespace& ns )
	: LGenericObj( ns ),
		m_vm( ns.m_vm ),
		m_parent( ns.m_parent ),
		m_name( ns.m_name ),
		m_arrangement( ns.m_arrangement ),
		m_funcs( ns.m_funcs ),
		m_namespaces( ns.m_namespaces ),
		m_typedefs( ns.m_typedefs ),
		m_generics( ns.m_generics ),
		m_comments( ns.m_comments ),
		m_selfComment( ns.m_selfComment )
{
}

// 名前取得
LString LNamespace::GetFullName( void ) const
{
	if ( m_parent == nullptr )
	{
		return	m_name ;
	}
	LString	strParentName = m_parent->GetFullName() ;
	if ( strParentName.IsEmpty() )
	{
		return	m_name ;
	}
	if ( m_name.IsEmpty() )
	{
		return	strParentName ;
	}
	return	strParentName + L"." + m_name ;
}

// 文字列として評価
bool LNamespace::AsString( LString& str ) const
{
	str = GetFullName() ;
	return	true ;
}

// 複製する（要素も全て複製処理する）
LObject * LNamespace::CloneObject( void ) const
{
	// ※実体の複製が必要なシーンはほぼないし面倒なので参照のコピー
	return	new LNamespace( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LNamespace::DuplicateObject( void ) const
{
	return	new LNamespace( *this ) ;
}

// 内部リソースを解放する
void LNamespace::DisposeObject( void )
{
	DisposeAllObjects() ;
}

// 全要素の複製（複製はせず参照の複製）
void LNamespace::DuplicateFrom( const LNamespace& nspace )
{
	LGenericObj::DuplicateFrom( nspace ) ;

	m_arrangement = nspace.m_arrangement ;

	m_funcs = nspace.m_funcs ;

	m_namespaces = nspace.m_namespaces ;
}

// ローカルクラス・名前空間以外の全ての要素を解放
void LNamespace::DisposeAllObjects( void )
{
	LGenericObj::DisposeObject() ;

	for ( auto iter : m_namespaces )
	{
		assert( iter.second != nullptr ) ;
		iter.second->DisposeObject() ;
	}

	m_parent = nullptr ;
	m_arrangement.ClearAll();
	m_funcs.clear() ;
	m_typedefs.clear() ;
	m_generics.clear() ;
	m_comments.clear() ;
	m_selfComment = nullptr ;
}

// 全ての要素を解放
void LNamespace::ReleaseAll( void )
{
	DisposeAllObjects() ;
	m_namespaces.clear() ;
}

// ローカル変数を定義
bool LNamespace::DeclareObjectAs
	( const wchar_t * pwszName, const LType& type, LObjPtr pObj )
{
	ssize_t	iElement = FindElementAs( pwszName ) ;
	if ( iElement >= 0 )
	{
		LCompiler::Error( errorDoubleDefinition, pwszName ) ;
		return	false ;
	}
	LObject::ReleaseRef
		( LGenericObj::SetElementAs( pwszName, pObj.Get() ) ) ;
	SetElementTypeAs( pwszName, type ) ;
	return	true ;
}

// ローカル変数の型を取得
const LType * LNamespace::GetLocalObjectTypeAs( const wchar_t * pwszName )
{
	ssize_t	iElement = FindElementAs( pwszName ) ;
	if ( (iElement < 0)
		|| ((size_t) iElement >= LGenericObj::m_types.size()) )
	{
		return	nullptr ;
	}
	return	&(LGenericObj::m_types.at(iElement)) ;
}

// ローカル変数を取得
LObjPtr LNamespace::GetLocalObjectAs( const wchar_t * pwszName )
{
	return	LGenericObj::GetElementAs( pwszName ) ;
}

// ローカルクラスを追加
bool LNamespace::AddClassAs( const wchar_t * pwszName, LPtr<LClass> pClass )
{
	if ( GetLocalNamespaceAs( pwszName ) != nullptr )
	{
		LCompiler::Error( errorDoubleDefinitionOfClass, pwszName ) ;
		return	false ;
	}
	return	AddNamespace( pwszName, LPtr<LNamespace>( pClass.Get() ) ) ;
}

// ローカルクラスを取得
// （※返却されたポインタを ReleaseRef してはならない）
LClass * LNamespace::GetLocalClassAs( const wchar_t * pwszName )
{
	LPtr<LNamespace>	pNamespace = GetLocalNamespaceAs( pwszName ) ;
	if ( pNamespace != nullptr )
	{
		return	dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
	}
	return	nullptr ;
}

// クラスを取得（親の名前空間も探す）
// （※返却されたポインタを ReleaseRef してはならない）
LClass * LNamespace::GetClassAs( const wchar_t * pwszName )
{
	LClass *	pClass = GetLocalClassAs( pwszName ) ;
	if ( (pClass == nullptr) && (m_parent != nullptr) )
	{
		pClass = m_parent->GetClassAs( pwszName ) ;
	}
	return	pClass ;
}

// 静的なローカル関数を追加
// （返り値はオーバーロードされた以前の関数）
LPtr<LFunctionObj>
	LNamespace::AddStaticFunctionAs
		( const wchar_t * pwszName, LPtr<LFunctionObj> pFunc )
{
	auto	lock = GetLock() ;

	pFunc->SetParentNamespace( this ) ;
	pFunc->SetName( pwszName ) ;

	auto	iter = m_funcs.find( pwszName ) ;
	if ( iter != m_funcs.end() )
	{
		return	iter->second.OverloadFunction( pFunc.Get() ) ;
	}
	else
	{
		LFunctionVariation	funcvar ;
		LPtr<LFunctionObj>	pNull = funcvar.OverloadFunction( pFunc.Get() ) ;
		m_funcs.insert
			( std::make_pair<std::wstring,LFunctionVariation>
					( pwszName, LFunctionVariation(funcvar) ) ) ;
		return	pNull ;
	}
}

// 静的なローカル関数群を取得
const LFunctionVariation *
			LNamespace::GetLocalStaticFunctionsAs( const wchar_t * pwszName )
{
	auto	lock = GetLock() ;
	auto	iter = m_funcs.find( pwszName ) ;
	if ( iter != m_funcs.end() )
	{
		return	&(iter->second) ;
	}
	return	nullptr ;
}

// 静的なローカル関数を取得
// （※返却されたポインタを ReleaseRef してはならない）
LPtr<LFunctionObj> LNamespace::GetLocalStaticCallableFunctionAs
	( const wchar_t * pwszName, const LArgumentListType& argListType )
{
	auto	lock = GetLock() ;
	auto	iter = m_funcs.find( pwszName ) ;
	if ( iter != m_funcs.end() )
	{
		LPtr<LFunctionObj>	pFunc = iter->second.GetCallableFunction( argListType ) ;
		if ( pFunc != nullptr )
		{
			return	pFunc ;
		}
	}
	return	nullptr ;
}

// 静的なローカル関数配列
const std::map<std::wstring,LFunctionVariation>&
				LNamespace::GetStaticFunctionList( void ) const
{
	return	m_funcs ;
}

// 名前空間を追加
bool LNamespace::AddNamespace
	( const wchar_t * pwszName, LPtr<LNamespace> pNamespace )
{
	auto	lock = GetLock() ;
	auto	iter = m_namespaces.find( pwszName ) ;
	if ( iter != m_namespaces.end() )
	{
		return	false ;
	}
	LPackage *	pPackage = LPackage::GetCurrent() ;
	if ( pPackage != nullptr )
	{
		LClass *	pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
		if ( pClass != nullptr )
		{
			pPackage->AddClass( pClass ) ;
			pClass->SetPackage( pPackage ) ;
		}
	}
	m_namespaces.insert
		( std::make_pair< std::wstring, LPtr<LNamespace> >
			( pwszName, LPtr<LNamespace>( pNamespace ) ) ) ;

	AddRef() ;
	pNamespace->m_parent = this ;
	return	true ;
}

// 名前空間を取得
LPtr<LNamespace> LNamespace::GetLocalNamespaceAs( const wchar_t * pwszName )
{
	auto	lock = GetLock() ;
	auto	iter = m_namespaces.find( pwszName ) ;
	if ( iter != m_namespaces.end() )
	{
		return	iter->second ;
	}
	return	nullptr ;
}

// 名前空間を取得（親の名前空間も探す）
LPtr<LNamespace> LNamespace::GetNamespaceAs( const wchar_t * pwszName )
{
	LPtr<LNamespace>	pNamespace = GetLocalNamespaceAs( pwszName ) ;
	if ( (pNamespace == nullptr) && (m_parent != nullptr) )
	{
		pNamespace = m_parent->GetNamespaceAs( pwszName ) ;
	}
	return	pNamespace ;
}

// 名前空間配列
const std::map< std::wstring, LPtr<LNamespace> >&
				LNamespace::GetNamespaceList( void ) const
{
	return	m_namespaces ;
}

// 型定義を追加
bool LNamespace::DefineTypeAs
	( const wchar_t * pwszName, const LType& type )
{
	auto	lock = GetLock() ;
	auto	iter = m_typedefs.find( pwszName ) ;
	if ( iter != m_typedefs.end() )
	{
		return	false ;
	}
	m_typedefs.insert
		( std::make_pair<std::wstring,LType>( pwszName, LType(type) ) ) ;
	return	true ;
}

// 定義された型を取得
const LType * LNamespace::GetTypeAs( const wchar_t * pwszName )
{
	auto	lock = GetLock() ;
	auto	iter = m_typedefs.find( pwszName ) ;
	if ( iter != m_typedefs.end() )
	{
		return	&(iter->second) ;
	}
	return	nullptr ;
}

// ジェネリック型定義を追加
bool LNamespace::DefineGenericType
	( const wchar_t * pwszName, const LNamespace::GenericDef& gendef )
{
	auto	lock = GetLock() ;
	auto	iter = m_generics.find( pwszName ) ;
	if ( iter != m_generics.end() )
	{
		return	false ;
	}
	m_generics.insert
		( std::make_pair<std::wstring,GenericDef>
						( pwszName, GenericDef(gendef) ) ) ;
	return	true ;
}

// ジェネリック型情報を取得
const LNamespace::GenericDef *
	LNamespace::GetGenericTypeAs( const wchar_t * pwszName )
{
	auto	lock = GetLock() ;
	auto	iter = m_generics.find( pwszName ) ;
	if ( iter != m_generics.end() )
	{
		return	&(iter->second) ;
	}
	return	nullptr ;
}

// コメントデータ作成
LType::LComment * LNamespace::MakeComment( const wchar_t * pwszComment )
{
	if ( (pwszComment == nullptr)
		|| (pwszComment[0] == 0) )
	{
		return	nullptr ;
	}
	std::shared_ptr<LType::LComment>
			pComment = std::make_shared<LType::LComment>( pwszComment ) ;
	m_comments.push_back( pComment ) ;
	return	pComment.get() ;
}

void LNamespace::SetSelfComment( const wchar_t * pwszComment )
{
	m_selfComment = std::make_shared<LType::LComment>( pwszComment ) ;
}




//////////////////////////////////////////////////////////////////////////////
// 名前空間リスト
//////////////////////////////////////////////////////////////////////////////

LNamespaceList::LNamespaceList( const LNamespaceList& nsl )
	: std::vector<LNamespace*>( nsl )
{
}

const LNamespaceList& LNamespaceList::operator = ( const LNamespaceList& nsl )
{
	std::vector<LNamespace*>::operator = ( nsl ) ;
	return	*this ;
}

const LNamespaceList& LNamespaceList::operator += ( const LNamespaceList& nsl )
{
	for ( auto pns : nsl )
	{
		if ( pns != nullptr )
		{
			AddNamespace( pns ) ;
		}
	}
	return	*this ;
}

// 名前空間を取得（親の名前空間も探す）
LPtr<LNamespace> LNamespaceList::GetNamespaceAs( const wchar_t * pwszName ) const
{
	for ( auto pns : *this )
	{
		assert( pns != nullptr ) ;
		if ( pns != nullptr )
		{
			LPtr<LNamespace>	pNamespace = pns->GetNamespaceAs( pwszName ) ;
			if ( pNamespace != nullptr )
			{
				return	pNamespace ;
			}
		}
	}
	return	nullptr ;
}

// クラスを取得（親の名前空間も探す）
LClass * LNamespaceList::GetClassAs( const wchar_t * pwszName ) const
{
	for ( auto pns : *this )
	{
		assert( pns != nullptr ) ;
		if ( pns != nullptr )
		{
			LClass *	pClass = pns->GetClassAs( pwszName ) ;
			if ( pClass != nullptr )
			{
				return	pClass ;
			}
		}
	}
	return	nullptr ;
}

// 型を取得（親の名前空間も探す）
const LType * LNamespaceList::GetTypeAs( const wchar_t * pwszName ) const
{
	for ( auto pns : *this )
	{
		assert( pns != nullptr ) ;
		if ( pns != nullptr )
		{
			const LType *	pType = pns->GetTypeAs( pwszName ) ;
			if ( pType != nullptr )
			{
				return	pType ;
			}
		}
	}
	return	nullptr ;
}

// ジェネリック型情報を取得（親の名前空間も探す）
const LNamespace::GenericDef *
			LNamespaceList::GetGenericTypeAs( const wchar_t * pwszName ) const
{
	for ( auto pns : *this )
	{
		assert( pns != nullptr ) ;
		if ( pns != nullptr )
		{
			const LNamespace::GenericDef *
				pGenType = pns->GetGenericTypeAs( pwszName ) ;
			if ( pGenType != nullptr )
			{
				return	pGenType ;
			}
		}
	}
	return	nullptr ;
}

// 名前空間追加
void LNamespaceList::AddNamespace( LNamespace * pNamespace )
{
	for ( auto pns : *this )
	{
		if ( pns != pNamespace )
		{
			return ;
		}
	}
	std::vector<LNamespace*>::push_back( pNamespace ) ;
}

void LNamespaceList::AddNamespaceList( const LNamespaceList& list )
{
	for ( auto pns : list )
	{
		AddNamespace( pns ) ;
	}
}



//////////////////////////////////////////////////////////////////////////////
// パッケージ（主にドキュメント生成時のため）
//////////////////////////////////////////////////////////////////////////////

thread_local LPackage *	LPackage::t_pCurrent = nullptr ;


