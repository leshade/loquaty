
#include "pch.h"


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
		LPluginExporter::DLLAddModule( std::make_shared<ExampleModule>() ) ;
		break ;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


// 仮想マシンにクラス群を追加する
void ExampleModule::ImportTo( LVirtualMachine& vm )
{
	// この DLL で定義されたネイティブ関数の実装を追加する
	vm.AddNativeFuncDefinitions() ;

	// @imclude "ExamplePlugin.lqs"
	LCompiler	compiler( vm ) ;
	compiler.IncludeScript( L"ExamplePlugin.lqs" ) ;
}

