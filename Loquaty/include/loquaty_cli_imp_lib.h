

#if	defined(_WIN64) || defined(WIN64)
	#if defined(NDEBUG) || !defined(_DEBUG)
		#ifdef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win64\\loquaty_exp.lib" )
		#endif
	#else
		#ifdef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win64\\loquaty_exp_db.lib" )
		#endif
	#endif
#elif	defined(_WIN32) || defined(WIN32)
	#if defined(NDEBUG) || !defined(_DEBUG)
		#ifdef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win32\\loquaty_exp.lib" )
		#endif
	#else
		#ifdef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win32\\loquaty_exp_db.lib" )
		#endif
	#endif
#endif


