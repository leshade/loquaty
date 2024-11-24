
#ifndef	__LOQUATY_OBJECT_H__
#define	__LOQUATY_OBJECT_H__	1

namespace	Loquaty
{
	class	LSyncLockerObj ;
	class	LAwaitingNotification ;

	//////////////////////////////////////////////////////////////////////////
	// 基底 Object
	//////////////////////////////////////////////////////////////////////////

	class	LObject	: public Object
	{
	public:
		// 排他同期
		struct	Synchronized
		{
			LContext *	pContext ;
			int			countSync ;
		} ;

		// オブジェクトモニタ
		class	Monitor
		{
		public:
			// モニタ種類
			enum	Type
			{
				typeSynchronized,
				typeNotification,
			} ;
			Type						m_type ;
			Monitor *					m_pNextChain ;
			LObject *					m_pAttached ;
			std::condition_variable		m_cvSignal ;
			volatile bool				m_flagSignaled ;

		public:
			// 構築関数
			Monitor( Type type )
				: m_type(type), m_pNextChain(nullptr),
					m_pAttached(nullptr), m_flagSignaled(false) {}
		} ;

	protected:
		LClass *			m_pClass ;		// ※簡略化のためクラスに対して参照カウンタは操作しない
		std::atomic<int>	m_countRef ;	// 参照カウンタ

		Synchronized *		m_pSynchronized ;
		Monitor *			m_pFirstMonitor ;

		friend class LAwaitingNotification ;

		LOQUATY_DLL_EXPORT
		static std::mutex	s_mutex ;		// 本当は全体ミューテックスじゃないほうが良いけど…

	public:
		LObject( LClass * pClass = nullptr )
			: m_pClass( pClass ),
				m_countRef(1),
				m_pSynchronized( nullptr ),
				m_pFirstMonitor( nullptr ) {}
		LObject( const LObject& obj )
			: m_pClass( obj.m_pClass ), m_countRef(1),
				m_pSynchronized( nullptr ),
				m_pFirstMonitor( nullptr ) {}

		// 消滅関数は仮想関数とする
		virtual ~LObject( void )
		{
		}

		// クラス取得
		LClass * GetClass( void ) const
		{
			return	m_pClass ;
		}
		// クラス設定
		void SetClass( LClass * pClass )
		{
			m_pClass = pClass ;
		}

	public:	// 参照カウンタ
		// 参照カウンタ加算
		int AddRef( void )
		{
			return	m_countRef.fetch_add(1) + 1 ;
		}
		static LObject * AddRef( LObject * pObj )
		{
			if ( pObj != nullptr )
			{
				pObj->AddRef() ;
			}
			return	pObj ;
		}
		// 参照カウンタ減算
		int ReleaseRef( void )
		{
			int	r = m_countRef.fetch_add(-1) ;
			if ( r == 1 )
			{
				FinalizeObject() ;
				delete	this ;
			}
			return	r - 1 ;
		}
		static LObject * ReleaseRef( LObject * pObj )
		{
			if ( pObj != nullptr )
			{
				if ( pObj->ReleaseRef() == 0 )
				{
					pObj = nullptr ;
				}
			}
			return	pObj ;
		}

	public:	// 基本的なオブジェクト操作の定義
		// 要素数
		virtual size_t GetElementCount( void ) const ;
		// 要素取得
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * GetElementAt( size_t index ) const ;
		virtual LObject * GetElementAs( const wchar_t * name ) const ;
		// 要素検索
		virtual ssize_t FindElementAs( const wchar_t * name ) const ;
		// 要素名取得
		virtual const wchar_t * GetElementNameAt( LString& strName, size_t index ) const ;
		// 要素型情報取得
		virtual LType GetElementTypeAt( size_t index ) const ;
		virtual LType GetElementTypeAs( const wchar_t * name ) const ;
		// 要素設定（pObj には AddRef されたポインタを渡す）
		// （以前の要素を返す / ReleaseRef が必要）
		virtual LObject * SetElementAt( size_t index, LObject * pObj ) ;
		virtual LObject * SetElementAs( const wchar_t * name, LObject * pObj ) ;
		// 要素の型情報設定
		virtual void SetElementTypeAt( size_t index, const LType& type ) ;
		virtual void SetElementTypeAs( const wchar_t * name, const LType& type ) ;
		// 保持するバッファへのポインタを返す
		// （この関数で取得したポインタへ変更を加えてはならない / ポインタを移動する場合は Clone する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LPointerObj * GetBufferPoiner( void ) ;

		// 整数値として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsInteger( LLong& value ) const ;
		// 浮動小数点として評価
		//（評価可能な場合には true を返し value に値を設定する）
		virtual bool AsDouble( LDouble& value ) const ;
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;
		// （式表現に近い）文字列に変換
		virtual bool AsExpression( LString& str ) const ;
		static LString ToExpression( LObject * pObj ) ;

		// 強制型変換
		// （可能なら AddRef されたポインタを返す / 不可能なら nullptr）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * CastClassTo( LClass * pClass ) ;

		// 複製する（要素も全て複製処理する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * DuplicateObject( void ) const ;

		// オブジェクト解放処理
		virtual void FinalizeObject( void ) ;
		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

	public:	// 要素アクセス・ヘルパー関数
		virtual LLong GetElementLongAt( size_t index, LLong valDefault = 0 ) ;
		virtual LDouble GetElementDoubleAt( size_t index, LDouble valDefault = 0.0 ) ;
		virtual LString GetElementStringAt( size_t index, const wchar_t * valDefault = nullptr ) ;
		virtual LLong GetElementLongAs( const wchar_t * name, LLong valDefault = 0 ) ;
		virtual LDouble GetElementDoubleAs( const wchar_t * name, LDouble valDefault = 0.0 ) ;
		virtual LString GetElementStringAs( const wchar_t * name, const wchar_t * valDefault = nullptr ) ;
		virtual void SetElementLongAt( size_t index, LLong value ) ;
		virtual void SetElementDoubleAt( size_t index, LDouble value ) ;
		virtual void SetElementStringAt( size_t index, const wchar_t * value ) ;
		virtual void SetElementLongAs( const wchar_t * name, LLong value ) ;
		virtual void SetElementDoubleAs( const wchar_t * name, LDouble value ) ;
		virtual void SetElementStringAs( const wchar_t * name, const wchar_t * value ) ;

	public:
		// 排他同期取得（成功なら true, false の場合は WaitSynchronized で取得）
		bool LockSynchronized( Synchronized * sync, LContext * context ) ;
		// 排他同期取得（LockSynchronized が false の場合 / true を返却するまで繰り返す）
		// （msecTimeout < 0 の時はタイムアウトしないで待つ）
		bool WaitSynchronized
			( Synchronized * sync, LContext * context, std::int64_t msecTimeout = -1 ) ;
		// 排他同期解放
		bool UnlockSynchronized( LContext * context ) ;

		// モニタ待機（msecTimeout < 0 の時はタイムアウトしないで待つ）
		// 通知があった場合は true を返す
		bool WaitNotification( LContext& context, std::int64_t msecTimeout = -1 ) ;

		// 非同期モニタ待機
		std::shared_ptr<LAwaitingNotification>
			AwaitNotification( LContext& context, std::int64_t msecTimeout = -1 ) ;

		// モニタ通知と分離（必ず s_mutex 同期中に呼び出す）
		void NotifyMonitor( Monitor::Type typeNotify ) ;
		void NotifyAllMonitor( Monitor::Type typeNotify ) ;

		// モニタ追加（必ず s_mutex 同期中に呼び出す）
		void AddMonitor( Monitor * monitor ) ;
		// モニタ分離（必ず s_mutex 同期中に呼び出す）
		void DetachMonitor( Monitor * monitor ) ;

		// モニタ同期処理に必要な locker を取得
		std::unique_lock<std::mutex> GetMonitorLock( void )
		{
			return	std::unique_lock<std::mutex>( s_mutex ) ;
		}

	public:
		// void finalize()
		static void method_finalize( LContext& context ) ;
		// Class getClass() const
		static void method_getClass( LContext& context ) ;
		// String toString() const
		static void method_toString( LContext& context ) ;
		// void notify()
		static void method_notify( LContext& context ) ;
		// void notifyAll()
		static void method_notifyAll( LContext& context ) ;
		// void wait()
		static void method_wait0( LContext& context ) ;
		// void wait( long timeout )
		static void method_wait1( LContext& context ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// スマートポインタ
	// ※ std::shared_ptr を使いたいところだが、
	//    ポインタを直接使いたい場面があるため固有実装する必要がある
	//////////////////////////////////////////////////////////////////////////

	template <class T>	class	LPtr
	{
	private:
		T *	m_pObject ;

	public:
		LPtr( T * p = nullptr ) : m_pObject( p ) {}
		LPtr( const LPtr<T>& ptr )
			: m_pObject( ptr.m_pObject )
		{
			LObject::AddRef( m_pObject ) ;
		}
		~LPtr( void )
		{
			LObject::ReleaseRef( m_pObject ) ;
		}
		const LPtr<T>& operator = ( const LPtr<T>& ptr )
		{
			LObject::AddRef( ptr.m_pObject ) ;
			LObject::ReleaseRef( m_pObject ) ;
			m_pObject = ptr.m_pObject ;
			return	*this ;
		}
		const LPtr<T>& operator = ( T * p )
		{
			LObject::ReleaseRef( m_pObject ) ;
			m_pObject = p ;
			return	*this ;
		}
		operator LPtr<LObject> ( void )
		{
			return	LPtr<LObject>( Get() ) ;
		}
		T * Get( void ) const
		{
			LObject::AddRef( m_pObject ) ;
			return	m_pObject ;
		}
		T * Ptr( void ) const
		{
			return	m_pObject ;
		}
		T * Detach( void )
		{
			T *	pObject = m_pObject ;
			m_pObject = nullptr ;
			return	pObject ;
		}
		T& operator * ( void )
		{
			return	*m_pObject ;
		}
		const T& operator * ( void ) const
		{
			return	*m_pObject ;
		}
		T * operator -> ( void ) const
		{
			return	m_pObject ;
		}
		bool operator == ( const LPtr<T>& p ) const
		{
			return	m_pObject == p.m_pObject ;
		}
		bool operator == ( T * p ) const
		{
			return	m_pObject == p ;
		}
		bool operator != ( const LPtr<T>& p ) const
		{
			return	m_pObject != p.m_pObject ;
		}
		bool operator != ( T * p ) const
		{
			return	m_pObject != p ;
		}
		LPtr<T> Clone( void ) const
		{
			return	(m_pObject != nullptr)
						? m_pObject->CloneObject() : nullptr ;
		}
	} ;

	typedef	LPtr<LObject>	LObjPtr ;


	//////////////////////////////////////////////////////////////////////////
	// Synchronized Locker オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LSyncLockerObj	: public LObject
	{
	protected:
		Synchronized	m_sync ;
		LObjPtr			m_pObj ;
		LContext *		m_pContext ;
		bool			m_flagLocked ;

	public:
		LSyncLockerObj
			( LClass * pClass, LObjPtr pObj, LContext * pContext )
			: LObject( pClass ), m_pObj( pObj ),
				m_pContext( pContext ), m_flagLocked( false )
		{
			assert( pObj != nullptr ) ;
			assert( pContext != nullptr ) ;

			m_flagLocked = m_pObj->LockSynchronized( &m_sync, m_pContext ) ;
		}
		bool IsLocked( void ) const
		{
			return	m_flagLocked ;
		}
		bool WaitSync( std::int64_t msecTimeout = 0 )
		{
			assert( !m_flagLocked ) ;
			if ( !m_flagLocked )
			{
				m_flagLocked =
					m_pObj->WaitSynchronized( &m_sync, m_pContext, msecTimeout ) ;
			}
			return	m_flagLocked ;
		}
		virtual ~LSyncLockerObj( void ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// タイムアウト残り時間
	//////////////////////////////////////////////////////////////////////////

	class	LTimeoutRemaining	: public Object
	{
	public:
		std::chrono::system_clock::time_point
						m_tpStart ;
		std::int64_t	m_msecTimeout ;
		bool			m_flagFinished ;

	public:
		LTimeoutRemaining( std::int64_t timeout )
			: m_msecTimeout( timeout ),
				m_flagFinished( false ),
				m_tpStart( std::chrono::system_clock::now() ){}

		std::int64_t Remaining( std::int64_t timeout )
		{
			if ( m_msecTimeout < 0 )
			{
				// -1 の時は待ち続ける
				return	timeout ;
			}
			std::int64_t	msecElapsed =
				std::chrono::duration_cast<std::chrono::milliseconds>
					( std::chrono::system_clock::now() - m_tpStart ).count() ;
			std::int64_t	msecLeft = m_msecTimeout
										- std::min( msecElapsed, m_msecTimeout ) ;
			return	std::min( timeout, msecLeft ) ;
		}
		bool IsTimeout( void ) const
		{
			std::int64_t	msecElapsed =
				std::chrono::duration_cast<std::chrono::milliseconds>
					( std::chrono::system_clock::now() - m_tpStart ).count() ;
			return	(m_msecTimeout >= 0)
						&& (msecElapsed >= m_msecTimeout) ;
		}
		void Finish( void )
		{
			m_flagFinished = true ;
			m_msecTimeout = 0 ;
		}
		bool IsFinished( void ) const
		{
			return	m_flagFinished ;
		}

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 非同期オブジェクト通知待機
	//////////////////////////////////////////////////////////////////////////

	class	LAwaitingNotification	: public LTimeoutRemaining
	{
	public:
		LObjPtr					m_pObj ;
		LObject::Synchronized *	m_pPrevSync ;
		std::unique_lock<std::mutex>
								m_lock ;
		bool					m_flagWaiting ;
		bool					m_flagNotified ;
		LObject::Monitor		m_monNotify ;
		LObject::Monitor		m_monSync ;

	public:
		LAwaitingNotification
			( LObjPtr pObj, LObject::Synchronized * pSync,
				std::unique_lock<std::mutex>& lock, std::int64_t timeout )
			: LTimeoutRemaining( timeout ),
				m_pObj( pObj ), m_pPrevSync( pSync ),
				m_lock( std::move(lock) ),
				m_flagWaiting( true ), m_flagNotified( false ),
				m_monNotify( LObject::Monitor::typeNotification ),
				m_monSync( LObject::Monitor::typeSynchronized ) { }

		virtual ~LAwaitingNotification( void ) ;

		bool Wait( std::int64_t timeout ) ;

	} ;


}


#endif

