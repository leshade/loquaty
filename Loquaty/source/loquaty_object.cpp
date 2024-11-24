
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////////
// Native オブジェクトの基底
//////////////////////////////////////////////////////////////////////////////

#if !defined(_DLL_IMPORT_LOQUATY)

// new/delete オーバーロード
//（主に DLL 側と EXE でアロケーションを統一するため）
LOQUATY_DLL_DECL(void *) Object::operator new ( size_t bytes )
{
	return	malloc( bytes ) ;
}

LOQUATY_DLL_DECL(void *) Object::operator new ( size_t bytes, void * ptr )
{
	return	ptr ;
}

LOQUATY_DLL_DECL(void *) Object::operator new
	( size_t bytes, const char * file, int line )	// MFC DEBUG_NEW 形式
{
#if	defined(_MSC_VER) && defined(_DEBUG)
	return	_malloc_dbg( bytes, _NORMAL_BLOCK, file, line ) ;
#else
	return	malloc( bytes ) ;
#endif
}

LOQUATY_DLL_DECL(void) Object::operator delete ( void * ptr )
{
	free( ptr ) ;
}

#endif



//////////////////////////////////////////////////////////////////////////////
// 基底 Object
//////////////////////////////////////////////////////////////////////////////

LOQUATY_DLL_DECL
( std::mutex	LObject::s_mutex ) ;


// 要素数
size_t LObject::GetElementCount( void ) const
{
	return	0 ;
}

// 要素取得
LObject * LObject::GetElementAt( size_t index ) const
{
	return	nullptr ;
}

LObject * LObject::GetElementAs( const wchar_t * name ) const
{
	return	nullptr ;
}

// 要素検索
ssize_t LObject::FindElementAs( const wchar_t * name ) const
{
	return	-1 ;
}

// 要素名取得
const wchar_t * LObject::GetElementNameAt( LString& strName, size_t index ) const
{
	return	nullptr ;
}

// 要素型情報取得
LType LObject::GetElementTypeAt( size_t index ) const
{
	LObjPtr	pElement = GetElementAt( index ) ;
	if ( pElement != nullptr )
	{
		return	LType( pElement->GetClass() ) ;
	}
	return	LType() ;
}

LType LObject::GetElementTypeAs( const wchar_t * name ) const
{
	LObjPtr	pElement = GetElementAs( name ) ;
	if ( pElement != nullptr )
	{
		return	LType( pElement->GetClass() ) ;
	}
	return	LType() ;
}

// 要素設定（pObj には AddRef されたポインタを渡す）
LObject * LObject::SetElementAt( size_t index, LObject * pObj )
{
	return	pObj ;
}

LObject * LObject::SetElementAs( const wchar_t * name, LObject * pObj )
{
	return	pObj ;
}

// 要素の型情報設定
void LObject::SetElementTypeAt( size_t index, const LType& type )
{
}

void LObject::SetElementTypeAs( const wchar_t * name, const LType& type )
{
}

// 保持するバッファへのポインタを返す
LPointerObj * LObject::GetBufferPoiner( void )
{
	return	nullptr ;
}

// 整数値として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LObject::AsInteger( LLong& value ) const
{
	return	false ;
}

// 浮動小数点として評価
//（評価可能な場合には true を返し value に値を設定する）
bool LObject::AsDouble( LDouble& value ) const
{
	return	false ;
}

// 文字列として評価
bool LObject::AsString( LString& str ) const
{
	str = L"" ;
	return	true ;
}

// （式表現に近い）文字列に変換
bool LObject::AsExpression( LString& str ) const
{
	return	AsString( str ) ;
}

LString LObject::ToExpression( LObject * pObj )
{
	if ( pObj != nullptr )
	{
		LString	str ;
		if ( pObj->AsExpression( str ) )
		{
			return	str ;
		}
	}
	return	L"null" ;
}

// 強制型変換
// （可能なら AddRef されたポインタを返す / 不可能なら nullptr）
LObject * LObject::CastClassTo( LClass * pClass )
{
	if ( (m_pClass != nullptr) && m_pClass->IsInstanceOf( pClass ) )
	{
		AddRef() ;
		return	this ;
	}
	else if ( dynamic_cast<LPointerClass*>( pClass ) != nullptr )
	{
		return	GetBufferPoiner() ;
	}
	else if ( dynamic_cast<LStringClass*>( pClass ) != nullptr )
	{
		LString	str ;
		if ( AsString( str ) )
		{
			return	new LStringObj( pClass, str ) ;
		}
	}
	else if ( dynamic_cast<LStringBufClass*>( pClass ) != nullptr )
	{
		LString	str ;
		if ( AsString( str ) )
		{
			return	new LStringBufObj( pClass, str ) ;
		}
	}
	else if ( dynamic_cast<LIntegerClass*>( pClass ) != nullptr )
	{
		LLong	value ;
		if ( AsInteger( value ) )
		{
			return	new LIntegerObj( pClass, value ) ;
		}
	}
	else if ( dynamic_cast<LDoubleClass*>( pClass ) != nullptr )
	{
		LDouble	value ;
		if ( AsDouble( value ) )
		{
			return	new LDoubleObj( pClass, value ) ;
		}
	}
	return	nullptr ;
}

// 複製する（要素も全て複製処理する）
LObject * LObject::CloneObject( void ) const
{
	return	new LObject( *this ) ;
}

// 複製する（要素は参照する形で複製処理する）
LObject * LObject::DuplicateObject( void ) const
{
	return	new LObject( *this ) ;
}

// オブジェクト解放処理
void LObject::FinalizeObject( void )
{
	if ( m_pClass != nullptr )
	{
		m_pClass->InvokeFinalizer( this ) ;
	}
}

// 内部リソースを解放する
void LObject::DisposeObject( void )
{
}

// 要素アクセス・ヘルパー関数
LLong LObject::GetElementLongAt( size_t index, LLong valDefault )
{
	LLong	val ;
	LObjPtr	pObj = GetElementAt( index ) ;
	if ( (pObj != nullptr) && pObj->AsInteger( val ) )
	{
		return	val ;
	}
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( (pPtrClass != nullptr)
		&& pPtrClass->GetBufferType().IsPrimitive() )
	{
		LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
		if ( pPtr != nullptr )
		{
			return	pPtr->LoadIntegerAt
						( index * pPtrClass->GetBufferType().GetDataBytes(),
									pPtrClass->GetBufferType().GetPrimitive() ) ;
		}
	}
	return	valDefault ;
}

LDouble LObject::GetElementDoubleAt( size_t index, LDouble valDefault )
{
	LDouble	val ;
	LObjPtr	pObj = GetElementAt( index ) ;
	if ( (pObj != nullptr) && pObj->AsDouble( val ) )
	{
		return	val ;
	}
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( (pPtrClass != nullptr)
		&& pPtrClass->GetBufferType().IsPrimitive() )
	{
		LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
		if ( pPtr != nullptr )
		{
			return	pPtr->LoadDoubleAt
						( index * pPtrClass->GetBufferType().GetDataBytes(),
									pPtrClass->GetBufferType().GetPrimitive() ) ;
		}
	}
	return	valDefault ;
}

LString LObject::GetElementStringAt( size_t index, const wchar_t * valDefault )
{
	LString	str ;
	LObjPtr	pObj = GetElementAt( index ) ;
	if ( (pObj != nullptr) && pObj->AsString( str ) )
	{
		return	str ;
	}
	return	valDefault ;
}

LLong LObject::GetElementLongAs( const wchar_t * name, LLong valDefault )
{
	LLong	val ;
	LObjPtr	pObj = GetElementAs( name ) ;
	if ( (pObj != nullptr) && pObj->AsInteger( val ) )
	{
		return	val ;
	}
	LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
	LArrangement::Desc	desc ;
	if ( (pPtr != nullptr)
		&& m_pClass->GetProtoArrangemenet().GetDescAs( desc, name )
		&& desc.m_type.IsPrimitive() )
	{
		return	pPtr->LoadIntegerAt( desc.m_location, desc.m_type.GetPrimitive() ) ;
	}
	return	valDefault ;
}

LDouble LObject::GetElementDoubleAs( const wchar_t * name, LDouble valDefault )
{
	LDouble	val ;
	LObjPtr	pObj = GetElementAs( name ) ;
	if ( (pObj != nullptr) && pObj->AsDouble( val ) )
	{
		return	val ;
	}
	LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
	LArrangement::Desc	desc ;
	if ( (pPtr != nullptr)
		&& m_pClass->GetProtoArrangemenet().GetDescAs( desc, name )
		&& desc.m_type.IsPrimitive() )
	{
		return	pPtr->LoadDoubleAt( desc.m_location, desc.m_type.GetPrimitive() ) ;
	}
	return	valDefault ;
}

LString LObject::GetElementStringAs( const wchar_t * name, const wchar_t * valDefault )
{
	LString	str ;
	LObjPtr	pObj = GetElementAs( name ) ;
	if ( (pObj != nullptr) && pObj->AsString( str ) )
	{
		return	str ;
	}
	return	valDefault ;
}

void LObject::SetElementLongAt( size_t index, LLong value )
{
	assert( m_pClass != nullptr ) ;
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( (pPtrClass != nullptr)
		&& pPtrClass->GetBufferType().IsPrimitive() )
	{
		LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
		if ( pPtr != nullptr )
		{
			pPtr->StoreIntegerAt
				( index * pPtrClass->GetBufferType().GetDataBytes(),
							pPtrClass->GetBufferType().GetPrimitive(), value ) ;
			return ;
		}
	}
	LClass *	pIntClass = m_pClass->VM().GetIntegerObjClass() ;
	LObject::ReleaseRef( SetElementAt( index, new LIntegerObj( pIntClass, value ) ) ) ;
}

void LObject::SetElementDoubleAt( size_t index, LDouble value )
{
	assert( m_pClass != nullptr ) ;
	LPointerClass *	pPtrClass = dynamic_cast<LPointerClass*>( m_pClass ) ;
	if ( (pPtrClass != nullptr)
		&& pPtrClass->GetBufferType().IsPrimitive() )
	{
		LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
		if ( pPtr != nullptr )
		{
			pPtr->StoreDoubleAt
				( index * pPtrClass->GetBufferType().GetDataBytes(),
							pPtrClass->GetBufferType().GetPrimitive(), value ) ;
			return ;
		}
	}
	LClass *	pDblClass = m_pClass->VM().GetDoubleObjClass() ;
	LObject::ReleaseRef( SetElementAt( index, new LDoubleObj( pDblClass, value ) ) ) ;
}

void LObject::SetElementStringAt( size_t index, const wchar_t * value )
{
	assert( m_pClass != nullptr ) ;
	LClass *	pStrClass = m_pClass->VM().GetStringBufClass() ;
	LObject::ReleaseRef( SetElementAt( index, new LStringBufObj( pStrClass, value ) ) ) ;
}

void LObject::SetElementLongAs( const wchar_t * name, LLong value )
{
	assert( m_pClass != nullptr ) ;
	LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
	LArrangement::Desc	desc ;
	if ( (pPtr != nullptr)
		&& m_pClass->GetProtoArrangemenet().GetDescAs( desc, name )
		&& desc.m_type.IsPrimitive() )
	{
		pPtr->StoreIntegerAt
			( desc.m_location, desc.m_type.GetPrimitive(), value ) ;
		return ;
	}
	LClass *	pIntClass = m_pClass->VM().GetIntegerObjClass() ;
	LObject::ReleaseRef( SetElementAs( name, new LIntegerObj( pIntClass, value ) ) ) ;
}

void LObject::SetElementDoubleAs( const wchar_t * name, LDouble value )
{
	assert( m_pClass != nullptr ) ;
	LPtr<LPointerObj>	pPtr = GetBufferPoiner() ;
	LArrangement::Desc	desc ;
	if ( (pPtr != nullptr)
		&& m_pClass->GetProtoArrangemenet().GetDescAs( desc, name )
		&& desc.m_type.IsPrimitive() )
	{
		pPtr->StoreDoubleAt
			( desc.m_location, desc.m_type.GetPrimitive(), value ) ;
		return ;
	}
	LClass *	pDblClass = m_pClass->VM().GetDoubleObjClass() ;
	LObject::ReleaseRef( SetElementAs( name, new LDoubleObj( pDblClass, value ) ) ) ;
}

void LObject::SetElementStringAs( const wchar_t * name, const wchar_t * value )
{
	assert( m_pClass != nullptr ) ;
	LClass *	pStrClass = m_pClass->VM().GetStringBufClass() ;
	LObject::ReleaseRef( SetElementAs( name, new LStringBufObj( pStrClass, value ) ) ) ;
}

// 排他同期取得（成功なら true, false の場合は WaitSynchronized で取得）
bool LObject::LockSynchronized( Synchronized * sync, LContext * context )
{
	assert( sync != nullptr ) ;
	assert( context != nullptr ) ;
	for ( ; ; )
	{
		std::lock_guard<std::mutex>	lock( s_mutex ) ;
		if ( m_pSynchronized != nullptr )
		{
			if ( m_pSynchronized->pContext == context )
			{
				sync = m_pSynchronized ;
				sync->countSync ++ ;
				break ;
			}
			else
			{
				AddMonitor( context->MakeSyncMonitor( Monitor::typeSynchronized ) ) ;
				return	false ;
			}
		}
		else
		{
			m_pSynchronized = sync ;
			sync->pContext = context ;
			sync->countSync = 1 ;
			break ;
		}
	}
	return	true ;
}

// 排他同期取得（LockSynchronized が false の場合 / true を返却するまで繰り返す）
// （msecTimeout < 0 の時はタイムアウトしないで待つ）
bool LObject::WaitSynchronized
	( Synchronized * sync, LContext * context, std::int64_t msecTimeout )
{
	assert( sync != nullptr ) ;
	assert( context != nullptr ) ;

	Monitor *	pMonitor = context->GetSyncMonitor() ;
	assert( pMonitor != nullptr ) ;

	std::unique_lock<std::mutex>	lock( s_mutex ) ;
	if ( m_pSynchronized != nullptr )
	{
		if ( msecTimeout < 0 )
		{
			pMonitor->m_cvSignal.wait
				( lock, [this](){ return m_pSynchronized == nullptr ; } ) ;
		}
		else if ( msecTimeout > 0 )
		{
			pMonitor->m_cvSignal.wait_for
				( lock, std::chrono::milliseconds(msecTimeout) ) ;
		}
	}
	assert( lock.owns_lock() ) ;
	if ( m_pSynchronized == nullptr )
	{
		m_pSynchronized = sync ;
		sync->pContext = context ;
		sync->countSync = 1 ;
		//
		DetachMonitor( pMonitor ) ;
		context->ReleaseSyncMonitor() ;
		return	true ;
	}
	return	false ;
}

// 排他同期解放
bool LObject::UnlockSynchronized( LContext * context )
{
	assert( context != nullptr ) ;
	Monitor *	pMonitor = context->GetSyncMonitor() ;

	std::lock_guard<std::mutex>	lock( s_mutex ) ;
	bool			flagUnlocked = false ;
	Synchronized *	sync = m_pSynchronized ;
	if ( (sync != nullptr) && (sync->pContext == context) )
	{
		if ( (-- (sync->countSync)) <= 0 )
		{
			m_pSynchronized = nullptr ;
			flagUnlocked = true ;
			NotifyAllMonitor( Monitor::typeSynchronized ) ;
		}
	}
	if ( pMonitor != nullptr )
	{
		DetachMonitor( pMonitor ) ;
		context->ReleaseSyncMonitor() ;
	}
	return	flagUnlocked ;
}

// モニタ待機（msecTimeout < 0 の時はタイムアウトしないで待つ）
// 通知があった場合は true を返す
bool LObject::WaitNotification( LContext& context, std::int64_t msecTimeout )
{
	std::unique_lock<std::mutex>	lock( s_mutex ) ;

	if ( (m_pSynchronized != nullptr)
		&& (m_pSynchronized->pContext == &context) )
	{
		//
		// 一時的に排他同期解放
		//
		Synchronized *	sync = m_pSynchronized ;
		m_pSynchronized = nullptr ;
		NotifyAllMonitor( Monitor::typeSynchronized ) ;
		//
		// 通知を受けるまで待機
		//
		Monitor	monNotify( Monitor::typeNotification ) ;
		AddMonitor( &monNotify ) ;
		//
		bool	flagNotified = false ;
		if ( msecTimeout < 0 )
		{
			monNotify.m_cvSignal.wait( lock ) ;
			flagNotified = true ;
		}
		else
		{
			std::cv_status result =
				monNotify.m_cvSignal.wait_for
					( lock, std::chrono::milliseconds(msecTimeout) ) ;
			flagNotified = (result != std::cv_status::timeout) ;
		}
		assert( lock.owns_lock() ) ;
		DetachMonitor( &monNotify ) ;
		//
		// 排他状態を復元
		//
		Monitor	monSync( Monitor::typeSynchronized ) ;
		AddMonitor( &monSync ) ;
		while ( m_pSynchronized != nullptr )
		{
			assert( lock.owns_lock() ) ;
			monSync.m_cvSignal.wait_for
					( lock, std::chrono::milliseconds(msecTimeout) ) ;
		}
		m_pSynchronized = sync ;
		DetachMonitor( &monSync ) ;
		return	flagNotified ;
	}
	else
	{
		context.ThrowExceptionError( exceptionDonotOwnMonitor ) ;
		return	false ;
	}
}

// 非同期モニタ待機
std::shared_ptr<LAwaitingNotification>
		LObject::AwaitNotification
			( LContext& context, std::int64_t msecTimeout )
{
	std::unique_lock<std::mutex>	lock( s_mutex ) ;

	if ( (m_pSynchronized != nullptr)
		&& (m_pSynchronized->pContext == &context) )
	{
		// 待機オブジェクト
		AddRef() ;
		std::shared_ptr<LAwaitingNotification>
				pAwaiting = std::make_shared<LAwaitingNotification>
					( LObjPtr( this ), m_pSynchronized, lock, msecTimeout ) ;

		// 一時的に排他同期解放
		m_pSynchronized = nullptr ;
		NotifyAllMonitor( Monitor::typeSynchronized ) ;

		// モニタを追加
		AddMonitor( &(pAwaiting->m_monNotify) ) ;

		return	pAwaiting ;
	}
	else
	{
		context.ThrowExceptionError( exceptionDonotOwnMonitor ) ;
		return	nullptr ;
	}
}

// モニタ通知と分離（必ず s_mutex 同期中に呼び出す）
void LObject::NotifyMonitor( Monitor::Type typeNotify )
{
	Monitor *	pLast = m_pFirstMonitor ;
	Monitor *	pPrev = nullptr ;
	while ( pLast != nullptr )
	{
		Monitor *	pNext = pLast->m_pNextChain ;
		if ( pLast->m_type == typeNotify )
		{
			pLast->m_pNextChain = nullptr ;
			pLast->m_cvSignal.notify_all() ;
			break ;
		}
		else
		{
			if ( pPrev != nullptr )
			{
				pPrev->m_pNextChain = pLast ;
			}
			else
			{
				m_pFirstMonitor = pLast ;
			}
			pPrev = pLast ;
		}
		pLast = pNext ;
	}
	if ( pPrev != nullptr )
	{
		pPrev->m_pNextChain = nullptr ;
	}
	else
	{
		m_pFirstMonitor = nullptr ;
	}
}

void LObject::NotifyAllMonitor( Monitor::Type typeNotify )
{
	Monitor *	pLast = m_pFirstMonitor ;
	Monitor *	pPrev = nullptr ;
	m_pFirstMonitor = nullptr ;
	while ( pLast != nullptr )
	{
		Monitor *	pNext = pLast->m_pNextChain ;
		if ( pLast->m_type == typeNotify )
		{
			pLast->m_pNextChain = nullptr ;
			pLast->m_flagSignaled = true ;
			pLast->m_cvSignal.notify_all() ;
		}
		else
		{
			if ( pPrev != nullptr )
			{
				pPrev->m_pNextChain = pLast ;
			}
			else
			{
				m_pFirstMonitor = pLast ;
			}
			pPrev = pLast ;
		}
		pLast = pNext ;
	}
	if ( pPrev != nullptr )
	{
		pPrev->m_pNextChain = nullptr ;
	}
}

// モニタ追加（必ず s_mutex 同期中に呼び出す）
void LObject::AddMonitor( Monitor * monitor )
{
	assert( monitor->m_pAttached == nullptr ) ;
	monitor->m_pNextChain = m_pFirstMonitor ;
	monitor->m_pAttached = this ;
	m_pFirstMonitor = monitor ;
}

// モニタ分離（必ず s_mutex 同期中に呼び出す）
void LObject::DetachMonitor( Monitor * monitor )
{
	Monitor *	pLast = m_pFirstMonitor ;
	Monitor *	pPrev = nullptr ;
	while ( pLast != nullptr )
	{
		Monitor *	pNext = pLast->m_pNextChain ;
		if ( pLast == monitor )
		{
			pLast->m_pNextChain = nullptr ;
			pLast->m_flagSignaled = true ;
			pLast->m_cvSignal.notify_all() ;
			break ;
		}
		else
		{
			if ( pPrev != nullptr )
			{
				pPrev->m_pNextChain = pLast ;
			}
			else
			{
				m_pFirstMonitor = pLast ;
			}
			pPrev = pLast ;
		}
		pLast = pNext ;
	}
	if ( pPrev != nullptr )
	{
		pPrev->m_pNextChain = nullptr ;
	}
	else
	{
		m_pFirstMonitor = nullptr ;
	}
	assert( monitor->m_pAttached == this ) ;
	monitor->m_pAttached = nullptr ;
}

// void finalize()
void LObject::method_finalize( LContext& _context )
{
	LQT_RETURN_VOID() ;
}

// Class getClass()
void LObject::method_getClass( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LObject, pObj ) ;

	LQT_RETURN_OBJECT( LObject::AddRef( pObj->GetClass() ) ) ;
}

// String toString() const
void LObject::method_toString( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LObject, pObj ) ;

	LString	str ;
	pObj->AsString( str ) ;

	LQT_RETURN_STRING( str ) ;
}

// void notify()
void LObject::method_notify( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LObject, pObj) ;

	std::unique_lock<std::mutex>	lock = pObj->GetMonitorLock() ;
	pObj->NotifyMonitor( LObject::Monitor::typeNotification ) ;

	LQT_RETURN_VOID() ;
}

// void notifyAll()
void LObject::method_notifyAll( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LObject, pObj) ;

	std::unique_lock<std::mutex>	lock = pObj->GetMonitorLock() ;
	pObj->NotifyAllMonitor( LObject::Monitor::typeNotification ) ;

	LQT_RETURN_VOID() ;
}

// void wait()
void LObject::method_wait0( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LObject, pObj ) ;

	std::shared_ptr<LAwaitingNotification>
			pAwait = pObj->AwaitNotification( _context, -1 ) ;
	if ( pAwait != nullptr )
	{
		_context.SetAwaiting
				( [pAwait]( std::int64_t msecTimeout )
			{
				return	pAwait->Wait( msecTimeout ) ;
			} ) ;
	}

	LQT_RETURN_VOID() ;
}

// void wait( long timeout )
void LObject::method_wait1( LContext& _context )
{
	LRuntimeArgList	_arglist( _context ) ;
	LQT_FUNC_THIS_OBJ( LObject, pObj ) ;
	LQT_FUNC_ARG_LONG( nTimeout ) ;

	std::shared_ptr<LAwaitingNotification>
			pAwait = pObj->AwaitNotification( _context, nTimeout ) ;
	if ( pAwait != nullptr )
	{
		_context.SetAwaiting
				( [pAwait]( std::int64_t msecTimeout )
			{
				return	pAwait->Wait( msecTimeout ) ;
			} ) ;
	}

	LQT_RETURN_VOID() ;
}




//////////////////////////////////////////////////////////////////////////////
// Synchronized Locker オブジェクト
//////////////////////////////////////////////////////////////////////////////

LSyncLockerObj::~LSyncLockerObj( void )
{
	if ( m_flagLocked )
	{
		m_pObj->UnlockSynchronized( m_pContext ) ;
	}
	else
	{
		std::unique_lock<std::mutex>	lock = m_pObj->GetMonitorLock() ;
		m_pObj->DetachMonitor( m_pContext->GetSyncMonitor() ) ;
	}
}



//////////////////////////////////////////////////////////////////////////////
// 非同期オブジェクト通知待機
//////////////////////////////////////////////////////////////////////////////

LAwaitingNotification::~LAwaitingNotification( void )
{
	assert( m_lock.owns_lock() ) ;
	assert( m_pObj != nullptr ) ;
	if ( m_flagWaiting )
	{
		m_pObj->DetachMonitor( &m_monNotify ) ;
	}
	else
	{
		m_pObj->DetachMonitor( &m_monSync ) ;
	}
}

bool LAwaitingNotification::Wait( std::int64_t timeout )
{
	if ( IsFinished() )
	{
		return	true ;
	}
	if ( m_flagWaiting )
	{
		// 通知を待機
		bool	flagTimeout = false ;
		m_flagNotified |= m_monNotify.m_flagSignaled ;
		if ( !m_flagNotified )
		{
			timeout = Remaining( timeout ) ;
			if ( timeout < 0 )
			{
				m_monNotify.m_cvSignal.wait( m_lock ) ;
				m_flagNotified = true ;
			}
			else
			{
				std::cv_status result =
					m_monNotify.m_cvSignal.wait_for
						( m_lock, std::chrono::milliseconds(timeout) ) ;
				m_flagNotified = (result != std::cv_status::timeout) ;
			}
			m_monNotify.m_flagSignaled = false ;
			flagTimeout = (timeout == 0) ;
		}
		if ( m_flagNotified || flagTimeout )
		{
			m_flagWaiting = false ;
			m_pObj->DetachMonitor( &m_monNotify ) ;
			m_pObj->AddMonitor( &m_monSync ) ;
		}
	}
	if ( !m_flagWaiting )
	{
		// 排他状態を復元
		if ( m_pObj->m_pSynchronized != nullptr )
		{
			assert( m_lock.owns_lock() ) ;
			m_monSync.m_cvSignal.wait_for
					( m_lock, std::chrono::milliseconds(timeout) ) ;
		}
		if ( m_pObj->m_pSynchronized == nullptr )
		{
			m_pObj->m_pSynchronized = m_pPrevSync ;
			Finish() ;
			return	true ;
		}
	}
	return	false ;
}

