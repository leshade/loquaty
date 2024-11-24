
#ifndef	__LOQUATY_DATE_TIME_H__
#define	__LOQUATY_DATE_TIME_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 時刻表現
	//////////////////////////////////////////////////////////////////////////

	struct	LDateTime
	{
		LUint16	year ;		// 1-
		LUint8	month ;		// [1, 12]
		LUint8	day ;		// [1, 31]
		LUint8	week ;		// [0, 6]   0:Sunday, 1:Monday, ...
		LUint8	hour ;		// [0, 23]
		LUint8	minute ;	// [0, 59]
		LUint8	second ;	// [0, 60]  (60: leap second)
		LUint16	msec ;		// [0, 999]  millisecond
		LUint16	usec ;		// [0, 999]  microsecond

		enum	WeekOfDay
		{
			Sunday, Monday, Tuesday, Wednesday,
			Thursday, Friday, Saturday,
		} ;

		// 累積日数（紀元元年～）
		std::int64_t GetNumberOfDays( void ) const ;
		static std::int64_t NumberOfDays( int nYear, int nMonth, int nDay ) ;

		void SetNumberOfDays( std::int64_t nDays ) ;

		// 閏年判定
		static bool IsLeapYear( int nYear ) ;

		// 月の日数
		static int DaysOfMonth( int nYear, int nMonth ) ;

		// 曜日
		static int DayOfWeek( int nYear, int nMonth, int nDay ) ;

		// 累積μ秒（紀元元年～）
		std::int64_t GetMicrosecond( void ) const ;
		void SetMicrosecond( std::int64_t usec ) ;

		// 累積秒（紀元元年～）
		std::int64_t GetSecond( void ) const ;
		void SetSecond( std::int64_t sec ) ;

		// ローカル時間取得
		bool GetLocalTime( void ) ;

		// グリニッチ標準時間取得
		bool GetGMTime( void ) ;

		// std::tm を設定
		void SetTm( const std::tm * ptm ) ;

		// 時差（秒単位）
		static long int GetTimeZone( void ) ;

		// タイムゾーン名
		static LString GetTimeZoneName( void ) ;
	} ;

}

#endif

