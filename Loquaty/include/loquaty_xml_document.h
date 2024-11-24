
#ifndef	__LOQUATY_XML_DOCUMENT_H__
#define	__LOQUATY_XML_DOCUMENT_H__	1

namespace	Loquaty
{

	//////////////////////////////////////////////////////////////////////////
	// XML ライクな簡易文書パーサー
	//////////////////////////////////////////////////////////////////////////

	class	LXMLDocParser	: public LTextFileParser
	{
	protected:
		std::map<LString,LString>	m_mapDTDEntities ;
		bool						m_flagStrictText ;
		size_t						m_nErrors ;

	public:
		LXMLDocParser( void )
			: m_flagStrictText( false ), m_nErrors( 0 ) {}
		LXMLDocParser( const LTextFileParser& src )
			: LTextFileParser( src ),
				m_flagStrictText( false ) , m_nErrors( 0 ){}
		LXMLDocParser( const LStringParser& spars )
			: LTextFileParser( spars ),
				m_flagStrictText( false ), m_nErrors( 0 ) {}
		LXMLDocParser( const LString& str )
			: LTextFileParser( str ),
				m_flagStrictText( false ), m_nErrors( 0 ) {}
		LXMLDocParser( const wchar_t * pwszText )
			: LTextFileParser( pwszText ),
				m_flagStrictText( false ), m_nErrors( 0 ) {}

		// 代入
		const LXMLDocParser& operator = ( const LXMLDocParser& xpars ) ;

	public:
		// ドキュメント全体を解釈
		virtual std::shared_ptr<LXMLDocument> ParseDocument( void ) ;
		// １つの要素を解釈
		virtual std::shared_ptr<LXMLDocument> ParseElement( void ) ;
		// <!DOCTYPE ... > 解釈
		void ParseDocTypeDeclaration( void ) ;

		// 属性リスト解釈
		void ParseAttributeList
			( std::shared_ptr<LXMLDocument> pDoc,
				const wchar_t * pwszEscChars, bool withOrder = false ) ;
		// 属性名取得
		LString ParseName( const wchar_t * pwszEscChars = nullptr ) ;
		// 属性値取得
		LString ParseValue( const wchar_t * pwszEscChars = nullptr ) ;

		// 指定文字まで読み飛ばす（入れ子解釈あり）
		virtual wchar_t PassDocument
			( const wchar_t * pwszClosers, const wchar_t * pwszEscChars = nullptr ) ;

		// LXMLDocument 生成
		virtual std::shared_ptr<LXMLDocument> NewDocument( void ) ;

		// エラー数
		size_t GetErrorCount( void ) const ;
		// エラー出力
		virtual void OnError( const wchar_t * pwszErrMsg ) ;

	public:
		// ドキュメントをシリアライズ
		bool SaveToFile
			( const wchar_t * pwszFile,
				const LXMLDocument& xmlDoc,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;
		void Serialize
			( LOutputStream& strm,
				const LXMLDocument& xmlDoc,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;
		void Serialize
			( LString& strXmlDoc,
				const LXMLDocument& xmlDoc,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;

		// 要素をシリアライズ
		void SerializeElements
			( LOutputStream& strm,
				const LXMLDocument& xmlDoc,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;
		void SerializeElements
			( LString& strXmlDoc,
				const LXMLDocument& xmlDoc,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;
		// タグをシリアライズ
		LString SerializeTag
			( const LXMLDocument& xmlDoc,
				const wchar_t * pwszLeader,
				const wchar_t * pwszCloser,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;
		// 属性をシリアライズ
		LString SerializeAtributes
			( const LXMLDocument& xmlDoc,
				size_t nFirstLineLeft = 0,
				size_t nReadableLineWidth = 60,
				const wchar_t * pwszReadableIndent = nullptr ) const ;

	public:
		// 空白の扱い（false の時は可読性優先）
		bool IsStrictText( void ) const
		{
			return	m_flagStrictText ;
		}
		void SetStrictText( bool flagStrict )
		{
			m_flagStrictText = flagStrict ;
		}

		// Document Type Declaration Entities
		std::map<LString,LString>& DTDEntities( void )
		{
			return	m_mapDTDEntities ;
		}
		const std::map<LString,LString>& GetDTDEntities( void ) const
		{
			return	m_mapDTDEntities ;
		}

	public:
		// XML エスケープ処理
		static LString EncodeXMLString
			( const wchar_t * pwszStr,
				const wchar_t * pwszReadableIndent = nullptr ) ;
		// XML エスケープ復号処理
		static LString DecodeXMLString
			( const wchar_t * pwszXML,
				bool flagStrict = false,
				const std::map<LString,LString> * pDTD = nullptr ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// XML ライクな簡易文書データ
	//////////////////////////////////////////////////////////////////////////

	class	LXMLDocument	: public Object
	{
	public:
		enum	ElementType
		{
			typeRoot,
			typeTag,
			typeDifinition,
			typeText,
			typeCDATA,
			typeComment,
		} ;

	protected:
		ElementType					m_type ;		// データの種類
		LString						m_tag ;			// タグ名
		LString						m_text ;		// テキスト, CDATA, コメント
		std::map<LString,LString>	m_attributes ;	// 属性データ
		std::vector<LString>		m_attrOrder ;
		std::vector< std::shared_ptr<LXMLDocument> >
									m_elements ;	// 要素

	public:
		LXMLDocument( void ) ;
		LXMLDocument( const wchar_t * pwszTag ) ;
		LXMLDocument( ElementType type, const wchar_t * pwszText = nullptr ) ;
		LXMLDocument( const LXMLDocument& xml ) ;

		const LXMLDocument& operator = ( const LXMLDocument& xml ) ;

	public:
		// データの種類
		ElementType GetType( void ) const
		{
			return	m_type ;
		}
		void SetType( ElementType type )
		{
			m_type = type ;
		}
		// タグ名取得
		const LString& GetTag( void ) const
		{
			return	m_tag ;
		}
		void SetTag( const wchar_t * pwszTag, ElementType type = typeTag )
		{
			m_type = type ;
			m_tag = pwszTag ;
		}
		// テキスト取得
		const LString& GetText( void ) const
		{
			return	m_text ;
		}
		void SetText( const LString& strText, ElementType type = typeText )
		{
			m_type = type ;
			m_text = strText ;
		}
		void SetText( const wchar_t * pwszText, ElementType type = typeText )
		{
			m_type = type ;
			m_text = pwszText ;
		}

	public:
		// 属性は存在するか？
		bool HasAttribute( const wchar_t * pwszName ) const ;
		// 属性を削除
		bool RemoveAttribute( const wchar_t * pwszName ) ;
		void RemoveAllAttributes( void ) ;

		// 属性値を取得
		LString GetAttrString
				( const wchar_t * pwszName,
					const wchar_t * pwszDefault = nullptr ) const ;
		LInt GetAttrInteger
				( const wchar_t * pwszName,
					LInt nDefault = 0, int nRadix = 10 ) const ;
		LLong GetAttrLong
				( const wchar_t * pwszName,
					LLong nDefault = 0, int nRadix = 10 ) const ;
		LDouble GetAttrNumber
				( const wchar_t * pwszName, LDouble nDefault = 0.0 ) const ;

		// 属性値を設定
		void SetAttrString
				( const wchar_t * pwszName, const wchar_t * pwszString ) ;
		void SetAttrInteger
				( const wchar_t * pwszName, LInt nValue, int nRadix = 10 ) ;
		void SetAttrLong
				( const wchar_t * pwszName, LLong nValue, int nRadix = 10 ) ;
		void SetAttrNumber
				( const wchar_t * pwszName, LDouble nValue ) ;

		// シリアライズ時の属性値の順序を追加する
		void AddAttrOrder( const wchar_t * pwszName ) ;

		// 属性配列
		std::map<LString,LString>& Attributes( void )
		{
			return	m_attributes ;
		}
		const std::map<LString,LString>& GetAttributes( void ) const
		{
			return	m_attributes ;
		}
		const std::vector<LString>& GetAttrOrder( void ) const
		{
			return	m_attrOrder ;
		}

	public:
		// 要素数取得
		size_t GetElementCount( void ) const ;

		// 要素取得
		std::shared_ptr<LXMLDocument> GetElementAt( size_t iElement ) const ;
		std::shared_ptr<LXMLDocument>
				GetTagAs( const wchar_t * pwszTag, size_t iFirst = 0 ) const ;
		LString GetTextElement( size_t iFirst = 0 ) const ;
		ssize_t FindElement
			( ElementType type,
				const wchar_t * pwszTag = nullptr, size_t iFirst = 0 ) const ;
		ssize_t FindElement( std::shared_ptr<LXMLDocument> pElement ) const ;

		// 要素追加
		void AddElement( std::shared_ptr<LXMLDocument> pElement ) ;
		void InsertElement
				( size_t index, std::shared_ptr<LXMLDocument> pElement ) ;

		// タグが存在していればそのタグを取得
		// 存在しない場合には作成して追加して取得
		std::shared_ptr<LXMLDocument>
			CreateTagAs( const wchar_t * pwszTag, size_t iFirst = 0 ) ;

		// 要素削除
		void RemoveElementAt( size_t iElement ) ;
		void RemoveAllElements( void ) ;

		// 要素配列
		std::vector< std::shared_ptr<LXMLDocument> >& Elements( void )
		{
			return	m_elements ;
		}
		const std::vector< std::shared_ptr<LXMLDocument> >& GetElements( void ) const
		{
			return	m_elements ;
		}

	public:
		// 階層検索 (タグ名を '>' 記号で区切って階層指定)
		std::shared_ptr<LXMLDocument>
				GetTagPathAs( const wchar_t * pwszTagPath ) const ;

		// 階層取得 (タグ名を '>' 記号で区切って階層指定)
		// 存在しない場合には作成する
		std::shared_ptr<LXMLDocument>
				CreateTagPathAs( const wchar_t * pwszTagPath ) ;


	} ;

	typedef	std::shared_ptr<LXMLDocument>	LXMLDocPtr ;

}


#endif
