
#ifndef	__LOQUATY_CONTEXT_H__
#define	__LOQUATY_CONTEXT_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 実行コンテキスト
	//////////////////////////////////////////////////////////////////////////

	class	LContext	: public Object
	{
	public:
		// 実行状態
		enum	ExecutionStatus
		{
			statusAborted	= -1,
			statusHalt,
			statusRunning,
			statusAwaiting,
		} ;
		static bool IsRunningStatus( ExecutionStatus status )
		{
			return	(status >= statusRunning) ;
		}

		// 待機脱出条件判定関数
		// 引数はミリ秒単位のタイムアウト時間で 0 は即座に復帰する
		// 待機状態を脱出する場合には true を
		// 待機状態を継続する場合には false を返す
		typedef	std::function<bool(std::int64_t)>	AwaitingFunc ;

		// 割り込み信号
		enum	InterruptSignal
		{
			interruptNo,
			interruptBreak,			// AsyncRun からの離脱
			interruptThrow,			// 例外送出
			interruptAbort,			// 強制終了
		} ;

	protected:
		// 状態保存
		struct	ContextState
		{
			ExecutionStatus		status ;
			size_t				sp, ap, fp ;
			ssize_t				xp, yp, ip ;
			LValue				valReturn ;
			LObjPtr				thrown ;
			LObjPtr				exception ;
			LCodeBuffer *		pCodeBuf ;
			AwaitingFunc		awaiting ;
		} ;

		// 割り込み関数での状態保存
		struct	InterruptSave
		{
			size_t				ap, fp ;
			LValue				valReturn ;
		} ;

	public:
		// 非同期実行用状態
		struct	AsyncState
		{
			bool			finished ;		// 実行は完了したか？
			ContextState	ctxSave ;		// 実行前の状態
			LValue			valRet ;		// 返された値
			LObjPtr			exception ;		// 処理されなかった例外
		} ;

	protected:
		LVirtualMachine&					m_vm ;			// 仮想マシン
		LCodeBuffer *						m_pCodeBuf ;	// 実行中のコードバッファ
		std::shared_ptr<LStackBuffer>		m_stack ;		// スタック
		ssize_t								m_ip ;			// コードポインタ
		LValue::Primitive					m_eximm ;		// 拡張即値

		LValue								m_valReturn ;	// 関数返り値

		LObjPtr								m_thrown ;		// 送出された例外オブジェクト
		LObjPtr								m_nullThrown ;
		LObjPtr								m_exception ;	// 処理中の例外オブジェクト
		std::unique_ptr<LObject::Monitor>	m_monitor ;		// 同期処理中のモニタ

		ExecutionStatus						m_status ;		// 実行状態
		AwaitingFunc						m_awaiting ;	// 待機状態の脱出条件
		volatile bool						m_interrupt ;	// コアループ離脱フラグ
		volatile bool						m_throwing ;	// 例外送出中
		size_t								m_jumped ;		// ジャンプ回数カウンタ（無限ループ離脱用）

		volatile InterruptSignal			m_signal ;		// 割り込み信号
		LObjPtr								m_pSignalObj ;

		std::recursive_mutex				m_mutexRun ;	// 実行中
		std::mutex							m_mutexStatus ;	// m_awaiting や m_signal の変更同期

		static thread_local LContext *		t_pCurrent ;

	public:
		LContext( LVirtualMachine& vm, size_t nInitSize = 0x100 ) ;
		virtual ~LContext( void ) ;

		// 現在のスレッドに設定された LContext 取得
		static LContext * GetCurrent( void )
		{
			return	t_pCurrent ;
		}

		// スレッドへ設定
		void SetCurrent( void )
		{
			t_pCurrent = this ;
		}

		class	Current
		{
		private:
			LContext *	m_prev ;
		public:
			Current( LContext& context )
			{
				m_prev = t_pCurrent ;
				t_pCurrent = &context ;
			}
			~Current( void )
			{
				t_pCurrent = m_prev ;
			}
		} ;

		// 関連付けられた仮想マシン
		LVirtualMachine& VM( void ) const
		{
			return	m_vm ;
		}

	public:
		////////////////////////////////////////////////////////////////////////////
		// 関数実行
		////////////////////////////////////////////////////////////////////////////

		// SyncCallFunction 関数は最も簡単な関数の実行方法ですが
		// 実行中のスクリプトから呼び出されたネイティブ関数から実行する場合、
		// 処理の中断・脱出ができないため注意が必要です。
		// 基本的に何も実行されていない状態から関数を呼び出す簡単な手段であって
		// LThreadObj からの利用が推奨されます。

		// AsyncCallFunction, AsyncProceed 関数も非同期的に実行できる点が
		// 異なりますが、基本的に SyncCallFunction と同じです。
		// LThreadObj からの利用が推奨されます。

		// InterruptFunction はネイティブ関数内からスクリプト関数を呼び出す場合の
		// 最も推奨される手段です。
		// この関数は実行中のスクリプトに割り込み的に関数の呼び出しを設定し、
		// 次に AsyncRun が実行されたときに割り込み関数が実行されるようにします。
		// 割り込み関数が終了すると元の実行位置に戻ります。
		// 割り込み関数が終了したときに実行すべきネイティブ関数(funcAfter)を指定でき
		// この関数内で GetReturnValue() を呼び出して関数の返り値を取得できますが
		// 割り込み等の理由によって必ず実行されるわけではないことに注意してください。

		// 関数実行（同期）
		std::tuple<LValue,LObjPtr>
			SyncCallFunction
				( LFunctionObj * pFunc,
					const LValue * pArgValues, size_t nArgCount ) ;

		// 関数実行開始（非同期）
		// （true が返された場合、関数は完了した）
		bool AsyncCallFunction
				( AsyncState& state,
					LFunctionObj * pFunc,
					const LValue * pArgValues, size_t nArgCount ) ;
		// 実行を継続する（非同期）
		// ※msecTimeout<0 の時、待機状態になるか、終了、又は外部割り込みなどの
		//   理由がない限り実行し続ける
		bool AsyncProceed( AsyncState& state, std::int64_t msecTimeout = 0 ) ;

		// 関数割り込み実行設定
		// ※funcReturned の第二引数には割り込み以前の返り値情報が格納されている。
		//   何もせずに復帰すると関数の返り値は実行コンテキストには反映されない。
		//   （割り込みは無作用）
		//   関数の返り値を設定してしたい場合には、第二引数に返り値を設定する。
		void InterruptFunction
			( LFunctionObj * pFunc,
				const LValue * pArgValues, size_t nArgCount,
					std::function<void(LContext&,LValue&)> funcReturned = nullptr ) ;

	protected:
		// 引数をプッシュ
		size_t PushArgument
			( LFunctionObj * pFunc,
				const LValue * pArgValues, size_t nArgCount,
				std::vector<LObjPtr>* pCastTempObjs ) ;
		// 実行状態をリセット
		void ResetState( ExecutionStatus status = statusRunning ) ;
		// 実行状態を保存
		void SaveContextState( ContextState& state ) const ;
		// 実行状態を復元
		void RestoreContextState( const ContextState& state ) ;
		// 実行結果を取得し完了する
		void FinishFunctionResult( AsyncState& state ) ;

	public:	// スタック
		std::shared_ptr<LStackBuffer>& Stack( void )
		{
			return	m_stack ;
		}
		LStackBuffer::Word GetArgAt( size_t index ) const
		{
			return	m_stack->ArgAt( index ) ;
		}

	public:	// 関数返り値
		// 関数返り値取得
		// （LObject* を受け取る場合には、受け取り側が RekeaseRef する）
		const LValue& GetReturnValue( void ) const
		{
			return	m_valReturn ;
		}
		// 関数返り値設定
		// （LObject* を返す場合には、返却前に AddRef する）
		void SetReturnValue( const LValue& val )
		{
			m_valReturn = val ;
		}

	public:	// 例外
		// 例外オブジェクト
		LObjPtr GetException( void ) const
		{
			return	m_exception ;
		}
		// 例外クリア
		void ClearException( void ) ;
		// 例外送出
		void ThrowException( LObjPtr pObj ) ;
		void ThrowException
			( const wchar_t * pwszErrMsg,
				const wchar_t * pwszClassName = nullptr ) ;
		void ThrowException( ErrorMessageIndex error ) ;
		static void ThrowExceptionError
			( const wchar_t * pwszErrMsg,
				const wchar_t * pwszClassName = nullptr ) ;
		static void ThrowExceptionError( ErrorMessageIndex error ) ;

	public:
		// 待機関数設定
		void SetAwaiting( AwaitingFunc func ) ;
		// 割り込み信号を設定する
		void SetInterruptSignal( InterruptSignal signal, LObjPtr pObj = nullptr ) ;

		// 非実行同期を取得する
		std::unique_lock<std::recursive_mutex> LockRunning( void )
		{
			return	std::unique_lock<std::recursive_mutex>( m_mutexRun ) ;
		}

	public:
		// 実行ステータス取得
		ExecutionStatus GetExecutionStatus( void ) const ;
		bool IsRunning( void ) const ;

		// 同期実行ループ（動作が完了するまで復帰しない）
		void SyncRun( void ) ;

		// 非同期実行ループ（待機状態になった場合脱出する）
		// ※msecTimeout<0 の時、待機状態になるか、終了、又は外部割り込みなどの
		//   理由がない限り実行し続ける
		ExecutionStatus AsyncRun( std::int64_t msecTimeout = 0 ) ;

	protected:
		// 割り込み信号処理
		bool ProcessSignal( void ) ;

		// 実行コアループ
		void RunCoreLoop( void ) ;

	public:	// オブジェクト生成
		// 文字列生成
		LStringObj * new_String( const LString& str ) ;
		LStringObj * new_String( const wchar_t * str ) ;
		// Integer 生成
		LIntegerObj * new_Integer( LLong val ) ;
		// Double 生成
		LDoubleObj * new_Double( LDouble val ) ;
		// ポインタ生成
		LPointerObj * new_Pointer( size_t nBytes ) ;
		LPointerObj * new_Pointer( const void * buf, size_t nBytes ) ;
		LPointerObj * new_Pointer( std::shared_ptr<LArrayBuffer> buf ) ;
		// Array 生成
		LArrayObj * new_Array( void ) ;
		LArrayObj * new_Array( LClass * pElementType ) ;
		// 例外オブジェクト生成
		LExceptionObj * new_Exception( ErrorMessageIndex err ) ;
		// オブジェクト生成
		LObject * new_Object( const wchar_t * pwszClassPath ) ;

	public:
		// 同期処理モニタを設定
		LObject::Monitor * MakeSyncMonitor( LObject::Monitor::Type type )
		{
			assert( m_monitor == nullptr ) ;
			m_monitor = std::make_unique<LObject::Monitor>( type ) ;
			return	m_monitor.get() ;
		}
		// 同期処理モニタを取得
		LObject::Monitor * GetSyncMonitor( void ) const
		{
			return	m_monitor.get() ;
		}
		// 同期処理モニタを解放
		void ReleaseSyncMonitor( void )
		{
			m_monitor = nullptr ;
		}

	public:
		// コードバッファ取得
		LCodeBuffer * GetCodeBuffer( void ) const
		{
			return	m_pCodeBuf ;
		}
		// 命令ポインタ取得
		ssize_t GetIP( void ) const
		{
			return	m_ip ;
		}

	protected:
		// 命令実行関数
		static const LCodeBuffer::PFN_Instruction	
						s_instruction[LCodeBuffer::codeInstructionCount] ;

		void instruction_NoOperation( const LCodeBuffer::Word& word ) ;
		void instruction_EnterFunc( const LCodeBuffer::Word& word ) ;
		void instruction_LeaveFunc( const LCodeBuffer::Word& word ) ;
		void instruction_EnterTry( const LCodeBuffer::Word& word ) ;
		void instruction_CallFinally( const LCodeBuffer::Word& word ) ;
		void instruction_RetFinally( const LCodeBuffer::Word& word ) ;
		void instruction_LeaveFinally( const LCodeBuffer::Word& word ) ;
		void instruction_LeaveTry( const LCodeBuffer::Word& word ) ;
		void instruction_Throw( const LCodeBuffer::Word& word ) ;
		void instruction_AllocStack( const LCodeBuffer::Word& word ) ;
		void instruction_FreeStack( const LCodeBuffer::Word& word ) ;
		void instruction_FreeStackLocal( const LCodeBuffer::Word& word ) ;
		void instruction_FreeLocalObj( const LCodeBuffer::Word& word ) ;
		void instruction_PushObjChain( const LCodeBuffer::Word& word ) ;
		void instruction_MakeObjChain( const LCodeBuffer::Word& word ) ;
		void instruction_Synchronize( const LCodeBuffer::Word& word ) ;
		void instruction_Return( const LCodeBuffer::Word& word ) ;
		void instruction_SetRetValue( const LCodeBuffer::Word& word ) ;
		void instruction_MoveAP( const LCodeBuffer::Word& word ) ;
		void instruction_Call( const LCodeBuffer::Word& word ) ;
		void instruction_CallVirtual( const LCodeBuffer::Word& word ) ;
		void instruction_CallIndirect( const LCodeBuffer::Word& word ) ;
		void instruction_ExImmPrefix( const LCodeBuffer::Word& word ) ;
		void instruction_ClearException( const LCodeBuffer::Word& word ) ;
		void instruction_LoadRetValue( const LCodeBuffer::Word& word ) ;
		void instruction_LoadException( const LCodeBuffer::Word& word ) ;
		void instruction_LoadArg( const LCodeBuffer::Word& word ) ;
		void instruction_LoadImm( const LCodeBuffer::Word& word ) ;
		void instruction_LoadLocal( const LCodeBuffer::Word& word ) ;
		void instruction_LoadLocalOffset( const LCodeBuffer::Word& word ) ;
		void instruction_LoadStack( const LCodeBuffer::Word& word ) ;
		void instruction_LoadFetchAddr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadFetchAddrOffset( const LCodeBuffer::Word& word ) ;
		void instruction_LoadFetchLAddr( const LCodeBuffer::Word& word ) ;
		void instruction_CheckLPtrAlign( const LCodeBuffer::Word& word ) ;
		void instruction_CheckPtrAlign( const LCodeBuffer::Word& word ) ;
		void instruction_LoadLPtr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadLPtrOffset( const LCodeBuffer::Word& word ) ;
		void instruction_LoadByPtr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadByLAddr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadByAddr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadLocalPtr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadObjectPtr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadLObjectPtr( const LCodeBuffer::Word& word ) ;
		void instruction_LoadObject( const LCodeBuffer::Word& word ) ;
		void instruction_NewObject( const LCodeBuffer::Word& word ) ;
		void instruction_AllocBuf( const LCodeBuffer::Word& word ) ;
		void instruction_RefObject( const LCodeBuffer::Word& word ) ;
		void instruction_FreeObject( const LCodeBuffer::Word& word ) ;
		void instruction_Move( const LCodeBuffer::Word& word ) ;
		void instruction_StoreLocalImm( const LCodeBuffer::Word& word ) ;
		void instruction_StoreLocal( const LCodeBuffer::Word& word ) ;
		void instruction_ExchangeLocal( const LCodeBuffer::Word& word ) ;
		void instruction_StoreByPtr( const LCodeBuffer::Word& word ) ;
		void instruction_StoreByLPtr( const LCodeBuffer::Word& word ) ;
		void instruction_StoreByLAddr( const LCodeBuffer::Word& word ) ;
		void instruction_StoreByAddr( const LCodeBuffer::Word& word ) ;
		void instruction_BinaryOperate( const LCodeBuffer::Word& word ) ;
		void instruction_UnaryOperate( const LCodeBuffer::Word& word ) ;
		void instruction_CastObject( const LCodeBuffer::Word& word ) ;
		void instruction_Jump( const LCodeBuffer::Word& word ) ;
		void instruction_JumpConditional( const LCodeBuffer::Word& word ) ;
		void instruction_JumpNonConditional( const LCodeBuffer::Word& word ) ;
		void instruction_GetElement( const LCodeBuffer::Word& word ) ;
		void instruction_GetElementAs( const LCodeBuffer::Word& word ) ;
		void instruction_GetElementName( const LCodeBuffer::Word& word ) ;
		void instruction_SetElement( const LCodeBuffer::Word& word ) ;
		void instruction_SetElementAs( const LCodeBuffer::Word& word ) ;
		void instruction_ObjectToInt( const LCodeBuffer::Word& word ) ;
		void instruction_ObjectToFloat( const LCodeBuffer::Word& word ) ;
		void instruction_ObjectToString( const LCodeBuffer::Word& word ) ;
		void instruction_IntToObject( const LCodeBuffer::Word& word ) ;
		void instruction_FloatToObject( const LCodeBuffer::Word& word ) ;
		void instruction_IntToFloat( const LCodeBuffer::Word& word ) ;
		void instruction_UintToFloat( const LCodeBuffer::Word& word ) ;
		void instruction_FloatToInt( const LCodeBuffer::Word& word ) ;
		void instruction_FloatToUint( const LCodeBuffer::Word& word ) ;
		
		void micro_instruction_FreeFor( ssize_t si ) ;
		void micro_instruction_FreeCount( size_t n ) ;
		void micro_instruction_CallFinally( void ) ;
		void micro_instruction_RetFinally( void ) ;
		void micro_instruction_LeaveFinally( void ) ;
		void micro_instruction_LeaveTry( void ) ;
		void micro_instruction_VerifyJumped( void ) ;
		void micro_instruction_Throw( void ) ;
		void micro_instruction_ThrowNullException( void ) ;
		void micro_instruction_ThrowIndexOutOfBounds( void ) ;
		void micro_instruction_ThrowPointerOutOfBounds( void ) ;
		void micro_instruction_ThrowAlignmentMismatch( void ) ;
		void micro_instruction_ThrowUnimplemented( LFunctionObj * pFunc ) ;
		void micro_instruction_ThrowNoVirtualVector
					( LClass * pThisClass, LClass * pVirtClass ) ;

		void micro_instruction_Call( LFunctionObj * pFunc ) ;

		std::uint8_t * micro_instruction_FetchAddr
					( size_t ptr, ssize_t off, size_t range, std::uint8_t type ) ;
		LValue::Primitive micro_instruction_CheckAlignment
					( std::uint8_t * ptr, std::uint8_t type ) ;

		// データ丸め関数（ロード・ストア）
		typedef LValue::Primitive (*PFN_MoveStackWord)( const LValue::Primitive& val ) ;

		static const PFN_MoveStackWord	s_pfnLoadLocal[LType::typeAllCount] ;
		static const PFN_MoveStackWord	s_pfnStoreLocal[LType::typeAllCount] ;

		static LValue::Primitive load_stack_bool( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_int8( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_uint8( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_int16( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_uint16( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_int32( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_uint32( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_int64( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_uint64( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_float( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_double( const LValue::Primitive& val ) ;
		static LValue::Primitive load_stack_ptr( const LValue::Primitive& val ) ;

		static LValue::Primitive store_stack_bool( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_int8( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_uint8( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_int16( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_uint16( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_int32( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_uint32( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_int64( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_uint64( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_float( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_double( const LValue::Primitive& val ) ;
		static LValue::Primitive store_stack_ptr( const LValue::Primitive& val ) ;

		// ロード関数
		typedef LValue::Primitive (*PFN_LoadFromPointer)( const void * ptr ) ;
		static const PFN_LoadFromPointer	s_pfnLoadFromPointer[LType::typeAllCount] ;

		static LValue::Primitive load_ptr_bool( const void * ptr ) ;
		static LValue::Primitive load_ptr_int8( const void * ptr ) ;
		static LValue::Primitive load_ptr_uint8( const void * ptr ) ;
		static LValue::Primitive load_ptr_int16( const void * ptr ) ;
		static LValue::Primitive load_ptr_uint16( const void * ptr ) ;
		static LValue::Primitive load_ptr_int32( const void * ptr ) ;
		static LValue::Primitive load_ptr_uint32( const void * ptr ) ;
		static LValue::Primitive load_ptr_int64( const void * ptr ) ;
		static LValue::Primitive load_ptr_uint64( const void * ptr ) ;
		static LValue::Primitive load_ptr_float( const void * ptr ) ;
		static LValue::Primitive load_ptr_double( const void * ptr ) ;
		static LValue::Primitive load_ptr_ptr( const void * ptr ) ;

		// ストア関数
		typedef void (*PFN_StoreIntoPointer)( void * ptr, const LValue::Primitive& val ) ;
		static const PFN_StoreIntoPointer	s_pfnStoreIntoPointer[LType::typeAllCount] ;

		static void store_ptr_bool( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_int8( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_uint8( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_int16( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_uint16( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_int32( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_uint32( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_int64( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_uint64( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_float( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_double( void * ptr, const LValue::Primitive& val ) ;
		static void store_ptr_ptr( void * ptr, const LValue::Primitive& val ) ;

	} ;

}

#endif

