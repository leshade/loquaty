
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// デバッガ
//////////////////////////////////////////////////////////////////////////////

// トレース実行クリア設定
void LDebugger::ClearStepTrace( LContext& context )
{
	InstancePtr	instance = context.GetDebuggerInstance() ;
	if ( instance != nullptr )
	{
		instance->m_mode = modeNormal ;
	}
}

// ステップ実行設定
void LDebugger::SetStepIn( LContext& context )
{
	InstancePtr	instance = context.GetDebuggerInstance() ;
	if ( instance == nullptr )
	{
		instance = CreateInstance() ;
		context.SetDebuggerInstance( instance ) ;
	}
	instance->m_mode = modeStepIn ;
	instance->m_bpCurStep.m_pCodeBuf = context.GetCodeBuffer() ;
	instance->m_bpCurStep.m_ipFirst = context.GetIP() ;
	instance->m_bpCurStep.m_ipEnd = context.GetIP() ;
}

void LDebugger::SetStepIn
	( LContext& context,
		size_t ipCurStepFirst, size_t ipCurStepEnd )
{
	InstancePtr	instance = context.GetDebuggerInstance() ;
	if ( instance == nullptr )
	{
		instance = CreateInstance() ;
		context.SetDebuggerInstance( instance ) ;
	}
	instance->m_mode = modeStepIn ;
	instance->m_bpCurStep.m_pCodeBuf = context.GetCodeBuffer() ;
	instance->m_bpCurStep.m_ipFirst = (ssize_t) ipCurStepFirst ;
	instance->m_bpCurStep.m_ipEnd = (ssize_t) ipCurStepEnd ;
}

// ステップオーバー設定
void LDebugger::SetStepOver
	( LContext& context,
		size_t ipCurStepFirst, size_t ipCurStepEnd,
		size_t ipNextStepFirst, size_t ipNextStepEnd )
{
	InstancePtr	instance = context.GetDebuggerInstance() ;
	if ( instance == nullptr )
	{
		instance = CreateInstance() ;
		context.SetDebuggerInstance( instance ) ;
	}
	instance->m_bpCurStep.m_pCodeBuf = context.GetCodeBuffer() ;
	instance->m_bpCurStep.m_ipFirst = (ssize_t) ipCurStepFirst ;
	instance->m_bpCurStep.m_ipEnd = (ssize_t) ipCurStepEnd ;
	instance->m_bpStepOver.m_pCodeBuf = context.GetCodeBuffer() ;
	instance->m_bpStepOver.m_ipFirst = (ssize_t) ipNextStepFirst ;
	instance->m_bpStepOver.m_ipEnd = (ssize_t) ipNextStepEnd ;
	instance->m_mode = modeStepOver ;
}

// コンテキスト・インスタンス生成
LDebugger::InstancePtr LDebugger::CreateInstance( void )
{
	return	std::make_shared<Instance>() ;
}

// コード実行前
void LDebugger::OnCodeInstruction
	( LContext& context,
		const LCodeBuffer * code, size_t ip,
		const LCodeBuffer::Word& word )
{
	InstancePtr	instance = context.GetDebuggerInstance() ;
	if ( instance != nullptr )
	{
		if ( instance->m_mode == modeStepOutStop )
		{
			if ( instance->m_bpCurStep.IsInBounds( code, ip ) )
			{
				// ジャンプアウトしていないのでステップオーバー継続
				instance->m_mode = modeStepOver ;
			}
			else
			{
				// ジャンプアウトしていた場合には停止
				auto	lock = LockDebugBreak() ;
				SetStepIn( context ) ;
				OnBreakPoint( context, reasonStep ) ;
				return ;
			}
		}
		if ( instance->m_mode == modeStepIn )
		{
			if ( !instance->m_bpCurStep.IsInBounds( code, ip ) )
			{
				// 命令単位でのステップ実行なので停止
				auto	lock = LockDebugBreak() ;
				OnBreakPoint( context, reasonStep ) ;
				return ;
			}
		}
		else if ( instance->m_mode == modeStepOver )
		{
			if ( instance->m_bpCurStep.IsInBounds( code, ip ) )
			{
				// ジャンプアウト命令の判定
				switch ( word.code )
				{
				case	LCodeBuffer::codeCallFinally:
				case	LCodeBuffer::codeRetFinally:
				case	LCodeBuffer::codeThrow:
				case	LCodeBuffer::codeReturn:
				case	LCodeBuffer::codeJump:
				case	LCodeBuffer::codeJumpConditional:
				case	LCodeBuffer::codeJumpNonConditional:
					instance->m_mode = modeStepOutStop ;
					break ;
				default:
					break ;
				}
			}
			else if ( instance->m_bpStepOver.IsInBounds( code, ip ) )
			{
				// 次の行まで実行したので停止
				auto	lock = LockDebugBreak() ;
				SetStepIn( context ) ;
				OnBreakPoint( context, reasonStep ) ;
				return ;
			}
		}
	}

	// ブレークポイント
	for ( auto iter = m_vBreakPoints.begin();
				iter != m_vBreakPoints.end(); iter ++ )
	{
		if ( (*iter)->IsInBounds( code, ip )
			&& (*iter)->OnBreakPoint( context ) )
		{
			auto	lock = LockDebugBreak() ;
			OnBreakPoint( context, reasonBreakPoint ) ;
			return ;
		}
	}
}

// デバッグ用出力
void LDebugger::OutputTrace( const wchar_t * pwszMessage )
{
	std::string	strMessage = LString(pwszMessage).ToString() ;
	LTrace( strMessage.c_str() ) ;
}

// 例外発生時
LObjPtr LDebugger::OnThrowException( LContext& context, LObjPtr pException )
{
	return	pException ;
}

// ブレークポイント
void LDebugger::OnBreakPoint( LContext& context, LDebugger::BreakReason reason )
{
}

// ブレークポイント追加
void LDebugger::AddBreakPoint( BreakPointPtr bp )
{
	m_vBreakPoints.push_back( bp ) ;
}

// ブレークポイント削除
void LDebugger::RemoveBreakPoint( BreakPointPtr bp )
{
	for ( auto iter = m_vBreakPoints.begin();
				iter != m_vBreakPoints.end(); iter ++ )
	{
		if ( *iter == bp )
		{
			m_vBreakPoints.erase( iter ) ;
			break ;
		}
	}
}

void LDebugger::RemoveAllBreakPoints( void )
{
	m_vBreakPoints.clear() ;
}

// 評価値文字列化
LString LDebugger::ToExpression( const LValue& value )
{
	if ( value.GetType().IsPrimitive() )
	{
		LString	strExpr ;
		if ( value.GetType().IsBoolean() )
		{
			strExpr = (value.Value().boolValue ? L"true" : L"false") ;
		}
		else if ( value.GetType().IsInteger() )
		{
			LPointerObj::ExprIntAsString
					( strExpr, value.Value().longValue ) ;
		}
		else
		{
			strExpr.SetNumberOf( value.Value().dblValue ) ;
		}
		return	strExpr ;
	}
	else
	{
		LPointerObj *	pPtr =
				dynamic_cast<LPointerObj*>( value.GetObject().Ptr() ) ;
		if ( (pPtr != nullptr)
			&& (pPtr->GetClass() != value.GetType().GetClass()) )
		{
			LPtr<LPointerObj>	pTempPtr =
						new LPointerObj( value.GetType().GetClass() ) ;
			*pTempPtr = *pPtr ;

			LString	strExpr ;
			if ( pTempPtr->AsExpression( strExpr ) )
			{
				return	strExpr ;
			}
		}
		return	LObject::ToExpression( value.GetObject().Ptr() ) ;
	}
}

// デバッグ時式を評価
LValue LDebugger::EvaluateExpr
	( LContext& context, LStringParser& sparsExpr,
		const wchar_t * pwszEscChars, int priority )
{
	LValue	val ;
	if ( !sparsExpr.PassSpace() )
	{
		return	val ;
	}
	do
	{
		// 脱出判定
		wchar_t	wch = sparsExpr.CurrentChar() ;
		if ( LStringParser::IsCharContained( wch, pwszEscChars ) )
		{
			return	val ;
		}

		// リテラル判定
		if ( sparsExpr.IsCharNumber( wch ) )
		{
			// 数値リテラル
			LExprValuePtr	xval = LCompiler::ParseNumberLiteral( sparsExpr ) ;
			val = *xval ;
			break ;
		}
		else if ( (wch == L'\"') || (wch == L'\'') )
		{
			// 文字列リテラル
			LString	strLiteral ;
			sparsExpr.NextChar() ;
			if ( !sparsExpr.NextEnclosedString( strLiteral, wch ) )
			{
				// エラー
				return	val ;
			}
			strLiteral = LStringParser::DecodeStringLiteral
								( strLiteral.c_str(), strLiteral.GetLength() ) ;
			if ( wch == L'\'' )
			{
				val = LValue( LType::typeUint32,
								LValue::MakeUint64( strLiteral.GetAt(0) ) ) ;
			}
			else
			{
				val = LValue( context.new_String( strLiteral ) ) ;
			}
			break ;
		}

		LString	strToken ;
		sparsExpr.NextToken( strToken ) ;

		Symbol::OperatorIndex
				opIndex = AsOperatorIndex( strToken.c_str() ) ;
		if ( opIndex != Symbol::opInvalid )
		{
			if ( opIndex == Symbol::opParenthesis )
			{
				// () 括弧
				val = EvaluateExpr( context, sparsExpr, L")" ) ;
				sparsExpr.HasNextChars( L")" ) ;
				break ;
			}
			else if ( Symbol::s_OperatorDescs[opIndex].
								priorityUnary != Symbol::priorityNo )
			{
				// 前置単項演算子
				val = EvaluateExpr
					( context, sparsExpr, pwszEscChars,
						Symbol::s_OperatorDescs[opIndex].priorityUnary ) ;
				val = EvaluateUnaryOperator( val, opIndex ) ;
				break ;
			}
			else
			{
				// エラー
				return	LValue() ;
			}
		}

		// 予約語
		if ( strToken == L"true" )
		{
			val = LValue( LType::typeBoolean, LValue::MakeBool( true ) ) ;
			break ;
		}
		else if ( strToken == L"false" )
		{
			val = LValue( LType::typeBoolean, LValue::MakeBool( false ) ) ;
			break ;
		}
		else if ( strToken == L"global" )
		{
			val = LValue( context.VM().Global() ) ;
			break ;
		}

		// ローカル変数
		val = GetLocalVariableAs( context, strToken.c_str() ) ;
		if ( !val.IsVoid() )
		{
			break ;
		}

		// this メンバ
		LValue	valThis = GetLocalVariableAs( context, L"this" ) ;
		if ( !valThis.IsVoid() )
		{
			val = GetMemberVariableAs( context, valThis, strToken.c_str() ) ;
			if ( !val.IsVoid() )
			{
				break ;
			}
		}

		// global 名前空間
		val = GetMemberVariableAs
				( context, LValue( context.VM().Global() ), strToken.c_str() ) ;
	}
	while ( false ) ;

	while ( sparsExpr.PassSpace() )
	{
		if ( val.IsVoid() || val.IsNull() )
		{
			// エラー
			return	val ;
		}

		// 脱出判定
		if ( LStringParser::IsCharContained( sparsExpr.CurrentChar(), pwszEscChars ) )
		{
			return	val ;
		}

		const size_t	iTempPos = sparsExpr.GetIndex() ;
		LString			strToken ;
		sparsExpr.NextToken( strToken ) ;

		Symbol::OperatorIndex
				opIndex = AsOperatorIndex( strToken.c_str() ) ;
		if ( (opIndex == Symbol::opInvalid)
			|| (Symbol::s_OperatorDescs[opIndex].
								priorityBinary == Symbol::priorityNo) )
		{
			// エラー
			return	LValue() ;
		}
		if ( Symbol::s_OperatorDescs[opIndex].priorityBinary <= priority )
		{
			sparsExpr.SeekIndex( iTempPos ) ;
			break ;
		}
		if ( opIndex == Symbol::opBracket )
		{
			// expr[index] 演算子
			LValue	valIndex = EvaluateExpr( context, sparsExpr, L"]" ) ;
			sparsExpr.HasNextChars( L"]" ) ;
			val = EvaluateElementAt( context, val, valIndex ) ;
		}
		else if ( opIndex == Symbol::opMemberOf )
		{
			// expr.member 演算子
			LString	strMember ;
			sparsExpr.NextToken( strMember ) ;
			val = GetMemberVariableAs( context, val, strMember.c_str() ) ;
		}
		else
		{
			// 二項演算子
			LValue	valRight =
				EvaluateExpr
					( context, sparsExpr, pwszEscChars,
						Symbol::s_OperatorDescs[opIndex].priorityBinary ) ;
			val = EvaluateBinaryOperator( val, valRight, opIndex ) ;
		}
	}
	return	val ;
}

// デバッグ時ローカル変数
LValue LDebugger::GetLocalVariableAs
	( LContext& context, const wchar_t * pwszName )
{
	const LCodeBuffer *	pCodeBuf = context.GetCodeBuffer() ;
	if ( pCodeBuf == nullptr )
	{
		return	LValue() ;
	}
	std::vector<LCodeBuffer::DebugLocalVarInfo>	dbgVarInfos ;
	pCodeBuf->GetDebugLocalVarInfos( dbgVarInfos, context.GetIP() ) ;

	for ( LCodeBuffer::DebugLocalVarInfo& dlvi : dbgVarInfos )
	{
		LLocalVarPtr	pVar = dlvi.m_varInfo->GetLocalVarAs( pwszName ) ;
		if ( pVar != nullptr )
		{
			return	GetLocalVar( context, pVar, pVar->GetLocation() ) ;
		}
	}
	return	LValue() ;
}

// デバッグ時メンバ変数
LValue LDebugger::GetMemberVariableAs
	( LContext& context,
		const LValue& valObj, const wchar_t * pwszName )
{
	if ( valObj.GetType().IsPointer() )
	{
		// ポインタ
		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( valObj.GetObject().Ptr() ) ;
		if ( pPtrObj == nullptr )
		{
			return	LValue() ;
		}
		LPointerClass *	pPtrClass = valObj.GetType().GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		const LType&	typeBuf = pPtrClass->GetBufferType() ;
		if ( typeBuf.IsStructure() )
		{
			// 構造体ポインタのメンバ
			LStructureClass *	pStructClass = typeBuf.GetStructureClass() ;
			assert( pStructClass != nullptr ) ;

			const LArrangementBuffer&
					arrangeBuf = pStructClass->GetProtoArrangemenet() ;
			LArrangement::Desc	desc ;
			if ( arrangeBuf.GetDescAs( desc, pwszName ) )
			{
				return	GetDataMember( context, pPtrObj, desc ) ;
			}
		}
	}
	else if ( valObj.GetType().IsObject() )
	{
		// オブジェクト
		if ( valObj.GetObject() == nullptr )
		{
			return	LValue() ;
		}
		ssize_t	iElement = valObj.GetObject()->FindElementAs( pwszName ) ;
		if ( iElement >= 0 )
		{
			LType	typeElement = valObj.GetObject()->GetElementTypeAt( (size_t) iElement ) ;
			LObjPtr	pElement = valObj.GetObject()->GetElementAt( (size_t) iElement ) ;
			if ( (pElement != nullptr)
				&& typeElement.IsPointer() )
			{
				LPointerObj *	pPtrObj =
						dynamic_cast<LPointerObj*>( pElement.Ptr() ) ;
				if ( pPtrObj != nullptr )
				{
					LPtr<LPointerObj>	pPtrMember =
							new LPointerObj( typeElement.GetClass() ) ;
					*pPtrMember = *pPtrObj ;
					return	LValue( pPtrMember ) ;
				}
			}
			return	LValue( typeElement, pElement ) ;
		}
		const LArrangementBuffer&
				arrangeBuf = valObj.GetType().GetClass()->GetProtoArrangemenet() ;
		LArrangement::Desc	desc ;
		if ( arrangeBuf.GetDescAs( desc, pwszName ) )
		{
			// クラスメンバ変数へのポインタ
			LPtr<LPointerObj>	pPtrObj = valObj.GetObject()->GetBufferPoiner() ;
			if ( pPtrObj != nullptr )
			{
				return	GetDataMember( context, pPtrObj.Ptr(), desc ) ;
			}
		}
	}
	return	LValue() ;		// エラー
}

// デバッグ時データ変数
LValue LDebugger::GetDataMember
	( LContext& context,
		LPointerObj * pPtrObj, const LArrangement::Desc& desc )
{
	LPtr<LPointerObj>	pPtrElement =
		new LPointerObj( context.VM().GetPointerClassAs( desc.m_type ) ) ;
	*pPtrElement = *pPtrObj ;
	*pPtrElement += (ssize_t) desc.m_location ;
	return	EvaluateData( LValue( pPtrElement ) ) ;
}

// デバッグ時データ型変数評価
LValue LDebugger::EvaluateData( const LValue& val )
{
	if ( val.GetType().IsPointer() )
	{
		LPointerClass *	pPtrClass = val.GetType().GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( val.GetObject().Ptr() ) ;

		LType	typeBuf = pPtrClass->GetBufferType() ;
		if ( typeBuf.IsPrimitive() && (pPtrObj != nullptr) )
		{
			// プリミティブ型
			std::uint8_t *	pBuf =
					pPtrObj->GetPointer( 0, typeBuf.GetDataBytes() ) ;
			if ( pBuf != nullptr )
			{
				LType::Primitive	type = typeBuf.GetPrimitive() ;
				if ( typeBuf.IsFloatingPointNumber() )
				{
					return	LValue( type, LValue::MakeDouble
								( (LPointerObj::s_pnfLoadAsDouble[type])( pBuf ) ) ) ;
				}
				else
				{
					return	LValue( type, LValue::MakeLong
								( (LPointerObj::s_pnfLoadAsLong[type])( pBuf ) ) ) ;
				}
			}
		}
	}
	else if ( val.GetType().IsObject() )
	{
		LIntegerObj *	pIntObj =
			dynamic_cast<LIntegerObj*>( val.GetObject().Ptr() ) ;
		if ( pIntObj != nullptr )
		{
			return	LValue( LType::typeInt64,
							LValue::MakeLong( pIntObj->m_value ) ) ;
		}
		LDoubleObj *	pFloatObj =
			dynamic_cast<LDoubleObj*>( val.GetObject().Ptr() ) ;
		if ( pFloatObj != nullptr )
		{
			return	LValue( LType::typeDouble,
							LValue::MakeDouble( pFloatObj->m_value ) ) ;
		}
	}
	return	val ;
}

// 演算子判定
Symbol::OperatorIndex
	LDebugger::AsOperatorIndex( const LString& strToken )
{
	for ( int i = 0; i < Symbol::opOperatorCount; i ++ )
	{
		if ( strToken == Symbol::s_OperatorDescs[i].pwszName )
		{
			return	Symbol::s_OperatorDescs[i].opIndex ;
		}
	}
	return	Symbol::opInvalid ;
}

// デバッグ時単項演算子
LValue LDebugger::EvaluateUnaryOperator
		( const LValue& value, Symbol::OperatorIndex opIndex )
{
	if ( value.GetType().IsPrimitive() )
	{
		if ( value.GetType().IsBoolean() )
		{
			switch ( opIndex )
			{
			case	Symbol::opLogicalNot:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( !value.Value().boolValue ) ) ;
			default:
				break ;
			}
		}
		else if ( value.GetType().IsInteger() )
		{
			switch ( opIndex )
			{
			case	Symbol::opAdd:
				return	value ;
			case	Symbol::opSub:
				return	LValue( value.GetType().GetPrimitive(),
								LValue::MakeLong( - value.Value().longValue ) ) ;
			case	Symbol::opBitNot:
				return	LValue( value.GetType().GetPrimitive(),
								LValue::MakeLong( ~ value.Value().longValue ) ) ;
			case	Symbol::opLogicalNot:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( !value.Value().boolValue ) ) ;
			default:
				break ;
			}
		}
		else
		{
			switch ( opIndex )
			{
			case	Symbol::opAdd:
				return	value ;
			case	Symbol::opSub:
				return	LValue( value.GetType().GetPrimitive(),
								LValue::MakeDouble( - value.Value().dblValue ) ) ;
			default:
				break ;
			}
		}
	}
	else if ( value.GetType().IsPointer() )
	{
		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( value.GetObject().Ptr() ) ;
		switch ( opIndex )
		{
		case	Symbol::opMul:
			return	EvaluateData( value ) ;
		case	Symbol::opLogicalNot:
			return	LValue( LType::typeBoolean,
					LValue::MakeBool( !((pPtrObj != nullptr)
									&& (pPtrObj->GetPointer() != nullptr)) ) ) ;
		default:
			break ;
		}
	}
	else
	{
		switch ( opIndex )
		{
		case	Symbol::opLogicalNot:
			return	LValue( LType::typeBoolean,
						LValue::MakeBool( value.GetObject() == nullptr ) ) ;
		default:
			break ;
		}
	}
	return	LValue() ;		// エラー
}

// デバッグ時二項演算子
LValue LDebugger::EvaluateBinaryOperator
		( const LValue& valLeft,
			const LValue& valRight, Symbol::OperatorIndex opIndex )
{
	switch ( opIndex )
	{
	case	Symbol::opLogicalAnd:
		return	LValue( LType::typeBoolean,
					LValue::MakeBool( valLeft.AsBoolean() && valRight.AsBoolean() ) ) ;
	case	Symbol::opLogicalOr:
		return	LValue( LType::typeBoolean,
					LValue::MakeBool( valLeft.AsBoolean() || valRight.AsBoolean() ) ) ;
	default:
		break ;
	}
	if ( valLeft.GetType().IsPrimitive() )
	{
		if ( valLeft.GetType().IsBoolean()
			&& valRight.GetType().IsBoolean() )
		{
			LBoolean	boolLeft = valLeft.AsBoolean() ;
			LBoolean	boolRight = valRight.AsBoolean() ;
			switch ( opIndex )
			{
			case	Symbol::opEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( boolLeft == boolRight ) ) ;

			case	Symbol::opNotEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( boolLeft != boolRight ) ) ;

			default:
				break ;
			}
		}
		else if ( valLeft.GetType().IsInteger()
				&& valRight.GetType().IsInteger() )
		{
			LLong	longLeft = valLeft.AsInteger() ;
			LLong	longRight = valRight.AsInteger() ;
			switch ( opIndex )
			{
			case	Symbol::opAdd:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft + longRight ) ) ;

			case	Symbol::opSub:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft - longRight ) ) ;

			case	Symbol::opMul:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft * longRight ) ) ;

			case	Symbol::opDiv:
				if ( longRight != 0 )
				{
					return	LValue( LType::typeInt64,
									LValue::MakeLong( longLeft / longRight ) ) ;
				}
				break ;

			case	Symbol::opMod:
				if ( longRight != 0 )
				{
					return	LValue( LType::typeInt64,
									LValue::MakeLong( longLeft % longRight ) ) ;
				}
				break ;

			case	Symbol::opBitAnd:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft & longRight ) ) ;

			case	Symbol::opBitOr:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft | longRight ) ) ;

			case	Symbol::opBitXor:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft ^ longRight ) ) ;

			case	Symbol::opShiftRight:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft >> longRight ) ) ;

			case	Symbol::opShiftLeft:
				return	LValue( LType::typeInt64,
								LValue::MakeLong( longLeft << longRight ) ) ;

			case	Symbol::opEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( longLeft == longRight ) ) ;

			case	Symbol::opNotEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( longLeft != longRight ) ) ;

			case	Symbol::opLessEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( longLeft <= longRight ) ) ;

			case	Symbol::opLessThan:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( longLeft < longRight ) ) ;

			case	Symbol::opGraterEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( longLeft >= longRight ) ) ;

			case	Symbol::opGraterThan:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( longLeft > longRight ) ) ;

			default:
				break ;
			}
		}
		else
		{
			LDouble	dblLeft = valLeft.AsDouble() ;
			LDouble	dblRight = valRight.AsDouble() ;
			switch ( opIndex )
			{
			case	Symbol::opAdd:
				return	LValue( LType::typeDouble,
								LValue::MakeDouble( dblLeft + dblRight ) ) ;

			case	Symbol::opSub:
				return	LValue( LType::typeDouble,
								LValue::MakeDouble( dblLeft - dblRight ) ) ;

			case	Symbol::opMul:
				return	LValue( LType::typeDouble,
								LValue::MakeDouble( dblLeft * dblRight ) ) ;

			case	Symbol::opDiv:
				return	LValue( LType::typeDouble,
								LValue::MakeDouble( dblLeft / dblRight ) ) ;

			case	Symbol::opEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( dblLeft == dblRight ) ) ;

			case	Symbol::opNotEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( dblLeft != dblRight ) ) ;

			case	Symbol::opLessEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( dblLeft <= dblRight ) ) ;

			case	Symbol::opLessThan:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( dblLeft < dblRight ) ) ;

			case	Symbol::opGraterEqual:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( dblLeft >= dblRight ) ) ;

			case	Symbol::opGraterThan:
				return	LValue( LType::typeBoolean,
								LValue::MakeBool( dblLeft > dblRight ) ) ;

			default:
				break ;
			}
		}
	}
	else if ( valLeft.GetType().IsPointer() )
	{
		LPointerClass *	pLeftPtrClass = valLeft.GetType().GetPointerClass() ;
		assert( pLeftPtrClass != nullptr ) ;

		LPointerObj *		pLeftPtrObj =
				dynamic_cast<LPointerObj*>( valLeft.GetObject().Ptr() ) ;
		LPointerObj *		pRightPtrObj ;
		std::uint8_t *		pLeftPtr = nullptr ;
		std::uint8_t *		pRightPtr = nullptr ;
		LPtr<LPointerObj>	pPtrObj ;

		switch ( opIndex )
		{
		case	Symbol::opAdd:
			if ( pLeftPtrObj != nullptr )
			{
				pPtrObj = new LPointerObj( pLeftPtrClass ) ;
				*pPtrObj = *pLeftPtrObj ;
				*pPtrObj += (ssize_t) (valRight.AsInteger()
								* (ssize_t) pLeftPtrClass->GetElementStride()) ;
				return	LValue( pPtrObj ) ;
			}
			return	valLeft ;

		case	Symbol::opSub:
			if ( pLeftPtrObj != nullptr )
			{
				pPtrObj = new LPointerObj( pLeftPtrClass ) ;
				*pPtrObj = *pLeftPtrObj ;
				*pPtrObj -= (ssize_t) (valRight.AsInteger()
								* (ssize_t) pLeftPtrClass->GetElementStride()) ;
				return	LValue( pPtrObj ) ;
			}
			return	valLeft ;

		case	Symbol::opEqual:
		case	Symbol::opEqualPtr:
			pRightPtrObj =
				dynamic_cast<LPointerObj*>( valRight.GetObject().Ptr() ) ;

			pLeftPtr = (pLeftPtrObj != nullptr) ? pLeftPtrObj->GetPointer() : nullptr ;
			pRightPtr = (pRightPtrObj != nullptr) ? pRightPtrObj->GetPointer() : nullptr ;

			return	LValue( LType::typeBoolean, LValue::MakeBool( pLeftPtr == pRightPtr ) ) ;

		case	Symbol::opNotEqual:
		case	Symbol::opNotEqualPtr:
			pRightPtrObj =
				dynamic_cast<LPointerObj*>( valRight.GetObject().Ptr() ) ;

			pLeftPtr = (pLeftPtrObj != nullptr) ? pLeftPtrObj->GetPointer() : nullptr ;
			pRightPtr = (pRightPtrObj != nullptr) ? pRightPtrObj->GetPointer() : nullptr ;

			return	LValue( LType::typeBoolean, LValue::MakeBool( pLeftPtr != pRightPtr ) ) ;

		default:
			break ;
		}
	}
	else
	{
		switch ( opIndex )
		{
		case	Symbol::opEqual:
		case	Symbol::opEqualPtr:
			return	LValue( LType::typeBoolean,
							LValue::MakeBool( valLeft.GetObject().Ptr()
											== valRight.GetObject().Ptr() ) ) ;

		case	Symbol::opNotEqual:
		case	Symbol::opNotEqualPtr:
			return	LValue( LType::typeBoolean,
							LValue::MakeBool( valLeft.GetObject().Ptr()
											!= valRight.GetObject().Ptr() ) ) ;

		default:
			break ;
		}
	}
	return	LValue() ;		// エラー
}

// デバッグ時要素間接参照
LValue LDebugger::EvaluateElementAt
		( LContext& context,
			const LValue& valLeft, const LValue& valIndex )
{
	if ( valLeft.GetType().IsPointer() )
	{
		// ポインタ
		LPointerObj *	pPtrObj =
				dynamic_cast<LPointerObj*>( valLeft.GetObject().Ptr() ) ;
		if ( pPtrObj == nullptr )
		{
			return	LValue() ;
		}
		LPointerClass *	pPtrClass = valLeft.GetType().GetPointerClass() ;
		assert( pPtrClass != nullptr ) ;

		LType	typeElement = pPtrClass->GetBufferType() ;
		if ( typeElement.IsDataArray() )
		{
			typeElement = typeElement.GetRefElementType() ;
		}
		LPtr<LPointerObj>	pPtrElement =
			new LPointerObj( context.VM().GetPointerClassAs( typeElement ) ) ;
		*pPtrElement = *pPtrObj ;
			new LPointerObj( context.VM().GetPointerClassAs( typeElement ) ) ;
		*pPtrElement += (ssize_t) (valIndex.AsInteger()
									* (ssize_t) typeElement.GetDataBytes()) ;
		return	EvaluateData( LValue( pPtrElement ) ) ;
	}
	else if ( valLeft.GetType().IsObject()
			&& (valLeft.GetObject() != nullptr) )
	{
		// オブジェクト要素間接参照
		ssize_t	iElement = -1 ;
		if ( valIndex.GetType().IsInteger() )
		{
			iElement = (ssize_t) valIndex.AsInteger() ;
		}
		else if ( valIndex.GetType().IsString() )
		{
			iElement = valLeft.GetObject()->
						FindElementAs( valIndex.AsString().c_str() ) ;
		}
		if ( iElement >= 0 )
		{
			LType	typeElement =
						valLeft.GetObject()->GetElementTypeAt( (size_t) iElement ) ;
			LObjPtr	pElement =
						valLeft.GetObject()->GetElementAt( (size_t) iElement ) ;
			if ( (pElement != nullptr)
				&& typeElement.IsPointer() )
			{
				LPointerObj *	pPtrObj =
						dynamic_cast<LPointerObj*>( pElement.Ptr() ) ;
				if ( pPtrObj != nullptr )
				{
					LPtr<LPointerObj>	pPtrMember =
							new LPointerObj( typeElement.GetClass() ) ;
					*pPtrMember = *pPtrObj ;
					return	LValue( pPtrMember ) ;
				}
			}
			return	LValue( typeElement, pElement ) ;
		}
	}
	return	LValue() ;		// エラー
}

// ローカル変数を取得
LValue LDebugger::GetLocalVar( LContext& context, LLocalVarPtr pVar, size_t iLoc )
{
	std::shared_ptr<LStackBuffer>	pStack = context.Stack() ;
	if ( pStack->fp() + iLoc >= std::min( pStack->sp(), pStack->dp() ) )
	{
		return	LValue() ;
	}
	LValue::Primitive
			val = pStack->GetAt( pStack->fp() + iLoc ) ;
	LString	strExpr ;
	if ( pVar->GetType().IsPrimitive() )
	{
		LType::Primitive	primType = pVar->GetType().GetPrimitive() ;
		return	LValue( primType, (LContext::s_pfnLoadLocal[primType])( val ) ) ;
	}
	else if ( pVar->GetType().IsPointer()
			&& (dynamic_cast<LPointerObj*>(val.pObject) != nullptr) )
	{
		LPtr<LPointerObj>	pPtr =
				new LPointerObj( pVar->GetType().GetClass() ) ;
		*pPtr = *(dynamic_cast<LPointerObj*>(val.pObject)) ;

		if ( (pVar->GetAllocType() == LLocalVar::allocPointer)
			&& (pStack->fp() + iLoc + 2 < pStack->GetLength()) )
		{
			LValue::Primitive
				index = pStack->GetAt( pStack->fp() + iLoc + 2 ) ;
			*pPtr += (ssize_t) index.longValue ;
		}
		return	LValue( pPtr ) ;
	}
	else
	{
		return	LValue( LObjPtr( LObject::AddRef( val.pObject ) ) ) ;
	}
}
