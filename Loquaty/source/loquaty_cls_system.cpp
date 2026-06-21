
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// System クラス
//////////////////////////////////////////////////////////////////////////////

// 構築
LSystemClass::LSystemClass
	( LVirtualMachine& vm,
		LClass * pClass, const wchar_t * pwszName )
	: LClass( vm, vm.Global(), pClass, pwszName ),
		m_logicalProcessors( (size_t) std::thread::hardware_concurrency() ),
		m_moduleFilePath( GetModuleFilePath() )
{
}

// クラス定義処理（ネイティブな実装）
void LSystemClass::ImplementClass( void )
{
	LValue	valIsWindows( LType::typeBoolean, LValue::MakeBool(m_isWindows) ) ;
	LValue	valIsAndroid( LType::typeBoolean, LValue::MakeBool(m_isAndroid) ) ;
	LValue	valArchBits( LType::typeInt32, LValue::MakeLong(m_archBits) ) ;
	LValue	valLogicalProcessors
					( LType::typeUint32, LValue::MakeLong(m_logicalProcessors) ) ;
	LValue	valModuleFilePath
					( LObjPtr( new LStringObj
						( m_vm.GetStringClass(), m_moduleFilePath ) ) ) ;

	VariableDesc	varDescs[] =
	{
		{
			L"isWindows", L"public static const", L"boolean",
			nullptr, &valIsWindows, L"Windows で実行中の時 true",
		},
		{
			L"isAndroid", L"public static const", L"boolean",
			nullptr, &valIsAndroid, L"Android で実行中の時 true",
		},
		{
			L"archBits", L"public static const", L"int",
			nullptr, &valArchBits,
			L"実行バイナリ・アーキテクチャ・ビット数\n"
			L"※64 ビット CPU、64 ビット OS でも 32 ビットアプリケーションで実行中の場合 32",
		},
		{
			L"logicalProcessors", L"public static const", L"uint",
			nullptr, &valLogicalProcessors, L"実行可能な論理プロセッサ数",
		},
		{
			L"moduleFilePath", L"public static const", L"String",
			nullptr, &valModuleFilePath,
			L"<hide_value/><summary>実行しているモジュールファイルパス</summary>",
		},
		{
			nullptr, nullptr, nullptr, nullptr, nullptr,
		}
	} ;

	AddClassMemberDefinitions( s_MemberDesc ) ;
	AddVariableDefinitions( varDescs ) ;
}

// 実行しているモジュールファイルパスを取得
LString LSystemClass::GetModuleFilePath( void )
{
	LString	strFilePath ;
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	strFilePath.resize( 0x1000 ) ;
	DWORD	dwLength =
		::GetModuleFileNameW
			( ::GetModuleHandle( nullptr ),
				strFilePath.data(), (DWORD) strFilePath.size() ) ;
	strFilePath.resize( dwLength ) ;

#else
	Dl_info	dl_info ;
	if ( dladdr( (void*) &LSystemClass::GetModuleFilePath, &dl_info ) != 0 )
	{
		strFilePath.FromString( std::string( dl_info.dli_fname ) ) ;
	}
#endif
	return	strFilePath ;
}

const LClass::NativeFuncDesc	LSystemClass::s_Functions[2] =
{
	{	// public static void trace( String text )
		L"trace",
		&LSystemClass::method_trace, false,
		L"public static", L"void", L"String text",
		L"デバッグ用に文字列を出力します。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::ClassMemberDesc	LSystemClass::s_MemberDesc =
{
	nullptr,
	LSystemClass::s_Functions,
	nullptr,
	nullptr,
	L"システムクラス"
} ;

// public static void trace( String text )
void LSystemClass::method_trace( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( text ) ;

	#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
		::OutputDebugStringW( text.c_str() ) ;

	#elif	defined(__ANDROID__)
		__android_log_print
			( ANDROID_LOG_DEBUG, "Loquaty", "%s", text.ToString().c_str() ) ;

	#endif

	LQT_RETURN_VOID() ;
}


