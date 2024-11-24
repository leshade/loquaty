

#if	defined(_WIN64) || defined(WIN64)
	#if defined(NDEBUG) || !defined(_DEBUG)
		#ifndef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win64\\loquaty.lib" )
		#else
			#pragma	comment( lib, "win64\\loquaty_imp.lib" )
			#pragma	comment( lib, "win64\\loquaty_exp.lib" )
		#endif
	#else
		#ifndef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win64\\loquaty_db.lib" )
		#else
			#pragma	comment( lib, "win64\\loquaty_imp_db.lib" )
			#pragma	comment( lib, "win64\\loquaty_exp_db.lib" )
		#endif
	#endif
#elif	defined(_WIN32) || defined(WIN32)
	#if defined(NDEBUG) || !defined(_DEBUG)
		#ifndef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win32\\loquaty.lib" )
		#else
			#pragma	comment( lib, "win32\\loquaty_imp.lib" )
			#pragma	comment( lib, "win32\\loquaty_exp.lib" )
		#endif
	#else
		#ifndef	_DLL_IMPORT_LOQUATY
			#pragma	comment( lib, "win32\\loquaty_db.lib" )
		#else
			#pragma	comment( lib, "win32\\loquaty_imp_db.lib" )
			#pragma	comment( lib, "win32\\loquaty_exp_db.lib" )
		#endif
	#endif
#endif


