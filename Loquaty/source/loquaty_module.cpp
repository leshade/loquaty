
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
	LString	strPath = LURLSchemer::SubPath
						( m_strPluginPath.c_str(), pwszName ) ;

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	HMODULE	hModule = ::LoadLibraryW( strPath.c_str() ) ;
	if ( hModule != NULL )
	{
		typedef	LModule* (*PFN_ENTRYPOINT)( void ) ;

		PFN_ENTRYPOINT	pfnEntryPoint =
			(PFN_ENTRYPOINT) GetProcAddress
								( hModule, "LoquatyPluginEntryPoint" ) ;

		if ( pfnEntryPoint != nullptr )
		{
			return	std::shared_ptr<LModule>( pfnEntryPoint() ) ;
		}
	}
#endif
	return	nullptr ;
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


