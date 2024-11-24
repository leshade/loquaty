﻿
#include <loquaty.h>

using namespace	Loquaty ;

const Symbol::ErrorDesc	Symbol::s_ExceptionDesc[errorExceptionCount] =
{
	{ errorNothing,
		nullptr		},

	{ exceptionNullPointer,
		L"NullPointerException"		},
	{ exceptionUnimplemented,
		L"NullPointerException"		},
	{ exceptionNoVirtualVector,
		L"NullPointerException"		},
	{ exceptionIndexOutOfBounds,
		L"IndexOutOfBoundsException"	},
	{ exceptionPointerOutOfBounds,
		L"PointerOutOfBoundsException"	},
	{ exceptionDonotOwnMonitor,
		L"IllegalMonitorStateException"	},
	{ exceptionElementTypeMismatch,
		L"ClassCastException" },
	{ exceptionPointerAlignmentMismatch,
		L"AlignmentMismatchException" },
	{ exceptionZeroDivision,
		L"ArithmeticException" },
	{ exceptionClockError,
		L"ClockException" },
	{ exceptionCannotOpenFile,
		L"IOException" },
} ;

const Symbol::ErrorDesc	Symbol::s_ErrorDesc[errorMessageTotalCount] =
{
	{ errorNothing,
		L""		},

	{ exceptionNullPointer,
		L"null ポインタを参照しています"		},
	{ exceptionUnimplemented,
		L"関数の実装が存在しません"		},
	{ exceptionNoVirtualVector,
		L"仮想関数ベクタを見つけられません"		},
	{ exceptionIndexOutOfBounds,
		L"指標が範囲外です"		},
	{ exceptionPointerOutOfBounds,
		L"ポインタが有効なバッファの範囲外です"		},
	{ exceptionDonotOwnMonitor,
		L"オブジェクトモニタを所有していません"	},
	{ exceptionElementTypeMismatch,
		L"要素の型が一致しません" },
	{ exceptionPointerAlignmentMismatch,
		L"ポインタのアライメントが不整合です" },
	{ exceptionZeroDivision,
		L"ゼロで除算しています" },
	{ exceptionClockError,
		L"時刻の取得に失敗しました" },
	{ exceptionCannotOpenFile,
		L"ファイルを開けませんでした" },

	{ errorUserMessage_opt1,
		L"%(0)" },
	{ errorMultipleDerivationClass,
		L"クラスの多重派生は出来ません"	},
	{ errorDerivedCirculatingClass,
		L"派生元クラスが循環しています"	},
	{ errorDerivedFromUndefinedClass,
		L"派生元クラスが未定義です"	},
	{ errorDerivedCirculatingImplement,
		L"実装インターフェースが循環しています"	},
	{ errorInterfaceHasNoMemberVariable,
		L"実装インターフェースにメンバ変数が含まれています"	},
	{ errorCannotAddClass,
		L"クラスを定義できませんでした"	},
	{ errorCannotDefineVariable,
		L"変数を定義できません"	},
	{ errorObjectCannotBePlacedInStruct,
		L"構造体にはオブジェクトを配置できません"	},
	{ errorReturnTypeMismatch,
		L"関数の返り値型が一致しません"	},
	{ errorInvalidDimensionSize,
		L"配列長が不正な値です" },
	{ errorTooHugeDimensionSize,
		L"配列長が大きすぎます" },
	{ errorDoubleDefinition,
		L"二重定義です" },
	{ errorDoubleDefinitionOfClass,
		L"クラスの二重定義です" },
	{ errorDoubleDefinitionOfMember,
		L"メンバ変数の名前が衝突しています" },
	{ errorDoubleDefinitionOfVar,
		L"変数の名前が衝突しています" },
	{ errorDoubleInitialized,
		L"二重に初期化しようとしています" },
	{ errorDoubleDefinitionOfLabel,
		L"ラベルの二重定義です" },
	{ errorDoubleDefinitionOfType,
		L"型の二重定義です" },
	{ errorSyntaxError,
		L"構文エラーです" },
	{ errorSyntaxErrorWithReservedWord,
		L"予約語の構文エラーです" },
	{ errorSyntaxErrorStaticMember,
		L"\'::\' 演算子の左辺は名前空間かクラスでなければなりません" },
	{ errorSyntaxErrorInstanceofRight,
		L"\'instanceof\' 演算子の右辺がクラスではありません" },
	{ errorExpressionError,
		L"数式コンパイル中にエラーが発生しました" },
	{ errorExpressionErrorRefFunc,
		L"評価できない関数参照があります" },
	{ errorExpressionErrorNoValue,
		L"評価できない数式が含まれています" },
	{ errorExpressionTooComplex,
		L"数式が複雑すぎて正常な命令を生成できませんでした" },
	{ errorInitExpressionError,
		L"初期値式の構文エラーです" },
	{ errorInitExprErrorTooMuchElement,
		L"初期値式の要素数が多すぎます" },
	{ errorNoInitExprForAutoVar,
		L"auto 変数に初期値式がありません" },
	{ errorUnavailableAutoType,
		L"auto で定義できない型です" },
	{ errorNotConstExprInitExpr,
		L"初期値式が定数式ではありません" },
	{ errorSyntaxErrorInInvalidScope,
		L"記述できないスコープ内です" },
	{ errorInvalidTypeName,
		L"有効な型名ではありません" },
	{ errorInvalidOperator,
		L"有効な演算子ではありません" },
	{ errorInvalidSizeofObject,
		L"オブジェクト型に対する sizeof 演算子" },
	{ errorUnoverloadableOperator,
		L"オーバーロードできない演算子です" },
	{ errorInvalidMember,
		L"有効なメンバではありません" },
	{ errorInvalidFunction,
		L"関数でないものを呼び出そうとしています" },
	{ errorInvalidIndirectFunction,
		L"\'.*\' 演算子の右辺が関数ではありません" },
	{ errorInvalidFunctionLeftExpr,
		L"関数呼び出しの左辺式（this オブジェクト）が不正です" },
	{ errorUndefinedClassName,
		L"未定義のクラス名です" },
	{ errorUndefinedSymbol,
		L"未定義シンボルです" },
	{ errorConnotAsUserSymbol_opt1,
		L"\'%(0)\' はユーザー定義名として使用できません" },
	{ errorNotExistingThis,
		L"this は存在しません" },
	{ errorNotExistingSuper,
		L"super は存在しません" },
	{ errorMismatchParentheses,
		L"\'(\' 括弧が \')\' と対応していません" },
	{ errorMismatchBrackets,
		L"\'[\' 括弧が \']\' と対応していません" },
	{ errorMismatchBraces,
		L"\'{\' 括弧が \'}\' と対応していません" },
	{ errorMismatchAngleBrackets,
		L"\'<\' 括弧が \'>\' と対応していません" },
	{ errorMismatchSinglQuotation,
		L"文字コードリテラルでシングルクォーテーションが閉じられていません" },
	{ errorMismatchDoubleQuotation,
		L"文字列がダブルクォーテーションが閉じられていません" },
	{ errorMismatchToken_opt1_opt2,
		L"\'%(0)\' が \'%(1)\' と対応していません" },
	{ errorNothingExpression,
		L"数式が記述されていないか途切れています" },
	{ errorInvalidArrayLength,
		L"配列要素数が不正です" },
	{ errorInvalidArrayElementType,
		L"Array 配列の要素型に使用できません" },
	{ errorInvalidMapElementType,
		L"Map 配列の要素型に使用できません" },
	{ errorMapElementNameSyntaxError,
		L"Map リテラルの要素名の構文エラーです" },
	{ errorNothingTypeExpression,
		L"型記述がありません" },
	{ errorNotGenericType,
		L"ジェネリック型ではありません" },
	{ errorNoGenericTypeArgment,
		L"ジェネリック型の引数がありません" },
	{ errorMismatchGenericTypeArgment,
		L"ジェネリック型の引数が適合しません" },
	{ errorCannotInstantiateGenType,
		L"ジェネリック型はジェネリック型引数の中でインスタンス化できません" },
	{ errorNothingReturn,
		L"関数に return 文が存在しないパスが存在します" },
	{ errorNothingArgmentList,
		L"関数引数がありません" },
	{ errorNoSeparationArgmentList,
		L"引数が \',\' 記号で分割されていません" },
	{ errorNoSeparationArrayList,
		L"要素リストが \',\' 記号で分割されていません" },
	{ errorNoSeparationMapList,
		L"Map 要素リストが \',\' 記号で分割されていません" },
	{ errorNoSeparationConditionalExpr,
		L"演算子 \'\?\' の右辺式が \':\' 記号で分割されていません" },
	{ errorInvalidExpressionOperator,
		L"演算子の構文エラーです" },
	{ errorNoExpressionOperator_opt1,
		L"\'%(0)\' の前に演算子が記述されていません" },
	{ errorNotExpressionOperator_opt1,
		L"\'%(0)\' は有効な演算子ではありません" },
	{ errorCannotConstForArrayElement,
		L"オブジェクト配列の要素型に const は使用できません" },
	{ errorCannotDataArrayToObject,
		L"データ配列はオブジェクトに変換できません" },
	{ errorUnavailableConstExpr,
		L"定数式として評価できません" },
	{ errorUnavailablePointerRef,
		L"プリミティブでないデータ型へのポインタ参照です" },
	{ errorMissmatchPointerAlignment,
		L"ポインタのアライメントが不整合です" },
	{ errorInvalidNumberLiteral,
		L"数値リテラルの表記が不正です" },
	{ errorNotDefinedOperator_opt2,
		L"\'%(1)\' は定義されていない演算子です" },
	{ errorInvalidForLeftValue_opt1,
		L"\'%(0)\' 演算子は代入可能な変数のみ操作できます" },
	{ errorCannotConstVariable_opt1,
		L"const な変数に対して \'%(0)\' 演算子を使用しています" },
	{ errorCannotCallNonConstFunction,
		L"const なオブジェクトに対して const でないメソッドを呼び出すことはできません" },
	{ errorCaptureNameMustBeOneSymbol,
		L"function 式のキャプチャーオブジェクトは単一シンボルで表現する必要があります" },
	{ errorExceptionInConstExpr,
		L"定数式評価中に例外が発生しました" },
	{ errorMismatchFunctionArgment,
		L"関数の引数が適合しません" },
	{ errorMismatchFunctionLeft,
		L"関数呼び出しの左辺式（this オブジェクト）が適合しません" },
	{ errorNotFoundMatchConstructor,
		L"引数に適合する構築関数が見つかりません" },
	{ errorNotFoundMatchFunction,
		L"引数に適合する関数が見つかりません" },
	{ errorNotFoundFunction,
		L"適合する関数が見つかりません" },
	{ errorNoArgmentOfFunction,
		L"関数の引数がありません" },
	{ errorMultiCandidateFunctions,
		L"関数の候補が複数あります" },
	{ errorNoStatementsOfFunction,
		L"関数の実装がありません" },
	{ errorFunctionWithImplementation_opt2,
		L"%(1) 修飾された関数の実装が記述されています" },
	{ errorInvalidStoreLeftExpr,
		L"代入の左辺式が有効な記憶領域ではありません" },
	{ errorCannotStoreToConst,
		L"const 修飾された記憶領域へ書き込もうとしています" },
	{ errorCannotStoreToConstPtr,
		L"書き換えできないポインタの代入操作です" },
	{ errorCannotSetToConstObject,
		L"const 修飾されたオブジェクトを変更しようとしています" },
	{ errorCannotUseFetchAddr,
		L"fetch_addr ポインタは使用できません" },
	{ errorFetchAddrVarMustHaveInitExpr,
		L"fetch_addr ポインタは初期化式が必要です" },
	{ errorCannotFetchAddr,
		L"アドレスをフェッチすることができない型です" },
	{ errorCannotOperateWithFetchAddr_opt2,
		L"fetch_addr ポインタから構造体の operator %(1) を呼び出すことはできません" },
	{ errorCannotCallWithFetchAddr,
		L"fetch_addr ポインタから構造体のメンバ関数を呼び出すことはできません" },
	{ errorCannotFetchAddrForArg,
		L"fetch_addr ポインタを関数の引数に渡すことはできません" },
	{ errorCannotFetchAddrToRet,
		L"fetch_addr ポインタを関数の返り値にできません" },
	{ errorTooHugeToRefFetchAddr,
		L"fetch_addr ポインタの参照範囲が広すぎます" },
	{ errorCannotNonRefToPointer,
		L"メモリ参照でない式をポインタに変換しようとしています" },
	{ errorCannotAllocBuffer,
		L"ポインタでないオブジェクトにバッファを確保しようとしています" },
	{ errorLoadByNonObjectPtr,
		L"非オブジェクトからメモリをロードしようとしています" },
	{ errorNotObjectToRefElement,
		L"非オブジェクトへの要素参照です" },
	{ errorNotObjectToSetElement,
		L"非オブジェクトを要素に設定しようとしています" },
	{ errorUnavailableCast_opt1_opt2,
		L"\'%(0)\' を \'%(1)\' に変換できません" },
	{ errorFailedConstExprToCast_opt1_opt2,
		L"定数式で \'%(0)\' を \'%(1)\' へ変換できませんでした" },
	{ errorNotFoundLabel,
		L"ラベルが見つかりません" },
	{ errorNotFoundSemicolonEnding,
		L"文末のセミコロン \';\' がありません" },
	{ errorNotFoundSemicolonEndingAfterReturn,
		L"void 関数の return 文の後ろにセミコロン \';\' がありません。" },
	{ errorNotFoundSyntax_opt1,
		L"\'%(0)\' が見つかりません" },
	{ errorExtraDescription,
		L"処理できない余分な記述があります" },
	{ errorNotFoundModule,
		L"モジュールが見つかりません" },
	{ errorNotFoundSourceFile,
		L"ソースファイルが見つかりません" },
	{ errorIsNotClassName,
		L"クラス名ではありません" },
	{ errorNonDerivableClass,
		L"派生できないクラスです" },
	{ errorIsNotStructName,
		L"構造体名ではありません" },
	{ errorIsNotNamespace,
		L"名前空間ではありません" },
	{ errorUnavailableDefineFunc,
		L"ここで関数を定義できません" },
	{ errorUnavailableCodeBlock,
		L"ここに処理手続きを記述できません" },
	{ errorUnavailableInitVarName,
		L"ここで初期化できない名前です" },
	{ errorAssertCaseLabelInSwitchBlock,
		L"case ラベルは switch ブロック内でなければなりません" },
	{ errorAssertDefaultInSwitchBlock,
		L"default ラベルは switch ブロック内でなければなりません" },
	{ errorCannotBreakBlock,
		L"ここで break できません" },
	{ errorCannotContinueBlock,
		L"ここで continue できません" },
	{ errorCannotThrowType,
		L"throw できない型です" },
	{ errorCannotCatchType,
		L"catch できない型です" },
	{ errorCannotSynchronizeType,
		L"synchronized できない型です" },
	{ errorCannotArrangeObject,
		L"バッファにオブジェクトは配置できません" },
	{ errorCannotPutDataOnObject,
		L"オブジェクトに非オブジェクト型は配置できません" },

	{ errorAssertSingleValue,
		L"コンパイラ・エラー : 単一評価値ではありません" },
	{ errorAssertValueOnTopOfStack,
		L"コンパイラ・エラー : 評価値がスタックの末尾ではありません" },
	{ errorAssertNotValueOnStack,
		L"コンパイラ・エラー : 評価値がスタック上に存在しません" },
	{ errorAssertMismatchStack,
		L"コンパイラ・エラー : スタックが不整合です" },
	{ errorAssertPointer,
		L"コンパイラ・エラー : ポインタではありません" },
	{ errorAssertJumpCode,
		L"コンパイラ・エラー : ジャンプ命令が不整合です" },
	{ errorMismatchLocalFrameToJump,
		L"ジャンプ先で初期化されないローカル変数が存在ます" },
	{ errorUnavailableJumpBlock,
		L"ジャンプできないブロックです" },
	{ errorNotExistCodeContext,
		L"コンパイラ・エラー：コード文脈が存在しません" },

	{ warningNumberRadix8,
		L"\'0\' から始まる数値リテラルを8進数として表現する場合、"
		L"\'0o\' プリフィックスが推奨されます。10進数なら \'0t\' を使用してください" },
	{ warningNotIntegerForArrayElement,
		L"配列指標が整数型ではありません" },
	{ warningNotStringForMapElement,
		L"Map 要素指標が String 型ではありません" },
	{ warningVarIndexWithFetchAddr,
		L"fetch_addr ポインタに定数式でないインデックスを使ってメモリを参照しています" },
	{ warningCallWhileFetchingAddr,
		L"fetch_addr 中に関数を呼び出しています" },
	{ warningInvalidFetchAddr,
		L"fetch_addr 修飾は無効です" },
	{ warningImplicitCast_opt1_opt2,
		L"\'%(0)\' を \'%(1)\' を暗黙に変換しています" },
	{ warningCallDeprecatedFunc,
		L"使用が推奨されない関数を呼び出しています" },
	{ warningRefDeprecatedVar,
		L"使用が推奨されない変数を参照しています" },
	{ warningMissmatchPointerAlignment,
		L"ポインタのアライメントが不整合かもしれません" },
	{ warningIntendedBlankStatement,
		L"意図した \';\' ですか？" },
	{ warningSameNameInDifferentScope,
		L"異なるスコープに同じ名前の変数があります" },
	{ warningInvalidModifierForLocalVar,
		L"ローカル変数に無効な修飾子が指定されています" },
	{ warningAllocateBufferOnLocal,
		L"ローカル記憶領域に構造体や配列を確保しています" },
	{ warningInvalidModifierForFunction,
		L"関数には無効な修飾子が指定されています" },
	{ warningFunctionNotOverrided,
		L"@override 指定されていますが、この関数はオーバーライドされません" },
	{ warningIgnoredConstModifier,
		L"const 修飾は無視されます" },
	{ warningNotConstExprInitExpr,
		L"初期値式が定数式ではありません (実行順序に注意が必要です)" },
	{ warningStoreMoveAsStoreOperator_opt1,
		L"%(0).operator := は定義されていないので = 演算子として実行します" },
} ;

const wchar_t * Loquaty::GetExceptionClassName( ErrorMessageIndex index )
{
	if ( index < errorExceptionCount )
	{
		assert( Symbol::s_ExceptionDesc[index].index == index ) ;
		return	Symbol::s_ExceptionDesc[index].msg ;
	}
	return	nullptr ;
}

const wchar_t * Loquaty::GetErrorMessage( ErrorMessageIndex index )
{
	assert( (size_t) index < sizeof(Symbol::s_ErrorDesc)/sizeof(Symbol::s_ErrorDesc[0]) ) ;
	assert( Symbol::s_ErrorDesc[index].index == index ) ;
	return	Symbol::s_ErrorDesc[index].msg ;
}

ErrorMessageIndex Loquaty::GetErrorIndexOf( const wchar_t * msg )
{
	const size_t	nCount = sizeof(Symbol::s_ErrorDesc)/sizeof(Symbol::s_ErrorDesc[0]) ;
	for ( size_t i = 0; i < nCount; i ++ )
	{
		if ( Symbol::s_ErrorDesc[i].msg == msg )
		{
			return	Symbol::s_ErrorDesc[i].index ;
		}
	}
	return	errorNothing ;
}
