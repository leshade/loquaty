
#ifndef	__LOQUATY_CLS_CONSOLE_H__
#define	__LOQUATY_CLS_CONSOLE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// コンソール
	//////////////////////////////////////////////////////////////////////////

	class	LConsoleClass	: public LClass
	{
	public:
	#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
		DWORD	m_dwConMode ;
	#else
		termios	m_termiosCooked ;
	#endif

	public:
		LConsoleClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName ) ;
		virtual ~LConsoleClass( void ) ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Functions[6] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// static void print( String msg )
		static void method_print( LContext& context ) ;
		// static void putchar( int c )
		static void method_putchar( LContext& context ) ;
		// static boolean kbhit()
		static void method_kbhit( LContext& context ) ;
		// static int getch()
		static void method_getch( LContext& context ) ;
		// static String input()
		static void method_input( LContext& context ) ;

	} ;

}

#endif

