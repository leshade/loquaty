
#ifndef	__LOQUATY_DEFS_H__
#define	__LOQUATY_DEFS_H__	1

#if	defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdarg.h>

#ifdef	_LOQUATY_USES_CPP_FILESYSTEM
	#include <fstream>
	#include <iostream>
	#include <filesystem>
#endif

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
//	#define NOMINMAX
	#include <windows.h>
	#undef	max
	#undef	min
#else
	#include <unistd.h>
	#include <termios.h>
#endif


// ※プラグインを DLL として供給したい場合には
// exe とそれ用の lib をマクロスイッチ(/D) _DLL_EXPORT_LOQUATY を定義してビルドし、
// dll とそれ用の lib をマクロスイッチ(/D) _DLL_IMPORT_LOQUATY を定義してビルドする。
// また C++ のラインタイムライブラリには dll を選択する(/MD, /MDd)。

#if	!defined(_DLL_EXPORT_LOQUATY) && !defined(_DLL_IMPORT_LOQUATY)
#define	_DLL_EXPORT_LOQUATY
#endif


// 共有したい static な変数や関数は LOQUATY_DLL_EXPORT で宣言し、
// LOQUATY_DLL_DECL(x); で定義する。

#if	defined(_DLL_EXPORT_LOQUATY)
	#if	defined(_MSC_VER)
		#define	LOQUATY_DLL_EXPORT	__declspec( dllexport )
		#define	LOQUATY_DLL_EXPORT_IMPL	__declspec( dllexport )
	#else
		#define	LOQUATY_DLL_EXPORT
		#define	LOQUATY_DLL_EXPORT_IMPL
	#endif
	#define	LOQUATY_DLL_DECL(x)	LOQUATY_DLL_EXPORT x

#elif	defined(_DLL_IMPORT_LOQUATY)
	#if	defined(_MSC_VER)
		#define	LOQUATY_DLL_EXPORT		__declspec( dllimport )
		#define	LOQUATY_DLL_EXPORT_IMPL	__declspec( dllexport )
	#else
		#define	LOQUATY_DLL_EXPORT
		#define	LOQUATY_DLL_EXPORT_IMPL
	#endif
	#define	LOQUATY_DLL_DECL(x)

#else
	#define	LOQUATY_DLL_EXPORT
	#define	LOQUATY_DLL_EXPORT_IMPL
	#define	LOQUATY_DLL_DECL(x)	LOQUATY_DLL_EXPORT x
#endif


// ※ビッグエンディアン CPU の場合 _LOQUATY_BIG_ENDIAN を定義してビルドする。
//   但し実動は未確認

// ※ C++ の std::filesystem や std::fstream を利用する場合には
//   _LOQUATY_USES_CPP_FILESYSTEM を定義してビルドする。
// ※そうでない場合には POSIX / Windows のファイル関数を利用する。


namespace	Loquaty
{
	// バージョン
	constexpr int			VersionMajor	= 1 ;
	constexpr int			VersionMinor	= 0 ;
	constexpr const char *	VersionString	= "1.00" ;

	// 型
	typedef	std::size_t	size_t ;
	typedef	ptrdiff_t	ssize_t ;

	// プリミティブ・タイプ
	typedef	bool			LBoolean ;
	typedef	std::int8_t		LByte ;
	typedef	std::uint8_t	LUbyte ;
	typedef	std::int16_t	LShort ;
	typedef	std::uint16_t	LUshort ;
	typedef	std::int32_t	LInt ;
	typedef	std::uint32_t	LUint ;
	typedef	std::int64_t	LLong ;
	typedef	std::uint64_t	LUlong ;
	typedef	std::int8_t		LInt8 ;
	typedef	std::uint8_t	LUint8 ;
	typedef	std::int16_t	LInt16 ;
	typedef	std::uint16_t	LUint16 ;
	typedef	std::int32_t	LInt32 ;
	typedef	std::uint32_t	LUint32 ;
	typedef	std::int64_t	LInt64 ;
	typedef	std::uint64_t	LUint64 ;
	typedef	float			LFloat ;
	typedef	double			LDouble ;

	// 文字列解釈
	class	LString ;
		class	LStringParser ;
			class	LSourceFile ;
			class	LXMLDocParser ;
	class	LXMLDocument ;
	class	LCompiler ;

	// ファイル
	class	LDirectory ;
		class	LDirectories ;
		class	LDirectorySchemer ;
			class	LURLSchemer ;
		class	LCppStdFileDirectory ;
	class	LPureFile ;
		class	LCppStdFile ;
		class	LBufferedFile ;
	class	LOutputStream ;
		class	LOutputUTF8Stream ;

	// 実行要素
	class	LArrangement ;
	class	LArrayBuffer ;
		class	LArrayBufStorage ;
			class	LStackBuffer ;
		class	LArrayBufAlias ;
	class	LCodeBuffer ;
	class	LType ;
	class	LValue ;
		class	LExprValue ;
			class	LLocalVar ;
	class	LPrototype ;
	class	LFunctionVariation ;
	class	LVirtualFuncVector ;
	class	LContext ;
	class	LNamespace ;
	class	LNamespaceList ;
	class	LVirtualMachine ;

	// オブジェクト
	class	Object ;
	class	LObject ;
		class	LClass ;
			class	LClassClass ;
			class	LDataArrayClass ;
			class	LPointerClass ;
			class	LIntegerClass ;
			class	LDoubleClass ;
			class	LStringClass ;
			class	LStringBufClass ;
			class	LArrayClass ;
			class	LMapClass ;
			class	LFunctionClass ;
			class	LGenericObjClass ;
				class	LNativeObjClass ;
				class	LStructureClass ;
				class	LExceptionClass ;
				class	LTaskClass ;
					class	LThreadClass ;
		class	LPointerObj ;
		class	LIntegerObj ;
		class	LDoubleObj ;
		class	LStringObj ;
			class	LStringBufObj ;
		class	LArrayObj ;
		class	LMapObj ;
			class	LGenericObj ;
				class	LExceptionObj ;
				class	LNativeObj ;
				class	LTaskObj ;
					class	LThreadObj ;
		class	LFunctionObj ;


	//////////////////////////////////////////////////////////////////////////
	// Native オブジェクトの基底
	//（Native オブジェクトとして扱いたい class はこのクラスから派生しておく）
	//////////////////////////////////////////////////////////////////////////

	class	Object
	{
	public:
		virtual ~Object( void ) {}

	public:
		// new/delete オーバーロード
		//（主に DLL 側と EXE でアロケーションを統一するため）
		//（※デストラクタが仮想関数であれば問題ないはずではあるが）
		//（但しランライムライブラリを DLL 側と EXE 側で統一する事）
		//（既述通り DLL のランライムライブラリの使用推奨）
		LOQUATY_DLL_EXPORT
		static void * operator new ( size_t bytes ) ;
		LOQUATY_DLL_EXPORT
		static void * operator new ( size_t bytes, void * ptr ) ;
		LOQUATY_DLL_EXPORT
		static void * operator new ( size_t bytes, const char * file, int line ) ;	// ※MFC DEBUG_NEW
		LOQUATY_DLL_EXPORT
		static void operator delete ( void * ptr ) ;
	} ;


	//////////////////////////////////////////////////////////////////////////
	// デバッグ出力
	//////////////////////////////////////////////////////////////////////////

	#if defined(NDEBUG) || !defined(_DEBUG)
		inline void LTrace( const char * pszTrace, ... ) {}
	#else
		inline void LTrace( const char * pszTrace, ... )
		{
			va_list	vl ;
			va_start( vl, pszTrace ) ;

			#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
				char	szBuf[0x400] ;
				_vsnprintf_s( szBuf, 0x400, 0x3FF, pszTrace, vl ) ;
				::OutputDebugString( szBuf ) ;
			#else
				vprintf( pszTrace, vl ) ;
			#endif
		}
	#endif


	//////////////////////////////////////////////////////////////////////////
	// リリース時にも実行したい assert マクロ（手抜き）
	//////////////////////////////////////////////////////////////////////////

	#if defined(NDEBUG) || !defined(_DEBUG)
		#define	LVerify(x)	(x)
	#else
		#define	LVerify(x)	if(!(x)) assert(false)
	#endif



	//////////////////////////////////////////////////////////////////////////
	// 軽量で簡易なミューテックス（スピンロック）
	//////////////////////////////////////////////////////////////////////////

	class	LSpinLockMutex
	{
	private:
		enum	State
		{
			spinUnlocked,
			spinLocked,
		} ;
		std::atomic<State>	m_spinLock ;

	public:
		LSpinLockMutex( void )
			: m_spinLock( spinUnlocked ) {}

		void SpinLock( void )
		{
			while ( m_spinLock.exchange
				( spinLocked, std::memory_order_acquire ) == spinLocked )
			{
			}
		}
		void SpinUnlock( void )
		{
			m_spinLock.store( spinUnlocked, std::memory_order_release ) ;
		}
	} ;

	class	LSpinLock
	{
	private:
		LSpinLockMutex&	m_mutex ;

	public:
		LSpinLock( const LSpinLockMutex& mutex )
			: m_mutex( const_cast<LSpinLockMutex&>( mutex ) )
		{
			m_mutex.SpinLock() ;
		}
		~LSpinLock( void )
		{
			m_mutex.SpinUnlock() ;
		}
	} ;

}

#endif


