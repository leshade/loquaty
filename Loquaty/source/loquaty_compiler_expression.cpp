
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// コンパイラ・数式解釈
//////////////////////////////////////////////////////////////////////////////

// 数式解釈
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvaluateExpression
	( LStringParser& sparsExpr,
		const LNamespaceList * pnslLocal,
		const wchar_t * pwszEscChars, int nEscPriority )
{
	if ( !sparsExpr.PassSpace() )
	{
		OnError( errorNothingExpression ) ;
		return	std::make_shared<LExprValue>() ;
	}
	LExprValuePtr	xval = nullptr ;
	LString			strToken ;
	do
	{
		// 脱出判定
		wchar_t	wch = sparsExpr.CurrentChar() ;
		if ( LStringParser::IsCharContained( wch, pwszEscChars ) )
		{
			OnError( errorNothingExpression ) ;
			return	std::make_shared<LExprValue>() ;
		}

		// リテラル判定
		if ( sparsExpr.IsCharNumber( wch ) )
		{
			xval = ParseNumberLiteral( sparsExpr ) ;
			break ;
		}
		else if ( (wch == L'\"') || (wch == L'\'') )
		{
			xval = ParseStringLiteral( sparsExpr, wch ) ;
			break ;
		}
		else if ( wch == L'[' )
		{
			xval = ParseArrayLiteral( sparsExpr, pnslLocal ) ;
			break ;
		}
		else if ( wch == L'{' )
		{
			xval = ParseMapLiteral( sparsExpr, pnslLocal ) ;
			break ;
		}
		else if ( wch == L'(' )
		{
			// (type) キャスト構文判定
			sparsExpr.NextChar() ;
			size_t	iTermPos = sparsExpr.GetIndex() ;
			LType	typeCast ;
			if ( sparsExpr.NextTypeExpr( typeCast, false, m_vm, pnslLocal )
				&& (sparsExpr.HasNextChars( L")" ) == L')') )
			{
				// (type) expr
				xval = EvaluateExpression
						( sparsExpr, pnslLocal, pwszEscChars, Symbol::priorityPrefix ) ;
				xval = EvalCastTypeTo( std::move(xval), typeCast, true ) ;
				break ;
			}
			sparsExpr.SeekIndex( iTermPos ) ;
			//
			// (expr) 式
			xval = EvaluateExpression( sparsExpr, pnslLocal, L")" ) ;
			if ( sparsExpr.HasNextChars( L")" ) != L')' )
			{
				OnError( errorMismatchParentheses ) ;
				return	xval ;
			}
			break ;
		}

		// 単項演算子判定
		LStringParser::TokenType
			typeToken = sparsExpr.NextToken( strToken ) ;
		assert( typeToken != LStringParser::tokenNothing ) ;

		Symbol::OperatorIndex
				opIndex = AsOperatorIndex( strToken.c_str() ) ;
		if ( (opIndex != Symbol::opInvalid)
			&& (GetOperatorDesc(opIndex).priorityUnary != Symbol::priorityNo) )
		{
			if ( opIndex == Symbol::opNew )
			{
				// new Type...
				xval = ParseNewOperator( sparsExpr, pnslLocal ) ;
			}
			else if ( opIndex == Symbol::opFunction )
			{
				// function ...
				xval = ParseFunctionExpr( sparsExpr, pnslLocal ) ;
			}
			else if ( opIndex == Symbol::opSizeOf )
			{
				// sizeof ...
				xval = ParseSizeOfOperator( sparsExpr, pnslLocal, pwszEscChars ) ;
			}
			else if ( opIndex == Symbol::opStaticMemberOf )
			{
				// :: ...
				xval = ParseGlobalMemberOf( sparsExpr ) ;
			}
			else
			{
				// その他の前置単項演算子
				xval = EvaluateExpression
						( sparsExpr, pnslLocal, pwszEscChars,
							GetOperatorDesc(opIndex).priorityUnary ) ;
				xval = EvalUnaryOperator
						( opIndex, false, std::move(xval) ) ;
			}
			break ;
		}

		// 予約語判定
		Symbol::ReservedWordIndex
				rwIndex = AsReservedWordIndex( strToken.c_str() ) ;
		if ( rwIndex != Symbol::rwiInvalid )
		{
			xval = ParseReservedWord( sparsExpr, rwIndex, pnslLocal ) ;
			if ( HasErrorOnCurrent() )
			{
				return	xval ;
			}
			break ;
		}

		// 名前解決
		xval = GetExprSymbolAs( strToken.c_str(), &sparsExpr, pnslLocal ) ;
		if ( xval != nullptr )
		{
			break ;
		}

		// エラー
		if ( opIndex != Symbol::opInvalid )
		{
			OnError( errorInvalidExpressionOperator, nullptr, strToken.c_str() ) ;
		}
		else
		{
			OnError( errorUndefinedSymbol, nullptr, strToken.c_str() ) ;
		}
		return	std::make_shared<LExprValue>() ;
	}
	while ( false ) ;

	while ( !HasErrorOnCurrent()
		&& (xval != nullptr) && sparsExpr.PassSpace() )
	{
		if ( xval->IsConstExprClass() )
		{
			// Type() 構文
			// Array<Type>[...]
			// Map<Type>{...}
			// Task<Type>[]{...}
			// Thread<Type>[]{...}
			LClass *	pTypeClass = xval->GetConstExprClass() ;
			assert( pTypeClass != nullptr ) ;
			xval = ParseTypeConstruction( sparsExpr, pTypeClass, pnslLocal ) ;
		}

		// 脱出判定
		if ( !sparsExpr.PassSpace() )
		{
			break ;
		}
		const size_t	iTermPos = sparsExpr.GetIndex() ;
		wchar_t			wch = sparsExpr.CurrentChar() ;
		if ( LStringParser::IsCharContained( wch, pwszEscChars ) )
		{
			break ;
		}

		// 演算子判定
		LStringParser::TokenType
			typeToken = sparsExpr.NextToken( strToken ) ;
		assert( typeToken != LStringParser::tokenNothing ) ;

		Symbol::OperatorIndex opIndex = AsOperatorIndex( strToken.c_str() ) ;
		if ( opIndex == Symbol::opInvalid )
		{
			if ( typeToken == LStringParser::tokenSymbol )
			{
				OnError( errorNoExpressionOperator_opt1, strToken.c_str() ) ;
			}
			else
			{
				OnError( errorNotExpressionOperator_opt1, strToken.c_str() ) ;
			}
			break ;
		}

		const Symbol::OperatorDesc&	opDesc = GetOperatorDesc( opIndex ) ;
		if ( opDesc.priorityUnaryPost != Symbol::priorityNo )
		{
			if ( opDesc.priorityUnaryPost <= nEscPriority )
			{
				sparsExpr.SeekIndex( iTermPos ) ;
				break ;
			}
			// 後置演算子
			xval = EvalUnaryOperator( opIndex, true, std::move(xval) ) ;
		}
		else if ( opDesc.priorityBinary != Symbol::priorityNo )
		{
			if ( opDesc.priorityBinary <= nEscPriority )
			{
				sparsExpr.SeekIndex( iTermPos ) ;
				break ;
			}
			// 二項演算子
			xval = ParseBinaryOperator
					( sparsExpr, std::move(xval),
						opIndex, pnslLocal, pwszEscChars ) ;
		}
		else
		{
			OnError( errorInvalidExpressionOperator, nullptr, strToken.c_str() ) ;
			break ;
		}
	}
	return	xval ;
}

// 定数式解釈
//////////////////////////////////////////////////////////////////////////////
LValue LCompiler::EvaluateConstExpr
	( LStringParser& sparsExpr,
		LVirtualMachine& vm,
		const LNamespaceList * pnslLocal,
		const wchar_t * pwszEscChars, int nEscPriority )
{
	LCompiler			compiler( vm, &sparsExpr ) ;
	LCompiler::Current	current( compiler ) ;

	LContext			context( vm ) ;
	LContext::Current	curCtx( context ) ;

	LExprValuePtr	xval =
		compiler.EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars, nEscPriority ) ;
	if ( xval != nullptr )
	{
		xval = compiler.EvalLoadToDiscloseRef( std::move(xval) ) ;
	}
	if ( (xval == nullptr) || compiler.HasErrorOnCurrent() )
	{
		return	LValue() ;
	}
	if ( !xval->IsConstExpr() )
	{
		return	LValue( xval->GetType() ) ;
	}
	assert( xval->IsSingle() ) ;
	return	*xval ;
}

LLong LCompiler::EvaluateConstExprAsLong
	( LStringParser& sparsExpr,
		LVirtualMachine& vm,
		const LNamespaceList * pnslLocal,
		const wchar_t * pwszEscChars, int nEscPriority )
{
	LValue	val =
		EvaluateConstExpr
			( sparsExpr, vm, pnslLocal, pwszEscChars, nEscPriority ) ;
	return	val.AsInteger() ;
}

// 演算子情報取得
//////////////////////////////////////////////////////////////////////////////
const Symbol::OperatorDesc&
	LCompiler::GetOperatorDesc( Symbol::OperatorIndex opIndex )
{
	assert( Symbol::s_OperatorDescs[opIndex].opIndex == opIndex ) ;
	return	Symbol::s_OperatorDescs[opIndex] ;
}

// 演算子解釈
//////////////////////////////////////////////////////////////////////////////
Symbol::OperatorIndex
	LCompiler::AsOperatorIndex( const wchar_t * pwszOp ) const
{
	auto	iter = m_mapOperatorDescs.find( pwszOp ) ;
	if ( iter != m_mapOperatorDescs.end() )
	{
		return	iter->second->opIndex ;
	}
	return	Symbol::opInvalid ;
}

// 予約語情報取得
//////////////////////////////////////////////////////////////////////////////
const Symbol::ReservedWordDesc&
	LCompiler::GetReservedWordDesc( Symbol::ReservedWordIndex rwIndex )
{
	assert( Symbol::s_ReservedWordDescs[rwIndex].rwIndex == rwIndex ) ;
	return	Symbol::s_ReservedWordDescs[rwIndex] ;
}

// 予約語解釈
//////////////////////////////////////////////////////////////////////////////
Symbol::ReservedWordIndex
	LCompiler::AsReservedWordIndex( const wchar_t * pwszName ) const
{
	auto	iter = m_mapReservedWordDescs.find( pwszName ) ;
	if ( iter != m_mapReservedWordDescs.end() )
	{
		return	iter->second->rwIndex ;
	}
	return	Symbol::rwiInvalid ;
}

// 予約語評価
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseReservedWord
	( LStringParser& sparsExpr,
		Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal )
{
	LExprValuePtr	xval ;
	if ( rwIndex == Symbol::rwiNull )
	{
		// null
		LValue	valNull( LType(), nullptr ) ;
		xval = std::make_shared<LExprValue>() ;
		xval->SetConstValue( valNull ) ;
	}
	else if ( rwIndex == Symbol::rwiFalse )
	{
		// false
		xval = std::make_shared<LExprValue>
					( LType::typeBoolean, LValue::MakeBool(false), true ) ;
	}
	else if ( rwIndex == Symbol::rwiTrue )
	{
		// true
		xval = std::make_shared<LExprValue>
					( LType::typeBoolean, LValue::MakeBool(true), true ) ;
	}
	else if ( rwIndex == Symbol::rwiGlobal )
	{
		// global
		xval = std::make_shared<LExprValue>( m_vm.Global() ) ;
	}
	else if ( rwIndex == Symbol::rwiThis )
	{
		// this
		xval = GetThisObject() ;
		if ( (xval == nullptr) && !HasErrorOnCurrent() )
		{
			OnError( errorNotExistingThis ) ;
		}
	}
	else if ( rwIndex == Symbol::rwiSuper )
	{
		// super
		xval = GetSuperObject( pnslLocal ) ;
		if ( (xval == nullptr) && !HasErrorOnCurrent() )
		{
			OnError( errorNotExistingSuper ) ;
		}
	}
	else if ( IsPrimitiveType( rwIndex ) )
	{
		// PrimitiveType( expr )
		if ( sparsExpr.HasNextChars( L"(" ) != L'(' )
		{
			OnError( errorExpressionError,
					GetReservedWordDesc(rwIndex).pwszName ) ;
			return	std::make_shared<LExprValue>() ;
		}
		xval = EvaluateExpression( sparsExpr, pnslLocal, L")" ) ;
		if ( sparsExpr.HasNextChars( L")" ) != L')' )
		{
			OnError( errorMismatchParentheses ) ;
			return	xval ;
		}
		LType::Primitive	primitive =
				(LType::Primitive) GetReservedWordDesc(rwIndex).mapValue ;
		assert( LType::AsPrimitive(GetReservedWordDesc(rwIndex).pwszName) == primitive ) ;
		assert( primitive < LType::typeObject ) ;
		//
		LType	typeCast( primitive ) ;
		xval = EvalCastTypeTo( std::move(xval), typeCast ) ;
	}
	else if ( rwIndex == Symbol::rwiOperator )
	{
		// operator ?
		size_t	iTempPos = sparsExpr.GetIndex() ;
		LString	strOperator ;
		sparsExpr.NextToken( strOperator ) ;
		//
		Symbol::OperatorIndex
				opIndex = AsOperatorIndex( strOperator.c_str() ) ;
		if ( opIndex == Symbol::opInvalid )
		{
			OnError( errorInvalidOperator, strOperator.c_str() ) ;
			sparsExpr.SeekIndex( iTempPos ) ;
		}
		else if ( !GetOperatorDesc(opIndex).overloadable )
		{
			OnError( errorUnoverloadableOperator, strOperator.c_str() ) ;
			sparsExpr.SeekIndex( iTempPos ) ;
		}
		else
		{
			const Symbol::OperatorDesc&	opdsc = GetOperatorDesc(opIndex) ;
			if ( opdsc.pwszPairName != nullptr )
			{
				if ( !sparsExpr.HasNextToken( opdsc.pwszPairName ) )
				{
					OnError( errorMismatchToken_opt1_opt2,
								opdsc.pwszName, opdsc.pwszPairName ) ;
					sparsExpr.SeekIndex( iTempPos ) ;
					return	std::make_shared<LExprValue>() ;
				}
			}
			LString	strOperator = GetReservedWordDesc(rwIndex).pwszName ;
			strOperator += L" " ;
			strOperator += opdsc.pwszName ;
			if ( opdsc.pwszPairName != nullptr )
			{
				strOperator += opdsc.pwszPairName ;
			}
			xval = GetExprSymbolAs( strOperator.c_str(), &sparsExpr, pnslLocal ) ;
			if ( xval == nullptr )
			{
				OnError( errorUndefinedSymbol, nullptr, strOperator.c_str() ) ;
			}
		}
	}
	else
	{
		OnError( errorSyntaxErrorWithReservedWord,
					GetReservedWordDesc(rwIndex).pwszName ) ;
		xval = std::make_shared<LExprValue>() ;
	}
	return	xval ;
}

// 数値リテラル解釈
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseNumberLiteral( LStringParser& sparsExpr )
{
	LLong	intPart = 0 ;
	bool	signMinus = false ;
	int		radix = 10 ;

	// 整数部評価
	// [{+|-}]{0x|0o|0b}(0-9)*
	signMinus = (sparsExpr.HasNextChars( L"+-" ) == L'-') ;

	sparsExpr.PassSpace() ;

	wchar_t	wch = sparsExpr.CurrentChar() ;
	if ( wch == L'0' )
	{
		wchar_t	wchRadix = sparsExpr.CurrentAt( 1 ) ;
		if ( (wchRadix == L'x') || (wchRadix == L'X') )
		{
			// 0x....
			radix = 16 ;
			sparsExpr.SeekIndex( sparsExpr.GetIndex() + 2 ) ;
		}
		else if ( (wchRadix == L'b') || (wchRadix == L'B') )
		{
			// 0b....
			radix = 2 ;
			sparsExpr.SeekIndex( sparsExpr.GetIndex() + 2 ) ;
		}
		else if ( (wchRadix == L'o') || (wchRadix == L'O') )
		{
			// 0o....
			radix = 8 ;
			sparsExpr.SeekIndex( sparsExpr.GetIndex() + 2 ) ;
		}
		else if ( (wchRadix == L't') || (wchRadix == L'T') )
		{
			// 0t....
			radix = 10 ;
			sparsExpr.SeekIndex( sparsExpr.GetIndex() + 2 ) ;
		}
		else
		{
			// 0...
			wchar_t	wchTestNum ;
			size_t	i = 0 ;
			do
			{
				wchTestNum = sparsExpr.CurrentAt( ++ i ) ;
			}
			while ( (wchTestNum >= L'0') && (wchTestNum < L'8') ) ;
			if ( (wchTestNum != L'.')
				&& (wchTestNum != L'8')
				&& (wchTestNum != L'9') )
			{
				radix = 8 ;
				if ( i >= 2 )
				{
					LCompiler::Warning( warningNumberRadix8, warning2 ) ;
				}
			}
		}
	}

	while ( !sparsExpr.IsEndOfString() )
	{
		wch = sparsExpr.CurrentChar() ;

		int	num = 0 ;
		if ( (wch >= L'0') && (wch <= L'9') )
		{
			num = (int) (wch - L'0') ;
		}
		else if ( (wch >= L'A') && (wch <= L'F') )
		{
			num = (int) (wch - L'A') + 10 ;
		}
		else if ( (wch >= L'a') && (wch <= L'f') )
		{
			num = (int) (wch - L'a') + 10 ;
		}
		else
		{
			break ;
		}
		if ( num >= radix )
		{
			LCompiler::Error( errorInvalidNumberLiteral ) ;
			return	LExprValue::MakeConstExprInt( 0 ) ;
		}
		intPart = intPart * radix + num ;

		sparsExpr.NextChar() ;
	}

	// 小数部評価
	if ( sparsExpr.HasNextChars( L"." ) != L'.' )
	{
		return	LExprValue::MakeConstExprInt
						( signMinus ? -intPart : intPart ) ;
	}

	LDouble	dblDecimal = 0.0 ;
	LDouble	dblDecPlace = 1.0 / radix ;
	while ( !sparsExpr.IsEndOfString() )
	{
		wch = sparsExpr.CurrentChar() ;

		int	num = 0 ;
		if ( (wch >= L'0') && (wch <= L'9') )
		{
			num = (int) (wch - L'0') ;
		}
		else if ( (wch >= L'A') && (wch <= L'F') )
		{
			num = (int) (wch - L'A') + 10 ;
		}
		else if ( (wch >= L'a') && (wch <= L'f') )
		{
			num = (int) (wch - L'a') + 10 ;
		}
		else
		{
			break ;
		}
		if ( num >= radix )
		{
			if ( (wch != L'E') && (wch != L'e')
				&& (wch != L'F') && (wch != L'f') )
			{
				LCompiler::Error( errorInvalidNumberLiteral ) ;
				return	LExprValue::MakeConstExprDouble( 0.0 ) ;
			}
			break ;
		}
		dblDecimal += dblDecPlace * num ;
		dblDecPlace /= radix ;

		sparsExpr.NextChar() ;
	}

	LDouble	dblNumber = (LDouble) intPart + dblDecimal ;
	if ( signMinus )
	{
		dblNumber = -dblNumber ;
	}

	// 指数部評価
	// {E|e}[{+|-}](0-9)*
	wch = sparsExpr.CurrentChar() ;
	if ( (radix <= 10) && ((wch == L'E') || (wch == L'e')) )
	{
		sparsExpr.NextChar() ;
		wch = sparsExpr.CurrentChar() ;

		bool	expMinus = (wch == L'-') ;
		if ( (wch == L'+') || (wch == L'-') )
		{
			sparsExpr.NextChar() ;
		}
		int		expNum = 0 ;
		while ( !sparsExpr.IsEndOfString() )
		{
			wch = sparsExpr.CurrentChar() ;

			int	num = 0 ;
			if ( (wch >= L'0') && (wch <= L'9') )
			{
				num = (int) (wch - L'0') ;
			}
			else
			{
				break ;
			}
			expNum = expNum * 10 + num ;

			sparsExpr.NextChar() ;
		}
		if ( expMinus )
		{
			expNum = -expNum ;
		}
		dblNumber *= pow( 10.0, expNum ) ;
	}

	// float 型後置詞
	wch = sparsExpr.CurrentChar() ;
	if ( (wch == L'f') || (wch == L'F') )
	{
		sparsExpr.NextChar() ;
		return	LExprValue::MakeConstExprFloat( dblNumber ) ;
	}
	else
	{
		return	LExprValue::MakeConstExprDouble( dblNumber ) ;
	}
}

// 文字／文字列リテラル解釈
LExprValuePtr LCompiler::ParseStringLiteral
		( LStringParser& sparsExpr, wchar_t wchQuotation )
{
	sparsExpr.PassSpace() ;
	if ( sparsExpr.CurrentChar() != wchQuotation )
	{
		OnError( errorSyntaxError ) ;
		return	LExprValue::MakeConstExprString( m_vm, LString() ) ;
	}
	sparsExpr.NextChar() ;

	LString	strLiteral ;
	if ( !sparsExpr.NextEnclosedString( strLiteral, wchQuotation, L"\r\n" ) )
	{
		if ( wchQuotation == L'\'' )
		{
			OnError( errorMismatchSinglQuotation ) ;
		}
		else
		{
			OnError( errorMismatchDoubleQuotation ) ;
		}
	}
	strLiteral = LStringParser::DecodeStringLiteral
						( strLiteral.c_str(), strLiteral.GetLength() ) ;
	if ( wchQuotation == L'\'' )
	{
		return	LExprValue::MakeConstExprInt( strLiteral.GetAt(0) ) ;
	}
	else
	{
		return	LExprValue::MakeConstExprString( m_vm, strLiteral ) ;
	}
}

// 配列リテラル解釈
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseArrayLiteral
	( LStringParser& sparsExpr,
		const LNamespaceList * pnslLocal, LClass * pCastElementType )
{
	LClass *		pElementClass = pCastElementType ;		// 要素型
	LClass *		pArrayClass = m_vm.GetArrayClassAs( pCastElementType ) ;
	LPtr<LArrayObj>	pArrayObj = new LArrayObj( pArrayClass ) ;

	sparsExpr.PassSpace() ;
	if ( sparsExpr.CurrentChar() != L'[' )
	{
		OnError( errorSyntaxError ) ;
		return	LExprValue::MakeConstExprObject( pArrayObj ) ;
	}
	sparsExpr.NextChar() ;

	if ( sparsExpr.HasNextChars( L"]" ) == L']' )
	{
		// [] : 空の配列
		return	LExprValue::MakeConstExprObject( pArrayObj ) ;
	}

	// ※初めは定数式として解釈しようとする
	LExprValuePtr	xval ;
	for ( ; ; )
	{
		// 要素
		xval = EvaluateExpression( sparsExpr, pnslLocal, L",)]};" ) ;
		if ( xval->GetType().IsPrimitive() )
		{
			xval = EvalAsObject( std::move(xval) ) ;
		}
		if ( pCastElementType != nullptr )
		{
			xval = EvalCastTypeTo( std::move(xval), pCastElementType ) ;
		}
		if ( !xval->IsConstExpr() || HasErrorOnCurrent() )
		{
			break ;
		}

		LObject *	pObj = xval->GetObject().Get() ;
		if ( pObj != nullptr )
		{
			pElementClass =
				GetCommonClasses( pElementClass, pObj->GetClass() ) ;
		}
		LObject::ReleaseRef
			( pArrayObj->SetElementAt
				( pArrayObj->GetElementCount(), pObj ) ) ;

		wchar_t	wch = sparsExpr.HasNextChars( L",]" ) ;
		if ( wch == L']' )
		{
			return	LExprValue::MakeConstExprObject
				( ConstExprCastToArrayElementType( pArrayObj, pElementClass ) ) ;
		}
		else if ( wch != L',' )
		{
			OnError( errorNoSeparationArrayList ) ;
			sparsExpr.PassStatement( L"]", L")};" ) ;
			return	LExprValue::MakeConstExprObject
				( ConstExprCastToArrayElementType( pArrayObj, pElementClass ) ) ;
		}
		if ( sparsExpr.HasNextChars( L"]" ) == L']' )
		{
			return	LExprValue::MakeConstExprObject
				( ConstExprCastToArrayElementType( pArrayObj, pElementClass ) ) ;
		}
	}
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L"]", L")};" ) ;
		return	LExprValue::MakeConstExprObject( pArrayObj ) ;
	}

	// 定数式では表現できないので実行時式に変換する
	pArrayObj = ConstExprCastToArrayElementType( pArrayObj, pElementClass ) ;
	if ( m_ctx == nullptr )
	{
		OnError( errorUnavailableConstExpr ) ;
		sparsExpr.PassStatement( L"]", L")};" ) ;
		return	LExprValue::MakeConstExprObject( pArrayObj ) ;
	}

	LExprValuePtr	xvalArray = ExprLoadImmObject( pArrayObj ) ;
	size_t			iElement = pArrayObj->GetElementCount() ;

	for ( ; ; )
	{
		if ( HasErrorOnCurrent() )
		{
			sparsExpr.PassStatement( L"]", L")};" ) ;
			break ;
		}

		// 要素追加
		if ( xval->GetType().IsPrimitive() )
		{
			xval = EvalAsObject( std::move(xval) ) ;
		}
		if ( pCastElementType != nullptr )
		{
			xval = EvalCastTypeTo( std::move(xval), pCastElementType ) ;
		}

		pElementClass =
			GetCommonClasses( pElementClass, xval->GetType().GetClass() ) ;

		ExprStoreObjectElementAt
			( xvalArray, ExprLoadImmInteger(iElement ++), std::move(xval) ) ;

		if ( HasErrorOnCurrent() )
		{
			break ;
		}

		// リスト判定
		wchar_t	wch = sparsExpr.HasNextChars( L",]" ) ;
		if ( wch == L']' )
		{
			break ;
		}
		else if ( wch != L',' )
		{
			OnError( errorNoSeparationArrayList ) ;
			break ;
		}
		if ( sparsExpr.HasNextChars( L"]" ) == L']' )
		{
			break ;
		}

		// 要素
		xval = EvaluateExpression( sparsExpr, pnslLocal, L",)]};" ) ;
	}
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L"]", L")};" ) ;
	}

	// 配列の要素型を設定する
	if ( (pCastElementType == nullptr)
		&& (pElementClass != nullptr) )
	{
		xvalArray = EvalCastTypeTo
			( std::move(xvalArray),
				LType( m_vm.GetArrayClassAs( pElementClass ) ), true ) ;
	}
	return	xvalArray ;
}

// 辞書配列リテラル解釈
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseMapLiteral
	( LStringParser& sparsExpr,
		const LNamespaceList * pnslLocal, LClass * pCastElementType )
{
	LClass *		pElementClass = nullptr ;		// 要素型
	LClass *		pMapClass = m_vm.GetMapClassAs( pCastElementType ) ;
	LPtr<LMapObj>	pMapObj = new LMapObj( pMapClass ) ;

	sparsExpr.PassSpace() ;
	if ( sparsExpr.CurrentChar() != L'{' )
	{
		OnError( errorSyntaxError ) ;
		return	LExprValue::MakeConstExprObject( pMapObj ) ;
	}
	sparsExpr.NextChar() ;

	if ( sparsExpr.HasNextChars( L"}" ) == L'}' )
	{
		// {} : 空の配列
		return	LExprValue::MakeConstExprObject( pMapObj ) ;
	}

	// ※初めは定数式として解釈しようとする
	LExprValuePtr	xval ;
	LString			strName ;
	for ( ; ; )
	{
		// 要素名
		if ( !ParseMapLiteralElementName( strName, sparsExpr ) )
		{
			break ;
		}
		if ( sparsExpr.HasNextChars( L":" ) != L':' )
		{
			OnError( errorMapElementNameSyntaxError, strName.c_str() ) ;
			break ;
		}

		// 要素
		xval = EvaluateExpression( sparsExpr, pnslLocal, L",)]};" ) ;
		if ( xval->GetType().IsPrimitive() )
		{
			xval = EvalAsObject( std::move(xval) ) ;
		}
		if ( pCastElementType != nullptr )
		{
			xval = EvalCastTypeTo( std::move(xval), pCastElementType ) ;
		}
		if ( !xval->IsConstExpr() || HasErrorOnCurrent() )
		{
			break ;
		}

		LObject *	pObj = xval->GetObject().Get() ;
		if ( pObj != nullptr )
		{
			pElementClass =
				GetCommonClasses( pElementClass, pObj->GetClass() ) ;
		}
		LObject::ReleaseRef
			( pMapObj->SetElementAs( strName.c_str(), pObj ) ) ;

		wchar_t	wch = sparsExpr.HasNextChars( L",}" ) ;
		if ( wch == L'}' )
		{
			return	LExprValue::MakeConstExprObject
				( ConstExprCastToMapElementType( pMapObj, pElementClass ) ) ;
		}
		else if ( wch != L',' )
		{
			OnError( errorNoSeparationMapList ) ;
			sparsExpr.PassStatement( L"}", L"])};" ) ;
			return	LExprValue::MakeConstExprObject
				( ConstExprCastToMapElementType( pMapObj, pElementClass ) ) ;
		}
		if ( sparsExpr.HasNextChars( L"}" ) == L'}' )
		{
			return	LExprValue::MakeConstExprObject
				( ConstExprCastToMapElementType( pMapObj, pElementClass ) ) ;
		}
	}
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L"}", L"]);" ) ;
		return	LExprValue::MakeConstExprObject( pMapObj ) ;
	}

	// 定数式では表現できないので実行時式に変換する
	pMapObj = ConstExprCastToMapElementType( pMapObj, pElementClass ) ;
	if ( m_ctx == nullptr )
	{
		OnError( errorUnavailableConstExpr ) ;
		sparsExpr.PassStatement( L"}", L"]);" ) ;
		return	LExprValue::MakeConstExprObject( pMapObj ) ;
	}

	LExprValuePtr	xvalMap = ExprLoadImmObject( pMapObj ) ;
	for ( ; ; )
	{
		if ( HasErrorOnCurrent() )
		{
			sparsExpr.PassStatement( L"}", L"]);" ) ;
			break ;
		}

		// 要素追加
		if ( xval->GetType().IsPrimitive() )
		{
			xval = EvalAsObject( std::move(xval) ) ;
		}
		if ( pCastElementType != nullptr )
		{
			xval = EvalCastTypeTo( std::move(xval), pCastElementType ) ;
		}

		pElementClass =
			GetCommonClasses( pElementClass, xval->GetType().GetClass() ) ;

		ExprStoreObjectElementAs
			( xvalMap, ExprLoadImmString(strName), std::move(xval) ) ;

		if ( HasErrorOnCurrent() )
		{
			break ;
		}

		// リスト判定
		wchar_t	wch = sparsExpr.HasNextChars( L",}" ) ;
		if ( wch == L'}' )
		{
			break ;
		}
		else if ( wch != L',' )
		{
			OnError( errorNoSeparationMapList ) ;
			break ;
		}
		if ( sparsExpr.HasNextChars( L"}" ) == L'}' )
		{
			break ;
		}

		// 要素名
		if ( !ParseMapLiteralElementName( strName, sparsExpr ) )
		{
			break ;
		}
		if ( sparsExpr.HasNextChars( L":" ) != L':' )
		{
			OnError( errorMapElementNameSyntaxError, strName.c_str() ) ;
			break ;
		}

		// 要素
		xval = EvaluateExpression( sparsExpr, pnslLocal, L",)]};" ) ;
	}
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L"]", L")};" ) ;
	}

	// 配列の要素型を設定する
	if ( (pCastElementType == nullptr)
		&& (pElementClass != nullptr) )
	{
		xvalMap = EvalCastTypeTo
			( std::move(xvalMap),
				LType( m_vm.GetMapClassAs( pElementClass ) ), true ) ;
	}
	return	xvalMap ;
}

// 辞書配列リテラルの要素名解釈
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::ParseMapLiteralElementName
		( LString& strName, LStringParser& sparsExpr )
{
	bool	flagSuccess = true ;
	wchar_t	wch = sparsExpr.HasNextChars( L"\"\'" ) ;
	if ( wch != 0 )
	{
		if ( !sparsExpr.NextEnclosedString( strName, wch, L"\r\n" ) )
		{
			OnError( errorMapElementNameSyntaxError, strName.c_str() ) ;
			flagSuccess = false ;
		}
		strName = LStringParser::DecodeStringLiteral
						( strName.c_str(), strName.GetLength() ) ;
	}
	else
	{
		LStringParser::TokenType	type = sparsExpr.NextToken( strName ) ;
		if ( type != LStringParser::tokenSymbol )
		{
			OnError( errorMapElementNameSyntaxError, strName.c_str() ) ;
			flagSuccess = false ;
		}
	}
	return	flagSuccess ;
}

// 名前解決（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::GetExprSymbolAs
	( const wchar_t * pwszName,
		LStringParser * psparsExpr, const LNamespaceList * pnslLocal )
{
	// ローカル変数を探す
	LExprValuePtr	xvalLocal = GetLocalVariable( pwszName ) ;
	if ( xvalLocal != nullptr )
	{
		return	xvalLocal ;
	}

	// this から探す
	LExprValuePtr	xvalThis = GetThisObject() ;
	if ( xvalThis != nullptr )
	{
		LExprValuePtr	xvalMember = GetExprVarMember( xvalThis, pwszName ) ;
		if ( xvalMember != nullptr )
		{
			return	xvalMember ;
		}
	}

	// ネストから探す
	LNamespaceList	nslTemp ;
	if ( m_ctx != nullptr )
	{
		CodeNestPtr	pNest = m_ctx->m_curNest ;
		while ( pNest != nullptr )
		{
			if ( pNest->m_namespace != nullptr )
			{
				nslTemp.AddNamespace( pNest->m_namespace.Ptr() ) ;
			}
			nslTemp.AddNamespaceList( pNest->m_nslUsing ) ;

			if ( (pNest->m_type == CodeNest::ctrlWith)
				&& (pNest->m_xvalObj != nullptr) )
			{
				LExprValuePtr	xvalMember =
						GetExprVarMember( pNest->m_xvalObj, pwszName ) ;
				if ( xvalMember != nullptr )
				{
					return	xvalMember ;
				}
			}
			pNest = pNest->m_prev ;
		}
	}

	// 名前空間から探す
	if ( pnslLocal != nullptr )
	{
		nslTemp.AddNamespaceList( *pnslLocal ) ;
	}
	for ( size_t i = 0; i < nslTemp.size(); i ++ )
	{
		LNamespace *	pnsLocal = nslTemp.at(i) ;
		while ( (pnsLocal != nullptr) && (pnsLocal != m_vm.Global().Ptr()) )
		{
			LExprValuePtr	xval =
					GetSymbolWithNamespaceAs
						( pwszName, psparsExpr, pnsLocal, &nslTemp ) ;
			if ( xval != nullptr )
			{
				return	xval ;
			}
			pnsLocal = pnsLocal->GetParentNamespace().Ptr() ;
		}
	}

	// グローバル空間から探す
	return	GetSymbolWithNamespaceAs
				( pwszName, psparsExpr, m_vm.Global().Ptr(), &nslTemp ) ;
}

// 名前空間から探す（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::GetSymbolWithNamespaceAs
	( const wchar_t * pwszName,
		LStringParser * psparsExpr, LNamespace * pnsLocal,
		const LNamespaceList * pnslLocal )
{
	assert( pnsLocal != nullptr ) ;
	LClass *	pLocalClass = dynamic_cast<LClass*>( pnsLocal ) ;

	// 静的なローカル変数（オブジェクト）
	const LType *	pVarType = pnsLocal->GetLocalObjectTypeAs( pwszName ) ;
	if ( (pVarType != nullptr)
		&& IsAccessibleTo( pLocalClass, pVarType->GetAccessModifier() ) )
	{
		LObjPtr	pVarObj = pnsLocal->GetLocalObjectAs( pwszName ) ;
		if ( pVarType->IsConst() )
		{
			if ( pVarType->IsModifiered( LType::modifierDeprecated ) )
			{
				OnWarning( warningRefDeprecatedVar, warning3, pwszName ) ;
			}
			// static const なオブジェクトは定数値
			return	std::make_shared<LExprValue>
						( *pVarType, pVarObj, true, true ) ;
		}
		else
		{
			// 名前空間への参照
			ssize_t	iElement = pnsLocal->FindElementAs( pwszName ) ;
			assert( iElement >= 0 ) ;

			pnsLocal->AddRef() ;
			LPtr<LNamespace>	pNamespace( pnsLocal ) ;

			LExprValuePtr	xvalObj =
				std::make_shared<LExprValue>( *pVarType, nullptr, false, false ) ;
			xvalObj->SetOptionRefByIndexOf
				( std::make_shared<LExprValue>( pNamespace ),
									ExprLoadImmInteger( iElement ) ) ;
			return	xvalObj ;
		}
	}

	// 静的なローカル変数（データ）
	LArrangement::Desc	descVar ;
	if ( pnsLocal->GetStaticArrangement().GetDescAs( descVar, pwszName )
		&& IsAccessibleTo( pLocalClass, descVar.m_type.GetAccessModifier() ) )
	{
		if ( descVar.m_type.GetModifiers() & LType::modifierDeprecated )
		{
			OnWarning( warningRefDeprecatedVar, warning3, pwszName ) ;
		}
		return	EvalLoadRefPtrStaticBuf
			( descVar.m_type,
				pnsLocal->GetStaticArrangement().GetBuffer(),
				descVar.m_location, descVar.m_size ) ;
	}

	// 静的な関数
	const LFunctionVariation *
			pFuncVar = pnsLocal->GetLocalStaticFunctionsAs( pwszName ) ;
	if ( pFuncVar != nullptr )
	{
		LClass *	pAbsFuncClass = m_vm.GetFunctionClass() ;
		return	std::make_shared<LExprValue>( pAbsFuncClass, pFuncVar ) ;
	}

	// クラス
	LClass *	pClass = pnsLocal->GetLocalClassAs( pwszName ) ;
	if ( pClass != nullptr )
	{
		pClass->AddRef() ;
		return	std::make_shared<LExprValue>
					( LType(pClass->GetClass()), pClass, true, true ) ;
	}

	// 名前空間
	LPtr<LNamespace>	pNamespace = pnsLocal->GetLocalNamespaceAs( pwszName ) ;
	if ( pNamespace != nullptr )
	{
		return	std::make_shared<LExprValue>( pNamespace ) ;
	}

	// 定義された型
	const LType *	pTypeDef = pnsLocal->GetTypeAs( pwszName ) ;
	if ( (pTypeDef != nullptr)
		&& pTypeDef->IsObject()
		&& (pTypeDef->GetClass() != nullptr) )
	{
		if ( pTypeDef->IsConst() )
		{
			OnWarning( warningIgnoredConstModifier, warning3,
						nullptr, pTypeDef->GetTypeName().c_str() ) ;
		}
		pClass = pTypeDef->GetClass() ;
		pClass->AddRef() ;
		return	std::make_shared<LExprValue>
					( LType(pClass->GetClass()), pClass, true, true ) ;
	}

	// ジェネリック型
	const LNamespace::GenericDef *
			pGenType = pnsLocal->GetGenericTypeAs( pwszName ) ;
	if ( (pGenType != nullptr)
		&& (psparsExpr != nullptr) )
	{
		pNamespace = ParseGenericType
			( *psparsExpr, pwszName, pGenType, true, pnslLocal ) ;
		if ( pNamespace != nullptr )
		{
			pClass = dynamic_cast<LClass*>( pNamespace.Ptr() ) ;
			if ( pClass != nullptr )
			{
				pClass->AddRef() ;
				return	std::make_shared<LExprValue>
							( LType(pClass->GetClass()), pClass, true, true ) ;
			}
			else
			{
				return	std::make_shared<LExprValue>( pNamespace ) ;
			}
		}
	}

	if ( pLocalClass != nullptr )
	{
		// 親クラスから探す
		LClass *	pSuperClass = pLocalClass->GetSuperClass() ;
		if ( pSuperClass != nullptr )
		{
			LExprValuePtr	xval =
				GetSymbolWithNamespaceAs
					( pwszName, psparsExpr, pSuperClass, pnslLocal ) ;
			if ( xval != nullptr )
			{
				return	xval ;
			}
		}
		for ( size_t i = 0; i < pLocalClass->GetImplementClasses().size(); i ++ )
		{
			LExprValuePtr	xval =
				GetSymbolWithNamespaceAs
					( pwszName, psparsExpr,
						pLocalClass->GetImplementClasses().at(i), pnslLocal ) ;
			if ( xval != nullptr )
			{
				return	xval ;
			}
		}
	}
	return	nullptr ;
}

// 適合する静的な関数を探す（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LPtr<LFunctionObj> LCompiler::GetStaticFunctionAs
	( const wchar_t * pwszName,
		const LArgumentListType& arglist, const LNamespaceList * pnslLocal )
{
	LPtr<LFunctionObj>	pFunc ;

	// this から探す
	LClass *	pThisClass = GetThisClass() ;
	if ( pThisClass != nullptr )
	{
		pFunc = GetStaticFunctionOfNamespaceAs( pwszName, arglist, pThisClass ) ;
		if ( pFunc != nullptr )
		{
			return	pFunc ;
		}
	}

	// ネストから探す
	if ( m_ctx != nullptr )
	{
		CodeNestPtr	pNest = m_ctx->m_curNest ;
		while ( pNest != nullptr )
		{
			if ( pNest->m_namespace != nullptr )
			{
				pFunc = GetStaticFunctionOfNamespaceAs
							( pwszName, arglist, pNest->m_namespace.Ptr() ) ;
				if ( pFunc != nullptr )
				{
					return	pFunc ;
				}
			}
			pNest = pNest->m_prev ;
		}
	}

	// 名前空間から探す
	if ( pnslLocal != nullptr )
	{
		for ( size_t i = 0; i < pnslLocal->size(); i ++ )
		{
			pFunc = GetStaticFunctionOfNamespaceAs
						( pwszName, arglist, pnslLocal->at(i) ) ;
			if ( pFunc != nullptr )
			{
				return	pFunc ;
			}
		}
	}

	// グローバル空間から探す
	return	GetStaticFunctionOfNamespaceAs
				( pwszName, arglist, m_vm.Global().Ptr() ) ;
}

LPtr<LFunctionObj> LCompiler::GetStaticFunctionOfNamespaceAs
	( const wchar_t * pwszName,
		const LArgumentListType& arglist, LNamespace * pnsLocal )
{
	while ( pnsLocal != nullptr )
	{
		const LFunctionVariation *
				pFuncVar = pnsLocal->GetLocalStaticFunctionsAs( pwszName ) ;
		if ( pFuncVar != nullptr )
		{
			LPtr<LFunctionObj>
					pFunc = pFuncVar->GetCallableFunction( arglist ) ;
			if ( pFunc != nullptr )
			{
				return	pFunc ;
			}
		}
		pnsLocal = pnsLocal->GetParentNamespace().Ptr() ;
	}
	return	nullptr ;
}


// ジェネリック型の解釈・インスタンス化
//////////////////////////////////////////////////////////////////////////////
LPtr<LNamespace> LCompiler::ParseGenericType
	( LStringParser& sparsExpr, const wchar_t * pwszName,
		const LNamespace::GenericDef * pGenType,
		bool instantiateGenType, const LNamespaceList * pnslLocal )
{
	const size_t	iSaveIndex = sparsExpr.GetIndex() ;

	// ジェネリック型引数解釈
	LString				strGenTypeName ;
	std::vector<LType>	vecTypes ;

	if ( !sparsExpr.ParseGenericArgList
		( vecTypes, strGenTypeName, false, pwszName, m_vm, pnslLocal ) )
	{
		sparsExpr.SeekIndex( iSaveIndex ) ;
		return	nullptr ;
	}

	// インスタンス化済みか？
	LClass *	pGenClass =
		pGenType->m_parent->GetClassAs( strGenTypeName.c_str() ) ;
	if ( pGenClass != nullptr )
	{
		pGenClass->AddRef() ;
		return	pGenClass ;
	}
	if ( !instantiateGenType )
	{
		sparsExpr.SeekIndex( iSaveIndex ) ;
		return	nullptr ;
	}

	// インスタンス化
	LPtr<LNamespace>	pInstance ;
	pInstance = pGenType->Instantiate( *this, vecTypes ) ;
	if ( pInstance == nullptr )
	{
		sparsExpr.SeekIndex( iSaveIndex ) ;
	}
	return	pInstance ;
}

// バッファを参照する式をポインタに変換可能なら変換する（型情報の正規化のみ）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::DiscloseRefPointerType( LExprValuePtr xvalRef )
{
	if ( xvalRef == nullptr )
	{
		return	nullptr ;
	}
	if ( !xvalRef->IsRefPointer() )
	{
		// 参照ポインタじゃなければそのまま
		return	xvalRef ;
	}
	if ( xvalRef->GetType().IsPrimitive() )
	{
		// プリミティブ型への参照はそのまま
		return	xvalRef ;
	}
	if ( xvalRef->GetType().IsDataArray() )
	{
		// データ配列への参照はポインタ型へ変換する
		LClass *	pPtrClass =
			m_vm.GetPointerClassAs
				( xvalRef->GetType().GetDataArrayClass()->
						GetElementType().ConstWith(xvalRef->GetType()) ) ;
		xvalRef->Type() = LType( pPtrClass ) ;
		xvalRef->MakeRefIntoPointer() ;
		return	xvalRef ;
	}
	if ( xvalRef->GetType().IsStructure() )
	{
		// 構造体はポインタ型へ変換する
		xvalRef->Type() = LType( m_vm.GetPointerClassAs( xvalRef->GetType() ) ) ;
		xvalRef->MakeRefIntoPointer() ;
		return	xvalRef ;
	}
	if ( xvalRef->GetType().IsStructurePtr() )
	{
		// 構造体ポインタ型はそのまま
		xvalRef->MakeRefIntoPointer() ;
		return	xvalRef ;
	}
	return	xvalRef ;
}

// メンバから探す（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::GetExprVarMember
	( LExprValuePtr xvalVar, const wchar_t * pwszName )
{
	if ( xvalVar == nullptr )
	{
		return	nullptr ;
	}
	if ( xvalVar->IsNamespace() )
	{
		LNamespaceList	nslLocal ;
		AddScopeToNamespaceList( nslLocal ) ;
		return	GetSymbolWithNamespaceAs
					( pwszName, nullptr,
						xvalVar->GetNamespace().Ptr(), &nslLocal ) ;
	}
	if ( xvalVar->IsConstExprClass() )
	{
		LNamespaceList	nslLocal ;
		AddScopeToNamespaceList( nslLocal ) ;
		return	GetSymbolWithNamespaceAs
					( pwszName, nullptr,
						xvalVar->GetConstExprClass(), &nslLocal ) ;
	}
	if ( xvalVar->GetType().IsPrimitive() )
	{
		return	nullptr ;
	}
	if ( xvalVar->IsRefPointer() )
	{
		// 参照型の場合は一度ポインタ型に変換する
		xvalVar = DiscloseRefPointerType( std::make_shared<LExprValue>(*xvalVar) ) ;
		if ( xvalVar->IsRefPointer() )
		{
			// プリミティブ型にメンバはない
			return	nullptr ;
		}
	}
	LClass *	pClass = xvalVar->GetType().GetClass() ;
	if ( pClass == nullptr )
	{
		return	nullptr ;
	}
	bool			flagConstVal = xvalVar->GetType().IsConst() ;
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( pClass ) ;
	if ( pPtrClass != nullptr )
	{
		// ポインタ型の場合は構造体のメンバが対象
		LStructureClass *
			pStructClass = pPtrClass->GetBufferType().GetStructureClass() ;
		if ( pStructClass != nullptr )
		{
			flagConstVal = pPtrClass->GetBufferType().IsConst() ;
			pClass = pStructClass ;
		}
	}
	else
	{
		// オブジェクトのメンバ変数を探す
		if ( xvalVar->IsConstExpr()
			&& (xvalVar->GetObject() != nullptr) )
		{
			LObjPtr	pElement = xvalVar->GetObject()->GetElementAs( pwszName ) ;
			LType	typeElement = xvalVar->GetObject()->GetElementTypeAs( pwszName ) ;
			if ( (pElement != nullptr)
				&& IsAccessibleTo( pClass, typeElement.GetAccessModifier() ) )
			{
				if ( typeElement.IsModifiered( LType::modifierDeprecated ) )
				{
					OnWarning( warningRefDeprecatedVar, warning3, pwszName ) ;
				}
				// 定数式のメンバ・オブジェクト
				return	std::make_shared<LExprValue>
						( typeElement.ConstWith(xvalVar->GetType()),
							pElement, true, xvalVar->IsUniqueObject() ) ;
			}
		}
		LObjPtr	pProtoObj = pClass->GetPrototypeObject() ;
		if ( pProtoObj != nullptr )
		{
			LType	typeElement = pProtoObj->GetElementTypeAs( pwszName ) ;
			ssize_t	iElement = pProtoObj->FindElementAs( pwszName ) ;
			if ( (iElement >= 0)
				&& IsAccessibleTo( pClass, typeElement.GetAccessModifier() ) )
			{
				if ( typeElement.IsModifiered( LType::modifierDeprecated ) )
				{
					OnWarning( warningRefDeprecatedVar, warning3, pwszName ) ;
				}
				typeElement.SetModifiers
					( typeElement.GetModifiers() & LType::modifierConst ) ;

				// オブジェクトのメンバ変数への参照
				LExprValuePtr	xvalElement =
					std::make_shared<LExprValue>
						( typeElement.ConstWith(xvalVar->GetType()), nullptr, false, false ) ;
				xvalElement->SetOptionRefByIndexOf
						( xvalVar, ExprLoadImmInteger( iElement ) ) ;
				return	xvalElement ;
			}
		}
	}

	LArrangement::Desc	desc ;
	if ( pClass->GetProtoArrangemenet().GetDescAs( desc, pwszName )
		&& IsAccessibleTo( pClass, desc.m_type.GetAccessModifier() ) )
	{
		if ( desc.m_type.GetModifiers() & LType::modifierDeprecated )
		{
			OnWarning( warningRefDeprecatedVar, warning3, pwszName ) ;
		}
		LType	typeElement = desc.m_type ;
		typeElement.SetModifiers
			( typeElement.GetModifiers() & LType::modifierConst ) ;

		// 構造体／バッファ上のメンバ変数への参照
		LExprValuePtr	xvalElement =
			std::make_shared<LExprValue>
				( GetLocalTypeOf(typeElement, flagConstVal),
					xvalVar->GetObject(),
					xvalVar->IsConstExpr(), xvalVar->IsUniqueObject() ) ;
		xvalElement->SetOptionRefPointerOffset
			( xvalVar, LExprValue::MakeConstExprInt( (LLong) desc.m_location ) ) ;
		return	DiscloseRefPointerType( std::move(xvalElement) ) ;
	}

	const std::vector<size_t> *
		pVirtVec = pClass->GetVirtualVector().FindFunction( pwszName ) ;
	if ( pVirtVec != nullptr )
	{
		// 仮想関数ベクタへの参照
		LClass *		pAbsFuncClass = m_vm.GetFunctionClass() ;
		LExprValuePtr	xvalVirtFunc =
				std::make_shared<LExprValue>( pAbsFuncClass, pClass, pVirtVec ) ;
		LExprValuePtr	xvalRefCallFunc =
				std::make_shared<LExprValue>
						( LType(pAbsFuncClass), nullptr, false, false ) ;
		xvalRefCallFunc->SetOptionRefCallThisOf( xvalVar, xvalVirtFunc ) ;
		return	xvalRefCallFunc ;
	}

	if ( pPtrClass != nullptr )
	{
		// ポインタ型のメソッドを参照
		const std::vector<size_t> *
			pVirtVec = pPtrClass->GetVirtualVector().FindFunction( pwszName ) ;
		if ( pVirtVec != nullptr )
		{
			LClass *		pAbsFuncClass = m_vm.GetFunctionClass() ;
			LExprValuePtr	xvalVirtFunc =
				std::make_shared<LExprValue>
					( pAbsFuncClass,
						pPtrClass->GetVirtualVector().MakeFuncVariationOf( pVirtVec ) ) ;
			LExprValuePtr	xvalRefCallFunc =
					std::make_shared<LExprValue>
						( LType(pAbsFuncClass), nullptr, false, false ) ;
			xvalRefCallFunc->SetOptionRefCallThisOf( xvalVar, xvalVirtFunc ) ;
			return	xvalRefCallFunc ;
		}
	}

	LNamespaceList	nslLocal ;
	AddScopeToNamespaceList( nslLocal ) ;
	return	GetSymbolWithNamespaceAs
				( pwszName, nullptr, pClass, &nslLocal ) ;
}

// ローカル変数を探す（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::GetLocalVariable( const wchar_t * pwszName )
{
	if ( m_ctx == nullptr )
	{
		return	nullptr ;
	}
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		LLocalVarPtr	pVar ;
		if ( pNest->m_frame != nullptr )
		{
			pVar = pNest->m_frame->GetLocalVarAs( pwszName ) ;
		}
		if ( pVar != nullptr )
		{
			assert( GetLocalVarAt( pVar->GetLocation() ) == pVar ) ;
			if ( pVar->GetAllocType() == LLocalVar::allocPointer )
			{
				assert( pVar->GetType().IsPointer() ) ;
				LExprValuePtr	xvalPtrIndex =
						GetLocalVarAt( pVar->GetLocation() + 2 ) ;
				//
				LExprValuePtr	xvalPtr =
					std::make_shared<LExprValue>
						( pVar->GetType(), nullptr, false, false ) ;
				xvalPtr->SetOptionPointerOffset( pVar, xvalPtrIndex ) ;
				return	xvalPtr ;
			}
			else if ( pVar->GetAllocType() == LLocalVar::allocLocalArray )
			{
				return	EvalLoadToDiscloseRef( pVar ) ;
			}
			return	pVar ;
		}
		if ( (pNest->m_type == CodeNest::ctrlWith)
				&& (pNest->m_xvalObj != nullptr) )
		{
			LExprValuePtr	xvalMember =
					GetExprVarMember( pNest->m_xvalObj, pwszName ) ;
			if ( xvalMember != nullptr )
			{
				return	xvalMember ;
			}
			LClass *	pClass = GetExprVarClassOf( pNest->m_xvalObj ) ;
			if ( pClass != nullptr )
			{
				LNamespaceList	nslLocal ;
				AddScopeToNamespaceList( nslLocal ) ;

				LExprValuePtr	xval =
					GetSymbolWithNamespaceAs
						( pwszName, nullptr, pClass, &nslLocal ) ;
				if ( xval != nullptr )
				{
					return	xval ;
				}
			}
		}
		if ( pNest->m_namespace != nullptr )
		{
			LNamespaceList	nslLocal ;
			AddScopeToNamespaceList( nslLocal ) ;

			LExprValuePtr	xval =
				GetSymbolWithNamespaceAs
					( pwszName, nullptr,
						pNest->m_namespace.Ptr(), &nslLocal ) ;
			if ( xval != nullptr )
			{
				return	xval ;
			}
		}
		pNest = pNest->m_prev ;
	}
	return	nullptr ;
}

// this 取得（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::GetThisObject( void )
{
	return	GetLocalVariable( L"this" ) ;
}

// super 取得（見つからなければ nullptr）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::GetSuperObject( const LNamespaceList * pnslLocal )
{
	LExprValuePtr	xvalThis = GetThisObject() ;
	if ( xvalThis == nullptr )
	{
		return	nullptr ;
	}
	LClass *	pClass = GetExprVarClassOf( xvalThis ) ;
	if ( pClass == nullptr )
	{
		return	nullptr ;
	}
	LClass *	pSuperClass = pClass->GetSuperClass() ;
	if ( pSuperClass == nullptr )
	{
		return	nullptr ;
	}
	LPointerClass *	pPtrClass = xvalThis->GetType().GetPointerClass() ;
	if ( pPtrClass != nullptr )
	{
		assert( dynamic_cast<LStructureClass*>(pSuperClass) != nullptr ) ;
		return	EvalCastTypeTo
			( std::move(xvalThis),
				LType(m_vm.GetPointerClassAs(LType(pSuperClass))).
								ConstWith(pPtrClass->GetBufferType()) ) ;
	}
	else
	{
		return	EvalCastTypeTo
			( std::move(xvalThis),
				LType(pSuperClass).ConstWith(xvalThis->GetType()) ) ;
	}
}

// this クラスを取得（見つからなければ nullptr / 構造体ポインタの場合には構造体クラス）
//////////////////////////////////////////////////////////////////////////////
LClass * LCompiler::GetThisClass( void )
{
	return	GetExprVarClassOf( GetLocalVariable( L"this" ) ) ;
}

// 評価値のクラスを取得（構造体ポインタの場合に構造体を取得）
//////////////////////////////////////////////////////////////////////////////
LClass * LCompiler::GetExprVarClassOf( LExprValuePtr xval )
{
	if ( xval == nullptr )
	{
		return	nullptr ;
	}
	LClass *	pClass = xval->GetType().GetClass() ;
	if ( pClass == nullptr )
	{
		return	nullptr ;
	}
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( pClass ) ;
	if ( pPtrClass != nullptr )
	{
		// this が構造体ポインタの場合には構造体のクラス
		LStructureClass *
			pStructClass = pPtrClass->GetBufferType().GetStructureClass() ;
		if ( pStructClass == nullptr )
		{
			return	nullptr ;
		}
		return	pStructClass ;
	}
	return	pClass ;
}

// 二つのクラスの共通クラスを取得する
//////////////////////////////////////////////////////////////////////////////
LClass * LCompiler::GetCommonClasses( LClass * pClass1, LClass * pClass2 ) const
{
	if ( pClass1 == nullptr )
	{
		return	pClass2 ;
	}
	if ( pClass2 == nullptr )
	{
		return	pClass1 ;
	}
	if ( pClass2->IsInstanceOf( pClass1 ) )
	{
		return	pClass1 ;
	}
	if ( pClass1->IsInstanceOf( pClass2 ) )
	{
		return	pClass2 ;
	}
	for ( ; ; )
	{
		pClass1 = pClass1->GetSuperClass() ;
		if ( pClass1 == nullptr )
		{
			break ;
		}
		if ( pClass2->IsInstanceOf( pClass1 ) )
		{
			return	pClass1 ;
		}
	}
	return	m_vm.GetObjectClass() ;
}

// 現在の文脈から見える名前空間をリストに追加する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::AddScopeToNamespaceList
	( LNamespaceList& nslNamespace, const LNamespaceList * pnslLocal ) const
{
	// ローカル空間
	if ( m_ctx != nullptr )
	{
		CodeNestPtr	pNest = m_ctx->m_curNest ;
		while ( pNest != nullptr )
		{
			if ( (pNest->m_type == CodeNest::ctrlWith)
					&& (pNest->m_xvalObj != nullptr) )
			{
				LClass *	pClass = GetExprVarClassOf( pNest->m_xvalObj ) ;
				if ( pClass != nullptr )
				{
					nslNamespace.AddNamespace( pClass ) ;
				}
			}
			if ( pNest->m_namespace != nullptr )
			{
				nslNamespace.AddNamespace( pNest->m_namespace.Ptr() ) ;
			}
			nslNamespace.AddNamespaceList( pNest->m_nslUsing );

			pNest = pNest->m_prev ;
		}

		// this クラス
		if ( m_ctx->m_proto->GetThisClass() != nullptr )
		{
			nslNamespace.AddNamespace( m_ctx->m_proto->GetThisClass() ) ;
		}
	}
	if ( pnslLocal != nullptr )
	{
		nslNamespace += *pnslLocal ;
	}
}

// ローカル／スタック上での型に変換する（構造体の場合にポインタにする）
//////////////////////////////////////////////////////////////////////////////
LType LCompiler::GetLocalTypeOf( const LType& type, bool constModify ) const
{
	if ( type.IsStructure() )
	{
		return	LType( m_vm.GetPointerClassAs( LType(type, constModify) ) ) ;
	}
	return	LType( type, constModify ) ;
}

// 引数での型に変換する（データ配列や構造体の場合にポインタにする）
//////////////////////////////////////////////////////////////////////////////
LType LCompiler::GetArgumentTypeOf( const LType& type, bool constModify ) const
{
	if ( type.IsStructure() )
	{
		return	LType( m_vm.GetPointerClassAs( LType(type, constModify) ) ) ;
	}
	if ( type.IsDataArray() )
	{
		LDataArrayClass *	pArrayClass = type.GetDataArrayClass() ;
		assert( pArrayClass != nullptr ) ;
		return	LType( m_vm.GetPointerClassAs
					( LType(pArrayClass->GetElementType(), constModify) ) ) ;
	}
	return	LType( type, constModify ) ;
}

// アクセス可能なスコープを取得
//////////////////////////////////////////////////////////////////////////////
LType::AccessModifier LCompiler::GetAccessibleScope( LClass * pClass )
{
	LClass *	pThisClass = GetThisClass() ;
	if ( pThisClass != nullptr )
	{
		if ( pClass == pThisClass )
		{
			return	LType::modifierPrivate ;
		}
		if ( (pClass != nullptr)
			&& pThisClass->IsInstanceOf( pClass ) )
		{
			return	LType::modifierProtected ;
		}
	}
	return	LType::modifierPublic ;
}

// アクセス可能か？
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::IsAccessibleTo( LClass * pClass, LType::AccessModifier accMode )
{
	return	(accMode <= GetAccessibleScope(pClass)) ;
}

// expr[, expr [, ...]] リスト（主に関数引数）評価
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::ParseExprList
	( LStringParser& sparsExpr,
		std::vector<LExprValuePtr>& xvalList,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	if ( !sparsExpr.PassSpace()
		|| LStringParser::IsCharContained
			( sparsExpr.CurrentChar(), pwszEscChars ) )
	{
		return	true ;
	}
	while ( sparsExpr.PassSpace() )
	{
		if ( LStringParser::IsCharContained
				( sparsExpr.CurrentChar(), pwszEscChars ) )
		{
			return	true ;
		}
		LExprValuePtr	xval =
			EvaluateExpression
				( sparsExpr, pnslLocal, pwszEscChars, Symbol::priorityList ) ;
		if ( HasErrorOnCurrent() )
		{
			return	false ;
		}
		assert( xval != nullptr ) ;
		xvalList.push_back( xval ) ;
		//
		if ( sparsExpr.HasNextChars( L"," ) != L',' )
		{
			if ( sparsExpr.PassSpace() 
				&& LStringParser::IsCharContained
					( sparsExpr.CurrentChar(), pwszEscChars ) )
			{
				break;
			}
			OnError( errorNoSeparationArgmentList ) ;
			return	false ;

		}
	}
	return	true ;
}

// リストの型情報を取得
//////////////////////////////////////////////////////////////////////////////
void LCompiler::GetExprListTypes
	( LArgumentListType& argListType,
		std::vector<LExprValuePtr>& xvalList ) const
{
	argListType.clear() ;
	for ( size_t i = 0; i < xvalList.size(); i ++ )
	{
		argListType.push_back( xvalList.at(i)->GetType() ) ;
	}
}

// リストが全て定数式か？
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::IsExprListConstExpr( std::vector<LExprValuePtr>& xvalList ) const
{
	for ( size_t i = 0; i < xvalList.size(); i ++ )
	{
		if ( !xvalList.at(i)->IsConstExpr() )
		{
			return	false ;
		}
	}
	return	true ;
}

// function 式 : function[capture-list](arglist)class:ret-type{statement-list}
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseFunctionExpr
	( LStringParser& sparsExpr, const LNamespaceList * pnslLocal )
{
	// キャプチャーリスト
	std::vector<LString>		vecCaptureNames ;
	std::vector<LExprValuePtr>	xvalCaptureList ;
	if ( sparsExpr.HasNextChars( L"[" ) == L'[' )
	{
		if ( !ParseFunctionCaptureList
			( sparsExpr, vecCaptureNames, xvalCaptureList, pnslLocal ) )
		{
			return	std::make_shared<LExprValue>() ;
		}
	}

	// 引数リスト
	std::shared_ptr<LPrototype>
				pPrototype = std::make_shared<LPrototype>() ;
	if ( sparsExpr.HasNextChars( L"(" ) != L'(' )
	{
		OnError( errorNoArgmentOfFunction,
					GetOperatorDesc(Symbol::opFunction).pwszName ) ;
		return	std::make_shared<LExprValue>() ;
	}
	ParsePrototypeArgmentList
			( sparsExpr, *pPrototype, pnslLocal, L")};" ) ;
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L")", L"};" ) ;
		return	std::make_shared<LExprValue>() ;
	}
	if ( sparsExpr.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		return	std::make_shared<LExprValue>() ;
	}

	// this クラス
	const bool	constThis = sparsExpr.HasNextToken( L"const" ) ;
	LClass *	pThisClass = nullptr ;
	{
		size_t		iThisClass = sparsExpr.GetIndex() ;
		LString		strThisClass ;
		if ( sparsExpr.NextToken( strThisClass ) == LStringParser::tokenSymbol )
		{
			LExprValuePtr	xval =
				GetExprSymbolAs( strThisClass.c_str(), &sparsExpr, pnslLocal ) ;
			if ( (xval != nullptr)
				&& xval->IsConstExprClass() )
			{
				pThisClass = xval->GetConstExprClass() ;
			}
			else
			{
				OnError( errorUndefinedClassName, strThisClass.c_str() ) ;
				return	std::make_shared<LExprValue>() ;
			}
		}
		else
		{
			if ( constThis )
			{
				OnError( errorSyntaxError, L"function()const" ) ;
				return	std::make_shared<LExprValue>() ;
			}
			sparsExpr.SeekIndex( iThisClass ) ;
		}
	}
	if ( pThisClass != nullptr )
	{
		pPrototype->SetThisClass( pThisClass, constThis ) ;
	}

	// 返り値型
	if ( sparsExpr.HasNextChars( L":" ) == L':' )
	{
		LNamespaceList	nslNamespace ;
		AddScopeToNamespaceList( nslNamespace, pnslLocal ) ;
		sparsExpr.NextTypeExpr
			( pPrototype->ReturnType(), true, m_vm, &nslNamespace ) ;
	}

	// 関数インスタンスを生成
	return	ExprFunctionInstance
				( sparsExpr, pPrototype,
					vecCaptureNames, xvalCaptureList, pnslLocal ) ;
}

bool LCompiler::ParseFunctionCaptureList
	( LStringParser& sparsExpr,
		std::vector<LString>& vecCaptureNames,
		std::vector<LExprValuePtr>& xvalCaptureList,
		const LNamespaceList * pnslLocal )
{
	LString	strCaptureName ;
	if ( sparsExpr.HasNextChars( L"]" ) != L']' )
	for ( ; ; )
	{
		LExprValuePtr	xvalObj ;
		if ( sparsExpr.NextToken( strCaptureName ) == LStringParser::tokenSymbol )
		{
			xvalObj = GetExprSymbolAs
					( strCaptureName.c_str(), &sparsExpr, pnslLocal ) ;
			if ( xvalObj != nullptr )
			{
				vecCaptureNames.push_back( strCaptureName ) ;
				xvalCaptureList.push_back
						( EvalLoadToDiscloseRef( std::move(xvalObj) ) ) ;
			}
			else
			{
				OnError( errorUndefinedSymbol, strCaptureName.c_str() ) ;
				break ;
			}
		}
		else
		{
			OnError( errorSyntaxError ) ;
			break ;
		}
		wchar_t	wch = sparsExpr.HasNextChars( L",]" ) ;
		if ( wch == L']' )
		{
			break ;
		}
		if ( wch != L',' )
		{
			OnError( errorCaptureNameMustBeOneSymbol ) ;
			break ;
		}
	}
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L"]", L"};" ) ;
		return	false ;
	}
	return	true ;
}

LExprValuePtr LCompiler::ExprFunctionInstance
	( LStringParser& sparsExpr,
		std::shared_ptr<LPrototype> pPrototype,
		std::vector<LString>& vecCaptureNames,
		std::vector<LExprValuePtr> xvalCaptureList,
		const LNamespaceList * pnslLocal )
{
	// 関数プロトタイプのクラスを作成する
	LClass *	pProtoClass = m_vm.GetFunctionClassAs( pPrototype ) ;

	// キャプチャーリストを含んだ関数プロトタイプを作成する
	std::shared_ptr<LPrototype>	pProtoFunc =
				std::make_shared<LPrototype>( *pPrototype ) ;

	assert( vecCaptureNames.size() == xvalCaptureList.size() ) ;
	for ( size_t i = 0; i < vecCaptureNames.size(); i ++ )
	{
		pProtoFunc->AddCaptureObjectType
			( vecCaptureNames.at(i).c_str(),
				xvalCaptureList.at(i)->GetType() ) ;
	}

	// 関数実装
	if ( sparsExpr.HasNextChars( L"{" ) != L'{' )
	{
		OnError( errorNoStatementsOfFunction,
					GetOperatorDesc(Symbol::opFunction).pwszName ) ;
		return	std::make_shared<LExprValue>() ;
	}
	std::shared_ptr<LCodeBuffer>	codeBuf = std::make_shared<LCodeBuffer>() ;
	codeBuf->AttachSourceFile
			( m_vm.SourceProducer().GetSafeSource( &sparsExpr ) ) ;
	pProtoFunc->SetFuncCode( codeBuf, 0 ) ;

	ContextPtr	ctxFunc = BeginFunctionBlock( pProtoFunc, codeBuf ) ;

	LNamespaceList	nslNamespace ;
	AddScopeToNamespaceList( nslNamespace, pnslLocal ) ;

	ParseStatementList( sparsExpr, &nslNamespace, L"}" ) ;

	EndFunctionBlock( ctxFunc ) ;

	if ( sparsExpr.HasNextChars( L"}" ) != L'}' )
	{
		OnError( errorMismatchBraces ) ;
		sparsExpr.PassStatement( L"}" ) ;
		return	std::make_shared<LExprValue>() ;
	}

	// 関数オブジェクト構築
	LClass *	pFuncClass =
		m_vm.GetFunctionClassAs
			( pProtoFunc, dynamic_cast<LFunctionClass*>( pProtoClass ) ) ;
	LExprValuePtr	xval = ExprLoadNewObject( pFuncClass ) ;

	// 関数オブジェクトの構築関数取得（キャプチャーオブジェクトを渡す）
	LArgumentListType	argCaptureList ;
	GetExprListTypes( argCaptureList, xvalCaptureList ) ;

	ssize_t	iInitFunc =
		pFuncClass->GetVirtualVector().FindCallableFunction
			( LClass::s_Constructor, argCaptureList,
				pFuncClass, GetAccessibleScope(pFuncClass) ) ;
	if ( iInitFunc < 0 )
	{
		OnError( errorNotFoundMatchConstructor,
					pFuncClass->GetFullClassName().c_str(),
					argCaptureList.ToString().c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}
	LPtr<LFunctionObj>	pInitFunc =
		pFuncClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
	assert( pInitFunc != nullptr ) ;

	// 構築関数を呼び出す
	ExprCallFunction( xval, pInitFunc, xvalCaptureList ) ;

	return	xval ;
}

// オブジェクト構築 : Type(arg-list)
// 配列知テラル構築 : Array<Type>[ ... ]
// 辞書配列リテラル構築 : Map<Type>{ ... }
// タスクリテラル構築 : Task<Type>[]{ ... }
// スレッドリテラル構築 : Thread<Type>[]{ ... }
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseTypeConstruction
	( LStringParser& sparsExpr,
		LClass * pTypeClass, const LNamespaceList * pnslLocal )
{
	assert( pTypeClass != nullptr ) ;
	if ( ((pTypeClass == m_vm.GetArrayClass())
			|| (pTypeClass == m_vm.GetMapClass()))
		&& (sparsExpr.HasNextChars( L"<" ) == L'<') )
	{
		// 要素型解釈
		LClass *	pElementType = nullptr ;
		LString		strElementType ;
		if ( sparsExpr.NextToken( strElementType ) == LStringParser::tokenSymbol )
		{
			LExprValuePtr	xvalElementType =
					GetExprSymbolAs
						( strElementType.c_str(), &sparsExpr, pnslLocal ) ;
			if ( (xvalElementType != nullptr)
				&& xvalElementType->IsConstExprClass() )
			{
				pElementType = xvalElementType->GetConstExprClass() ;
			}
		}
		if ( (strElementType.Find( L">" ) < 0)
			&& (sparsExpr.HasNextChars( L">" ) != L'>') )
		{
			OnError( errorMismatchAngleBrackets ) ;
			return	std::make_shared<LExprValue>() ;
		}
		if ( pElementType == nullptr )
		{
			if ( pTypeClass == m_vm.GetArrayClass() )
			{
				OnError( errorInvalidArrayElementType,
							nullptr, strElementType.c_str() ) ;
			}
			else
			{
				OnError( errorInvalidMapElementType,
							nullptr, strElementType.c_str() ) ;
			}
			return	std::make_shared<LExprValue>() ;
		}
		sparsExpr.PassSpace() ;
		//
		if ( pTypeClass == m_vm.GetArrayClass() )
		{
			if ( sparsExpr.CurrentChar() == L'[' )
			{
				// Array<Type>[ ... ]
				return	ParseArrayLiteral( sparsExpr, pnslLocal, pElementType ) ;
			}
			// Array<Type>
			LClass *	pArrayClass = m_vm.GetArrayClassAs( pElementType ) ;
			assert( pArrayClass != nullptr ) ;
			return	ParseTypeConstruction( sparsExpr, pArrayClass, pnslLocal ) ;
		}
		else
		{
			assert( pTypeClass == m_vm.GetMapClass() ) ;
			if ( sparsExpr.CurrentChar() == L'{' )
			{
				// Map<Type>{ ... }
				return	ParseMapLiteral( sparsExpr, pnslLocal, pElementType ) ;
			}
			// Map<Type>
			LClass *	pMapClass = m_vm.GetMapClassAs( pElementType ) ;
			assert( pMapClass != nullptr ) ;
			return	ParseTypeConstruction( sparsExpr, pMapClass, pnslLocal ) ;
		}
	}
	else if ( (pTypeClass == m_vm.GetTaskClass())
				|| (pTypeClass == m_vm.GetThreadClass()) )
	{
		// 返り値型解釈
		LType	typeRet ;
		if ( sparsExpr.HasNextChars( L"<" ) == L'<' )
		{
			if ( !sparsExpr.NextTypeExpr
					( typeRet, true, m_vm, pnslLocal, 0, true, false ) )
			{
				return	std::make_shared<LExprValue>() ;
			}
			if ( sparsExpr.HasNextChars( L">" ) != L'>' )
			{
				OnError( errorMismatchAngleBrackets ) ;
				return	std::make_shared<LExprValue>() ;
			}
			if ( pTypeClass == m_vm.GetTaskClass() )
			{
				pTypeClass = m_vm.GetTaskClassAs( typeRet ) ;
			}
			else
			{
				assert( pTypeClass == m_vm.GetThreadClass() ) ;
				pTypeClass = m_vm.GetThreadClassAs( typeRet ) ;
			}
		}

		// キャプチャーリスト、又は関数実装判定
		bool	flagTaskInstance = false ;
		size_t	iSaveIndex = sparsExpr.GetIndex() ;
		if ( sparsExpr.HasNextChars( L"[" ) == L'[' )
		{
			if ( sparsExpr.PassStatement( L"]" ) == L']' )
			{
				flagTaskInstance = (sparsExpr.HasNextChars( L"{" ) == L'{') ;
			}
		}
		else
		{
			flagTaskInstance = (sparsExpr.HasNextChars( L"{" ) == L'{') ;
		}
		sparsExpr.SeekIndex( iSaveIndex ) ;

		if ( flagTaskInstance )
		{
			// Task/Thread インスタンス作成
			LExprValuePtr	xvalTask = ExprLoadNewObject( pTypeClass ) ;

			// キャプチャーリスト解釈
			std::vector<LString>		vecCaptureNames ;
			std::vector<LExprValuePtr>	xvalCaptureList ;
			if ( sparsExpr.HasNextChars( L"[" ) == L'[' )
			{
				if ( !ParseFunctionCaptureList
					( sparsExpr, vecCaptureNames, xvalCaptureList, pnslLocal ) )
				{
					return	std::make_shared<LExprValue>() ;
				}
			}

			// 関数プロトタイプ
			std::shared_ptr<LPrototype>
					pPrototype = std::make_shared<LPrototype>() ;
			pPrototype->ReturnType() = typeRet ;

			// 関数インスタンス作成
			LExprValuePtr	xvalFunc =
				ExprFunctionInstance
					( sparsExpr, pPrototype,
						vecCaptureNames, xvalCaptureList, pnslLocal ) ;

			// begin 関数呼び出し
			std::vector<LExprValuePtr>	xvalList ;
			xvalList.push_back( std::move(xvalFunc) ) ;

			LArgumentListType	argListType ;
			GetExprListTypes( argListType, xvalList ) ;

			ssize_t	iBeginFunc =
				pTypeClass->GetVirtualVector().FindCallableFunction
					( L"begin", argListType,
						pTypeClass, GetAccessibleScope(pTypeClass) ) ;
			if ( iBeginFunc < 0 )
			{
				OnError( errorNotFoundFunction, L"begin" ) ;
			}
			else
			{
				LPtr<LFunctionObj>	pBeginFunc =
						pTypeClass->GetVirtualVector().
								GetFunctionAt( (size_t) iBeginFunc ) ;
				assert( pBeginFunc != nullptr ) ;

				ExprCallFunction( xvalTask, pBeginFunc, xvalList ) ;
			}

			return	xvalTask ;
		}
	}

	LType	type( pTypeClass ) ;
	ParseTypeObjectArray( sparsExpr, type ) ;
	pTypeClass = type.GetClass() ;

	if ( sparsExpr.HasNextChars( L"(" ) == L'(' )
	{
		// Type( arg-list )
		std::vector<LExprValuePtr>	xvalList ;
		if ( !ParseExprList( sparsExpr, xvalList, pnslLocal, L")};" ) )
		{
			assert( HasErrorOnCurrent() ) ;
			sparsExpr.PassStatement( L")", L"};" ) ;
			return	std::make_shared<LExprValue>() ;
		}
		if ( sparsExpr.HasNextChars( L")" ) != L')' )
		{
			OnError( errorMismatchParentheses ) ;
			return	std::make_shared<LExprValue>() ;
		}

		// 構築関数を探す
		LArgumentListType	argListType ;
		GetExprListTypes( argListType, xvalList ) ;
		//
		LPtr<LFunctionObj>	pInitFunc ;
		//
		ssize_t	iInitFunc =
			pTypeClass->GetVirtualVector().FindCallableFunction
				( LClass::s_Constructor, argListType,
					pTypeClass, GetAccessibleScope(pTypeClass) ) ;
		if ( iInitFunc < 0 )
		{
			if ( xvalList.size() > 0 )
			{
				OnError( errorNotFoundMatchConstructor,
							pTypeClass->GetFullClassName().c_str(),
							argListType.ToString().c_str() ) ;
				return	std::make_shared<LExprValue>() ;
			}
		}
		else
		{
			pInitFunc = pTypeClass->GetVirtualVector().
									GetFunctionAt( (size_t) iInitFunc ) ;
			assert( pInitFunc != nullptr ) ;
		}

		// オブジェクトを生成
		LExprValuePtr		xval ;
		LStructureClass *	pStructClass =
								dynamic_cast<LStructureClass*>( pTypeClass ) ;
		if ( pStructClass != nullptr )
		{
			// 構造体の場合にはポインタを生成してバッファを確保する
			LType	typeStruct( pTypeClass ) ;
			xval = ExprLoadNewObject( m_vm.GetPointerClassAs( typeStruct ) ) ;
			ExprCodeAllocBuffer( xval, LExprValue::MakeConstExprInt
											( typeStruct.GetDataBytes() ) ) ;
		}
		else
		{
			// オブジェクトを生成
			xval = ExprLoadNewObject( pTypeClass ) ;
		}

		// 構築関数を呼び出す
		if ( pInitFunc != nullptr )
		{
			ExprCallFunction( xval, pInitFunc, xvalList ) ;
		}

		return	xval ;
	}
	// Type
	pTypeClass->AddRef() ;
	return	std::make_shared<LExprValue>
				( LType(pTypeClass->GetClass()), pTypeClass, true, true ) ;
}

// オブジェクト型の配列修飾
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseTypeObjectArray( LStringParser& sparsExpr, LType& type )
{
	if ( type.CanArrangeOnBuf() )
	{
		if ( sparsExpr.HasNextChars( L"*" ) == L'*' )
		{
			type = LType( m_vm.GetPointerClassAs( type ) ) ;
		}
	}
	if ( !type.CanArrangeOnBuf() )
	{
		while ( sparsExpr.HasNextChars( L"[" ) == L'[' )
		{
			if ( type.IsConst() )
			{
				OnWarning
					( errorCannotConstForArrayElement,
						warning1, nullptr, type.GetTypeName().c_str() ) ;
				type.SetModifiers( 0 ) ;
			}
			type = LType( m_vm.GetArrayClassAs( type.GetClass() ) ) ;
			//
			if ( sparsExpr.HasNextChars( L"]" ) != L']' )
			{
				OnError( errorMismatchBrackets ) ;
				break ;
			}
		}
	}
}

// new 演算子 : new Type[]...(arg-list)
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseNewOperator
	( LStringParser& sparsExpr, const LNamespaceList * pnslLocal )
{
	// 配列修飾を含まない型
	LType	typeNew ;
	if ( !sparsExpr.NextTypeExpr( typeNew, true, m_vm, pnslLocal, 0, false ) )
	{
		return	std::make_shared<LExprValue>() ;
	}

	if ( typeNew.CanArrangeOnBuf() )
	{
		//
		// メモリを確保する : new Type[][]...( arg-list )
		//
		LExprValuePtr	xvalNewLength = LExprValue::MakeConstExprInt( 1 ) ;
		const LType		typeNewElement = typeNew ;
		size_t			nSubElementCount = 1 ;
		if ( sparsExpr.HasNextChars( L"[" ) == L'[' )
		{
			// 確保配列サイズ評価
			xvalNewLength = EvaluateExpression( sparsExpr, pnslLocal, L"]};" ) ;
			if ( HasErrorOnCurrent() )
			{
				return	std::make_shared<LExprValue>() ;
			}
			if ( sparsExpr.HasNextChars( L"]" ) != L']' )
			{
				OnError( errorMismatchBrackets ) ;
				return	std::make_shared<LExprValue>() ;
			}
			assert( xvalNewLength != nullptr ) ;
			if ( !xvalNewLength->GetType().IsInteger()
				|| (xvalNewLength->IsConstExpr()
					&& (xvalNewLength->AsInteger() < 0)) )
			{
				OnError( errorInvalidArrayLength ) ;
				return	std::make_shared<LExprValue>() ;
			}

			// 配列型解釈 : []...
			std::vector<size_t>	dimArray ;
			while ( sparsExpr.HasNextChars( L"[" ) == L'[' )
			{
				LExprValuePtr	xvalSize =
					EvaluateExpression( sparsExpr, pnslLocal, L"]};" ) ;
				if ( HasErrorOnCurrent() )
				{
					return	std::make_shared<LExprValue>() ;
				}
				if ( sparsExpr.HasNextChars( L"]" ) != L']' )
				{
					OnError( errorMismatchBrackets ) ;
					return	std::make_shared<LExprValue>() ;
				}
				assert( xvalSize != nullptr ) ;
				if ( !xvalSize->GetType().IsInteger()
					|| !xvalSize->IsConstExpr()
					|| (xvalSize->AsInteger() < 0) )
				{
					OnError( errorInvalidArrayLength ) ;
					return	std::make_shared<LExprValue>() ;
				}
				nSubElementCount *= (size_t) xvalSize->AsInteger() ;
				dimArray.push_back( (size_t) xvalSize->AsInteger() ) ;
			}
			if ( dimArray.size() >= 1 )
			{
				typeNew = LType( m_vm.GetDataArrayClassAs( typeNew, dimArray ) ) ;
			}
		}

		// バイト数を計算する
		LExprValuePtr	xvalBufBytes = xvalNewLength ;
		const size_t	nDataStride = typeNew.GetDataBytes() ;
		if ( typeNew.GetDataBytes() != 1 )
		{
			xvalBufBytes =
				EvalBinaryOperator
					( std::move(xvalBufBytes), Symbol::opMul,
						LExprValue::MakeConstExprInt( (LLong) nDataStride ), pnslLocal ) ;
		}

		// バッファを確保しポインタを返す
		LExprValuePtr	xval =
			ExprLoadNewObject( m_vm.GetPointerClassAs( typeNew ) ) ;
		ExprCodeAllocBuffer( xval, std::move(xvalBufBytes) ) ;

		// 初期化引数 : ( [<expr> [, <expr> ...]] )
		if ( sparsExpr.HasNextChars( L"(" ) != L'(' )
		{
			// 初期化引数はないので new したままを返す
			return	xval ;
		}
		std::vector<LExprValuePtr>	xvalList ;
		if ( !ParseExprList( sparsExpr, xvalList, pnslLocal, L")};" ) )
		{
			assert( HasErrorOnCurrent() ) ;
			sparsExpr.PassStatement( L")", L"};" ) ;
			return	xval ;
		}
		if ( sparsExpr.HasNextChars( L")" ) != L')' )
		{
			OnError( errorMismatchParentheses ) ;
			return	xval ;
		}
		if ( !MustBeRuntimeExpr() )
		{
			return	xval ;
		}

		// ポインタのインクリメント演算子を取得する
		const Symbol::UnaryOperatorDef *	pbopPtrInc =
			LClass::FindMatchUnaryOperator
					( Symbol::opIncrement,
							LPointerClass::s_UnaryPtrOperatorDefs,
							LPointerClass::UnaryPtrOperatorCount ) ;
		assert( pbopPtrInc != nullptr ) ;

		LCodeBuffer::ImmediateOperand	immopStride ;
		immopStride.value.longValue = typeNewElement.GetDataBytes() ;

		LCodeBuffer::ImmediateOperand	immopPtrInc ;
		immopPtrInc.pfnOp1 = pbopPtrInc->m_pfnOp ;

		// 整数のデクリメント演算子を取得する
		const Symbol::UnaryOperatorDef *	pbopIntDec =
			LClass::FindMatchUnaryOperator
					( Symbol::opDecrement,
							LIntegerClass::s_UnaryOperatorDefs,
							LIntegerClass::UnaryOperatorCount ) ;
		assert( pbopIntDec != nullptr ) ;

		LCodeBuffer::ImmediateOperand	immopIntDec ;
		immopIntDec.pfnOp1 = pbopIntDec->m_pfnOp ;

		// 多次元配列の場合の真の要素数を xvalNewLength に計算する
		if ( nSubElementCount != 1 )
		{
			xvalNewLength =
				EvalBinaryOperator
					( std::move(xvalNewLength),
						Symbol::opMul,
						LExprValue::MakeConstExprInt(nSubElementCount) ) ;
		}

		// 確保した要素に順次構築関数を呼び出す
		LExprValuePtr		xvalTempPtr = ExprLoadClone( xval ) ;
		LArgumentListType	argListType ;
		GetExprListTypes( argListType, xvalList ) ;

		LStructureClass *	pStructClass = typeNewElement.GetStructureClass() ;
		if ( pStructClass != nullptr )
		{
			// 構造体の構築関数取得
			ssize_t	iInitFunc =
				pStructClass->GetVirtualVector().FindCallableFunction
					( LClass::s_Constructor, argListType,
						pStructClass, GetAccessibleScope(pStructClass) ) ;
			if ( iInitFunc < 0 )
			{
				OnError( errorNotFoundMatchConstructor,
							pStructClass->GetFullClassName().c_str(),
							argListType.ToString().c_str() ) ;
				return	xval ;
			}
			LPtr<LFunctionObj>	pInitFunc =
				pStructClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
			assert( pInitFunc != nullptr ) ;

			LExprValuePtr	xvalLoopCount = ExprMakeOnStack( std::move(xvalNewLength) ) ;

			// while ( xvalLoopCount != 0 )
			CodePointPtr	cpLoopStart = GetCurrentCodePointer( true ) ;
			LExprValuePtr	xvalCmp = EvalBinaryOperator
										( xvalLoopCount, Symbol::opGraterThan,
											LExprValue::MakeConstExprInt(0), pnslLocal ) ;
			CodePointPtr	cpBreakJump = ExprCodeJumpNonIf( std::move(xvalCmp) ) ;

			// 構築関数呼び出し
			ExprCallFunction( xvalTempPtr, pInitFunc, xvalList ) ;
			ExprCodeFreeTempStack() ;

			ssize_t	iStackTempPtr = GetBackIndexOnStack( xvalTempPtr ) ;
			ssize_t	iStackLoopCount = GetBackIndexOnStack( xvalLoopCount ) ;
			assert( iStackTempPtr >= 0 ) ;
			assert( iStackLoopCount >= 0 ) ;

			// ++ xvalTempPtr
			AddCode( LCodeBuffer::codeExImmPrefix,
						0, 0, LType::typeInt64, 0, &immopStride ) ;
			AddCode( LCodeBuffer::codeUnaryOperate,
						iStackTempPtr, 0,
						Symbol::opIncrement, 0, &immopPtrInc ) ;
			AddCode( LCodeBuffer::codeFreeObject, iStackTempPtr + 1 ) ;
			AddCode( LCodeBuffer::codeMove, 0, iStackTempPtr + 1, 0, 1 ) ;

			// -- xvalLoopCount
			AddCode( LCodeBuffer::codeUnaryOperate,
						iStackLoopCount, 0,
						Symbol::opDecrement, 0, &immopIntDec ) ;
			AddCode( LCodeBuffer::codeMove, 0, iStackLoopCount + 1, 0, 1 ) ;

			ExprCodeJump( cpLoopStart ) ;
			FixJumpDestination( cpBreakJump, GetCurrentCodePointer( true ) ) ;
			return	xval ;
		}
		if ( typeNewElement.IsPrimitive() )
		{
			// プリミティブ型配列は全要素を指定引数で初期化する
			if ( xvalList.size() != 1 )
			{
				OnError( errorNotFoundMatchConstructor,
							pStructClass->GetFullClassName().c_str(),
							argListType.ToString().c_str() ) ;
				return	xval ;
			}

			LExprValuePtr	xvalLoopCount = ExprMakeOnStack( std::move(xvalNewLength) ) ;

			// while ( xvalLoopCount != 0 )
			CodePointPtr	cpLoopStart = GetCurrentCodePointer( true ) ;
			LExprValuePtr	xvalCmp = EvalBinaryOperator
										( xvalLoopCount, Symbol::opGraterThan,
											LExprValue::MakeConstExprInt(0), pnslLocal ) ;
			CodePointPtr	cpBreakJump = ExprCodeJumpNonIf( std::move(xvalCmp) ) ;

			// 初期値ストア
			ExprStorePtrPrimitive
				( xvalTempPtr, nullptr,
					typeNewElement.GetPrimitive(), xvalList.at(0) ) ;
			ExprCodeFreeTempStack() ;

			ssize_t	iStackTempPtr = GetBackIndexOnStack( xvalTempPtr ) ;
			ssize_t	iStackLoopCount = GetBackIndexOnStack( xvalLoopCount ) ;
			assert( iStackTempPtr >= 0 ) ;
			assert( iStackLoopCount >= 0 ) ;

			// ++ xvalTempPtr
			AddCode( LCodeBuffer::codeExImmPrefix,
						0, 0, LType::typeInt64, 0, &immopStride ) ;
			AddCode( LCodeBuffer::codeUnaryOperate,
						iStackTempPtr, 0,
						Symbol::opIncrement, 0, &immopPtrInc ) ;
			AddCode( LCodeBuffer::codeFreeObject, iStackTempPtr + 1 ) ;
			AddCode( LCodeBuffer::codeMove, 0, iStackTempPtr + 1, 0, 1 ) ;

			// -- xvalLoopCount
			AddCode( LCodeBuffer::codeUnaryOperate,
						iStackLoopCount, 0,
						Symbol::opDecrement, 0, &immopIntDec ) ;
			AddCode( LCodeBuffer::codeMove, 0, iStackLoopCount + 1, 0, 1 ) ;

			ExprCodeJump( cpLoopStart ) ;
			FixJumpDestination( cpBreakJump, GetCurrentCodePointer( true ) ) ;
			return	xval ;
		}
		else
		{
			// それ以外は初期化引数は指定できない
			OnError( errorSyntaxError, nullptr,
						argListType.ToString().c_str() ) ;
			return	xval ;
		}
	}
	else
	{
		//
		// 配列型修飾
		//
		ParseTypeObjectArray( sparsExpr, typeNew ) ;
		//
		// オブジェクトを構築する : new Type( arg-list )
		//
		assert( !typeNew.IsPrimitive() ) ;
		LClass *	pNewClass = typeNew.GetClass() ;
		assert( pNewClass != nullptr ) ;

		LExprValuePtr	xval = ExprLoadNewObject( pNewClass ) ;

		// 初期化引数 : ( [<expr> [, <expr> ...]] )
		if ( sparsExpr.HasNextChars( L"(" ) != L'(' )
		{
			// デフォルトの構築関数取得
			LArgumentListType	argListType ;
			ssize_t	iInitFunc =
				pNewClass->GetVirtualVector().FindCallableFunction
					( LClass::s_Constructor, argListType,
						pNewClass, GetAccessibleScope(pNewClass) ) ;
			if ( iInitFunc >= 0 )
			{
				// デフォルトの構築関数を呼び出す
				LPtr<LFunctionObj>	pInitFunc =
					pNewClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
				assert( pInitFunc != nullptr ) ;

				std::vector<LExprValuePtr>	xvalList ;
				ExprCallFunction( xval, pInitFunc, xvalList ) ;
			}
			return	xval ;
		}

		// 構築関数引数解釈
		std::vector<LExprValuePtr>	xvalList ;
		if ( !ParseExprList( sparsExpr, xvalList, pnslLocal, L")};" ) )
		{
			assert( HasErrorOnCurrent() ) ;
			sparsExpr.PassStatement( L")", L"};" ) ;
			return	xval ;
		}
		if ( sparsExpr.HasNextChars( L")" ) != L')' )
		{
			OnError( errorMismatchParentheses ) ;
			return	xval ;
		}
		if ( !MustBeRuntimeExpr() )
		{
			return	xval ;
		}
		LArgumentListType	argListType ;
		GetExprListTypes( argListType, xvalList ) ;

		// 構築関数取得
		ssize_t	iInitFunc =
			pNewClass->GetVirtualVector().FindCallableFunction
				( LClass::s_Constructor, argListType,
					pNewClass, GetAccessibleScope(pNewClass) ) ;
		if ( iInitFunc < 0 )
		{
			if ( xvalList.size() > 0 )
			{
				OnError( errorNotFoundMatchConstructor,
							pNewClass->GetFullClassName().c_str(),
							argListType.ToString().c_str() ) ;
			}
			return	xval ;
		}
		LPtr<LFunctionObj>	pInitFunc =
			pNewClass->GetVirtualVector().GetFunctionAt( (size_t) iInitFunc ) ;
		assert( pInitFunc != nullptr ) ;

		ExprCallFunction( xval, pInitFunc, xvalList ) ;

		return	xval ;
	}
}

// sizeof 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseSizeOfOperator
	( LStringParser& sparsExpr,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	const size_t	iSaveIndex = sparsExpr.GetIndex() ;
	if ( sparsExpr.HasNextChars( L"(" ) == L'(' )
	{
		// sizeof(type-expr)
		LType	type ;
		if ( sparsExpr.NextTypeExpr( type, false, m_vm, pnslLocal ) )
		{
			if ( sparsExpr.HasNextChars( L")" ) != L')' )
			{
				OnError( errorMismatchParentheses ) ;
				sparsExpr.PassStatement( L")", L"};" ) ;
				return	LExprValue::MakeConstExprInt(0) ;
			}
			return	EvalSizeOfType( type ) ;
		}
	}
	sparsExpr.SeekIndex( iSaveIndex ) ;

	// sizeof expr
	LExprValuePtr	xval =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(Symbol::opSizeOf).priorityUnary ) ;
	if ( HasErrorOnCurrent() )
	{
		return	LExprValue::MakeConstExprInt(0) ;
	}
	return	ExprSizeOfExpr( std::move(xval) ) ;
}

LExprValuePtr LCompiler::EvalSizeOfType( const LType& type )
{
	if ( type.CanArrangeOnBuf() )
	{
		return	LExprValue::MakeConstExprInt( type.GetDataBytes() ) ;
	}
	else
	{
		assert( type.IsRuntimeObject() ) ;
		OnError( errorInvalidSizeofObject, type.GetTypeName().c_str() ) ;
		return	LExprValue::MakeConstExprInt(0) ;
	}
}

LExprValuePtr LCompiler::ExprSizeOfExpr( LExprValuePtr xval )
{
	if ( xval->IsConstExprClass() )
	{
		return	EvalSizeOfType( LType( xval->GetConstExprClass() ) ) ;
	}
	if ( xval->GetType().CanArrangeOnBuf() )
	{
		return	LExprValue::MakeConstExprInt( xval->GetType().GetDataBytes() ) ;
	}
	if ( xval->IsConstExpr() )
	{
		// 定数式
		LObjPtr	pObj = xval->GetObject() ;
		if ( pObj == nullptr )
		{
			OnError( exceptionNullPointer ) ;
			return	LExprValue::MakeConstExprInt(0) ;
		}
		return	LExprValue::MakeConstExprInt( pObj->GetElementCount() ) ;
	}
	else if ( xval->GetType().IsRuntimeObject() )
	{
		// オブジェクトへの sizeof
		return	EvalCodeUnaryOperate
					( LType(LType::typeUint64), std::move(xval),
						Symbol::opSizeOf,
						&LClass::operator_sizeof, nullptr, nullptr ) ;
	}
	else
	{
		OnError( errorInvalidSizeofObject,
					xval->GetType().GetTypeName().c_str() ) ;
		return	LExprValue::MakeConstExprInt(0) ;
	}
}

// 前置 :: 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseGlobalMemberOf( LStringParser& sparsExpr )
{
	return	ParseOperatorStaticMemberOf
				( sparsExpr, std::make_shared<LExprValue>( m_vm.Global() ) ) ;
}

// 単項演算
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalUnaryOperator
	( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval )
{
	if ( xval == nullptr )
	{
		return	nullptr;
	}
	xval = EvalMakeInstanceIfConstExpr( std::move(xval) ) ;

	if ( (opIndex == Symbol::opMul) && !unaryPost )
	{
		// ポインタ参照 *ptr 演算子
		return	EvalPrimitiveUnaryRefPtr( std::move( xval) ) ;
	}
	if ( xval->GetType().IsPrimitive() )
	{
		// プリミティブ型
		return	EvalPrimitiveUnaryOperator
						( opIndex, unaryPost, std::move(xval) ) ;
	}
	else if ( xval->GetType().IsPointer() )
	{
		// ポインタ型
		return	EvalPointerUnaryOperator
						( opIndex, unaryPost, std::move(xval) ) ;
	}
	else if ( xval->GetType().IsStructure() )
	{
		// 構造体への参照の場合には構造体への演算子を実行可能
		if ( xval->IsRefPointer() )
		{
			return	EvalStructUnaryOperator
						( opIndex, unaryPost, std::move(xval) ) ;
		}
	}
	if ( xval->GetType().IsDataArray()
		|| xval->GetType().IsStructure() )
	{
		// データ配列への直接の演算は不可
		OnError( errorNotDefinedOperator_opt2,
					xval->GetType().GetTypeName().c_str(),
					GetOperatorDesc(opIndex).pwszName ) ;
		return	xval ;
	}
	// オブジェクト
	return	EvalObjectUnaryOperator( opIndex, unaryPost, std::move(xval) ) ;
}

// ポインタ参照 *ptr 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalPrimitiveUnaryRefPtr( LExprValuePtr xval )
{
	xval = EvalLoadToDiscloseRef( std::move(xval) ) ;

	if ( !xval->GetType().IsPointer() )
	{
		OnError( errorInvalidOperator, nullptr, L"*" ) ;
		return	xval ;
	}
	const LType		typeElement = xval->GetType().GetRefElementType().
												ConstWith( xval->GetType() ) ;
	LExprValuePtr	xvalRef =
		std::make_shared<LExprValue>
			( typeElement, xval->GetObject(),
				xval->IsConstExpr(), xval->IsUniqueObject() ) ;
	xvalRef->SetOptionRefPointerOffset( xval, LExprValue::MakeConstExprInt(0) ) ;
	return	xvalRef ;
}

// プリミティブ型の単項演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalPrimitiveUnaryOperator
	( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval )
{
	assert( xval != nullptr ) ;

	const Symbol::UnaryOperatorDef *	puopDef = nullptr ;
	if ( xval->GetType().IsInteger()
		|| xval->GetType().IsBoolean() )
	{
		// 整数型の演算子取得
		puopDef = LClass::FindMatchUnaryOperator
					( opIndex, LIntegerClass::s_UnaryOperatorDefs,
								LIntegerClass::UnaryOperatorCount ) ;
	}
	else if ( xval->GetType().IsFloatingPointNumber() )
	{
		// 浮動小数点型の演算子取得
		puopDef = LClass::FindMatchUnaryOperator
					( opIndex, LDoubleClass::s_UnaryOperatorDefs,
								LDoubleClass::UnaryOperatorCount ) ;
	}
	if ( puopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
				xval->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval ;
	}
	if ( GetOperatorDesc(opIndex).leftValue )
	{
		// エラー判定
		if ( !xval->IsReference() )
		{
			OnError( errorInvalidForLeftValue_opt1,
						GetOperatorDesc(opIndex).pwszName ) ;
			return	xval ;
		}
		if ( xval->GetType().IsConst() )
		{
			OnError( errorCannotConstVariable_opt1,
						GetOperatorDesc(opIndex).pwszName ) ;
			return	xval ;
		}
		assert( !xval->IsConstExpr() ) ;
	}
	else
	{
		// メモリ参照の場合、ロードする
		xval = EvalLoadToDiscloseRef( std::move(xval) ) ;
		assert( xval != nullptr ) ;
	}
	if ( xval->IsConstExpr() )
	{
		// 定数式で演算子実行
		assert( xval->IsSingle() ) ;
		assert( !xval->IsReference() ) ;
		//
		xval->Value() =
			(puopDef->m_pfnOp)( xval->Value(), puopDef->m_pInstance ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		if ( puopDef->m_typeRet.IsBoolean() )
		{
			xval->Type() = puopDef->m_typeRet ;
		}
		return	xval ;
	}
	else
	{
		// 実行時式で演算子実行
		LExprValuePtr		xvalBefore ;
		LType::Primitive	primType = xval->GetType().GetPrimitive() ;
		if ( unaryPost )
		{
			xvalBefore = ExprLoadClone( xval ) ;
		}
		LExprValuePtr	xvalLeft ;
		const LType		typeLeft = xval->GetType() ;
		if ( GetOperatorDesc(opIndex).leftValue )
		{
			xvalLeft = xval ;
			xval = ExprLoadClone( std::move(xval) ) ;
		}
		xval = EvalCodeUnaryOperate
					( typeLeft, std::move(xval),
						puopDef->m_opIndex,
						puopDef->m_pfnOp,
						puopDef->m_pInstance, puopDef->m_pOpFunc ) ;

		if ( GetOperatorDesc(opIndex).leftValue )
		{
			ExprCodeStore( std::move(xvalLeft), xval ) ;
		}
		xval = (unaryPost ? xvalBefore : xval) ;
		//
		if ( puopDef->m_typeRet.IsBoolean() )
		{
			xval->Type() = puopDef->m_typeRet ;
		}
		return	xval ;
	}
}

// ポインタ型の単項演算
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalPointerUnaryOperator
	( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval )
{
	assert( xval != nullptr ) ;

	LPointerClass *	pPtrClass = xval->GetType().GetPointerClass() ;
	assert( pPtrClass != nullptr ) ;
	if ( pPtrClass == nullptr )
	{
		OnError( errorExpressionError ) ;
		return	xval ;
	}
	const LValue::Primitive
		stride = LValue::MakeLong( pPtrClass->GetElementStride() ) ;

	// ポインタ演算子取得
	const Symbol::UnaryOperatorDef *	puopDef = nullptr ;
	bool								flagOffsetPointer = false ;
	if ( xval->IsConstExpr()
		|| (xval->IsOffsetPointer() && xval->IsPtrOffsetOnLocal()) )
	{
		puopDef = LClass::FindMatchUnaryOperator
					( opIndex, LPointerClass::s_UnaryOffsetOperatorDefs,
								LPointerClass::UnaryOffsetOperatorCount ) ;
		flagOffsetPointer = true ;
	}
	else
	{
		puopDef = LClass::FindMatchUnaryOperator
					( opIndex, LPointerClass::s_UnaryPtrOperatorDefs,
								LPointerClass::UnaryPtrOperatorCount ) ;
	}
	if ( puopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
				xval->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval ;
	}
	if ( GetOperatorDesc(opIndex).leftValue )
	{
		if ( xval->GetType().IsConst() )
		{
			// const 変数への操作エラー
			OnError( errorCannotConstVariable_opt1, GetOperatorDesc(opIndex).pwszName ) ;
		}
		if ( !xval->IsReference()
			&& ((xval->GetOption2() == nullptr)
				|| !xval->GetOption2()->IsOnLocal()) )
		{
			// 非左辺式への操作エラー
			OnError( errorInvalidForLeftValue_opt1, GetOperatorDesc(opIndex).pwszName ) ;
		}
	}

	if ( xval->IsConstExpr() )
	{
		// 定数式で演算子実行
		LPointerObj *	pPtrObj =
			dynamic_cast<LPointerObj*>( xval->GetObject().Ptr() ) ;
		assert( pPtrObj != nullptr ) ;
		if ( pPtrObj == nullptr )
		{
			OnError( errorExpressionError ) ;
			return	xval ;
		}
		LValue::Primitive	value = LValue::MakeLong( 0 ) ;
		//
		value = (puopDef->m_pfnOp)( value, stride.pVoidPtr ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		pPtrObj = new LPointerObj( *pPtrObj ) ;
		pPtrObj += (ssize_t) value.longValue ;
		return	std::make_shared<LExprValue>
					( xval->GetType(), pPtrObj, true, xval->IsUniqueObject() ) ;
	}
	else
	{
		// 実行時式で演算子実行
		LExprValuePtr	xvalBefore ;
		if ( unaryPost )
		{
			if ( flagOffsetPointer )
			{
				assert( xval->IsOffsetPointer() ) ;
				xvalBefore = std::make_shared<LExprValue>
								( xval->GetType(), nullptr, false, false ) ;
				xvalBefore->SetOptionPointerOffset
					( xval->GetOption1(), ExprLoadClone( xval->GetOption2() ) ) ;
			}
			else 
			{
				xvalBefore = ExprLoadClone( xval ) ;
			}
		}
		LExprValuePtr	xvalLeft ;
		LExprValuePtr	xvalStore ;
		if ( GetOperatorDesc(opIndex).leftValue )
		{
			if ( flagOffsetPointer )
			{
				xvalLeft = xval->GetOption2() ;
			}
			else
			{
				xvalLeft = xval ;
				xval = ExprLoadClone( std::move(xval) ) ;
			}
		}
		const LType	typePtr = xval->GetType() ;
		if ( flagOffsetPointer )
		{
			LExprValuePtr	xvalPtr = xval->GetOption1() ;
			xvalStore =
				EvalCodeUnaryOperate
						( LType(LType::typeInt64),
								EvalCastTypeTo( xval->GetOption2(),
												LType(LType::typeInt64) ),
								puopDef->m_opIndex,
								puopDef->m_pfnOp,
								stride.pVoidPtr, puopDef->m_pOpFunc ) ;
			xval = std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
			xval->SetOptionPointerOffset( xvalPtr, xvalStore ) ;
		}
		else
		{
			xval = EvalCodeUnaryOperate
						( typePtr, std::move(xval),
							puopDef->m_opIndex,
							puopDef->m_pfnOp,
							stride.pVoidPtr, puopDef->m_pOpFunc ) ;
			xvalStore = xval ;
		}
		if ( GetOperatorDesc(opIndex).leftValue )
		{
			ExprCodeStore( std::move(xvalLeft), xvalStore ) ;
		}
		return	unaryPost ? xvalBefore : xval ;
	}
}

// 構造体の単項演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalStructUnaryOperator
	( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval )
{
	assert( xval->IsRefPointer() ) ;

	LStructureClass *	pStructClass = xval->GetType().GetStructureClass() ;
	assert( pStructClass != nullptr ) ;

	// 演算子取得
	const Symbol::UnaryOperatorDef *
		puopDef = pStructClass->GetMatchUnaryOperator( opIndex ) ;
	if ( puopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
				xval->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval ;
	}
	if ( xval->IsFetchAddrPointer() )
	{
		OnError( errorCannotOperateWithFetchAddr_opt2,
				xval->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval ;
	}
	if ( !puopDef->m_constThis && xval->GetType().IsConst() )
	{
		OnError( errorCannotConstVariable_opt1,
					GetOperatorDesc(opIndex).pwszName ) ;
	}

	// ポインタに変換
	LExprValuePtr	xvalPtr = std::make_shared<LExprValue>( *xval ) ;
	xvalPtr->Type() = LType( m_vm.GetPointerClassAs( xval->GetType() ) ) ;
	xvalPtr->MakeRefIntoPointer() ;
	xvalPtr = EvalMakePointerInstance( std::move(xvalPtr) ) ;
	xval = nullptr ;

	// 構造体にオーバーロードされた演算子を実行
	return	EvalCodeUnaryOperate
				( puopDef->m_typeRet, std::move(xvalPtr),
					puopDef->m_opIndex,
					puopDef->m_pfnOp,
					puopDef->m_pInstance, puopDef->m_pOpFunc ) ;
}

// オブジェクトの単項演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalObjectUnaryOperator
	( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval )
{
	assert( xval != nullptr ) ;
	assert( !xval->GetType().IsPrimitive() ) ;
	assert( !xval->GetType().IsDataArray() ) ;

	LClass *	pObjClass = xval->GetType().GetClass() ;
	assert( pObjClass != nullptr ) ;

	// 演算子取得
	const Symbol::UnaryOperatorDef *
		puopDef = pObjClass->GetMatchUnaryOperator( opIndex ) ;
	if ( puopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
				xval->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval ;
	}
	if ( !puopDef->m_constThis && xval->GetType().IsConst() )
	{
		OnError( errorCannotConstVariable_opt1,
					GetOperatorDesc(opIndex).pwszName ) ;
	}

	// オーバーロードされた演算子を実行
	if ( xval->IsConstExpr() && puopDef->m_constExpr )
	{
		// 定数式
		if ( xval->GetObject() == nullptr )
		{
			OnError( exceptionNullPointer ) ;
			return	xval ;
		}
		LValue::Primitive	value = xval->Value() ;
		value = (puopDef->m_pfnOp)( value, puopDef->m_pInstance ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		if ( puopDef->m_typeRet.IsObject() )
		{
			return	std::make_shared<LExprValue>
						( puopDef->m_typeRet, value.pObject, true, false ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>
						( puopDef->m_typeRet.GetPrimitive(), value, true ) ;
		}
	}
	else
	{
		// 実行時式
		return	EvalCodeUnaryOperate
					( puopDef->m_typeRet, std::move(xval),
						puopDef->m_opIndex,
						puopDef->m_pfnOp,
						puopDef->m_pInstance, puopDef->m_pOpFunc ) ;
	}
}

// 二項演算（※例外的にそれ以外のものも含む）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseBinaryOperator
	( LStringParser& sparsExpr,
		LExprValuePtr xval,
		Symbol::OperatorIndex opIndex,
		const LNamespaceList * pnslLocal,
		const wchar_t * pwszEscChars )
{
	// 特殊な演算子判別
	switch ( opIndex )
	{
	case	Symbol::opLogicalAnd:
	case	Symbol::opLogicalOr:
		return	ParseOperatorLogical
					( sparsExpr, xval, opIndex, pnslLocal, pwszEscChars ) ;

	case	Symbol::opStaticMemberOf:
		return	ParseOperatorStaticMemberOf( sparsExpr, xval ) ;

	case	Symbol::opMemberOf:
		return	ParseOperatorMemberOf( sparsExpr, xval ) ;

	case	Symbol::opMemberCallOf:
		return	ParseOperatorMemberCallOf
					( sparsExpr, xval, pnslLocal, pwszEscChars ) ;

	case	Symbol::opParenthesis:
		return	ParseOperatorCallFunction( sparsExpr, xval, pnslLocal ) ;

	case	Symbol::opBracket:
		return	ParseOperatorElement( sparsExpr, xval, pnslLocal ) ;

	case	Symbol::opInstanceOf:
		return	ParseOperatorInstanceOf
					( sparsExpr, xval, pnslLocal, pwszEscChars ) ;

	case	Symbol::opConditional:
		return	ParseOperatorConditionalChoise
					( sparsExpr, xval, pnslLocal, pwszEscChars ) ;

	case	Symbol::opSequencing:
		return	ParseOperatorSequencing
					( sparsExpr, xval, pnslLocal, pwszEscChars ) ;

	default:
		break ;
	}

	// 二項演算子
	assert( GetOperatorDesc(opIndex).priorityBinary != Symbol::priorityNo ) ;

	LExprValuePtr	xval2 =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(opIndex).priorityBinary ) ;

	return	EvalBinaryOperator
				( std::move(xval), opIndex, std::move(xval2), pnslLocal ) ;
}

// 二項演算
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalBinaryOperator
	( LExprValuePtr xval1,
		Symbol::OperatorIndex opIndex, LExprValuePtr xval2,
		const LNamespaceList * pnslLocal )
{
	xval1 = EvalMakeInstanceIfConstExpr( std::move(xval1) ) ;
	xval2 = EvalMakeInstanceIfConstExpr( std::move(xval2) ) ;

	if ( xval1->GetType().IsDataArray() )
	{
		// データ配列への直接の演算は不可
		OnError( errorNotDefinedOperator_opt2,
					xval1->GetType().GetTypeName().c_str(),
					GetOperatorDesc(opIndex).pwszName ) ;
	}
	if ( xval2->GetType().IsDataArray() )
	{
		// データ配列への直接の演算は不可
		OnError( errorNotDefinedOperator_opt2,
					xval2->GetType().GetTypeName().c_str(),
					GetOperatorDesc(opIndex).pwszName ) ;
	}
	if ( HasErrorOnCurrent() )
	{
		return	std::make_shared<LExprValue>() ;
	}

	// オーバーロードされた演算子関数を探す
	// ※右辺がプリミティブ型の場合には静関数でのオーバーロードを認めない
	if ( xval2->GetType().IsObject() )
	{
		const Symbol::OperatorDesc&
							opDesc = GetOperatorDesc( opIndex ) ;
		LString	strOpFunc = GetReservedWordDesc(Symbol::rwiOperator).pwszName ;
		strOpFunc += L" " ;
		strOpFunc += opDesc.pwszName ;
		if ( opDesc.pwszPairName != nullptr )
		{
			strOpFunc += opDesc.pwszPairName ;
		}
		LArgumentListType	argTypes ;
		argTypes.push_back( xval1->GetType() ) ;
		argTypes.push_back( xval2->GetType() ) ;

		LPtr<LFunctionObj>	pOpFunc =
			GetStaticFunctionAs( strOpFunc.c_str(), argTypes, pnslLocal ) ;
		if ( pOpFunc != nullptr )
		{
			// オーバーロードされた関数を呼び出す
			std::vector<LExprValuePtr>	argList ;
			argList.push_back( xval1 ) ;
			argList.push_back( xval2 ) ;

			return	ExprCallFunction( nullptr, pOpFunc, argList ) ;
		}
	}

	if ( opIndex == Symbol::opStore )
	{
		// 代入処理
		ExprCodeStore( xval1, std::move(xval2) ) ;
		return	xval1 ;
	}
	if ( xval1->GetType().IsPrimitive() )
	{
		// プリミティブ型
		return	EvalPrimitiveBinaryOperator
					( std::move(xval1), opIndex, std::move(xval2) ) ;
	}
	if ( xval1->GetType().IsPointer() )
	{
		// ポインタ型
		return	EvalPointerBinaryOperator
					( std::move(xval1), opIndex, std::move(xval2) ) ;
	}
	if ( xval1->GetType().IsStructure() )
	{
		// 構造体への参照の場合には構造体への演算子を実行可能
		if ( xval1->IsRefPointer() )
		{
			return	EvalStructBinaryOperator
						( std::move(xval1), opIndex, std::move(xval2) ) ;
		}
	}
	// オブジェクトの二項演算子
	return	EvalObjectBinaryOperator
				( std::move(xval1), opIndex, std::move(xval2) ) ;
}

// :: 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorStaticMemberOf
	( LStringParser& sparsExpr, LExprValuePtr xval )
{
	assert( xval != nullptr ) ;

	LNamespace *	pNamespace = nullptr ;
	LClass *		pClass = nullptr ;
	if ( xval->IsNamespace() )
	{
		pNamespace = xval->GetNamespace().Ptr() ;
		pClass = dynamic_cast<LClass*>( pNamespace ) ;
	}
	else if ( xval->IsConstExprClass() )
	{
		pClass = xval->GetConstExprClass() ;
		pNamespace = pClass ;
	}
	else
	{
		pClass = xval->GetType().GetClass() ;
		pNamespace = pClass ;
	}
	if ( pNamespace == nullptr )
	{
		OnError( errorSyntaxErrorStaticMember ) ;
		return	std::make_shared<LExprValue>() ;
	}

	Symbol::OperatorIndex	opIndex = Symbol::opInvalid ;
	LString					strMember ;
	if ( sparsExpr.NextToken( strMember ) != LStringParser::tokenSymbol )
	{
		OnError( errorInvalidMember, strMember.c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}
	if ( strMember == GetReservedWordDesc(Symbol::rwiOperator).pwszName )
	{
		// operator ?
		size_t	iTempPos = sparsExpr.GetIndex() ;
		LString	strOperator ;
		sparsExpr.NextToken( strOperator ) ;
		//
		opIndex = AsOperatorIndex( strOperator.c_str() ) ;
		if ( opIndex == Symbol::opInvalid )
		{
			OnError( errorInvalidOperator, nullptr, strOperator.c_str() ) ;
			sparsExpr.SeekIndex( iTempPos ) ;
		}
		else if ( !GetOperatorDesc(opIndex).overloadable )
		{
			OnError( errorUnoverloadableOperator, strOperator.c_str() ) ;
			sparsExpr.SeekIndex( iTempPos ) ;
		}
		else
		{
			const Symbol::OperatorDesc&	opdsc = GetOperatorDesc(opIndex) ;
			if ( opdsc.pwszPairName != nullptr )
			{
				if ( !sparsExpr.HasNextToken( opdsc.pwszPairName ) )
				{
					OnError( errorMismatchToken_opt1_opt2,
								opdsc.pwszName, opdsc.pwszPairName ) ;
					sparsExpr.SeekIndex( iTempPos ) ;
				}
			}
			strMember += L" " ;
			strMember += opdsc.pwszName ;
			if ( opdsc.pwszPairName != nullptr )
			{
				strOperator += opdsc.pwszPairName ;
			}
		}
	}

	if ( pClass != nullptr )
	{
		// 仮想関数を探す
		const std::vector<size_t> *
			pVirtVec = pClass->GetVirtualVector().FindFunction( strMember.c_str() ) ;
		if ( pVirtVec != nullptr )
		{
			// 仮想関数の直接の参照
			return	std::make_shared<LExprValue>
				( m_vm.GetFunctionClass(),
					pClass->GetVirtualVector().MakeFuncVariationOf(  pVirtVec ) ) ;
		}
	}

	// 静的なメンバ
	LNamespaceList	nslLocal ;
	nslLocal.AddNamespace( pNamespace ) ;
	AddScopeToNamespaceList( nslLocal ) ;

	LExprValuePtr	xvalMember =
		GetSymbolWithNamespaceAs
			( strMember.c_str(), &sparsExpr, pNamespace, &nslLocal ) ;
	if ( xvalMember == nullptr )
	{
		OnError( errorInvalidMember, strMember.c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}
	return	xvalMember ;
}

// . 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorMemberOf
	( LStringParser& sparsExpr, LExprValuePtr xval )
{
	assert( xval != nullptr ) ;

	LString	strMember ;
	if ( sparsExpr.NextToken( strMember ) != LStringParser::tokenSymbol )
	{
		OnError( errorInvalidMember, strMember.c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}
	LExprValuePtr	xvalMember = GetExprVarMember( xval, strMember.c_str() ) ;
	if ( xvalMember == nullptr )
	{
		OnError( errorInvalidMember, strMember.c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}
	return	xvalMember ;
}

// .* 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorMemberCallOf
	( LStringParser& sparsExpr, LExprValuePtr xval,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	LExprValuePtr	xval2 =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(Symbol::opMemberCallOf).priorityBinary ) ;
	if ( HasErrorOnCurrent() )
	{
		return	xval2 ;
	}
	if ( !xval2->GetType().IsFunction() )
	{
		OnError( errorInvalidIndirectFunction ) ;
		return	xval2 ;
	}

	LExprValuePtr	xvalFunc =
		std::make_shared<LExprValue>( xval2->GetType(), nullptr, false, false ) ;
	xvalFunc->SetOptionRefCallThisOf( xval, xval2 ) ;
	return	xvalFunc ;
}

// () 演算子（関数呼び出し）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorCallFunction
	( LStringParser& sparsExpr,
		LExprValuePtr xval, const LNamespaceList * pnslLocal )
{
	if ( !xval->GetType().IsFunction() )
	{
		OnError( errorInvalidFunction ) ;
		return	xval ;
	}

	// 引数解釈
	std::vector<LExprValuePtr>	xvalArgList ;
	if ( !ParseExprList( sparsExpr, xvalArgList, pnslLocal, L")" ) )
	{
		assert( HasErrorOnCurrent() ) ;
		return	xval ;
	}
	if ( sparsExpr.HasNextChars( L")" ) != L')' )
	{
		OnError( errorMismatchParentheses ) ;
		return	xval ;
	}

	if ( xval->IsFunctionVariation() )
	{
		// func( ... ) 形式  （複数候補）
		//
		// 但しプロトタイプに this クラスが設定されている場合
		// (this.*func) と同じ意味に解釈する
		assert( xval->GetFuncVariation() != nullptr ) ;

		LArgumentListType	argListType ;
		GetExprListTypes( argListType, xvalArgList ) ;

		LPtr<LFunctionObj>	pFuncObj =
				xval->GetFuncVariation()->GetCallableFunction( argListType ) ;
		if ( pFuncObj == nullptr )
		{
			OnError( errorNotFoundMatchFunction,
						xval->GetFuncVariation()->GetFunctionName().c_str(),
						argListType.ToString().c_str() ) ;
			return	xval ;
		}
		return	ExprCallFunction( GetThisObject(), pFuncObj, xvalArgList ) ;
	}
	else if ( xval->IsRefVirtualFunction() )
	{
		// class::func( ... ) 形式の仮想関数
		//
		assert( xval->GetVirtFuncClass() != nullptr ) ;
		assert( xval->GetVirtFunctions() != nullptr ) ;
		LExprValuePtr	xvalThis = GetThisObject() ;
		LClass *		pThisClass = xval->GetVirtFuncClass() ;
		const std::vector<size_t> *
						pVirtFuncs = xval->GetVirtFunctions() ;

		return	ExprCallVirtualFunction
					( std::move(xvalThis),
						pThisClass, pVirtFuncs, xvalArgList ) ;
	}
	else if ( xval->IsRefCallFunction() )
	{
		// (obj.*func)( ... ) 形式の関数
		//
		assert( xval->GetOption1() != nullptr ) ;
		assert( xval->GetOption2() != nullptr ) ;
		LExprValuePtr	xvalThis = xval->GetOption1() ;
		LExprValuePtr	xvalFunc = xval->GetOption2() ;
		return	ExprIndirectCallFunction
					( std::move(xvalThis), std::move(xvalFunc), xvalArgList ) ;
	}
	else if ( xval->IsOperatorFunction() )
	{
		// operator ? ( ... ) 形式の関数
		//
		// (this.*operator ?)( ... ) と同じ意味に解釈する
		//
		LExprValuePtr	xvalThis = GetThisObject() ;
		if ( xvalThis == nullptr )
		{
			OnError( errorNotExistingThis ) ;
			return	xval ;
		}
		return	ExprIndirectCallFunction
					( std::move(xvalThis), std::move(xval), xvalArgList ) ;
	}
	else
	{
		// func( ... ) 形式
		//
		if ( xval->IsConstExprFunction() )
		{
			LPtr<LFunctionObj>	pFuncObj = xval->GetConstExprFunction() ;
			if ( pFuncObj == nullptr )
			{
				OnError( exceptionNullPointer )  ;
				return	xval ;
			}
			LArgumentListType	argListType ;
			GetExprListTypes( argListType, xvalArgList ) ;
			if ( !pFuncObj->GetPrototype()->
						DoesMatchArgmentListWith( argListType ) )
			{
				OnError( errorMismatchFunctionArgment,
							pFuncObj->GetPrintName().c_str(),
							argListType.ToString().c_str() ) ;
				return	xval ;
			}
			return	ExprCallFunction( GetThisObject(), pFuncObj, xvalArgList ) ;
		}
		LFunctionClass *	pFuncClass = xval->GetType().GetFunctionClass() ;
		if ( pFuncClass == nullptr )
		{
			OnError( errorInvalidFunction ) ;
			return	xval ;
		}
		return	ExprIndirectCallFunction
					( GetThisObject(), std::move(xval), xvalArgList ) ;
	}
}

// 関数呼び出し
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprCallFunction
	( LExprValuePtr xvalThis,
		LPtr<LFunctionObj> pFunc,
		std::vector<LExprValuePtr>& xvalArgList )
{
	assert( pFunc != nullptr ) ;

	std::shared_ptr<LPrototype>	pProto = pFunc->GetPrototype() ;
	assert( pProto != nullptr ) ;

	// 関数プロトタイプと引数は適合するか？
	LArgumentListType	argListType ;
	GetExprListTypes( argListType, xvalArgList ) ;

	if ( (pProto == nullptr)
		|| !pProto->DoesMatchArgmentListWith( argListType ) )
	{
		OnError( errorMismatchFunctionArgment,
					pFunc->GetPrintName().c_str(),
					argListType.ToString().c_str() ) ;
		return	xvalThis ;
	}
	if ( pProto->GetModifiers() & LType::modifierDeprecated )
	{
		OnWarning( warningCallDeprecatedFunc,
					warning3, pFunc->GetPrintName().c_str() ) ;
	}

	if ( ((xvalThis == nullptr) || xvalThis->IsConstExpr())
		&& IsExprListConstExpr(xvalArgList)
		&& pFunc->IsCallableInConstExpr() )
	{
		// 定数式として関数呼び出し
		return	ConstExprCallFunction( xvalThis, pFunc, xvalArgList ) ;
	}

	// 引数をプッシュ
	ExprCodePushArgument( *pProto, xvalThis, xvalArgList ) ;
	AssertNoFetchedAddrOnLocal() ;

	// 呼び出し命令
	LCodeBuffer::ImmediateOperand	immFunc ;
	immFunc.pFunc = pFunc.Ptr() ;

	AddCode( LCodeBuffer::codeCall, 0, 0, 0, 0, &immFunc ) ;

	// スタック解放と返り値取得
	return	ExprCodeReturnValue( *pProto ) ;
}

// 定数式として関数呼び出し
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ConstExprCallFunction
	( LExprValuePtr xvalThis,
		LPtr<LFunctionObj> pFunc,
		std::vector<LExprValuePtr>& xvalArgList )
{
	LContext			context( m_vm, 0x20 ) ;
	LContext::Current	current( context ) ;

	assert( pFunc != nullptr ) ;

	std::shared_ptr<LPrototype>	pProto = pFunc->GetPrototype() ;
	assert( pProto != nullptr ) ;

	if ( pProto->GetModifiers() & LType::modifierDeprecated )
	{
		OnWarning( warningCallDeprecatedFunc,
					warning3, pFunc->GetPrintName().c_str() ) ;
	}

	// 引数をプッシュ
	std::vector<LValue>	argValues ;
	ConstExprPushArgument
		( argValues, *pProto, xvalThis, xvalArgList ) ;
	if ( HasErrorOnCurrent() )
	{
		return	std::make_shared<LExprValue>() ;
	}

	// 呼び出し実行
	auto [valRet, pExcept] =
			context.SyncCallFunction
				( pFunc.Ptr(), argValues.data(), argValues.size() ) ;

	// 返り値
	return	ConstExprReturnValue( *pProto, valRet, pExcept ) ;
}

// 間接関数呼び出し
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprIndirectCallFunction
	( LExprValuePtr xvalThis,
		LExprValuePtr xvalFunc,
		std::vector<LExprValuePtr>& xvalArgList )
{
	assert( xvalFunc != nullptr );
	if ( !xvalFunc->GetType().IsFunction() )
	{
		OnError( errorInvalidFunction ) ;
		return	xvalThis ;
	}

	if ( xvalFunc->IsOperatorFunction() )
	{
		// (this.*operator ?)(...) 呼び出し
		if ( xvalThis == nullptr )
		{
			OnError( errorInvalidFunctionLeftExpr ) ;
			return	xvalThis ;
		}
		const Symbol::OperatorDef *	pOpDef = xvalFunc->GetOperatorFunction() ;
		if ( !pOpDef->m_constThis && xvalThis->GetType().IsConst() )
		{
			OnError( errorCannotConstVariable_opt1,
					GetOperatorDesc(pOpDef->m_opIndex).pwszName ) ;
		}
		if ( pOpDef->m_opClass == Symbol::operatorBinary )
		{
			// 二項演算子
			if ( xvalArgList.size() != 1 )
			{
				OnError( errorMismatchFunctionArgment ) ;
				return	xvalThis ;
			}
			const Symbol::BinaryOperatorDef *
				pBinOpDef = (const Symbol::BinaryOperatorDef*) pOpDef ;
			return	EvalCodeBinaryOperate
				( pOpDef->m_typeRet,
					EvalCastTypeTo(xvalThis, LType(pOpDef->m_pThisClass)),
					EvalCastTypeTo(xvalArgList.at(0), pBinOpDef->m_typeRight),
					pBinOpDef->m_opIndex,
					pBinOpDef->m_pfnOp,
					pBinOpDef->m_pInstance, pBinOpDef->m_pOpFunc ) ;
		}
		else
		{
			// 単項演算子
			if ( xvalArgList.size() != 0 )
			{
				OnError( errorMismatchFunctionArgment ) ;
				return	xvalThis ;
			}
			const Symbol::UnaryOperatorDef *
				pUnaryOpDef = (const Symbol::UnaryOperatorDef*) pOpDef ;
			return	EvalCodeUnaryOperate
				( pOpDef->m_typeRet,
					EvalCastTypeTo(xvalThis, LType(pOpDef->m_pThisClass)),
					pUnaryOpDef->m_opIndex,
					pUnaryOpDef->m_pfnOp,
					pUnaryOpDef->m_pInstance, pUnaryOpDef->m_pOpFunc ) ;
		}
	}

	// 関数プロトタイプと引数は適合するか？
	LArgumentListType	argListType ;
	GetExprListTypes( argListType, xvalArgList ) ;

	if ( xvalFunc->IsFunctionVariation() )
	{
		// 複数候補の静関数
		LPtr<LFunctionObj>	pFuncObj =
				xvalFunc->GetFuncVariation()->GetCallableFunction( argListType ) ;
		if ( pFuncObj == nullptr )
		{
			OnError( errorNotFoundMatchFunction,
						xvalFunc->GetFuncVariation()->GetFunctionName().c_str(),
						argListType.ToString().c_str() ) ;
			return	std::make_shared<LExprValue>() ;
		}
		return	ExprCallFunction( xvalThis, pFuncObj, xvalArgList ) ;
	}
	else if ( xvalFunc->IsRefVirtualFunction() )
	{
		// 複数候補の仮想関数
		LClass *		pThisClass = xvalFunc->GetVirtFuncClass() ;
		const std::vector<size_t> *
						pVirtFuncs = xvalFunc->GetVirtFunctions() ;

		return	ExprCallVirtualFunction
			( std::move(xvalThis), pThisClass, pVirtFuncs, xvalArgList ) ;
	}

	// 関数オブジェクト
	LFunctionClass *	pFuncClass = xvalFunc->GetType().GetFunctionClass() ;
	assert( pFuncClass != nullptr ) ;

	std::shared_ptr<LPrototype>	pProto = pFuncClass->GetFuncPrototype() ;
	if ( (pProto == nullptr)
		|| !pProto->DoesMatchArgmentListWith( argListType ) )
	{
		OnError( errorMismatchFunctionArgment,
					nullptr, argListType.ToString().c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}

	xvalFunc = ExprPrepareToPushClone( std::move(xvalFunc), false ) ;

	// 引数をプッシュ
	ExprCodePushArgument( *pProto, xvalThis, xvalArgList ) ;
	AssertNoFetchedAddrOnLocal() ;

	// 呼び出し命令
	ExprPushClone( xvalFunc, false ) ;
	AddCode( LCodeBuffer::codeCallIndirect ) ;
	PopExprValueOnStacks( 1 ) ;

	// スタック解放と返り値取得
	return	ExprCodeReturnValue( *pProto ) ;
}

// 仮想関数呼び出し
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprCallVirtualFunction
	( LExprValuePtr xvalThis,
		LClass * pThisClass,
		const std::vector<size_t> * pVirtFuncs,
		std::vector<LExprValuePtr>& xvalArgList )
{
	const LVirtualFuncVector&	vecVirtFunc = pThisClass->GetVirtualVector() ;

	// 適合関数取得
	LArgumentListType	argListType ;
	GetExprListTypes( argListType, xvalArgList ) ;

	ssize_t	iVirtual =
			vecVirtFunc.FindCallableFunction
				( pVirtFuncs, argListType,
					pThisClass, GetAccessibleScope(pThisClass) ) ;
	if ( iVirtual < 0 )
	{
		OnError( errorNotFoundMatchFunction,
					vecVirtFunc.GetFunctionNameOf(pVirtFuncs).c_str(),
					argListType.ToString().c_str() ) ;
		for ( size_t i = 0; i < pVirtFuncs->size(); i ++ )
		{
			LPtr<LFunctionObj>	pFunc = vecVirtFunc.GetFunctionAt( pVirtFuncs->at(i) ) ;
			if ( (pFunc != nullptr) && (pFunc->GetPrototype() != nullptr) )
			{
				PrintString
					( pFunc->GetFullPrintName() + L": "
						+ pFunc->GetPrototype()->TypeToString() + L"\n" ) ;
			}
		}
		return	std::make_shared<LExprValue>() ;
	}
	LStructureClass *
			pStructClass = xvalThis->GetType().GetPtrStructureClass() ;
	if ( pStructClass != nullptr )
	{
		// 構造体の場合には通常の関数として呼び出し
		LPtr<LFunctionObj>
			pFuncObj = vecVirtFunc.GetFunctionAt( (size_t) iVirtual ) ;
		assert( pFuncObj != nullptr ) ;
		//
		return	ExprCallFunction( xvalThis, pFuncObj, xvalArgList ) ;
	}
	else
	{
		// 仮想関数呼び出し
		return	ExprCallVirtualFunction
					( xvalThis, pThisClass, (size_t) iVirtual, xvalArgList ) ;
	}
}

LExprValuePtr LCompiler::ExprCallVirtualFunction
	( LExprValuePtr xvalThis,
		LClass * pThisClass, size_t iVirtual,
		std::vector<LExprValuePtr>& xvalArgList )
{
	assert( pThisClass != nullptr ) ;
	const LVirtualFuncVector&	vecVirtFunc = pThisClass->GetVirtualVector() ;

	LPtr<LFunctionObj>	pFuncObj = vecVirtFunc.GetFunctionAt( iVirtual ) ;
	assert( pFuncObj != nullptr ) ;

	std::shared_ptr<LPrototype>	pProto = pFuncObj->GetPrototype() ;
	assert( pProto != nullptr ) ;

	// 関数プロトタイプと引数は適合するか？
	LArgumentListType	argListType ;
	GetExprListTypes( argListType, xvalArgList ) ;

	if ( (pProto == nullptr)
		|| !pProto->DoesMatchArgmentListWith( argListType ) )
	{
		OnError( errorMismatchFunctionArgment,
					pFuncObj->GetPrintName().c_str(),
					argListType.ToString().c_str() ) ;
		return	xvalThis ;
	}
	if ( pProto->GetModifiers() & LType::modifierDeprecated )
	{
		OnWarning( warningCallDeprecatedFunc,
					warning3, pFuncObj->GetPrintName().c_str() ) ;
	}

	if ( ((xvalThis == nullptr) || xvalThis->IsConstExpr())
		&& IsExprListConstExpr(xvalArgList)
		&& pFuncObj->IsCallableInConstExpr() )
	{
		// 定数式として関数呼び出し
		return	ConstExprCallFunction( xvalThis, pFuncObj, xvalArgList ) ;
	}

	// 引数をプッシュ
	ExprCodePushArgument( *pProto, xvalThis, xvalArgList ) ;
	AssertNoFetchedAddrOnLocal() ;

	// 呼び出し命令
	LCodeBuffer::ImmediateOperand	immClass ;
	immClass.pClass = pThisClass ;

	AddCode( LCodeBuffer::codeCallVirtual,
					0, 0, 0, iVirtual, &immClass ) ;

	// スタック解放と返り値取得
	return	ExprCodeReturnValue( *pProto ) ;
}

// 関数引数をプッシュ
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodePushArgument
	( const LPrototype& proto,
		LExprValuePtr xvalThis,
		const std::vector<LExprValuePtr>& xvalArgList )
{
	// this オブジェクト
	LExprValuePtr	xvalTempThis ;
	if ( proto.GetThisClass() != nullptr )
	{
		if ( xvalThis == nullptr )
		{
			OnError( errorInvalidFunctionLeftExpr ) ;
			return ;
		}
		if ( xvalThis->IsFetchAddrPointer() )
		{
			OnError( errorCannotCallWithFetchAddr,
						xvalThis->GetType().GetTypeName().c_str() ) ;
			return ;
		}
		LType	typeThisCast = GetLocalTypeOf
								( proto.GetThisClass(), proto.IsConstThis() ) ;
		xvalTempThis = EvalCastTypeTo( xvalThis, typeThisCast ) ;
		if ( HasErrorOnCurrent() )
		{
			return ;
		}
		if ( xvalThis->GetType().IsConst() && !proto.IsConstThis() )
		{
			OnError( errorCannotCallNonConstFunction ) ;
			return ;
		}
		xvalTempThis = ExprPrepareToPushClone( std::move(xvalTempThis), false ) ;
	}

	// 引数
	std::vector<LExprValuePtr>	xvalLoadedArgList ;
	for ( size_t i = 0; i < xvalArgList.size(); i ++ )
	{
		LExprValuePtr	xvalArg =
			ExprPrepareToPushClone
				( EvalCastTypeTo( std::move(xvalArgList.at(i)),
						proto.GetArgListType().GetArgTypeAt(i) ), false ) ;
		if ( xvalArg->IsFetchAddrPointer() )
		{
			OnError( errorCannotFetchAddrForArg,
						xvalArg->GetType().GetTypeName().c_str() ) ;
		}
		xvalLoadedArgList.push_back( xvalArg ) ;
	}

	// 省略された引数
	for ( size_t i = xvalArgList.size();
					i < proto.GetArgListType().size(); i ++ )
	{
		LValue	valDef = proto.GetDefaultArgAt( i ) ;
		if ( valDef.IsVoid() )
		{
			OnError( errorNoArgmentOfFunction ) ;
			return ;
		}
		LExprValuePtr	xvalDef = std::make_shared<LExprValue>( valDef.Clone() ) ;
		xvalLoadedArgList.push_back
			( ExprPrepareToPushClone
				( EvalCastTypeTo( std::move(xvalDef),
						proto.GetArgListType().GetArgTypeAt(i) ), false ) ) ;
	}

	// スタックにプッシュ
	AddCode( LCodeBuffer::codeMoveAP ) ;

	if ( xvalTempThis != nullptr )
	{
		ExprPushClone( xvalTempThis, false ) ;
	}
	for ( size_t i = 0; i < xvalLoadedArgList.size(); i ++ )
	{
		ExprPushClone( xvalLoadedArgList.at(i), false ) ;
	}
}

void LCompiler::ConstExprPushArgument
	( std::vector<LValue>& argValues,
		const LPrototype& proto,
		LExprValuePtr xvalThis,
		const std::vector<LExprValuePtr>& xvalArgList )
{
	// this オブジェクト
	LObjPtr	pThisObj ;
	if ( proto.GetThisClass() != nullptr )
	{
		pThisObj = (xvalThis != nullptr) ? xvalThis->GetObject() : nullptr ;
		if ( pThisObj == nullptr )
		{
			OnError( exceptionNullPointer ) ;
			return ;
		}
		pThisObj = pThisObj->CastClassTo( proto.GetThisClass() ) ;
		if ( pThisObj == nullptr )
		{
			OnError( errorFailedConstExprToCast_opt1_opt2,
						xvalThis->GetType().GetTypeName().c_str(),
						proto.GetThisClass()->GetFullClassName().c_str() ) ;
			return ;
		}
		if ( xvalThis->GetType().IsConst() && !proto.IsConstThis() )
		{
			OnError( errorCannotCallNonConstFunction ) ;
			return ;
		}
		argValues.push_back( LValue( pThisObj ) ) ;
	}

	// 引数
	for ( size_t i = 0; i < xvalArgList.size(); i ++ )
	{
		LExprValuePtr	xvalArg =
			EvalCastTypeTo( xvalArgList.at(i),
							proto.GetArgListType().GetArgTypeAt(i) ) ;
		xvalArg = EvalMakeInstance( std::move(xvalArg) ) ;
		if ( HasErrorOnCurrent() )
		{
			return ;
		}
		if ( !xvalArg->IsConstExpr() )
		{
			OnError( errorUnavailableConstExpr ) ;
			return ;
		}
		argValues.push_back( LValue(*xvalArg) ) ;
	}

	// 省略された引数
	for ( size_t i = xvalArgList.size();
					i < proto.GetArgListType().size(); i ++ )
	{
		LValue	valDef = proto.GetDefaultArgAt( i ) ;
		if ( valDef.IsVoid() )
		{
			OnError( errorNoArgmentOfFunction ) ;
			return ;
		}
		argValues.push_back( LValue(valDef.Clone()) ) ;
	}
}

// 関数引数のスタック解放と返り値取得
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprCodeReturnValue( const LPrototype& proto )
{
	// スタックを開放
	ExprCodeFreeTempStack() ;

	// 返り値を取得
	if ( proto.GetReturnType().IsVoid() || HasErrorOnCurrent() )
	{
		return	std::make_shared<LExprValue>() ;
	}
	LExprValuePtr	xvalRetValue =
						std::make_shared<LExprValue>
							( proto.GetReturnType(), nullptr, false, false ) ;
	if ( !proto.GetReturnType().IsVoid() )
	{
		PushExprValueOnStack( xvalRetValue ) ;
		AddCode( LCodeBuffer::codeLoadRetValue ) ;

		ExprTreatObjectChain( xvalRetValue ) ;
	}
	return	xvalRetValue ;
}

LExprValuePtr LCompiler::ConstExprReturnValue
	( const LPrototype& proto, const LValue& valRet, LObjPtr pExcept )
{
	// 例外
	if ( pExcept != nullptr )
	{
		LString	strClass ;
		if ( pExcept->GetClass() != nullptr )
		{
			strClass = pExcept->GetClass()->GetFullClassName() ;
		}
		LString	strErrMsg ;
		pExcept->AsString( strErrMsg ) ;

		OnError( errorExceptionInConstExpr,
					strClass.c_str(), strErrMsg.c_str() ) ;
		return	std::make_shared<LExprValue>() ;
	}

	// 返り値取得
	if ( !proto.GetReturnType().IsVoid() )
	{
		if ( proto.GetReturnType().IsObject() )
		{
			return	std::make_shared<LExprValue>
						( proto.GetReturnType(),
							valRet.GetObject(), true, false ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>
						( proto.GetReturnType().GetPrimitive(),
							valRet.Value(), true ) ;
		}
	}
	else
	{
		return	std::make_shared<LExprValue>() ;
	}
}

// ローカル変数に fetch_addr ポインタが存在するかチェックし警告を出力する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::AssertNoFetchedAddrOnLocal( void )
{
	if ( m_ctx == nullptr )
	{
		return ;
	}
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( (pNest->m_frame != nullptr)
			&& pNest->m_frame->AreThereAnyFetchAddr() )
		{
			OnWarning( warningCallWhileFetchingAddr, warning1 ) ;
			break ;
		}
		pNest = pNest->m_prev ;
	}
}

// [] 演算子（要素参照）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorElement
	( LStringParser& sparsExpr, LExprValuePtr xval,
			const LNamespaceList * pnslLocal )
{
	xval = EvalLoadToDiscloseRef( std::move(xval) ) ;
	xval = EvalMakeInstanceIfConstExpr( std::move(xval) ) ;

	// 要素評価
	LExprValuePtr	xvalIndex =
			EvaluateExpression( sparsExpr, pnslLocal, L"]" ) ;
	if ( HasErrorOnCurrent() )
	{
		sparsExpr.PassStatement( L"]", L"};" ) ;
		return	xval ;
	}
	if ( sparsExpr.HasNextChars( L"]" ) != L']' )
	{
		OnError( errorMismatchBrackets ) ;
		sparsExpr.PassStatement( L"]", L"};" ) ;
		return	xval ;
	}
	xvalIndex = EvalLoadToDiscloseRef( std::move(xvalIndex) ) ;

	// 要素参照
	return	EvalOperatorElement( std::move(xval), std::move(xvalIndex) ) ;
}

LExprValuePtr LCompiler::EvalOperatorElement
	( LExprValuePtr xval, LExprValuePtr xvalIndex )
{
	// 要素型判定
	const Symbol::BinaryOperatorDef *	pbopDef = nullptr ;
	if ( xval->GetType().IsMap() )
	{
		if ( !xvalIndex->GetType().IsString()
			&& !xvalIndex->GetType().IsStringBuf()
			&& !xvalIndex->GetType().IsInteger() )
		{
			OnWarning( warningNotStringForMapElement, warning3 ) ;
		}
		if ( !xvalIndex->GetType().IsInteger() )
		{
			LClass *	pStrClass = m_vm.GetStringClass() ;
			xvalIndex = EvalCastTypeTo( std::move(xvalIndex), LType(pStrClass) ) ;
		}
	}
	else if ( xval->GetType().IsPointer()
			|| xval->GetType().IsArray()
			|| xval->GetType().IsString()
			|| xval->GetType().IsStringBuf() )
	{
		if ( !xvalIndex->GetType().IsInteger() )
		{
			OnWarning( warningNotIntegerForArrayElement, warning3 ) ;
		}
		xvalIndex = EvalCastTypeTo( std::move(xvalIndex), LType(LType::typeInt64) ) ;
	}
	else
	{
		LClass *	pClass = xval->GetType().GetClass() ;
		if ( pClass != nullptr )
		{
			pbopDef = pClass->GetMatchBinaryOperator
				( Symbol::opBracket,
					xvalIndex->GetType(), GetAccessibleScope(pClass) ) ;
		}
		if ( pbopDef == nullptr )
		{
			OnError( errorInvalidOperator, nullptr, L"[]" ) ;
			return	xval ;
		}
	}
	if ( HasErrorOnCurrent() )
	{
		return	xval ;
	}

	if ( xval->GetType().IsPointer() )
	{
		// ポインタのインデックス付き参照
		const LType		typeElement = xval->GetType().GetRefElementType().
												ConstWith( xval->GetType() ) ;
		const size_t	nStride = typeElement.GetDataBytes() ;
		if ( nStride != 1 )
		{
			xvalIndex =
				EvalBinaryOperator
					( std::move(xvalIndex), Symbol::opMul,
						LExprValue::MakeConstExprInt( (LLong) nStride ) ) ;
		}
		LExprValuePtr	xvalRef =
			std::make_shared<LExprValue>
				( typeElement, xval->GetObject(),
					xval->IsConstExpr(), xval->IsUniqueObject() ) ;
		xvalRef->SetOptionRefPointerOffset( xval, xvalIndex ) ;
		return	DiscloseRefPointerType( xvalRef ) ;
	}
	else if ( xval->GetType().IsArray()
			|| xval->GetType().IsString()
			|| xval->GetType().IsStringBuf() )
	{
		// Array|String|StringBuf オブジェクトの要素参照
		LExprValuePtr	xvalElement =
				std::make_shared<LExprValue>
					( xval->GetType().GetRefElementType().
						ConstWith( xval->GetType() ), nullptr, false, false ) ;
		if ( xval->GetType().IsString() )
		{
			xvalElement->Type() = xvalElement->GetType().ConstType() ;
		}
		xvalElement->SetOptionRefByIndexOf( xval, xvalIndex ) ;
		return	xvalElement ;
	}
	else if ( xval->GetType().IsMap() )
	{
		// Map オブジェクトの要素参照
		LExprValuePtr	xvalElement =
				std::make_shared<LExprValue>
					( xval->GetType().GetRefElementType().
						ConstWith( xval->GetType() ), nullptr, false, false ) ;
		if ( xvalIndex->GetType().IsInteger() )
		{
			xvalElement->SetOptionRefByIndexOf( xval, xvalIndex ) ;
		}
		else
		{
			xvalElement->SetOptionRefByStrOf( xval, xvalIndex ) ;
		}
		return	xvalElement ;
	}
	else if ( pbopDef != nullptr )
	{
		if ( xval->GetType().IsStructure() )
		{
			return	EvalStructBinaryOperator
						( xval, Symbol::opBracket, xvalIndex ) ;
		}
		else
		{
			return	EvalObjectBinaryOperator
						( xval, Symbol::opBracket, xvalIndex ) ;
		}
	}
	else
	{
		OnError( errorInvalidOperator, nullptr, L"[]" ) ;
		return	xval ;
	}
}

// instanceof 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorInstanceOf
	( LStringParser& sparsExpr, LExprValuePtr xval,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	LExprValuePtr	xvalClass =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(Symbol::opInstanceOf).priorityBinary ) ;

	if ( !xvalClass->IsConstExprClass() )
	{
		OnError( errorSyntaxErrorInstanceofRight ) ;
		return	std::make_shared<LExprValue>() ;
	}

	LExprValuePtr	xvalRet =
		EvalCodeBinaryOperate
			( LType(LType::typeBoolean),
				std::move(xval), std::move(xvalClass),
				Symbol::opInstanceOf,
				&LClass::operator_instanceof, nullptr, nullptr ) ;
	return	xvalRet ;
}

// && 演算子, || 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorLogical
	( LStringParser& sparsExpr, LExprValuePtr xval,
		Symbol::OperatorIndex opIndex,
		const LNamespaceList * pnslLocal,
		const wchar_t * pwszEscChars )
{
	xval = EvalAsBoolean( std::move(xval) ) ;
	if ( xval->IsConstExpr() )
	{
		// 左辺が定数式の場合
		const bool	booExpr1 = (xval->AsInteger() != 0) ;
		if ( (opIndex == Symbol::opLogicalAnd) && !booExpr1 )
		{
			// 右辺は無視して false
			ParseToSkipExpression
				( sparsExpr, pnslLocal, pwszEscChars,
					GetOperatorDesc(opIndex).priorityBinary ) ;
			return	std::make_shared<LExprValue>
				( LType::typeBoolean, LValue::MakeBool(false), true ) ;
		}
		else if ( (opIndex == Symbol::opLogicalOr) && booExpr1 )
		{
			// 右辺は無視して true
			ParseToSkipExpression
				( sparsExpr, pnslLocal, pwszEscChars,
					GetOperatorDesc(opIndex).priorityBinary ) ;
			return	std::make_shared<LExprValue>
				( LType::typeBoolean, LValue::MakeBool(true), true ) ;
		}
		else
		{
			// 右辺を boolean にして返す
			return	EvalAsBoolean
				( EvaluateExpression
					( sparsExpr, pnslLocal, pwszEscChars,
						GetOperatorDesc(opIndex).priorityBinary ) ) ;
		}
	}

	xval = ExprMakeOnStack( std::move(xval) ) ;

	CodePointPtr	cpCJump ;
	if( opIndex == Symbol::opLogicalAnd )
	{
		// && では左辺が false の時ジャンプ
		cpCJump = ExprCodeJumpNonIf( xval ) ;
	}
	else
	{
		// || では左辺が true の時ジャンプ
		assert( opIndex == Symbol::opLogicalOr ) ;
		cpCJump = ExprCodeJumpIf( xval ) ;
	}

	// 右辺の結果を結果とする
	LExprValuePtr	xval2 =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(opIndex).priorityBinary ) ;

	ExprCodeMove( xval, EvalAsBoolean( std::move(xval2) ) ) ;

	FixJumpDestination( cpCJump, GetCurrentCodePointer( true ) ) ;

	return	xval ;
}

// ? : 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorConditionalChoise
	( LStringParser& sparsExpr, LExprValuePtr xval,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	xval = EvalAsBoolean( std::move(xval) ) ;
	if ( xval->IsConstExpr() )
	{
		// 定数式
		const bool	booExpr1 = (xval->AsInteger() != 0) ;
		if ( booExpr1 )
		{
			// true の時は１つめの式を評価する
			xval =  EvaluateExpression
				( sparsExpr, pnslLocal, pwszEscChars,
					GetOperatorDesc(Symbol::opConditional).priorityBinary ) ;
		}
		else
		{
			// false の時は１つめの式を無視する
			ParseToSkipExpression
				( sparsExpr, pnslLocal, pwszEscChars,
					GetOperatorDesc(Symbol::opConditional).priorityBinary ) ;
		}
		if ( sparsExpr.HasNextChars( L":" ) != L';' )
		{
			OnError( errorNoSeparationConditionalExpr ) ;
			sparsExpr.PassStatement( L":", L";}" ) ;
			return	std::make_shared<LExprValue>() ;
		}
		if ( booExpr1 )
		{
			// true の時は２つめの式を無視する
			ParseToSkipExpression
				( sparsExpr, pnslLocal, pwszEscChars,
					GetOperatorDesc(Symbol::opConditional).priorityBinary ) ;
		}
		else
		{
			// false の時は１つめの式を無視する
			xval = EvaluateExpression
				( sparsExpr, pnslLocal, pwszEscChars,
					GetOperatorDesc(Symbol::opConditional).priorityBinary ) ;
		}
		return	xval ;
	}

	// 仮の演算結果
	LExprValuePtr	xvalTemp = ExprLoadImmInteger(0) ;
	assert( IsExprValueOnStack( xvalTemp ) ) ;

	// 分岐
	CodePointPtr	cpCJump = ExprCodeJumpNonIf( std::move(xval) ) ;

	// 真の場合の式
	LExprValuePtr	xval1 =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(Symbol::opConditional).priorityBinary ) ;
	xval1 = ExprMakeInstance( std::move(xval1) ) ;

	// 結果をコピー
	LType	typeExpr = xval1->GetType() ;
	xvalTemp->Type() = typeExpr ;
	ExprCodeMove( xvalTemp, std::move(xval1) ) ;
	ExprCodeFreeTempStack() ;

	CodePointPtr	cpJumpBreak = ExprCodeJump() ;
	FixJumpDestination( cpCJump, GetCurrentCodePointer( true ) ) ;

	// : 記号で分割
	if ( sparsExpr.HasNextChars( L":" ) != L';' )
	{
		OnError( errorNoSeparationConditionalExpr ) ;
		sparsExpr.PassStatement( L":", L";}" ) ;
		return	std::make_shared<LExprValue>() ;
	}

	// 偽の場合の式
	LExprValuePtr	xval2 =
		EvaluateExpression
			( sparsExpr, pnslLocal, pwszEscChars,
				GetOperatorDesc(Symbol::opConditional).priorityBinary ) ;
	xval2 = ExprMakeInstance
			( EvalCastTypeTo( std::move(xval2), typeExpr ) ) ;

	ExprCodeMove( xvalTemp, std::move(xval2) ) ;
	ExprCodeFreeTempStack() ;

	// 処理合流
	FixJumpDestination( cpJumpBreak, GetCurrentCodePointer( true ) ) ;

	// オブジェクト形式の場合には yp チェーン追加
	ExprTreatObjectChain( xvalTemp ) ;

	return	xvalTemp ;
}

// , 演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ParseOperatorSequencing
	( LStringParser& sparsExpr, LExprValuePtr xval,
		const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars )
{
	return	EvaluateExpression
		( sparsExpr, pnslLocal, pwszEscChars,
			GetOperatorDesc(Symbol::opSequencing).priorityBinary ) ;
}

// 数式を読み飛ばす
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ParseToSkipExpression
	( LStringParser& sparsExpr,
		const LNamespaceList * pnslLocal,
		const wchar_t * pwszEscChars, int nEscPriority )
{
	CodePointPtr	cpStart = GetCurrentCodePointer( true ) ;

	EvaluateExpression( sparsExpr, pnslLocal, pwszEscChars, nEscPriority ) ;

	DiscardCodeAfterPoint( cpStart ) ;
}

// プリミティブに対する二項演算
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalPrimitiveBinaryOperator
	( LExprValuePtr xval1,
		Symbol::OperatorIndex opIndex, LExprValuePtr xval2 )
{
	assert( xval1->GetType().IsPrimitive() ) ;

	if ( opIndex == Symbol::opStoreMove )
	{
		ExprCodeStore( xval1, std::move(xval2) ) ;
		return	xval1 ;
	}

	// 特殊条件 : primitive + String
	if ( (opIndex == Symbol::opAdd)
		&& (dynamic_cast<LStringClass*>
				( xval2->GetType().GetClass() ) != nullptr) )
	{
		LClass *	pStrClass = m_vm.GetStringClass() ;
		xval1 = EvalCastTypeTo( std::move(xval1), LType(pStrClass) ) ;
		return	EvalBinaryOperator( xval1, opIndex, xval2 ) ;
	}
	if ( xval2->GetType().IsObject() )
	{
		if ( (dynamic_cast<LIntegerClass*>
					( xval2->GetType().GetClass() ) != nullptr) )
		{
			xval2 = EvalCastTypeTo( std::move(xval2), LType(LType::typeInt64) ) ;
		}
		else if ( (dynamic_cast<LDoubleClass*>
					( xval2->GetType().GetClass() ) != nullptr) )
		{
			xval2 = EvalCastTypeTo( std::move(xval2), LType(LType::typeDouble) ) ;
		}
		else
		{
			LString	strOperator = L"operator " ;
			strOperator += GetOperatorDesc(opIndex).pwszName ;
			strOperator += L"(" ;
			strOperator += xval2->GetType().GetTypeName() ;
			strOperator += L")" ;
			//
			OnError( errorNotDefinedOperator_opt2,
					xval1->GetType().GetTypeName().c_str(), strOperator.c_str() ) ;
			return	std::make_shared<LExprValue>() ;
		}
	}

	// 代入演算子の場合、左辺値を一度ロードする
	LExprValuePtr			xvalLeft ;
	Symbol::OperatorIndex	opPrimitive = opIndex ;
	if ( GetOperatorDesc(opIndex).leftValue )
	{
		if ( xval1->GetType().IsConst() )
		{
			// const 変数への操作エラー
			OnError( errorCannotConstVariable_opt1 ) ;
		}
		if ( !xval1->IsReference() || xval1->IsConstExpr() )
		{
			// 非左辺式への操作エラー
			OnError( errorInvalidForLeftValue_opt1, GetOperatorDesc(opIndex).pwszName ) ;
		}
		opPrimitive = GetOperatorDesc(opIndex).opExStore ;
		xvalLeft = xval1 ;
		xval1 = ExprLoadClone( xval1 ) ;
	}

	// プリミティブ型を揃える
	assert( xval2->GetType().IsPrimitive() ) ;
	LType::Primitive	primRet =
			LType::MaxPrimitiveOf( xval1->GetType().GetPrimitive(),
									xval2->GetType().GetPrimitive() ) ;
	const LType	typeRet( primRet ) ;

	xval1 = EvalCastTypeTo( std::move(xval1), typeRet ) ;
	xval2 = EvalCastTypeTo( std::move(xval2), typeRet ) ;

	// 適合演算子検索
	const Symbol::BinaryOperatorDef *	pbopDef = nullptr ;
	if ( LType::IsFloatingPointPrimitive( primRet ) )
	{
		pbopDef = LClass::FindMatchBinaryOperator
					( opPrimitive, nullptr, typeRet,
						LDoubleClass::s_BinaryOperatorDefs,
						LDoubleClass::BinaryOperatorCount ) ;
	}
	else if ( LType::IsUnsignedIntegerPrimitive( primRet ) )
	{
		pbopDef = LClass::FindMatchBinaryOperator
					( opPrimitive, nullptr, typeRet,
						LIntegerClass::s_UintOperatorDefs,
						LIntegerClass::BinaryOperatorCount ) ;
	}
	else
	{
		pbopDef = LClass::FindMatchBinaryOperator
					( opPrimitive, nullptr, typeRet,
						LIntegerClass::s_IntOperatorDefs,
						LIntegerClass::BinaryOperatorCount ) ;
	}
	if ( pbopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
					xval1->GetType().GetTypeName().c_str(),
					GetOperatorDesc(opIndex).pwszName ) ;
		return	std::make_shared<LExprValue>() ;
	}

	// 右辺値をキャスト（何もしないはず）
	xval2 = EvalCastTypeTo( std::move(xval2), pbopDef->m_typeRight ) ;

	// 演算子実行
	if ( xval1->IsConstExpr() && xval2->IsConstExpr() )
	{
		// 定数式
		LValue::Primitive	value =
			(pbopDef->m_pfnOp)
				( xval1->Value(),
					xval2->Value(), pbopDef->m_pInstance ) ;

		CheckExceptionInConstExpr() ;

		xval1 = std::make_shared<LExprValue>( primRet, value, true ) ;
	}
	else
	{
		// 実行時式
		xval1 = EvalCodeBinaryOperate
					( typeRet, std::move(xval1), std::move(xval2),
						pbopDef->m_opIndex,
						pbopDef->m_pfnOp,
						pbopDef->m_pInstance, pbopDef->m_pOpFunc ) ;

		if ( GetOperatorDesc(opIndex).leftValue )
		{
			// 結果を代入
			ExprCodeStore( xvalLeft, std::move(xval1) ) ;
			xval1 = xvalLeft ;
		}
	}
	if ( pbopDef->m_typeRet.IsBoolean() )
	{
		xval1->Type() = pbopDef->m_typeRet ;
	}

	return	xval1 ;
}

// ポインタに対する二項演算
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalPointerBinaryOperator
	( LExprValuePtr xval1,
		Symbol::OperatorIndex opIndex, LExprValuePtr xval2 )
{
	const LType		typePtr1 = xval1->GetType() ;
	LPointerClass *	pPtrClass = typePtr1.GetPointerClass() ;
	assert( pPtrClass != nullptr ) ;
	if ( pPtrClass == nullptr )
	{
		OnError( errorExpressionError ) ;
		return	xval1 ;
	}
	LStructureClass *
			pStructClass = pPtrClass->GetBufferType().GetStructureClass() ;
	if ( (pStructClass != nullptr)
		&& (opIndex != Symbol::opEqualPtr) && (opIndex != Symbol::opNotEqualPtr) )
	{
		// 構造体ポインタの場合、オーバーロードされた演算子取得
		const Symbol::BinaryOperatorDef *	pbopDef =
			pStructClass->GetMatchBinaryOperator
				( opIndex, xval2->GetType(), GetAccessibleScope(pStructClass) ) ;
		if ( pbopDef != nullptr )
		{
			if ( xval1->IsFetchAddrPointer() )
			{
				OnError( errorCannotOperateWithFetchAddr_opt2,
						xval1->GetType().GetTypeName().c_str(),
						GetOperatorDesc(opIndex).pwszName ) ;
				return	xval1 ;
			}
			if ( !pbopDef->m_constThis && pPtrClass->GetBufferType().IsConst() )
			{
				OnError( errorCannotConstVariable_opt1,
						GetOperatorDesc(opIndex).pwszName ) ;
			}

			// 右辺値をキャスト
			xval2 = EvalCastTypeTo( std::move(xval2), pbopDef->m_typeRight ) ;

			// 構造体にオーバーロードされた演算子を実行
			return	EvalCodeBinaryOperate
						( pbopDef->m_typeRet,
							std::move(xval1), std::move(xval2),
							pbopDef->m_opIndex,
							pbopDef->m_pfnOp,
							pbopDef->m_pInstance, pbopDef->m_pOpFunc ) ;
		}
		if ( opIndex == Symbol::opStoreMove )
		{
			return	ExprCodeStoreMoveStructure
						( std::move(xval1), std::move(xval2) ) ;
		}
	}
	if ( opIndex == Symbol::opStoreMove )
	{
		ExprCodeStore( xval1, std::move(xval2) ) ;
		return	xval1 ;
	}

	const LValue::Primitive
				stride = LValue::MakeLong( pPtrClass->GetElementStride() ) ;
	const bool	flagConstExpr = xval1->IsConstExpr() && xval2->IsConstExpr() ;
	const bool	flagComparator = Symbol::IsComparator(opIndex) ;
	const bool	flagOffsetOperator = !flagConstExpr && !flagComparator
										&& xval1->IsOffsetPointer() ;
	const bool	flagFetchAddrPointer = xval1->IsFetchAddrPointer() ;

	// 代入演算子の場合、左辺値を一度ロードする
	LExprValuePtr			xvalPtr ;
	LExprValuePtr			xvalLeft ;
	Symbol::OperatorIndex	opPrimitive = opIndex ;
	if ( GetOperatorDesc(opIndex).leftValue )
	{
		if ( xval1->GetType().IsConst() )
		{
			// const 変数への操作エラー
			OnError( errorCannotConstVariable_opt1 ) ;
		}
		if ( !xval1->IsReference() || xval1->IsConstExpr() )
		{
			// 非左辺式への操作エラー
			OnError( errorInvalidForLeftValue_opt1, GetOperatorDesc(opIndex).pwszName ) ;
		}
		opPrimitive = GetOperatorDesc(opIndex).opExStore ;
		xvalPtr = xval1 ;

		if ( flagOffsetOperator )
		{
			xvalLeft = xval1->GetOption2() ;
			xval1 = ExprLoadClone( xval1->GetOption2() ) ;
		}
		else
		{
			xvalLeft = xval1 ;
			xval1 = ExprLoadClone( xval1 ) ;
		}
	}
	else if ( flagOffsetOperator )
	{
		xvalPtr = xval1 ;
		xvalLeft = xval1->GetOption2() ;
		xval1 = ExprLoadClone( xval1->GetOption2() ) ;
	}
	xval1 = EvalLoadToDiscloseRef( std::move(xval1) ) ;

	// 演算子取得
	const Symbol::BinaryOperatorDef *	pbopDef = nullptr ;
	if ( (xval1->IsConstExpr() && xval2->IsConstExpr())
		|| flagComparator || flagOffsetOperator || flagFetchAddrPointer )
	{
		pbopDef = LClass::FindMatchBinaryOperator
					( opPrimitive, nullptr, LType(LType::typeInt64),
						LPointerClass::s_BinaryOffsetOperatorDefs,
						LPointerClass::BinaryOffsetOperatorCount ) ;
	}
	else
	{
		pbopDef = LClass::FindMatchBinaryOperator
					( opPrimitive, nullptr, xval2->GetType(),
						LPointerClass::s_BinaryPtrOperatorDefs,
						LPointerClass::BinaryPtrOperatorCount ) ;
	}
	if ( pbopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
				xval1->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval1 ;
	}

	// 右辺式キャスト
	LType	typeRet = xval1->GetType() ;
	if ( flagComparator )
	{
		xval1 = ExprFetchPointerAddr( std::move(xval1), 0, 0 ) ;
		xval2 = ExprFetchPointerAddr
				( EvalCastTypeTo( std::move(xval2), typePtr1 ), 0, 0 ) ;
		typeRet = pbopDef->m_typeRet ;
	}
	else
	{
		// 右辺式を整数に変換する
		assert( pbopDef->m_typeRight.IsInteger() ) ;
		xval2 = EvalCastTypeTo( std::move(xval2), pbopDef->m_typeRight ) ;

		if ( flagOffsetOperator )
		{
			typeRet = pbopDef->m_typeRet ;
		}
	}

	if ( xval1->IsConstExpr() && xval2->IsConstExpr() )
	{
		// 定数式で演算子実行
		LPointerObj *	pPtrObj =
			dynamic_cast<LPointerObj*>( xval1->GetObject().Ptr() ) ;
		assert( pPtrObj != nullptr ) ;
		if ( pPtrObj == nullptr )
		{
			OnError( errorExpressionError ) ;
			return	xval1 ;
		}
		LValue::Primitive	value = flagComparator
										? LValue::MakeLong( xval1->AsInteger() )
										: LValue::MakeLong( 0 ) ;
		LValue::Primitive	offset = LValue::MakeLong( xval2->AsInteger() ) ;
		//
		value = (pbopDef->m_pfnOp)( value, offset, stride.pVoidPtr ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		if ( flagComparator )
		{
			assert( typeRet.GetPrimitive() == LType::typeBoolean ) ;
			return	std::make_shared<LExprValue>
						( typeRet.GetPrimitive(), value, true ) ;
		}
		else
		{
			pPtrObj = new LPointerObj( *pPtrObj ) ;
			pPtrObj += (ssize_t) value.longValue ;
			return	std::make_shared<LExprValue>
						( xval1->GetType(), pPtrObj, true, false ) ;
		}
	}
	else
	{
		// 実行時式で演算実行
		xval1 = EvalCodeBinaryOperate
					( typeRet,
						std::move(xval1), std::move(xval2),
						pbopDef->m_opIndex,
						pbopDef->m_pfnOp,
						stride.pVoidPtr, pbopDef->m_pOpFunc ) ;
		if ( GetOperatorDesc(opIndex).leftValue )
		{
			ExprCodeStore( std::move(xvalLeft), xval1 ) ;
		}
		if ( flagOffsetOperator )
		{
			LExprValuePtr	xvalResPtr =
								std::make_shared<LExprValue>
									( xvalPtr->GetType(), nullptr, false, false ) ;
			xvalResPtr->SetOptionPointerOffset( xvalPtr->GetOption1(), xval1 ) ;
			xval1 = xvalResPtr ;
		}
		return	xval1 ;
	}
}

// 構造体参照に対する二項演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalStructBinaryOperator
	( LExprValuePtr xval1,
		Symbol::OperatorIndex opIndex, LExprValuePtr xval2 )
{
	assert( xval1->IsRefPointer() ) ;

	LStructureClass *	pStructClass = xval1->GetType().GetStructureClass() ;
	assert( pStructClass != nullptr ) ;

	// 演算子取得
	const Symbol::BinaryOperatorDef *	pbopDef =
			pStructClass->GetMatchBinaryOperator
				( opIndex, xval2->GetType(), GetAccessibleScope(pStructClass) ) ;
	if ( pbopDef == nullptr )
	{
		OnError( errorNotDefinedOperator_opt2,
				xval1->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval1 ;
	}
	if ( xval1->IsFetchAddrPointer() )
	{
		OnError( errorCannotOperateWithFetchAddr_opt2,
				xval1->GetType().GetTypeName().c_str(),
				GetOperatorDesc(opIndex).pwszName ) ;
		return	xval1 ;
	}
	if ( !pbopDef->m_constThis && xval1->GetType().IsConst() )
	{
		OnError( errorCannotConstVariable_opt1,
				GetOperatorDesc(opIndex).pwszName ) ;
	}

	// ポインタに変換
	LExprValuePtr	xvalPtr = std::make_shared<LExprValue>( *xval1 ) ;
	xvalPtr->Type() = LType( m_vm.GetPointerClassAs( xval1->GetType() ) ) ;
	xvalPtr->MakeRefIntoPointer() ;
	xvalPtr = EvalMakePointerInstance( std::move(xvalPtr) ) ;
	xval1 = nullptr ;

	// 右辺値をキャスト
	xval2 = EvalCastTypeTo( std::move(xval2), pbopDef->m_typeRight ) ;

	// 構造体にオーバーロードされた演算子を実行
	return	EvalCodeBinaryOperate
				( pbopDef->m_typeRet,
					std::move(xvalPtr), std::move(xval2),
					pbopDef->m_opIndex,
					pbopDef->m_pfnOp,
					pbopDef->m_pInstance, pbopDef->m_pOpFunc ) ;
}

// オブジェクトに対する二項演算子
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalObjectBinaryOperator
	( LExprValuePtr xval1,
		Symbol::OperatorIndex opIndex, LExprValuePtr xval2 )
{
	LClass *	pObjClass = xval1->GetType().GetClass() ;
	assert( pObjClass != nullptr ) ;

	// 演算子取得
	const Symbol::BinaryOperatorDef *	pbopDef =
			pObjClass->GetMatchBinaryOperator
				( opIndex, xval2->GetType(), GetAccessibleScope(pObjClass) ) ;
	Symbol::BinaryOperatorDef	bopDefTemp ;
	if ( (opIndex == Symbol::opEqualPtr)
		|| (opIndex == Symbol::opNotEqualPtr) )
	{
		bopDefTemp.m_opClass = Symbol::operatorBinary ;
		bopDefTemp.m_opIndex = opIndex ;
		bopDefTemp.m_typeRet = LType( LType::typeBoolean ) ;
		bopDefTemp.m_pThisClass = pObjClass ;
		bopDefTemp.m_pfnOp = (opIndex == Symbol::opEqualPtr)
									? &LClass::operator_equal
									: &LClass::operator_not_equal ;
		bopDefTemp.m_typeRight = LType( pObjClass ) ;
		bopDefTemp.m_pInstance = nullptr ;
		pbopDef = &bopDefTemp ;
	}
	if ( pbopDef == nullptr )
	{
		if ( (opIndex == Symbol::opEqual)
			|| (opIndex == Symbol::opNotEqual) )
		{
			bopDefTemp.m_opClass = Symbol::operatorBinary ;
			bopDefTemp.m_opIndex = opIndex ;
			bopDefTemp.m_typeRet = LType( LType::typeBoolean ) ;
			bopDefTemp.m_pThisClass = pObjClass ;
			bopDefTemp.m_pfnOp = (opIndex == Symbol::opEqual)
										? &LClass::operator_equal
										: &LClass::operator_not_equal ;
			bopDefTemp.m_typeRight = LType( pObjClass ) ;
			bopDefTemp.m_pInstance = nullptr ;
			pbopDef = &bopDefTemp ;
		}
		else if ( opIndex == Symbol::opStoreMove )
		{
			OnWarning( warningStoreMoveAsStoreOperator_opt1, warning3,
						pObjClass->GetFullClassName().c_str() ) ;
			ExprCodeStore( xval1, std::move(xval2) ) ;
			return	xval1 ;
		}
		else
		{
			OnError( errorNotDefinedOperator_opt2,
					xval1->GetType().GetTypeName().c_str(),
					GetOperatorDesc(opIndex).pwszName ) ;
			return	xval1 ;
		}
	}
	if ( !pbopDef->m_constThis && xval1->GetType().IsConst() )
	{
		OnError( errorCannotConstVariable_opt1,
				GetOperatorDesc(opIndex).pwszName ) ;
	}

	// 右辺値をキャスト
	xval2 = EvalCastTypeTo( std::move(xval2), pbopDef->m_typeRight ) ;

	// オブジェクトにオーバーロードされた演算子を実行
	if ( xval1->IsConstExpr() && xval2->IsConstExpr() )
	{
		// 定数式
		if ( xval1->GetObject() == nullptr )
		{
			OnError( exceptionNullPointer ) ;
			return	xval1 ;
		}
		LValue::Primitive	value1 = xval1->Value() ;
		LValue::Primitive	value2 = xval2->Value() ;
		LValue::Primitive	valRet ;
		valRet = (pbopDef->m_pfnOp)( value1, value2, pbopDef->m_pInstance ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		if ( pbopDef->m_typeRet.IsObject() )
		{
			return	std::make_shared<LExprValue>
						( pbopDef->m_typeRet, valRet.pObject, true, false ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>
						( pbopDef->m_typeRet.GetPrimitive(), valRet, true ) ;
		}
	}
	else
	{
		// 実行時式
		return	EvalCodeBinaryOperate
					( pbopDef->m_typeRet,
						std::move(xval1), std::move(xval2),
						pbopDef->m_opIndex,
						pbopDef->m_pfnOp,
						pbopDef->m_pInstance, pbopDef->m_pOpFunc ) ;
	}
}

// 定数式中で例外が発生したか確認しエラーを出力する
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::CheckExceptionInConstExpr( void )
{
	LContext *	pContext = LContext::GetCurrent() ;
	if ( (pContext != nullptr)
		&& (pContext->GetException() != nullptr) )
	{
		OutputExceptionAsError( pContext->GetException() ) ;
		pContext->ClearException() ;
		return	true ;
	}
	return	false ;
}

// 例外オブジェクトをエラー出力
//////////////////////////////////////////////////////////////////////////////
void LCompiler::OutputExceptionAsError( LObjPtr pExcept )
{
	if ( pExcept != nullptr )
	{
		LString	strClass ;
		if ( pExcept->GetClass() != nullptr )
		{
			strClass = pExcept->GetClass()->GetFullClassName() ;
		}
		LString	strErrMsg ;
		pExcept->AsString( strErrMsg ) ;

		OnError( errorExceptionInConstExpr,
					strClass.c_str(), strErrMsg.c_str() ) ;
	}
}

// boolean として評価する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalAsBoolean( LExprValuePtr xval )
{
	assert( xval != nullptr ) ;
	xval = EvalLoadToDiscloseRef( std::move(xval) ) ;
	xval = EvalMakeInstanceIfConstExpr( std::move(xval) ) ;

	if ( xval->GetType().IsPrimitive() )
	{
		// プリミティブ型 -> boolean
		if ( xval->GetType().IsBoolean() )
		{
			return	xval ;
		}
		if ( xval->GetType().IsFloatingPointNumber() )
		{
			OnWarning( warningImplicitCast_opt1_opt2, warning3,
						xval->GetType().GetTypeName().c_str(), L"boolean" ) ;
		}
		return	EvalBinaryOperator
					( std::move(xval), Symbol::opNotEqual,
								LExprValue::MakeConstExprInt(0) ) ;
	}
	if ( xval->GetType().IsPointer() )
	{
		// ポインタ型 -> boolean
		LExprValuePtr	xvalNull = ExprLoadImmInteger( 0 ) ;
		xvalNull->Type() = LType( nullptr, LType::modifierFetchAddr ) ;
		//
		return	EvalBinaryOperator
					( ExprFetchPointerAddr( std::move(xval), 0, 0 ),
							Symbol::opNotEqual, std::move(xvalNull) ) ;
	}
	else if ( xval->GetType().IsDataArray()
			|| xval->GetType().IsStructure() )
	{
		// データ配列や構造体からは boolean に変換できない
		OnError( errorUnavailableCast_opt1_opt2,
				xval->GetType().GetTypeName().c_str(), L"boolean" ) ;
		return	xval ;
	}

	// オブジェクト -> boolean
	LExprValuePtr	xvalNull = ExprLoadImmInteger( 0 ) ;
	xvalNull->Type() = LType( nullptr, LType::modifierFetchAddr ) ;
	//
	return	EvalBinaryOperator
				( ExprFetchPointerAddr( std::move(xval), 0, 0 ),
						Symbol::opNotEqual, std::move(xvalNull) ) ;
}

// プリミティブ型をオブジェクトに変換する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalAsObject( LExprValuePtr xval )
{
	if ( xval->GetType().IsPrimitive() )
	{
		if ( xval->GetType().IsInteger() || xval->GetType().IsBoolean() )
		{
			return	EvalCastTypeTo
						( std::move(xval),
							LType( m_vm.GetIntegerObjClass() ) ) ;
		}
		else if ( xval->GetType().IsFloatingPointNumber() )
		{
			return	EvalCastTypeTo
						( std::move(xval),
							LType( m_vm.GetDoubleObjClass() ) ) ;
		}
	}
	return	xval ;
}

// キャストする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalCastTypeTo
	( LExprValuePtr xval, const LType& typeCast, bool explicitCast )
{
	if ( typeCast.IsFunction() && xval->IsConstExpr() )
	{
		return	ConstExprCastFuncTo( std::move(xval), typeCast, explicitCast ) ;
	}
	if ( xval->IsRefPointer() && xval->IsFetchAddrPointer() )
	{
		xval = EvalLoadToDiscloseRef( std::move(xval) ) ;
	}
	LType::CastMethod	castMethod = xval->GetType().CanCastTo( typeCast ) ;
	if ( castMethod == LType::castImpossible )
	{
		// 変換不可能
		OnError( errorUnavailableCast_opt1_opt2,
					xval->GetType().GetTypeName().c_str(),
					typeCast.GetTypeName().c_str() ) ;
		return	xval ;
	}
	if ( xval->IsFetchAddrPointer()
		&& !(typeCast.GetModifiers() & LType::modifierFetchAddr) )
	{
		// fetch_addr から通常のポインタ等へは変換不可能
		OnError( errorUnavailableCast_opt1_opt2,
					xval->GetType().GetTypeName().c_str(),
					typeCast.GetTypeName().c_str() ) ;
		return	xval ;
	}
	if ( xval->GetType() == typeCast )
	{
		// 同じ型はそのまま
		return	xval ;
	}
	if ( !explicitCast && (castMethod >= LType::castableExplicitly) )
	{
		// 警告
		OnWarning( warningImplicitCast_opt1_opt2,
					(castMethod >= LType::castableDangerous)
									? warning1 : warning2,
					xval->GetType().GetTypeName().c_str(),
					typeCast.GetTypeName().c_str() ) ;
	}
	if ( (castMethod == LType::castableDataToPtr)
		|| (castMethod == LType::castableConstDataToPtr) )
	{
		// 参照型のポインタ化
		if ( xval->IsRefPointer() || xval->IsOnLocal() )
		{
			return	EvalCastTypeTo
				( MakeRefPointerToPointer( std::move(xval) ),
										typeCast, explicitCast ) ;
		}
		else
		{
			OnError( errorCannotNonRefToPointer ) ;
			return	xval ;
		}
	}
	if ( castMethod == LType::castableObjectToPtr )
	{
		// オブジェクトのポインタ化
		return	EvalCastTypeTo
			( EvalCloneAsPointer( std::move(xval) ), typeCast, explicitCast ) ;
	}

	// xval の型情報を変更しても大丈夫なように複製する
	xval = EvalMakeUniqueValue( xval ) ;
	if ( xval->IsConstExpr() )
	{
		xval = EvalMakeInstance( std::move(xval) ) ;
	}

	if ( (castMethod == LType::castableJust)
		|| (castMethod == LType::castableUpCast) )
	{
		// 型情報のみ変更
		assert( xval->GetType().IsObject() ) ;
		LType::Modifiers	accMod = xval->GetType().GetModifiers()
												& LType::accessMask ;
		xval->Type() = typeCast ;
		xval->Type().SetModifiers( typeCast.GetAccessModifier() | accMod ) ;
		return	xval ;
	}

	if ( (castMethod == LType::castableConstCast)
		|| (castMethod == LType::castableDownCast)
		|| (castMethod == LType::castableCrossCast) )
	{
		//アップキャスト以外は動的キャストを行う
		assert( xval->GetType().IsRuntimeObject() ) ;
		if ( xval->IsConstExpr() )
		{
			// 定数式
			if ( xval->GetObject() != nullptr )
			{
				LObjPtr	pObj = xval->GetObject()->CastClassTo( typeCast.GetClass() ) ;
				if ( pObj == nullptr )
				{
					OnError( errorFailedConstExprToCast_opt1_opt2,
								xval->GetType().GetTypeName().c_str(),
								typeCast.GetTypeName().c_str() ) ;
				}
				return	std::make_shared<LExprValue>
							( typeCast, pObj, true, xval->IsUniqueObject() ) ;
			}
			else
			{
				return	std::make_shared<LExprValue>( typeCast, nullptr, true, false ) ;
			}
		}

		// 実行時式でのオブジェクトへの変換
		assert( IsExprValueOnStack( xval ) ) ;
		ssize_t	iStack = GetBackIndexOnStack( xval ) ;
		assert( iStack >= 0 ) ;
		if ( iStack < 0 )
		{
			OnError( errorAssertNotValueOnStack ) ;
		}
		LCodeBuffer::ImmediateOperand	immop ;
		immop.pClass = typeCast.GetClass() ;

		AddCode( LCodeBuffer::codeCastObject, iStack, 0, 0, 0, &immop ) ;
		AddCode( LCodeBuffer::codeFreeObject, iStack + 1 ) ;
		AddCode( LCodeBuffer::codeMove, 0, iStack + 1, 0, 1 ) ;

		// 型情報変更
		assert( xval->GetType().IsRuntimeObject() ) ;
		LType::Modifiers	accMod = xval->GetType().GetModifiers()
													& LType::accessMask ;
		xval->Type() = typeCast ;
		xval->Type().SetModifiers( typeCast.GetAccessModifier() | accMod ) ;
		return	xval ;
	}

	if ( typeCast.IsBoolean() )
	{
		// boolean への変換
		return	EvalAsBoolean( std::move(xval) ) ;
	}

	if ( (castMethod == LType::castablePrecision)
		|| (castMethod == LType::castableConvertNum) )
	{
		// 数値の型変更
		assert( xval->GetType().IsPrimitive() ) ;
		if ( xval->IsConstExpr() )
		{
			// 定数式
			if ( typeCast.IsFloatingPointNumber() )
			{
				return	std::make_shared<LExprValue>
							( typeCast.GetPrimitive(),
								LValue::MakeDouble( xval->AsDouble() ), true ) ;
			}
			else
			{
				LLong	val = xval->AsInteger() ;
				size_t	nBits = typeCast.GetDataBytes() * 8 ;
				if ( nBits != 64 )
				{
					if ( typeCast.IsUnsignedInteger() )
					{
						val &= (((LLong)1) << nBits) - 1 ;
					}
					else
					{
						val <<= (64 - nBits) ;
						val >>= (64 - nBits) ;
					}
				}
				return	std::make_shared<LExprValue>
							( typeCast.GetPrimitive(),
									LValue::MakeLong( val ), true ) ;
			}
		}
		xval = ExprMakeOnStack( std::move(xval) ) ;
		assert( IsExprValueOnStack( xval ) ) ;
		ssize_t	iStack = GetBackIndexOnStack( xval ) ;
		assert( iStack >= 0 ) ;
		if ( iStack < 0 )
		{
			OnError( errorAssertNotValueOnStack ) ;
		}
		if ( typeCast.IsFloatingPointNumber() )
		{
			if ( !xval->GetType().IsFloatingPointNumber() )
			{
				if ( xval->GetType().IsUnsignedInteger() )
				{
					AddCode( LCodeBuffer::codeUintToFloat, iStack, iStack ) ;
				}
				else
				{
					AddCode( LCodeBuffer::codeIntToFloat, iStack, iStack ) ;
				}
			}
		}
		else
		{
			if ( xval->GetType().IsFloatingPointNumber() )
			{
				if ( typeCast.IsUnsignedInteger() )
				{
					AddCode( LCodeBuffer::codeFloatToUint, iStack, iStack ) ;
				}
				else
				{
					AddCode( LCodeBuffer::codeFloatToInt, iStack, iStack ) ;
				}
			}
			else if ( castMethod == LType::castableConvertNum )
			{
				/* ※高速化のため省略
				size_t	nBits = typeCast.GetDataBytes() * 8 ;
				if ( typeCast.IsUnsignedInteger() )
				{
					LCodeBuffer::ImmediateOperand	immop ;
					immop.pfnOp2 = &LIntegerClass::operator_bit_and ;
					//
					LExprValuePtr	xvalMask =
							ExprLoadImmInteger( (((LLong)1) << nBits) - 1 ) ;
					assert( GetBackIndexOnStack(xvalMask) == 0 ) ;
					assert( GetBackIndexOnStack( xval ) == iStack + 1 ) ;
					//
					AddCode( LCodeBuffer::codeBinaryOperate,
							iStack+1, 0, Symbol::opBitAnd, 1, &immop ) ;
					AddCode( LCodeBuffer::codeMove, 0, iStack+1 ) ;
				}
				else
				{
					LCodeBuffer::ImmediateOperand	immop ;
					immop.pfnOp2 = &LIntegerClass::operator_shift_left ;
					//
					LExprValuePtr	xvalShifter = ExprLoadImmInteger( 64 - nBits ) ;
					assert( GetBackIndexOnStack(xvalShifter) == 0 ) ;
					assert( GetBackIndexOnStack( xval ) == iStack + 1 ) ;
					//
					AddCode( LCodeBuffer::codeBinaryOperate,
							iStack, 0, Symbol::opShiftLeft, 0, &immop ) ;
					//
					immop.pfnOp2 = &LIntegerClass::operator_shift_right_a ;
					AddCode( LCodeBuffer::codeBinaryOperate,
							0, 1, Symbol::opShiftRight, 2, &immop ) ;
					//
					AddCode( LCodeBuffer::codeMove, 0, iStack+1 ) ;
				}
				ExprCodeFreeTempStack() ;
				*/
			}
		}
		xval->Type() = LType( typeCast.GetPrimitive(),
								xval->GetType().GetModifiers() ) ;
		return	xval ;
	}

	if ( (castMethod == LType::castableUpCastPtr)
		|| (castMethod == LType::castableConstCastPtr)
		|| (castMethod == LType::castableDownCastPtr)
		|| (castMethod == LType::castableCastStrangerPtr) )
	{
		// ポインタの型変更
		assert( xval->GetType().IsPointer() ) ;
		assert( typeCast.IsPointer() ) ;
		ssize_t	iCastOffset = 0 ;
		//
		LStructureClass *	pStructClass = xval->GetType().GetPtrStructureClass() ;
		LStructureClass *	pCastStructClass = typeCast.GetPtrStructureClass() ;
		if ( (pStructClass != nullptr) && (pCastStructClass != nullptr) )
		{
			// ダウンキャスト？
			const size_t *	pOffset =
					pStructClass->GetOffsetCastTo( pCastStructClass ) ;
			if ( pOffset != nullptr )
			{
				iCastOffset = (ssize_t) *pOffset ;
			}
			else
			{
				// アップキャスト？
				pOffset = pCastStructClass->GetOffsetCastTo( pStructClass ) ;
				if ( pOffset != nullptr )
				{
					iCastOffset = - (ssize_t) *pOffset ;
				}
			}
		}
		LType	typeValue( typeCast.GetClass(), xval->GetType().GetModifiers() ) ;
		if ( iCastOffset != 0 )
		{
			LExprValuePtr	xvalCastPtr =
				std::make_shared<LExprValue>( typeValue, nullptr, false, false ) ;
			xvalCastPtr->SetOptionPointerOffset
					( xval, LExprValue::MakeConstExprInt(iCastOffset) ) ;
			if ( explicitCast )
			{
				return	EvalCheckPointerAlignmenet( std::move(xvalCastPtr) ) ;
			}
			else
			{
				return	xvalCastPtr ;
			}
		}
		else
		{
			xval->Type() = typeValue ;
			if ( explicitCast )
			{
				return	EvalCheckPointerAlignmenet( std::move(xval) ) ;
			}
			else
			{
				return	xval ;
			}
		}
	}

	if ( castMethod == LType::castableArrayToPtr )
	{
		assert( xval->GetType().IsDataArray() ) ;
		xval = DiscloseRefPointerType( std::move(xval) ) ;
		assert( !xval->GetType().IsDataArray() ) ;
		assert( xval->GetType() == typeCast ) ;
		return	EvalCastTypeTo( std::move(xval), typeCast ) ;
	}

	if ( (castMethod == LType::castableBoxing)
		|| (castMethod == LType::castableNumToStr) )
	{
		assert( xval->GetType().IsPrimitive() ) ;
		if ( xval->IsConstExpr() )
		{
			// 定数式
			if ( typeCast.IsString() )
			{
				LString	str ;
				if ( xval->GetType().IsFloatingPointNumber() )
				{
					str = LString::NumberOf( xval->AsDouble() ) ;
				}
				else
				{
					str = LString::IntegerOf( xval->AsInteger() ) ;
				}
				LClass *	pClass = m_vm.GetStringClass() ;
				return	std::make_shared<LExprValue>
							( LType(pClass),
								new LStringObj( pClass, str ), true, false ) ;
			}
			else if ( typeCast.IsDoubleObj() )
			{
				LClass *	pClass = m_vm.GetDoubleObjClass() ;
				return	std::make_shared<LExprValue>
							( LType(pClass),
								new LDoubleObj( pClass, xval->AsDouble() ), true, false ) ;
			}
			else if ( typeCast.IsIntegerObj() )
			{
				LClass *	pClass = m_vm.GetIntegerObjClass() ;
				return	std::make_shared<LExprValue>
							( LType(pClass),
								new LIntegerObj( pClass, xval->AsInteger() ), true, false ) ;
			}
		}

		// 実行時式でのオブジェクトへの変換
		xval = ExprMakeOnStack( std::move(xval) ) ;
		assert( IsExprValueOnStack( xval ) ) ;
		ssize_t	iStack = GetBackIndexOnStack( xval ) ;
		assert( iStack >= 0 ) ;
		if ( iStack < 0 )
		{
			OnError( errorAssertNotValueOnStack ) ;
		}
		LExprValuePtr	xvalObj ;
		if ( xval->GetType().IsFloatingPointNumber() )
		{
			AddCode( LCodeBuffer::codeFloatToObject, iStack ) ;

			xvalObj = std::make_shared<LExprValue>
						( LType(m_vm.GetDoubleObjClass()), nullptr, false, false ) ;
		}
		else
		{
			AddCode( LCodeBuffer::codeIntToObject, iStack ) ;

			xvalObj = std::make_shared<LExprValue>
					( LType(m_vm.GetIntegerObjClass()), nullptr, false, false ) ;
		}
		PushExprValueOnStack( xvalObj ) ;
		ExprTreatObjectChain( xvalObj ) ;

		if ( castMethod == LType::castableNumToStr )
		{
			iStack = GetBackIndexOnStack( xvalObj ) ;
			assert( iStack >= 0 ) ;
			if ( iStack < 0 )
			{
				OnError( errorAssertNotValueOnStack ) ;
			}
			AddCode( LCodeBuffer::codeObjectToString, iStack ) ;

			xvalObj = std::make_shared<LExprValue>
						( LType(m_vm.GetStringClass()), nullptr, false, false ) ;
			PushExprValueOnStack( xvalObj ) ;
			ExprTreatObjectChain( xvalObj ) ;
		}
		return	xvalObj ;
	}

	if ( castMethod == LType::castableUnboxing )
	{
		if ( xval->IsConstExpr() )
		{
			// 定数式
			if ( typeCast.IsInteger() || typeCast.IsBoolean() )
			{
				return	LExprValue::MakeConstExprInt( xval->AsInteger() ) ;
			}
			else if ( typeCast.IsFloatingPointNumber() )
			{
				return	LExprValue::MakeConstExprFloat( xval->AsDouble() ) ;
			}
			else
			{
				OnError( errorUnavailableCast_opt1_opt2,
							xval->GetType().GetTypeName().c_str(),
							typeCast.GetTypeName().c_str() ) ;
				return	xval ;
			}
		}

		// 結果を受け取るスタック確保
		LExprValuePtr	xvalNum =
				std::make_shared<LExprValue>( typeCast, nullptr, false, false ) ;
		PushExprValueOnStack( xvalNum ) ;
		AddCode( LCodeBuffer::codeAllocStack, 0, 0, 0, 1 ) ;

		// 実行時式での数値への変換
		xval = ExprMakeOnStack( std::move(xval) ) ;
		assert( IsExprValueOnStack( xval ) ) ;
		ssize_t	iSrc = GetBackIndexOnStack( xval ) ;
		ssize_t	iDst = GetBackIndexOnStack( xvalNum ) ;
		assert( iSrc >= 0 ) ;
		assert( iDst >= 0 ) ;
		if ( (iSrc < 0) || (iDst < 0) )
		{
			OnError( errorAssertNotValueOnStack ) ;
		}
		if ( typeCast.IsInteger() || typeCast.IsBoolean() )
		{
			AddCode( LCodeBuffer::codeObjectToInt, iSrc, iDst ) ;
		}
		else if ( typeCast.IsFloatingPointNumber() )
		{
			AddCode( LCodeBuffer::codeObjectToFloat, iSrc, iDst ) ;
		}
		else
		{
			OnError( errorUnavailableCast_opt1_opt2,
						xval->GetType().GetTypeName().c_str(),
						typeCast.GetTypeName().c_str() ) ;
		}
		return	xvalNum ;
	}

	OnError( errorUnavailableCast_opt1_opt2,
				xval->GetType().GetTypeName().c_str(),
				typeCast.GetTypeName().c_str() ) ;
	return	xval ;
}

// 定数式の関数参照をキャストする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ConstExprCastFuncTo
	( LExprValuePtr xval, const LType& typeCast, bool explicitCast )
{
	assert( xval->IsConstExpr() ) ;
	assert( typeCast.IsFunction() ) ;
	LFunctionClass *	pFuncClass = typeCast.GetFunctionClass() ;
	std::shared_ptr<LPrototype>
						pFuncProto = pFuncClass->GetFuncPrototype() ;

	if ( xval->IsFunctionVariation() )
	{
		// 静的な関数群
		const LFunctionVariation *	pFuncVar = xval->GetFuncVariation() ;
		assert( pFuncVar != nullptr ) ;
		if ( pFuncProto == nullptr )
		{
			if ( pFuncVar->size() > 1 )
			{
				OnError( errorMultiCandidateFunctions,
						pFuncVar->GetFunctionName().c_str() ) ;
			}
			LPtr<LFunctionObj>	pFunc = pFuncVar->at(0) ;
			return	std::make_shared<LExprValue>
							( typeCast, pFunc, true, true ) ;
		}
		else
		{
			for ( size_t i = 0; i < pFuncVar->size(); i ++ )
			{
				LPtr<LFunctionObj>	pFunc = pFuncVar->at(i) ;
				if ( pFunc->IsEqualPrototype( *pFuncProto ) )
				{
					return	std::make_shared<LExprValue>
									( typeCast, pFunc, true, true ) ;
				}
			}
			OnError( errorNotFoundFunction,
					pFuncVar->GetFunctionName().c_str(),
					typeCast.GetTypeName().c_str() ) ;
		}
	}
	else if ( xval->IsRefVirtualFunction() )
	{
		// 仮想関数リスト
		LClass *	pVirtClass = xval->GetVirtFuncClass() ;
		const std::vector<size_t> *
					pVirtFuncs =  xval->GetVirtFunctions() ;
		const LVirtualFuncVector&
					vecVirtFunc = pVirtClass->GetVirtualVector() ;
		if ( pFuncProto == nullptr )
		{
			if ( pVirtFuncs->size() > 1 )
			{
				OnError( errorMultiCandidateFunctions ) ;
			}
			assert( pVirtFuncs->size() >= 1 ) ;
			LPtr<LFunctionObj>	pFunc =
						vecVirtFunc.GetFunctionAt( pVirtFuncs->at(0) ) ;
			return	std::make_shared<LExprValue>
							( typeCast, pFunc, true, true ) ;
		}
		for ( size_t i = 0; i < pVirtFuncs->size(); i ++ )
		{
			LPtr<LFunctionObj>	pFunc =
					vecVirtFunc.GetFunctionAt( pVirtFuncs->at(i) ) ;
			assert( pFunc != nullptr ) ;
			if ( pFunc->IsEqualPrototype( *pFuncProto ) )
			{
				return	std::make_shared<LExprValue>
							( typeCast, pFunc, true, true ) ;
			}
		}
		OnError( errorNotFoundFunction, nullptr,
				typeCast.GetTypeName().c_str() ) ;
	}
	else if ( xval->IsOperatorFunction() )
	{
		// 演算子
		const Symbol::OperatorDef *	popDef = xval->GetOperatorFunction() ;
		assert( popDef != nullptr ) ;
		const Symbol::OperatorDesc&	opDesc = GetOperatorDesc(popDef->m_opIndex) ;
		//
		LString	strOpFunc ;
		if ( popDef->m_pThisClass != nullptr )
		{
			strOpFunc += popDef->m_pThisClass->GetFullClassName() ;
			strOpFunc += L"::" ;
		}
		strOpFunc += L"operator " ;
		strOpFunc += opDesc.pwszName ;
		if ( opDesc.pwszPairName != nullptr )
		{
			strOpFunc += opDesc.pwszPairName ;
		}
		OnError( errorUnavailableCast_opt1_opt2,
					strOpFunc.c_str(), typeCast.GetTypeName().c_str() ) ;
	}
	else if ( xval->IsConstExprFunction() )
	{
		// 関数オブジェクト
		LPtr<LFunctionObj>	pFunc = xval->GetConstExprFunction() ;
		if ( (pFunc != nullptr) && (pFuncProto != nullptr) )
		{
			if ( !pFunc->IsEqualPrototype( *pFuncProto ) )
			{
				OnError( errorUnavailableCast_opt1_opt2,
							xval->GetType().GetTypeName().c_str(),
							typeCast.GetTypeName().c_str() ) ;
				return	xval ;
			}
		}
		return	std::make_shared<LExprValue>( typeCast, pFunc, true, true ) ;
	}
	else if ( xval->IsNull() )
	{
		return	std::make_shared<LExprValue>( typeCast, nullptr, true, false ) ;
	}
	else
	{
		// 変換できない
		OnError( errorUnavailableCast_opt1_opt2,
					xval->GetType().GetTypeName().c_str(),
					typeCast.GetTypeName().c_str() ) ;
	}
	return	xval ;
}

// 配列オブジェクトを Type[] へ変換する
//////////////////////////////////////////////////////////////////////////////
LPtr<LArrayObj> LCompiler::ConstExprCastToArrayElementType
	( LPtr<LArrayObj> pArrayObj, LClass * pElementType )
{
	if ( (pArrayObj == nullptr) || (pElementType == nullptr) )
	{
		return	pArrayObj ;
	}
	LClass *	pArrayClass = m_vm.GetArrayClassAs( pElementType ) ;
	LObject *	pObj = pArrayObj->CastClassTo( pArrayClass ) ;
	if ( pObj == nullptr )
	{
		OnError( errorFailedConstExprToCast_opt1_opt2,
				LType(pArrayObj->GetClass()).GetTypeName().c_str(),
				LType(pArrayClass).GetTypeName().c_str() ) ;
	}
	LArrayObj *	pCastedObj = dynamic_cast<LArrayObj*>( pObj ) ;
	if ( pCastedObj == nullptr )
	{
		delete	pObj ;
	}
	return	pCastedObj ;
}

// 辞書配列オブジェクトを Map<Type> へ変換する
//////////////////////////////////////////////////////////////////////////////
LPtr<LMapObj> LCompiler::ConstExprCastToMapElementType
	( LPtr<LMapObj> pMapObj, LClass * pElementType )
{
	if ( (pMapObj == nullptr) || (pElementType == nullptr) )
	{
		return	pMapObj ;
	}
	LClass *	pMapClass = m_vm.GetMapClassAs( pElementType ) ;
	LObject *	pObj = pMapObj->CastClassTo( pMapClass ) ;
	if ( pObj == nullptr )
	{
		OnError( errorFailedConstExprToCast_opt1_opt2,
				LType(pMapObj->GetClass()).GetTypeName().c_str(),
				LType(pMapClass).GetTypeName().c_str() ) ;
	}
	LMapObj *	pCastedObj = dynamic_cast<LMapObj*>( pObj ) ;
	if ( pCastedObj == nullptr )
	{
		delete	pObj ;
	}
	return	pCastedObj ;
}

// ポインタのアライメントチェックする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalCheckPointerAlignmenet( LExprValuePtr xvalPtr )
{
	if ( !(xvalPtr->GetType().IsPointer()) )
	{
		return	xvalPtr ;
	}
	LType			typePtr = xvalPtr->GetType() ;
	LExprValuePtr	xvalOffset ;
	ssize_t			iPtrOffset ;
	iPtrOffset = EvalPointerOffset( xvalPtr, xvalOffset, 0 ) ;

	if ( !(xvalPtr->GetType().IsPointer()) )
	{
		assert( xvalOffset == nullptr ) ;
		assert( iPtrOffset == 0 ) ;
		OnError( errorAssertPointer ) ;
		return	xvalPtr ;
	}
	LPointerClass *	pPtrClass = typePtr.GetPointerClass() ;
	assert( pPtrClass != nullptr ) ;

	const size_t	nAlign = pPtrClass->GetElementAlign() ;

	if ( xvalOffset == nullptr )
	{
		if ( xvalPtr->IsConstExpr() )
		{
			LObjPtr	pPtr = xvalPtr->GetObject() ;
			if ( pPtr == nullptr )
			{
				return	xvalPtr ;
			}
			LPointerObj *	pPtrObj = dynamic_cast<LPointerObj*>( pPtr.Ptr() ) ;
			if ( (pPtrObj != nullptr)
				&& !pPtrObj->CheckAlignment( iPtrOffset, nAlign ) )
			{
				OnError( errorMissmatchPointerAlignment ) ;
			}
			if ( pPtrObj != nullptr )
			{
				pPtrObj = new LPointerObj( *pPtrObj ) ;
				*pPtrObj += iPtrOffset ;
			}
			return	std::make_shared<LExprValue>( typePtr, pPtrObj, true, false ) ;
		}
		if ( (iPtrOffset % nAlign) != 0 )
		{
			OnWarning( warningMissmatchPointerAlignment, warning2 ) ;
		}
		if ( IsExprValueOnStack( xvalPtr ) )
		{
			const ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
			assert( iStackPtr >= 0 ) ;
			AddCode( LCodeBuffer::codeCheckPtrAlign,
						iStackPtr, 0, nAlign, iPtrOffset ) ;
		}
		if ( iPtrOffset != 0 )
		{
			LExprValuePtr	xvalTempPtr =
				std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
			xvalTempPtr->SetOptionPointerOffset
				( xvalPtr, LExprValue::MakeConstExprInt(iPtrOffset) ) ;
			return	xvalTempPtr ;
		}
		else
		{
			return	xvalPtr ;
		}
	}
	else
	{
		if ( xvalOffset->IsConstExpr() )
		{
			if ( ((xvalOffset->AsInteger() + iPtrOffset) % nAlign) != 0 )
			{
				OnWarning( warningMissmatchPointerAlignment, warning2 ) ;
			}
		}
		if ( xvalPtr->IsOnLocal() && IsExprValueOnStack(xvalOffset) )
		{
			ssize_t	iVar = GetLocalVarIndex( xvalPtr ) ;
			ssize_t	iStack = GetBackIndexOnStack( xvalOffset ) ;
			assert( iVar >= 0 ) ;
			assert( iStack >= 0 ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iVar ;

			AddCode( LCodeBuffer::codeCheckLPtrAlign,
						iStack, 0, nAlign, iPtrOffset, &immop ) ;
		}
		LExprValuePtr	xvalTempPtr =
			std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
		xvalTempPtr->SetOptionPointerOffset( xvalPtr, xvalOffset ) ;
		//
		if ( iPtrOffset != 0 )
		{
			LExprValuePtr	xvalTempPtr2 =
				std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
			xvalTempPtr2->SetOptionPointerOffset
				( xvalTempPtr, LExprValue::MakeConstExprInt(iPtrOffset) ) ;
			return	xvalTempPtr2 ;
		}
		else
		{
			return	xvalTempPtr ;
		}
	}
}

// ポインタ（参照型の場合にはポインタへ変換して）の実アドレスをフェッチする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprFetchPointerAddr
	( LExprValuePtr xval,
		size_t iOffset, size_t nRange, LLocalVarPtr pAssumeVar )
{
	if ( xval->IsRefPointer() )
	{
		xval = DiscloseRefPointerType( EvalMakeUniqueValue(xval) ) ;
	}
	xval = EvalLoadToDiscloseRef( std::move(xval) ) ;

	if ( xval->IsFetchAddrPointer() )
	{
		// 既にフェッチ済み
		// ポインタとオフセットを計算する
		LExprValuePtr	xvalPtr = xval ;
		LExprValuePtr	xvalOffset ;
		ssize_t			iPtrOffset ;
		iPtrOffset = EvalPointerOffset( xvalPtr, xvalOffset, iOffset ) ;

		// fetch_addr ローカル変数を参照している場合、フェッチ範囲を更新する
		if ( xvalPtr->IsOnLocal() && (xvalOffset == nullptr) )
		{
			HasFetchPointerRange( xvalPtr, iPtrOffset + nRange ) ;
		}

		// アドレスをコピーする
		assert( xvalPtr->IsFetchAddrPointer() ) ;
		LExprValuePtr	xvalAddr = ExprLoadClone( std::move(xvalPtr) ) ;

		// アドレスを加算する
		if ( iPtrOffset != 0 )
		{
			if ( xvalOffset != nullptr )
			{
				xvalOffset = EvalBinaryOperator
					( std::move(xvalOffset),
							Symbol::opAdd,
							ExprLoadImmInteger(iPtrOffset) ) ;
			}
			else
			{
				xvalOffset = ExprLoadImmInteger(iPtrOffset) ;
			}
		}
		if ( xvalOffset != nullptr )
		{
			LValue::Primitive	primitive = LValue::MakeLong(1) ;
			xvalAddr = EvalCodeBinaryOperate
				( xvalAddr->GetType(),
					std::move(xvalAddr), std::move(xvalOffset),
					Symbol::opAdd,
					&LPointerClass::operator_offset_add,
					primitive.pVoidPtr, nullptr ) ;
		}

		xvalAddr->Type() = xval->GetType() ;
		xvalAddr->Type().SetModifiers
			( xval->GetType().GetModifiers() | LType::modifierFetchAddr ) ;
		return	xvalAddr ;
	}
	else if ( xval->IsOffsetPointer() )
	{
		// オフセット付きポインタ形式のアドレス取得
		LExprValuePtr	xvalPtr = xval ;
		LExprValuePtr	xvalOffset ;
		ssize_t			iPtrOffset ;
		iPtrOffset = EvalPointerOffset( xvalPtr, xvalOffset, iOffset ) ;

		if ( xval->GetType().IsObject()
				&& !xval->GetType().CanArrangeOnBuf() )
		{
			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iPtrOffset ;

			size_t			nAlign = 1 ;
			LPointerClass *	pPtrClass = xval->GetType().GetPointerClass() ;
			if ( pPtrClass != nullptr )
			{
				nAlign = pPtrClass->GetElementAlign() ;
			}
			if ( xvalPtr->IsOnLocal() && (xvalOffset != nullptr)
						&& (GetLocalVarIndex( xvalPtr ) < 0x100) )
			{
				// ローカル変数ポインタと非定数なオフセット
				ssize_t	iVarPtr = GetLocalVarIndex( xvalPtr ) ;
				assert( iVarPtr >= 0 ) ;
				assert( iVarPtr < 0x100 ) ;

				xvalOffset = ExprMakeOnStack( std::move(xvalOffset) ) ;
				const ssize_t	iStackOffset = GetBackIndexOnStack( xvalOffset ) ;
				assert( iStackOffset >= 0 ) ;

				if ( pAssumeVar != nullptr )
				{
					m_ctx->m_fetchVars.push_back
						( FetchAddrVar( GetCurrentCodePointer( false ), pAssumeVar ) ) ;
				}
				AddCode( LCodeBuffer::codeLoadFetchLAddr,
						(size_t) iVarPtr, iStackOffset, nAlign, nRange, &immop ) ;
			}
			else
			{
				LExprValuePtr	xvalAddrPtr = ExprMakeOnStack( std::move(xvalPtr) ) ;
				if ( xvalOffset != nullptr )
				{
					// 非定数なオフセット
					xvalOffset = ExprMakeOnStack( std::move(xvalOffset) ) ;
					const ssize_t	iStackOffset = GetBackIndexOnStack( xvalOffset ) ;
					assert( iStackOffset >= 0 ) ;

					const ssize_t	iStackPtr = GetBackIndexOnStack( xvalAddrPtr ) ;
					assert( iStackPtr >= 0 ) ;

					if ( pAssumeVar != nullptr )
					{
						m_ctx->m_fetchVars.push_back
							( FetchAddrVar( GetCurrentCodePointer( false ), pAssumeVar ) ) ;
					}
					AddCode( LCodeBuffer::codeLoadFetchAddrOffset,
								iStackPtr, iStackOffset, nAlign, nRange, &immop ) ;
				}
				else
				{
					// 定数オフセット
					const ssize_t	iStackPtr = GetBackIndexOnStack( xvalAddrPtr ) ;
					assert( iStackPtr >= 0 ) ;

					if ( pAssumeVar != nullptr )
					{
						m_ctx->m_fetchVars.push_back
							( FetchAddrVar( GetCurrentCodePointer( false ), pAssumeVar ) ) ;
					}
					AddCode( LCodeBuffer::codeLoadFetchAddr,
								iStackPtr, 0, nAlign, nRange, &immop ) ;
				}
			}
			LExprValuePtr	xvalAddr =
				std::make_shared<LExprValue>
					( xval->GetType(), nullptr, false, false ) ;
			xvalAddr->Type().SetModifiers
				( xval->GetType().GetModifiers() | LType::modifierFetchAddr ) ;
			PushExprValueOnStack( xvalAddr ) ;
			return	xvalAddr ;
		}
	}
	else if( xval->GetType().IsObject()
			&& !xval->GetType().CanArrangeOnBuf() )
	{
		// ポインタ又はオブジェクトのアドレス取得
		assert( xval->IsSingle() ) ;
		xval = ExprMakeOnStack( std::move(xval) ) ;
		ssize_t	iStackPtr = GetBackIndexOnStack( xval ) ;
		assert( iStackPtr >= 0 ) ;

		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = iOffset ;

		size_t			nAlign = 1 ;
		LPointerClass *	pPtrClass = xval->GetType().GetPointerClass() ;
		if ( pPtrClass != nullptr )
		{
			nAlign = pPtrClass->GetElementAlign() ;
		}

		if ( pAssumeVar != nullptr )
		{
			m_ctx->m_fetchVars.push_back
				( FetchAddrVar( GetCurrentCodePointer( false ), pAssumeVar ) ) ;
		}
		AddCode( LCodeBuffer::codeLoadFetchAddr,
						iStackPtr, 0, nAlign, nRange, &immop ) ;

		LExprValuePtr	xvalAddr =
			std::make_shared<LExprValue>
					( xval->GetType(), nullptr, false, false ) ;
		xvalAddr->Type().SetModifiers
			( xval->GetType().GetModifiers() | LType::modifierFetchAddr ) ;
		PushExprValueOnStack( xvalAddr ) ;
		return	xvalAddr ;
	}
	OnError( errorCannotFetchAddr, xval->GetType().GetTypeName().c_str() ) ;
	return	xval ;
}

// 参照形式の場合ロードして、ポインタのオフセット形式の場合統合して
// 必ず単一要素の状態にする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalMakeInstance( LExprValuePtr xval )
{
	xval = EvalMakePointerInstance
				( EvalLoadToDiscloseRef( std::move(xval) ) ) ;
	if ( xval == nullptr )
	{
		return	nullptr ;
	}
	return	EvalMakeInstanceIfConstExpr( xval ) ;
}

LExprValuePtr LCompiler::ExprMakeInstance( LExprValuePtr xval )
{
	xval = EvalMakeInstance( std::move(xval) ) ;
	if ( (xval != nullptr) && !HasErrorOnCurrent() )
	{
		if ( xval->IsConstExpr() )
		{
			if ( xval->GetType().IsPrimitive() )
			{
				return	ExprLoadImmPrimitive
						( xval->GetType().GetPrimitive(), xval->Value() ) ;
			}
			else
			{
				return	ExprLoadImmObject
							( xval->GetType(), xval->GetObject(),
									true, xval->IsUniqueObject() ) ;
			}
		}
	}
	return	xval ;
}

// 定数式の特殊表現の場合、一般表現に変換する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalMakeInstanceIfConstExpr( LExprValuePtr xval )
{
	if ( xval->IsConstExpr() )
	{
		if ( xval->IsFunctionVariation() && xval->IsConstExpr() )
		{
			const LFunctionVariation *	pFuncVar = xval->GetFuncVariation() ;
			if ( (pFuncVar != nullptr) && (pFuncVar->size() == 1) )
			{
				// 関数
				LPtr<LFunctionObj>	pFunc = pFuncVar->at(0) ;
				assert( pFunc != nullptr ) ;
				return	std::make_shared<LExprValue>
							( LType( pFunc->GetClass() ), pFunc, true, true ) ;
			}
		}
		if ( xval->IsFunctionVariation()
			|| xval->IsOperatorFunction()
			|| xval->IsRefCallFunction()
			|| xval->IsRefVirtualFunction() )
		{
			// 単一要素化できない関数表現
			OnError( errorExpressionErrorRefFunc ) ;
			return	xval ;
		}
		if ( xval->IsRefPointer() )
		{
			return	EvalLoadToDiscloseRef( std::move(xval) ) ;
		}
		if ( !xval->IsSingle() )
		{
			// 単一要素化できない参照表現
			OnError( errorExpressionErrorNoValue ) ;
			return	xval ;
		}
		if ( xval->IsNamespace() )
		{
			LPtr<LNamespace>	pNamespace = xval->GetNamespace() ;
			return	std::make_shared<LExprValue>
					( LType( pNamespace->GetClass() ), pNamespace, true, true ) ;
		}
		else if ( xval->GetType().IsPrimitive() )
		{
			return	std::make_shared<LExprValue>
					( xval->GetType().GetPrimitive(), xval->Value(), true ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>
					( xval->GetType(), xval->GetObject(),
									true, xval->IsUniqueObject() ) ;
		}
	}
	return	xval ;
}

// ポインタのオフセット形式の場合、
// ポインタを複製してオフセットをポインタに統合する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalMakePointerInstance( LExprValuePtr xval )
{
	if ( xval == nullptr )
	{
		return	nullptr ;
	}
	if ( xval->IsOffsetPointer() && !HasErrorOnCurrent() )
	{
		if ( xval->IsFetchAddrPointer() )
		{
			OnWarning( warningVarIndexWithFetchAddr, warning1 ) ;
		}
		// オフセット計算
		LType			typePtr = xval->GetType() ;
		LExprValuePtr	xvalPtr = xval ;
		LExprValuePtr	xvalOffset ;
		ssize_t			iPtrOffset ;
		iPtrOffset = EvalPointerOffset( xvalPtr, xvalOffset, 0 ) ;

		// ポインタを複製
		if ( xval->IsFetchAddrPointer() )
		{
			typePtr.SetModifiers
				( typePtr.GetModifiers() | LType::modifierFetchAddr ) ;
		}
		xvalPtr = EvalCloneAsPointer( std::move(xvalPtr) ) ;
		xvalPtr->Type() = typePtr ;

		// アドレスを加算する
		if ( (iPtrOffset != 0) || (xvalOffset != nullptr) )
		{
			xvalOffset =
				EvalPointerOffsetInstance( std::move(xvalOffset), iPtrOffset ) ;

			LValue::Primitive		primitive = LValue::MakeLong(1) ;
			Symbol::PFN_OPERATOR2	pfnOpPtrAdd ;
			if ( typePtr.IsFetchAddr() )
			{
				pfnOpPtrAdd = &LPointerClass::operator_offset_add ;
			}
			else
			{
				pfnOpPtrAdd = &LPointerClass::operator_ptr_add ;
			}
			xvalPtr = EvalCodeBinaryOperate
				( typePtr, std::move(xvalPtr), std::move(xvalOffset),
					Symbol::opAdd, pfnOpPtrAdd, primitive.pVoidPtr, nullptr ) ;
		}
		return	xvalPtr ;
	}
	return	xval ;
}

// 最終的なポインタとオフセットを決定する
//////////////////////////////////////////////////////////////////////////////
ssize_t LCompiler::EvalPointerOffset
	( LExprValuePtr& xvalPtr, LExprValuePtr& xvalOffset, ssize_t iOffset )
{
	if ( xvalPtr->IsOffsetPointer() )
	{
		if ( xvalOffset != nullptr )
		{
			if ( xvalOffset->IsConstExpr()
				&& xvalOffset->GetType().IsInteger() )
			{
				iOffset += (ssize_t) xvalOffset->AsInteger() ;
				xvalOffset = nullptr ;
			}
			else
			{
				xvalOffset = EvalLoadToDiscloseRef( std::move(xvalOffset) ) ;
				xvalOffset = 
					EvalCastTypeTo
						( std::move(xvalOffset), LType(LType::typeInt64) ) ;
			}
		}
		LExprValuePtr	xvalTempPtr = xvalPtr ;
		while ( xvalTempPtr->IsOffsetPointer() )
		{
			LExprValuePtr	xvalSubPtr = xvalTempPtr->GetOption1() ;
			LExprValuePtr	xvalSubOffset = xvalTempPtr->GetOption2() ;
			assert( xvalSubOffset != nullptr ) ;

			xvalSubOffset =
				EvalCastTypeTo
					( std::move(xvalSubOffset), LType(LType::typeInt64) ) ;
			assert( xvalSubOffset != nullptr ) ;

			if ( xvalSubOffset->IsConstExpr()
				&& xvalSubOffset->GetType().IsInteger() )
			{
				iOffset += (ssize_t) xvalSubOffset->AsInteger() ;
			}
			else if ( xvalOffset != nullptr )
			{
				xvalOffset = EvalBinaryOperator
					( std::move(xvalSubOffset),
							Symbol::opAdd, std::move(xvalOffset) ) ;
			}
			else
			{
				xvalOffset = xvalSubOffset ;
			}
			xvalTempPtr = xvalSubPtr ;
		}
		xvalPtr = xvalTempPtr ;
	}
	return	iOffset ;
}

LExprValuePtr LCompiler::EvalPointerOffsetInstance
				( LExprValuePtr xvalOffset, ssize_t iOffset )
{
	if ( xvalOffset != nullptr )
	{
		if ( xvalOffset->IsConstExpr()
			&& xvalOffset->GetType().IsInteger() )
		{
			iOffset += (ssize_t) xvalOffset->AsInteger() ;
			xvalOffset = nullptr ;
		}
		else
		{
			xvalOffset = EvalLoadToDiscloseRef( std::move(xvalOffset) ) ;
			xvalOffset = 
				EvalCastTypeTo
					( std::move(xvalOffset), LType(LType::typeInt64) ) ;
		}
	}
	if ( iOffset != 0 )
	{
		if ( xvalOffset != nullptr )
		{
			xvalOffset = EvalBinaryOperator
				( std::move(xvalOffset),
					Symbol::opAdd,
					std::make_shared<LExprValue>
						( LType::typeInt64,
							LValue::MakeLong(iOffset), true ) ) ;
		}
		else
		{
			xvalOffset =
				std::make_shared<LExprValue>
					( LType::typeInt64,
						LValue::MakeLong(iOffset), true ) ;
		}
	}
	return	xvalOffset ;
}

// 参照ポインタの場合、ポインタ形式に変更する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::MakeRefPointerToPointer( LExprValuePtr xval )
{
	if ( xval->IsRefPointer() )
	{
		// ポインタ先への参照
		if ( xval->OptionType() == LExprValue::exprRefPointer )
		{
			assert( IsUniqueExprValue( xval ) ) ;
			if ( !xval->GetType().IsPointer() )
			{
				if ( xval->GetType().IsDataArray() )
				{
					xval->Type() =
						LType( m_vm.GetPointerClassAs
							( xval->GetType().GetDataArrayClass()->GetElementType() ) ) ;
				}
				else
				{
					xval->Type() = LType( m_vm.GetPointerClassAs( xval->GetType() ) ) ;
				}
			}
			xval->MakeRefIntoPointer() ;
			return	xval ;
		}
		else
		{
			// オフセット付きポインタ参照
			assert( xval->OptionType() == LExprValue::exprRefPtrOffset ) ;

			LType	typePtr = xval->GetType() ;
			if ( !typePtr.IsPointer() )
			{
				if ( xval->GetType().IsDataArray() )
				{
					typePtr = LType( m_vm.GetPointerClassAs
							( xval->GetType().GetDataArrayClass()->GetElementType() ) ) ;
				}
				else
				{
					typePtr = LType( m_vm.GetPointerClassAs( xval->GetType() ) ) ;
				}
			}

			LExprValuePtr	xvalPtr = std::make_shared<LExprValue>( *xval ) ;
			xvalPtr->Type() = typePtr ;
			xvalPtr->MakeRefIntoPointer() ;
			return	xvalPtr ;
		}
	}
	else if ( xval->IsOnLocal() )
	{
		// ローカル変数への参照
		if ( xval->GetType().IsPrimitive()
			|| xval->GetType().IsStructure()
			|| xval->GetType().IsDataArray() )
		{
			// ポインタへ変換
			ssize_t	iVar = GetLocalVarIndex( xval ) ;
			assert( iVar >= 0 ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iVar ;

			AddCode( LCodeBuffer::codeLoadLocalPtr,
					0, 0, 0, xval->GetType().GetDataBytes(), &immop ) ;

			LType	typePtr ;
			if ( xval->GetType().IsDataArray() )
			{
				LDataArrayClass *	pArrayClass = xval->GetType().GetDataArrayClass() ;
				typePtr = LType( pArrayClass->GetElementPtrClass() ) ;
			}
			else
			{
				typePtr = LType( m_vm.GetPointerClassAs( xval->GetType() ) ) ;
			}
			xval = std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;

			PushExprValueOnStack( xval ) ;
			ExprTreatObjectChain( xval ) ;

			return	xval ;
		}
		else
		{
			// オブジェクトやポインタの場合にはロードする
			return	EvalLoadToDiscloseRef( std::move(xval) ) ;
		}
	}
	return	xval ;
}

// 参照形式の実行時式を実際にロードして評価する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalLoadToDiscloseRef( LExprValuePtr xval )
{
	if ( xval == nullptr )
	{
		return	nullptr ;
	}
	if ( xval->IsRefPointer() )
	{
		// ポインタ先への参照
		// ポインタへ変換
		LType	typeElement = xval->GetType() ;
		xval = MakeRefPointerToPointer( std::move(xval) ) ;

		if ( !typeElement.IsPrimitive() )
		{
			// 配列や構造体の場合にはポインタに変換する
			if ( typeElement.IsStructure() || typeElement.IsDataArray() )
			{
				return	xval ;
			}
			// 構造体でもプリミティブ型でない参照はエラー
			OnError( errorUnavailablePointerRef ) ;
			return	xval ;
		}
		if ( xval->IsConstExpr() )
		{
			// static const なバッファへの参照は定数値
			LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( xval->GetObject().Ptr() ) ;
			if ( (pPtrObj == nullptr)
				|| (pPtrObj->GetPointer() == nullptr) )
			{
				OnError( exceptionNullPointer ) ;
				return	std::make_shared<LExprValue>() ;
			}
			const LType::Primitive	primType = typeElement.GetPrimitive() ;
			if ( typeElement.IsInteger() || typeElement.IsBoolean() )
			{
				LValue::Primitive	primValue ;
				primValue.longValue = pPtrObj->LoadIntegerAt( 0, primType ) ;
				return	std::make_shared<LExprValue>( primType, primValue, true ) ;
			}
			else
			{
				LValue::Primitive	primValue ;
				primValue.dblValue = pPtrObj->LoadDoubleAt( 0, primType ) ;
				return	std::make_shared<LExprValue>( primType, primValue, true ) ;
			}
		}
		// ポインタ先をロード
		return	ExprLoadPtrPrimitive
					( typeElement.GetPrimitive(), std::move(xval) ) ;
	}
	else if ( xval->IsOnLocal() )
	{
		// ローカル変数
		if ( xval->GetType().IsStructure()
			|| xval->GetType().IsDataArray() )
		{
			// 構造体の場合はポインタに変換する
			return	MakeRefPointerToPointer( std::move(xval) ) ;
		}
		else
		{
			// ローカル変数をロードする
			if ( xval->IsConstExpr() )
			{
				return	std::make_shared<LExprValue>( xval->Clone() ) ; 
			}
			ssize_t	iVar = GetLocalVarIndex( xval ) ;
			assert( iVar >= 0 ) ;

			assert( xval->IsSingle() ) ;
			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = (LLong) iVar ;
			AddCode( LCodeBuffer::codeLoadLocal,
					0, 0, xval->GetType().GetPrimitive(), 0, &immop ) ;
			//
			if ( xval->IsTypeNeededToRelease() )
			{
				AddCode( LCodeBuffer::codeRefObject, 0 ) ;
			}
			xval = std::make_shared<LExprValue>
						( xval->GetType().ExConst(), nullptr, false, false ) ;
			PushExprValueOnStack( xval ) ;
			ExprTreatObjectChain( xval ) ;
			return	xval ;
		}
	}
	if ( xval->IsRefByIndex() )
	{
		// オブジェクト要素への参照
		LExprValuePtr	xvalObj = EvalLoadToDiscloseRef( xval->GetOption1() ) ;
		LExprValuePtr	xvalIndex = EvalLoadToDiscloseRef( xval->GetOption2() ) ;

		xvalIndex = EvalCastTypeTo( std::move(xvalIndex), LType(LType::typeInt64) ) ;

		return	EvalLoadObjectElementAt
					( xval->GetType().ExConst(),
							std::move(xvalObj), std::move(xvalIndex) ) ;
	}
	if ( xval->IsRefByString() )
	{
		// オブジェクト要素への参照
		LExprValuePtr	xvalObj = EvalLoadToDiscloseRef( xval->GetOption1() ) ;
		LExprValuePtr	xvalStr = EvalLoadToDiscloseRef( xval->GetOption2() ) ;

		LClass *	pStrClass = m_vm.GetStringClass() ;
		xvalStr = EvalCastTypeTo( std::move(xvalStr), LType(pStrClass) ) ;

		return	EvalLoadObjectElementAs
					( xval->GetType().ExConst(),
							std::move(xvalObj), std::move(xvalStr) ) ;
	}

	// 参照じゃなければそのまま返す
	return	xval ;
}

// 実行時式で static なバッファへの参照ポインタをロードする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalLoadRefPtrStaticBuf
	( const LType& typeData,
		std::shared_ptr<LArrayBuffer> pBuf, size_t iOffset, size_t nBytes )
{
	LClass *	pPtrClass = m_vm.GetPointerClassAs( typeData ) ;
	LPtr<LPointerObj>	pPtrObj = new LPointerObj( pPtrClass ) ;
	pPtrObj->SetPointer( pBuf, iOffset, nBytes ) ;

	if ( typeData.IsConst() )
	{
		// static const なデータは定数式として評価する
		LExprValuePtr	xvalRefPtr =
			std::make_shared<LExprValue>
				( GetLocalTypeOf(typeData),
					pPtrObj, true, false, LExprValue::exprRefPointer ) ;
		return	DiscloseRefPointerType( xvalRefPtr ) ;
	}
	else
	{
		// ポインタをロードする
		LExprValuePtr	xvalPtr =
				ExprLoadImmObject( LType(pPtrClass), pPtrObj ) ;
		LExprValuePtr	xvalRef =
				std::make_shared<LExprValue>( typeData, nullptr, false, false ) ;
		xvalRef->SetOptionRefPointerOffset
				( xvalPtr, LExprValue::MakeConstExprInt(0) ) ;
		return	xvalRef ;
	}
}

// 実行時式がユニークでない場合、その複製を返す
// 返り値は必ずユニークなので型情報などを変更してもよい
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalMakeUniqueValue( LExprValuePtr& xval )
{
	if ( !IsUniqueExprValue( xval ) )
	{
		// xval の型情報を変更しても大丈夫なように複製する
		if ( IsExprValueOnStack( xval ) )
		{
			return	ExprLoadClone( std::move(xval) ) ;
		}
		else if ( xval->IsOnLocal() )
		{
			return	EvalLoadToDiscloseRef( std::move(xval) ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>( *xval ) ;
		}
	}
	return	xval ;
}

// 実行時式で即値をロードする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprLoadImmPrimitive
				( LType::Primitive type, const LValue::Primitive& val )
{
	LCodeBuffer::ImmediateOperand	immop ;
	immop.value = val ;

	AddCode( LCodeBuffer::codeLoadImm, 0, 0, type, 0, &immop ) ;

	LExprValuePtr	xval =
		std::make_shared<LExprValue>( type, immop.value, false ) ;
	PushExprValueOnStack( xval ) ;

	return	xval ;
}

LExprValuePtr LCompiler::ExprLoadImmInteger( LLong val )
{
	return	ExprLoadImmPrimitive( LType::typeInt64, LValue::MakeLong(val) ) ;
}

// 実行時式で即値オブジェクトをロードする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprLoadImmObject
	( const LType& type, LObjPtr pObj, bool fObjChain, bool fUniqueObj )
{
	if ( (m_ctx == nullptr)
		|| (m_ctx->m_codeBuf == nullptr) )
	{
		OnError( errorUnavailableConstExpr ) ;
		return	std::make_shared<LExprValue>() ;
	}
	assert( type.IsObject() ) ;

	m_ctx->m_codeBuf->AddObjectToPool( pObj ) ;

	LCodeBuffer::ImmediateOperand	immop ;
	immop.value.pObject = pObj.Ptr() ;

	if ( fUniqueObj
		|| (pObj == nullptr)
		|| (dynamic_cast<LNamespace*>( pObj.Ptr() ) != nullptr)
		|| (dynamic_cast<LFunctionObj*>( pObj.Ptr() ) != nullptr) )
	{
		AddCode( LCodeBuffer::codeLoadImm, 0, 0, LType::typeObject, 0, &immop ) ;
		if ( pObj != nullptr )
		{
			AddCode( LCodeBuffer::codeRefObject, 0 ) ;
		}
	}
	else
	{
		AddCode( LCodeBuffer::codeLoadObject, 0, 0, 0, 0, &immop ) ;
	}

	LExprValuePtr	xval = 
			std::make_shared<LExprValue>( type, nullptr, false, false ) ;
	PushExprValueOnStack( xval ) ;

	if ( fObjChain )
	{
		ExprTreatObjectChain( xval ) ;
	}

	return	xval ;
}

LExprValuePtr LCompiler::ExprLoadImmObject( LObjPtr pObj )
{
	if ( pObj == nullptr )
	{
		return	ExprLoadImmObject( LType(m_vm.GetObjectClass()), pObj ) ;
	}
	else
	{
		return	ExprLoadImmObject( LType(pObj->GetClass()), pObj ) ;
	}
}

LExprValuePtr LCompiler::ExprLoadImmString( const LString& str )
{
	LStringClass *	pStrClass = m_vm.GetStringClass() ;
	return	ExprLoadImmObject
		( LType(pStrClass), new LStringObj( pStrClass, str ) ) ;
}

// 実行時式で新規オブジェクトを作成しロードする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprLoadNewObject( LClass * pClass )
{
	LCodeBuffer::ImmediateOperand	immop ;
	immop.pClass = pClass ;

	AddCode( LCodeBuffer::codeNewObject, 0, 0, 0, 0, &immop ) ;

	LExprValuePtr	xval = 
		std::make_shared<LExprValue>( LType(pClass), nullptr, false, false ) ;
	PushExprValueOnStack( xval ) ;
	ExprTreatObjectChain( xval ) ;

	return	xval ;
}

// 実行時式でポインタにバッファを確保する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeAllocBuffer
	( LExprValuePtr& xvalPtr, LExprValuePtr xvalBytes )
{
	if ( !xvalPtr->GetType().IsPointer()
		|| xvalPtr->IsFetchAddrPointer() )
	{
		OnError( errorCannotAllocBuffer ) ;
		return ;
	}
	xvalPtr = ExprMakeOnStack( std::move(xvalPtr) ) ;
	xvalBytes = ExprMakeOnStack( std::move(xvalBytes) ) ;

	ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
	ssize_t	iStackBytes = GetBackIndexOnStack( xvalBytes ) ;

	assert( iStackPtr >= 0 ) ;
	assert( iStackBytes >= 0 ) ;

	LCodeBuffer::ImmediateOperand	immop ;
	immop.value.longValue = 0 ;
	immop.pClass = nullptr ;

	if ( xvalPtr->GetType().IsPointer() )
	{
		LPointerClass *	pPtrClass = xvalPtr->GetType().GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		LType	type = pPtrClass->GetDataType() ;
		if ( type.IsStructure() )
		{
			immop.pClass = type.GetStructureClass() ;
		}
	}

	AddCode( LCodeBuffer::codeAllocBuf, iStackPtr, iStackBytes, 0, 0, &immop ) ;
}

// スタック上にない場合、スタック上に確保する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprMakeOnStack( LExprValuePtr xval )
{
	xval = EvalLoadToDiscloseRef( std::move(xval) ) ;
	if ( !IsExprValueOnStack( xval ) )
	{
		xval = ExprMakeInstance( std::move(xval) ) ;

		if ( !IsExprValueOnStack( xval ) && !HasErrorOnCurrent() )
		{
			OnError( errorExpressionErrorNoValue ) ;
		}
	}
	return	xval ;
}

// ポインタとして複製する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalCloneAsPointer( LExprValuePtr xval )
{
	if ( xval->IsConstExpr() )
	{
		xval = EvalMakeInstance( xval ) ;
	}
	if ( xval->IsConstExpr() )
	{
		xval = std::make_shared<LExprValue>( xval->Clone() ) ;

		LObjPtr	pObj = xval->GetObject() ;
		if ( pObj != nullptr )
		{
			LPtr<LPointerObj>	pPtr = pObj->GetBufferPoiner() ;
			if ( pPtr != nullptr )
			{
				return	std::make_shared<LExprValue>
							( LType(pPtr->GetClass()), pPtr, true, false ) ;
			}
		}
		if ( !xval->GetType().IsObject() )
		{
			OnError( errorCannotNonRefToPointer ) ;
		}
		return	std::make_shared<LExprValue>
					( LType(m_vm.GetPointerClass()), nullptr, true, false ) ;
	}
	else if ( xval->IsOnLocal() )
	{
		if ( xval->GetType().IsStructure()
			|| xval->GetType().IsDataArray() )
		{
			// 構造体の場合はポインタに変換する
			return	MakeRefPointerToPointer( std::move(xval) ) ;
		}
		else if ( xval->GetType().IsObject() )
		{
			// ローカル変数のオブジェクトの内部バッファへのポインタを取得する
			ssize_t	iVar = GetLocalVarIndex( xval ) ;
			assert( iVar >= 0 ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = (LLong) iVar ;
			AddCode( LCodeBuffer::codeLoadLObjectPtr, 0, 0, 0, 0, &immop ) ;

			LType	typePtr = xval->GetType() ;
			if ( !typePtr.IsPointer() )
			{
				typePtr = LType( m_vm.GetPointerClass() ) ;
			}
			xval = std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
			PushExprValueOnStack( xval ) ;
			ExprTreatObjectChain( xval ) ;
			return	xval ;
		}
		else
		{
			OnError( errorCannotNonRefToPointer ) ;
		}
		return	std::make_shared<LExprValue>
					( LType(m_vm.GetPointerClass()), nullptr, true, false ) ;
	}
	else
	{
		xval = ExprMakeOnStack( std::move(xval) ) ;
		if ( !xval->GetType().IsObject() )
		{
			OnError( errorCannotNonRefToPointer ) ;
		}
		ssize_t	iStack = GetBackIndexOnStack( xval ) ;
		assert( iStack >= 0 ) ;

		AddCode( LCodeBuffer::codeLoadObjectPtr, iStack ) ;

		LType	typePtr = xval->GetType() ;
		if ( !typePtr.IsPointer() )
		{
			typePtr = LType( m_vm.GetPointerClass() ) ;
		}
		xval = std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
		PushExprValueOnStack( xval ) ;
		ExprTreatObjectChain( xval ) ;
		return	xval ;
	}
}

// 実行時式で値の複製（参照型の場合には参照先）をロードする
//（オブジェクトやポインタの場合には参照カウンタを AddRef して
//  スタック上に yp バックリンクを作成する）
//（オフセット付きポインタの場合にはオフセットのみ複製した
//  新たなオフセット付きポインタを作成する）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprLoadClone( LExprValuePtr xval )
{
	if ( !IsExprValueOnStack( xval ) )
	{
		return	ExprMakeOnStack( xval ) ;
	}
	assert( xval->IsSingle() ) ;

	ssize_t	iStack = GetBackIndexOnStack( xval ) ;
	assert( iStack >= 0 ) ;

	LExprValuePtr	xvalDup = std::make_shared<LExprValue>( *xval ) ;
	PushExprValueOnStack( xvalDup ) ;
	AddCode( LCodeBuffer::codeLoadStack, iStack ) ;

	if ( xvalDup->IsTypeNeededToRelease() )
	{
		assert( GetBackIndexOnStack(xvalDup) == 0 ) ;
		AddCode( LCodeBuffer::codeRefObject, GetBackIndexOnStack(xvalDup) ) ;
	}
	ExprTreatObjectChain( xvalDup ) ;
	return	xvalDup ;
}

// 実行時式で値の複製（参照型の場合には参照先）をロードする
//（yp リンクなどは行わず、AddRef して単一要素だけをプッシュする）
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprPushClone( LExprValuePtr xval, bool withAddRef )
{
	if ( xval->IsConstExpr() )
	{
		// 即値をロードする
		if ( xval->GetType().IsPrimitive() )
		{
			return	ExprLoadImmPrimitive
						( xval->GetType().GetPrimitive(), xval->Value() ) ;
		}
		else
		{
			assert( withAddRef ) ;
			return	ExprLoadImmObject
						( xval->GetType(), xval->GetObject(),
								false, xval->IsUniqueObject() ) ;
		}
	}
	if ( xval->IsOnLocal() )
	{
		// ローカル変数をロードする
		assert( !xval->GetType().IsStructure() ) ;
		assert( !xval->GetType().IsDataArray() ) ;

		ssize_t	iVar = GetLocalVarIndex( xval ) ;
		assert( iVar >= 0 ) ;

		assert( xval->IsSingle() ) ;
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = (LLong) iVar ;
		AddCode( LCodeBuffer::codeLoadLocal,
				0, 0, xval->GetType().GetPrimitive(), 0, &immop ) ;

		if ( withAddRef && xval->IsTypeNeededToRelease() )
		{
			AddCode( LCodeBuffer::codeRefObject, 0 ) ;
		}

		xval = std::make_shared<LExprValue>( *xval ) ;
		xval->Type() = xval->GetType().ExConst() ;
		PushExprValueOnStack( xval ) ;
		return	xval ;
	}
	if ( xval->IsOffsetPointer() )
	{
		// ポインタをロードする
		const LType		typePtr = xval->GetType() ;
		LExprValuePtr	xvalPtr = xval->GetOption1() ;
		LExprValuePtr	xvalOffset = xval->GetOption2() ;

		if ( xvalPtr->IsOnLocal() )
		{
			xvalPtr = ExprPushClone( std::move(xvalPtr), false ) ;
		}
		else
		{
			assert( IsExprValueOnStack( xvalPtr ) ) ;
			xvalPtr = ExprPushClone( std::move(xvalPtr), false ) ;
		}

		LValue::Primitive		primitive = LValue::MakeLong(1) ;
		Symbol::PFN_OPERATOR2	pfnOpPtrAdd ;
		if ( typePtr.IsFetchAddr() )
		{
			pfnOpPtrAdd = &LPointerClass::operator_offset_add ;
		}
		else
		{
			pfnOpPtrAdd = &LPointerClass::operator_ptr_add ;
			//
			LCodeBuffer::ImmediateOperand	immop ;
			immop.value = primitive ;

			AddCode( LCodeBuffer::codeExImmPrefix,
						0, 0, LType::typeInt64, 0, &immop ) ;
		}

		ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
		ssize_t	iStackOffset = GetBackIndexOnStack( xvalOffset ) ;
		assert( iStackPtr == 0 ) ;
		assert( iStackOffset >= 0 ) ;

		PopExprValueOnStacks( 1 ) ;

		LCodeBuffer::ImmediateOperand	immop ;
		immop.pfnOp2 = pfnOpPtrAdd ;

		AddCode( LCodeBuffer::codeBinaryOperate,
				iStackPtr, iStackOffset, Symbol::opAdd, 0, &immop ) ;

		AddCode( LCodeBuffer::codeMove, 0, iStackPtr + 1, 0, 1 ) ;

		LExprValuePtr	xvalDup =
				std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
		PushExprValueOnStack( xvalDup ) ;
		assert( withAddRef ) ;

		return	xvalDup ;
	}

	if ( !xval->IsSingle()
		|| !IsExprValueOnStack( xval ) )
	{
		OnError( errorAssertNotValueOnStack ) ;
		return	xval ;
	}

	ssize_t	iStack = GetBackIndexOnStack( xval ) ;
	assert( iStack >= 0 ) ;

	LExprValuePtr	xvalDup =
			std::make_shared<LExprValue>
				( xval->GetType(), nullptr, false, false ) ;
	PushExprValueOnStack( xvalDup ) ;
	AddCode( LCodeBuffer::codeLoadStack, iStack ) ;

	if ( withAddRef && xvalDup->IsTypeNeededToRelease() )
	{
		assert( GetBackIndexOnStack(xvalDup) == 0 ) ;
		AddCode( LCodeBuffer::codeRefObject, GetBackIndexOnStack(xvalDup) ) ;
	}
	return	xvalDup ;
}

// 確実に ExprPushClone を実行できるような事前処理
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprPrepareToPushClone( LExprValuePtr xval, bool withAddRef )
{
	if ( xval->IsConstExpr() )
	{
		// 即値は即値のまま
		xval = EvalMakeInstanceIfConstExpr( std::move(xval) ) ;
		//
		if ( !withAddRef && xval->GetType().IsNeededToRelease() )
		{
			// 但しオブジェクトの場合にはオブジェクトチェーンを構築するため
			// 一度スタック上にロードする
			return	ExprLoadClone( std::move(xval) ) ;
		}
		return	xval ;
	}
	if ( xval->IsOnLocal()
		&& !xval->GetType().IsStructure()
		&& !xval->GetType().IsDataArray() )
	{
		// 単一要素のローカル変数は直接ロードできるのでそのまま
		ssize_t	iVar = GetLocalVarIndex( xval ) ;
		assert( iVar >= 0 ) ;

		LLocalVarPtr	pVar = GetLocalVarAt( (size_t) iVar ) ;
		if ( pVar->GetAllocType() != LLocalVar::allocPointer )
		{
			return	xval ;
		}
	}
	if ( withAddRef
		&& xval->IsOffsetPointer() && !xval->IsFetchAddrPointer() )
	{
		// ポインタの場合はオフセットを事前に計算する
		const LType		typePtr = xval->GetType() ;
		LExprValuePtr	xvalPtr = xval ;
		LExprValuePtr	xvalOffset ;
		ssize_t	iOffset = EvalPointerOffset( xvalPtr, xvalOffset, 0 ) ;

		if ( !xvalPtr->IsOnLocal()
			&& !xvalPtr->GetType().IsStructure()
			&& !xvalPtr->GetType().IsDataArray() )
		{
			xvalPtr = ExprMakeOnStack
						( EvalLoadToDiscloseRef( std::move(xvalPtr) ) ) ;
		}
		if ( (iOffset != 0) || (xvalOffset != nullptr) )
		{
			if ( xvalOffset == nullptr )
			{
				assert( iOffset != 0 ) ;
				xvalOffset = ExprLoadImmInteger( iOffset ) ;
			}
			else if ( iOffset != 0 )
			{
				assert( xvalOffset != nullptr ) ;
				LExprValuePtr	xvalAdd = ExprLoadImmInteger( iOffset ) ;
				xvalOffset = EvalBinaryOperator
					( std::move(xvalOffset),
							Symbol::opAdd, std::move(xvalAdd) ) ;
			}
		}
		xval = std::make_shared<LExprValue>( typePtr, nullptr, false, false ) ;
		xval->SetOptionPointerOffset
				( xvalPtr, ExprMakeOnStack( std::move(xvalOffset) ) ) ;
		return	xval ;
	}
	return	ExprMakeInstance( std::move(xval) ) ;
}

// 単一要素がオブジェクトの場合には yp チェーンを構築する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprTreatObjectChain( LExprValuePtr xval )
{
	if ( !xval->IsTypeNeededToRelease() )
	{
		return ;
	}
	assert( xval->IsSingle() ) ;
	if ( !xval->IsSingle() )
	{
		OnError( errorAssertSingleValue ) ;
	}
	if ( (m_ctx == nullptr)
		|| (m_ctx->m_xvaStack.FindBack(xval) != 0) )
	{
		OnError( errorAssertValueOnTopOfStack ) ;
	}

	LExprValuePtr	xvalStackObj = std::make_shared<LExprValue>() ;
	xval->SetOption1( xvalStackObj ) ;

	PushExprValueOnStack( xvalStackObj ) ;
	AddCode( LCodeBuffer::codePushObjChain ) ;
}

// （スタック上以外で）ユニークかどうか判定する
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::IsUniqueExprValue( const LExprValuePtr& xval ) const
{
	long	n = xval.use_count() ;
	if ( IsExprValueOnStack( xval ) )
	{
		n -- ;
	}
	return	(n <= 1) ;
}

// 実行時式でスタック上の値をコピーする
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeMove( LExprValuePtr xvalDst, LExprValuePtr xvalSrc )
{
	xvalSrc = ExprMakeOnStack( std::move(xvalSrc) ) ;

	if ( !IsExprValueOnStack( xvalDst )
		|| !IsExprValueOnStack( xvalSrc ) )
	{
		OnError( errorAssertNotValueOnStack ) ;
		return ;
	}

	ssize_t	iDst = GetBackIndexOnStack( xvalDst ) ;
	ssize_t	iSrc = GetBackIndexOnStack( xvalSrc ) ;
	assert( iDst >= 0 ) ;
	assert( iSrc >= 0 ) ;

	if ( xvalDst->IsTypeNeededToRelease() )
	{
		assert( xvalSrc->IsTypeNeededToRelease() ) ;
		if ( xvalSrc->IsTypeNeededToRelease() )
		{
			AddCode( LCodeBuffer::codeFreeObject, iDst ) ;
		}
	}

	xvalDst = nullptr ;
	xvalSrc = nullptr ;

	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	AddCode( LCodeBuffer::codeMove, iSrc, iDst, 0, nFreeCount ) ;
}

// 実行時式で値をストアする
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeStore
	( LExprValuePtr xvalRefDst, LExprValuePtr xvalSrc, bool disregardConst )
{
	xvalSrc = EvalLoadToDiscloseRef( std::move(xvalSrc) ) ;

	if ( xvalRefDst->GetType().IsConst() && !disregardConst )
	{
		OnError( errorCannotStoreToConst ) ;
		return ;
	}
	if ( !xvalRefDst->IsReference() )
	{
		OnError( errorInvalidStoreLeftExpr ) ;
		return ;
	}

	xvalSrc = EvalCastTypeTo( std::move(xvalSrc), xvalRefDst->GetType() ) ;

	if ( xvalRefDst->IsRefPointer() )
	{
		// ポインタ先への参照
		if ( xvalRefDst->OptionType() == LExprValue::exprRefPointer )
		{
			assert( !xvalRefDst->GetType().IsPointer() ) ;
			if ( xvalRefDst->IsConstExpr()
				|| !xvalRefDst->GetType().IsPrimitive() )
			{
				OnError( errorInvalidStoreLeftExpr ) ;
				return ;
			}
			const LType::Primitive
							primType = xvalRefDst->GetType().GetPrimitive() ;
			const LType		typePtr( m_vm.GetPointerClassAs( xvalRefDst->GetType() ) ) ;
			LExprValuePtr	xvalPtr = std::make_shared<LExprValue>
											( typePtr, nullptr, false, false ) ;
			xvalPtr->SetOptionPointerOffset
						( xvalRefDst, LExprValue::MakeConstExprInt(0) ) ;

			ExprStorePtrPrimitive
				( std::move(xvalPtr), nullptr, primType, std::move(xvalSrc) ) ;
		}
		else
		{
			// オフセット付きポインタ参照
			assert( xvalRefDst->OptionType() == LExprValue::exprRefPtrOffset ) ;
			assert( xvalRefDst->GetType().CanArrangeOnBuf() ) ;
			if ( xvalRefDst->GetType().IsPrimitive() )
			{
				// プリミティブ型へのポインタ先へのストア
				const LType::Primitive
								primType = xvalRefDst->GetType().GetPrimitive() ;
				const LType		typePtr( m_vm.GetPointerClassAs( xvalRefDst->GetType() ) ) ;

				LExprValuePtr	xvalPtr = std::make_shared<LExprValue>( *xvalRefDst ) ;
				xvalPtr->MakeRefIntoPointer() ;
				xvalPtr->Type() = typePtr ;

				ExprStorePtrPrimitive
						( xvalPtr, nullptr, primType, std::move(xvalSrc) ) ;
			}
			else if ( xvalRefDst->GetType().IsStructure() )
			{
				// 構造体へのポインタ先へのストア
				LStructureClass * pStructClass =
									xvalRefDst->GetType().GetStructureClass() ;
				assert( pStructClass != nullptr ) ;

				ExprStorePtrStructure
					( xvalRefDst, nullptr, pStructClass, std::move(xvalSrc) ) ;
			}
			else
			{
				OnError( errorInvalidStoreLeftExpr ) ;
				return ;
			}
		}
		return ;
	}
	else if ( xvalRefDst->IsOnLocal() || xvalRefDst->IsPointerOnLocal() )
	{
		// ローカル変数
		ssize_t	iVarDst = GetLocalVarIndex
							( xvalRefDst->IsOnLocal()
								? xvalRefDst
								: xvalRefDst->GetNonOffsetPointer() ) ;
		assert( iVarDst >= 0 ) ;
		if ( iVarDst >= 0 )
		{
			LLocalVarPtr	pVarDst = GetLocalVarAt( (size_t) iVarDst ) ;
			assert( pVarDst != nullptr ) ;
			assert( pVarDst->GetLocation() == (size_t) iVarDst ) ;

			if ( pVarDst->IsReadOnly() )
			{
				if ( xvalRefDst->GetType().IsPointer() )
				{
					OnError( errorCannotStoreToConstPtr ) ;
				}
				else
				{
					OnError( errorCannotStoreToConst ) ;
				}
			}

			bool	flagClearPtrOffset = true ;
			if ( (pVarDst->GetAllocType() == LLocalVar::allocPointer)
				&& xvalSrc->IsOffsetPointer() )
			{
				LExprValuePtr	xvalPtr = xvalSrc ;
				LExprValuePtr	xvalOffset ;
				ssize_t			iOffset = EvalPointerOffset( xvalPtr, xvalOffset ) ;
				xvalOffset = EvalPointerOffsetInstance( std::move(xvalOffset), iOffset ) ;

				LLocalVarPtr	pVarOffset = GetLocalVarAt( (size_t) iVarDst + 2 ) ;
				assert( pVarOffset != nullptr ) ;

				ExprCodeStore( pVarOffset, xvalOffset ) ;

				xvalSrc = xvalPtr ;
				flagClearPtrOffset = false ;
			}

			xvalSrc = ExprMakeOnStack( std::move(xvalSrc) ) ;
			assert( xvalSrc->IsSingle() ) ;

			ssize_t	iStackSrc = GetBackIndexOnStack( xvalSrc ) ;
			assert( iStackSrc >= 0 ) ;

			xvalRefDst = nullptr ;
			xvalSrc = nullptr ;

			const size_t	nFreeCount = CountFreeTempStack() ;
			PopExprValueOnStacks( nFreeCount ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = (LLong) iVarDst ;

			LType::Primitive	primType = pVarDst->GetType().GetPrimitive() ;

			if ( pVarDst->GetAllocType() == LLocalVar::allocPrimitive )
			{
				AddCode( LCodeBuffer::codeStoreLocal,
						iStackSrc, 0, primType, nFreeCount, &immop ) ;
			}
			else if ( (pVarDst->GetAllocType() == LLocalVar::allocObject)
					|| (pVarDst->GetAllocType() == LLocalVar::allocPointer) )
			{
				if ( IsUniqueExprValue(xvalSrc) )
				{
					AddCode( LCodeBuffer::codeExchangeLocal,
								iStackSrc, 0, primType, nFreeCount, &immop ) ;
				}
				else
				{
					AddCode( LCodeBuffer::codeFreeLocalObj,
								0, 0, primType, 0, &immop ) ;
					AddCode( LCodeBuffer::codeRefObject,
								iStackSrc ) ;
					AddCode( LCodeBuffer::codeStoreLocal,
								iStackSrc, 0, primType, nFreeCount, &immop ) ;
				}
				if ( flagClearPtrOffset
					&& (pVarDst->GetAllocType() == LLocalVar::allocPointer) )
				{
					immop.value.longValue = 0 ;
					AddCode( LCodeBuffer::codeStoreLocalImm,
								0, 0, LType::typeInt64, iVarDst + 2, &immop ) ;
				}
			}
			else
			{
				OnError( errorInvalidStoreLeftExpr ) ;
			}
		}
		return ;
	}
	else if ( xvalRefDst->IsRefByIndex() )
	{
		// オブジェクト要素への参照
		LExprValuePtr	xvalObj = EvalLoadToDiscloseRef( xvalRefDst->GetOption1() ) ;
		LExprValuePtr	xvalIndex = EvalLoadToDiscloseRef( xvalRefDst->GetOption2() ) ;

		xvalIndex = EvalCastTypeTo( std::move(xvalIndex), LType(LType::typeInt64) ) ;

		ExprStoreObjectElementAt
					( std::move(xvalObj),
						std::move(xvalIndex), std::move(xvalSrc) ) ;
		return ;
	}
	else if ( xvalRefDst->IsRefByString() )
	{
		// オブジェクト要素への参照
		LExprValuePtr	xvalObj = EvalLoadToDiscloseRef( xvalRefDst->GetOption1() ) ;
		LExprValuePtr	xvalStr = EvalLoadToDiscloseRef( xvalRefDst->GetOption2() ) ;

		LClass *	pStrClass = m_vm.GetStringClass() ;
		xvalStr = EvalCastTypeTo( std::move(xvalStr), LType(pStrClass) ) ;

		return	ExprStoreObjectElementAs
					( std::move(xvalObj),
						std::move(xvalStr), std::move(xvalSrc) ) ;
		return ;
	}
	else
	{
		OnError( errorInvalidStoreLeftExpr ) ;
	}
}

// 実行時式で値をストアする（:= 演算子での構造体のコピー）
LExprValuePtr LCompiler::ExprCodeStoreMoveStructure
	( LExprValuePtr xvalDst, LExprValuePtr xvalSrc )
{
	const LType		typePtr1 = xvalDst->GetType() ;
	LPointerClass *	pPtrClass = typePtr1.GetPointerClass() ;
	assert( pPtrClass != nullptr ) ;
	if ( pPtrClass == nullptr )
	{
		OnError( errorExpressionError ) ;
		return	xvalDst ;
	}
	LStructureClass *
			pStructClass = pPtrClass->GetBufferType().GetStructureClass() ;
	if ( pStructClass == nullptr )
	{
		OnError( errorExpressionError ) ;
		return	xvalDst ;
	}

	// 構造体にオーバーロードされた演算子取得
	const Symbol::BinaryOperatorDef *	pbopDef =
		pStructClass->GetMatchBinaryOperator
			( Symbol::opStoreMove,
				xvalSrc->GetType(), GetAccessibleScope(pStructClass) ) ;
	if ( pbopDef != nullptr )
	{
		if ( xvalDst->IsFetchAddrPointer() )
		{
			OnError( errorCannotOperateWithFetchAddr_opt2,
					xvalDst->GetType().GetTypeName().c_str(),
					GetOperatorDesc(Symbol::opStoreMove).pwszName ) ;
			return	xvalDst ;
		}
		if ( !pbopDef->m_constThis && pPtrClass->GetBufferType().IsConst() )
		{
			OnError( errorCannotConstVariable_opt1,
					GetOperatorDesc(Symbol::opStoreMove).pwszName ) ;
			return	xvalDst ;
		}

		// 右辺値をキャスト
		xvalSrc = EvalCastTypeTo( std::move(xvalSrc), pbopDef->m_typeRight ) ;

		// 構造体にオーバーロードされた演算子を実行
		return	EvalCodeBinaryOperate
					( pbopDef->m_typeRet,
						std::move(xvalDst), std::move(xvalSrc),
						pbopDef->m_opIndex,
						pbopDef->m_pfnOp,
						pbopDef->m_pInstance, pbopDef->m_pOpFunc ) ;
	}

	// デフォルトの構造体の := 演算子（メモリのコピー）
	if ( pPtrClass->GetBufferType().IsConst() )
	{
		OnError( errorCannotStoreToConst ) ;
		return	xvalDst ;
	}

	// 右辺値をキャスト
	xvalSrc = EvalCastTypeTo
		( std::move(xvalSrc),
				LType( m_vm.GetPointerClassAs
						( LType( pStructClass, LType::modifierConst ) ) ) ) ;

	// copy 関数取得
	std::vector<LExprValuePtr>	xvalArgList ;
	xvalArgList.push_back( std::move(xvalSrc) ) ;
	xvalArgList.push_back
		( LExprValue::MakeConstExprInt( pStructClass->GetStructBytes() ) ) ;

	LArgumentListType	argListType ;
	GetExprListTypes( argListType, xvalArgList ) ;

	const LVirtualFuncVector&	vecVirtFunc = pPtrClass->GetVirtualVector() ;
	const std::vector<size_t> *	pVirtCopy = vecVirtFunc.FindFunction( L"copy" ) ;
	if ( pVirtCopy == nullptr )
	{
		OnError( errorNotFoundFunction,
					L"copy", pPtrClass->GetFullClassName().c_str() ) ;
		return	xvalDst ;
	}
	ssize_t	iVirtCopy =
			vecVirtFunc.FindCallableFunction
				( pVirtCopy, argListType,
					pStructClass, GetAccessibleScope(pStructClass) ) ;
	if ( iVirtCopy < 0 )
	{
		OnError( errorNotFoundFunction,
					L"copy", pPtrClass->GetFullClassName().c_str() ) ;
		return	xvalDst ;
	}
	LPtr<LFunctionObj>
		pFuncObj = vecVirtFunc.GetFunctionAt( (size_t) iVirtCopy ) ;
	assert( pFuncObj != nullptr ) ;
	//
	ExprCallFunction( xvalDst, pFuncObj, xvalArgList ) ;

	return	xvalDst ;
}

// オブジェクト要素をロードする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalLoadObjectElementAt
	( const LType& type,
		LExprValuePtr xvalObj, LExprValuePtr xvalIndex )
{
	xvalObj = EvalMakeInstance( std::move(xvalObj) ) ;
	xvalIndex = EvalMakeInstance( std::move(xvalIndex) ) ;

	assert( xvalObj->IsSingle() ) ;
	assert( xvalIndex->IsSingle() ) ;

	if ( !xvalObj->IsSingle()
		|| !xvalObj->GetType().IsRuntimeObject()
		|| xvalObj->IsFetchAddrPointer() )
	{
		OnError( errorNotObjectToRefElement ) ;
		return	xvalObj ;
	}
	xvalIndex = EvalCastTypeTo
					( std::move(xvalIndex), LType(LType::typeInt64) ) ;

	if ( xvalObj->IsConstExpr() && xvalIndex->IsConstExpr() )
	{
		// 定数式
		LObjPtr	pObj = xvalObj->GetObject() ;
		if ( pObj == nullptr )
		{
			OnError( exceptionNullPointer ) ;
			return	xvalObj ;
		}
		LLong	index = xvalIndex->AsInteger() ;

		LObjPtr	pElement = pObj->GetElementAt( (size_t) index ) ;
		return	std::make_shared<LExprValue>
					( type, pElement, true, xvalObj->IsUniqueObject() ) ;
	}

	xvalObj = ExprMakeOnStack( std::move(xvalObj) ) ;
	xvalIndex = ExprMakeOnStack( std::move(xvalIndex) ) ;

	const ssize_t	iStackObj = GetBackIndexOnStack( xvalObj ) ;
	const ssize_t	iStackIndex = GetBackIndexOnStack( xvalIndex ) ;
	assert( iStackObj >= 0 ) ;
	assert( iStackIndex >= 0 ) ;

	xvalObj = nullptr ;
	xvalIndex = nullptr ;

	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	AddCode( LCodeBuffer::codeGetElement,
				iStackObj, iStackIndex, 0, nFreeCount ) ;

	LExprValuePtr	xvalElement =
			std::make_shared<LExprValue>( type, nullptr, false, false ) ;
	PushExprValueOnStack( xvalElement ) ;
	ExprTreatObjectChain( xvalElement ) ;

	return	xvalElement ;
}

LExprValuePtr LCompiler::EvalLoadObjectElementAs
	( const LType& type,
		LExprValuePtr xvalObj, LExprValuePtr xvalStr )
{
	xvalObj = EvalMakeInstance( std::move(xvalObj) ) ;
	xvalStr = EvalMakeInstance( std::move(xvalStr) ) ;

	assert( xvalObj->IsSingle() ) ;
	assert( xvalStr->IsSingle() ) ;

	if ( !xvalObj->IsSingle()
		|| !xvalObj->GetType().IsRuntimeObject()
		|| xvalObj->IsFetchAddrPointer() )
	{
		OnError( errorNotObjectToRefElement ) ;
		return	xvalObj ;
	}
	xvalStr = EvalCastTypeTo
					( std::move(xvalStr), LType(m_vm.GetStringClass()) ) ;

	if ( xvalObj->IsConstExpr() && xvalStr->IsConstExpr() )
	{
		// 定数式
		LObjPtr	pObj = xvalObj->GetObject() ;
		if ( (pObj == nullptr) || (xvalStr->GetObject() == nullptr) )
		{
			OnError( exceptionNullPointer ) ;
			return	xvalObj ;
		}
		LString	str = xvalStr->AsString() ;

		LObjPtr	pElement = pObj->GetElementAs( str.c_str() ) ;
		return	std::make_shared<LExprValue>
					( type, pElement, true, xvalObj->IsUniqueObject() ) ;
	}

	xvalObj = ExprMakeOnStack( std::move(xvalObj) ) ;
	xvalStr = ExprMakeOnStack( std::move(xvalStr) ) ;

	const ssize_t	iStackObj = GetBackIndexOnStack( xvalObj ) ;
	const ssize_t	iStackStr = GetBackIndexOnStack( xvalStr ) ;
	assert( iStackObj >= 0 ) ;
	assert( iStackStr >= 0 ) ;

	xvalObj = nullptr ;
	xvalStr = nullptr ;

	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	AddCode( LCodeBuffer::codeGetElementAs,
				iStackObj, iStackStr, 0, nFreeCount ) ;

	LExprValuePtr	xvalElement =
			std::make_shared<LExprValue>( type, nullptr, false, false ) ;
	PushExprValueOnStack( xvalElement ) ;
	ExprTreatObjectChain( xvalElement ) ;

	return	xvalElement ;
}

// 実行時式でオブジェクト要素をストアする
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprStoreObjectElementAt
	( LExprValuePtr xvalObj,
		LExprValuePtr xvalIndex, LExprValuePtr xvalElement )
{
	xvalObj = EvalMakeInstance( std::move(xvalObj) ) ;
	xvalIndex = EvalMakeInstance( std::move(xvalIndex) ) ;

	assert( xvalObj->IsSingle() ) ;
	assert( xvalIndex->IsSingle() ) ;

	if ( xvalObj->GetType().IsConst() )
	{
		OnError( errorCannotSetToConstObject ) ;
		return ;
	}
	if ( !xvalObj->IsSingle()
		|| !xvalObj->GetType().IsRuntimeObject()
		|| xvalObj->IsFetchAddrPointer() )
	{
		OnError( errorNotObjectToRefElement ) ;
		return ;
	}
	if ( !xvalElement->GetType().IsRuntimeObject() )
	{
		OnError( errorNotObjectToSetElement ) ;
		return ;
	}
	xvalIndex = EvalCastTypeTo
					( std::move(xvalIndex), LType(LType::typeInt64) ) ;

	xvalObj = ExprMakeOnStack( std::move(xvalObj) ) ;
	xvalIndex = ExprMakeOnStack( std::move(xvalIndex) ) ;
	xvalElement = ExprMakeOnStack( std::move(xvalElement) ) ;

	const ssize_t	iStackObj = GetBackIndexOnStack( xvalObj ) ;
	const ssize_t	iStackIndex = GetBackIndexOnStack( xvalIndex ) ;
	const ssize_t	iStackElement = GetBackIndexOnStack( xvalElement ) ;
	assert( iStackObj >= 0 ) ;
	assert( iStackIndex >= 0 ) ;
	assert( iStackElement >= 0 ) ;

	xvalObj = nullptr ;
	xvalIndex = nullptr ;
	xvalElement = nullptr ;

	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	AddCode( LCodeBuffer::codeSetElement,
				iStackObj, iStackIndex,
				iStackElement, nFreeCount ) ;
}

void LCompiler::ExprStoreObjectElementAs
	( LExprValuePtr xvalObj,
		LExprValuePtr xvalStr, LExprValuePtr xvalElement )
{
	xvalObj = EvalMakeInstance( std::move(xvalObj) ) ;
	xvalStr = EvalMakeInstance( std::move(xvalStr) ) ;

	assert( xvalObj->IsSingle() ) ;
	assert( xvalStr->IsSingle() ) ;

	if ( xvalObj->GetType().IsConst() )
	{
		OnError( errorCannotSetToConstObject ) ;
		return ;
	}
	if ( !xvalObj->IsSingle()
		|| !xvalObj->GetType().IsRuntimeObject()
		|| xvalObj->IsFetchAddrPointer() )
	{
		OnError( errorNotObjectToRefElement ) ;
		return ;
	}
	if ( !xvalElement->GetType().IsRuntimeObject() )
	{
		OnError( errorNotObjectToSetElement ) ;
		return ;
	}
	xvalStr = EvalCastTypeTo
					( std::move(xvalStr), LType(m_vm.GetStringClass()) ) ;

	xvalObj = ExprMakeOnStack( std::move(xvalObj) ) ;
	xvalStr = ExprMakeOnStack( std::move(xvalStr) ) ;
	xvalElement = ExprMakeOnStack( std::move(xvalElement) ) ;

	const ssize_t	iStackObj = GetBackIndexOnStack( xvalObj ) ;
	const ssize_t	iStackStr = GetBackIndexOnStack( xvalStr ) ;
	const ssize_t	iStackElement = GetBackIndexOnStack( xvalElement ) ;
	assert( iStackObj >= 0 ) ;
	assert( iStackStr >= 0 ) ;
	assert( iStackElement >= 0 ) ;

	xvalObj = nullptr ;
	xvalStr = nullptr ;
	xvalElement = nullptr ;

	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	AddCode( LCodeBuffer::codeSetElementAs,
				iStackObj, iStackStr,
				iStackElement, nFreeCount ) ;
}

// 実行時式でポインタの指すメモリからロードする
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::ExprLoadPtrPrimitive
	( LType::Primitive type, LExprValuePtr xvalPtr,
		LExprValuePtr xvalOffset, size_t iOffset )
{
	xvalPtr = EvalLoadToDiscloseRef( std::move(xvalPtr) ) ;
	if ( xvalOffset != nullptr )
	{
		xvalOffset = EvalMakeInstance( std::move(xvalOffset) ) ;
	}
	ssize_t	iPtrOffset = EvalPointerOffset( xvalPtr, xvalOffset, iOffset ) ;
	if ( !xvalPtr->IsOnLocal() )
	{
		xvalPtr = EvalLoadToDiscloseRef( std::move(xvalPtr) ) ;
	}

	assert( xvalPtr->IsSingle() ) ;
	assert( (xvalOffset == nullptr) || xvalOffset->IsSingle() ) ;

	if ( !xvalPtr->IsSingle()
		|| !xvalPtr->GetType().IsObject() )
	{
		OnError( errorLoadByNonObjectPtr ) ;
		return	std::make_shared<LExprValue>
					( LType(type), nullptr, false, false ) ;
	}
	if ( xvalOffset != nullptr )
	{
		if ( xvalPtr->IsFetchAddrPointer() )
		{
			OnWarning( warningVarIndexWithFetchAddr, warning1 ) ;
		}
		// 非定数オフセットを伴うロード命令はないので
		// 一度アドレスをフェッチして直接ロードする
		LExprValuePtr	xvalPtrOffset =
			std::make_shared<LExprValue>
				( LType(m_vm.GetPointerClassAs( LType(type) )),
											nullptr, false, false ) ;
		xvalPtrOffset->SetOptionPointerOffset( xvalPtr, xvalOffset ) ;

		LExprValuePtr	xvalFetchedAddr =
			ExprFetchPointerAddr
				( xvalPtrOffset, iPtrOffset, LType(type).GetDataBytes() ) ;

		const ssize_t	iStackPtr = GetBackIndexOnStack( xvalFetchedAddr ) ;
		assert( iStackPtr >= 0 ) ;

		AddCode( LCodeBuffer::codeLoadByAddr, iStackPtr, 0, type, 0 ) ;

		LExprValuePtr	xvalLoaded =
			std::make_shared<LExprValue>
					( LType(type), nullptr, false, false ) ;
		PushExprValueOnStack( xvalLoaded ) ;

		return	xvalLoaded ;
	}
	else if ( xvalPtr->IsFetchAddrPointer() )
	{
		assert( xvalOffset == nullptr ) ;
		if ( xvalPtr->IsOnLocal() )
		{
			// ローカル上の fetch_addr ポインタ変数を通じてのロード
			ssize_t	iVarPtr = GetLocalVarIndex( xvalPtr ) ;
			assert( iVarPtr >= 0 ) ;

			LLocalVarPtr	pVarPtr = GetLocalVarAt( (size_t) iVarPtr ) ;
			assert( pVarPtr != nullptr ) ;
			pVarPtr->HasFetchPointerRange
					( iPtrOffset + LType(type).GetDataBytes() ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iVarPtr ;

			AddCode( LCodeBuffer::codeLoadByLAddr,
							0, 0, type, iPtrOffset, &immop ) ;
		}
		else
		{
			// 安全でない fetch_addr からのロード
			OnWarning( warningVarIndexWithFetchAddr, warning1 ) ;

			xvalPtr = ExprMakeOnStack( std::move(xvalPtr) ) ;

			const ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
			assert( iStackPtr >= 0 ) ;

			AddCode( LCodeBuffer::codeLoadByAddr,
							iStackPtr, 0, type, iPtrOffset ) ;
		}
		LExprValuePtr	xvalLoaded =
			std::make_shared<LExprValue>
					( LType(type), nullptr, false, false ) ;
		PushExprValueOnStack( xvalLoaded ) ;

		return	xvalLoaded ;
	}
	else
	{
		assert( xvalOffset == nullptr ) ;
		if ( xvalPtr->IsOnLocal() )
		{
			ssize_t	iVar = GetLocalVarIndex( xvalPtr ) ;
			assert( iVar >= 0 ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iVar ;

			AddCode( LCodeBuffer::codeLoadLPtr,
							0, 0, type, iPtrOffset, &immop ) ;
		}
		else
		{
			xvalPtr = ExprMakeOnStack( std::move(xvalPtr) ) ;

			const ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
			assert( iStackPtr >= 0 ) ;

			AddCode( LCodeBuffer::codeLoadByPtr,
							iStackPtr, 0, type, iPtrOffset ) ;
		}
		LExprValuePtr	xvalLoaded =
			std::make_shared<LExprValue>
					( LType(type), nullptr, false, false ) ;
		PushExprValueOnStack( xvalLoaded ) ;

		return	xvalLoaded ;
	}
}

// 実行時式でポインタの指すメモリへストアする
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprStorePtrPrimitive
	( LExprValuePtr xvalPtr,
			LExprValuePtr xvalOffset /*can be null*/,
			LType::Primitive type, LExprValuePtr xvalSrc )
{
	if ( !MustBeRuntimeExpr() )
	{
		return ;
	}
	xvalPtr = EvalLoadToDiscloseRef( std::move(xvalPtr) ) ;
	if ( xvalOffset != nullptr )
	{
		xvalOffset = EvalMakeInstance( std::move(xvalOffset) ) ;
	}
	xvalSrc = EvalMakeInstance( std::move(xvalSrc) ) ;

	ssize_t	iPtrOffset = EvalPointerOffset( xvalPtr, xvalOffset, 0 ) ;
	if ( !xvalPtr->IsOnLocal() )
	{
		xvalPtr = EvalLoadToDiscloseRef( std::move(xvalPtr) ) ;
	}

	assert( xvalPtr->IsSingle() ) ;
	assert( (xvalOffset == nullptr) || xvalOffset->IsSingle() ) ;

	xvalSrc = ExprMakeOnStack( std::move(xvalSrc) ) ;
	assert( GetBackIndexOnStack( xvalSrc ) >= 0 ) ;

	if ( xvalOffset != nullptr )
	{
		if ( xvalPtr->IsFetchAddrPointer() )
		{
			OnWarning( warningVarIndexWithFetchAddr, warning1 ) ;
		}
		// 非定数オフセットを伴うストア命令はないので
		// 一度アドレスをフェッチして直接ストアする
		LExprValuePtr	xvalPtrOffset =
			std::make_shared<LExprValue>
				( LType(m_vm.GetPointerClassAs( LType(type) )),
											nullptr, false, false ) ;
		xvalPtrOffset->SetOptionPointerOffset( xvalPtr, xvalOffset ) ;

		LExprValuePtr	xvalFetchedAddr =
			ExprFetchPointerAddr
				( xvalPtrOffset, iPtrOffset, LType(type).GetDataBytes() ) ;

		const ssize_t	iStackPtr = GetBackIndexOnStack( xvalFetchedAddr ) ;
		assert( iStackPtr >= 0 ) ;

		const ssize_t	iStackSrc = GetBackIndexOnStack( xvalSrc ) ;
		assert( iStackSrc >= 0 ) ;

		AddCode( LCodeBuffer::codeStoreByAddr,
					iStackPtr, iStackSrc, type, 0 ) ;
		return ;
	}
	else if ( xvalPtr->IsFetchAddrPointer() )
	{
		assert( xvalOffset == nullptr ) ;
		if ( xvalPtr->IsOnLocal() )
		{
			// ローカル上の fetch_addr ポインタ変数を通じてのストア
			ssize_t	iVarPtr = GetLocalVarIndex( xvalPtr ) ;
			assert( iVarPtr >= 0 ) ;

			LLocalVarPtr	pVarPtr = GetLocalVarAt( (size_t) iVarPtr ) ;
			assert( pVarPtr != nullptr ) ;
			pVarPtr->HasFetchPointerRange
					( iPtrOffset + LType(type).GetDataBytes() ) ;

			const ssize_t	iStackSrc = GetBackIndexOnStack( xvalSrc ) ;
			assert( iStackSrc >= 0 ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iVarPtr ;

			AddCode( LCodeBuffer::codeStoreByLAddr,
							0, iStackSrc, type, iPtrOffset, &immop ) ;
		}
		else
		{
			// 安全でない fetch_addr へのストア
			OnWarning( warningVarIndexWithFetchAddr, warning1 ) ;

			xvalPtr = ExprMakeOnStack( std::move(xvalPtr) ) ;

			const ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
			assert( iStackPtr >= 0 ) ;

			const ssize_t	iStackSrc = GetBackIndexOnStack( xvalSrc ) ;
			assert( iStackSrc >= 0 ) ;

			AddCode( LCodeBuffer::codeStoreByAddr,
							iStackPtr, iStackSrc, type, iPtrOffset ) ;
		}
		return ;
	}
	else
	{
		assert( xvalOffset == nullptr ) ;
		if ( xvalPtr->IsOnLocal() )
		{
			ssize_t	iVar = GetLocalVarIndex( xvalPtr ) ;
			assert( iVar >= 0 ) ;

			const ssize_t	iStackSrc = GetBackIndexOnStack( xvalSrc ) ;
			assert( iStackSrc >= 0 ) ;

			LCodeBuffer::ImmediateOperand	immop ;
			immop.value.longValue = iVar ;

			AddCode( LCodeBuffer::codeStoreByLPtr,
							0, iStackSrc, type, iPtrOffset, &immop ) ;
		}
		else
		{
			xvalPtr = ExprMakeOnStack( std::move(xvalPtr) ) ;

			const ssize_t	iStackPtr = GetBackIndexOnStack( xvalPtr ) ;
			assert( iStackPtr >= 0 ) ;

			const ssize_t	iStackSrc = GetBackIndexOnStack( xvalSrc ) ;
			assert( iStackSrc >= 0 ) ;

			AddCode( LCodeBuffer::codeStoreByPtr,
							iStackPtr, iStackSrc, type, iPtrOffset ) ;
		}
		return ;
	}
}

void LCompiler::ExprStorePtrStructure
	( LExprValuePtr xvalPtr,
			LExprValuePtr xvalOffset /*can be null*/,
			LStructureClass * pStructClass, LExprValuePtr xvalSrc )
{
	if ( !MustBeRuntimeExpr() )
	{
		return ;
	}
	xvalPtr = EvalLoadToDiscloseRef( std::move(xvalPtr) ) ;
	if ( xvalOffset != nullptr )
	{
		xvalOffset = EvalMakeInstance( std::move(xvalOffset) ) ;
	}
	xvalSrc = EvalMakeInstance( std::move(xvalSrc) ) ;

	// 構造体ポインタのキャスト
	LType	typeStructPtr
				( m_vm.GetPointerClassAs( LType(pStructClass) ) ) ;
	xvalPtr = EvalCastTypeTo( std::move(xvalPtr), typeStructPtr ) ;

	LType	typeStructCPtr
				( m_vm.GetPointerClassAs
						( LType(pStructClass).ConstType() ) ) ;
	xvalSrc = EvalCastTypeTo( std::move(xvalSrc), typeStructCPtr ) ;

	// fetch_addr 判定
	Symbol::PFN_OPERATOR2	pfnOpStore = nullptr ;
	if ( xvalPtr->IsFetchAddrPointer()
		|| xvalSrc->IsFetchAddrPointer() )
	{
		// アドレスをフェッチする
		if ( xvalOffset != nullptr )
		{
			LExprValuePtr	xvalOffPtr =
				std::make_shared<LExprValue>
							( xvalPtr->GetType(), nullptr, false, false ) ;
			xvalOffPtr->SetOptionPointerOffset
							( std::move(xvalPtr), std::move(xvalOffset) ) ;
			xvalPtr = xvalOffPtr ;
		}
		xvalPtr = ExprFetchPointerAddr
					( std::move(xvalPtr),
						0, pStructClass->GetStructBytes() ) ;
		xvalSrc = ExprFetchPointerAddr
					( std::move(xvalSrc),
						0, pStructClass->GetStructBytes() ) ;

		pfnOpStore = &LStructureClass::operator_store_fetch_addr ;
	}
	else
	{
		// 代入元、代入先ポインタを計算する
		xvalPtr = EvalMakePointerInstance( std::move(xvalPtr) ) ;
		xvalSrc = EvalMakePointerInstance( std::move(xvalSrc) ) ;

		pfnOpStore = &LStructureClass::operator_store_ptr ;
	}

	// コピーを実行する
	LValue::Primitive	primBytes =
				LValue::MakeLong( pStructClass->GetStructBytes() ) ;
	EvalCodeBinaryOperate
		( LType(LType::typeInt64),
			std::move(xvalPtr), std::move(xvalSrc),
			Symbol::opStore, pfnOpStore, primBytes.pVoidPtr, nullptr ) ;
}

// 実行時式で演算子を実行する
//////////////////////////////////////////////////////////////////////////////
LExprValuePtr LCompiler::EvalCodeBinaryOperate
	( const LType& typeRet,
		LExprValuePtr xval1, LExprValuePtr xval2,
		Symbol::OperatorIndex opIndex,
		Symbol::PFN_OPERATOR2 pfnOp, void* pInstance,
		LFunctionObj * pOpFunc )
{
	xval1 = EvalMakeInstance( std::move(xval1) ) ;
	xval2 = EvalMakeInstance( std::move(xval2) ) ;

	if ( xval1->IsConstExpr() && xval2->IsConstExpr() && (pfnOp != nullptr) )
	{
		// 定数式
		LValue::Primitive	val1 = xval1->Value() ;
		LValue::Primitive	val2 = xval2->Value() ;
		LValue::Primitive	valRet = pfnOp( val1, val2, pInstance ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		if ( typeRet.IsPrimitive() )
		{
			return	std::make_shared<LExprValue>
						( typeRet.GetPrimitive(), valRet, true ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>
						( typeRet, valRet.pObject, true, false ) ;
		}
	}
	if ( pfnOp == nullptr )
	{
		// オーバーロード関数の呼び出し
		if ( pOpFunc == nullptr )
		{
			OnError( errorNotDefinedOperator_opt2,
					nullptr, GetOperatorDesc(opIndex).pwszName ) ;
			return	std::make_shared<LExprValue>() ;
		}
		std::vector<LExprValuePtr>	arg ;
		arg.push_back( xval2 ) ;

		pOpFunc->AddRef() ;
		return	ExprCallFunction( xval1, pOpFunc, arg ) ;
	}

	xval1 = ExprMakeOnStack( std::move(xval1) ) ;
	xval2 = ExprMakeOnStack( std::move(xval2) ) ;

	const ssize_t	iStackVal1 = GetBackIndexOnStack( xval1 ) ;
	const ssize_t	iStackVal2 = GetBackIndexOnStack( xval2 ) ;
	assert( iStackVal1 >= 0 ) ;
	assert( iStackVal2 >= 0 ) ;

	xval1 = nullptr ;
	xval2 = nullptr ;

	// 演算子実行
	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	LCodeBuffer::ImmediateOperand	immop ;
	if ( pInstance != nullptr )
	{
		immop.pExtra = pInstance ;
		AddCode( LCodeBuffer::codeExImmPrefix,
					0, 0, LType::typeInt64, 0, &immop ) ;
	}

	immop.pfnOp2 = pfnOp ;
	AddCode( LCodeBuffer::codeBinaryOperate,
			iStackVal1, iStackVal2, opIndex, nFreeCount, &immop ) ;

	// 返り値
	LExprValuePtr	xvalRet =
			std::make_shared<LExprValue>( typeRet, nullptr, false, false ) ;
	PushExprValueOnStack( xvalRet ) ;
	ExprTreatObjectChain( xvalRet ) ;

	return	xvalRet ;
}

LExprValuePtr LCompiler::EvalCodeUnaryOperate
	( const LType& typeRet, LExprValuePtr xval,
		Symbol::OperatorIndex opIndex,
		Symbol::PFN_OPERATOR1 pfnOp, void* pInstance,
		LFunctionObj * pOpFunc )
{
	xval = EvalMakeInstance( std::move(xval) ) ;

	if ( xval->IsConstExpr() && (pfnOp != nullptr) )
	{
		// 定数式
		LValue::Primitive	value = xval->Value() ;
		LValue::Primitive	valRet = pfnOp( value, pInstance ) ;
		//
		CheckExceptionInConstExpr() ;
		//
		if ( typeRet.IsPrimitive() )
		{
			return	std::make_shared<LExprValue>
						( typeRet.GetPrimitive(), valRet, true ) ;
		}
		else
		{
			return	std::make_shared<LExprValue>
						( typeRet, valRet.pObject, true, false ) ;
		}
	}
	if ( pOpFunc != nullptr )
	{
		// オーバーロード関数の呼び出し
		if ( pOpFunc == nullptr )
		{
			OnError( errorNotDefinedOperator_opt2,
					nullptr, GetOperatorDesc(opIndex).pwszName ) ;
			return	std::make_shared<LExprValue>() ;
		}
		std::vector<LExprValuePtr>	arg ;
		pOpFunc->AddRef() ;
		return	ExprCallFunction( xval, pOpFunc, arg ) ;
	}

	xval = ExprMakeOnStack( std::move(xval) ) ;

	const ssize_t	iStackVal = GetBackIndexOnStack( xval ) ;
	assert( iStackVal >= 0 ) ;

	xval = nullptr ;

	// 演算子実行
	const size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	LCodeBuffer::ImmediateOperand	immop ;
	if ( pInstance != nullptr )
	{
		immop.pExtra = pInstance ;
		AddCode( LCodeBuffer::codeExImmPrefix,
					0, 0, LType::typeInt64, 0, &immop ) ;
	}

	immop.pfnOp1 = pfnOp ;
	AddCode( LCodeBuffer::codeUnaryOperate,
				iStackVal, 0, opIndex, nFreeCount, &immop ) ;

	// 返り値
	LExprValuePtr	xvalRet =
			std::make_shared<LExprValue>( typeRet, nullptr, false, false ) ;
	PushExprValueOnStack( xvalRet ) ;
	ExprTreatObjectChain( xvalRet ) ;

	return	xvalRet ;
}

// コンパイル時の評価値情報をスタック上にもプッシュする
//////////////////////////////////////////////////////////////////////////////
void LCompiler::PushExprValueOnStack( LExprValuePtr xval )
{
	if( !MustBeRuntimeExpr() )
	{
		return ;
	}
	assert( m_ctx != nullptr ) ;
	m_ctx->m_xvaStack.push_back( xval ) ;
}

// コンパイル時の評価値情報をスタック上から削除する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::PopExprValueOnStacks( size_t nCount )
{
	if( !MustBeRuntimeExpr() )
	{
		return ;
	}
	assert( m_ctx != nullptr ) ;
	m_ctx->m_xvaStack.FreeStack( nCount ) ;
}

// 未使用の一時スタック数をカウントする
//////////////////////////////////////////////////////////////////////////////
size_t LCompiler::CountFreeTempStack( void )
{
	if( m_ctx == nullptr )
	{
		return	0 ;
	}
	return	m_ctx->m_xvaStack.CountBackTemporaries() ;
}

// 実行時式でスタックを解放する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::ExprCodeFreeStack( size_t nCount )
{
	if( !MustBeRuntimeExpr() )
	{
		return ;
	}
	if ( nCount == 0 )
	{
		return ;
	}
	if ( !m_ctx->m_codeBarrier
		&& (nCount == 1) && (m_ctx->m_codeBuf->GetCodeSize() > 0) )
	{
		const size_t	nCodeSize = m_ctx->m_codeBuf->GetCodeSize() ;
		const LCodeBuffer::Word&
				wordLast = m_ctx->m_codeBuf->GetCodeAt( nCodeSize - 1 ) ;

		if ( LCodeBuffer::IsLoadPushCodeWithoutEffects
				( (LCodeBuffer::InstructionCode) wordLast.code ) )
		{
			// 直前のロード命令を削除するして解放命令も出力しない
			DiscardCodeAfterPoint( nCodeSize - 1 ) ;
			PopExprValueOnStacks( nCount ) ;
			return ;
		}
	}
	if ( !m_ctx->m_codeBarrier
		&& (m_ctx->m_codeBuf->GetCodeSize() > 0) )
	{
		const size_t	nCodeSize = m_ctx->m_codeBuf->GetCodeSize() ;
		LCodeBuffer::Word&
				wordLast = m_ctx->m_codeBuf->CodeAt( nCodeSize - 1 ) ;
		if ( wordLast.code == LCodeBuffer::codeFreeStack )
		{
			// 直前も codeFreeStack なので解放する数を合計する
			wordLast.imm += (std::int32_t) nCount ;
			PopExprValueOnStacks( nCount ) ;
			return ;
		}
	}

	AddCode( LCodeBuffer::codeFreeStack, 0, 0, 0, nCount ) ;
	PopExprValueOnStacks( nCount ) ;
}

// 未使用の一時スタックを開放する（解放した数を返す）
//////////////////////////////////////////////////////////////////////////////
size_t LCompiler::ExprCodeFreeTempStack( void )
{
	const size_t	nFreeCount = CountFreeTempStack() ;
	if ( nFreeCount == 0 )
	{
		return	0 ;
	}
	ExprCodeFreeStack( nFreeCount ) ;
	return	nFreeCount ;
}

// 現在のコードポインタを取得する（ジャンプ先などのため）
// （※暗黙に ExprCodeFreeTempStack が呼び出される）
//////////////////////////////////////////////////////////////////////////////
LCompiler::CodePointPtr
	LCompiler::LCompiler::GetCurrentCodePointer( bool flagDestPos )
{
	ExprCodeFreeTempStack() ;

	CodePointPtr	cpPos = std::make_shared<CodePoint>( !flagDestPos ) ;
	if ( !MustBeRuntimeExpr() )
	{
		return	cpPos ;
	}
	assert( m_ctx != nullptr ) ;

	m_ctx->m_codeBarrier = true ;

	cpPos->m_src = m_iSrcStatement ;
	cpPos->m_pos = m_ctx->m_codeBuf->m_buffer.size() ;
	cpPos->m_nest = m_ctx->m_curNest ;
	cpPos->m_stack = m_ctx->m_xvaStack.size() ;

	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( pNest->m_frame != nullptr )
		{
			cpPos->m_locals.insert
				( cpPos->m_locals.begin(),
					std::make_shared<LLocalVarArray>( *(pNest->m_frame) ) ) ;
		}
		else
		{
			cpPos->m_locals.insert
				( cpPos->m_locals.begin(),
					std::make_shared<LLocalVarArray>() ) ;
		}
		pNest = pNest->m_prev ;
	}

	m_ctx->m_codePoints.push_back( cpPos ) ;
	return	cpPos ;
}

// ジャンプ命令（命令のコードポインタを返す）
// （※暗黙に ExprCodeFreeTempStack が呼び出される）
//////////////////////////////////////////////////////////////////////////////
LCompiler::CodePointPtr
	LCompiler::ExprCodeJump( LCompiler::CodePointPtr cpDest )
{
	if ( !MustBeRuntimeExpr() )
	{
		return	GetCurrentCodePointer( false ) ;
	}

	// 解放スタック計算
	size_t	nFreeCount = CountFreeTempStack() ;
	assert( nFreeCount == 0 ) ;
	PopExprValueOnStacks( nFreeCount ) ;

	CodePointPtr	cpPos = GetCurrentCodePointer( false ) ;
//	nFreeCount += CountFreeStackToJump( cpPos, cpDest ) ;
	// ※ nFreeCount は FixJumpDestination 内で加算される

	// ジャンプコード
	LCodeBuffer::ImmediateOperand	immop ;
	immop.value.longValue = 0 ;
	if ( cpDest != nullptr )
	{
		immop.value.longValue =
			(ssize_t) cpDest->m_pos
						- (m_ctx->m_codeBuf->m_buffer.size() + 1) ;
	}
	AddCode( LCodeBuffer::codeJump, 0, 0, 0, nFreeCount, &immop ) ;

	if ( cpDest != nullptr )
	{
		// ジャンプ位置情報登録
		FixJumpDestination( cpPos, cpDest ) ;
	}
	return	cpPos ;
}

LCompiler::CodePointPtr
	LCompiler::ExprCodeJumpIf
		( LExprValuePtr xvalCond, LCompiler::CodePointPtr cpDest )
{
	if ( xvalCond->IsConstExpr() && xvalCond->GetType().IsBoolean() )
	{
		if ( xvalCond->AsInteger() == 0 )
		{
			return	nullptr ;
		}
		return	ExprCodeJump( cpDest ) ;
	}
	return	ExprCodeJumpConditional
				( LCodeBuffer::codeJumpConditional,
							std::move(xvalCond), cpDest ) ;
}

LCompiler::CodePointPtr
	LCompiler::ExprCodeJumpNonIf
		( LExprValuePtr xvalCond, LCompiler::CodePointPtr cpDest )
{
	if ( xvalCond->IsConstExpr() && xvalCond->GetType().IsBoolean() )
	{
		if ( xvalCond->AsInteger() == 0 )
		{
			return	nullptr ;
		}
		return	ExprCodeJump( cpDest ) ;
	}
	return	ExprCodeJumpConditional
				( LCodeBuffer::codeJumpNonConditional,
							std::move(xvalCond), cpDest ) ;
}

LCompiler::CodePointPtr LCompiler::ExprCodeJumpConditional
	( LCodeBuffer::InstructionCode code,
		LExprValuePtr xvalCond, LCompiler::CodePointPtr cpDest )
{
	if ( !MustBeRuntimeExpr() )
	{
		return	GetCurrentCodePointer( false ) ;
	}
	xvalCond = ExprMakeOnStack( std::move(xvalCond) ) ;

	ssize_t	iStackCond = GetBackIndexOnStack( xvalCond ) ;
	assert( iStackCond >= 0 ) ;

	xvalCond = nullptr ;

	// 解放スタック計算
	size_t	nFreeCount = CountFreeTempStack() ;
	PopExprValueOnStacks( nFreeCount ) ;

	CodePointPtr	cpPos = GetCurrentCodePointer( false ) ;

//	nFreeCount += CountFreeStackToJump( cpPos, cpDest ) ;
	// ※ nFreeCount は FixJumpDestination 内で加算される

	// ジャンプコード
	LCodeBuffer::ImmediateOperand	immop ;
	immop.value.longValue = 0 ;
	if ( cpDest != nullptr )
	{
		immop.value.longValue = cpDest->m_pos ;
	}
	AddCode( code, iStackCond, 0, 0, nFreeCount, &immop ) ;

	if ( cpDest != nullptr )
	{
		// ジャンプ位置情報登録
		FixJumpDestination( cpPos, cpDest ) ;
	}
	return	cpPos ;
}

// 指定コードポインタにあるジャンプ命令のジャンプ先を確定する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::FixJumpDestination( CodePointPtr cpJumpCode, CodePointPtr cpDest )
{
	if ( !MustBeRuntimeExpr() )
	{
		return ;
	}
	if ( cpJumpCode == nullptr )
	{
		return ;
	}
	assert( cpDest != nullptr ) ;
	assert( m_ctx != nullptr ) ;
	for ( auto jp : m_ctx->m_jumpPoints )
	{
		if ( jp.m_cpJump == cpJumpCode )
		{
			if ( jp.m_cpDst != cpDest )
			{
				OnError( errorAssertJumpCode ) ;
			}
			else
			{
				OnWarning( errorAssertJumpCode, warning3 ) ;
			}
			return ;
		}
	}

	// ジャンプ命令が条件ジャンプか？
	bool	flagConditionalJump = false ;
	LCodeBuffer::Word *	pWord = GetCodeAt( cpJumpCode ) ;
	if ( pWord != nullptr )
	{
		if ( pWord->code == LCodeBuffer::codeJump )
		{
			flagConditionalJump = false ;
		}
		else if ( (pWord->code == LCodeBuffer::codeJumpConditional)
				|| (pWord->code == LCodeBuffer::codeJumpNonConditional) )
		{
			flagConditionalJump = true ;
		}
		else
		{
			OnError( errorAssertJumpCode,
						GetLabelNameOfCodePoint(cpDest).c_str() ) ;
			return ;
		}
	}
	else
	{
		return ;
	}

	// スタックとローカルフレームの整合性を確認し、解放するスタック数を取得
	size_t	nFreeStackSize = CountFreeStackToJump( cpJumpCode, cpDest ) ;
	if ( HasErrorOnCurrent() )
	{
		return ;
	}
	size_t	nDstLocalSize = CountStackOnfCodePoint( cpDest ) ;

	// try ブロックを超える場合には, codeCallFinally, codeLeaveTry 命令を挿入する
	size_t			nTryBlocks = 0 ;
	size_t			nLeaveFinallyFreeStack = 0 ;
	size_t			iLocalNest = cpJumpCode->m_locals.size() - 1 ;
	CodeNestPtr		pNest = cpJumpCode->m_nest ;
	while ( (pNest != nullptr) && (pNest != cpDest->m_nest) )
	{
		nLeaveFinallyFreeStack +=
			cpJumpCode->m_locals.at(iLocalNest)->GetUsedSize() ;
		//
		if ( (pNest->m_type == CodeNest::ctrlTry)
			|| (pNest->m_type == CodeNest::ctrlCatch) )
		{
			if ( (pNest->m_type == CodeNest::ctrlCatch)
					&& (pNest->m_prev == cpDest->m_nest) )
			{
				pNest = pNest->m_prev ;
				break ;
			}
			InsertCode( cpJumpCode->m_pos,
							LCodeBuffer::codeCallFinally ) ;
			if ( pNest->m_type == CodeNest::ctrlCatch )
			{
				pNest = pNest->m_prev ;
				assert( pNest->m_type == CodeNest::ctrlTry ) ;
			}
			nTryBlocks ++ ;
			nLeaveFinallyFreeStack = 0 ;
		}
		else if ( pNest->m_type == CodeNest::ctrlFinally )
		{
			if ( nLeaveFinallyFreeStack > 0 )
			{
				InsertCode( cpJumpCode->m_pos,
							LCodeBuffer::codeFreeStack,
								0, 0, 0, nLeaveFinallyFreeStack ) ;
				nLeaveFinallyFreeStack = 0 ;
			}
			InsertCode( cpJumpCode->m_pos,
							LCodeBuffer::codeLeaveFinally ) ;
			pNest = pNest->m_prev ;
			assert( pNest->m_type == CodeNest::ctrlTry ) ;
			nTryBlocks ++ ;
		}
		pNest = pNest->m_prev ;
		assert( (pNest == nullptr) || (iLocalNest != 0) ) ;
		iLocalNest -- ;
	}
	if ( pNest != cpDest->m_nest )
	{
		OnError( errorUnavailableJumpBlock,
					GetLabelNameOfCodePoint(cpDest).c_str() ) ;
		return ;
	}
	if ( nTryBlocks > 0 )
	{
		if ( flagConditionalJump )
		{
			OnError( errorUnavailableJumpBlock,
						GetLabelNameOfCodePoint(cpDest).c_str() ) ;
		}

		// try ブロックを抜けた後は sp を調整する
		// （基本的には以前の sp に戻っているはずではあるが）
		LCodeBuffer::ImmediateOperand	immop ;
		immop.value.longValue = nDstLocalSize ;

		InsertCode( cpJumpCode->m_pos,
					LCodeBuffer::codeFreeStackLocal, 0, 0, 0, 0, &immop ) ;

		pWord = GetCodeAt( cpJumpCode ) ;
		if ( pWord != nullptr )
		{
			if ( !flagConditionalJump )
			{
				pWord->imm = 0 ;
			}
			nFreeStackSize = 0 ;
		}
	}

	pWord = GetCodeAt( cpJumpCode ) ;
	if ( pWord != nullptr )
	{
		// ジャンプ先確定
		if ( cpDest->m_pos != SIZE_MAX )
		{
			pWord->imop.value.longValue = cpDest->m_pos ;
		}

		// 解放フレーム加算
		if ( nTryBlocks == 0 )
		{
			if ( !flagConditionalJump )
			{
				pWord->imm += (std::int32_t) nFreeStackSize ;
			}
			else
			{
				if ( nFreeStackSize >= 0x10000 )
				{
					OnError( errorExpressionTooComplex ) ;
				}
				pWord->sop2 = (std::uint8_t) (nFreeStackSize & 0xFF) ;
				pWord->op3 = (std::uint8_t) ((nFreeStackSize >> 8) & 0xFF) ;
			}
		}
	}

	// ジャンプ位置情報登録
	m_ctx->m_jumpPoints.push_back( JumpPoint( cpJumpCode, cpDest ) ) ;
}

// ジャンプ先へのスタック解放数を計算する
// （ジャンプできない場合はエラーを発生する）
//////////////////////////////////////////////////////////////////////////////
size_t LCompiler::CountFreeStackToJump
	( CodePointPtr cpJumpCode, CodePointPtr cpDest )
{
	if ( cpDest == nullptr )
	{
		return	0 ;
	}
	// スタックの整合性確認（式の中でのジャンプの整合性の確認）
	if ( (cpJumpCode->m_stack != cpDest->m_stack)
							&& (cpDest->m_stack != 0) )
	{
		OnError( errorAssertMismatchStack,
					GetLabelNameOfCodePoint(cpDest).c_str() ) ;
		return	0 ;
	}

	// ローカルフレームの整合性確認
	const size_t	minLocals = cpDest->m_locals.size() ;
	const size_t	maxLocals = cpJumpCode->m_locals.size() ;
	if ( minLocals > maxLocals )
	{
		// 同じスコープか、脱出方向にしかジャンプできない
		OnError( errorMismatchLocalFrameToJump,
					GetLabelNameOfCodePoint(cpDest).c_str() ) ;
		return	0 ;
	}
	for ( size_t i = 0; i < minLocals; i ++ )
	{
		if ( !cpDest->m_locals.at(i)->DoesMatchWith
				( *(cpJumpCode->m_locals.at(i)), (i + 1 < minLocals) ) )
		{
			OnError( errorMismatchLocalFrameToJump,
						GetLabelNameOfCodePoint(cpDest).c_str() ) ;
			return	0 ;
		}
	}

	assert( CountStackOnfCodePoint(cpDest) <= CountStackOnfCodePoint(cpJumpCode) ) ;
	return	CountStackOnfCodePoint(cpJumpCode)
					- CountStackOnfCodePoint(cpDest) ;
}

// スタック総数をカウントする
//////////////////////////////////////////////////////////////////////////////
size_t LCompiler::CountStackOnfCodePoint( CodePointPtr cp ) const
{
	if ( cp == nullptr )
	{
		return	0 ;
	}
	size_t	nCount = cp->m_stack ;

	for ( size_t i = 0; i < cp->m_locals.size(); i ++ )
	{
		nCount += cp->m_locals.at(i)->GetUsedSize() ;
	}
	return	nCount ;
}

// 指定のコードポインタ以降のコード出力をすべて破棄する
//////////////////////////////////////////////////////////////////////////////
void LCompiler::DiscardCodeAfterPoint( CodePointPtr cpSave )
{
	assert( cpSave != nullptr ) ;
	DiscardCodeAfterPoint( cpSave->m_pos ) ;
}

void LCompiler::DiscardCodeAfterPoint( size_t iCodePos )
{
	if ( (m_ctx == nullptr) || (m_ctx->m_codeBuf == nullptr) )
	{
		return ;
	}
	for ( size_t i = 0; i < m_ctx->m_codePoints.size(); i ++ )
	{
		CodePointPtr	cpp = m_ctx->m_codePoints.at(i) ;
		if ( (cpp->m_pos > iCodePos)
			|| (cpp->m_bind && (cpp->m_pos == iCodePos)) )
		{
			cpp->m_pos = SIZE_MAX ;
		}
	}
	m_ctx->m_codeBuf->m_buffer.resize( iCodePos ) ;
}

// 実行時式を要求（定数式で無ければならない場合エラーを出力し false）
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::MustBeRuntimeExpr( void )
{
	if ( (m_ctx == nullptr) || (m_ctx->m_codeBuf == nullptr) )
	{
		OnError( errorUnavailableConstExpr ) ;
		return	false ;
	}
	return	true ;
}

// ジャンプ先のラベル名を取得する
//////////////////////////////////////////////////////////////////////////////
LString LCompiler::GetLabelNameOfCodePoint( CodePointPtr cpDest ) const
{
	if ( m_ctx == nullptr )
	{
		return	LString() ;
	}
	for ( auto iter : m_ctx->m_mapLabels )
	{
		if ( iter.second.m_cpLabel == cpDest )
		{
			return	LString( iter.first ) ;
		}
	}
	return	LString() ;
}

// 命令出力
void LCompiler::AddCode
	( LCodeBuffer::InstructionCode code,
		size_t sop1, size_t sop2, size_t op3, ssize_t imm32,
					const LCodeBuffer::ImmediateOperand * imm64 )
{
	if ( !MustBeRuntimeExpr() )
	{
		return ;
	}
	if ( (sop1 >= 0x100) || (sop2 >= 0x100) || (op3 >= 0x100) )
	{
		OnError( errorExpressionTooComplex ) ;
		return ;
	}
	LCodeBuffer::Word	word ;
	word.code = code ;
	word.sop1 = (std::uint8_t) sop1 ;
	word.sop2 = (std::uint8_t) sop2 ;
	word.op3 = (std::uint8_t) op3 ;
	word.imm = (std::int32_t) imm32 ;
	word.imop.value.longValue = 0 ;
	if ( imm64 != nullptr )
	{
		word.imop = *imm64 ;
	}
	m_ctx->m_codeBuf->AddCode( word ) ;

	m_ctx->m_codeBarrier = false ;
}

void LCompiler::InsertCode
	( size_t iCodePoint,
		LCodeBuffer::InstructionCode code,
		size_t sop1, size_t sop2, size_t op3, ssize_t imm32,
					const LCodeBuffer::ImmediateOperand * imm64 )
{
	if ( !MustBeRuntimeExpr() )
	{
		return ;
	}
	if ( iCodePoint > m_ctx->m_codeBuf->m_buffer.size() )
	{
		assert( iCodePoint == SIZE_MAX ) ;
		return ;
	}
	if ( (sop1 >= 0x100) || (sop2 >= 0x100) || (op3 >= 0x100) )
	{
		OnError( errorExpressionTooComplex ) ;
		return ;
	}

	// コード位置修正
	bool	flagPointMoved = false ;
	for ( size_t i = 0; i < m_ctx->m_codePoints.size(); i ++ )
	{
		CodePointPtr	cpp = m_ctx->m_codePoints.at(i) ;
		if ( cpp->m_pos != SIZE_MAX )
		{
			if ( (cpp->m_pos > iCodePoint)
				|| (cpp->m_bind && (cpp->m_pos == iCodePoint)) )
			{
				cpp->m_pos += 1 ;
				flagPointMoved = true ;
			}
		}
	}

	// コード挿入
	LCodeBuffer::Word	word ;
	word.code = code ;
	word.sop1 = (std::uint8_t) sop1 ;
	word.sop2 = (std::uint8_t) sop2 ;
	word.op3 = (std::uint8_t) op3 ;
	word.imm = (std::int32_t) imm32 ;
	word.imop.value.longValue = 0 ;
	if ( imm64 != nullptr )
	{
		word.imop = *imm64 ;
	}
	m_ctx->m_codeBuf->InsertCode( iCodePoint, word ) ;

	// ジャンプ位置修正
	if ( flagPointMoved )
	{
		UpdateCodeJumpPoints() ;
	}
}

// 指定コードポインタにあるコードを取得する
//////////////////////////////////////////////////////////////////////////////
LCodeBuffer::Word * LCompiler::GetCodeAt( CodePointPtr cpCode )
{
	if ( (m_ctx != nullptr)
		&& (m_ctx->m_codeBuf != nullptr)
		&& (cpCode != nullptr)
		&& (cpCode->m_pos < m_ctx->m_codeBuf->m_buffer.size()) )
	{
		return	&(m_ctx->m_codeBuf->CodeAt(cpCode->m_pos)) ;
	}
	return	nullptr ;
}

// ジャンプ命令のジャンプ先の更新
//////////////////////////////////////////////////////////////////////////////
void LCompiler::UpdateCodeJumpPoints( void )
{
	if ( (m_ctx == nullptr) || (m_ctx->m_codeBuf == nullptr) )
	{
		return ;
	}
	for ( auto jp : m_ctx->m_jumpPoints )
	{
		if ( (jp.m_cpDst->m_pos == SIZE_MAX)
			|| (jp.m_cpJump->m_pos == SIZE_MAX) )
		{
			continue ;
		}
		assert( jp.m_cpDst->m_pos <= m_ctx->m_codeBuf->m_buffer.size() ) ;
		assert( jp.m_cpJump->m_pos <= m_ctx->m_codeBuf->m_buffer.size() ) ;
		if ( (jp.m_cpDst->m_pos > m_ctx->m_codeBuf->m_buffer.size())
			|| (jp.m_cpJump->m_pos > m_ctx->m_codeBuf->m_buffer.size()) )
		{
			continue ;
		}
		LCodeBuffer::Word&	word =
				m_ctx->m_codeBuf->CodeAt(jp.m_cpJump->m_pos) ;
		assert( (word.code == LCodeBuffer::codeJump)
			|| (word.code == LCodeBuffer::codeJumpConditional)
			|| (word.code == LCodeBuffer::codeJumpNonConditional)
			|| (word.code == LCodeBuffer::codeLoadImm) ) ;
		if ( (word.code == LCodeBuffer::codeJump)
			|| (word.code == LCodeBuffer::codeJumpConditional)
			|| (word.code == LCodeBuffer::codeJumpNonConditional)
			|| (word.code == LCodeBuffer::codeLoadImm) )
		{
			word.imop.value.longValue = jp.m_cpDst->m_pos ;
		}
	}
}

// 実行時式でスタック上に確保されているか？
//////////////////////////////////////////////////////////////////////////////
bool LCompiler::IsExprValueOnStack( LExprValuePtr xval ) const
{
	if ( m_ctx != nullptr )
	{
		return	(m_ctx->m_xvaStack.Find( xval ) >= 0) ;
	}
	return	false ;
}

// 実行時式でスタック上の指標（後ろから見た）を取得
//////////////////////////////////////////////////////////////////////////////
ssize_t LCompiler::GetBackIndexOnStack( LExprValuePtr xval ) const
{
	if ( m_ctx != nullptr )
	{
		return	m_ctx->m_xvaStack.FindBack( xval ) ;
	}
	return	-1 ;
}

// ローカルフレーム上での指標を取得
//////////////////////////////////////////////////////////////////////////////
ssize_t LCompiler::GetLocalVarIndex( LExprValuePtr xval ) const
{
	if ( m_ctx == nullptr )
	{
		return	-1 ;
	}
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( pNest->m_frame != nullptr )
		{
			ssize_t	iLoc = pNest->m_frame->FindLocalVar( xval ) ;
			if ( iLoc >= 0 )
			{
				return	iLoc ;
			}
		}
		pNest = pNest->m_prev ;
	}
	return	-1 ;
}

// ローカルフレーム上のエントリ取得
//////////////////////////////////////////////////////////////////////////////
LLocalVarPtr LCompiler::GetLocalVarAt( size_t iLocal ) const
{
	if ( m_ctx == nullptr )
	{
		return	nullptr ;
	}
	CodeNestPtr	pNest = m_ctx->m_curNest ;
	while ( pNest != nullptr )
	{
		if ( pNest->m_frame != nullptr )
		{
			LLocalVarPtr	pVar = pNest->m_frame->GetLocalVarAt(iLocal) ;
			if ( pVar != nullptr )
			{
				return	pVar ;
			}
		}
		pNest = pNest->m_prev ;
	}
	return	nullptr ;
}

// fetch_addr なローカル変数なら参照領域を更新
//////////////////////////////////////////////////////////////////////////////
void LCompiler::HasFetchPointerRange( LExprValuePtr xval, size_t nRange )
{
	if ( xval->IsFetchAddrPointer() && xval->IsOnLocal() )
	{
		ssize_t	iVar = GetLocalVarIndex( xval ) ;
		assert( iVar >= 0 ) ;

		LLocalVarPtr	pVar = GetLocalVarAt( (size_t) iVar ) ;
		assert( pVar != nullptr ) ;

		pVar->HasFetchPointerRange( nRange ) ;
	}
}


