
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Exception オブジェクト
//////////////////////////////////////////////////////////////////////////////

// 文字列として評価
bool LExceptionObj::AsString( LString& str ) const
{
	str = m_string ;
	return	true ;
}

// （式表現に近い）文字列に変換
bool LExceptionObj::AsExpression( LString& str, std::uint64_t flags ) const
{
	str += L"new " ;
	str += m_pClass->GetFullClassName() ;
	str += L"(\"" ;
	str += LStringParser::EncodeStringLiteral
				( m_string.c_str(), m_string.GetLength() ) ;
	str += L"\")" ;
	return	true ;
}

// 複製する（要素も全て複製処理する）
LObject * LExceptionObj::CloneObject( void ) const
{
	return	new LExceptionObj( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LExceptionObj::DuplicateObject( void ) const
{
	return	new LExceptionObj( *this ) ;
}

// エラー送出ソース位置情報を取得する
// 投げられたソース情報を取得する
bool LExceptionObj::GetThrownSourceInfo
	( LString& strSource, LStringParser::LineInfo& linf ) const
{
	if ( (m_pCodeBuf != nullptr)
		&& (m_pCodeBuf->GetSourceFile() != nullptr) )
	{
		LSourceFilePtr	pSource = m_pCodeBuf->GetSourceFile() ;
		const LCodeBuffer::DebugSourceInfo*
			pdsi = m_pCodeBuf->FindDebugSourceInfo( m_iThrown ) ;
		if ( pdsi != nullptr )
		{
			linf = pSource->FindLineContainingIndexAt( pdsi->m_iSrcFirst ) ;
			strSource = pSource->GetFileName() ;
			return	(linf.iLine >= 1) ;
		}
	}
	return	false ;
}

// Exception( String msg )
void LExceptionObj::method_init( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LExceptionObj, pExcept ) ;
	LQT_FUNC_ARG_STRING( msg ) ;

	pExcept->m_string = msg ;

	LQT_RETURN_VOID() ;
}

// String getThrownSource( long* lineNum )
void LExceptionObj::method_getThrownSource( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LExceptionObj, pExcept ) ;
	LQT_FUNC_ARG_POINTER( LLong, lineNum ) ;

	LString					strSource ;
	LStringParser::LineInfo	linf ;
	if ( !pExcept->GetThrownSourceInfo( strSource, linf ) )
	{
		LQT_RETURN_OBJECT( nullptr ) ;
	}
	if ( lineNum != nullptr )
	{
		*lineNum = linf.iLine ;
	}
	LQT_RETURN_STRING( strSource ) ;
}

