
#ifndef	__LOQUATY_COMPILER_H__
#define	__LOQUATY_COMPILER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// コンパイラ
	//////////////////////////////////////////////////////////////////////////

	class	LCompiler	: public Object
	{
	private:
		// 予約語
		std::map<std::wstring,const Symbol::ReservedWordDesc*>	m_mapReservedWordDescs ;

		// 演算子
		std::map<std::wstring,const Symbol::OperatorDesc*>		m_mapOperatorDescs ;

	public:
		// 警告レベル
		enum	WarningLevel
		{
			warning0,	// 全ての警告を抑制
			warning1,	// 重大な警告（レベル１）のみ
			warning2,	// レベル１と２の警告を出力
			warning3,	// デフォルト
			warning4,	// 全ての警告と情報を出力
		} ;

		class	CodePoint ;
		typedef	std::shared_ptr<CodePoint>	CodePointPtr ;

		// 変数遅延初期化情報
		class	DelayInitVar	: public LParserSegment
		{
		public:
			LObjPtr					m_pRefObj ;
			LArrangementBuffer *	m_pArrange ;
			LString					m_strName ;
			const LNamespaceList *	m_pnslLocal ;

		public:
			DelayInitVar
				( const LParserSegment& ps,
					LObjPtr pRefObj,
					LArrangementBuffer *pArrange,
					const wchar_t * pwszVarName,
					const LNamespaceList * pnslLocal )
				: LParserSegment( ps ), m_pRefObj( pRefObj ),
					m_pArrange( pArrange ), m_strName( pwszVarName ),
					m_pnslLocal( pnslLocal ) {}
		} ;
		typedef	std::shared_ptr<DelayInitVar>	DelayInitVarPtr ;

		// 関数遅延実装情報
		class	DelayImplement	: public LParserSegment
		{
		public:
			LPtr<LFunctionObj>	m_pFunc ;
			LNamespaceList		m_nslLocal ;

		public:
			DelayImplement
				( const LParserSegment& ps,
					LPtr<LFunctionObj> pFunc,
					const LNamespaceList& nslLocal )
				: LParserSegment( ps ),
					m_pFunc( pFunc ),
					m_nslLocal( nslLocal ) {}
		} ;
		typedef	std::shared_ptr<DelayImplement>	DelayImplementPtr ;

		// 入れ子空間
		class	CodeNest	: public Object
		{
		public:
			enum	ControlType
			{
				ctrlNo,
				ctrlFunction,
				ctrlStatementList,
				ctrlIf,		ctrlSwitch,
				ctrlWhile,	ctrlDo,		ctrlFor,
				ctrlTry,	ctrlCatch,	ctrlFinally,
				ctrlWith,	ctrlSynchronized,
				ctrlClass,	ctrlStruct,	ctrlNamespace,
			} ;
			std::shared_ptr<CodeNest>	m_prev ;
			ControlType					m_type ;
			LPtr<LNamespace>			m_namespace ;	// class, namespace
			LNamespaceList				m_nslUsing ;	// using namespace
			LLocalVarArrayPtr			m_frame ;
			LLocalVarPtr				m_var ;			// switch 分岐値
			LExprValuePtr				m_xvalObj ;		// with, synchronized
			CodePointPtr				m_cpPushExcept ;// try
			CodePointPtr				m_cpPushFinally ;
			CodePointPtr				m_cpJumpNext ;	// switch 次の case へのジャンプ
			CodePointPtr				m_cpDefault ;	// default ラベル位置
			CodePointPtr				m_cpContinue ;	// continue ジャンプ先
			std::vector<CodePointPtr>	m_vecBreaks ;	// break ジャンプ元位置
			std::vector<CodePointPtr>	m_vecContinues ;// continue ジャンプ元位置
			CodePointPtr				m_cpVarBegin ;	// ローカル変数開始位置
			bool						m_forever ;		// 無限ループ
			bool						m_returned ;	// return があった？

			std::vector<DelayInitVarPtr>	m_vecDelayInitVars ;
			std::vector<DelayImplementPtr>	m_vecDelayImplements ;

		public:
			CodeNest( ControlType type = ctrlNo )
				: m_type( type ),
					m_forever( false ),
					m_returned( false ) {}
			LLocalVarArrayPtr MakeFrame( void ) ;
		} ;
		typedef	std::shared_ptr<CodeNest>	CodeNestPtr ;

		// ジャンプ命令用ポインタ情報
		class	CodePoint	: public Object
		{
		public:
			size_t			m_src ;			// ソース上の位置
			size_t			m_pos ;			// ジャンプ命令のコード位置
			bool			m_bind ;		// 命令位置か？ true:命令位置, false:ジャンプ先
			CodeNestPtr		m_nest ;		// コード位置の入れ子空間
			std::vector<LLocalVarArrayPtr>
							m_locals ;		// ローカル状態
			size_t			m_stack ;		// 使用一時スタック

		public:
			CodePoint( bool bind )
				: m_src( 0 ), m_pos( SIZE_MAX ),
					m_bind( bind ), m_stack( 0 ) {}
			CodePoint( const CodePoint& cp ) ;
			const CodePoint& operator = ( const CodePoint& cp ) ;
			void Release( void ) ;			// ※ m_nest の循環参照解除のため必須
		} ;

		// デバッグ用ローカル変数配置情報
		class	DebugLocalVarInfo
		{
		public:
			LLocalVarArrayPtr	m_varInfo ;
			CodePointPtr		m_cpFirst ;
			CodePointPtr		m_cpEnd ;

		public:
			DebugLocalVarInfo( void ) { }
			DebugLocalVarInfo( const DebugLocalVarInfo& dlvi ) ;
			const DebugLocalVarInfo& operator =( const DebugLocalVarInfo& dlvi ) ;
		} ;

		// ジャンプ命令情報
		class	JumpPoint	: public Object
		{
		public:
			CodePointPtr	m_cpJump ;		// 移動元ジャンプ命令の位置
			CodePointPtr	m_cpDst ;		// ジャンプ先

		public:
			JumpPoint( void ) {}
			JumpPoint( const JumpPoint& jp ) ;
			JumpPoint( CodePointPtr cpJump, CodePointPtr cpDst ) ;
			const JumpPoint& operator = (const JumpPoint& jp ) ;
		} ;

		// 前方参照ジャンプ命令情報
		class	ForwardJumpPoint	: public Object
		{
		public:
			CodePointPtr	m_cpJump ;		// 移動元ジャンプ命令の位置
			LString			m_strDstLabel ;	// ジャンプ先ラベル名

		public:
			ForwardJumpPoint( void ) {}
			ForwardJumpPoint( const ForwardJumpPoint& fjp ) ;
			ForwardJumpPoint( CodePointPtr cpJump, const wchar_t * pwszLabel ) ;
			const ForwardJumpPoint& operator = (const ForwardJumpPoint& fjp ) ;
		} ;

		// ラベル情報
		class	LabelEntry	: public Object
		{
		public:
			CodePointPtr	m_cpLabel ;		// ラベルの位置

		public:
			LabelEntry( CodePointPtr cp ) : m_cpLabel(cp) {}
			LabelEntry( const LabelEntry& label ) ;
			const LabelEntry& operator = ( const LabelEntry& label ) ;
		} ;

		// fetch_addr ローカル変数情報
		class	FetchAddrVar
		{
		public:
			CodePointPtr	m_cpFetch ;		// fetch 命令の位置
			LLocalVarPtr	m_pVar ;		// 割り当てられたローカル変数

		public:
			FetchAddrVar( void ) {}
			FetchAddrVar( const FetchAddrVar& fav ) ;
			FetchAddrVar( CodePointPtr cpFetch, LLocalVarPtr pVar ) ;
			const FetchAddrVar& operator = ( const FetchAddrVar& fav ) ;
		} ;

		// コンパイル文脈情報
		class	Context	: public Object
		{
		public:
			std::shared_ptr<Context>			m_prev ;
			LString								m_name ;		// 関数名
			std::shared_ptr<LPrototype>			m_proto ;		// 関数プロトタイプ
			CodeNestPtr							m_curNest ;		// 入れ子空間
			std::shared_ptr<LCodeBuffer>		m_codeBuf ;		// 出力先コードバッファ
			bool								m_codeBarrier ;
			size_t								m_iBeginFunc ;	// 初期化コード長
			size_t								m_iCurStCode ;	// 現在の文の開始コード位置
			LExprValueArray						m_xvaStack ;	// スタックの状態
			std::map<std::wstring,LabelEntry>	m_mapLabels ;	// ラベル情報
			std::vector<ForwardJumpPoint>		m_jumpForwards ;// 前方参照ジャンプ
			std::vector<CodePointPtr>			m_codePoints ;	// CodePointPtr 履歴
			std::vector<JumpPoint>				m_jumpPoints ;	// ジャンプ位置情報
			std::vector<FetchAddrVar>			m_fetchVars ;	// fetch_addr 変数
			std::vector<DebugLocalVarInfo>		m_dbgVarInfos ;	// デバッグ情報
		public:
			Context( void ) ;
			void Release( void ) ;
		} ;
		typedef	std::shared_ptr<Context>	ContextPtr ;

	protected:
		// 仮想マシン
		LVirtualMachine&					m_vm ;

		// コンパイル中のソースコード
		LStringParser *						m_pSource ;
		size_t								m_iSrcStatement ;	// 現在の文の先頭位置
		LString								m_commentBefore ;	// 直前に記述されたコメント

		// コンパイル中の関数
		ContextPtr							m_ctx ;

		// 警告出力レベル
		int									m_warningLevel ;

		// エラー数
		size_t								m_nErrors ;
		size_t								m_nWarnings ;

		ErrorMessageIndex					m_hasCurError ;

		// 現在の LCompiler
		static thread_local LCompiler *	t_pCurrent ;

	public:
		LCompiler( LVirtualMachine& vm, LStringParser * src = nullptr ) ;
		~LCompiler( void ) ;

	public:
		// 現在のスレッドに設定された LCompiler 取得
		static LCompiler * GetCurrent( void )
		{
			return	t_pCurrent ;
		}

		// スレッドへ設定
		class	Current
		{
		private:
			LCompiler *	m_prev ;
		public:
			Current( LCompiler& compiler )
			{
				m_prev = t_pCurrent ;
				t_pCurrent = &compiler ;
			}
			~Current( void )
			{
				t_pCurrent = m_prev ;
			}
		} ;

		// 仮想マシン
		LVirtualMachine& VM( void ) const
		{
			return	m_vm ;
		}


		//////////////////////////////////////////////////////////////////////
		// コンパイル
		//////////////////////////////////////////////////////////////////////

		// ソースファイルをコンパイルする
		void DoCompile( LStringParser * pSource ) ;
		void IncludeScript
			( const wchar_t * pwszFileName, LDirectory * pDirPath = nullptr ) ;

		// エラー発生数
		size_t GetErrorCount( void ) const
		{
			return	m_nErrors ;
		}
		size_t GetWarningCount( void ) const
		{
			return	m_nWarnings ;
		}

		// 警告レベルを設定
		void SetWarningLevel( int warnLevel )
		{
			m_warningLevel = warnLevel ;
		}


		//////////////////////////////////////////////////////////////////////
		// 数式解釈
		//////////////////////////////////////////////////////////////////////

		// ※数式系関数名規則
		//   Eval～      : 定数式 / 実行時式・両対応
		//   Parse～     : Eval と同じだが特定の書式の解釈を意味する
		//   Expr～      : 実行時式としてコードを出力
		//   ConstExpr～ : 定数式としてのみ評価
		//   Get～       : エラーは出力しない / nullptr を返しうる

		// ※数式系関数動作補足
		// 〇LExprValuePtr はコンパイル時の型情報と定数式の評価値を保持する。
		// 〇更に実行時式の場合、ローカル変数、又はスタック自体を表現する。
		// 〇LExprValuePtr で参照されている間、それに対応する実行時のスタックは解放されない。
		// 〇演算子などの処理で、引数を解放して結果を受け取りたい場合には
		//   他に何も LExprValuePtr を共有していない状態で std::move() で受け渡す。
		// 〇逆に LExprValuePtr を保持したまま新しい値をプッシュしたい場合には
		//   LExprValuePtr を保持したままにしておく。

	public:
		// 数式解釈
		LExprValuePtr EvaluateExpression
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr,
				int nEscPriority = Symbol::priorityLowest ) ;
		// 定数式解釈
		static LValue EvaluateConstExpr
			( LStringParser& sparsExpr,
				LVirtualMachine& vm,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr,
				int nEscPriority = Symbol::priorityLowest ) ;
		static LLong EvaluateConstExprAsLong
			( LStringParser& sparsExpr,
				LVirtualMachine& vm,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr,
				int nEscPriority = Symbol::priorityLowest ) ;

	public:
		// 演算子情報取得
		static const Symbol::OperatorDesc&
				GetOperatorDesc( Symbol::OperatorIndex opIndex ) ;
		// 演算子解釈
		Symbol::OperatorIndex
				AsOperatorIndex( const wchar_t * pwszOp ) const ;

		// 予約語情報取得
		static const Symbol::ReservedWordDesc&
				GetReservedWordDesc( Symbol::ReservedWordIndex rwIndex ) ;
		// 予約語解釈
		Symbol::ReservedWordIndex
				AsReservedWordIndex( const wchar_t * pwszName ) const ;
		// 予約語評価
		LExprValuePtr ParseReservedWord
			( LStringParser& sparsExpr,
				Symbol::ReservedWordIndex rwIndex,
				const LNamespaceList * pnslLocal = nullptr ) ;

	public:
		// 数値リテラル解釈
		static LExprValuePtr ParseNumberLiteral( LStringParser& sparsExpr ) ;
		// 文字／文字列リテラル解釈
		LExprValuePtr ParseStringLiteral
				( LStringParser& sparsExpr, wchar_t wchQuotation ) ;
		// 配列リテラル解釈
		LExprValuePtr ParseArrayLiteral
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr,
				LClass * pCastElementType = nullptr ) ;
		// 辞書配列リテラル解釈
		LExprValuePtr ParseMapLiteral
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr,
				LClass * pCastElementType = nullptr ) ;
		// 辞書配列リテラルの要素名解釈
		bool ParseMapLiteralElementName
				( LString& strName, LStringParser& sparsExpr ) ;
		// 名前解決（見つからなければ nullptr）
		LExprValuePtr GetExprSymbolAs
			( const wchar_t * pwszName,
				LStringParser * psparsExpr,
				const LNamespaceList * pnslLocal = nullptr ) ;
		// 名前空間から探す（見つからなければ nullptr）
		LExprValuePtr GetSymbolWithNamespaceAs
			( const wchar_t * pwszName,
				LStringParser * psparsExpr, LNamespace * pnsLocal,
				const LNamespaceList * pnslLocal ) ;
		// 適合する静的な関数を探す（見つからなければ nullptr）
		LPtr<LFunctionObj> GetStaticFunctionAs
			( const wchar_t * pwszName,
				const LArgumentListType& arglist,
				const LNamespaceList * pnslLocal ) ;
		LPtr<LFunctionObj> GetStaticFunctionOfNamespaceAs
			( const wchar_t * pwszName,
				const LArgumentListType& arglist, LNamespace * pnsLocal ) ;
		// ジェネリック型の解釈・インスタンス化
		LPtr<LNamespace> ParseGenericType
			( LStringParser& sparsExpr,
				const wchar_t * pwszName,
				const LNamespace::GenericDef * pGenType,
				bool instantiateGenType,
				const LNamespaceList * pnslLocal ) ;
		// バッファを参照する式をポインタに変換可能なら変換する（型情報の正規化のみ）
		LExprValuePtr DiscloseRefPointerType( LExprValuePtr xvalRef ) ;
		// メンバから探す（見つからなければ nullptr）
		LExprValuePtr GetExprVarMember
			( LExprValuePtr xvalVar, const wchar_t * pwszName ) ;
		// ローカル変数を探す（見つからなければ nullptr）
		LExprValuePtr GetLocalVariable( const wchar_t * pwszName ) ;
		// this 取得（見つからなければ nullptr）
		LExprValuePtr GetThisObject( void ) ;
		// super 取得（見つからなければ nullptr）
		LExprValuePtr GetSuperObject( const LNamespaceList * pnslLocal = nullptr ) ;
		// this クラスを取得（見つからなければ nullptr / 構造体ポインタの場合には構造体クラス）
		LClass * GetThisClass( void ) ;
		// 評価値のクラスを取得（構造体ポインタの場合に構造体を取得）
		static LClass * GetExprVarClassOf( LExprValuePtr xval ) ;
		// 二つのクラスの共通クラスを取得する
		LClass * GetCommonClasses( LClass * pClass1, LClass * pClass2 ) const ;
		// 現在の文脈から見える名前空間をリストに追加する
		void AddScopeToNamespaceList
			( LNamespaceList& nslNamespace,
				const LNamespaceList * pnslLocal = nullptr ) const ;
		// ローカル／スタック上での型に変換する（構造体の場合にポインタにする）
		LType GetLocalTypeOf( const LType& type, bool constModify = false ) const ;
		// 引数での型に変換する（データ配列や構造体の場合にポインタにする）
		LType GetArgumentTypeOf( const LType& type, bool constModify = false ) const ;
		// アクセス可能なスコープを取得
		LType::AccessModifier GetAccessibleScope( LClass * pClass ) ;
		// アクセス可能か？
		bool IsAccessibleTo( LClass * pClass, LType::AccessModifier accMode );

	public:
		// expr[, expr [, ...]] リスト（主に関数引数）評価
		bool ParseExprList
			( LStringParser& sparsExpr,
				std::vector<LExprValuePtr>& xvalList,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr ) ;
		// リストの型情報を取得
		void GetExprListTypes
			( LArgumentListType& argListType,
				std::vector<LExprValuePtr>& xvalList ) const ;
		// リストが全て定数式か？
		bool IsExprListConstExpr( std::vector<LExprValuePtr>& xvalList ) const ;

	public:
		// function 式 : function[capture-list](arglist)class:ret-type{statement-list}
		LExprValuePtr ParseFunctionExpr
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr ) ;
		bool ParseFunctionCaptureList
			( LStringParser& sparsExpr,
				std::vector<LString>& vecCaptureNames,
				std::vector<LExprValuePtr>& xvalCaptureList,
				const LNamespaceList * pnslLocal = nullptr ) ;
		LExprValuePtr ExprFunctionInstance
			( LStringParser& sparsExpr,
				std::shared_ptr<LPrototype> pPrototype,
				std::vector<LString>& vecCaptureNames,
				std::vector<LExprValuePtr> xvalCaptureList,
				const LNamespaceList * pnslLocal = nullptr ) ;

	public:
		// オブジェクト構築 : Type(arg-list)
		// 配列知テラル構築 : Array<Type>[ ... ]
		// 辞書配列リテラル構築 : Map<Type>{ ... }
		// タスクリテラル構築 : Task<Type>[]{ ... }
		// スレッドリテラル構築 : Thread<Type>[]{ ... }
		LExprValuePtr ParseTypeConstruction
			( LStringParser& sparsExpr,
				LClass * pTypeClass,
				const LNamespaceList * pnslLocal = nullptr ) ;
		// オブジェクト型の配列修飾
		void ParseTypeObjectArray( LStringParser& sparsExpr, LType& type ) ;
		// new 演算子 : new Type[]...(arg-list)
		LExprValuePtr ParseNewOperator
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr ) ;
		// sizeof 演算子
		LExprValuePtr ParseSizeOfOperator
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr ) ;
		LExprValuePtr EvalSizeOfType( const LType& type ) ;
		LExprValuePtr ExprSizeOfExpr( LExprValuePtr xval ) ;
		// 前置 :: 演算子
		LExprValuePtr ParseGlobalMemberOf( LStringParser& sparsExpr ) ;
		// 単項演算
		LExprValuePtr EvalUnaryOperator
			( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval ) ;
	protected:
		// ポインタ参照 *ptr 演算子
		LExprValuePtr EvalPrimitiveUnaryRefPtr( LExprValuePtr xval ) ;
		// プリミティブ型の単項演算子
		LExprValuePtr EvalPrimitiveUnaryOperator
			( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval ) ;
		// ポインタ型の単項演算子
		LExprValuePtr EvalPointerUnaryOperator
			( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval ) ;
		// 構造体の単項演算子
		LExprValuePtr EvalStructUnaryOperator
			( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval ) ;
		// オブジェクトの単項演算子
		LExprValuePtr EvalObjectUnaryOperator
			( Symbol::OperatorIndex opIndex, bool unaryPost, LExprValuePtr xval ) ;

	public:
		// 二項演算（※例外的にそれ以外のものも含む）
		LExprValuePtr ParseBinaryOperator
			( LStringParser& sparsExpr,
				LExprValuePtr xval,
				Symbol::OperatorIndex opIndex,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr ) ;
		// 二項演算
		LExprValuePtr EvalBinaryOperator
			( LExprValuePtr xval1,
				Symbol::OperatorIndex opIndex, LExprValuePtr xval2,
				const LNamespaceList * pnslLocal = nullptr ) ;

	protected:
		// :: 演算子
		LExprValuePtr ParseOperatorStaticMemberOf
			( LStringParser& sparsExpr, LExprValuePtr xval ) ;
		// . 演算子
		LExprValuePtr ParseOperatorMemberOf
			( LStringParser& sparsExpr, LExprValuePtr xval ) ;
		// .* 演算子
		LExprValuePtr ParseOperatorMemberCallOf
			( LStringParser& sparsExpr, LExprValuePtr xval,
					const LNamespaceList * pnslLocal = nullptr,
					const wchar_t * pwszEscChars = nullptr ) ;
		// () 演算子（関数呼び出し）
		LExprValuePtr ParseOperatorCallFunction
			( LStringParser& sparsExpr, LExprValuePtr xval,
					const LNamespaceList * pnslLocal = nullptr ) ;

	public:
		// 関数呼び出し
		LExprValuePtr ExprCallFunction
			( LExprValuePtr xvalThis,
				LPtr<LFunctionObj> pFunc,
				std::vector<LExprValuePtr>& xvalArgList ) ;
		// 定数式として関数呼び出し
		LExprValuePtr ConstExprCallFunction
			( LExprValuePtr xvalThis,
				LPtr<LFunctionObj> pFunc,
				std::vector<LExprValuePtr>& xvalArgList ) ;
		// 間接関数呼び出し
		LExprValuePtr ExprIndirectCallFunction
			( LExprValuePtr xvalThis,
				LExprValuePtr xvalFunc,
				std::vector<LExprValuePtr>& xvalArgList ) ;
		// 仮想関数呼び出し
		LExprValuePtr ExprCallVirtualFunction
			( LExprValuePtr xvalThis,
				LClass * pThisClass,
				const std::vector<size_t> * pVirtFuncs,
				std::vector<LExprValuePtr>& xvalArgList ) ;
		LExprValuePtr ExprCallVirtualFunction
			( LExprValuePtr xvalThis,
				LClass * pThisClass, size_t iVirtual,
				std::vector<LExprValuePtr>& xvalArgList ) ;
		// 関数引数をプッシュ
		void ExprCodePushArgument
			( const LPrototype& proto,
				LExprValuePtr xvalThis,
				const std::vector<LExprValuePtr>& xvalArgList ) ;
		void ConstExprPushArgument
			( std::vector<LValue>& argValues,
				const LPrototype& proto,
				LExprValuePtr xvalThis,
				const std::vector<LExprValuePtr>& xvalArgList ) ;
		// 関数引数のスタック解放と返り値取得
		LExprValuePtr ExprCodeReturnValue( const LPrototype& proto ) ;
		LExprValuePtr ConstExprReturnValue
			( const LPrototype& proto,
				const LValue& valRet, LObjPtr pExcept ) ;
		// ローカル変数に fetch_addr ポインタが存在するかチェックし警告を出力する
		void AssertNoFetchedAddrOnLocal( void ) ;

	public:
		// [] 演算子（要素参照）
		LExprValuePtr ParseOperatorElement
			( LStringParser& sparsExpr, LExprValuePtr xval,
					const LNamespaceList * pnslLocal = nullptr ) ;
		LExprValuePtr EvalOperatorElement
			( LExprValuePtr xval, LExprValuePtr xvalIndex ) ;
		// instanceof 演算子
		LExprValuePtr ParseOperatorInstanceOf
			( LStringParser& sparsExpr, LExprValuePtr xval,
					const LNamespaceList * pnslLocal = nullptr,
					const wchar_t * pwszEscChars = nullptr ) ;
		// && 演算子, || 演算子
		LExprValuePtr ParseOperatorLogical
			( LStringParser& sparsExpr, LExprValuePtr xval,
				Symbol::OperatorIndex opIndex,
					const LNamespaceList * pnslLocal = nullptr,
					const wchar_t * pwszEscChars = nullptr ) ;
		// ? : 演算子
		LExprValuePtr ParseOperatorConditionalChoise
			( LStringParser& sparsExpr, LExprValuePtr xval,
					const LNamespaceList * pnslLocal = nullptr,
					const wchar_t * pwszEscChars = nullptr ) ;
		// , 演算子
		LExprValuePtr ParseOperatorSequencing
			( LStringParser& sparsExpr, LExprValuePtr xval,
					const LNamespaceList * pnslLocal = nullptr,
					const wchar_t * pwszEscChars = nullptr ) ;
		// 数式を読み飛ばす
		void ParseToSkipExpression
			( LStringParser& sparsExpr,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr,
				int nEscPriority = Symbol::priorityLowest ) ;

	public:
		// プリミティブに対する二項演算
		LExprValuePtr EvalPrimitiveBinaryOperator
			( LExprValuePtr xval1,
				Symbol::OperatorIndex opIndex, LExprValuePtr xval2 ) ;
		// ポインタに対する二項演算
		LExprValuePtr EvalPointerBinaryOperator
			( LExprValuePtr xval1,
				Symbol::OperatorIndex opIndex, LExprValuePtr xval2 ) ;
		// 構造体に対する二項演算子
		LExprValuePtr EvalStructBinaryOperator
			( LExprValuePtr xval1,
				Symbol::OperatorIndex opIndex, LExprValuePtr xval2 ) ;
		// オブジェクトに対する二項演算子
		LExprValuePtr EvalObjectBinaryOperator
			( LExprValuePtr xval1,
				Symbol::OperatorIndex opIndex, LExprValuePtr xval2 ) ;
		// 定数式中で例外が発生したか確認しエラーを出力する
		bool CheckExceptionInConstExpr( void ) ;
		// 例外オブジェクトをエラー出力
		void OutputExceptionAsError( LObjPtr pExcept ) ;

	public:
		// boolean として評価する
		LExprValuePtr EvalAsBoolean( LExprValuePtr xval ) ;
		// プリミティブ型をオブジェクトに変換する
		LExprValuePtr EvalAsObject( LExprValuePtr xval ) ;
		// キャストする
		LExprValuePtr EvalCastTypeTo
			( LExprValuePtr xval, const LType& typeCast, bool explicitCast = false ) ;
		// 定数式の関数参照をキャストする
		LExprValuePtr ConstExprCastFuncTo
			( LExprValuePtr xval, const LType& typeCast, bool explicitCast = false ) ;
		// 配列オブジェクトを Type[] へ変換する
		LPtr<LArrayObj> ConstExprCastToArrayElementType
			( LPtr<LArrayObj> pArrayObj, LClass * pElementType ) ;
		// 辞書配列オブジェクトを Map<Type> へ変換する
		LPtr<LMapObj> ConstExprCastToMapElementType
			( LPtr<LMapObj> pMapObj, LClass * pElementType ) ;
		// ポインタのアライメントチェックする
		LExprValuePtr EvalCheckPointerAlignmenet( LExprValuePtr xvalPtr ) ;
		// ポインタ（参照型の場合にはポインタへ変換して）の実アドレスをフェッチする
		LExprValuePtr ExprFetchPointerAddr
			( LExprValuePtr xvalPtr,
				size_t iOffset, size_t nRange, LLocalVarPtr pAssumeVar = nullptr ) ;
		// 参照形式の場合ロードして、ポインタのオフセット形式の場合統合して
		// 必ず単一要素の状態にする
		LExprValuePtr EvalMakeInstance( LExprValuePtr xval ) ;
		LExprValuePtr ExprMakeInstance( LExprValuePtr xval ) ;
		// 定数式の特殊表現の場合、一般表現に変換する
		LExprValuePtr EvalMakeInstanceIfConstExpr( LExprValuePtr xval ) ;
		// ポインタのオフセット形式の場合、
		// ポインタを複製してオフセットをポインタに統合する
		LExprValuePtr EvalMakePointerInstance( LExprValuePtr xval ) ;
		// 最終的なポインタとオフセットを決定する
		ssize_t EvalPointerOffset
			( LExprValuePtr& xvalPtr,
				LExprValuePtr& xvalOffset /*can be null*/, ssize_t iOffset = 0 ) ;
		LExprValuePtr EvalPointerOffsetInstance
			( LExprValuePtr xvalOffset /*can be null*/, ssize_t iOffset = 0 ) ;
		// 参照ポインタの場合、ポインタ形式に変更する
		LExprValuePtr MakeRefPointerToPointer( LExprValuePtr xval ) ;
		// 参照形式の実行時式を実際にロードして評価する
		LExprValuePtr EvalLoadToDiscloseRef( LExprValuePtr xval ) ;
		// static なバッファへの参照ポインタ
		LExprValuePtr EvalLoadRefPtrStaticBuf
			( const LType& typeData,
				std::shared_ptr<LArrayBuffer> pBuf, size_t iOffset, size_t nBytes ) ;
		// 実行時式がユニークでない場合、その複製を返す
		// 返り値は必ずユニークなので型情報などを変更してもよい
		LExprValuePtr EvalMakeUniqueValue( LExprValuePtr& xval ) ;

	public:
		// 実行時式で即値をロードする
		LExprValuePtr ExprLoadImmPrimitive
			( LType::Primitive type, const LValue::Primitive& val ) ;
		LExprValuePtr ExprLoadImmInteger( LLong val ) ;
		// 実行時式で即値オブジェクトをロードする
		LExprValuePtr ExprLoadImmObject
				( const LType& type, LObjPtr pObj,
					bool fObjChain = true, bool fUniqueObj = false ) ;
		LExprValuePtr ExprLoadImmObject( LObjPtr pObj ) ;
		LExprValuePtr ExprLoadImmString( const LString& str ) ;
		// 実行時式で新規オブジェクトを作成しロードする
		LExprValuePtr ExprLoadNewObject( LClass * pClass ) ;
		// 実行時式でポインタにバッファを確保する
		void ExprCodeAllocBuffer
			( LExprValuePtr& xvalPtr, LExprValuePtr xvalBytes ) ;
		// スタック上にない場合、スタック上に確保する
		LExprValuePtr ExprMakeOnStack( LExprValuePtr xval ) ;
		// ポインタとして複製する
		LExprValuePtr EvalCloneAsPointer( LExprValuePtr xval ) ;
		// 実行時式で値の複製（参照型の場合には参照先）をロードする
		//（オブジェクトやポインタの場合には参照カウンタを AddRef して
		//  スタック上に yp バックリンクを作成する）
		//（オフセット付きポインタの場合にはオフセットのみ複製した
		//  新たなオフセット付きポインタを作成する）
		LExprValuePtr ExprLoadClone( LExprValuePtr xval ) ;
		// 実行時式で値の複製（参照型の場合には参照先）をロードする
		//（yp リンクなどは行わず、AddRef して単一要素だけをプッシュする）
		LExprValuePtr ExprPushClone( LExprValuePtr xval, bool withAddRef ) ;
		// 確実に ExprPushClone を実行できるような事前処理
		LExprValuePtr ExprPrepareToPushClone( LExprValuePtr xval, bool withAddRef ) ;
		// 単一要素がオブジェクトの場合には yp チェーンを構築する
		void ExprTreatObjectChain( LExprValuePtr xval ) ;
		// （スタック上以外で）ユニークかどうか判定する
		bool IsUniqueExprValue( const LExprValuePtr& xval ) const ;

	public:
		// 実行時式でスタック上の値をコピーする
		void ExprCodeMove( LExprValuePtr xvalDst, LExprValuePtr xvalSrc ) ;
		// 実行時式で値をストアする
		void ExprCodeStore
			( LExprValuePtr xvalRefDst,
				LExprValuePtr xvalSrc, bool disregardConst = false ) ;
		// 実行時式で値をストアする（:= 演算子での構造体のコピー）
		LExprValuePtr ExprCodeStoreMoveStructure
			( LExprValuePtr xvalRefDst, LExprValuePtr xvalSrc ) ;
		// オブジェクト要素をロードする
		LExprValuePtr EvalLoadObjectElementAt
			( const LType& type,
				LExprValuePtr xvalObj, LExprValuePtr xvalIndex ) ;
		LExprValuePtr EvalLoadObjectElementAs
			( const LType& type,
				LExprValuePtr xvalObj, LExprValuePtr xvalStr ) ;
		// 実行時式でオブジェクト要素をストアする
		void ExprStoreObjectElementAt
			( LExprValuePtr xvalObj,
				LExprValuePtr xvalIndex, LExprValuePtr xvalElement ) ;
		void ExprStoreObjectElementAs
			( LExprValuePtr xvalObj,
				LExprValuePtr xvalStr, LExprValuePtr xvalElement ) ;
		// 実行時式でポインタの指すメモリからロードする
		LExprValuePtr ExprLoadPtrPrimitive
			( LType::Primitive type, LExprValuePtr xvalPtr,
				LExprValuePtr xvalOffset = nullptr, size_t iOffset = 0 ) ;
		// 実行時式でポインタの指すメモリへストアする
		void ExprStorePtrPrimitive
			( LExprValuePtr xvalPtr,
					LExprValuePtr xvalOffset /*can be null*/,
					LType::Primitive type, LExprValuePtr xvalSrc ) ;
		void ExprStorePtrStructure
			( LExprValuePtr xvalPtr,
					LExprValuePtr xvalOffset /*can be null*/,
					LStructureClass * pStructClass, LExprValuePtr xvalSrc ) ;
		// 演算子を実行する
		LExprValuePtr EvalCodeBinaryOperate
			( const LType& typeRet,
				LExprValuePtr xval1, LExprValuePtr xval2,
				Symbol::OperatorIndex opIndex,
				Symbol::PFN_OPERATOR2 pfnOp, void* pInstance,
				LFunctionObj * pOpFunc ) ;
		LExprValuePtr EvalCodeUnaryOperate
			( const LType& typeRet, LExprValuePtr xval,
				Symbol::OperatorIndex opIndex,
				Symbol::PFN_OPERATOR1 pfnOp, void* pInstance,
				LFunctionObj * pOpFunc ) ;

	public:
		// コンパイル時の評価値情報をスタック上にもプッシュする
		void PushExprValueOnStack( LExprValuePtr xval ) ;
		// コンパイル時の評価値情報をスタック上から削除する
		void PopExprValueOnStacks( size_t nCount ) ;
		// 未使用の一時スタック数をカウントする
		size_t CountFreeTempStack( void ) ;

		// 実行時式でスタックを解放する
		void ExprCodeFreeStack( size_t nCount ) ;
		// 未使用の一時スタックを開放する（解放した数を返す）
		size_t ExprCodeFreeTempStack( void ) ;

	public:
		// 現在のコードポインタを取得する（ジャンプ先などのため）
		// （※暗黙に ExprCodeFreeTempStack が呼び出される）
		CodePointPtr GetCurrentCodePointer( bool flagDestPos ) ;

		// ジャンプ命令（命令のコードポインタを返す）
		// （※暗黙に ExprCodeFreeTempStack が呼び出される）
		CodePointPtr ExprCodeJump( CodePointPtr cpDest = nullptr ) ;
		CodePointPtr ExprCodeJumpIf
			( LExprValuePtr xvalCond, CodePointPtr cpDest = nullptr ) ;
		CodePointPtr ExprCodeJumpNonIf
			( LExprValuePtr xvalCond, CodePointPtr cpDest = nullptr ) ;
		CodePointPtr ExprCodeJumpConditional
			( LCodeBuffer::InstructionCode code,
				LExprValuePtr xvalCond, CodePointPtr cpDest = nullptr ) ;

		// 指定コードポインタにあるジャンプ命令のジャンプ先を確定する
		void FixJumpDestination( CodePointPtr cpJumpCode, CodePointPtr cpDest ) ;
		// ジャンプ先へのスタック解放数を計算する
		// （ジャンプできない場合はエラーを発生する）
		size_t CountFreeStackToJump
			( CodePointPtr cpJumpCode, CodePointPtr cpDest ) ;
		// スタック総数をカウントする
		size_t CountStackOnfCodePoint( CodePointPtr cp ) const ;

		// 指定のコードポインタ以降のコード出力をすべて破棄する
		void DiscardCodeAfterPoint( CodePointPtr cpSave ) ;
		void DiscardCodeAfterPoint( size_t iCodePos ) ;
		// 実行時式を要求（定数式で無ければならない場合エラーを出力し false）
		bool MustBeRuntimeExpr( void ) ;
		// ジャンプ先のラベル名を取得する
		LString GetLabelNameOfCodePoint( CodePointPtr cpDest ) const ;

	protected:
		// 命令出力
		void AddCode
			( LCodeBuffer::InstructionCode code,
				size_t sop1 = 0, size_t sop2 = 0, size_t op3 = 0,
				ssize_t imm32 = 0,
				const LCodeBuffer::ImmediateOperand * imm64 = nullptr ) ;
		void InsertCode
			( size_t iCodePoint,
				LCodeBuffer::InstructionCode code,
				size_t sop1 = 0, size_t sop2 = 0, size_t op3 = 0,
				ssize_t imm32 = 0,
				const LCodeBuffer::ImmediateOperand * imm64 = nullptr ) ;

		// 指定コードポインタにあるコードを取得する
		LCodeBuffer::Word * GetCodeAt( CodePointPtr cpCode ) ;

		// ジャンプ命令のジャンプ先の更新
		void UpdateCodeJumpPoints( void ) ;

		// 実行時式でスタック上に確保されているか？
		bool IsExprValueOnStack( LExprValuePtr xval ) const ;
		// 実行時式でスタック上の指標（後ろから見た）を取得
		ssize_t GetBackIndexOnStack( LExprValuePtr xval ) const ;

		// ローカルフレーム上での指標を取得
		ssize_t GetLocalVarIndex( LExprValuePtr xval ) const ;
		// ローカルフレーム上のエントリ取得
		LLocalVarPtr GetLocalVarAt( size_t iLocal ) const ;
		// fetch_addr なローカル変数なら参照領域を更新
		void HasFetchPointerRange( LExprValuePtr xval, size_t nRange ) ;

	public:
		//////////////////////////////////////////////////////////////////////
		// 文解釈
		//////////////////////////////////////////////////////////////////////

		// 文解釈
		void ParseStatementList
			( LStringParser& sparsSrc,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr ) ;
		void ParseOneStatement
			( LStringParser& sparsSrc,
				const LNamespaceList * pnslLocal = nullptr ) ;

		// 文開始処理
		void OnBeginStatement( LStringParser& sparsSrc ) ;
		// 文終了時処理
		void OnEndStatement
			( LStringParser& sparsSrc, Symbol::ReservedWordIndex rwIndex ) ;

		// デバッグ用ソースコード行開始位置設定
		void ResetDebugSourceLineInfo( LStringParser& sparsSrc ) ;
		// デバッグ用ソースコード情報追加
		void AddDebugSourceInfo( LStringParser& sparsSrc ) ;

		// 関数ブロック開始
		ContextPtr BeginFunctionBlock
			( std::shared_ptr<LPrototype> proto,
				std::shared_ptr<LCodeBuffer> codeBuf,
				const wchar_t * pwszFuncName = nullptr,
				CodeNest::ControlType typeBaseNest = CodeNest::ctrlFunction ) ;
		// 関数ブロック終了
		void EndFunctionBlock( ContextPtr ctx ) ;

		// 名前空間ブロック開始
		ContextPtr BeginNamespaceBlock
			( LPtr<LNamespace> pNamespace,
				CodeNest::ControlType typeBaseNest = CodeNest::ctrlNamespace ) ;
		// 名前空間ブロック終了
		void EndNamespaceBlock( ContextPtr ctx ) ;

	public:
		// コード生成を要求可能な入れ子状態を要求（そうでない場合エラーを出力し false）
		bool MustBeRuntimeCode( void ) ;
		// 名前空間内でなければならない（そうでない場合エラーを出力し false）
		bool MustBeInNamespace( void ) ;
		// 名前空間を取得
		LPtr<LNamespace> GetCurrentNamespace( void ) const ;
		// 文末のセミコロンを読み飛ばす
		// セミコロンがない場合、エラーを出力しセミコロンまで読み飛ばす
		void HasSemicolonForEndOfStatement( LStringParser& sparsSrc ) ;
		// 入れ子情報追加
		CodeNestPtr PushNest( CodeNest::ControlType type ) ;
		// 入れ子情報削除
		CodeNestPtr PopNest( CodeNestPtr pNest ) ;
		// ラベルを設置する
		void PutLabel( const LString& strLabel ) ;

		// 入れ子のリリーサー
		class	NestPopper
		{
		private:
			LCompiler&	m_compiler ;
			CodeNestPtr	m_pNest ;
		public:
			NestPopper( LCompiler& compiler, CodeNestPtr pNest )
				: m_compiler( compiler ), m_pNest( pNest ) {}
			~NestPopper( void ) { Pop() ; }
			void Pop( void )
			{
				if ( m_pNest != nullptr )
					m_compiler.PopNest( m_pNest ) ;
				m_pNest = nullptr ;
			}
		} ;

		// 現在の入れ子でのローカル変数の使用枠を取得する
		size_t GetLocalNestVarUsedSize( void ) ;
		// ローカル変数で定義されたサイズだけスタックを確保する命令を出力する
		void ExprCodeAllocLocalFrame( size_t nLastUsed ) ;
		// 必要であればローカル変数に yp チェーンを構築するコードを出力する
		void ExprCodeTreatLocalObjectChain( LLocalVarPtr pVar ) ;

		// ローカル変数を定義する
		//（スタックを確保し、必要であれば yp チェーンを構築力する）
		LLocalVarPtr ExprCodeDeclareLocalVar
				( const wchar_t * pwszName, const LType& typeVar ) ;
		// ローカル変数を定義する（情報を登録するだけ）
		LLocalVarPtr DeclareLocalVar
				( const wchar_t * pwszName, const LType& typeVar ) ;

		// 現在の一時スタックをローカルフレームに移し替えて
		// ローカル変数に変換する
		LLocalVarPtr MakeTempStackIntoLocalVar( LExprValuePtr xval ) ;
		LLocalVarPtr MakeTempStackIntoLocalVarAs
					( const wchar_t * pwszName, LExprValuePtr xval ) ;
		// ローカル変数定義妥当性検証
		bool VerifyLocalVarDeclarationAs
					( const wchar_t * pwszName, const LType& typeVar ) ;
		bool VerifyLocalVarDeclarationTypeAs
					( const wchar_t * pwszName, const LType& typeVar ) ;

	public:
		// 有効なユーザー定義名か？
		bool IsValidUserName( const wchar_t * pwszName ) const ;
		// アクセス修飾子フラグを解釈
		LType::Modifiers ParseAccessModifiers
			( LStringParser& sparsExpr,
				LType::Modifiers accModAdd = 0,
				LType::Modifiers accModExclusions = 0 ) ;
		LType::Modifiers GetModifierOfReservedWord
			( Symbol::ReservedWordIndex rwIndex ) ;
		// 関数プロトタイプ引数リストを解釈
		void ParsePrototypeArgmentList
			( LStringParser& sparsExpr, LPrototype& proto,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr ) ;
		// 非オブジェクト変数の初期値を解釈
		void ParseDataInitExpression
			( LStringParser& sparsExpr,
				const LType& typeData, LPtr<LPointerObj> pPtr,
				const LNamespaceList * pnslLocal = nullptr,
				const wchar_t * pwszEscChars = nullptr ) ;

	protected:
		// 予約語解釈
		typedef void (LCompiler::*PFN_ParseStatement)
						( LStringParser& sparsSrc,
							Symbol::ReservedWordIndex rwIndex,
							const LNamespaceList * pnslLocal ) ;
		static const PFN_ParseStatement
						s_pfnParseStatement[Symbol::rwiReservedWordCount] ;

		// @import
		void ParseStatement_import
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// @include
		void ParseStatement_incllude
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// @error
		void ParseStatement_error
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// @todo
		void ParseStatement_todo
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// enum
		void ParseStatement_enum
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// class, struct, namespace
		void ParseStatement_class
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
	public:
		void ParseImplementationClass
			( LStringParser& sparsSrc, LPtr<LNamespace> pNamespace,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
	protected:
		LClass * ParseNamespaceExpr
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		bool IsInstantiatingGenericType( LStringParser& sparsSrc ) ;
		void ParseInstantiatingGenericType
			( LStringParser& sparsSrc,
				LPtr<LNamespace> pNamespace, const wchar_t * pwszName,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		void ParseDeclarationGenericType
			( LStringParser& sparsSrc,
				LPtr<LNamespace> pNamespace, const wchar_t * pwszName,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// typedef
		void ParseStatement_typedef
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// using
		void ParseStatement_using
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// for
		void ParseStatement_for
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		void ParseStatement_for_autovar
			( LStringParser& sparsSrc,
				CodeNestPtr pNestFor, const wchar_t * pwszVarName,
				const LNamespaceList * pnslLocal ) ;
		void ParseStatement_for_autoiter
			( LStringParser& sparsSrc,
				CodeNestPtr pNestFor, const wchar_t * pwszVarName,
				const LNamespaceList * pnslLocal ) ;
		void ParseStatement_for_params
			( LStringParser& sparsSrc,
				CodeNestPtr pNestFor, const LNamespaceList * pnslLocal ) ;
		// forever
		void ParseStatement_forever
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// while
		void ParseStatement_while
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// do
		void ParseStatement_do
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// if
		void ParseStatement_if
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// switch
		void ParseStatement_switch
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// case
		void ParseStatement_case
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// default
		void ParseStatement_default
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// break
		void ParseStatement_break
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// continue
		void ParseStatement_continue
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// goto
		void ParseStatement_goto
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// try
		void ParseStatement_try
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// throw
		void ParseStatement_throw
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// return
		void ParseStatement_return
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// with
		void ParseStatement_with
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// synchronized
		void ParseStatement_synchronized
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;

		// 宣言文か式文 : const, void, boolean, byte, short, char, int, long,
		//					float, double, int8, uint8, int16, uint16,
		//					int32, uint32, int64
		void ParseStatement_def_expr
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;

		// 宣言文 : static, abstract, native, fetch_addr,
		//			public, protected, private, @override, @deprecated,
		void ParseStatement_definition
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;
		// 宣言文 : auto
		void ParseStatement_definition_auto
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;

		// 式文 : function, operator,
		//			this, super, global, null, false, true
		void ParseStatement_expr
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;

		// 構文エラー : extends, implements, in, else, catch, finally,
		void ParseStatement_syntax_error
			( LStringParser& sparsSrc,
				Symbol::ReservedWordIndex rwIndex, const LNamespaceList * pnslLocal ) ;

	protected:
		// 宣言文か式文
		void ParseDefinitionOrExpression
			( LStringParser& sparsSrc, const LNamespaceList * pnslLocal ) ;
		// 変数又は関数の宣言・定義文
		void ParseDeclarationAndDefinition
			( LStringParser& sparsSrc,
				LType::Modifiers accModLeft, const LNamespaceList * pnslLocal ) ;
		void ParseDeclarationAndDefinition
			( LStringParser& sparsSrc, LType::Modifiers accModLeft,
				const LType& type, const LNamespaceList * pnslLocal ) ;
		// operator 関数宣言・定義
		void ParseOperatorFuncDeclaration
			( LStringParser& sparsSrc, LType::Modifiers accModFunc,
				const LType& typeRet, const LNamespaceList * pnslLocal ) ;
		// auto 変数定義
		void ParseDefinitionAsAutoVar
			( LStringParser& sparsSrc,
				LType::Modifiers accModLeft, const LNamespaceList * pnslLocal ) ;
		// namespace, class, struct メンバ修飾子
		LType DefaultAccessModifierTypeOf( const LType& type ) const ;
		LType::Modifiers DefaultAccessModifiers( LType::Modifiers accMode ) const ;

		// メンバ変数定義
		void ParseMemberVariable
			( LStringParser& sparsSrc,
				LPtr<LNamespace> pNamespace,
				const LType& typeVar, const wchar_t * pwszName,
				const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars ) ;
		void DefineMemberVariable
			( LPtr<LNamespace> pNamespace,
				const LType& typeVar,
				const wchar_t * pwszName, LExprValuePtr xvalInit ) ;
		void DefineVariableOnArrangeBuf
			( LArrangementBuffer& arrangeBuf,
				const LType& typeVar,
				const wchar_t * pwszName, LExprValuePtr xvalInit ) ;
		void DefineStaticVariable
			( LPtr<LNamespace> pNamespace,
				const LType& typeVar,
				const wchar_t * pwszName, LExprValuePtr xvalInit ) ;
		void DefineObjectMemberVariable
			( LObjPtr pProto, const LType& typeVar,
				const wchar_t * pwszName, LExprValuePtr xvalInit ) ;
		LObjPtr GetObjectOfInitExpr
			( const LType& typeVar,
				const wchar_t * pwszName, LExprValuePtr xvalInit ) ;

		// メンバ変数初期値遅延実装
		void ParseDelayInitVar( const DelayInitVar& delayInit ) ;

		// ローカル変数定義
		void ParseLocalVariableDefinition
			( const LType& typeVar,
				const wchar_t * pwszName, LStringParser& sparsSrc,
				const LNamespaceList * pnslLocal, bool flagConstructerParam ) ;
		// ローカル変数の初期値式を評価
		void ParseLocalVariableInitExpr
			( LStringParser& sparsSrc, LExprValuePtr xvalVar,
				const LNamespaceList * pnslLocal, const wchar_t * pwszEscChars ) ;

		// コンストラクタ定義判定・解釈
		// コンストラクタなら関数を解釈し次へ進めたうえで true を返す
		bool ParseConstructer
			( LStringParser& sparsSrc,
				LType::Modifiers accModLeft, const LNamespaceList * pnslLocal ) ;

		// メンバ関数解釈
		void ParseMemberFunction
			( LStringParser& sparsSrc,
				LType::Modifiers accModFunc, const LType& typeRet,
				const wchar_t * pwszFuncName, const LNamespaceList * pnslLocal ) ;

		// 関数後置修飾解釈
		void ParsePostFunctionModifiers
			( LStringParser& sparsSrc, std::shared_ptr<LPrototype> pProto ) ;
		// 関数実装解釈
		void ParseFunctionImplementation
			( LStringParser& sparsSrc,
				LPtr<LFunctionObj> pFunc, const LNamespaceList * pnslLocal ) ;

		// 関数遅延実装
		void ParseDelayFuncImplement( const DelayImplement& delayImpl ) ;
		// 構築関数初期化リスト解釈
		void ParseInitListOfConstructer
			( LStringParser& sparsSrc,
				LPtr<LFunctionObj> pFunc, const LNamespaceList * pnslLocal ) ;
		// 親クラスのデフォルトの構築関数呼び出し
		void ExprCodeCallDefaultConstructer
			( LPtr<LFunctionObj> pFunc,
				const std::set<LClass*>& setCalledInit ) ;

	public:
		//////////////////////////////////////////////////////////////////////
		// エラー
		//////////////////////////////////////////////////////////////////////

		// 現在のステートメントでエラーが発生したか？
		bool HasErrorOnCurrent( void ) const ;
		// 現在のステートメントのエラーフラグをクリア
		void ClearErrorOnCurrent( void ) ;

		// エラー出力
		virtual void OnError
			( ErrorMessageIndex err,
				const wchar_t * opt1 = nullptr,
				const wchar_t * opt2 = nullptr ) ;
		static void Error
			( ErrorMessageIndex err,
				const wchar_t * opt1 = nullptr,
				const wchar_t * opt2 = nullptr ) ;
		// 警告出力
		virtual void OnWarning
			( ErrorMessageIndex err, int nLevel = 0,
				const wchar_t * opt1 = nullptr,
				const wchar_t * opt2 = nullptr ) ;
		static void Warning
			( ErrorMessageIndex err, int nLevel = 0,
				const wchar_t * opt1 = nullptr,
				const wchar_t * opt2 = nullptr ) ;
		// エラー文字列フォーマット
		LString FormatErrorLineString
			( const wchar_t * pwszType,
				ErrorMessageIndex err,
				const wchar_t * opt1 = nullptr,
				const wchar_t * opt2 = nullptr ) ;
		static LString FormatErrorString
			( const wchar_t * pwszType,
				ErrorMessageIndex err,
				const wchar_t * opt1 = nullptr,
				const wchar_t * opt2 = nullptr ) ;
		// 現在のソースと行番号・桁
		virtual LString GetCurrentFileLine( void ) const ;
		// 文字列出力
		virtual void PrintString( const LString& str ) ;

	} ;

}


#endif

