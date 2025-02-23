

#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// XML ライクな文書パーサー
//////////////////////////////////////////////////////////////////////////////

// 代入
const LXMLDocParser& LXMLDocParser::operator = ( const LXMLDocParser& xpars )
{
	LTextFileParser::operator = ( xpars ) ;
	m_mapDTDEntities = xpars.m_mapDTDEntities ;
	m_flagStrictText = xpars.m_flagStrictText ;
	m_nErrors = xpars.m_nErrors ;
	return	*this ;
}

// ドキュメント全体を解釈
//////////////////////////////////////////////////////////////////////////////
std::shared_ptr<LXMLDocument> LXMLDocParser::ParseDocument( void )
{
	LXMLDocPtr	pXmlDoc = NewDocument() ;
	while ( !IsEndOfString() )
	{
		LXMLDocPtr	pElement = ParseElement() ;
		if ( pElement != nullptr )
		{
			pXmlDoc->AddElement( pElement ) ;
		}
		else if ( HasNextString( L"</" ) )
		{
			OnError( (LString(L"</") + ParseName()
						+ L"> に対応するタグがありません").c_str() ) ;
			break ;
		}
	}
	return	pXmlDoc ;
}

// １つの要素を解釈
//////////////////////////////////////////////////////////////////////////////
std::shared_ptr<LXMLDocument> LXMLDocParser::ParseElement( void )
{
	size_t	iStartPos = GetIndex() ;
	if ( !m_flagStrictText )
	{
		if ( !PassSpace() )
		{
			return	nullptr ;
		}
		iStartPos = GetIndex() ;
	}
	if ( (CurrentAt(0) == L'<') && (CurrentAt(1) == L'/') )
	{
		// 閉じタグ
		return	nullptr ;
	}
	if ( CurrentChar() == L'<' )
	{
		NextChar() ;

		wchar_t	wch = CurrentChar() ;
		if ( wch == '!' )
		{
			if ( HasNextString( L"!--" ) )
			{
				// <!-- コメント -->
				iStartPos = GetIndex() ;
				if ( !SeekString( L"-->" ) )
				{
					OnError( L"<!-- に対応する --> が見つかりません" ) ;
					return	nullptr ;
				}
				LXMLDocPtr	pDoc = NewDocument() ;
				pDoc->SetText
					( Middle( iStartPos, GetIndex() - 3 - iStartPos ),
						LXMLDocument::typeComment ) ;
				return	pDoc ;
			}
			else if ( HasNextString( L"![CDATA[" ) )
			{
				// <![CDATA[ テキスト ]]>
				iStartPos = GetIndex() ;
				if ( !SeekString( L"]]>" ) )
				{
					OnError( L"<![CDATA[ に対応する ]]> が見つかりません" ) ;
					return	nullptr ;
				}
				LXMLDocPtr	pDoc = NewDocument() ;
				pDoc->SetText
					( Middle( iStartPos, GetIndex() - 3 - iStartPos ),
						LXMLDocument::typeCDATA ) ;
				return	pDoc ;
			}
			else if ( HasNextString( L"!DOCTYPE" ) )
			{
				// <!DOCTYPE ... >
				ParseDocTypeDeclaration() ;
			}
			else
			{
				PassDocument( L">", nullptr ) ;
			}
		}
		else if ( wch == '?' )
		{
			// <?tag ... ?>
			NextChar() ;

			LXMLDocPtr	pDoc ;
			LString		strTag = ParseName( L"?>" ) ;
			if ( !strTag.IsEmpty() )
			{
				pDoc = NewDocument() ;
				pDoc->SetTag( strTag.c_str(), LXMLDocument::typeDifinition ) ;
				ParseAttributeList( pDoc, L"?>", true ) ;
			}
			PassSpace() ;
			if ( !HasNextString( L"?>" ) )
			{
				OnError( L"<? に対応する ?> が見つかりません" ) ;
				return	nullptr ;
			}
			return	pDoc ;
		}
		else
		{
			// <tag ... > ... </tag>
			LXMLDocPtr	pDoc ;
			LString		strTag = ParseName( L">" ) ;
			if ( !strTag.IsEmpty() )
			{
				pDoc = NewDocument() ;
				pDoc->SetTag( strTag.c_str(), LXMLDocument::typeTag ) ;
				ParseAttributeList( pDoc, L"/>" ) ;
			}
			else
			{
				OnError( L"タグ名がありません" ) ;
				PassDocument( L">", nullptr ) ;
				return	nullptr ;
			}
			PassSpace() ;
			if ( HasNextString( L"/>" ) )
			{
				return	pDoc ;
			}
			if ( HasNextChars( L">" ) != L'>' )
			{
				OnError( L"< に対応する > が見つかりません" ) ;
				PassDocument( L">", nullptr ) ;
				return	pDoc ;
			}
			while ( !IsEndOfString() )
			{
				LXMLDocPtr	pElement = ParseElement() ;
				if ( pElement != nullptr )
				{
					pDoc->AddElement( pElement ) ;
				}
				else if ( HasNextString( L"</" ) )
				{
					LString	strName = ParseName( L">" ) ;
					if ( strName != strTag )
					{
						OnError( (LString(L"<") + strTag + L"> と </"
								+ strName + L"> が対応していません").c_str() ) ;
					}
					PassDocument( L">", nullptr ) ;
					break ;
				}
			}
			return	pDoc ;
		}
	}
	else
	{
		// テキスト要素
		size_t	iStartPos = GetIndex() ;
		if ( SeekString( L"<" ) )
		{
			SeekIndex( GetIndex() - 1 ) ;
		}
		LString	strXML = Middle( iStartPos, GetIndex() - iStartPos ) ;
		LString	strText =
			DecodeXMLString
				( strXML.c_str(), m_flagStrictText, &m_mapDTDEntities ) ;
		if ( strText.IsEmpty() )
		{
			return	nullptr ;
		}
		LXMLDocPtr	pDoc = NewDocument() ;
		pDoc->SetText( strText, LXMLDocument::typeText ) ;
		return	pDoc ;
	}
	return	nullptr ;
}

// <!DOCTYPE ... > 解釈
//////////////////////////////////////////////////////////////////////////////
void LXMLDocParser::ParseDocTypeDeclaration( void )
{
	if ( PassDocument( L"[", L">" ) != L'[' )
	{
		if ( PassDocument( L">", nullptr ) != L'>' )
		{
			OnError( L"<![DOCTYPE に対応する > が見つかりません" ) ;
		}
		return ;
	}
	while ( !IsEndOfString() )
	{
		if ( PassDocument( L"<", L"]>" ) == L'<' )
		{
			if ( (NextChar() == L'!')
				&& HasNextString( L"ENTITY" ) )
			{
				// <!ENTITY ... >
				LString	strName = ParseName() ;
				if ( !strName.IsEmpty() )
				{
					m_mapDTDEntities.insert
						( std::make_pair( strName, ParseValue() ) ) ;
				}
			}
			if ( PassDocument( L">", nullptr ) != L'>' )
			{
				OnError( L"< に対応する > が見つかりません" ) ;
			}
		}
		else
		{
			break ;
		}
	}
	if ( PassDocument( L"]", L">" ) != L']' )
	{
		OnError( L"<![DOCTYPE 内 [ に対応する ] が見つかりません" ) ;
	}
	if ( PassDocument( L">", nullptr ) != L'>' )
	{
		OnError( L"<![DOCTYPE に対応する > が見つかりません" ) ;
	}
}

// 属性リスト解釈
//////////////////////////////////////////////////////////////////////////////
void LXMLDocParser::ParseAttributeList
	( std::shared_ptr<LXMLDocument> pDoc,
			const wchar_t * pwszEscChars, bool withOrder )
{
	while ( PassSpace() )
	{
		if ( IsCharContained( CurrentChar(), pwszEscChars ) )
		{
			return ;
		}

		LString	strName = ParseName( pwszEscChars ) ;
		if ( strName.IsEmpty() )
		{
			OnError( (LString(L"属性の構文エラーです (<")
						+ pDoc->GetTag() + L">タグ内)").c_str() ) ;
			return ;
		}
		if ( HasNextChars( L"=" ) != L'=' )
		{
			OnError( (LString(L"<") + pDoc->GetTag()
						+ L"> タグ内 " + strName
						+ L" 属性の後ろに = がありません").c_str() ) ;
			return ;
		}
		LString	strValue = ParseValue( pwszEscChars ) ;

		pDoc->SetAttrString( strName.c_str(), strValue.c_str() ) ;

		if ( withOrder )
		{
			pDoc->AddAttrOrder( strName.c_str() ) ;
		}
	}
}

// 属性名取得
//////////////////////////////////////////////////////////////////////////////
LString LXMLDocParser::ParseName( const wchar_t * pwszEscChars )
{
	PassSpace() ;
	if ( IsCharContained( CurrentChar(), pwszEscChars ) )
	{
		return	LString() ;
	}

	LString	strName ;
	size_t	iPos = GetIndex() ;
	if ( NextToken( strName ) != LStringParser::tokenSymbol )
	{
		SeekIndex( iPos ) ;
		return	LString() ;
	}
	while ( PassSpace() )
	{
		if ( IsCharContained( CurrentChar(), pwszEscChars ) )
		{
			break ;
		}
		iPos = GetIndex() ;
		wchar_t	wch = HasNextChars( L":-" ) ;
		if ( wch == 0 )
		{
			break ;
		}
		PassSpace() ;
		if ( IsCharContained( CurrentChar(), pwszEscChars ) )
		{
			SeekIndex( iPos ) ;
			break ;
		}
		LString	strToken ;
		if ( NextToken( strToken ) != LStringParser::tokenSymbol )
		{
			SeekIndex( iPos ) ;
			break ;
		}
		strName += wch ;
		strName += strToken ;
	}
	return	strName ;
}

// 属性値取得
//////////////////////////////////////////////////////////////////////////////
LString LXMLDocParser::ParseValue( const wchar_t * pwszEscChars )
{
	PassSpace() ;
	if ( IsCharContained( CurrentChar(), pwszEscChars ) )
	{
		return	LString() ;
	}

	wchar_t	wchQuote = HasNextChars( L"\"\'" ) ;
	if ( wchQuote == 0 )
	{
		OnError( L"属性が引用符で囲まれていません" ) ;
		return	LString() ;
	}
	size_t	iPos = GetIndex() ;
	size_t	iEnd = iPos ;
	while ( !IsEndOfString() )
	{
		if ( NextChar() == wchQuote )
		{
			break ;
		}
		iEnd = GetIndex() ;
	}
	return	DecodeXMLString( Middle( iPos, iEnd - iPos ).c_str(),
								m_flagStrictText, &m_mapDTDEntities ) ;
}

// 指定文字まで読み飛ばす（入れ子解釈あり）
//////////////////////////////////////////////////////////////////////////////
wchar_t LXMLDocParser::PassDocument
	( const wchar_t * pwszClosers, const wchar_t * pwszEscChars )
{
	for ( ; ; )
	{
		if ( !PassSpace() )
		{
			return	0 ;
		}

		// 適合文字判定
		wchar_t	wch = CurrentChar() ;
		if ( IsCharContained( wch, pwszClosers ) )
		{
			NextChar() ;
			return	wch ;
		}

		// 脱出文字判定
		if ( IsCharContained( wch, pwszEscChars ) )
		{
			return	0 ;
		}

		if ( (wch == L'\"') || (wch == L'\'') )
		{
			NextChar() ;
			while ( !IsEndOfString() )
			{
				if ( NextChar() == wch )
				{
					break ;
				}
			}
		}
		else if ( wch == L'<' )
		{
			NextChar() ;
			PassDocument( L">", nullptr ) ;
		}
		else if ( wch == L'[' )
		{
			NextChar() ;
			PassDocument( L"]", nullptr ) ;
		}
		else
		{
			PassToken() ;
		}
	}
}

// LXMLDocument 生成
//////////////////////////////////////////////////////////////////////////////
std::shared_ptr<LXMLDocument> LXMLDocParser::NewDocument( void )
{
	return	std::make_shared<LXMLDocument>() ;
}

// エラー数
//////////////////////////////////////////////////////////////////////////////
size_t LXMLDocParser::GetErrorCount( void ) const
{
	return	m_nErrors ;
}

// エラー出力
//////////////////////////////////////////////////////////////////////////////
void LXMLDocParser::OnError( const wchar_t * pwszErrMsg )
{
#if	defined(_DEBUG)
	LTrace( "XMLDocParser:error: %s\n",
				LString(pwszErrMsg).ToString().c_str() ) ;
#endif
	m_nErrors ++ ;
}

// ドキュメントをシリアライズ
//////////////////////////////////////////////////////////////////////////////
bool LXMLDocParser::SaveToFile
	( const wchar_t * pwszFile,
		const LXMLDocument& xmlDoc,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	LFilePtr	file = LURLSchemer::Open( pwszFile, LDirectory::modeCreate ) ;
	if ( file == nullptr )
	{
		return	false ;
	}
	LOutputUTF8Stream	strm( file ) ;
	Serialize( strm, xmlDoc, nReadableLineWidth, pwszReadableIndent ) ;
	return	true ;
}

void LXMLDocParser::Serialize
	( LOutputStream& strm,
		const LXMLDocument& xmlDoc,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	switch ( xmlDoc.GetType() )
	{
	case	LXMLDocument::typeRoot:
		SerializeElements
			( strm, xmlDoc,
				nReadableLineWidth, pwszReadableIndent ) ;
		break ;

	case	LXMLDocument::typeTag:
		if ( xmlDoc.GetElementCount() > 0 )
		{
			strm << pwszReadableIndent
					<< SerializeTag( xmlDoc, L"<", L">",
						nReadableLineWidth, pwszReadableIndent ) ;
			if ( xmlDoc.GetElementCount() == 1 )
			{
				SerializeElements( strm, xmlDoc, nReadableLineWidth, nullptr ) ;
				strm << L"</" << xmlDoc.GetTag() << L">" ;
			}
			else
			{
				LString			strSubIndent ;
				const wchar_t * pwszSubIndent = nullptr ;
				if ( pwszReadableIndent != nullptr )
				{
					strm << L"\r\n" ;
					strSubIndent = pwszReadableIndent ;
					strSubIndent += L"\t" ;
					pwszSubIndent = strSubIndent.c_str() ;
				}
				SerializeElements
					( strm, xmlDoc, nReadableLineWidth, pwszSubIndent ) ;

				strm << pwszReadableIndent
						<< L"</" << xmlDoc.GetTag() << L">" ;
			}
		}
		else
		{
			strm << pwszReadableIndent
					<< SerializeTag( xmlDoc, L"<", L"/>",
						nReadableLineWidth, pwszReadableIndent ) ;
		}
		if ( pwszReadableIndent != nullptr )
		{
			strm << L"\r\n" ;
		}
		break ;

	case	LXMLDocument::typeDifinition:
		strm << pwszReadableIndent
				<< SerializeTag( xmlDoc, L"<?", L"?>",
								nReadableLineWidth, pwszReadableIndent ) ;
		if ( pwszReadableIndent != nullptr )
		{
			strm << L"\r\n" ;
		}
		break ;

	case	LXMLDocument::typeText:
		strm << pwszReadableIndent
				<< EncodeXMLString( xmlDoc.GetText().c_str(), pwszReadableIndent ) ;
		if ( pwszReadableIndent != nullptr )
		{
			strm << L"\r\n" ;
		}
		break ;

	case	LXMLDocument::typeCDATA:
		strm << pwszReadableIndent << L"<![CDATA[" << xmlDoc.GetText() << L"]]>" ;
		if ( pwszReadableIndent != nullptr )
		{
			strm << L"\r\n" ;
		}
		break ;

	case	LXMLDocument::typeComment:
		strm << pwszReadableIndent << L"<!--" << xmlDoc.GetText() << L"-->" ;
		if ( pwszReadableIndent != nullptr )
		{
			strm << L"\r\n" ;
		}
		break ;
	}
}

void LXMLDocParser::Serialize
	( LString& strXmlDoc,
		const LXMLDocument& xmlDoc,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	std::shared_ptr<LArrayBufStorage>
		buf = std::make_shared<LArrayBufStorage>() ;
	buf->ReserveBuffer( 0x100 ) ;

	std::shared_ptr<LBufferedFile>
		file =std::make_shared<LBufferedFile>( LDirectory::modeWrite, buf ) ;

	LOutputUTF8Stream	strm( file ) ;

	Serialize( strm, xmlDoc, nReadableLineWidth, pwszReadableIndent ) ;

	strXmlDoc.FromUTF8( *buf ) ;
}

// 要素をシリアライズ
//////////////////////////////////////////////////////////////////////////////
void LXMLDocParser::SerializeElements
	( LOutputStream& strm,
		const LXMLDocument& xmlDoc,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	for ( size_t i = 0; i < xmlDoc.GetElementCount(); i ++ )
	{
		LXMLDocPtr	pDoc = xmlDoc.GetElementAt( i ) ;
		assert( pDoc != nullptr ) ;
		if ( pDoc != nullptr )
		{
			Serialize( strm, *pDoc, nReadableLineWidth, pwszReadableIndent ) ;
		}
	}
}

void LXMLDocParser::SerializeElements
	( LString& strXmlDoc,
		const LXMLDocument& xmlDoc,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	std::shared_ptr<LArrayBufStorage>
		buf = std::make_shared<LArrayBufStorage>() ;
	buf->ReserveBuffer( 0x100 ) ;

	std::shared_ptr<LBufferedFile>
		file =std::make_shared<LBufferedFile>( LDirectory::modeWrite, buf ) ;

	LOutputUTF8Stream	strm( file ) ;

	SerializeElements( strm, xmlDoc, nReadableLineWidth, pwszReadableIndent ) ;

	strXmlDoc.FromUTF8( *buf ) ;
}

// タグをシリアライズ
//////////////////////////////////////////////////////////////////////////////
LString LXMLDocParser::SerializeTag
	( const LXMLDocument& xmlDoc,
		const wchar_t * pwszLeader,
		const wchar_t * pwszCloser,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	LString	strTag = pwszLeader ;
	strTag += xmlDoc.GetTag() ;

	if ( xmlDoc.GetAttributes().size() > 0 )
	{
		LString	strIndent ;
		if ( pwszReadableIndent != nullptr )
		{
			strIndent = pwszReadableIndent ;
			strIndent += L"\t" ;
			pwszReadableIndent = strIndent.c_str();
		}

		strTag += L" " ;
		strTag += SerializeAtributes
					( xmlDoc, strTag.GetLength(),
						nReadableLineWidth, pwszReadableIndent ) ;
	}

	strTag += pwszCloser ;
	return	strTag ;
}

// 属性をシリアライズ
//////////////////////////////////////////////////////////////////////////////
LString LXMLDocParser::SerializeAtributes
	( const LXMLDocument& xmlDoc,
		size_t nFirstLineLeft,
		size_t nReadableLineWidth,
		const wchar_t * pwszReadableIndent ) const
{
	LString		strAttributes ;
	ssize_t		nLineBase = - (ssize_t) nFirstLineLeft ;

	std::set<LString>					setAttrs ;
	const std::vector<LString>&			attrOrder = xmlDoc.GetAttrOrder() ;
	const std::map<LString,LString>&	attributes = xmlDoc.GetAttributes() ;
	for ( size_t i = 0; i < attrOrder.size(); i ++ )
	{
		auto iter = attributes.find( attrOrder.at(i) ) ;
		if ( iter == attributes.end() )
		{
			continue ;
		}
		LString	strText = EncodeXMLString( iter->second.c_str(), nullptr ) ;
		if ( !strAttributes.IsEmpty()
			&& (pwszReadableIndent != nullptr)
			&& (strAttributes.GetLength() - nLineBase
				+ iter->first.length() + strText.GetLength() > nReadableLineWidth) )
		{
			strAttributes += L"\r\n" ;
			nLineBase = strAttributes.GetLength() ;
			strAttributes += pwszReadableIndent ;
		}
		if ( !strAttributes.IsEmpty() )
		{
			strAttributes += L" " ;
		}
		strAttributes += iter->first ;
		strAttributes += L"=\"" ;
		strAttributes += EncodeXMLString( iter->second.c_str(), nullptr ) ;
		strAttributes += L"\"" ;

		setAttrs.insert( iter->first ) ;
	}

	for ( auto iter = attributes.begin(); iter != attributes.end(); iter ++ )
	{
		if ( setAttrs.count( iter->first ) > 0 )
		{
			continue ;
		}
		LString	strText = EncodeXMLString( iter->second.c_str(), nullptr ) ;
		if ( !strAttributes.IsEmpty()
			&& (pwszReadableIndent != nullptr)
			&& (strAttributes.GetLength() - nLineBase
				+ iter->first.length() + strText.GetLength() > nReadableLineWidth) )
		{
			strAttributes += L"\r\n" ;
			nLineBase = strAttributes.GetLength() ;
			strAttributes += pwszReadableIndent ;
		}
		if ( !strAttributes.IsEmpty() )
		{
			strAttributes += L" " ;
		}
		strAttributes += iter->first ;
		strAttributes += L"=\"" ;
		strAttributes += EncodeXMLString( iter->second.c_str(), nullptr ) ;
		strAttributes += L"\"" ;
	}
	return	strAttributes ;
}

// XML エスケープ処理
//////////////////////////////////////////////////////////////////////////////
LString LXMLDocParser::EncodeXMLString
	( const wchar_t * pwszStr, const wchar_t * pwszReadableIndent )
{
	static const wchar_t *	s_pwszEscChar[] =
	{
		L"&lt;", L"&gt;", L"&quot;", L"&amp;", L"&nbsp;", nullptr
	} ;
	static const wchar_t	s_wchEscChar[] =
	{
		L'<', L'>', L'\"', L'&', 0xA0, L'\0'
	} ;

	LStringParser	sparsStr = pwszStr ;
	LString			strEncoded ;
	LString			strCode;
	size_t			iLast = 0 ;
	wchar_t			wchLast = 0 ;

	strEncoded.reserve( sparsStr.GetLength() + 0x20 ) ;

	while ( !sparsStr.IsEndOfString() )
	{
		wchar_t	wch = sparsStr.CurrentChar() ;
		if ( wch <= L' ' )
		{
			if ( (wch != L' ') || (wchLast <= L' ') || (sparsStr.CurrentAt(1) == 0) )
			{
				strEncoded += sparsStr.Middle
								( iLast, sparsStr.GetIndex() - iLast ) ;
				strCode.SetIntegerOf( wch ) ;
				strEncoded += L"&#" ;
				strEncoded += strCode ;
				strEncoded += L";" ;
				wchLast = L';' ;

				if ( (wch == L'\n') && (pwszReadableIndent != nullptr) )
				{
					strEncoded += L"\r\n" ;
					strEncoded += pwszReadableIndent ;
					wchLast = 0 ;
				}
				iLast = sparsStr.GetIndex() + 1 ;
			}
			else
			{
				wchLast = wch ;
			}
		}
		else if ( wch <= 0xA0 )
		{
			for ( int i = 0; s_pwszEscChar[i] != nullptr; i ++ )
			{
				if ( s_wchEscChar[i] == wch )
				{
					strEncoded +=
						sparsStr.Middle( iLast, sparsStr.GetIndex() - iLast ) ;
					strEncoded += s_pwszEscChar[i] ;

					iLast = sparsStr.GetIndex() + 1 ;
					break ;
				}
			}
			wchLast = wch ;
		}
		else
		{
			wchLast = wch ;
		}
		sparsStr.NextChar() ;
	}

	strEncoded += sparsStr.Middle( iLast, sparsStr.GetLength() - iLast ) ;
	return	strEncoded ;
}

// XML エスケープ復号処理
//////////////////////////////////////////////////////////////////////////////
LString LXMLDocParser::DecodeXMLString
	( const wchar_t * pwszXML,
		bool flagStrict, const std::map<LString,LString> * pDTD )
{
	static const wchar_t *	s_pwszEscChar[] =
	{
		L"lt", L"gt", L"quot", L"amp", L"nbsp", nullptr
	} ;
	static const wchar_t	s_wchSpecChar[] =
	{
		L'<', L'>', L'\"', L'&', 0xA0, L'\0'
	} ;

	LStringParser	sparsXML = pwszXML ;
	LString			strDecoded ;
	LString			strName ;
	size_t			iLast = 0 ;

	strDecoded.reserve( sparsXML.GetLength() ) ;

	if ( !flagStrict )
	{
		sparsXML.TrimRight() ;
		sparsXML.PassSpace() ;
		iLast = sparsXML.GetIndex() ;
	}

	while ( !sparsXML.IsEndOfString() )
	{
		wchar_t	wch = sparsXML.CurrentChar() ;
		if ( wch == L'&' )
		{
			strDecoded += sparsXML.Middle( iLast, sparsXML.GetIndex() - iLast ) ;
			//
			sparsXML.NextChar() ;
			if ( sparsXML.CurrentChar() == L'#' )
			{
				sparsXML.NextChar() ;
				if ( (sparsXML.CurrentChar() == L'x')
					|| (sparsXML.CurrentChar() == L'X') )
				{
					// &#x...; 表記
					sparsXML.NextEnclosedString( strName, L';' ) ;
					strDecoded += (wchar_t) strName.AsInteger( nullptr, 16 ) ;
				}
				else
				{
					// &#...; 表記
					sparsXML.NextEnclosedString( strName, L';' ) ;
					strDecoded += (wchar_t) strName.AsInteger() ;
				}
			}
			else
			{
				// &...; 形式
				sparsXML.NextEnclosedString( strName, L';' ) ;

				bool	flagFound = false ;
				if ( pDTD != nullptr )
				{
					auto	iter = pDTD->find( strName.c_str() ) ;
					if ( iter != pDTD->end() )
					{
						strDecoded += iter->second ;
						flagFound = true ;
					}
				}
				if ( !flagFound )
				{
					for ( int i = 0; s_pwszEscChar[i] != nullptr; i ++ )
					{
						if ( strName == s_pwszEscChar[i] )
						{
							strDecoded += s_wchSpecChar[i] ;
							break ;
						}
					}
				}
			}
			iLast = sparsXML.GetIndex() ;
		}
		else if ( (wch == L'\n') && !flagStrict )
		{
			LString	strText =
				sparsXML.Middle( iLast, sparsXML.GetIndex() - iLast ) ;
			strText.TrimRight() ;
			strDecoded += strText ;

			sparsXML.PassSpace() ;
			iLast = sparsXML.GetIndex() ;
		}
		else
		{
			sparsXML.NextChar() ;
		}
	}

	strDecoded += sparsXML.Middle( iLast, sparsXML.GetLength() - iLast ) ;
	return	strDecoded ;
}



//////////////////////////////////////////////////////////////////////////////
// XML ライクな簡易文書データ
//////////////////////////////////////////////////////////////////////////////

LXMLDocument::LXMLDocument( void )
	: m_type( typeRoot )
{
}

LXMLDocument::LXMLDocument( const wchar_t * pwszTag )
	: m_type( typeTag ), m_tag( pwszTag )
{
}

LXMLDocument::LXMLDocument( ElementType type, const wchar_t * pwszText )
	: m_type( type ), m_text( pwszText )
{
}

LXMLDocument::LXMLDocument( const LXMLDocument& xml )
	: m_type( xml.m_type ),
		m_tag( xml.m_tag ),
		m_text( xml.m_text ),
		m_attributes( xml.m_attributes ),
		m_elements( xml.m_elements )
{
}

const LXMLDocument& LXMLDocument::operator = ( const LXMLDocument& xml )
{
	m_type = xml.m_type ;
	m_tag = xml.m_tag ;
	m_text = xml.m_text ;
	m_attributes = xml.m_attributes ;
	m_elements = xml.m_elements ;
	return	*this ;
}

// 属性は存在するか？
bool LXMLDocument::HasAttribute( const wchar_t * pwszName ) const
{
	return	(m_attributes.find( pwszName ) != m_attributes.end()) ;
}

// 属性を削除
bool LXMLDocument::RemoveAttribute( const wchar_t * pwszName )
{
	auto	iter = m_attributes.find( pwszName ) ;
	if ( iter == m_attributes.end() )
	{
		return	false ;
	}
	m_attributes.erase( iter ) ;
	return	true ;
}

void LXMLDocument::RemoveAllAttributes( void )
{
	m_attributes.clear() ;
	m_attrOrder.clear() ;
}

// 属性値を取得
LString LXMLDocument::GetAttrString
	( const wchar_t * pwszName, const wchar_t * pwszDefault ) const
{
	auto	iter = m_attributes.find( pwszName ) ;
	if ( iter == m_attributes.end() )
	{
		return	LString( pwszDefault ) ;
	}
	return	iter->second ;
}

LInt LXMLDocument::GetAttrInteger
	( const wchar_t * pwszName, LInt nDefault, int nRadix ) const
{
	return	(LInt) GetAttrLong( pwszName, nDefault, nRadix ) ;
}

LLong LXMLDocument::GetAttrLong
	( const wchar_t * pwszName, LLong nDefault, int nRadix ) const
{
	auto	iter = m_attributes.find( pwszName ) ;
	if ( iter == m_attributes.end() )
	{
		return	nDefault ;
	}
	bool	flagHasInt = false ;
	LLong	nValue = iter->second.AsInteger( &flagHasInt, nRadix ) ;
	if ( !flagHasInt )
	{
		return	nDefault ;
	}
	return	nValue ;
}

LDouble LXMLDocument::GetAttrNumber
	( const wchar_t * pwszName, LDouble nDefault ) const
{
	auto	iter = m_attributes.find( pwszName ) ;
	if ( iter == m_attributes.end() )
	{
		return	nDefault ;
	}
	bool	flagHasNum = false ;
	LDouble	nValue = iter->second.AsNumber( &flagHasNum ) ;
	if ( !flagHasNum )
	{
		return	nDefault ;
	}
	return	nValue ;
}

// 属性値を設定
void LXMLDocument::SetAttrString
		( const wchar_t * pwszName, const wchar_t * pwszString )
{
	auto	iter = m_attributes.find( pwszName ) ;
	if ( iter == m_attributes.end() )
	{
		m_attributes.insert
			( std::make_pair<LString,LString>( pwszName, pwszString ) ) ;
	}
	else
	{
		iter->second = pwszString ;
	}
}

void LXMLDocument::SetAttrInteger
		( const wchar_t * pwszName, LInt nValue, int nRadix )
{
	SetAttrString( pwszName, LString::IntegerOf( nValue, 0, nRadix ).c_str() ) ;
}

void LXMLDocument::SetAttrLong
		( const wchar_t * pwszName, LLong nValue, int nRadix )
{
	SetAttrString( pwszName, LString::IntegerOf( nValue, 0, nRadix ).c_str() ) ;
}

void LXMLDocument::SetAttrNumber
		( const wchar_t * pwszName, LDouble nValue )
{
	SetAttrString( pwszName, LString::NumberOf( nValue, 16, true ).c_str() ) ;
}

// シリアライズ時の属性値の順序を追加する
void LXMLDocument::AddAttrOrder( const wchar_t * pwszName )
{
	m_attrOrder.push_back( LString(pwszName) ) ;
}

// 要素数取得
size_t LXMLDocument::GetElementCount( void ) const
{
	return	m_elements.size() ;
}

// 要素取得
std::shared_ptr<LXMLDocument> LXMLDocument::GetElementAt( size_t iElement ) const
{
	if ( iElement >= m_elements.size() )
	{
		return	nullptr ;
	}
	return	m_elements.at( iElement ) ;
}

std::shared_ptr<LXMLDocument>
	LXMLDocument::GetTagAs( const wchar_t * pwszTag, size_t iFirst ) const
{
	LString	strTag = pwszTag ;
	for ( size_t i = iFirst; i < m_elements.size(); i ++ )
	{
		LXMLDocPtr	pDoc = m_elements.at(i) ;
		if ( (pDoc->m_type == typeTag)
			&& (pDoc->m_tag == strTag) )
		{
			return	pDoc ;
		}
	}
	return	nullptr ;
}

LString LXMLDocument::GetTextElement( size_t iFirst ) const
{
	for ( size_t i = iFirst; i < m_elements.size(); i ++ )
	{
		LXMLDocPtr	pDoc = m_elements.at(i) ;
		if ( (pDoc->m_type == typeText)
			|| (pDoc->m_type == typeCDATA) )
		{
			return	pDoc->m_text ;
		}
	}
	return	LString() ;
}

ssize_t LXMLDocument::FindElement
	( ElementType type, const wchar_t * pwszTag, size_t iFirst ) const
{
	LString	strTag = pwszTag ;
	for ( size_t i = iFirst; i < m_elements.size(); i ++ )
	{
		LXMLDocPtr	pDoc = m_elements.at(i) ;
		if ( (pDoc->m_type == type)
			&& (pDoc->m_tag == strTag) )
		{
			return	(ssize_t) i ;
		}
	}
	return	-1 ;
}

ssize_t LXMLDocument::FindElement( std::shared_ptr<LXMLDocument> pElement ) const
{
	for ( size_t i = 0; i < m_elements.size(); i ++ )
	{
		if ( m_elements.at(i) == pElement )
		{
			return	(ssize_t) i ;
		}
	}
	return	-1 ;
}

// 要素追加
void LXMLDocument::AddElement( std::shared_ptr<LXMLDocument> pElement )
{
	m_elements.push_back( pElement ) ;
}

void LXMLDocument::InsertElement
		( size_t index, std::shared_ptr<LXMLDocument> pElement )
{
	if ( index < m_elements.size() )
	{
		m_elements.insert( m_elements.begin() + index, pElement ) ;
	}
	else
	{
		m_elements.push_back( pElement ) ;
	}
}

// タグが存在していればそのタグを取得
// 存在しない場合には作成して追加して取得
std::shared_ptr<LXMLDocument>
	LXMLDocument::CreateTagAs( const wchar_t * pwszTag, size_t iFirst )
{
	LXMLDocPtr	pTag = GetTagAs( pwszTag, iFirst ) ;
	if ( pTag == nullptr )
	{
		pTag = std::make_shared<LXMLDocument>( pwszTag ) ;
		m_elements.push_back( pTag ) ;
	}
	return	pTag ;
}

// 要素削除
void LXMLDocument::RemoveElementAt( size_t iElement )
{
	if ( iElement < m_elements.size() )
	{
		m_elements.erase( m_elements.begin() + iElement ) ;
	}
}

void LXMLDocument::RemoveAllElements( void )
{
	m_elements.clear() ;
}

// 階層検索 (タグ名を '>' 記号で区切って階層指定)
std::shared_ptr<LXMLDocument>
	LXMLDocument::GetTagPathAs( const wchar_t * pwszTagPath ) const
{
	if ( (pwszTagPath == nullptr) || (pwszTagPath[0] == 0) )
	{
		return	nullptr ;
	}
	size_t	iSub = 0 ;
	while ( pwszTagPath[iSub] != 0 )
	{
		if ( pwszTagPath[iSub] == L'>' )
		{
			break ;
		}
		iSub ++ ;
	}
	if ( pwszTagPath[iSub] == L'>' )
	{
		LXMLDocPtr	pTag = GetTagAs( LString( pwszTagPath, iSub ).c_str() ) ;
		if ( pTag != nullptr )
		{
			return	pTag->GetTagPathAs( pwszTagPath + iSub + 1 ) ;
		}
		return	nullptr ;
	}
	else
	{
		return	GetTagAs( pwszTagPath ) ;
	}
}

// 階層取得 (タグ名を '>' 記号で区切って階層指定)
// 存在しない場合には作成する
std::shared_ptr<LXMLDocument>
	LXMLDocument::CreateTagPathAs( const wchar_t * pwszTagPath )
{
	if ( (pwszTagPath == nullptr) || (pwszTagPath[0] == 0) )
	{
		return	nullptr ;
	}
	size_t	iSub = 0 ;
	while ( pwszTagPath[iSub] != 0 )
	{
		if ( pwszTagPath[iSub] == L'>' )
		{
			break ;
		}
		iSub ++ ;
	}
	if ( pwszTagPath[iSub] == L'>' )
	{
		LXMLDocPtr	pTag = CreateTagAs( LString( pwszTagPath, iSub ).c_str() ) ;
		return	pTag->CreateTagPathAs( pwszTagPath + iSub + 1 ) ;
	}
	else
	{
		return	CreateTagAs( pwszTagPath ) ;
	}
}




//////////////////////////////////////////////////////////////////////////////
// XMLDocParser クラス用 native 実装宣言
//////////////////////////////////////////////////////////////////////////////

DECL_LOQUATY_CONSTRUCTOR(XMLDocParser);
DECL_LOQUATY_CONSTRUCTOR_N(XMLDocParser,1);
DECL_LOQUATY_FUNC(XMLDocParser_operator_smov_2);
DECL_LOQUATY_FUNC(XMLDocParser_operator_smov_3);
DECL_LOQUATY_FUNC(XMLDocParser_parseDocument);
DECL_LOQUATY_FUNC(XMLDocParser_parseElement);
DECL_LOQUATY_FUNC(XMLDocParser_getErrorCount);
DECL_LOQUATY_FUNC(XMLDocParser_getErrorMessage);
DECL_LOQUATY_FUNC(XMLDocParser_saveToFile);
DECL_LOQUATY_FUNC(XMLDocParser_serialize);
DECL_LOQUATY_FUNC(XMLDocParser_serializeToString);
DECL_LOQUATY_FUNC(XMLDocParser_isStrictText);
DECL_LOQUATY_FUNC(XMLDocParser_setStrictText);
DECL_LOQUATY_FUNC(XMLDocParser_encodeXMLString);
DECL_LOQUATY_FUNC(XMLDocParser_decodeXMLString);

class	LXMLDocParserObj	: public LXMLDocParser
{
protected:
	LString	m_strErrMsg ;

public:
	// 代入
	const LXMLDocParserObj& operator = ( const LXMLDocParserObj& xpars )
	{
		LXMLDocParser::operator = ( xpars ) ;
		m_strErrMsg = xpars.m_strErrMsg ;
		return	*this ;
	}
	const LXMLDocParserObj& operator = ( const wchar_t * pwszStr )
	{
		LXMLDocParser::operator = ( pwszStr ) ;
		return	*this ;
	}
	// エラー出力
	virtual void OnError( const wchar_t * pwszErrMsg )
	{
		LXMLDocParser::OnError( pwszErrMsg ) ;
		m_strErrMsg += pwszErrMsg ;
		m_strErrMsg += L"\n" ;
	}
	// エラー取得
	const LString& GetErrorMessage( void ) const
	{
		return	m_strErrMsg ;
	}
} ;



//////////////////////////////////////////////////////////////////////////////
// XMLDocParser クラス用 native 実装
//////////////////////////////////////////////////////////////////////////////

// XMLDocParser() ;
IMPL_LOQUATY_CONSTRUCTOR(XMLDocParser)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LXMLDocParserObj, pThis, () ) ;

	LQT_RETURN_VOID() ;
}

// XMLDocParser( String str ) ;
IMPL_LOQUATY_CONSTRUCTOR_N(XMLDocParser,1)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LXMLDocParserObj, pThis, () ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	*pThis = str.c_str() ;

	LQT_RETURN_VOID() ;
}

// XMLDocParser operator :=( XMLDocParser xpars )
IMPL_LOQUATY_FUNC(XMLDocParser_operator_smov_2)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParserObj, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocParserObj, xpars ) ;
	LQT_VERIFY_NULL_PTR( xpars ) ;

	*pThis = *xpars ;

	LQT_RETURN_OBJECT( LQT_ARG_OBJECT(0) ) ;
}

// XMLDocParser operator := ( String str ) ;
IMPL_LOQUATY_FUNC(XMLDocParser_operator_smov_3)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParserObj, pThis ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	*pThis = str.c_str() ;

	LQT_RETURN_OBJECT( LQT_ARG_OBJECT(0) ) ;
}

// XMLDocument parseDocument() ;
IMPL_LOQUATY_FUNC(XMLDocParser_parseDocument)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;

	LPtr<LNativeObj>
		pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( pThis->ParseDocument() ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}

// XMLDocument parseElement() ;
IMPL_LOQUATY_FUNC(XMLDocParser_parseElement)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;

	LPtr<LNativeObj>
		pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( pThis->ParseElement() ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}

// ulong getErrorCount() const ;
IMPL_LOQUATY_FUNC(XMLDocParser_getErrorCount)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;

	LQT_RETURN_ULONG( pThis->GetErrorCount() ) ;
}

// String getErrorMessage() const ;
IMPL_LOQUATY_FUNC(XMLDocParser_getErrorMessage)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParserObj, pThis ) ;

	LQT_RETURN_STRING( pThis->GetErrorMessage() ) ;
}

// boolean saveToFile
//	( String path, XMLDocument xmlDoc, ulong nReadableLineWidth = 60 ) ;
IMPL_LOQUATY_FUNC(XMLDocParser_saveToFile)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;
	LQT_FUNC_ARG_STRING( path ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlDoc ) ;
	LQT_VERIFY_NULL_PTR( xmlDoc ) ;
	LQT_FUNC_ARG_ULONG( nReadableLineWidth ) ;

	LQT_RETURN_BOOL
		( pThis->SaveToFile
			( path.c_str(), *xmlDoc,
				(size_t) nReadableLineWidth,
				(pThis->IsStrictText() ? nullptr : L"") ) ) ;
}

// void serialize
//	( OutputStream strm, XMLDocument xmlDoc, ulong nReadableLineWidth = 60 ) ;
IMPL_LOQUATY_FUNC(XMLDocParser_serialize)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LOutputStream, strm ) ;
	LQT_VERIFY_NULL_PTR( strm ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlDoc ) ;
	LQT_VERIFY_NULL_PTR( xmlDoc ) ;
	LQT_FUNC_ARG_ULONG( nReadableLineWidth ) ;

	pThis->Serialize
		( *strm, *xmlDoc,
			(size_t) nReadableLineWidth,
			(pThis->IsStrictText() ? nullptr : L"") ) ;

	LQT_RETURN_VOID() ;
}

// String serializeToString
//	( XMLDocument xmlDoc, ulong nReadableLineWidth = 60 ) ;
IMPL_LOQUATY_FUNC(XMLDocParser_serializeToString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlDoc ) ;
	LQT_VERIFY_NULL_PTR( xmlDoc ) ;
	LQT_FUNC_ARG_ULONG( nReadableLineWidth ) ;

	LString	strXmlDoc ;
	pThis->Serialize
		( strXmlDoc, *xmlDoc,
			(size_t) nReadableLineWidth,
			(pThis->IsStrictText() ? nullptr : L"") ) ;

	LQT_RETURN_STRING( strXmlDoc ) ;
}

// boolean isStrictText() const ;
IMPL_LOQUATY_FUNC(XMLDocParser_isStrictText)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;

	LQT_RETURN_BOOL( pThis->IsStrictText() ) ;
}

// void setStrictText( boolean strict ) ;
IMPL_LOQUATY_FUNC(XMLDocParser_setStrictText)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;
	LQT_FUNC_ARG_BOOL( strict ) ;

	pThis->SetStrictText( strict ) ;

	LQT_RETURN_VOID() ;
}

// String encodeXMLString( String str, String indent = null ) const ;
IMPL_LOQUATY_FUNC(XMLDocParser_encodeXMLString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;
	LQT_FUNC_ARG_STRING( str ) ;
	LQT_FUNC_ARG_STRING( indent ) ;

	LQT_RETURN_STRING
		( LXMLDocParser::EncodeXMLString
			( str.c_str(),
				(pThis->IsStrictText() ? nullptr : indent.c_str()) ) ) ;
}

// String decodeXMLString( String str ) const ;
IMPL_LOQUATY_FUNC(XMLDocParser_decodeXMLString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocParser, pThis ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	LQT_RETURN_STRING
		( LXMLDocParser::DecodeXMLString
			( str.c_str(), pThis->IsStrictText(), &(pThis->GetDTDEntities()) ) ) ;
}



//////////////////////////////////////////////////////////////////////////////
// XMLDocument クラス用 native 実装宣言
//////////////////////////////////////////////////////////////////////////////

DECL_LOQUATY_CONSTRUCTOR(XMLDocument);
DECL_LOQUATY_CONSTRUCTOR_N(XMLDocument,1);
DECL_LOQUATY_FUNC(XMLDocument_operator_smov);
DECL_LOQUATY_FUNC(XMLDocument_getType);
DECL_LOQUATY_FUNC(XMLDocument_setType);
DECL_LOQUATY_FUNC(XMLDocument_getTag);
DECL_LOQUATY_FUNC(XMLDocument_setTag);
DECL_LOQUATY_FUNC(XMLDocument_getText);
DECL_LOQUATY_FUNC(XMLDocument_setText);
DECL_LOQUATY_FUNC(XMLDocument_hasAttribute);
DECL_LOQUATY_FUNC(XMLDocument_removeAttribute);
DECL_LOQUATY_FUNC(XMLDocument_removeAllAttributes);
DECL_LOQUATY_FUNC(XMLDocument_getAttrString);
DECL_LOQUATY_FUNC(XMLDocument_getAttrInteger);
DECL_LOQUATY_FUNC(XMLDocument_getAttrNumber);
DECL_LOQUATY_FUNC(XMLDocument_setAttrString);
DECL_LOQUATY_FUNC(XMLDocument_setAttrInteger);
DECL_LOQUATY_FUNC(XMLDocument_setAttrNumber);
DECL_LOQUATY_FUNC(XMLDocument_addAttrOrder);
DECL_LOQUATY_FUNC(XMLDocument_getElementCount);
DECL_LOQUATY_FUNC(XMLDocument_getElementAt);
DECL_LOQUATY_FUNC(XMLDocument_getTagAs);
DECL_LOQUATY_FUNC(XMLDocument_getTextElement);
DECL_LOQUATY_FUNC(XMLDocument_findElement);
DECL_LOQUATY_FUNC(XMLDocument_findElementPtr);
DECL_LOQUATY_FUNC(XMLDocument_addElement);
DECL_LOQUATY_FUNC(XMLDocument_insertElement);
DECL_LOQUATY_FUNC(XMLDocument_createTagAs);
DECL_LOQUATY_FUNC(XMLDocument_removeElementAt);
DECL_LOQUATY_FUNC(XMLDocument_removeAllElements);
DECL_LOQUATY_FUNC(XMLDocument_getTagPathAs);
DECL_LOQUATY_FUNC(XMLDocument_createTagPathAs);




//////////////////////////////////////////////////////////////////////////////
// XMLDocParser クラス用 native 実装
//////////////////////////////////////////////////////////////////////////////

// XMLDocument( )
IMPL_LOQUATY_CONSTRUCTOR(XMLDocument)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LXMLDocument, pThis, () ) ;

	LQT_RETURN_VOID() ;
}

// XMLDocument( XMLDocument xmlDoc )
IMPL_LOQUATY_CONSTRUCTOR_N(XMLDocument,1)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LXMLDocument, pThis, () ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlDoc ) ;
	LQT_VERIFY_NULL_PTR( xmlDoc ) ;

	*pThis = *xmlDoc ;

	LQT_RETURN_VOID() ;
}

// XMLDocument operator :=( XMLDocument xmlDoc )
IMPL_LOQUATY_FUNC(XMLDocument_operator_smov)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlDoc ) ;
	LQT_VERIFY_NULL_PTR( xmlDoc ) ;

	*pThis = *xmlDoc ;

	LQT_RETURN_OBJECT( LQT_ARG_OBJECT(0) ) ;
}

// int getType( ) const
IMPL_LOQUATY_FUNC(XMLDocument_getType)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;

	LQT_RETURN_INT( pThis->GetType() ) ;
}

// void setType( int type )
IMPL_LOQUATY_FUNC(XMLDocument_setType)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_INT( type ) ;

	pThis->SetType( (LXMLDocument::ElementType) type ) ;

	LQT_RETURN_VOID() ;
}

// String getTag( ) const
IMPL_LOQUATY_FUNC(XMLDocument_getTag)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;

	LQT_RETURN_STRING( pThis->GetTag() ) ;
}

// void setTag( String tag, int type )
IMPL_LOQUATY_FUNC(XMLDocument_setTag)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( tag ) ;
	LQT_FUNC_ARG_INT( type ) ;

	pThis->SetTag( tag.c_str(), (LXMLDocument::ElementType) type ) ;

	LQT_RETURN_VOID() ;
}

// String getText( ) const
IMPL_LOQUATY_FUNC(XMLDocument_getText)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;

	LQT_RETURN_STRING( pThis->GetText() ) ;
}

// void setText( String text, int type )
IMPL_LOQUATY_FUNC(XMLDocument_setText)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( text ) ;
	LQT_FUNC_ARG_INT( type ) ;

	pThis->SetText( text.c_str(), (LXMLDocument::ElementType) type ) ;

	LQT_RETURN_VOID() ;
}

// boolean hasAttribute( String name ) const
IMPL_LOQUATY_FUNC(XMLDocument_hasAttribute)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;

	LQT_RETURN_BOOL( pThis->HasAttribute( name.c_str() ) ) ;
}

// boolean removeAttribute( String name )
IMPL_LOQUATY_FUNC(XMLDocument_removeAttribute)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;

	LQT_RETURN_BOOL( pThis->RemoveAttribute( name.c_str() ) ) ;
}

// void removeAllAttributes( )
IMPL_LOQUATY_FUNC(XMLDocument_removeAllAttributes)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;

	pThis->RemoveAllAttributes() ;

	LQT_RETURN_VOID() ;
}

// String getAttrString( String name, String strDefault ) const
IMPL_LOQUATY_FUNC(XMLDocument_getAttrString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;
	LQT_FUNC_ARG_STRING( strDefault ) ;

	LQT_RETURN_STRING( pThis->GetAttrString( name.c_str(), strDefault.c_str() ) ) ;
}

// long getAttrInteger( String name, long nDefault, int radix ) const
IMPL_LOQUATY_FUNC(XMLDocument_getAttrInteger)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;
	LQT_FUNC_ARG_LONG( nDefault ) ;
	LQT_FUNC_ARG_INT( radix ) ;

	LQT_RETURN_LONG( pThis->GetAttrLong( name.c_str(), nDefault, radix ) ) ;
}

// double getAttrNumber( String name, double nDefault ) const
IMPL_LOQUATY_FUNC(XMLDocument_getAttrNumber)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;
	LQT_FUNC_ARG_DOUBLE( nDefault ) ;

	LQT_RETURN_DOUBLE( pThis->GetAttrNumber( name.c_str(), nDefault ) ) ;
}

// void setAttrString( String name, String str )
IMPL_LOQUATY_FUNC(XMLDocument_setAttrString)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	pThis->SetAttrString( name.c_str(), str.c_str() ) ;

	LQT_RETURN_VOID() ;
}

// void setAttrInteger( String name, long value, int radix )
IMPL_LOQUATY_FUNC(XMLDocument_setAttrInteger)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;
	LQT_FUNC_ARG_LONG( value ) ;
	LQT_FUNC_ARG_INT( radix ) ;

	pThis->SetAttrLong( name.c_str(), value, radix ) ;

	LQT_RETURN_VOID() ;
}

// void setAttrNumber( String name, double value )
IMPL_LOQUATY_FUNC(XMLDocument_setAttrNumber)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;
	LQT_FUNC_ARG_DOUBLE( value ) ;

	pThis->SetAttrNumber( name.c_str(), value ) ;

	LQT_RETURN_VOID() ;
}

// void addAttrOrder( String name )
IMPL_LOQUATY_FUNC(XMLDocument_addAttrOrder)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( name ) ;

	pThis->AddAttrOrder( name.c_str() ) ;

	LQT_RETURN_VOID() ;
}

// ulong getElementCount( ) const
IMPL_LOQUATY_FUNC(XMLDocument_getElementCount)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;

	LQT_RETURN_ULONG( pThis->GetElementCount() ) ;
}

// XMLDocument getElementAt( ulong index ) const
IMPL_LOQUATY_FUNC(XMLDocument_getElementAt)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_ULONG( index ) ;

	LXMLDocPtr	xmlDoc = pThis->GetElementAt( (size_t) index ) ;
	if ( xmlDoc == nullptr )
	{
		LQT_RETURN_OBJECT( nullptr ) ;
	}
	LPtr<LNativeObj>
		pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( xmlDoc ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}

// XMLDocument getTagAs( String tag, ulong first ) const
IMPL_LOQUATY_FUNC(XMLDocument_getTagAs)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( tag ) ;
	LQT_FUNC_ARG_ULONG( first ) ;

	LXMLDocPtr	xmlDoc = pThis->GetTagAs( tag.c_str(), (size_t) first ) ;
	if ( xmlDoc == nullptr )
	{
		LQT_RETURN_OBJECT( nullptr ) ;
	}
	LPtr<LNativeObj>
		pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( xmlDoc ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}

// String getTextElement( ulong first ) const
IMPL_LOQUATY_FUNC(XMLDocument_getTextElement)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_ULONG( first ) ;

	LQT_RETURN_STRING( pThis->GetTextElement( (size_t) first ) ) ;
}

// long findElement( int type, String tag, ulong first ) const
IMPL_LOQUATY_FUNC(XMLDocument_findElement)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_INT( type ) ;
	LQT_FUNC_ARG_STRING( tag ) ;
	LQT_FUNC_ARG_ULONG( first ) ;

	LQT_RETURN_LONG
		( pThis->FindElement
			( (LXMLDocument::ElementType) type, tag.c_str(), (size_t) first ) ) ;
}

// long findElementPtr( XMLDocument xmlElement ) const
IMPL_LOQUATY_FUNC(XMLDocument_findElementPtr)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlElement ) ;

	LQT_RETURN_LONG( pThis->FindElement( xmlElement ) ) ;
}

// void addElement( XMLDocument xmlElement )
IMPL_LOQUATY_FUNC(XMLDocument_addElement)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlElement ) ;
	LQT_VERIFY_NULL_PTR( xmlElement ) ;

	pThis->AddElement( xmlElement ) ;

	LQT_RETURN_VOID() ;
}

// void insertElement( ulong index, XMLDocument xmlElement )
IMPL_LOQUATY_FUNC(XMLDocument_insertElement)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_ULONG( index ) ;
	LQT_FUNC_ARG_NOBJ( LXMLDocument, xmlElement ) ;
	LQT_VERIFY_NULL_PTR( xmlElement ) ;

	pThis->InsertElement( (size_t) index, xmlElement ) ;

	LQT_RETURN_VOID() ;
}

// XMLDocument createTagAs( String tag, ulong first )
IMPL_LOQUATY_FUNC(XMLDocument_createTagAs)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( tag ) ;
	LQT_FUNC_ARG_ULONG( first ) ;

	LPtr<LNativeObj>	pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( pThis->CreateTagAs( tag.c_str(), (size_t) first ) ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}

// void removeElementAt( ulong index )
IMPL_LOQUATY_FUNC(XMLDocument_removeElementAt)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_ULONG( index ) ;

	pThis->RemoveElementAt( (size_t) index ) ;

	LQT_RETURN_VOID() ;
}

// void removeAllElements( )
IMPL_LOQUATY_FUNC(XMLDocument_removeAllElements)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;

	pThis->RemoveAllElements() ;

	LQT_RETURN_VOID() ;
}

// XMLDocument getTagPathAs( String tagPath ) const
IMPL_LOQUATY_FUNC(XMLDocument_getTagPathAs)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( tagPath ) ;

	LPtr<LNativeObj>	pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( pThis->GetTagPathAs( tagPath.c_str() ) ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}

// XMLDocument createTagPathAs( String tagPath )
IMPL_LOQUATY_FUNC(XMLDocument_createTagPathAs)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LXMLDocument, pThis ) ;
	LQT_FUNC_ARG_STRING( tagPath ) ;

	LPtr<LNativeObj>	pXMLDoc( new LNativeObj( LQT_GET_CLASS(XMLDocument) ) ) ;
	pXMLDoc->SetNative( pThis->CreateTagPathAs( tagPath.c_str() ) ) ;

	LQT_RETURN_OBJECT( pXMLDoc ) ;
}



