
#ifndef	__LOQUATY_OBJ_EXCEPTION_H__
#define	__LOQUATY_OBJ_EXCEPTION_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Exception オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LExceptionObj	: public LGenericObj
	{
	public:
		LString			m_string ;
		LCodeBuffer *	m_pCodeBuf ;
		size_t			m_iThrown ;

	public:
		LExceptionObj
			( LClass * pClass, const wchar_t * pwszInit = nullptr )
			: LGenericObj( pClass ), m_string( pwszInit ),
				m_pCodeBuf( nullptr ), m_iThrown( 0 ) {}
		LExceptionObj( const LExceptionObj& obj )
			: LGenericObj( obj ), m_string( obj.m_string ),
				m_pCodeBuf( obj.m_pCodeBuf ), m_iThrown( obj.m_iThrown ) {}

	public:
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		// （式表現に近い）文字列に変換
		virtual bool AsExpression( LString& str, std::uint64_t flags = 0 ) const ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// エラー送出ソース位置情報を取得する
		// 投げられたソース情報を取得する
		bool GetThrownSourceInfo
			( LString& strSource, LStringParser::LineInfo& linf ) const ;

	public:
		// Exception( String msg )
		static void method_init( LContext& context ) ;

	} ;

}

#endif

