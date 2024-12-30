
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////
// 時刻表現
//////////////////////////////////////////////////////////////////////////

// 累積日数（紀元元年～）
std::int64_t LDateTime::GetNumberOfDays( void ) const
{
	return	NumberOfDays( year, month, day ) ;
}

std::int64_t LDateTime::NumberOfDays( int nYear, int nMonth, int nDay )
{
	std::int64_t	numDays = (nYear - 1) * 365 ;
	numDays += ((nYear - 1) / 4)
					- ((nYear - 1) / 100) + ((nYear - 1) / 400) ;
	for ( int m = 1; m < nMonth; m ++ )
	{
		numDays += DaysOfMonth( nYear, m ) ;
	}
	return	numDays + nDay - 1 ;
}

void LDateTime::SetNumberOfDays( std::int64_t nDays )
{
	const std::uint32_t	daysOf400Years = 365 * 400 + 97 ;
	const std::uint32_t	daysOf100Years = 365 * 100 + 24 ;
	const std::uint32_t	daysOf4Years = 365 * 4 + 1 ;
	const std::uint32_t	daysOf1Year = 365 ;
	const std::int64_t	daysTotal = nDays ;
	std::uint32_t	year400 = (std::uint32_t) (nDays / daysOf400Years) ;
	nDays %= daysOf400Years ;
	std::uint32_t	year100 = (std::uint32_t) (nDays / daysOf100Years) ;
	nDays %= daysOf100Years ;
	std::uint32_t	year4 = (std::uint32_t) (nDays / daysOf4Years) ;
	nDays %= daysOf4Years ;
	std::uint32_t	year1 = (std::uint32_t) (nDays / daysOf1Year) ;
	nDays %= daysOf1Year ;

	year = (LInt16) (year400 * 400 + year100 * 100
								+ year4 * 4 + year1 + 1) ;
	if ( (nDays == 0) && (daysTotal + 1 == NumberOfDays( year, 1, 1 )) )
	{
		year -- ;
		nDays += daysOf1Year ;
	}
	month = 0 ;
	for ( int m = 1; m <= 12; m ++ )
	{
		int	daysOfMonth = DaysOfMonth( year, m ) ;
		if ( nDays < daysOfMonth )
		{
			month = (LUint8) m ;
			break ;
		}
		nDays -= daysOfMonth ;
	}
	day = (LUint8) (nDays + 1) ;
	week = (LUint8) DayOfWeek( year, month, day ) ;
}

// 閏年判定
bool LDateTime::IsLeapYear( int nYear )
{
	return	((nYear % 4) == 0)
				&& (((nYear % 100) != 0)
					|| ((nYear % 400) == 0)) ;
}

// 月の日数
int LDateTime::DaysOfMonth( int nYear, int nMonth )
{
	static const int	s_dayOfMonth[12] =
	{
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	} ;
	if ( (nMonth < 1) || (nMonth > 12) )
	{
		return	0 ;
	}
	if ( nMonth != 2 )
	{
		return	s_dayOfMonth[nMonth - 1] ;
	}
	if ( IsLeapYear( nYear ) )
	{
		return	29 ;
	}
	return	28 ;
}

int LDateTime::DayOfWeek( int nYear, int nMonth, int nDay )
{
	return	(NumberOfDays( nYear, nMonth, nDay ) + 1) % 7 ;
}

// 累積μ秒（紀元元年～）
std::int64_t LDateTime::GetMicrosecond( void ) const
{
	const std::int64_t	usecOf1Sec  = 1000 * 1000 ;

	std::int64_t	nDays = GetNumberOfDays() ;
	return	GetSecond() * usecOf1Sec
				+ (int) msec * 1000
				+ (int) usec ;
}

void LDateTime::SetMicrosecond( std::int64_t usec )
{
	const std::int64_t	usecOf1Sec  = 1000 * 1000 ;
	SetSecond( usec / usecOf1Sec ) ;

	std::int64_t	nTemp = usec % usecOf1Sec ;
	usec = (LUint16) (nTemp % 1000) ;
	msec = (LUint16) (nTemp % 1000) ;
}

// 累積秒（紀元元年～）
std::int64_t LDateTime::GetSecond( void ) const
{
	const std::int64_t	usecOf1Min  = 60 ;
	const std::int64_t	usecOf1Hour = 60 * usecOf1Min ;
	const std::int64_t	usecOf1Day  = 24 * usecOf1Hour ;

	std::int64_t	nDays = GetNumberOfDays() ;
	return	nDays * usecOf1Day
				+ hour * usecOf1Hour
				+ minute * usecOf1Min
				+ second ;
}

void LDateTime::SetSecond( std::int64_t sec )
{
	const std::int64_t	usecOf1Min  = 60 ;
	const std::int64_t	usecOf1Hour = 60 * usecOf1Min ;
	const std::int64_t	usecOf1Day  = 24 * usecOf1Hour ;

	std::int64_t	nDays = sec / usecOf1Day ;
	std::int64_t	nTemp = sec % usecOf1Day ;

	second = (LUint8) (nTemp % 60) ;
	nTemp /= 60 ;

	minute = (LUint8) (nTemp % 60) ;
	hour = (LUint8) (nTemp / 60) ;

	SetNumberOfDays( nDays ) ;
}

// ローカル時間取得
bool LDateTime::GetLocalTime( void )
{
	std::timespec	ts ;
	if ( std::timespec_get( &ts, TIME_UTC ) == 0 )
	{
		return	false ;
	}

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	std::tm		tmLocal ;
	localtime_s( &tmLocal, &(ts.tv_sec) ) ;

	const std::tm *	ptmLocal = &tmLocal ;
#else
	const std::tm *	ptmLocal = std::localtime( &(ts.tv_sec) ) ;

#endif

	SetTm( ptmLocal ) ;
	msec = (LUint16) (ts.tv_nsec / 1000000) ;
	usec = (LUint16) ((ts.tv_nsec / 1000) % 1000) ;

	return	true ;
}

// グリニッチ標準時間取得
bool LDateTime::GetGMTime( void )
{
	std::timespec	ts ;
	if ( std::timespec_get( &ts, TIME_UTC ) == 0 )
	{
		return	false ;
	}

#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	std::tm		tmGM ;
	gmtime_s( &tmGM, &(ts.tv_sec) ) ;

	const std::tm *	ptmGM = &tmGM ;

#else
	const std::tm *	ptmGM = std::gmtime( &(ts.tv_sec) ) ;

#endif


	SetTm( ptmGM ) ;
	msec = (LUint16) (ts.tv_nsec / 1000000) ;
	usec = (LUint16) ((ts.tv_nsec / 1000) % 1000) ;

	return	true ;
}

// std::tm を設定
void LDateTime::SetTm( const std::tm * ptm )
{
	assert( ptm != nullptr ) ;
	if ( ptm != nullptr )
	{
		year = (LUint16) (ptm->tm_year + 1900) ;
		month = (LUint8) (ptm->tm_mon + 1) ;
		day = (LUint8) (ptm->tm_mday) ;
		week = (LUint8) (ptm->tm_wday) ;
		hour = (LUint8) (ptm->tm_hour) ;
		minute = (LUint8) (ptm->tm_min) ;
		second = (LUint8) (ptm->tm_sec) ;
		msec = 0 ;
		usec = 0 ;
	}
}

// 時差（秒単位）
long int LDateTime::GetTimeZone( void )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	TIME_ZONE_INFORMATION	tzi ;
	memset( &tzi, 0, sizeof(tzi) ) ;

	DWORD	dwResult = GetTimeZoneInformation( &tzi ) ;
	if ( (dwResult == TIME_ZONE_ID_UNKNOWN)
		|| (dwResult == TIME_ZONE_ID_STANDARD) )
	{
		return	tzi.Bias * 60 ;
	}
	else if ( dwResult == TIME_ZONE_ID_DAYLIGHT )
	{
		return	(tzi.Bias + tzi.DaylightBias) * 60 ;
	}
	return	0 ;

#else
	tzset() ;
	return	timezone ;

#endif
}

// タイムゾーン名
LString LDateTime::GetTimeZoneName( void )
{
#if	defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

	TIME_ZONE_INFORMATION	tzi ;
	memset( &tzi, 0, sizeof(tzi) ) ;

	DWORD	dwResult = GetTimeZoneInformation( &tzi ) ;
	if ( (dwResult == TIME_ZONE_ID_UNKNOWN)
		|| (dwResult == TIME_ZONE_ID_STANDARD) )
	{
		return	LString(tzi.StandardName) ;
	}
	else if ( dwResult == TIME_ZONE_ID_DAYLIGHT )
	{
		return	LString(tzi.DaylightName) ;
	}
	return	LString() ;

#else
	tzset() ;
	return	LString( tzname[0] ) ;

#endif
}

