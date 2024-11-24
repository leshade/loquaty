
#include <loquaty.h>
#include <random>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////
// 算術関数クラス
//////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LMathClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc	LMathClass::s_Functions[24] =
{
	{	// public static double sqrt( double x )
		L"sqrt",
		&LMathClass::method_sqrt, true,
		L"public static", L"double", L"double x",
		L"x の平方根を計算します。", nullptr
	},
	{	// public static double pow( double x, double y )
		L"pow",
		&LMathClass::method_pow, true,
		L"public static", L"double", L"double x, double y",
		L"<summary><italic>x<sup>y</sup></italic> を計算します。</summary>", nullptr
	},
	{	// public static double exp( double x )
		L"exp",
		&LMathClass::method_exp, true,
		L"public static", L"double", L"double x",
		L"<summary><italic>e<sup>x</sup></italic> を計算します。</summary>", nullptr
	},
	{	// public static double log( double x )
		L"log",
		&LMathClass::method_log, true,
		L"public static", L"double", L"double x",
		L"<summary>log<italic><sub>e</sub></italic>(x) を計算します。</summary>", nullptr
	},
	{	// public static double log10( double x )
		L"log10",
		&LMathClass::method_log10, true,
		L"public static", L"double", L"double x",
		L"<summary>log<sub>10</sub>(x) を計算します。</summary>", nullptr
	},
	{	// public static double sin( double x )
		L"sin",
		&LMathClass::method_sin, true,
		L"public static", L"double", L"double x",
		L"x の正弦を計算します。", nullptr
	},
	{	// public static double cos( double x )
		L"cos",
		&LMathClass::method_cos, true,
		L"public static", L"double", L"double x",
		L"x の余弦を計算します。", nullptr
	},
	{	// public static double tan( double x )
		L"tan",
		&LMathClass::method_tan, true,
		L"public static", L"double", L"double x",
		L"x の接線を計算します。", nullptr
	},
	{	// public static double asin( double x )
		L"asin",
		&LMathClass::method_asin, true,
		L"public static", L"double", L"double x",
		L"x の逆正弦を計算します。", nullptr
	},
	{	// public static double acos( double x )
		L"acos",
		&LMathClass::method_acos, true,
		L"public static", L"double", L"double x",
		L"x の逆余弦を計算します。", nullptr
	},
	{	// public static double atan( double x )
		L"atan",
		&LMathClass::method_atan, true,
		L"public static", L"double", L"double x",
		L"x の逆接線を計算します。", nullptr
	},
	{	// public static double atan2( double y, double x )
		L"atan2",
		&LMathClass::method_atan2, true,
		L"public static", L"double", L"double y, double x",
		L"y/x の逆接線を計算します。", nullptr
	},
	{	// public static double floor( double x )
		L"floor",
		&LMathClass::method_floor, true,
		L"public static", L"double", L"double x",
		L"x 以下の最大の整数値の浮動小数点値を取得します。", nullptr
	},
	{	// public static double round( double x )
		L"round",
		&LMathClass::method_round, true,
		L"public static", L"double", L"double x",
		L"x の最近値の整数の浮動小数点値を取得します。", nullptr
	},
	{	// public static long abs( long x )
		L"abs",
		&LMathClass::method_iabs, true,
		L"public static", L"long", L"long x",
		L"x の絶対値を取得します。", nullptr
	},
	{	// public static double abs( double x )
		L"abs",
		&LMathClass::method_fabs, true,
		L"public static", L"double", L"double x",
		L"x の絶対値を取得します。", nullptr
	},
	{	// public static long max( long x, long y )
		L"max",
		&LMathClass::method_imax, true,
		L"public static", L"long", L"long x, long y",
		L"x と y の最大値を取得します。", nullptr
	},
	{	// public static double max( double x, double y )
		L"max",
		&LMathClass::method_fmax, true,
		L"public static", L"double", L"double x, double y",
		L"x と y の最大値を取得します。", nullptr
	},
	{	// public static long min( long x, long y )
		L"min",
		&LMathClass::method_imin, true,
		L"public static", L"long", L"long x, long y",
		L"x と y の最小値を取得します。", nullptr
	},
	{	// public static double min( double x, double y )
		L"min",
		&LMathClass::method_fmin, true,
		L"public static", L"double", L"double x, double y",
		L"x と y の最小値を取得します。", nullptr
	},
	{	// public static long clamp( long x, long l, long h )
		L"clamp",
		&LMathClass::method_iclamp, true,
		L"public static", L"long", L"long x, long l, long h",
		L"x を [l, h] 区間にクリップします。", nullptr
	},
	{	// public static double clamp( double x, double l, double h )
		L"clamp",
		&LMathClass::method_fclamp, true,
		L"public static", L"double", L"double x, double l, double h",
		L"x を [l, h] 区間にクリップします。", nullptr
	},
	{	// public static double random( double x )
		L"random",
		&LMathClass::method_random, false,
		L"public static", L"double", L"double x",
		L"[0, x) 範囲の一様分布な乱数を発生します。", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::VariableDesc		LMathClass::s_Variables[3] =
{
	{
		L"PI", L"public static const", L"double",
		L"3.14159265358979323846", nullptr, L"円周率π",
	},
	{
		L"E", L"public static const", L"double",
		L"2.71828182845904523536", nullptr, L"自然対数 e",
	},
	{
		nullptr, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LMathClass::s_MemberDesc =
{
	nullptr,
	LMathClass::s_Functions,
	LMathClass::s_Variables,
	nullptr,
	nullptr,
} ;

// static double sqrt( double x )
void LMathClass::method_sqrt( LContext& _context )
{
	LQT_RETURN_DOUBLE( sqrt( _context.GetArgAt(0).dblValue ) ) ;
}

// static double pow( double x, double y )
void LMathClass::method_pow( LContext& _context )
{
	LQT_RETURN_DOUBLE( pow( _context.GetArgAt(0).dblValue,
								_context.GetArgAt(1).dblValue ) ) ;
}

// static double exp( double x )
void LMathClass::method_exp( LContext& _context )
{
	LQT_RETURN_DOUBLE( exp( _context.GetArgAt(0).dblValue ) ) ;
}

// static double log( double x )
void LMathClass::method_log( LContext& _context )
{
	LQT_RETURN_DOUBLE( log( _context.GetArgAt(0).dblValue ) ) ;
}

// static double log10( double x )
void LMathClass::method_log10( LContext& _context )
{
	LQT_RETURN_DOUBLE( log10( _context.GetArgAt(0).dblValue ) ) ;
}

// static double sin( double x )
void LMathClass::method_sin( LContext& _context )
{
	LQT_RETURN_DOUBLE( sin( _context.GetArgAt(0).dblValue ) ) ;
}

// static double cos( double x )
void LMathClass::method_cos( LContext& _context )
{
	LQT_RETURN_DOUBLE( cos( _context.GetArgAt(0).dblValue ) ) ;
}

// static double tan( double x )
void LMathClass::method_tan( LContext& _context )
{
	LQT_RETURN_DOUBLE( tan( _context.GetArgAt(0).dblValue ) ) ;
}

// static double asin( double x )
void LMathClass::method_asin( LContext& _context )
{
	LQT_RETURN_DOUBLE( asin( _context.GetArgAt(0).dblValue ) ) ;
}

// static double acos( double x )
void LMathClass::method_acos( LContext& _context )
{
	LQT_RETURN_DOUBLE( acos( _context.GetArgAt(0).dblValue ) ) ;
}

// static double atan( double x )
void LMathClass::method_atan( LContext& _context )
{
	LQT_RETURN_DOUBLE( atan( _context.GetArgAt(0).dblValue ) ) ;
}

// static double atan2( double y, double x )
void LMathClass::method_atan2( LContext& _context )
{
	LQT_RETURN_DOUBLE( atan2( _context.GetArgAt(0).dblValue,
								_context.GetArgAt(1).dblValue ) ) ;
}

// static double floor( double x )
void LMathClass::method_floor( LContext& _context )
{
	LQT_RETURN_DOUBLE( floor( _context.GetArgAt(0).dblValue ) ) ;
}

// static double round( double x )
void LMathClass::method_round( LContext& _context )
{
	LQT_RETURN_DOUBLE( round( _context.GetArgAt(0).dblValue ) ) ;
}

// static long abs( long x )
void LMathClass::method_iabs( LContext& _context )
{
	LQT_RETURN_LONG( abs( _context.GetArgAt(0).longValue ) ) ;
}

// static double abs( double x )
void LMathClass::method_fabs( LContext& _context )
{
	LQT_RETURN_DOUBLE( fabs( _context.GetArgAt(0).dblValue ) ) ;
}

// static long max( long x, long y )
void LMathClass::method_imax( LContext& _context )
{
	LQT_RETURN_LONG( std::max( _context.GetArgAt(0).longValue,
								_context.GetArgAt(1).longValue) ) ;
}

// static double max( double x, double y )
void LMathClass::method_fmax( LContext& _context )
{
	LQT_RETURN_DOUBLE( std::max( _context.GetArgAt(0).dblValue,
									_context.GetArgAt(1).dblValue) ) ;
}

// static long min( long x, long y )
void LMathClass::method_imin( LContext& _context )
{
	LQT_RETURN_LONG( std::min( _context.GetArgAt(0).longValue,
								_context.GetArgAt(1).longValue) ) ;
}

// static double min( double x, double y )
void LMathClass::method_fmin( LContext& _context )
{
	LQT_RETURN_DOUBLE( std::min( _context.GetArgAt(0).dblValue,
									_context.GetArgAt(1).dblValue) ) ;
}

// static long clamp( long x, long l, long h )
void LMathClass::method_iclamp( LContext& _context )
{
	LQT_RETURN_LONG
		( std::min( std::max
			( _context.GetArgAt(0).longValue,
				_context.GetArgAt(1).longValue ),
					_context.GetArgAt(2).longValue ) ) ;
}

// static double clamp( double x, double l, double h )
void LMathClass::method_fclamp( LContext& _context )
{
	LQT_RETURN_DOUBLE
		( std::min( std::max
			( _context.GetArgAt(0).dblValue,
				_context.GetArgAt(1).dblValue ),
					_context.GetArgAt(2).dblValue ) ) ;
}

// static double random( double x )
void LMathClass::method_random( LContext& _context )
{
	static std::random_device	s_random ;
	static std::mt19937			s_engine( s_random() ) ;
	std::uniform_real_distribution<double>
								dist( 0.0, _context.GetArgAt(0).dblValue ) ;

	LQT_RETURN_DOUBLE( dist( s_engine ) ) ;
}


