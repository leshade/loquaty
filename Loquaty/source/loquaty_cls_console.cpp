
#include <loquaty.h>

#include <stdio.h>

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#include <conio.h>

#else
#include <unistd.h>
#include <termios.h>

inline bool _kbhit(void)
{
	fd_set	fds ;
	timeval	tv ;

	FD_ZERO( &fds ) ;
	FD_SET( 0, &fds ) ;

	tv.tv_sec = 0 ;
	tv.tv_usec = 0 ;

	int	r = select( 1, &fds, nullptr, nullptr, &tv ) ;
	return	(r != -1) && FD_ISSET( 0, &fds ) ;
}

inline int _getch(void)
{
	if ( _kbhit() )
	{
		return	getchar() ;
	}
	return	0 ;
}

#endif


using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// コンソール
//////////////////////////////////////////////////////////////////////////////

LConsoleClass::LConsoleClass
	( LVirtualMachine& vm,
		LClass * pClass, const wchar_t * pwszName )
	: LClass( vm, vm.Global(), pClass, pwszName )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	HANDLE	hStd = ::GetStdHandle( STD_OUTPUT_HANDLE ) ;
	DWORD	dwMode = 0 ;
	::GetConsoleMode( hStd, &dwMode ) ;
	m_dwConMode = dwMode ;
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING ;
	::SetConsoleMode( hStd, dwMode ) ;
#else
	tcgetattr( STDIN_FILENO, &m_termiosCooked ) ;

	termios termiosRaw ;
	termiosRaw = m_termiosCooked ;
	cfmakeraw( &termiosRaw ) ;
	tcsetattr( STDIN_FILENO, 0, &termiosRaw ) ;
#endif
}

LConsoleClass::~LConsoleClass( void )
{
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	HANDLE	hStd = ::GetStdHandle( STD_OUTPUT_HANDLE ) ;
	::SetConsoleMode( hStd, m_dwConMode ) ;
#else
	tcsetattr( STDIN_FILENO, 0, &m_termiosCooked ) ;
#endif
}

// クラス定義処理（ネイティブな実装）
void LConsoleClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LConsoleClass::s_Functions[6] =
{
	{	// public static void print( String msg )
		L"print",
		&LConsoleClass::method_print, false,
		L"public static", L"void", L"String msg",
		L"コンソールに文字列を出力します。", nullptr
	},
	{	// public static void putchar( int c )
		L"putchar",
		&LConsoleClass::method_putchar, false,
		L"public static", L"void", L"int c",
		L"コンソールに文字を1文字出力します。文字コードは環境に依存します。", nullptr
	},
	{	// public static boolean kbhit()
		L"kbhit",
		&LConsoleClass::method_kbhit, false,
		L"public static", L"boolean", L"",
		L"キー入力が存在するか調べます。", nullptr
	},
	{	// public static int getch()
		L"getch",
		&LConsoleClass::method_getch, false,
		L"public static", L"int", L"",
		L"キー入力が存在する場合、その文字コードを取得します。\n"
		L"キー入力が存在しない場合はも即座に関数は復帰します。\n"
		L"<return>入力されたーコード。入力が存在しない場合は 0</return>", nullptr
	},
	{	// public static String input()
		L"input",
		&LConsoleClass::method_input, false,
		L"public static", L"String", L"",
		L"コンソールから文字列を１行入力します。\n"
		L"関数は入力を受け取るまで復帰しません。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::ClassMemberDesc	LConsoleClass::s_MemberDesc =
{
	nullptr,
	LConsoleClass::s_Functions,
	nullptr,
	nullptr,
	L"Console クラスはレガシー・スタイルなコンソールへの入出力関数を提供します。\n"
	L"コンソールは仮想ターミナル シーケンスが有効な状態に設定されています。",
} ;

// void print( String msg )
void LConsoleClass::method_print( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_ARG_STRING( msg ) ;

	printf( "%s", msg.ToString().c_str() ) ;

	LQT_RETURN_VOID() ;
}

// void putchar( int c )
void LConsoleClass::method_putchar( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_ARG_INT( c ) ;

	putchar( c ) ;

	LQT_RETURN_VOID() ;
}

// boolean kbhit()
void LConsoleClass::method_kbhit( LContext& _context )
{
	LQT_RETURN_BOOL( _kbhit() ) ;
}

// int getch()
void LConsoleClass::method_getch( LContext& _context )
{
	LQT_RETURN_INT( _getch() ) ;
}

// static String input()
void LConsoleClass::method_input( LContext& _context )
{
	std::shared_ptr<std::string>	pInputBuf = std::make_shared<std::string>() ;
	_context.SetAwaiting
		( [&_context,pInputBuf]( std::int64_t msecTimeout )
		{
			int	ch = _getch() ;
			if ( ch != 0 )
			{
				#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
				putchar( ch ) ;
				#endif

				pInputBuf->push_back( (char) ch ) ;

				if ( (ch == '\n') || (ch == '\r') )
				{
					_context.SetReturnValue
						( LValue( LObjPtr( _context.new_String( *pInputBuf ) ) ) ) ;
					return	true ;
				}
			}
			return	false ;
		} ) ;

	LQT_RETURN_VOID() ;
}

