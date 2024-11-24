#pragma once

#include <loquaty.h>
#include <loquaty_lib.h>

using namespace Loquaty ;


#include "ExampleWindow.h"


// このサンプル DLL モジュール
class	ExampleModule	: public LModule
{
public:
	// 仮想マシンにクラス群を追加する
	virtual void ImportTo( LVirtualMachine& vm ) ;

} ;


