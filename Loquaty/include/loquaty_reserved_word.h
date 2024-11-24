
#ifndef	__LOQUATY_RESERVED_WORD_H__
#define	__LOQUATY_RESERVED_WORD_H__	1

namespace	Loquaty
{
	namespace	Symbol
	{
		// 予約語
		enum	ReservedWordIndex
		{
			rwiInvalid	= -1,
			rwiImport, rwiInclude, rwiError, rwiTodo,
			rwiClass, rwiStruct, rwiNamespace, rwiTypeDef,
			rwiFunction, rwiExtends, rwiImplements,
			rwiFor, fwiIn, fwiForever, rwiWhile, rwiDo,
			rwiIf, rwiElse, rwiSwitch, rwiCase, rwiDefault,
			rwiBreak, rwiContinue, rwiGoto,
			rwiTry, rwiCatch, rwiFinally,
			rwiThrow, rwiReturn, rwiWith,
			rwiOperator, rwiSynchronized,
			rwiStatic, rwiAbstract, rwiNative, rwiConst,  rwiFetchAddr,
			rwiPublic, rwiProtected, rwiPrivate,
			rwiOverride, rwiDeprecated,
			rwiAuto, rwiVoid, rwiBoolean,
			rwiByte, rwiUbyte, rwiShort, rwiUshort,
			rwiInt, rwiUint, rwiLong, rwiUlong, rwiFloat, rwiDouble,
			rwiInt8, rwiUint8, rwiInt16, rwiUint16,
			rwiInt32, rwiUint32, rwiInt64, rwiUint64,
			rwiThis, rwiSuper, rwiGlobal, rwiNull, rwiFalse, rwiTrue,
			rwiReservedWordCount,

			rwiFirstAccessModifier	= rwiSynchronized,
			rwiLastAccessModifier	= rwiDeprecated,

			rwiFirstPrimitiveType	= rwiBoolean,
			rwiLastPrimitiveType	= rwiUint64,
		} ;

		struct	ReservedWordDesc
		{
			ReservedWordIndex	rwIndex ;
			const wchar_t *		pwszName ;
			int					mapValue ;
		} ;
		extern const ReservedWordDesc	s_ReservedWordDescs[rwiReservedWordCount] ;

		inline bool IsPrimitiveType( ReservedWordIndex rwIndex )
		{
			return	(rwIndex >= rwiFirstPrimitiveType)
						&& (rwIndex <= rwiLastPrimitiveType) ;
		}

		inline bool IsAccessModifier( ReservedWordIndex rwIndex )
		{
			return	(rwIndex >= rwiFirstAccessModifier)
						&& (rwIndex <= rwiLastAccessModifier) ;
		}
	}

}

#endif

