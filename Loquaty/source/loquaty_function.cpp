
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// 関数の引数の型配列
//////////////////////////////////////////////////////////////////////////////

// 引数型取得
LType LArgumentListType::GetArgTypeAt( size_t index ) const
{
	if ( index < std::vector<LType>::size() )
	{
		return	std::vector<LType>::at( index ) ;
	}
	return	LType() ;
}

// 引数型設定（配列長自動伸長）
void LArgumentListType::SetArgTypeAt( size_t index, const LType& type )
{
	if ( index >= std::vector<LType>::size() )
	{
		std::vector<LType>::resize( index + 1 ) ;
	}
	std::vector<LType>::at(index) = type ;
}

// 引数リストの一致判定
bool LArgumentListType::IsEqualArgmentList( const LArgumentListType& arglist ) const
{
	if ( std::vector<LType>::size() != arglist.size() )
	{
		return	false ;
	}
	for ( size_t i = 0; i < std::vector<LType>::size(); i ++ )
	{
		if ( !std::vector<LType>::at(i).IsEqual( arglist.at(i) ) )
		{
			return	false ;
		}
	}
	return	true ;
}

// 暗黙の型変換可能な引数リスト判定
bool LArgumentListType::DoesMatchArgmentListWith( const LArgumentListType& arglist ) const
{
	if ( std::vector<LType>::size() != arglist.size() )
	{
		return	false ;
	}
	for ( size_t i = 0; i < std::vector<LType>::size(); i ++ )
	{
		if ( arglist.at(i).CanImplicitCastTo
					( std::vector<LType>::at(i) ) == LType::castImpossible )
		{
			return	false ;
		}
	}
	return	true ;
}

// 引数型リスト文字列化
//////////////////////////////////////////////////////////////////////////////
LString LArgumentListType::ToString( void ) const
{
	LString	str ;
	for ( size_t i = 0; i < std::vector<LType>::size(); i ++ )
	{
		if ( i >= 1 )
		{
			str += L"," ;
		}
		str += std::vector<LType>::at(i).GetTypeName() ;
	}
	return	str ;
}



//////////////////////////////////////////////////////////////////////////////
// 関数の引数の名前と型配列
//////////////////////////////////////////////////////////////////////////////

// 引数名取得（省略されている場合は nullptr）
const wchar_t * LNamedArgumentListType::GetArgNameAt( size_t index ) const
{
	if ( index < m_argNames.size() )
	{
		std::shared_ptr<LString>	pName = m_argNames.at( index ) ;
		if ( pName != nullptr )
		{
			return	pName->c_str() ;
		}
	}
	return	nullptr ;
}

// 引数名設定
void LNamedArgumentListType::SetArgNameAt( size_t index, const wchar_t * pwszName )
{
	if ( index >= m_argNames.size() )
	{
		m_argNames.resize( index + 1, nullptr ) ;
	}
	m_argNames.at(index) = std::make_shared<LString>( pwszName ) ;
}




//////////////////////////////////////////////////////////////////////////////
// 関数プロトタイプ
//////////////////////////////////////////////////////////////////////////////

// 暗黙の型変換可能な引数リスト判定
bool LPrototype::DoesMatchArgmentListWith( const LArgumentListType& arglist ) const
{
	if ( arglist.size() > m_argListType.size() )
	{
		return	false ;
	}
	if ( arglist.size() < m_argListType.size() )
	{
		LArgumentListType	tmpArgs = arglist ;
		for ( size_t i = arglist.size(); i < m_argListType.size(); i ++ )
		{
			LValue	valDef = GetDefaultArgAt( i ) ;
			if ( valDef.IsVoid() )
			{
				return	false ;
			}
			tmpArgs.SetArgTypeAt( i, valDef.GetType() ) ;
		}
		return	m_argListType.DoesMatchArgmentListWith( tmpArgs ) ;
	}
	else
	{
		return	m_argListType.DoesMatchArgmentListWith( arglist ) ;
	}
}

// プロトタイプ文字列化
LString LPrototype::TypeToString( void ) const
{
	LString	str = m_typeReturn.GetTypeName()
					+ L"(" + m_argListType.ToString() + L")" ;

	if ( m_refObjTypes.size() >= 1 )
	{
		str += L"[" ;
		str += m_refObjTypes.ToString() ;
		str += L"]" ;
	}

	if ( m_codeBuf != nullptr )
	{
		str += L"{" ;
		str += LString::IntegerOf
					( reinterpret_cast<LLong>( m_codeBuf.get() ), 0, 16 );
		str += L"}" ;
	}

	if ( m_pThisClass != nullptr )
	{
		if ( IsConstThis() )
		{
			str += L"const " ;
		}
		str += m_pThisClass->GetFullClassName() ;
	}
	return	str ;
}

// 関数キャプチャー引数型情報を追加
void LPrototype::AddCaptureObjectType
	( const wchar_t * pwszName, const LType& type )
{
	size_t	index = m_refObjTypes.size() ;
	m_refObjTypes.push_back( type ) ;
	m_refObjTypes.SetArgNameAt( index, pwszName ) ;
}



//////////////////////////////////////////////////////////////////////////////
// 関数オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 複製する（要素も全て複製処理する）
// （返り値は呼び出し側の責任で ReleaseRef）
LObject * LFunctionObj::CloneObject( void ) const
{
	return	new LFunctionObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
// （返り値は呼び出し側の責任で ReleaseRef）
LObject * LFunctionObj::DuplicateObject( void ) const
{
	return	new LFunctionObj( *this ) ;
}

// 内部リソースを解放する
void LFunctionObj::DisposeObject( void )
{
	LObject::DisposeObject() ;

	m_prototype = nullptr ;
	m_codeBuf = nullptr ;
	m_funcNative = nullptr ;
	m_refObjects.clear() ;
}

// private を不可視に設定する
void LFunctionObj::MakePrivateInvisible( void )
{
	if ( (m_prototype->GetModifiers()
					& LType::accessMask) == LType::modifierPrivate )
	{
		MakeInvisible() ;
	}
}

// 無条件に不可視に設定する
void LFunctionObj::MakeInvisible( void )
{
	m_prototype = std::make_shared<LPrototype>( *m_prototype ) ;
	m_prototype->MakeInvisible() ;
}

// 関数名
LString LFunctionObj::GetFullName( void ) const
{
	if ( !m_name.IsEmpty() )
	{
		LString	strNamespace ;
		if ( m_namespace != nullptr )
		{
			strNamespace = m_namespace->GetFullName() ; ;
		}
		else if ( (m_prototype != nullptr)
				&& (m_prototype->GetThisClass() != nullptr) )
		{
			strNamespace = m_prototype->GetThisClass()->GetFullClassName() ;
		}
		if ( !strNamespace.IsEmpty() )
		{
			return	strNamespace + L"." + m_name ;
		}
		return	m_name ;
	}
	return	LString() ;
}

// 関数名（表記用）
LString LFunctionObj::GetPrintName( void ) const
{
	if ( (m_name == LClass::s_Constructor)
		&& (m_prototype != nullptr)
		&& (m_prototype->GetThisClass() != nullptr) )
	{
		return	m_prototype->GetThisClass()->GetClassName() ;
	}
	return	m_name ;
}

LString LFunctionObj::GetFullPrintName( void ) const
{
	if ( !m_name.IsEmpty() )
	{
		LString	strNamespace ;
		if ( m_namespace != nullptr )
		{
			strNamespace = m_namespace->GetFullName() ; ;
		}
		else if ( (m_prototype != nullptr)
				&& (m_prototype->GetThisClass() != nullptr) )
		{
			strNamespace = m_prototype->GetThisClass()->GetFullClassName() ;
		}
		if ( !strNamespace.IsEmpty() )
		{
			return	strNamespace + L"." + GetPrintName() ;
		}
		return	m_name ;
	}
	return	LString() ;
}

// 関数実装コードのデバッグ用出力
void LFunctionObj::TraceCodeMnemonicList( void ) const
{
	if ( m_codeBuf == nullptr )
	{
		return ;
	}
	const std::vector<LCodeBuffer::Word>&	bufCodes = m_codeBuf->GetBuffer() ;
	for ( size_t i = 0; i < bufCodes.size(); i ++ )
	{
		LTrace( "#%d: %s\n", (int) i,
			m_codeBuf->MnemonicOf( bufCodes.at(i), i ).ToString().c_str() ) ;
	}
}

// 低水準ネイティブ関数
void LFunctionObj::SetNakedNative
	( LFunctionObj::NakedNativeFuncPtr func, const CallAttributes& callAttr )
{
	if ( callAttr.protocol == protocolLoquaty )
	{
		m_funcNative = (PFN_NativeProc) func ;
		return ;
	}
	m_funcNative = NakedInvoker::MakeFunction( this, func, callAttr ) ;
}

// 関数キャプチャー引数を追加
void LFunctionObj::SetCaptureObjectList( const std::vector<LValue>& refObjs )
{
	m_refObjects = refObjs ;
}

// 関数キャプチャー引数の数を取得
size_t LFunctionObj::GetCaptureObjectCount( void ) const
{
	return	m_refObjects.size() ;
}

// 関数キャプチャー引数を取得
LValue LFunctionObj::GetCaptureObjectAt( size_t index ) const
{
	if ( index < m_refObjects.size() )
	{
		return	m_refObjects.at(index) ;
	}
	return	LValue() ;
}


//////////////////////////////////////////////////////////////////////////////
// ネイティブ関数呼び出し
//////////////////////////////////////////////////////////////////////////////

// 引数をネイティブ形式に変換する
void NakedInvoker::MakeArgument
	( LContext& context, const LPrototype& proto,
		const LFunctionObj::CallAttributes& callAttr )
{
	LRuntimeArgList	argList( context ) ;

	const LNamedArgumentListType&	argTypes = proto.GetArgListType() ;
	const size_t	count = argTypes.size() ;

	if ( (proto.GetModifiers() & LType::modifierStatic)
		&& (proto.GetThisClass() != nullptr) )
	{
		m_arg.reserve( (count + 1)
						* (sizeof(LStackBuffer::Word)/sizeof(void*)) ) ;

		LType	typeThis( proto.GetThisClass() ) ;
		AddArgument( argList, typeThis, callAttr ) ;
	}
	else
	{
		m_arg.reserve( count * (sizeof(LStackBuffer::Word)/sizeof(void*)) ) ;
	}
	for ( size_t i = 0; i < count; i ++ )
	{
		AddArgument( argList, argTypes[i], callAttr ) ;
	}
}

void NakedInvoker::AddArgument
	( LRuntimeArgList& argList,
		const LType& type,
		const LFunctionObj::CallAttributes& callAttr )
{
	LStackBuffer::Word	word = argList.NextPrimitive() ;

	if ( type.IsPrimitive() )
	{
		if ( type.GetPrimitive() == LType::typeFloat )
		{
			word.flValue = (LFloat) word.dblValue ;
		}
		if ( (sizeof(void*) == sizeof(LStackBuffer::Word))
			|| (sizeof(void*) >= type.GetDataBytes()) )
		{
			m_arg.push_back( word.pVoidPtr ) ;
		}
		else
		{
			assert( sizeof(void*)*2 == sizeof(LStackBuffer::Word) ) ;
			void**	ppVoid = &(word.pVoidPtr) ;
			m_arg.push_back( ppVoid[0] ) ;
			m_arg.push_back( ppVoid[1] ) ;
		}
	}
	else if ( type.IsString() )
	{
		LStringObj*	pStrObj = dynamic_cast<LStringObj*>( word.pObject ) ;
		size_t		iBuf ;
		switch ( callAttr.string )
		{
		case	LFunctionObj::stringToMBS:
			// const char* として渡す
			iBuf = m_str.size() ;
			m_str.resize( iBuf + 1 ) ;
			if ( pStrObj != nullptr )
			{
				m_arg.push_back
					( pStrObj->m_string.ToString( m_str[iBuf] ).c_str() ) ;
			}
			else if ( word.pObject != nullptr )
			{
				LString	str ;
				word.pObject->AsString( str ) ;
				m_arg.push_back
					( str.ToString( m_str[iBuf] ).c_str() ) ;
			}
			else
			{
				m_arg.push_back( nullptr ) ;
			}
			break ;

		case	LFunctionObj::stringToWCS:
			// const wchar_t* として渡す
			if ( pStrObj != nullptr )
			{
				m_arg.push_back( pStrObj->m_string.c_str() ) ;
			}
			else if ( word.pObject != nullptr )
			{
				iBuf = m_wstr.size() ;
				m_wstr.resize( iBuf + 1 ) ;
				word.pObject->AsString( m_wstr[iBuf] ) ;
				m_arg.push_back( m_wstr[iBuf].c_str() ) ;
			}
			else
			{
				m_arg.push_back( nullptr ) ;
			}
			break ;

		case	LFunctionObj::stringToUTF8:
			// const uint8_t* として渡す
			iBuf = m_utf8.size() ;
			m_utf8.resize( iBuf + 1 ) ;
			if ( pStrObj != nullptr )
			{
				pStrObj->m_string.ToUTF8( m_utf8[iBuf] ) ;
				m_utf8[iBuf].push_back( 0 ) ;
				m_arg.push_back( m_utf8[iBuf].data() ) ;
			}
			else if ( word.pObject != nullptr )
			{
				LString	str ;
				word.pObject->AsString( str ) ;
				str.ToUTF8( m_utf8[iBuf] ) ;
				m_utf8[iBuf].push_back( 0 ) ;
				m_arg.push_back( m_utf8[iBuf].data() ) ;
			}
			else
			{
				m_arg.push_back( nullptr ) ;
			}
			break ;

		case	LFunctionObj::stringToUTF16:
			// const uint16_t* として渡す
			iBuf = m_utf16.size() ;
			m_utf16.resize( iBuf + 1 ) ;
			if ( pStrObj != nullptr )
			{
				pStrObj->m_string.ToUTF16( m_utf16[iBuf] ) ;
				m_utf16[iBuf].push_back( 0 ) ;
				m_arg.push_back( m_utf16[iBuf].data() ) ;
			}
			else if ( word.pObject != nullptr )
			{
				LString	str ;
				word.pObject->AsString( str ) ;
				str.ToUTF16( m_utf16[iBuf] ) ;
				m_utf16[iBuf].push_back( 0 ) ;
				m_arg.push_back( m_utf16[iBuf].data() ) ;
			}
			else
			{
				m_arg.push_back( nullptr ) ;
			}
			break ;

		case	LFunctionObj::stringObject:
		default:
			// LObject* として渡す
			m_obj.push_back( LObjPtr( LObject::AddRef( word.pObject ) ) ) ;
			m_arg.push_back( word.pObject ) ;
			break ;
		}
	}
	else if ( type.IsPointer() )
	{
		switch ( callAttr.pointer )
		{
		case	LFunctionObj::pointerNaked:
			// 直接のネイティブポインタとして渡す
			if ( word.pObject != nullptr )
			{
				LPtr<LPointerObj>	pPtrObj( word.pObject->GetBufferPoiner() ) ;
				if ( pPtrObj != nullptr )
				{
					m_obj.push_back( LObjPtr( pPtrObj ) ) ;
					m_arg.push_back( pPtrObj->GetPointer() ) ;
					break ;
				}
			}
			m_arg.push_back( nullptr ) ;
			break ;

		case	LFunctionObj::pointerObject:
		default:
			// LObject* として渡す
			m_obj.push_back( LObjPtr( LObject::AddRef( word.pObject ) ) ) ;
			m_arg.push_back( word.pObject ) ;
			break ;
		}
	}
	else if ( dynamic_cast<LNativeObjClass*>( type.GetClass() ) != nullptr )
	{
		switch ( callAttr.nativeObj )
		{
		case	LFunctionObj::nativeObjPtr:
			// C++ クラスのポインタとして渡す
			if ( word.pObject != nullptr )
			{
				std::shared_ptr<Object>
					pNativeObj = LNativeObj::GetNativeObject( word.pObject ) ;
				m_nativeObj.push_back( pNativeObj ) ;
				m_arg.push_back( pNativeObj.get() ) ;
				break ;
			}
			m_arg.push_back( nullptr ) ;
			break ;

		case	LFunctionObj::nativeObject:
		default:
			// LObject* として渡す
			m_obj.push_back( LObjPtr( LObject::AddRef( word.pObject ) ) ) ;
			m_arg.push_back( word.pObject ) ;
			break ;
		}
	}
	else
	{
		// LObject* として渡す
		m_obj.push_back( LObjPtr( LObject::AddRef( word.pObject ) ) ) ;
		m_arg.push_back( word.pObject ) ;
	}
}

// 引数を受け渡し可能か？
bool NakedInvoker::HasCapacity( const LPrototype& proto )
{
	size_t	count = 0 ;

	if ( (proto.GetModifiers() & LType::modifierStatic)
		&& (proto.GetThisClass() != nullptr) )
	{
		count ++ ;
	}
	const LNamedArgumentListType&	argTypes = proto.GetArgListType() ;
	for ( size_t i = 0; i < argTypes.size(); i ++ )
	{
		if ( argTypes[i].IsPrimitive() )
		{
			if ( (sizeof(void*) == sizeof(LStackBuffer::Word))
				|| (sizeof(void*) >= argTypes[i].GetDataBytes()) )
			{
				count ++ ;
			}
			else
			{
				count += 2 ;
			}
		}
		else
		{
			count ++ ;
		}
	}
	return	(count <= MaxArguments) ;
}

// 返り値変換
LValue NakedInvoker::ToLValue( int* )
{
	return	LValue() ;
}

LValue NakedInvoker::ToLValue( bool value )
{
	return	LValue( LType::typeBoolean, LValue::MakeBool(value) ) ;
}

LValue NakedInvoker::ToLValue( int value )
{
	return	LValue( LType::typeInt32, LValue::MakeLong(value) ) ;
}

LValue NakedInvoker::ToLValue( std::int64_t value )
{
	return	LValue( LType::typeInt64, LValue::MakeLong(value) ) ;
}

LValue NakedInvoker::ToLValue( float value )
{
	return	LValue( LType::typeFloat, LValue::MakeDouble(value) ) ;
}

LValue NakedInvoker::ToLValue( double value )
{
	return	LValue( LType::typeDouble, LValue::MakeDouble(value) ) ;
}

LValue NakedInvoker::ToLValue( LObject* value )
{
	return	LValue( LObjPtr( value ) ) ;
}

// ブリッジ関数生成
std::function<void(LContext&)>
	NakedInvoker::MakeFunction
		( LFunctionObj* pFunc,
			LFunctionObj::NakedNativeFuncPtr func,
			LFunctionObj::CallAttributes callAttr  )
{
	if ( callAttr.protocol == LFunctionObj::protocolLoquaty )
	{
		return	reinterpret_cast<PFN_NativeProc>( func ) ;
	}

	const LType&	typeRet = pFunc->GetPrototype()->GetReturnType() ;

	if ( callAttr.protocol == LFunctionObj::protocolStdcall )
	{
		if ( typeRet.IsVoid() )
		{
			return	MakeStdCallFunction<int*>( pFunc, func, callAttr ) ;
		}
		else if ( typeRet.IsBoolean() )
		{
			return	MakeStdCallFunction<bool>( pFunc, func, callAttr ) ;
		}
		else if ( typeRet.IsInteger() )
		{
			if ( typeRet.GetDataBytes() == 8 )
			{
				return	MakeStdCallFunction<std::int64_t>( pFunc, func, callAttr ) ;
			}
			else
			{
				return	MakeStdCallFunction<int>( pFunc, func, callAttr ) ;
			}
		}
		else if ( typeRet.IsFloatingPointNumber() )
		{
			if ( typeRet.GetDataBytes() == 8 )
			{
				return	MakeStdCallFunction<double>( pFunc, func, callAttr ) ;
			}
			else
			{
				return	MakeStdCallFunction<float>( pFunc, func, callAttr ) ;
			}
		}
		else
		{
			return	MakeStdCallFunction<LObject*>( pFunc, func, callAttr ) ;
		}
	}
	if ( callAttr.protocol == LFunctionObj::protocolCdecl )
	{
		if ( typeRet.IsVoid() )
		{
			return	MakeCdeclFunction<int*>( pFunc, func, callAttr ) ;
		}
		else if ( typeRet.IsBoolean() )
		{
			return	MakeCdeclFunction<bool>( pFunc, func, callAttr ) ;
		}
		else if ( typeRet.IsInteger() )
		{
			if ( typeRet.GetDataBytes() == 8 )
			{
				return	MakeCdeclFunction<std::int64_t>( pFunc, func, callAttr ) ;
			}
			else
			{
				return	MakeCdeclFunction<int>( pFunc, func, callAttr ) ;
			}
		}
		else if ( typeRet.IsFloatingPointNumber() )
		{
			if ( typeRet.GetDataBytes() == 8 )
			{
				return	MakeCdeclFunction<double>( pFunc, func, callAttr ) ;
			}
			else
			{
				return	MakeCdeclFunction<float>( pFunc, func, callAttr ) ;
			}
		}
		else
		{
			return	MakeCdeclFunction<LObject*>( pFunc, func, callAttr ) ;
		}
	}
	return	nullptr ;
}


//////////////////////////////////////////////////////////////////////////////
// （同名関数の）異なるプロトタイプ関数の集合
//////////////////////////////////////////////////////////////////////////////

// 関数追加、又はオーバーライド
// （返り値はオーバーライドされた以前の関数）
LPtr<LFunctionObj> LFunctionVariation::OverloadFunction( LFunctionObj * pFunc )
{
	assert( pFunc != nullptr ) ;
	for ( size_t i = 0; i < std::vector< LPtr<LFunctionObj> >::size(); i ++ )
	{
		LPtr<LFunctionObj>	pOldFunc = std::vector< LPtr<LFunctionObj> >::at(i) ;
		assert( pOldFunc != nullptr ) ;
		if ( pOldFunc->IsEqualArgmentList( pFunc->GetArgListType() ) )
		{
			std::vector< LPtr<LFunctionObj> >::at(i).SetPtr( pFunc ) ;
			pFunc->SetVariationIndex( i ) ;
			return	pOldFunc ;
		}
	}
	pFunc->SetVariationIndex( std::vector< LPtr<LFunctionObj> >::size() ) ;
	std::vector< LPtr<LFunctionObj> >::push_back( LPtr<LFunctionObj>(pFunc) ) ;
	return	LPtr<LFunctionObj>() ;
}

// 適合関数取得
// （※返却されたポインタを ReleaseRef してはならない）
LPtr<LFunctionObj> LFunctionVariation::GetCallableFunction
					( const LArgumentListType& argListType ) const
{
	for ( size_t i = 0; i < std::vector< LPtr<LFunctionObj> >::size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc = std::vector< LPtr<LFunctionObj> >::at(i) ;
		assert( pFunc != nullptr ) ;
		if ( pFunc->DoesMatchArgmentListWith( argListType ) )
		{
			return	pFunc ;
		}
	}
	return	LPtr<LFunctionObj>() ;
}

// 関数名取得
LString LFunctionVariation::GetFunctionName( void ) const
{
	if ( std::vector< LPtr<LFunctionObj> >::size() >= 1 )
	{
		assert( std::vector< LPtr<LFunctionObj> >::at(0) != nullptr ) ;
		return	std::vector< LPtr<LFunctionObj> >::at(0)->GetName() ;
	}
	return	LString() ;
}



//////////////////////////////////////////////////////////////////////////////
// 仮想関数ベクタ
//////////////////////////////////////////////////////////////////////////////

// 関数追加（追加された関数インデックスを返す）
size_t LVirtualFuncVector::AddFunction
		( const wchar_t * pwszName, LPtr<LFunctionObj> pFunc )
{
	pFunc->SetName( pwszName ) ;

	size_t	iFunc = std::vector< LPtr<LFunctionObj> >::size() ;
	std::vector< LPtr<LFunctionObj> >::push_back( pFunc ) ;
	auto	iter = m_mapIndexByName.find( pwszName ) ;
	if ( iter != m_mapIndexByName.end() )
	{
		iter->second.push_back( iFunc ) ;
	}
	else
	{
		m_mapIndexByName.insert
			( std::make_pair< std::wstring, std::vector<size_t> >
						( pwszName, std::vector<size_t>{iFunc} ) ) ;
	}
	return	iFunc ;
}

// 関数追加、またはオーバーライド
// （返り値は追加された関数インデックスと、オーバーライドされた以前の関数）
std::tuple< size_t, LPtr<LFunctionObj> >
	LVirtualFuncVector::OverrideFunction
		( const wchar_t * pwszName, LPtr<LFunctionObj> pFunc,
			const LPrototype * pAsProto, bool mustBeOverride )
{
	pFunc->SetName( pwszName ) ;

	auto	iter = m_mapIndexByName.find( pwszName ) ;
	if ( iter != m_mapIndexByName.end() )
	{
		// 既に同じ名前の関数が存在するのでオーバーライド判定
		if ( pAsProto == nullptr )
		{
			assert( pFunc->GetPrototype() != nullptr ) ;
			pAsProto = pFunc->GetPrototype().get() ;
		}
		for ( size_t i = 0; i < iter->second.size(); i ++ )
		{
			size_t	iFunc = iter->second.at(i) ;

			LPtr<LFunctionObj>	pEntry =
					std::vector< LPtr<LFunctionObj> >::at( iFunc ) ;
			assert( pAsProto != nullptr ) ;
			if ( pEntry->IsEqualPrototype( *pAsProto ) )
			{
				// オーバーライド
				std::vector< LPtr<LFunctionObj> >::at( iFunc ) = pFunc ;
				if ( !mustBeOverride )
				{
					pFunc->SetVariationIndex( i ) ;
				}
				return	std::make_tuple( iFunc, pEntry ) ;
			}
		}

		// 引数だけ一致する同名の関数が存在する場合、不可視関数に設定する
		for ( size_t i = 0; i < iter->second.size(); i ++ )
		{
			size_t	iFunc = iter->second.at(i) ;

			LPtr<LFunctionObj>	pEntry =
					std::vector< LPtr<LFunctionObj> >::at( iFunc ) ;
			assert( pAsProto != nullptr ) ;
			if ( pEntry->IsEqualArgmentList( pAsProto->GetArgListType() )
				&& (pEntry->IsConstThis() == pAsProto->IsConstThis()) )
			{
				pEntry.SetPtr( new LFunctionObj( *pEntry ) ) ;
				pEntry->MakeInvisible() ;
				std::vector< LPtr<LFunctionObj> >::at( iFunc ) = pEntry ;
			}
		}

		if ( !mustBeOverride )
		{
			// 追加
			size_t	iFunc = std::vector< LPtr<LFunctionObj> >::size() ;
			pFunc->SetVariationIndex( iter->second.size() ) ;
			std::vector< LPtr<LFunctionObj> >::push_back( pFunc ) ;
			iter->second.push_back( iFunc ) ;
			return	std::make_tuple( iFunc, LPtr<LFunctionObj>() ) ;
		}
	}
	else if ( !mustBeOverride )
	{
		// 同じ名前の関数はないので追加
		size_t	iFunc = std::vector< LPtr<LFunctionObj> >::size() ;
		std::vector< LPtr<LFunctionObj> >::push_back( pFunc ) ;
		//
		m_mapIndexByName.insert
			( std::make_pair< std::wstring, std::vector<size_t> >
						( pwszName, std::vector<size_t>{iFunc} ) ) ;
		return	std::make_tuple( iFunc, LPtr<LFunctionObj>() ) ;
	}

	return	std::make_tuple
			( std::vector< LPtr<LFunctionObj> >::size(), LPtr<LFunctionObj>() ) ;
}

// 関数検索
const std::vector<size_t> *
	LVirtualFuncVector::FindFunction( const wchar_t * pwszName ) const
{
	auto	iter = m_mapIndexByName.find( pwszName ) ;
	if ( iter == m_mapIndexByName.end() )
	{
		return	nullptr ;
	}
	return	&(iter->second) ;
}

ssize_t LVirtualFuncVector::FindFunction
	( const wchar_t * pwszName,
		const LArgumentListType& argListType,
		LClass * pThisClass, LType::AccessModifier accScope ) const
{
	const std::vector<size_t> *	pFuncVec = FindFunction( pwszName ) ;
	if ( pFuncVec == nullptr )
	{
		return	-1 ;
	}
	for ( size_t i = 0; i < pFuncVec->size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc =
				std::vector< LPtr<LFunctionObj> >::at( pFuncVec->at(i) ) ;
		if ( pFunc->IsEnableAccess( accScope )
			&& pFunc->IsEqualArgmentList( argListType ) )
		{
			if ( (pThisClass != nullptr)
				&& (pFunc->GetThisClass() != pThisClass) )
			{
				continue ;
			}
			return	(ssize_t) pFuncVec->at(i) ;
		}
	}
	return	-1 ;
}

ssize_t LVirtualFuncVector::FindCallableFunction
	( const wchar_t * pwszName,
		const LArgumentListType& argListType,
		LClass * pThisClass, LType::AccessModifier accScope ) const
{
	const std::vector<size_t> *	pFuncVec = FindFunction( pwszName ) ;
	if ( pFuncVec == nullptr )
	{
		return	-1 ;
	}
	return	FindCallableFunction
				( pFuncVec, argListType, pThisClass, accScope ) ;
}

ssize_t LVirtualFuncVector::FindCallableFunction
	( const std::vector<size_t> * pFuncs,
		const LArgumentListType& argListType,
		LClass * pThisClass, LType::AccessModifier accScope ) const
{
	ssize_t	iCallable = -1 ;
	for ( size_t i = 0; i < pFuncs->size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc =
				std::vector< LPtr<LFunctionObj> >::at( pFuncs->at(i) ) ;
		if ( (pThisClass != nullptr)
			&& (pFunc->GetThisClass() != pThisClass) )
		{
			continue ;
		}
		if ( pFunc->IsEnableAccess( accScope ) )
		{
			if ( pFunc->IsEqualArgmentList( argListType ) )
			{
				return	(ssize_t) pFuncs->at(i) ;
			}
			else if ( pFunc->DoesMatchArgmentListWith( argListType ) )
			{
				iCallable = (iCallable < 0) ? pFuncs->at(i) : iCallable ;
			}
		}
	}
	if ( (iCallable < 0) && (pThisClass != nullptr) )
	{
		if ( pThisClass->GetSuperClass() != nullptr )
		{
			iCallable = FindCallableFunction
				( pFuncs, argListType, pThisClass->GetSuperClass(), accScope ) ;
			if ( iCallable >= 0 )
			{
				return	iCallable ;
			}
		}
		for ( LClass * pImplClass : pThisClass->GetImplementClasses() )
		{
			iCallable = FindCallableFunction
							( pFuncs, argListType, pImplClass, accScope ) ;
			if ( iCallable >= 0 )
			{
				return	iCallable ;
			}
		}
		return	FindCallableFunction
					( pFuncs, argListType, nullptr, accScope ) ;
	}
	return	iCallable ;
}

// private 関数を不可視関数に変更する
void LVirtualFuncVector::MakePrivateInvisible( void )
{
	for ( size_t i = 0; i < std::vector< LPtr<LFunctionObj> >::size(); i ++ )
	{
		LPtr<LFunctionObj>	pFunc = std::vector< LPtr<LFunctionObj> >::at(i) ;
		assert( pFunc != nullptr ) ;
		if ( pFunc != nullptr )
		{
			// private メンバは不可視に変更する
			pFunc->MakePrivateInvisible() ;
		}
	}
}

// 関数取得（解放不要ポインタ）
LPtr<LFunctionObj> LVirtualFuncVector::GetFunctionAt( size_t index ) const
{
	if ( index < std::vector< LPtr<LFunctionObj> >::size() )
	{
		return	std::vector< LPtr<LFunctionObj> >::at( index ) ;
	}
	return	LPtr<LFunctionObj>() ;
}

// 関数候補リストを関数リストへ変換
std::shared_ptr<LFunctionVariation>
		LVirtualFuncVector::MakeFuncVariationOf
			( const std::vector<size_t> * pVirtFuncs ) const
{
	std::shared_ptr<LFunctionVariation>
			pFuncVar = std::make_shared<LFunctionVariation>() ;

	if ( pVirtFuncs != nullptr )
	{
		for ( size_t i = 0; i < pVirtFuncs->size(); i ++ )
		{
			LPtr<LFunctionObj>	pFunc = GetFunctionAt( pVirtFuncs->at(i) ) ;
			if ( pFunc != nullptr )
			{
				pFuncVar->push_back( pFunc ) ;
			}
		}
	}
	return	pFuncVar ;
}

// 関数名リストを取得
std::shared_ptr< std::vector<LString> >
		LVirtualFuncVector::GetFuncNameList( void ) const
{
	std::shared_ptr< std::vector<LString> >
			pList = std::make_shared< std::vector<LString> >() ;
	for ( auto iter = m_mapIndexByName.begin();
				iter != m_mapIndexByName.end(); iter ++ )
	{
		pList->push_back( iter->first ) ;
	}
	return	pList ;
}

// 関数名取得
LString LVirtualFuncVector::GetFunctionNameOf
				( const std::vector<size_t> * pFuncs ) const
{
	if ( (pFuncs != nullptr) && (pFuncs->size() >= 1) )
	{
		assert( GetFunctionAt( pFuncs->at(0) ) != nullptr ) ;
		return	GetFunctionAt( pFuncs->at(0) )->GetName() ;
	}
	return	LString() ;
}



//////////////////////////////////////////////////////////////////////////////
// 関数の引数を受け取り用ランタイム・ヘルパー
//////////////////////////////////////////////////////////////////////////////

LStackBuffer::Word LRuntimeArgList::NextPrimitive( void )
{
	return	m_context.GetArgAt( m_next ++ ) ;
}

LBoolean LRuntimeArgList::NextBoolean( void )
{
	return	m_context.GetArgAt( m_next ++ ).boolValue ;
}

LInt LRuntimeArgList::NextInt( void )
{
	return	(LInt) m_context.GetArgAt( m_next ++ ).longValue ;
}

LLong LRuntimeArgList::NextLong( void )
{
	return	m_context.GetArgAt( m_next ++ ).longValue ;
}

LFloat LRuntimeArgList::NextFloat( void )
{
	return	(LFloat) m_context.GetArgAt( m_next ++ ).dblValue ;
}

LDouble LRuntimeArgList::NextDouble( void )
{
	return	m_context.GetArgAt( m_next ++ ).dblValue ;
}

LString LRuntimeArgList::NextString( void )
{
	LObject *	pObj = m_context.GetArgAt( m_next ++ ).pObject ;
	if ( pObj != nullptr )
	{
		LString	str ;
		if ( pObj->AsString( str ) )
		{
			return	str ;
		}
	}
	return	LString() ;
}

LObjPtr LRuntimeArgList::NextObject( void )
{
	return	LObjPtr( LObject::AddRef( m_context.GetArgAt( m_next ++ ).pObject ) ) ;
}

std::shared_ptr<Object> LRuntimeArgList::NextNativeObject( void )
{
	LObjPtr	pObj = NextObject() ;
	if ( pObj != nullptr )
	{
		m_objPool.push_back( pObj ) ;
		//
		return	LNativeObj::GetNativeObject( pObj.Ptr() ) ;
	}
	return	nullptr ;
}

std::uint8_t * LRuntimeArgList::NextPointer( size_t nBytes )
{
	LObjPtr	pObj = NextObject() ;
	if ( pObj != nullptr )
	{
		m_objPool.push_back( pObj ) ;
		//
		LPtr<LPointerObj>	pPtrObj( pObj->GetBufferPoiner() ) ;
		if ( pPtrObj != nullptr )
		{
			m_objPool.push_back( pPtrObj ) ;
			return	pPtrObj->GetPointer( 0, nBytes ) ;
		}
	}
	return	nullptr ;
}

void LRuntimeArgList::SetNativeObject( size_t index, std::shared_ptr<Object> pObj )
{
	LNativeObj::SetNativeObject( m_context.GetArgAt( index ).pObject, pObj ) ;
}

