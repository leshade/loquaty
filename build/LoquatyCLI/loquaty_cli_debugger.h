
#if	!defined(__LOQUATY_CLI_DEBUGGER_H__)
#define	__LOQUATY_CLI_DEBUGGER_H__	1

namespace	Loquaty
{

	//////////////////////////////////////////////////////////////////////////
	// Loquaty CLI Debugger
	//////////////////////////////////////////////////////////////////////////

	class	LCLIDebugger	: public LDebugger
	{
	public:
		// デバッグ用出力
		virtual void OutputTrace( const wchar_t * pwszMessage ) ;
		// 例外発生時
		virtual LObjPtr OnThrowException
			( LContext& context, LObjPtr pException ) ;
		// ブレークポイント
		virtual void OnBreakPoint( LContext& context, BreakReason reason ) ;

	public:
		// 現在の停止位置のコードを表示する
		void TraceCurrentCode( LContext& context ) ;
		// コマンドライン対話
		void DoDialogue( LContext& context ) ;
		// コマンド処理
		bool DoCommandLine( LContext& context, LStringParser& cmdline ) ;
		// ステップイン設定
		void DoCommandStepIn( LContext& context ) ;
		// ステップオーバー設定
		void DoCommandStepOver( LContext& context ) ;
		// 現在の関数のコードリスト表示
		void DoCommandCodeList( LContext& context ) ;
		// 現在の関数の位置でのローカル変数リスト表示
		void DoCommandVariableList( LContext& context ) ;
		// 式の値を表示
		void DoCommandExpression( LContext& context, LStringParser& cmdline ) ;
		// ブレークポイント操作
		void DoCommandBreakPoint( LContext& context, LStringParser& cmdline ) ;
		// コマンド一覧表示
		void DoCommandHelp( void ) ;

	} ;

}

#endif

