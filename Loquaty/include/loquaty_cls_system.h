
#ifndef	__LOQUATY_CLS_SYSTEM_H__
#define	__LOQUATY_CLS_SYSTEM_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// System クラス
	//////////////////////////////////////////////////////////////////////////

	class	LSystemClass	: public LClass
	{
	public:
	#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
		const bool		m_isWindows = true ;
	#else
		const bool		m_isWindows = false ;
	#endif
	#if	defined(__ANDROID__)
		const bool		m_isAndroid = true ;
	#else
		const bool		m_isAndroid = false ;
	#endif
	#if	defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) \
						|| defined(__aarch64__) || defined(__ARM_64BIT_STATE)
		const int		m_archBits = 64 ;
	#else
		const int		m_archBits = 32 ;
	#endif
		const size_t	m_logicalProcessors ;

		const LString	m_moduleFilePath ;

	public:
		// 構築
		LSystemClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName ) ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	public:
		// 実行しているモジュールファイルパスを取得
		static LString GetModuleFilePath( void ) ;

	private:
		static const NativeFuncDesc		s_Functions[2] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// public static void trace( String text )
		static void method_trace( LContext& context ) ;
	} ;

}

#endif

