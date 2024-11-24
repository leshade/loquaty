
#ifndef	__LOQUATY_CLS_MATH_H__
#define	__LOQUATY_CLS_MATH_H__	1

namespace	Loquaty
{

	//////////////////////////////////////////////////////////////////////////
	// 算術関数クラス
	//////////////////////////////////////////////////////////////////////////

	class	LMathClass	: public LClass
	{
	public:
		LMathClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName )
			: LClass( vm, vm.Global(), pClass, pwszName ) { }

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Functions[24] ;
		static const VariableDesc		s_Variables[3] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// static double sqrt( double x )
		static void method_sqrt( LContext& context ) ;
		// static double pow( double x, double y )
		static void method_pow( LContext& context ) ;
		// static double exp( double x )
		static void method_exp( LContext& context ) ;
		// static double log( double x )
		static void method_log( LContext& context ) ;
		// static double log10( double x )
		static void method_log10( LContext& context ) ;
		// static double sin( double x )
		static void method_sin( LContext& context ) ;
		// static double cos( double x )
		static void method_cos( LContext& context ) ;
		// static double tan( double x )
		static void method_tan( LContext& context ) ;
		// static double asin( double x )
		static void method_asin( LContext& context ) ;
		// static double acos( double x )
		static void method_acos( LContext& context ) ;
		// static double atan( double x )
		static void method_atan( LContext& context ) ;
		// static double atan2( double y, double x )
		static void method_atan2( LContext& context ) ;
		// static double floor( double x )
		static void method_floor( LContext& context ) ;
		// static double round( double x )
		static void method_round( LContext& context ) ;
		// static long abs( long x )
		static void method_iabs( LContext& context ) ;
		// static double abs( double x )
		static void method_fabs( LContext& context ) ;
		// static long max( long x, long y )
		static void method_imax( LContext& context ) ;
		// static double max( double x, double y )
		static void method_fmax( LContext& context ) ;
		// static long min( long x, long y )
		static void method_imin( LContext& context ) ;
		// static double min( double x, double y )
		static void method_fmin( LContext& context ) ;
		// static long clamp( long x, long l, long h )
		static void method_iclamp( LContext& context ) ;
		// static double clamp( double x, double l, double h )
		static void method_fclamp( LContext& context ) ;
		// static double random( double x )
		static void method_random( LContext& context ) ;

	} ;


}

#endif

