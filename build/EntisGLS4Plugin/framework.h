#pragma once

#include <loquaty.h>
#include <loquaty_lib.h>

#include <sakuraglx/sakuraglx.h>
#include <linkgls4.h>

using namespace Loquaty ;


// この DLL モジュール
class	EntisGLS4Module	: public LModule
{
private:
	bool	m_initialized ;

public:
	// 構築
	EntisGLS4Module( void )
		: m_initialized(false) {}

	// 消滅
	~EntisGLS4Module( void ) ;

	// 仮想マシンにクラス群を追加する
	virtual void ImportTo( LVirtualMachine& vm ) ;

} ;


