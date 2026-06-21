
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// モジュール・プロデューサー
//////////////////////////////////////////////////////////////////////////////

// モジュールを取得する
LModulePtr LModuleProducer::GetModule( const wchar_t * pwszName )
{
	std::unique_lock<std::mutex>	lock( m_mutex ) ;

	auto	iter = m_mapModules.find( pwszName ) ;
	if ( iter != m_mapModules.end() )
	{
		return	iter->second ;
	}
	return	nullptr ;
}

// モジュールを生成／取得する
LModulePtr LModuleProducer::ProduceModule( const wchar_t * pwszName )
{
	std::unique_lock<std::mutex>	lock( m_mutex ) ;

	auto	iter = m_mapModules.find( pwszName ) ;
	if ( iter != m_mapModules.end() )
	{
		return	iter->second ;
	}

	for ( auto prod : m_producers )
	{
		LModulePtr	pModule = prod( pwszName ) ;
		if ( pModule != nullptr )
		{
			m_mapModules.insert
				( std::make_pair<std::wstring,LModulePtr>
							( pwszName, LModulePtr(pModule) ) ) ;
			return	pModule ;
		}
	}
	return	nullptr ;
}

// プロデューサーを追加する
void LModuleProducer::AddProducer( Producer producer )
{
	std::unique_lock<std::mutex>	lock( m_mutex ) ;
	m_producers.push_back( producer ) ;
}

void LModuleProducer::AddProducer( std::shared_ptr<LModuleProducer> producer )
{
	AddProducer( [producer]( const wchar_t * name )
					{ return	producer->ProduceModule( name ) ; } ) ;
}

// ネイティブ関数を取得する
LFunctionObj::NakedNativeFuncPtr
				LModuleProducer::GetFunction( const wchar_t * pwszName )
{
	std::unique_lock<std::mutex>	lock( m_mutex ) ;

	for ( auto iter : m_mapModules )
	{
		LFunctionProducerPtr	producer =
			std::dynamic_pointer_cast<LFunctionProducer>( iter.second ) ;
		if ( producer != nullptr )
		{
			LFunctionObj::NakedNativeFuncPtr
					func = producer->GetFunction( pwszName ) ;
			if ( func != nullptr )
			{
				return	func ;
			}
		}
	}
	return	nullptr ;
}

// 複製
const LModuleProducer&
	LModuleProducer::operator = ( const LModuleProducer& mp )
{
	m_producers = mp.m_producers ;
	m_mapModules = mp.m_mapModules ;
	return	*this ;
}



//////////////////////////////////////////////////////////////////////////////
// プラグイン・モジュール・プロデューサー
//////////////////////////////////////////////////////////////////////////////

// モジュールを生成／取得する
LModulePtr LPluginModuleProducer::ProduceModule( const wchar_t * pwszName )
{
	ModuleHandle	hModule = nullptr ;
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	if ( (pwszName != nullptr) && (pwszName[0] != 0) )
	{
		LString	strPath = LURLSchemer::SubPath
							( m_strPluginPath.c_str(), pwszName ) ;
		hModule = ::LoadLibraryW( strPath.c_str() ) ;
	}
	else
	{
		hModule = ::GetModuleHandle( nullptr ) ;
	}
#else
	if ( (pwszName != nullptr) && (pwszName[0] != 0) )
	{
		LString		strPath = LURLSchemer::SubPath
								( m_strPluginPath.c_str(), pwszName ) ;
		hModule = dlopen( strPath.ToString().c_str(), RTLD_LAZY ) ;

		dlerror() ;
	}
#endif
	if ( hModule != nullptr )
	{
		typedef	LModule* (*PFN_ENTRYPOINT)( void ) ;

	#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
		PFN_ENTRYPOINT	pfnEntryPoint =
			(PFN_ENTRYPOINT) GetProcAddress
								( hModule, "LoquatyPluginEntryPoint" ) ;
	#else
		PFN_ENTRYPOINT	pfnEntryPoint =
			(PFN_ENTRYPOINT) dlsym( hModule, "LoquatyPluginEntryPoint" ) ;
		if ( dlerror() != nullptr )
		{
			pfnEntryPoint = nullptr ;
		}
	#endif

		if ( pfnEntryPoint != nullptr )
		{
			return	std::make_shared<Instance>
						( hModule, std::shared_ptr<LModule>( pfnEntryPoint() ) ) ;
		}
		return	std::make_shared<Instance>( hModule, nullptr ) ;
	}
	return	nullptr ;
}

// Instance 構築
LPluginModuleProducer::Instance::Instance
		( ModuleHandle handle, LModulePtr module )
	: m_handle( handle ), m_module( module )
{
}

// 仮想マシンにクラス群を追加する
void LPluginModuleProducer::Instance::ImportTo( LVirtualMachine& vm )
{
	if ( m_module != nullptr )
	{
		m_module->ImportTo( vm ) ;
	}
}

// ネイティブ関数を取得する
LFunctionObj::NakedNativeFuncPtr
	LPluginModuleProducer::Instance::GetFunction( const wchar_t * pwszName )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	std::string	str ;
	return	reinterpret_cast<LFunctionObj::NakedNativeFuncPtr>
				( ::GetProcAddress
					( m_handle, LString(pwszName).ToString(str).c_str() ) ) ;
#else
	std::string	str ;
	LFunctionObj::NakedNativeFuncPtr
		pfnFuncPtr =
			reinterpret_cast<LFunctionObj::NakedNativeFuncPtr>
				( dlsym( m_handle, LString(pwszName).ToString(str).c_str() ) ) ;
	if ( dlerror() != nullptr )
	{
		return	nullptr ;
	}
	return	pfnFuncPtr ;
#endif
}



//////////////////////////////////////////////////////////////////////////////
// プラグイン・エクスポート・モジュール
//////////////////////////////////////////////////////////////////////////////

#if	defined(_DLL_IMPORT_LOQUATY)

// プラグイン・エントリ・ポイント
extern "C" LOQUATY_DLL_EXPORT_IMPL
LModule* LoquatyPluginEntryPoint( void )
{
	return	new LPluginExporter() ;
}

#endif

// シングルトン（DLL 側用）
LPluginExporter	LPluginExporter::m_exporter ;


// モジュール追加
void LPluginExporter::AddModule( LModulePtr module )
{
	m_modules.push_back( module ) ;
}

// 仮想マシンにクラス群を追加する
void LPluginExporter::ImportTo( LVirtualMachine& vm )
{
	for ( auto module : m_exporter.m_modules )
	{
		module->ImportTo( vm ) ;
	}
}

// DLL エントリポイントの DLL_PROCESS_ATTACH 等で追加しておく
void LPluginExporter::DLLAddModule( LModulePtr module )
{
	m_exporter.AddModule( module ) ;
}


