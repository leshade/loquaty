﻿
#ifndef	__LOQUATY_ERROR_H__
#define	__LOQUATY_ERROR_H__	1

namespace	Loquaty
{
	// エラーメッセージ
	enum	ErrorMessageIndex
	{
		errorNothing,

		exceptionNullPointer,
		exceptionUnimplemented,
		exceptionNoVirtualVector,
		exceptionIndexOutOfBounds,
		exceptionPointerOutOfBounds,
		exceptionDonotOwnMonitor,
		exceptionElementTypeMismatch,
		exceptionPointerAlignmentMismatch,
		exceptionZeroDivision,
		exceptionClockError,
		exceptionCannotOpenFile,

		errorUserMessage_opt1,
		errorMultipleDerivationClass,
		errorDerivedCirculatingClass,
		errorDerivedFromUndefinedClass,
		errorDerivedCirculatingImplement,
		errorInterfaceHasNoMemberVariable,
		errorCannotAddClass,
		errorCannotDefineVariable,
		errorObjectCannotBePlacedInStruct,
		errorReturnTypeMismatch,
		errorInvalidDimensionSize,
		errorTooHugeDimensionSize,
		errorDoubleDefinition,
		errorDoubleDefinitionOfClass,
		errorDoubleDefinitionOfMember,
		errorDoubleDefinitionOfVar,
		errorDoubleInitialized,
		errorDoubleDefinitionOfLabel,
		errorDoubleDefinitionOfType,
		errorSyntaxError,
		errorSyntaxErrorWithReservedWord,
		errorSyntaxErrorStaticMember,
		errorSyntaxErrorInstanceofRight,
		errorExpressionError,
		errorExpressionErrorRefFunc,
		errorExpressionErrorNoValue,
		errorExpressionTooComplex,
		errorInitExpressionError,
		errorInitExprErrorTooMuchElement,
		errorNoInitExprForAutoVar,
		errorUnavailableAutoType,
		errorNotConstExprInitExpr,
		errorSyntaxErrorInInvalidScope,
		errorInvalidTypeName,
		errorInvalidOperator,
		errorInvalidSizeofObject,
		errorUnoverloadableOperator,
		errorInvalidMember,
		errorInvalidFunction,
		errorInvalidIndirectFunction,
		errorInvalidFunctionLeftExpr,
		errorUndefinedClassName,
		errorUndefinedSymbol,
		errorConnotAsUserSymbol_opt1,
		errorNotExistingThis,
		errorNotExistingSuper,
		errorMismatchParentheses,
		errorMismatchBrackets,
		errorMismatchBraces,
		errorMismatchAngleBrackets,
		errorMismatchSinglQuotation,
		errorMismatchDoubleQuotation,
		errorMismatchToken_opt1_opt2,
		errorNothingExpression,
		errorInvalidArrayLength,
		errorInvalidArrayElementType,
		errorInvalidMapElementType,
		errorMapElementNameSyntaxError,
		errorNothingTypeExpression,
		errorNotGenericType,
		errorNoGenericTypeArgment,
		errorMismatchGenericTypeArgment,
		errorCannotInstantiateGenType,
		errorNothingReturn,
		errorNothingArgmentList,
		errorNoSeparationArgmentList,
		errorNoSeparationArrayList,
		errorNoSeparationMapList,
		errorNoSeparationConditionalExpr,
		errorInvalidExpressionOperator,
		errorNoExpressionOperator_opt1,
		errorNotExpressionOperator_opt1,
		errorCannotConstForArrayElement,
		errorCannotDataArrayToObject,
		errorUnavailableConstExpr,
		errorUnavailablePointerRef,
		errorMissmatchPointerAlignment,
		errorInvalidNumberLiteral,
		errorNotDefinedOperator_opt2,
		errorInvalidForLeftValue_opt1,
		errorCannotConstVariable_opt1,
		errorCannotCallNonConstFunction,
		errorCaptureNameMustBeOneSymbol,
		errorExceptionInConstExpr,
		errorMismatchFunctionArgment,
		errorMismatchFunctionLeft,
		errorNotFoundMatchConstructor,
		errorNotFoundMatchFunction,
		errorNotFoundFunction,
		errorNoArgmentOfFunction,
		errorMultiCandidateFunctions,
		errorNoStatementsOfFunction,
		errorFunctionWithImplementation_opt2,
		errorInvalidStoreLeftExpr,
		errorCannotStoreToConst,
		errorCannotStoreToConstPtr,
		errorCannotSetToConstObject,
		errorCannotUseFetchAddr,
		errorFetchAddrVarMustHaveInitExpr,
		errorCannotFetchAddr,
		errorCannotOperateWithFetchAddr_opt2,
		errorCannotCallWithFetchAddr,
		errorCannotFetchAddrForArg,
		errorCannotFetchAddrToRet,
		errorTooHugeToRefFetchAddr,
		errorCannotNonRefToPointer,
		errorCannotAllocBuffer,
		errorLoadByNonObjectPtr,
		errorNotObjectToRefElement,
		errorNotObjectToSetElement,
		errorUnavailableCast_opt1_opt2,
		errorFailedConstExprToCast_opt1_opt2,
		errorNotFoundLabel,
		errorNotFoundSemicolonEnding,
		errorNotFoundSemicolonEndingAfterReturn,
		errorNotFoundSyntax_opt1,
		errorExtraDescription,
		errorNotFoundModule,
		errorNotFoundSourceFile,
		errorIsNotClassName,
		errorNonDerivableClass,
		errorIsNotStructName,
		errorIsNotNamespace,
		errorCannotCreateAbstractClass,
		errorUnavailableDefineFunc,
		errorUnavailableCodeBlock,
		errorUnavailableInitVarName,
		errorAssertCaseLabelInSwitchBlock,
		errorAssertDefaultInSwitchBlock,
		errorCannotBreakBlock,
		errorCannotContinueBlock,
		errorCannotThrowType,
		errorCannotCatchType,
		errorCannotSynchronizeType,
		errorCannotArrangeObject,
		errorCannotPutDataOnObject,

		errorAssertSingleValue,
		errorAssertValueOnTopOfStack,
		errorAssertNotValueOnStack,
		errorAssertMismatchStack,
		errorAssertPointer,
		errorAssertJumpCode,
		errorMismatchLocalFrameToJump,
		errorUnavailableJumpBlock,
		errorNotExistCodeContext,

		warningNumberRadix8,
		warningNotIntegerForArrayElement,
		warningNotStringForMapElement,
		warningVarIndexWithFetchAddr,
		warningCallWhileFetchingAddr,
		warningInvalidFetchAddr,
		warningImplicitCast_opt1_opt2,
		warningCallDeprecatedFunc,
		warningRefDeprecatedVar,
		warningMissmatchPointerAlignment,
		warningIntendedBlankStatement,
		warningSameNameInDifferentScope,
		warningInvalidModifierForLocalVar,
		warningAllocateBufferOnLocal,
		warningInvalidModifierForFunction,
		warningFunctionNotOverrided,
		warningIgnoredConstModifier,
		warningNotConstExprInitExpr,
		warningStoreMoveAsStoreOperator_opt1,

		errorMessageTotalCount,
		errorExceptionCount	= errorUserMessage_opt1,
	} ;

	namespace	Symbol
	{
		struct	ErrorDesc
		{
			ErrorMessageIndex	index ;
			const wchar_t *		msg ;
		} ;

		extern const ErrorDesc	s_ExceptionDesc[errorExceptionCount] ;
		extern const ErrorDesc	s_ErrorDesc[errorMessageTotalCount] ;
	}

	const wchar_t * GetExceptionClassName( ErrorMessageIndex index ) ;
	const wchar_t * GetErrorMessage( ErrorMessageIndex index ) ;
	ErrorMessageIndex GetErrorIndexOf( const wchar_t * msg ) ;

}


#endif

