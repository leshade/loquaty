

#if	defined(_WIN64) || defined(WIN64)
	#if defined(NDEBUG) || !defined(_DEBUG)
		#ifndef	_DLL_IMPORT_LOQUATY
			#if	defined(_DLL)
				#pragma	comment( lib, "win64\\loquaty.lib" )
			#else
				#pragma	comment( lib, "win64\\loquaty_mt.lib" )
			#endif
		#else
			#pragma	comment( lib, "win64\\loquaty_imp.lib" )
		#endif
	#else
		#ifndef	_DLL_IMPORT_LOQUATY
			#if	defined(_DLL)
				#pragma	comment( lib, "win64\\loquaty_db.lib" )
			#else
				#pragma	comment( lib, "win64\\loquaty_mtdb.lib" )
			#endif
		#else
			#pragma	comment( lib, "win64\\loquaty_imp_db.lib" )
		#endif
	#endif
#elif	defined(_WIN32) || defined(WIN32)
	#if defined(NDEBUG) || !defined(_DEBUG)
		#ifndef	_DLL_IMPORT_LOQUATY
			#if	defined(_DLL)
				#pragma	comment( lib, "win32\\loquaty.lib" )
			#else
				#pragma	comment( lib, "win32\\loquaty_mt.lib" )
			#endif
		#else
			#pragma	comment( lib, "win32\\loquaty_imp.lib" )
		#endif
	#else
		#ifndef	_DLL_IMPORT_LOQUATY
			#if	defined(_DLL)
				#pragma	comment( lib, "win32\\loquaty_db.lib" )
			#else
				#pragma	comment( lib, "win32\\loquaty_mtdb.lib" )
			#endif
		#else
			#pragma	comment( lib, "win32\\loquaty_imp_db.lib" )
		#endif
	#endif
#endif


