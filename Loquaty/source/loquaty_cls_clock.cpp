
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////
// システム時計
//////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LClockClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LClockClass::s_Functions[8] =
{
	{	// public static double sec()
		L"sec",
		&LClockClass::method_sec, false,
		L"public static", L"double", L"",
		L"システム・クロックを取得します。\n"
		L"<return>秒単位のクロック</return>", nullptr
	},
	{	// public static long msec()
		L"msec",
		&LClockClass::method_msec, false,
		L"public static", L"long", L"",
		L"システム・クロックを取得します。\n"
		L"<return>ミリ秒単位のクロック</return>", nullptr
	},
	{	// public static long usec()
		L"usec",
		&LClockClass::method_usec, false,
		L"public static", L"long", L"",
		L"システム・クロックを取得します。\n"
		L"<return>マイクロ秒単位のクロック</return>", nullptr
	},
	{	// public static DateTime* localDate()
		L"localDate",
		&LClockClass::method_localDate, false,
		L"public static", L"DateTime*", L"",
		L"現在の時刻をローカルのタイムゾーンで取得します。\n"
		L"<return>ローカルのタイムゾーンでの現時刻</return>", nullptr
	},
	{	// public static DateTime* utcDate()
		L"utcDate",
		&LClockClass::method_utcDate, false,
		L"public static", L"DateTime*", L"",
		L"現在の時刻を UTC で取得します。\n"
		L"<return>UTC での現時刻</return>", nullptr
	},
	{	// public static long timeZone()
		L"timeZone",
		&LClockClass::method_timeZone, false,
		L"public static", L"long", L"",
		L"ローカルのタイムゾーンと UTC の時差を秒単位で取得します。\n"
		L"Clock.localDate() + (double) Clock.timeZone() == Clock.utcDate() の関係が成立します。\n"
		L"※実際に実行すると関数の実行に要する時間差によって厳密には == が成立しません。", nullptr
	},
	{	// public static String timeZoneName()
		L"timeZoneName",
		&LClockClass::method_timeZoneName, false,
		L"public static", L"String", L"",
		L"タイムゾーンの名前を取得します。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::ClassMemberDesc		LClockClass::s_MemberDesc =
{
	nullptr,
	LClockClass::s_Functions,
	nullptr,
	nullptr,
	L"Clock はシステム・クロックを取得するクラスです。\n"
	L"このクラスのクロックは DateTime のエポック時間とは互換性がありません。"
	L"経過時間を測定することに利用できます。",
} ;

// static double sec()
void LClockClass::method_sec( LContext& _context )
{
	std::timespec	ts ;
	if ( std::timespec_get( &ts, TIME_UTC ) == 0 )
	{
		_context.ThrowException( exceptionClockError ) ;
		LQT_RETURN_LONG( 0 ) ;
	}
	LQT_RETURN_DOUBLE( ts.tv_sec + (double) ts.tv_nsec / 1000000000.0 ) ;
}

// static long msec()
void LClockClass::method_msec( LContext& _context )
{
	std::timespec	ts ;
	if ( std::timespec_get( &ts, TIME_UTC ) == 0 )
	{
		_context.ThrowException( exceptionClockError ) ;
		LQT_RETURN_LONG( 0 ) ;
	}
	LQT_RETURN_LONG( (LLong) ts.tv_sec * 1000 + (ts.tv_nsec / 1000000) ) ;
}

// static long usec()
void LClockClass::method_usec( LContext& _context )
{
	std::timespec	ts ;
	if ( std::timespec_get( &ts, TIME_UTC ) == 0 )
	{
		_context.ThrowException( exceptionClockError ) ;
		LQT_RETURN_LONG( 0 ) ;
	}
	LQT_RETURN_LONG( (LLong) ts.tv_sec * 1000000 + (ts.tv_nsec / 1000) ) ;
}

// static DateTime* localDate()
void LClockClass::method_localDate( LContext& _context )
{
	LDateTime	date ;
	if ( !date.GetLocalTime() )
	{
		_context.ThrowException( exceptionClockError ) ;
		LQT_RETURN_OBJECT( nullptr ) ;
	}

	LQT_RETURN_POINTER_STRUCT( date ) ;
}

// static DateTime* utcDate()
void LClockClass::method_utcDate( LContext& _context )
{
	LDateTime	date ;
	if ( !date.GetGMTime() )
	{
		_context.ThrowException( exceptionClockError ) ;
		LQT_RETURN_OBJECT( nullptr ) ;
	}

	LQT_RETURN_POINTER_STRUCT( date ) ;
}

// static long timeZone()
void LClockClass::method_timeZone( LContext& _context )
{
	LQT_RETURN_LONG( LDateTime::GetTimeZone() ) ;
}

// static String timeZoneName()
void LClockClass::method_timeZoneName( LContext& _context )
{
	LQT_RETURN_STRING( LDateTime::GetTimeZoneName() ) ;
}



//////////////////////////////////////////////////////////////////////////
// 時刻表現
//////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LDateTimeStructure::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc		LDateTimeStructure::s_Virtuals[8] =
{
	{	// public void DateTime()
		LClass::s_Constructor,
		&LDateTimeStructure::method_init, false,
		L"public", L"void", L"",
		L"DateTime 構造体を現在のローカル時間で初期化して構築します。", nullptr
	},
	{	// public void DateTime( long usec )
		LClass::s_Constructor,
		&LDateTimeStructure::method_init1, true,
		L"public", L"void", L"long usec",
		L"DateTime 構造体を紀元元年を基準とする時間で初期化して構築します。\n"
		L"<param name=\"usec\">紀元元年を基準とするマイクロ秒</param>", nullptr
	},
	{	// public void DateTime( double sec )
		LClass::s_Constructor,
		&LDateTimeStructure::method_init2, true,
		L"public", L"void", L"double sec",
		L"DateTime 構造体を紀元元年を基準とする時間で初期化して構築します。\n"
		L"<param name=\"sec\">紀元元年を基準とする秒</param>", nullptr
	},
	{	// public long usec() const
		L"usec",
		&LDateTimeStructure::method_usec, true,
		L"public const", L"long", L"",
		L"紀元元年を基準とする時間を取得します。\n"
		L"<return>紀元元年を基準とするマイクロ秒</return>", nullptr
	},
	{	// public double sec() const
		L"sec",
		&LDateTimeStructure::method_sec, true,
		L"public const", L"double", L"",
		L"紀元元年を基準とする時間を取得します。\n"
		L"<return>紀元元年を基準とする秒</return>", nullptr
	},
	{	// public long days() const
		L"days",
		&LDateTimeStructure::method_days, true,
		L"public const", L"long", L"",
		L"紀元元年を基準とする日数を取得します。\n"
		L"<return>紀元元年を基準とする日数</return>", nullptr
	},
	{	// public String format( String form ) const
		L"format",
		&LDateTimeStructure::method_format, true,
		L"public const", L"String", L"String form",
		L"日時・時刻を文字列化します。\n"
		L"<param name=\"form\">日時・時刻の書式。ex.&quot;Y/M/D h:m:s&quot;<br/>"
		L"<div class=\"indent1\">Y:四桁の年<br/>"
		L"M:二桁の月<br/>D:二桁の日<br/>W:Sunday, Monday,...<br/>w:Sun, Mon,...<br/>"
		L"h:二桁の時間（24時間）<br/>m:二桁の分<br/>s:二桁の秒<br/>"
		L"c:二桁の百分の１秒<br/>x:三桁のミリ秒</div></param>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::VariableDesc			LDateTimeStructure::s_Variables[17] =
{
	{
		L"year", L"public", L"int16", nullptr, nullptr, L"西暦年（1～）",
	},
	{
		L"month", L"public", L"uint8", nullptr, nullptr, L"月（1～12）",
	},
	{
		L"day", L"public", L"uint8", nullptr, nullptr, L"日（1～31）",
	},
	{
		L"week", L"public", L"uint8", nullptr, nullptr, L"曜日（0～6）",
	},
	{
		L"hour", L"public", L"uint8", nullptr, nullptr, L"時間（0～23）",
	},
	{
		L"minute", L"public", L"uint8", nullptr, nullptr, L"分（0～59）",
	},
	{
		L"second", L"public", L"uint8", nullptr, nullptr, L"秒（0～59）",
	},
	{
		L"msec", L"public", L"uint16", nullptr, nullptr, L"ミリ秒（0～999）",
	},
	{
		L"usec", L"public", L"uint16", nullptr, nullptr, L"マイクロ秒（0～999）",
	},
	{
		L"Sunday", L"public static const", L"uint8", L"0", nullptr, L"日曜日",
	},
	{
		L"Monday", L"public static const", L"uint8", L"1", nullptr, L"月曜日",
	},
	{
		L"Tuesday", L"public static const", L"uint8", L"2", nullptr, L"火曜日",
	},
	{
		L"Wednesday", L"public static const", L"uint8", L"3", nullptr, L"水曜日",
	},
	{
		L"Thursday", L"public static const", L"uint8", L"4", nullptr, L"木曜日",
	},
	{
		L"Friday", L"public static const", L"uint8", L"5", nullptr, L"金曜日",
	},
	{
		L"Saturday", L"public static const", L"uint8", L"6", nullptr, L"土曜日",
	},
	{
		nullptr, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::BinaryOperatorDesc	LDateTimeStructure::s_BinaryOps[5] =
{
	{	// DateTime* operator + ( double sec ) const
		L"+", &LDateTimeStructure::operator_add, true, true,
		L"DateTime*", L"double sec",
		L"時刻に秒を加算した新しい DateTime を作成して返します。\n"
		L"整数の場合、ポインタの加算になってしまうことに注意してください。", nullptr,
	},
	{	// DateTime* operator - ( double sec ) const
		L"-", &LDateTimeStructure::operator_sub, true, true,
		L"DateTime*", L"double sec",
		L"時刻に秒を減算した新しい DateTime を作成して返します。\n"
		L"整数の場合、ポインタの減算になってしまうことに注意してください。", nullptr,
	},
	{	// DateTime* operator += ( double sec )
		L"+=", &LDateTimeStructure::operator_sadd, true, false,
		L"DateTime*", L"double sec",
		L"時刻に秒を加算します。\n"
		L"整数の場合、ポインタの加算になってしまうことに注意してください。", nullptr,
	},
	{	// DateTime* operator -= ( double sec )
		L"-=", &LDateTimeStructure::operator_ssub, true, false,
		L"DateTime*", L"double sec",
		L"時刻に秒を減算します。\n"
		L"整数の場合、ポインタの減算になってしまうことに注意してください。", nullptr,
	},
	{
		nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc		LDateTimeStructure::s_MemberDesc =
{
	LDateTimeStructure::s_Virtuals,
	nullptr,
	LDateTimeStructure::s_Variables,
	LDateTimeStructure::s_BinaryOps,
	L"DateTime は日時を保持する構造体です。\n"
	L"紀元元年を基準とする時間に変換する機能を有していますが、"
	L"この実装は独自なのでシステムに依存しません。但し閏年は考慮しますが閏秒は考慮されません。\n"
	L"また Clock クラスのクロックと互換性がないことに注意してください。",
} ;

// void DataTime()
void LDateTimeStructure::method_init( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;

	pdt->GetLocalTime() ;

	LQT_RETURN_VOID() ;
}

// void DataTime( long usec )
void LDateTimeStructure::method_init1( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;
	LQT_FUNC_ARG_LONG( usec ) ;

	pdt->SetMicrosecond( usec ) ;

	LQT_RETURN_VOID() ;
}

// void DataTime( double sec )
void LDateTimeStructure::method_init2( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;
	LQT_FUNC_ARG_DOUBLE( sec ) ;

	pdt->SetMicrosecond( (std::int64_t) (sec * (1000.0 * 1000.0)) ) ;

	LQT_RETURN_VOID() ;
}

// long usec() const
void LDateTimeStructure::method_usec( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;

	LQT_RETURN_LONG( pdt->GetMicrosecond() ) ;
}

// double sec() const
void LDateTimeStructure::method_sec( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;

	LQT_RETURN_DOUBLE( (double) pdt->GetMicrosecond() / (1000.0 * 1000.0) ) ;
}

// long days() const
void LDateTimeStructure::method_days( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;

	LQT_RETURN_LONG( pdt->GetNumberOfDays() ) ;
}

// String format( String form ) const
void LDateTimeStructure::method_format( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_POINTER( LDateTime, pdt ) ;
	LQT_FUNC_ARG_STRING( strForm ) ;

	static const wchar_t *	s_pwszDayOfWeek[] =
	{
		L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
		L"Thursday", L"Friday", L"Saturday",
	} ;
	static const wchar_t *	s_pwszLeadDayOfWeek[] =
	{
		L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat",
	} ;

	std::map<LString,LString>	mapForm ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"Y"), LString::IntegerOf(pdt->year,4) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"M"), LString::IntegerOf(pdt->month,2) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"D"), LString::IntegerOf(pdt->day,2) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"W"), LString(s_pwszDayOfWeek[pdt->week]) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"w"), LString(s_pwszLeadDayOfWeek[pdt->week]) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"h"), LString::IntegerOf(pdt->hour,2) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"m"), LString::IntegerOf(pdt->minute,2) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"s"), LString::IntegerOf(pdt->second,2) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"c"), LString::IntegerOf(pdt->msec/10,2) ) ) ;
	mapForm.insert
		( std::make_pair<LString,LString>
			( LString(L"x"), LString::IntegerOf(pdt->msec,3) ) ) ;

	LString	strFormatted =
		strForm.Replace
			( []( LStringParser& spars )
				{
					if ( LStringParser::IsCharContained
								( spars.CurrentChar(), L"YMDWwhmscx" ) )
					{
						spars.NextChar() ;
						return	true ;
					}
					return	false ;
				},
			[&mapForm]( const LString& key )
				{
					auto	iter = mapForm.find( key.c_str() ) ;
					if ( iter != mapForm.end() )
					{
						return	iter->second ;
					}
					return	LString() ;
				} ) ;

	LQT_RETURN_STRING( strFormatted ) ;
}

// DateTime* operator + ( double sec ) const
LValue::Primitive LDateTimeStructure::operator_add
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;

	if ( (pContext == nullptr)
		|| (val1.pObject == nullptr) )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}

	LPtr<LPointerObj>	pDatePtr = val1.pObject->GetBufferPoiner() ;
	LDateTime *			pDate = pDatePtr->Ptr<LDateTime>() ;
	if ( pDate == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LDateTime	dateRes ;
	dateRes.SetMicrosecond
		( pDate->GetMicrosecond()
			+ (std::int64_t) (val2.dblValue * (1000.0 * 1000.0)) ) ;

	return	LValue::MakeObjectPtr
				( pContext->new_Pointer( &dateRes, sizeof(dateRes) ) ) ;
}

// DateTime* operator - ( double sec ) const
LValue::Primitive LDateTimeStructure::operator_sub
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;

	if ( (pContext == nullptr)
		|| (val1.pObject == nullptr) )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}

	LPtr<LPointerObj>	pDatePtr = val1.pObject->GetBufferPoiner() ;
	LDateTime *			pDate = pDatePtr->Ptr<LDateTime>() ;
	if ( pDate == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	LDateTime	dateRes ;
	dateRes.SetMicrosecond
		( pDate->GetMicrosecond()
			- (std::int64_t) (val2.dblValue * (1000.0 * 1000.0)) ) ;

	return	LValue::MakeObjectPtr
				( pContext->new_Pointer( &dateRes, sizeof(dateRes) ) ) ;
}

// DateTime* operator += ( double sec ) const
LValue::Primitive LDateTimeStructure::operator_sadd
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;

	if ( (pContext == nullptr)
		|| (val1.pObject == nullptr) )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}

	LPtr<LPointerObj>	pDatePtr = val1.pObject->GetBufferPoiner() ;
	LDateTime *			pDate = pDatePtr->Ptr<LDateTime>() ;
	if ( pDate == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	pDate->SetMicrosecond
		( pDate->GetMicrosecond()
			+ (std::int64_t) (val2.dblValue * (1000.0 * 1000.0)) ) ;

	val1.pObject->AddRef() ;
	return	LValue::MakeObjectPtr( val1.pObject ) ;
}

// DateTime* operator -= ( double sec ) const
LValue::Primitive LDateTimeStructure::operator_ssub
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	LContext *		pContext = LContext::GetCurrent() ;
	assert( pContext != nullptr ) ;

	if ( (pContext == nullptr)
		|| (val1.pObject == nullptr) )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}

	LPtr<LPointerObj>	pDatePtr = val1.pObject->GetBufferPoiner() ;
	LDateTime *			pDate = pDatePtr->Ptr<LDateTime>() ;
	if ( pDate == nullptr )
	{
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	pDate->SetMicrosecond
		( pDate->GetMicrosecond()
			- (std::int64_t) (val2.dblValue * (1000.0 * 1000.0)) ) ;

	val1.pObject->AddRef() ;
	return	LValue::MakeObjectPtr( val1.pObject ) ;
}




