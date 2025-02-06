// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "pch.h"
#include <loquaty/gls4_loquaty.h>


BOOL APIENTRY DllMain
(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		LPluginExporter::DLLAddModule( std::make_shared<EntisGLS4Module>() ) ;
		break ;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


// 消滅
EntisGLS4Module::~EntisGLS4Module( void )
{
	if ( m_initialized )
	{
		SakuraGL::Finalize() ;
		m_initialized = false ;
	}
}

// 仮想マシンにクラス群を追加する
void EntisGLS4Module::ImportTo( LVirtualMachine& vm )
{
	if ( !m_initialized )
	{
		m_initialized = true ;
		SakuraGL::Initialize() ;
	}

	// EntisGLS4 ネイティブ関数の実装を追加する
	DefineEntisGLS4NativeFunctions( &vm ) ;
}

