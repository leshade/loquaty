
#ifndef	__LOQUATY_CLS_CLOCK_H__
#define	__LOQUATY_CLS_CLOCK_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// システム時計
	//////////////////////////////////////////////////////////////////////////

	class	LClockClass	: public LClass
	{
	public:
		LClockClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName )
			: LClass( vm, vm.Global(), pClass, pwszName ) { }

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Functions[8] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// static double sec()
		static void method_sec( LContext& context ) ;
		// static long msec()
		static void method_msec( LContext& context ) ;
		// static long usec()
		static void method_usec( LContext& context ) ;
		// static DateTime* localDate()
		static void method_localDate( LContext& context ) ;
		// static DateTime* utcDate()
		static void method_utcDate( LContext& context ) ;
		// static long timeZone()
		static void method_timeZone( LContext& context ) ;
		// static String timeZone()
		static void method_timeZoneName( LContext& context ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 時刻表現
	//////////////////////////////////////////////////////////////////////////

	class	LDateTimeStructure	: public LStructureClass
	{
	public:
		LDateTimeStructure
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName )
			: LStructureClass( vm, vm.Global(), pClass, pwszName ) { }

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc			s_Virtuals[8] ;
		static const VariableDesc			s_Variables[17] ;
		static const BinaryOperatorDesc		s_BinaryOps[5] ;
		static const ClassMemberDesc		s_MemberDesc ;

	public:
		// void DateTime()
		static void method_init( LContext& context ) ;
		// void DateTime( long usec )
		static void method_init1( LContext& context ) ;
		// void DateTime( double sec )
		static void method_init2( LContext& context ) ;
		// long usec() const
		static void method_usec( LContext& context ) ;
		// double sec() const
		static void method_sec( LContext& context ) ;
		// long days() const
		static void method_days( LContext& context ) ;
		// String format( String form ) const
		static void method_format( LContext& context ) ;

		// DateTime* operator + ( double sec ) const
		static LValue::Primitive operator_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// DateTime* operator - ( double sec ) const
		static LValue::Primitive operator_sub
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// DateTime* operator += ( double sec ) const
		static LValue::Primitive operator_sadd
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// DateTime* operator -= ( double sec ) const
		static LValue::Primitive operator_ssub
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;


}

#endif

