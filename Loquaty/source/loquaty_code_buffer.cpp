
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 実行コード・バッファ
//////////////////////////////////////////////////////////////////////////////

// 副作用のないロード（プッシュ）命令か？
bool LCodeBuffer::IsLoadPushCodeWithoutEffects( InstructionCode code )
{
	return	(codeLoadRetValue <= code) && (code <= codeLoadByAddr)
			&& (code != codeCheckLPtrAlign) && (code != codeCheckPtrAlign) ;
}

// コード追加
void LCodeBuffer::AddCode( const LCodeBuffer::Word& word )
{
	m_buffer.push_back( word ) ;
}

// コード挿入
void LCodeBuffer::InsertCode( size_t iPos, const LCodeBuffer::Word& word )
{
	assert( iPos <= m_buffer.size() ) ;
	m_buffer.insert( m_buffer.begin() + iPos, word ) ;

	for ( size_t i = 0; i < m_dbgSrcInfos.size(); i ++ )
	{
		DebugSourceInfo&	dbsi = m_dbgSrcInfos.at(i) ;
		if ( dbsi.m_iCodeFirst > iPos )
		{
			dbsi.m_iCodeFirst ++ ;
		}
		if ( dbsi.m_iCodeEnd >= iPos )
		{
			dbsi.m_iCodeEnd ++ ;
		}
	}
	for ( size_t i = 0; i < m_dbgVarInfos.size(); i ++ )
	{
		DebugLocalVarInfo&	dblvi = m_dbgVarInfos.at(i) ;
		if ( dblvi.m_iCodeFirst > iPos )
		{
			dblvi.m_iCodeFirst ++ ;
		}
		if ( dblvi.m_iCodeEnd >= iPos )
		{
			dblvi.m_iCodeEnd ++ ;
		}
	}
}

// デバッグ情報追加
void LCodeBuffer::AttachFunction( LFunctionObj * pFunc )
{
	m_pFunction = pFunc ;
}

LFunctionObj * LCodeBuffer::GetFunction( void ) const
{
	return	m_pFunction ;
}

void LCodeBuffer::AttachSourceFile( LSourceFilePtr pSourceFile )
{
	m_pSourceFile = pSourceFile ;
}

LSourceFilePtr LCodeBuffer::GetSourceFile( void ) const
{
	return	m_pSourceFile ;
}

void LCodeBuffer::AddDebugSourceInfo
	( const LCodeBuffer::DebugSourceInfo& dbSrcInf )
{
	m_dbgSrcInfos.push_back( dbSrcInf ) ;
}

const LCodeBuffer::DebugSourceInfo *
	LCodeBuffer::FindDebugSourceInfo( size_t iCodePos ) const
{
	const DebugSourceInfo *	pbdsiPrev = nullptr ;
	const DebugSourceInfo *	pbdsiNext = nullptr ;
	for ( size_t i = 0; i < m_dbgSrcInfos.size(); i ++ )
	{
		const DebugSourceInfo&	dbsi = m_dbgSrcInfos.at(i) ;
		if ( (dbsi.m_iCodeFirst <= iCodePos)
			&& (iCodePos < dbsi.m_iCodeEnd) )
		{
			return	&dbsi ;
		}
		if ( (iCodePos < dbsi.m_iCodeFirst)
			&& ((pbdsiPrev == nullptr)
				|| (dbsi.m_iCodeFirst < pbdsiPrev->m_iCodeFirst)) )
		{
			pbdsiPrev = &dbsi ;
		}
		if ( (iCodePos >= dbsi.m_iCodeEnd)
			&& ((pbdsiNext == nullptr)
				|| (pbdsiNext->m_iCodeEnd < dbsi.m_iCodeEnd)) )
		{
			pbdsiNext = &dbsi ;
		}
	}
	if ( pbdsiNext != nullptr )
	{
		return	pbdsiNext ;
	}
	return	pbdsiPrev ;
}

const LCodeBuffer::DebugSourceInfo *
	LCodeBuffer::FindDebugSourceInfoAtSource( size_t iSrcPos ) const
{
	for ( size_t i = 0; i < m_dbgSrcInfos.size(); i ++ )
	{
		const DebugSourceInfo&	dbsi = m_dbgSrcInfos.at(i) ;
		if ( (dbsi.m_iSrcFirst <= iSrcPos)
			&& (iSrcPos < dbsi.m_iSrcEnd) )
		{
			return	&dbsi ;
		}
	}
	return	nullptr ;
}

void LCodeBuffer::AddDebugLocalVarInfo
	( LLocalVarArrayPtr pVar, size_t iFirst, size_t iEnd )
{
	DebugLocalVarInfo	dbgVarInfo ;
	dbgVarInfo.m_varInfo = pVar ;
	dbgVarInfo.m_iCodeFirst = iFirst ;
	dbgVarInfo.m_iCodeEnd = iEnd ;

	m_dbgVarInfos.push_back( dbgVarInfo ) ;
}

void LCodeBuffer::GetDebugLocalVarInfos
	( std::vector<LCodeBuffer::DebugLocalVarInfo>& dbgVarInfos, size_t iPos ) const
{
	for ( size_t i = 0; i < m_dbgVarInfos.size(); i ++ )
	{
		const DebugLocalVarInfo&	dlvi = m_dbgVarInfos.at(i) ;
		if ( (dlvi.m_iCodeFirst <= iPos)
			&& (iPos < dlvi.m_iCodeEnd) )
		{
			dbgVarInfos.push_back( dlvi ) ;
		}
	}
}

// コードを疑似ニーモニックに変換
LString LCodeBuffer::MnemonicOf
		( const LCodeBuffer::Word& word, size_t iCodePos ) const
{
	if ( word.code >= codeInstructionCount )
	{
		return	LString( L"???" ) ;
	}
	const MnemonicInfo&	mnnInfo = s_MnemonicInfos[word.code] ;

	LString	strMnemonic = FormatMnemonic( mnnInfo.mnemonic, word ) ;

	strMnemonic += LString(L" ")
					* (16 - std::min( (int) strMnemonic.GetLength(), 15 )) ;

	strMnemonic += FormatMnemonic( mnnInfo.operands, word ) ;

	if ( mnnInfo.flags & optionSubSp )
	{
		if ( word.imm > 0 )
		{
			strMnemonic += L", pop(" + LString::IntegerOf(word.imm) + L")" ;
		}
	}
	if ( mnnInfo.flags & optionSubSp2 )
	{
		size_t	nCount = word.sop2 + (size_t) word.op3 * 0x100 ;
		if ( nCount > 0 )
		{
			strMnemonic += L", cpop(" + LString::IntegerOf(nCount) + L")" ;
		}
	}

	if ( mnnInfo.flags & (optionFpImop | optionFpImm | optionFpSop1) )
	{
		size_t	iVar = (size_t) word.imop.value.longValue ;
		if ( mnnInfo.flags & optionFpImm )
		{
			iVar = (size_t) word.imm ;
		}
		else if ( mnnInfo.flags & optionFpSop1 )
		{
			iVar = (size_t) word.sop1 ;
		}
		for ( const DebugLocalVarInfo& dlvi : m_dbgVarInfos )
		{
			if ( (dlvi.m_iCodeFirst <= iCodePos)
				&& (iCodePos < dlvi.m_iCodeEnd) )
			{
				const std::wstring *
					pstrName = dlvi.m_varInfo->GetLocalVarNameAt( iVar ) ;
				if ( pstrName != nullptr )
				{
					if ( strMnemonic.GetLength() < 48 )
					{
						strMnemonic += LString(L" ")
										* (int)(48 - strMnemonic.GetLength()) ;
					}
					strMnemonic += L"  ; " ;
					strMnemonic += *pstrName ;
					//
					LLocalVarPtr	pVar = dlvi.m_varInfo->GetLocalVarAt( iVar ) ;
					if ( pVar != nullptr )
					{
						strMnemonic += L": " ;
						strMnemonic += pVar->GetType().GetTypeName() ;
					}
					break ;
				}
			}
		}
	}

	if ( (mnnInfo.flags & optionVirtual)
		&& (word.imop.pClass != nullptr) )
	{
		const LVirtualFuncVector&
				vecVirtFunc = word.imop.pClass->GetVirtualVector() ;
		LPtr<LFunctionObj>
				pFuncObj = vecVirtFunc.GetFunctionAt( (size_t) word.imm ) ;
		if ( pFuncObj != nullptr )
		{
			if ( strMnemonic.GetLength() < 48 )
			{
				strMnemonic += LString(L" ")
								* (int)(48 - strMnemonic.GetLength()) ;
			}
			strMnemonic += L"  ; " ;
			strMnemonic += pFuncObj->GetPrintName() ;
			//
			if ( pFuncObj->GetPrototype() != nullptr )
			{
				strMnemonic += L": " ;
				strMnemonic += pFuncObj->GetPrototype()->TypeToString() ;
			}
		}
	}
	return	strMnemonic ;
}

LString LCodeBuffer::FormatMnemonic
		( const wchar_t * mnemonic, const LCodeBuffer::Word& word )
{
	return	LString::Format( mnemonic, [word]( const LString& key )
		{
			if ( key == L"imm" )
			{
				return	LString::IntegerOf( word.imm ) ;
			}
			if ( key == L"+imm" )
			{
				if ( word.imm >= 0 )
				{
					LString	strImm = L"+" ;
					strImm += LString::IntegerOf( word.imm ) ;
					return	strImm ;
				}
				else
				{
					return	LString::IntegerOf( word.imm ) ;
				}
			}
			if ( key == L"imop.i" )
			{
				return	LString::IntegerOf( word.imop.value.longValue ) ;
			}
			if ( key == L"+imop.i" )
			{
				if ( word.imop.value.longValue >= 0 )
				{
					LString	strImop = L"+" ;
					strImop += LString::IntegerOf( word.imop.value.longValue ) ;
					return	strImop ;
				}
				else
				{
					return	LString::IntegerOf( word.imop.value.longValue ) ;
				}
			}
			if ( key == L"sop1" )
			{
				return	LString::IntegerOf( word.sop1 ) ;
			}
			if ( key == L"sop2" )
			{
				return	LString::IntegerOf( word.sop2 ) ;
			}
			if ( key == L"op3" )
			{
				return	LString::IntegerOf( word.op3 ) ;
			}
			if ( key == L"op3.type" )
			{
				if ( word.op3 < LType::typePrimitiveCount )
				{
					return	LString( LType::s_pwszPrimitiveTypeName[word.op3] ) ;
				}
				else
				{
					return	LString( L"obj" ) ;
				}
			}
			if ( (key == L"op3.operator")
					&& (word.op3 < Symbol::opOperatorCount) )
			{
				LString	strOp = L"op." ;
				strOp += Symbol::s_OperatorDescs[word.op3].pwszFuncName ;
				return	strOp ;
			}
			if ( key == L"imop" )
			{
				LType::Primitive	type = (LType::Primitive) word.op3 ;
				if ( type == LType::typeBoolean )
				{
					return	LString( word.imop.value.boolValue ? L"true" : L"false" ) ;
				}
				else if ( LType::IsObjectType( type ) )
				{
					return	LObject::ToExpression( word.imop.value.pObject ) ;
				}
				else if ( LType::IsFloatingPointPrimitive( type ) )
				{
					return	LString::NumberOf( word.imop.value.dblValue ) ;
				}
				else
				{
					return	LString::IntegerOf( word.imop.value.longValue ) ;
				}
			}
			if ( key == L"imop.func" )
			{
				if ( word.imop.pFunc != nullptr )
				{
					return	word.imop.pFunc->GetFullPrintName() ;
				}
				else
				{
					return	LString( L"null" ) ;
				}
			}
			if ( key == L"imop.cls" )
			{
				if ( word.imop.pClass != nullptr )
				{
					return	word.imop.pClass->GetFullClassName() ;
				}
				else
				{
					return	LString( L"null" ) ;
				}
			}
			if ( key == L"imop.obj" )
			{
				return	LObject::ToExpression( word.imop.value.pObject ) ;
			}
			return	LString( L"?" ) ;
		} ) ;
}

const LCodeBuffer::MnemonicInfo
	LCodeBuffer::s_MnemonicInfos[LCodeBuffer::codeInstructionCount] =
{
	{	codeNoOperation,
		L"nop",				L"",			0	},
	{	codeEnterFunc,
		L"fnc.enter",		L"sp+=%(imm)",	0	},
	{	codeLeaveFunc,
		L"fnc.leave",		L"",			0	},
	{	codeEnterTry,
		L"try.enter ",		L"",			0	},
	{	codeCallFinally,
		L"finally",			L"",			0	},
	{	codeRetFinally,
		L"finally.ret",		L"",			0	},
	{	codeLeaveFinally,
		L"finally.leave",	L"",			0	},
	{	codeLeaveTry,
		L"try.leave",		L"",			0	},
	{	codeThrow,
		L"throw",			L"",			0	},
	{	codeAllocStack,
		L"push.alloc",		L"sp+=%(imm)",		0	},
	{	codeFreeStack,
		L"pop.free",		L"sp-=%(imm)",		0	},
	{	codeFreeStackLocal,
		L"move",			L"sp, fp%(+imop.i)",	0	},
	{	codeFreeLocalObj,
		L"obj.free",		L"fp[%(imop.i)]",	optionFpImop	},
	{	codePushObjChain,
		L"push.chain",		L"",			0	},
	{	codeMakeObjChain,
		L"make.chain",		L"fp[%(imop.i)]",	optionFpImop	},
	{	codeSynchronize,
		L"obj.sync",		L"fp[%(imop.i)]",	optionFpImop	},
	{	codeReturn,
		L"return",			L"",			optionSubSp	},
	{	codeSetRetValue,
		L"move",			L"rval, st(%(sop1)).%(op3.type)",	optionSubSp	},
	{	codeMoveAP,
		L"move",			L"ap, sp-%(imm)",	0	},
	{	codeCall,
		L"call",			L"%(imop.func)",	0	},
	{	codeCallVirtual,
		L"call.virtual",	L"%(imop.cls)[%(imm)]",	optionVirtual	},
	{	codeCallIndirect,
		L"call.func",		L"",	0	},
	{	codeExImmPrefix,
		L"move",			L"eximm, %(imop)",	0	},
	{	codeClearException,
		L"move",			L"except, null",	0	},
	{	codeLoadRetValue,
		L"push",			L"rval",	0	},
	{	codeLoadException,
		L"push",			L"except",	0	},
	{	codeLoadArg,
		L"push",			L"ap[%(imm)]",	optionApImm	},
	{	codeLoadImm,
		L"push",			L"%(imop)",	0	},
	{	codeLoadLocal,
		L"push",			L"fp[%(imop.i)].%(op3.type)",	optionFpImop	},
	{	codeLoadLocalOffset,
		L"push",			L"fp[st(%(sop1))%(+imop.i)].%(op3.type)",	0	},
	{	codeLoadStack,
		L"push",			L"st(%(sop1))",	0	},
	{	codeLoadFetchAddr,
		L"push.faddr",		L"st(%(sop1)), %(imop.i), %(imm), %(op3)",	0	},
	{	codeLoadFetchAddrOffset,
		L"push.faddr",		L"st(%(sop1)), st(%(sop2))%(+imop.i), %(imm), %(op3)",	0	},
	{	codeLoadFetchLAddr,
		L"push.faddr",		L"fp[%(sop1)], st(%(sop2))%(+imop.i), %(imm), %(op3)",	optionFpSop1	},
	{	codeCheckLPtrAlign,
		L"chk.align",		L"fp[%(imop.i)]+st(%(sop1))%(+imm), %(op3)",	optionFpImop	},
	{	codeCheckPtrAlign,
		L"chk.align",		L"st(%(sop1))%(+imm), %(op3)",	0	},
	{	codeLoadLPtr,
		L"push.load",		L"[fp[%(imop.i)]%(+imm)].%(op3.type)",	optionFpImop	},
	{	codeLoadLPtrOffset,
		L"push.load",		L"[fp[%(imop.i)]+st(%(sop1))%(+imm)].%(op3.type)",	optionFpImop	},
	{	codeLoadByPtr,
		L"push.load",		L"[st(%(sop1))%(+imm)].%(op3.type)",	0	},
	{	codeLoadByLAddr,
		L"push.load",		L"*[fp[%(imop.i)]%(+imm)].%(op3.type)",	0	},
	{	codeLoadByAddr,
		L"push.load",		L"*[st(%(sop1))%(+imm)].%(op3.type)",	0	},
	{	codeLoadLocalPtr,
		L"push.ptr",		L"&fp[%(imop.i)], %(imm)",	optionFpImop	},
	{	codeLoadObjectPtr,
		L"push.ptr",		L"sp[%(sop1)].ptr",		0	},
	{	codeLoadLObjectPtr,
		L"push.ptr",		L"fp[%(imop.i)].ptr",	optionFpImop	},
	{	codeLoadObject,
		L"push.obj",		L"%(imop.obj)",	0	},
	{	codeNewObject,
		L"push.new",		L"%(imop.cls)",	0	},
	{	codeAllocBuf,
		L"alloc",			L"st(%(sop1)), st(%(sop2)), %(imop.cls)",	0	},
	{	codeRefObject,
		L"addref",			L"st(%(sop1))",	0	},
	{	codeFreeObject,
		L"free",			L"st(%(sop1))",	0	},
	{	codeMove,
		L"move",			L"st(%(sop2)), st(%(sop1))",	optionSubSp	},
	{	codeStoreLocalImm,
		L"move",			L"fp[%(imm)].%(op3.type), %(imop)",	optionFpImm	},
	{	codeStoreLocal,
		L"move",			L"fp[%(imop.i)].%(op3.type), st(%(sop1))",	optionSubSp | optionFpImop	},
	{	codeExchangeLocal,
		L"xchg",			L"fp[%(imop.i)].%(op3.type), st(%(sop1))",	optionSubSp | optionFpImop	},
	{	codeStoreByPtr,
		L"move",			L"[st(%(sop1))%(+imm)].%(op3.type), st(%(sop2))",	0	},
	{	codeStoreByLPtr,
		L"move",			L"[fp[%(imop.i)]%(+imm)].%(op3.type), st(%(sop2))",	0	},
	{	codeStoreByLAddr,
		L"move",			L"*[fp[%(imop.i)%(+imm)].%(op3.type), st(%(sop2))",	optionFpImop	},
	{	codeStoreByAddr,
		L"move",			L"*[st(%(sop1))%(+imm)].%(op3.type), st(%(sop2))",	0	},
	{	codeBinaryOperate,
		L"%(op3.operator)",	L"st(%(sop1)), st(%(sop2))",	optionSubSp	},
	{	codeUnaryOperate,
		L"%(op3.operator)",	L"st(%(sop1))",	optionSubSp	},
	{	codeCastObject,
		L"push.cast",		L"st(%(sop1)), %(imop.cls)",	optionSubSp	},
	{	codeJump,
		L"jump",			L"#%(imop.i)",	optionSubSp	},
	{	codeJumpConditional,
		L"jump.nz",			L"st(%(sop1)), pop(%(imm)), #%(imop.i)",	optionSubSp2	},
	{	codeJumpNonConditional,
		L"jump.z",			L"st(%(sop1)), pop(%(imm)), #%(imop.i)",	optionSubSp2	},
	{	codeGetElement,
		L"push.objat",		L"st(%(sop1)), st(%(sop2))",	optionSubSp	},
	{	codeGetElementAs,
		L"push.objas",		L"st(%(sop1)), st(%(sop2))",	optionSubSp	},
	{	codeGetElementName,
		L"push.objkey",		L"st(%(sop1)), st(%(sop2))",	optionSubSp	},
	{	codeSetElement,
		L"obj.setat",		L"st(%(sop1)), st(%(sop2)), st(%(op3))",	optionSubSp	},
	{	codeSetElementAs,
		L"obj.setas",		L"st(%(sop1)), st(%(sop2)), st(%(op3))",	optionSubSp	},
	{	codeObjectToInt,
		L"cvt.o2int",		L"st(%(sop2)), st(%(sop1))",	0	},
	{	codeObjectToFloat,
		L"cvt.o2float",		L"st(%(sop2)), st(%(sop1))",	0	},
	{	codeObjectToString,
		L"push.asstr",		L"st(%(sop1))",	0	},
	{	codeIntToObject,
		L"push.intobj",		L"st(%(sop1))",	0	},
	{	codeFloatToObject,
		L"push.floatobj",	L"st(%(sop1))",	0	},
	{	codeIntToFloat,
		L"cvt.i2float",		L"st(%(sop2)), st(%(sop1))",	0	},
	{	codeUintToFloat,
		L"cvt.ui2float",	L"st(%(sop2)), st(%(sop1))",	0	},
	{	codeFloatToInt,
		L"cvt.f2int",		L"st(%(sop2)), st(%(sop1))",	0	},
	{	codeFloatToUint,
		L"cvt.f2uint",		L"st(%(sop2)), st(%(sop1))",	0	},
} ;


