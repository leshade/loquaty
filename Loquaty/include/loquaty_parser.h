
#ifndef	__LOQUATY_PARSER_H__
#define	__LOQUATY_PARSER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 文字列パーサー
	//////////////////////////////////////////////////////////////////////////

	class	LStringParser	: public LString
	{
	private:
		size_t	m_index ;
		size_t	m_iBoundFirst ;
		size_t	m_iBoundEnd ;

	protected:
		static std::uint32_t	s_maskPunctuation[4] ;		// 記号マスク
		static std::uint32_t	s_maskSpecialMark[4] ;		// 特殊記号マスク

		enum	SpecialOperator
		{
			opMoveShiftRight,			// >>=
			opMoveShiftLeft,			// <<=
			opShiftRight,				// >>
			opShiftLeft,				// <<
			opCompareLE,				// <=
			opCompareGE,				// >=
			opSpecialCount,
		} ;
		static const wchar_t *const	s_pwszSpecialOps[opSpecialCount] ;	// 特殊記号から始まる２文字以上の演算子

	public:
		LStringParser( void )
			: m_index( 0 ), m_iBoundFirst( 0 ), m_iBoundEnd( 0 ) {}
		LStringParser( const LStringParser& parser )
			: LString( parser ),
				m_index( parser.m_index ),
				m_iBoundFirst( parser.m_iBoundFirst ),
				m_iBoundEnd( parser.m_iBoundEnd ) {}
		LStringParser( const LString& str )
			: LString( str ), m_index( 0 ),
				m_iBoundFirst( 0 ), m_iBoundEnd( str.GetLength() ) {}
		LStringParser( const wchar_t * pwszText )
			: LString( pwszText ), m_index( 0 ),
				m_iBoundFirst( 0 ), m_iBoundEnd( SIZE_MAX )
		{
			m_iBoundEnd = LString::GetLength() ;
		}

		// 文字列代入
		const LStringParser& operator = ( const LStringParser& spars )
		{
			LString::operator = ( spars ) ;
			m_index = spars.m_index ;
			m_iBoundFirst = spars.m_iBoundFirst ;
			m_iBoundEnd = spars.m_iBoundEnd ;
			return	*this ;
		}
		const LStringParser& operator = ( const wchar_t * pwszStr )
		{
			m_index = 0 ;
			LString::operator = ( pwszStr ) ;
			return	*this ;
		}

		// 位置情報保存
		class	StateSaver
		{
		public:
			LStringParser&	m_spars ;
			size_t			m_index ;
			size_t			m_iBoundFirst ;
			size_t			m_iBoundEnd ;

		public:
			StateSaver( LStringParser& spars )
				: m_spars( spars ),
					m_index( spars.m_index ),
					m_iBoundFirst( spars.m_iBoundFirst ),
					m_iBoundEnd( spars.m_iBoundEnd ) {}
			~StateSaver( void )
			{
				m_spars.m_index = m_index ;
				m_spars.m_iBoundFirst = m_iBoundFirst ;
				m_spars.m_iBoundEnd = m_iBoundEnd ;
			}
		} ;

	protected:
		// 文字列を変更した後に呼び出される
		virtual void OnModified( void ) ;

	public:
		// 解釈位置
		size_t GetIndex( void ) const
		{
			return	m_index ;
		}
		void SeekIndex( size_t index )
		{
			m_index = index ;
		}
		size_t SkipIndex( ssize_t offset )
		{
			if ( offset >= 0 )
			{
				m_index += offset ;
			}
			else
			{
				m_index -= std::min( (size_t) -offset, m_index ) ;
			}
			return	m_index ;
		}

		// 終端に到達したか？
		bool IsEndOfString( void ) const
		{
			return	(m_index >= m_iBoundEnd) ;
		}

		// 領域設定
		void SetBounds( size_t iFirst, size_t iEnd ) ;

		// 次の1文字を取得する
		wchar_t CurrentChar( void ) const
		{
			return	IsEndOfString() ? 0 : LString::at(m_index) ;
		}
		// 現在の位置から指定位置の文字を取得する
		wchar_t CurrentAt( size_t index ) const
		{
			return	(m_index + index >= m_iBoundEnd)
							? 0 : LString::at(m_index + index) ;
		}
		// 次の1文字を取得して指標を進める
		wchar_t NextChar( void )
		{
			return	IsEndOfString() ? 0 : LString::at(m_index++) ;
		}

		// 空白文字を読み飛ばす（終端に到達した場合は false を返す）
		virtual bool PassSpace( void ) ;

		// 空白を除いた次の文字が指定のいずれかの文字であるならそれを取得する
		// 但し指標はその文字のまま進めない
		wchar_t CheckNextChars( const wchar_t * pwszNext ) ;

		// 空白を除いた次の文字が指定のいずれかの文字であるならそれを取得し次へ進める
		// 何れの文字でもないなら指標を進めず 0 を返す（但し空白は飛ばす）
		wchar_t HasNextChars( const wchar_t * pwszNext ) ;

		// 現在の位置から次の文字列が一致するかテストし、一致なら次へ進め true を返す
		// そうでないなら指標を進めず false を返す
		bool HasNextString( const wchar_t * pwszNext ) ;

		// 指定文字列と一致する次の位置まで指標を進め true を返す
		// そうでないなら末尾まで移動して false を返す
		bool SeekString( const wchar_t * pwszNext ) ;

		// 現在の位置から次のトークンが一致するかテストし、一致なら次へ進め true を返す
		// そうでないなら指標を進めず false を返す
		bool HasNextToken( const wchar_t * pwszNext ) ;

		// トークン種別
		enum	TokenType
		{
			tokenNothing,		// 空白（終端に到達した）
			tokenSymbol,		// 数字と記号以外の文字から始まる記号以外の文字列
			tokenNumber,		// 数字から始まる記号以外の文字列（数値リテラルではないことに注意）
			tokenMark,			// 記号文字列（但し特殊記号で分割される）
		} ;
		// １トークンの終端位置まで読み飛ばす
		virtual TokenType PassToken( void ) ;

		// 次の１トークンを取得して指標を進める
		virtual TokenType NextToken( LString& strToken ) ;

		// 現在の位置から指定記号で区切られている区間の文字列を取得する
		// 該当の文字を見つけるか終端に到達するとそこまでの文字列を取得し
		// １文字読み進め、区切り文字コードを返す（終端の場合 0）
		virtual wchar_t NextStringTerm
			( LString& strTerm, std::function<bool(wchar_t)> funcIsDeli ) ;
		// 現在の位置から空白文字で区切られた文字列
		virtual wchar_t NextStringTermBySpace( LString& strTerm ) ;
		// 現在の位置から指定のいずれかの文字で区切られた文字列
		virtual wchar_t NextStringTermByChars
			( LString& strTerm, const wchar_t * pwszChars ) ;
		// 現在の位置から行末までの文字列
		//（strLine に改行は含まず、改行を読み飛ばす。\r\n の場合は２文字読み飛ばす）
		virtual wchar_t NextLine( LString& strLine ) ;

		// 閉じ記号まで読み飛ばす
		virtual bool PassEnclosedString
			( wchar_t wchCloser, const wchar_t * pwszEscChars = nullptr ) ;

		// 閉じ記号までの文字列を取得し、閉じ記号を読み飛ばし true を返す
		// pwszEscChars 文字列に含まれるいずれかの文字を見つけた場合も
		// その文字までの区間の文字列を strEnclosed に取得するが
		// その文字は読み飛ばさず false を返す
		virtual bool NextEnclosedString
			( LString& strEnclosed,
				wchar_t wchCloser, const wchar_t * pwszEscChars = nullptr ) ;

		// 空白文字判定
		static bool IsCharSpace( wchar_t wch )
		{
			return	(wch <= L' ') || (wch == 0xA0) || (wch == 0xFEFF) ;
		}
		// 数字文字判定
		static bool IsCharNumber( wchar_t wch )
		{
			return	(wch >= L'0') && (wch <= L'9') ;
		}
		// 記号か？
		static bool IsPunctuation( wchar_t wch )
		{
			return	!(wch & ~0x7F)
				&& (s_maskPunctuation[wch >> 5] & (1 << (wch & 0x1F))) ;
		}
		// 特殊区切り記号か？
		static bool IsSpecialMark( wchar_t wch )
		{
			return	!(wch & ~0x7F)
				&& (s_maskSpecialMark[wch >> 5] & (1 << (wch & 0x1F))) ;
		}
		// 文字列に文字が含まれるか？（pwszChars==nullptr の場合は常に false）
		static bool IsCharContained( wchar_t wch, const wchar_t * pwszChars ) ;

		// 文字指標に該当する行を探す
		struct	LineInfo
		{
			LUlong	iLine ;		// 行番号（1 から始まる / 0 は見つからなかった）
			LUlong	iStart ;	// 行先頭の文字指標
			LUlong	iEnd ;		// 次の行の先頭文字指標
		} ;
		LineInfo FindLineContainingIndexAt
					( size_t index, wchar_t wchRet = L'\n' ) const ;

	public:
		// シンボルとして有効な文字列か？
		static bool IsValidAsSymbol( const wchar_t * pwszStr ) ;
		// 文字列リテラルとして特殊文字を \ 記号エンコードする
		static LString EncodeStringLiteral( const wchar_t * pwszStr, size_t nStrLen = 0 ) ;
		// 文字列リテラルとして \ 記号エスケープをデコードする
		static LString DecodeStringLiteral( const wchar_t * pwszStr, size_t nStrLen = 0 ) ;

	public:
		// 構文を指定の文字を見つけるまで読み飛ばす
		// トークン単位でシークし、pwszClosers 中の何れかを見つけたら
		// 見つけた文字を返し、その次の文字へ指標を移動する
		// その前に pwszEscChars の中のいずれかの文字を見つけた場合、
		// 指標は移動せず 0 を返す
		// 但し、括弧の入れ子や、文字列リテラルなどの中にある pwszClosers は
		// 無視される
		virtual wchar_t PassStatement
			( const wchar_t * pwszClosers, const wchar_t * pwszEscChars = nullptr ) ;

		// 文の末尾まで読み飛ばす
		// セミコロンを見つけたらその文字を
		// 途中で '{' 括弧を見つけたら '}' 閉じ括弧までを読み飛ばしその文字を返す
		// ※ '... ;' 又は '... { ... }' 形式の文の読み飛ばし
		virtual wchar_t PassStatementBlock( void ) ;

		// 現在の位置から型を解釈（型表現の終端まで移動する）
		// ['const']opt ObjectType ['[]'...]
		// ['const']opt 'Array' '<' ObjectType '>' ['[]'...]
		// ['const']opt 'Map' '<' ObjectType '>' ['[]'...]
		// ['const']opt 'Function' '<' <proto> '>' ['[]'...]
		// ['const']opt 'Task' '<' ReturnType '>' ['[]'...]
		// ['const']opt 'Thread' '<' ReturnType '>' ['[]'...]
		// ['const']opt DataType ['['<dim-size>']'...]opt [*]opt
		bool NextTypeExpr
			( LType& type, bool mustHave,
				LVirtualMachine& vm, const LNamespaceList * pnslNamespace,
				LType::Modifiers accModAdd = 0,
				bool arrayModifiable = true, bool instantiateGenType = true ) ;
		// 型名を解釈（一致する名前の型、又は名前の後ろに引数があるジェネリック型）
		bool ParseTypeName
			( LType& type, bool mustHave,
				const wchar_t * pwszName,
				LVirtualMachine& vm,
				const LNamespaceList * pnslNamespace,
				LType::Modifiers accModAdd = 0, bool instantiateGenType = true ) ;
		// ジェネリック型の引数を解釈
		bool ParseGenericArgList
			( std::vector<LType>& vecTypes, LString& strGenTypeName,
				bool mustHave, const wchar_t * pwszName,
				LVirtualMachine& vm, const LNamespaceList * pnslNamespace ) ;

		// 関数プロトタイプ型表記を解釈
		// <ret-type> '(' <arg-type-list> ')' ['['<capture-type-list>']'] [<this-type>]opt
		bool NextPrototype
			( LPrototype& proto, bool mustHave,
				LVirtualMachine& vm,
				const LNamespaceList * pnslNamespace,
				bool instantiateGenType = true ) ;

		// この文字列を読み込んだ元のファイル名を取得する
		virtual LString GetFileName( void ) const ;
		// 開いたディレクトリを取得する
		virtual LDirectoryPtr GetFileDirectory( void ) const ;

	} ;



	//////////////////////////////////////////////////////////////////////////
	// 区間情報
	//////////////////////////////////////////////////////////////////////////

	class	LParserSegment	: public Object
	{
	public:
		LStringParser *	m_pParser ;
		size_t			m_iFirst ;
		size_t			m_iEnd ;

	public:
		LParserSegment
			( LStringParser * pParser = nullptr,
				size_t iFirst = 0, size_t iEnd = SIZE_MAX )
			: m_pParser( pParser ), m_iFirst( iFirst ), m_iEnd( iEnd ) {}
		LParserSegment( const LParserSegment& ps )
			: m_pParser( ps.m_pParser ),
				m_iFirst( ps.m_iFirst ), m_iEnd( ps.m_iEnd ) {}
		const LParserSegment& operator = ( const LParserSegment& ps )
		{
			m_pParser = ps.m_pParser ;
			m_iFirst = ps.m_iFirst ;
			m_iEnd = ps.m_iEnd ;
			return	*this ;
		}
	} ;

}

#endif

