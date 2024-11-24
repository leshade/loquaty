
#ifndef	__LOQUATY_OBJ_THREAD_H__
#define	__LOQUATY_OBJ_THREAD_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Task オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LTaskObj	: public LGenericObj
	{
	protected:
		LContext						m_context ;
		LPtr<LFunctionObj>				m_pFunc ;

		LContext::AsyncState			m_state ;
		bool							m_flagAsync ;
		volatile bool					m_flagFinished ;

		std::mutex						m_mutex ;
		std::condition_variable			m_cvFinished ;

		static thread_local LTaskObj *	t_pCurrent ;

	public:
		LTaskObj( LClass * pClass ) ;
		LTaskObj( const LTaskObj& obj ) ;

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// 関数実行（同期）
		std::tuple<LValue,LObjPtr>
			SyncCallFunction
				( LFunctionObj * pFunc,
					const LValue * pArgValues, size_t nArgCount ) ;
		std::tuple<LValue,LObjPtr>
			SyncCallFunctionAs
				( LObjPtr pThisObj,
					const wchar_t * pwszFuncName,
					const LValue * pArgValues, size_t nArgCount ) ;

		// LContext 取得
		LContext& Context( void )
		{
			return	m_context ;
		}
		const LContext& GetContext( void ) const
		{
			return	m_context ;
		}

	public:	// 軽量スレッド
		// 軽量スレッドとして実行を開始する
		bool BeginAsync( LPtr<LFunctionObj> pFunc ) ;
		// 軽量スレッドとして実行を継続する
		bool AsyncProceed( std::int64_t msecTimeout = 0 ) ;
		// 完了時の処理
		virtual void OnFinished( void ) ;

	public:	// 共通
		// スレッドは完了したか？
		bool IsFinished( void ) const ;
		// スレッド完了を待つ（msecTimeout < 0 の時はタイムアウトしないで待つ）
		bool WaitForThread( std::int64_t msecTimeout = -1 ) ;
		// 返り値取得
		const LValue& GetReturnedValue( void ) const ;
		// ハンドルされなかった例外取得
		LObjPtr GetUnhandledException( void ) const ;
		// 例外を送出する
		void ThrowException( LObjPtr pObj ) ;
		// 即座に強制終了させる
		//（通常スレッドで別スレッドから呼び出した場合には
		//  完全なスレッドの終了は WaitForThread() を呼び出す必要がある）
		void ForceTermination( void ) ;
		// 一時的に実行を停止する
		std::unique_lock<std::recursive_mutex> LockRunning( void ) ;
		// 現在のスレッドを取得する
		static LTaskObj * GetCurrent( void ) ;

	public:
		// boolean begin( Function<Object()> func )
		static void method_begin( LContext& context ) ;
		// boolean proceed( long timeout = 0 )
		static void method_proceed( LContext& context ) ;
		// boolean isFinished() const
		static void method_isFinished( LContext& context ) ;
		// boolean waitFor()
		static void method_waitFor0( LContext& context ) ;
		// boolean waitFor( long msecTimeout )
		static void method_waitFor1( LContext& context ) ;
		// Object getReturned() const
		static void method_getReturned( LContext& context ) ;
		// Object getUnhandledException() const
		static void method_getUnhandledException( LContext& context ) ;
		// void finish()
		static void method_finish( LContext& context ) ;
		// void throw( Object obj )
		static void method_throw( LContext& context ) ;

		// static Task getCurrent()
		static void method_getCurrent( LContext& context ) ;
		// static void rest()
		static void method_rest( LContext& context ) ;

	} ;



	//////////////////////////////////////////////////////////////////////////
	// Thread オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LThreadObj	: public LTaskObj
	{
	protected:
		std::unique_ptr<std::thread>	m_thread ;

		LThreadObj *					m_pListPrev ;
		LThreadObj *					m_pListNext ;

		friend class LVirtualMachine ;

	public:
		LThreadObj( LClass * pClass ) ;
		LThreadObj( const LThreadObj& obj ) ;
		virtual ~LThreadObj( void ) ;

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:	
		// スレッド開始
		bool BeginThread( LPtr<LFunctionObj> pFunc ) ;
		// 完了時の処理
		virtual void OnFinished( void ) ;

	public:
		// boolean begin( Function<Object()> func )
		static void method_begin( LContext& context ) ;
		// Object getReturned() const
		static void method_getReturned( LContext& context ) ;

		// static Thread getCurrent()
		static void method_getCurrent( LContext& context ) ;
		// static void sleep( long msecTimeout )
		static void method_sleep( LContext& context ) ;

	} ;

}

#endif

