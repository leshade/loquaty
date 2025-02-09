
#ifndef	__LOQUATY_DEBUGGER_H__
#define	__LOQUATY_DEBUGGER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// デバッガ
	//////////////////////////////////////////////////////////////////////////

	class	LDebugger	: public Object
	{
	public:
		// ブレークポイント
		class	BreakPoint	: public Object
		{
		public:
			const LCodeBuffer *	m_pCodeBuf ;	// 対象コードバッファ
			ssize_t				m_ipFirst ;		// コード範囲 [m_ipFirst, m_ipEnd]
			ssize_t				m_ipEnd ;
			LStringParser		m_exprCond ;	// 条件式

		public:
			BreakPoint( const LCodeBuffer * pCodeBuf = nullptr,
							ssize_t ip = LCodeBuffer::InvalidCodePos )
				: m_pCodeBuf( pCodeBuf ), m_ipFirst( ip ), m_ipEnd( ip ) { }
			BreakPoint( const LCodeBuffer * pCodeBuf,
						ssize_t ipFirst, ssize_t ipEnd,
						const wchar_t * pwszCondExpr = nullptr )
				: m_pCodeBuf( pCodeBuf ),
					m_ipFirst( ipFirst ), m_ipEnd( ipEnd ),
					m_exprCond( pwszCondExpr ) { }
			BreakPoint( const BreakPoint& bp )
				: m_pCodeBuf( bp.m_pCodeBuf ),
					m_ipFirst( bp.m_ipFirst ), m_ipEnd( bp.m_ipEnd ),
					m_exprCond( bp.m_exprCond ) { }

			// ブレークポイント範囲内か？
			bool IsInBounds( const LCodeBuffer * code, ssize_t ip ) const
			{
				return	(m_pCodeBuf == code)
						&& (m_ipFirst <= ip) && (ip <= m_ipEnd) ;
			}
			// ブレークポイントに到達した
			// true を返すとブレークポイントの条件を満たす
			virtual bool OnBreakPoint( LContext& context )
			{
				if ( !m_exprCond.IsEmpty() )
				{
					m_exprCond.SeekIndex( 0 ) ;
					return	EvaluateExpr( context, m_exprCond ).AsBoolean() ;
				}
				return	true ;
			}
		} ;

		typedef	std::shared_ptr<BreakPoint>	BreakPointPtr ;

		// ブレーク原因
		enum	BreakReason
		{
			reasonBreakPoint,	// ブレークポイント
			reasonStep,			// ステップ実行
		} ;

		// デバッグ実行モード
		enum	DebugMode
		{
			modeNormal,			// 通常実行
			modeStepIn,			// ステップ実行
			modeStepOver,		// ステップオーバー
			modeStepOutStop,	// ステップ停止
		} ;

		// コンテキスト・インスタンス
		class	Instance: public Object
		{
		public:
			DebugMode	m_mode ;
			BreakPoint	m_bpCurStep ;
			BreakPoint	m_bpStepOver ;

		public:
			Instance( void ) : m_mode( modeNormal ) { }
		} ;

		typedef	std::shared_ptr<Instance>	InstancePtr ;

	protected:
		// ブレークポイント
		std::vector<BreakPointPtr>	m_vBreakPoints ;

		// デバッグ排他的実行用ミューテックス
		std::recursive_mutex		m_mutexDebugBreak ;

	public:
		// トレース実行クリア設定
		void ClearStepTrace( LContext& context ) ;
		// ステップ実行設定
		void SetStepIn( LContext& context ) ;
		void SetStepIn
			( LContext& context,
				size_t ipCurStepFirst, size_t ipCurStepEnd ) ;
		// ステップオーバー設定
		void SetStepOver
			( LContext& context,
				size_t ipCurStepFirst, size_t ipCurStepEnd,
				size_t ipNextStepFirst, size_t ipNextStepEnd ) ;
		// コンテキスト・インスタンス生成
		virtual InstancePtr CreateInstance( void ) ;

	public:
		// コード実行前
		virtual void OnCodeInstruction
			( LContext& context,
				const LCodeBuffer * code, size_t ip,
				const LCodeBuffer::Word& word ) ;
		// デバッグ用出力
		virtual void OutputTrace( const wchar_t * pwszMessage ) ;
		// 例外発生時
		virtual LObjPtr OnThrowException
			( LContext& context, LObjPtr pException ) ;
		// ブレークポイント
		virtual void OnBreakPoint( LContext& context, BreakReason reason ) ;

		// デバッグ排他的実行用 locker を取得
		std::unique_lock<std::recursive_mutex> LockDebugBreak( void )
		{
			return	std::unique_lock<std::recursive_mutex>( m_mutexDebugBreak ) ;
		}

	public:
		// ブレークポイント追加
		void AddBreakPoint( BreakPointPtr bp ) ;
		// ブレークポイント削除
		void RemoveBreakPoint( BreakPointPtr bp ) ;
		void RemoveAllBreakPoints( void ) ;
		// ブレークポイントリスト取得
		const std::vector<BreakPointPtr>& GetBreakPoints(void) const
		{
			return	m_vBreakPoints ;
		}

	public:
		// 評価値文字列化
		static LString ToExpression( const LValue& value ) ;
		// デバッグ時式を評価
		static LValue EvaluateExpr
			( LContext& context,
				LStringParser& sparsExpr,
				const wchar_t * pwszEscChars = nullptr,
				int priority = Symbol::priorityLowest ) ;

		// デバッグ時ローカル変数
		static LValue GetLocalVariableAs
			( LContext& context, const wchar_t * pwszName ) ;
		// デバッグ時メンバ変数
		static LValue GetMemberVariableAs
			( LContext& context,
					const LValue& valObj, const wchar_t * pwszName ) ;
		// デバッグ時データ変数
		static LValue GetDataMember
			( LContext& context,
				LPointerObj * pPtrObj, const LArrangement::Desc& desc ) ;
		// デバッグ時データ型変数評価
		static LValue EvaluateData( const LValue& val ) ;

		// 演算子判定
		static Symbol::OperatorIndex AsOperatorIndex( const LString& strToken ) ;
		// デバッグ時単項演算子
		static LValue EvaluateUnaryOperator
				( const LValue& value, Symbol::OperatorIndex opIndex ) ;
		// デバッグ時二項演算子
		static LValue EvaluateBinaryOperator
				( const LValue& valLeft,
					const LValue& valRight, Symbol::OperatorIndex opIndex ) ;
		// デバッグ時要素間接参照
		static LValue EvaluateElementAt
				( LContext& context,
					const LValue& valLeft, const LValue& valIndex ) ;

		// ローカル変数を取得
		static LValue GetLocalVar( LContext& context, LLocalVarPtr pVar, size_t iLoc ) ;
		

	} ;

}

#endif

