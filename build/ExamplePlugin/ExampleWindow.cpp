
#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
// サンプル・ウィンドウ
//////////////////////////////////////////////////////////////////////////////

LExampleWindow::LExampleWindow( LVirtualMachine& vm, LObjPtr pObj )
	: m_pObj( pObj ),
		m_hwnd( nullptr ),
		m_closing( true ),
		m_painted( false ),
		m_width( 0 ),
		m_height( 0 ),
		m_scale( 1 ),
		m_hDCView( nullptr ),
		m_hBitmap( nullptr ),
		m_hDefBitmap( nullptr ),
		m_pPixels( nullptr )
{
	// GDI+ 初期化
	Gdiplus::GdiplusStartup( &m_gdipToken, &m_gdipStartupInput, 0 ) ;

	// コールバック用スレッド
	m_threadCallback = vm.new_Thread() ;
}

LExampleWindow::~LExampleWindow( void )
{
	Close() ;

	m_grphView = nullptr ;
	m_imgView = nullptr ;

	// GDI+ 終了
	Gdiplus::GdiplusShutdown( m_gdipToken ) ;
}

// ウィンドウ作成
bool LExampleWindow::Create
	( const wchar_t * pwszCaption, LUint width, LUint height, LUint scale )
{
	// バックバッファ作成
	m_imgView = std::make_unique<Gdiplus::Bitmap>( width, height, PixelFormat32bppRGB ) ;
	m_grphView = std::make_unique<Gdiplus::Graphics>( m_imgView.get() ) ;
	m_width = width ;
	m_height = height ;
	m_scale = scale ;

	// バックバッファ（DIB）作成
	m_hDCView = ::CreateCompatibleDC( nullptr ) ;

	memset( &m_bmiView, 0, sizeof(BITMAPINFO) ) ;
	m_bmiView.bmiHeader.biSize = sizeof(BITMAPINFOHEADER) ;
	m_bmiView.bmiHeader.biBitCount = 32 ;
	m_bmiView.bmiHeader.biWidth = width ;
	m_bmiView.bmiHeader.biHeight = height ;
	m_bmiView.bmiHeader.biCompression = BI_RGB ;
	m_bmiView.bmiHeader.biPlanes = 1 ;
	//
	m_hBitmap =
		::CreateDIBSection
			( m_hDCView, &m_bmiView,
				DIB_RGB_COLORS, (void**) &m_pPixels, nullptr, 0 ) ;
	if ( m_hBitmap != nullptr )
	{
		m_hDefBitmap = (HBITMAP) ::SelectObject( m_hDCView, m_hBitmap ) ;
	}

	// ウィンドウクラス登録
	WNDCLASS	wndcls ;
	memset( &wndcls, 0, sizeof(wndcls) ) ;
	wndcls.style = 0 ;
	wndcls.lpfnWndProc = &LExampleWindow::ExampleWindowProc ;
	wndcls.cbClsExtra = 0 ;
	wndcls.cbWndExtra = sizeof(LExampleWindow*) ;
	wndcls.hInstance = ::GetModuleHandle( nullptr ) ;
	wndcls.hCursor  = ::LoadCursor( nullptr, IDC_ARROW ) ;
	wndcls.hbrBackground = (HBRUSH) ::GetStockObject( BLACK_BRUSH ) ;
	wndcls.lpszClassName  = "ExampleWindow" ;

	::RegisterClass( &wndcls ) ;

	// ウィンドウ作成
	assert( m_hwnd == nullptr ) ;
	std::string	strCaption = LString(pwszCaption).ToString() ;
	const int	cxScreen = ::GetSystemMetrics( SM_CXSCREEN ) ;
	const int	cyScreen = ::GetSystemMetrics( SM_CYSCREEN ) ;
	m_hwnd = ::CreateWindowEx
		( 0, wndcls.lpszClassName, strCaption.c_str(),
			WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX,
			(cxScreen - width * scale) / 2,
			(cyScreen - height * scale) / 2,
			width * scale, height * scale,
			nullptr, nullptr, ::GetModuleHandle( nullptr ), this ) ;
	if ( m_hwnd == nullptr )
	{
		return	false ;
	}
	m_closing = false ;
	::ShowWindow( m_hwnd, SW_SHOW ) ;

//	::SetTimer( m_hwnd, 1, 33, nullptr ) ;
	::timeBeginPeriod( 1 ) ;
	m_dwLastTimer = ::timeGetTime() ;

	return	true ;
}

// ウィンドウを閉じる
void LExampleWindow::Close( void )
{
	if ( m_hwnd != nullptr )
	{
		::DestroyWindow( m_hwnd ) ;
		m_hwnd = nullptr ;
		m_pObj.Release() ;
	}
	if ( m_hDCView != nullptr )
	{
		::SelectObject( m_hDCView, m_hDefBitmap ) ;
		::DeleteObject( m_hBitmap ) ;
		::DeleteDC( m_hDCView ) ;
		m_hBitmap = nullptr ;
		m_hDefBitmap = nullptr ;
		m_hDCView = nullptr ;
	}
}

// メッセージポンプ
bool LExampleWindow::PumpMessage( void )
{
	m_painted = false ;
	while ( !m_closing && !m_painted )
	{
		MSG	msg ;
		if ( ::PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			::TranslateMessage( &msg ) ;
			::DispatchMessage( &msg ) ;
		}
		else
		{
			if ( ::timeGetTime() - m_dwLastTimer >= 33 )
			{
				// 手動で WM_TIMER を呼び出す
				// ※デフォルトだと精度が悪いので
				::SendMessage( m_hwnd, WM_TIMER, 1, 0 ) ;
			}
			break ;
		}
	}
	return	!m_closing ;
}

// 塗りつぶし矩形描画
void LExampleWindow::FillRectangle
		( const LRGBA& rgba, int x, int y, LUint width, LUint height )
{
	if ( m_grphView != nullptr )
	{
		Gdiplus::SolidBrush	brush( Gdiplus::Color( rgba.a, rgba.r, rgba.g, rgba.b ) ) ;
		m_grphView->FillRectangle
			( &brush, (INT) x, (INT) y, (INT) width, (INT) height ) ;
	}
}

// 塗りつぶし多角形描画
void LExampleWindow::FillPolygon( const LRGBA& rgba, const LPointF* pPoints, LUint count )
{
	if ( m_grphView != nullptr )
	{
		std::vector<Gdiplus::PointF>	points ;
		points.resize( (size_t) count ) ;
		for ( size_t i = 0; i < count; i ++ )
		{
			points[i].X = pPoints[i].x ;
			points[i].Y = pPoints[i].y ;
		}
		Gdiplus::SolidBrush	brush( Gdiplus::Color( rgba.a, rgba.r, rgba.g, rgba.b ) ) ;
		m_grphView->FillPolygon( &brush, points.data(), (INT) count ) ;
	}
}

// 連続直線描画
void LExampleWindow::DrawLines( const LRGBA& rgba, const LPointF* pPoints, LUint count )
{
	if ( m_grphView != nullptr )
	{
		std::vector<Gdiplus::PointF>	points ;
		points.resize( (size_t) count ) ;
		for ( size_t i = 0; i < count; i ++ )
		{
			points[i].X = pPoints[i].x ;
			points[i].Y = pPoints[i].y ;
		}
		Gdiplus::Pen	pen( Gdiplus::Color( rgba.a, rgba.r, rgba.g, rgba.b ) ) ;
		m_grphView->DrawLines( &pen, points.data(), (INT) count ) ;
	}
}

// テキスト描画
void LExampleWindow::DrawText
	( int x, int y, int size, const LRGBA& rgba, const wchar_t * pwszText )
{
	if ( m_grphView != nullptr )
	{
		Gdiplus::SolidBrush	brush( Gdiplus::Color( rgba.a, rgba.r, rgba.g, rgba.b ) ) ;
		Gdiplus::Font		font( Gdiplus::FontFamily::GenericMonospace(),
									(Gdiplus::REAL) size,
									Gdiplus::FontStyleRegular,
									Gdiplus::UnitPixel ) ;
		Gdiplus::PointF		point( (Gdiplus::REAL) x, (Gdiplus::REAL) y ) ;

		m_grphView->DrawString( pwszText, -1, &font, point, &brush ) ;
	}
}

// ウィンドウ関数
LRESULT CALLBACK LExampleWindow::ExampleWindowProc
		( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	LExampleWindow *	pWnd = nullptr ;
	if ( msg == WM_NCCREATE )
	{
		LPCREATESTRUCT	pcs = reinterpret_cast<LPCREATESTRUCT>( lp ) ;
		pWnd = reinterpret_cast<LExampleWindow*>( pcs->lpCreateParams ) ;
		::SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( pWnd ) ) ;
		pWnd->m_hwnd = hwnd ;
	}
	else
	{
		pWnd = reinterpret_cast<LExampleWindow*>
					( ::GetWindowLongPtr( hwnd, GWLP_USERDATA ) ) ;
		if ( pWnd != nullptr )
		{
			if ( pWnd->m_hwnd != hwnd )
			{
				pWnd = nullptr ;
			}
		}
	}
	if ( pWnd != nullptr )
	{
		return	pWnd->OnWindowProc( hwnd, msg, wp, lp ) ;
	}
	return	::DefWindowProc( hwnd, msg, wp, lp ) ;
}

LRESULT LExampleWindow::OnWindowProc
		( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	if ( msg == WM_PAINT )
	{
		OnPaint() ;

		PAINTSTRUCT	ps ;
		::BeginPaint( m_hwnd, &ps ) ;

/*
		Gdiplus::Graphics	grph( ps.hdc ) ;
		grph.DrawImage
			( m_imgView.get(), 0, 0,
				(INT) (m_width * m_scale), (INT) (m_height * m_scale) ) ;
*/
		// ※Graphics::DrawImage はあまりに遅いので……
		Gdiplus::Rect		rect( 0, 0, (INT) m_width, (INT) m_height ) ;
		Gdiplus::BitmapData	bmData ;
		m_imgView->LockBits
			( &rect, Gdiplus::ImageLockModeRead,
						PixelFormat32bppRGB, &bmData ) ;
		for ( LUint y = 0; y < m_height; y ++ )
		{
			const std::uint32_t*	pSrc =
				(const std::uint32_t*) (((std::uint8_t*) bmData.Scan0)
											+ (int) y * bmData.Stride) ;
			std::uint32_t*	pDst =
				(std::uint32_t*) (m_pPixels
					+ (int) (m_height - y - 1) * m_bmiView.bmiHeader.biWidth * 4) ;
			for ( LUint x = 0; x < m_width; x ++ )
			{
				pDst[x] = pSrc[x] ;
			}
		}
		m_imgView->UnlockBits( &bmData ) ;

		::StretchBlt
			( ps.hdc, 0, 0,
				(int) (m_width * m_scale), (int) (m_height * m_scale),
				m_hDCView, 0, 0, (int) m_width, (int) m_height, SRCCOPY ) ;

		::EndPaint( m_hwnd, &ps ) ;
		m_painted = true ;
		return	0 ;
	}
	else if ( msg == WM_TIMER )
	{
		m_dwLastTimer = ::timeGetTime() ;
		OnTimer() ;

		::InvalidateRect( hwnd, nullptr, false ) ;
		::UpdateWindow( hwnd ) ;

		return	0 ;
	}
	else if ( msg == WM_KEYDOWN )
	{
		OnKeyDown( (int) wp ) ;
		return	0 ;
	}
	else if ( msg == WM_KEYUP )
	{
		OnKeyUp( (int) wp ) ;
		return	0 ;
	}
	else if ( msg == WM_CLOSE )
	{
		m_closing = true ;
		return	0 ;
	}
	else if ( msg == WM_CREATE )
	{
		RECT	rectWindow, rectClient ;
		::GetWindowRect( hwnd, &rectWindow ) ;
		::GetClientRect( hwnd, &rectClient ) ;

		const int	cxScreen = ::GetSystemMetrics( SM_CXSCREEN ) ;
		const int	cyScreen = ::GetSystemMetrics( SM_CYSCREEN ) ;
		const int	width = (rectWindow.right - rectWindow.left)
							+ ((int) (m_width * m_scale)
								- (rectClient.right - rectClient.left)) ;
		const int	height = (rectWindow.bottom - rectWindow.top)
							+ ((int) (m_height * m_scale)
								- (rectClient.bottom - rectClient.top)) ;
		::MoveWindow
			( hwnd, (cxScreen - width) / 2,
					(cyScreen - height) / 2, width, height, true ) ;
		return	0 ;
	}
	return	::DefWindowProc( hwnd, msg, wp, lp ) ;
}

// タイマー処理
void LExampleWindow::OnTimer( void )
{
	auto	[valRet, except] =
		m_threadCallback->SyncCallFunctionAs( m_pObj, L"onTimer", nullptr, 0 ) ;
	if ( except != nullptr )
	{
		LString	lstr ;
		except->AsString( lstr ) ;

		std::string	str = lstr.ToString() ;
		printf( "onTimer:exception:%s\n", str.c_str() ) ;
	}
}

// 描画処理
void LExampleWindow::OnPaint( void )
{
	m_grphView->Clear( Gdiplus::Color( 0, 0, 0 ) ) ;

	auto	[valRet, except] =
		m_threadCallback->SyncCallFunctionAs( m_pObj, L"onPaint", nullptr, 0 ) ;
	if ( except != nullptr )
	{
		LString	lstr ;
		except->AsString( lstr ) ;

		std::string	str = lstr.ToString() ;
		printf( "onPaint:exception:%s\n", str.c_str() ) ;
	}
}

// キー押下
void LExampleWindow::OnKeyDown( int vkey )
{
	LValue	valArg[1] =
	{
		LValue( LType::typeInt32, LValue::MakeLong( vkey ) ),
	} ;
	auto	[valRet, except] =
		m_threadCallback->SyncCallFunctionAs( m_pObj, L"onKeyDown", valArg, 1 ) ;
	if ( except != nullptr )
	{
		LString	lstr ;
		except->AsString( lstr ) ;

		std::string	str = lstr.ToString() ;
		printf( "onKeyDown:exception:%s\n", str.c_str() ) ;
	}
}

// キー解放
void LExampleWindow::OnKeyUp( int vkey )
{
	LValue	valArg[1] =
	{
		LValue( LType::typeInt32, LValue::MakeLong( vkey ) ),
	} ;
	auto	[valRet, except] =
		m_threadCallback->SyncCallFunctionAs( m_pObj, L"onKeyUp", valArg, 1 ) ;
	if ( except != nullptr )
	{
		LString	lstr ;
		except->AsString( lstr ) ;

		std::string	str = lstr.ToString() ;
		printf( "onKeyUp:exception:%s\n", str.c_str() ) ;
	}
}




// native 関数実装
//////////////////////////////////////////////////////////////////////////////

// ExampleWindow( )
IMPL_LOQUATY_CONSTRUCTOR(ExampleWindow)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_INIT_NOBJ( LExampleWindow, pThis, ( _context.VM(), LQT_ARG_OBJECT(0) ) ) ;

	LQT_RETURN_VOID() ;
}

// boolean createWindow( String caption, uint width, uint height, uint scale )
IMPL_LOQUATY_FUNC(ExampleWindow_createWindow)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;
	LQT_FUNC_ARG_STRING( caption ) ;
	LQT_FUNC_ARG_UINT( width ) ;
	LQT_FUNC_ARG_UINT( height ) ;
	LQT_FUNC_ARG_UINT( scale ) ;

	LBoolean	valRet = pThis->Create( caption.c_str(), width, height, scale ) ;

	LQT_RETURN_BOOL( valRet ) ;
}

// void closeWindow( )
IMPL_LOQUATY_FUNC(ExampleWindow_closeWindow)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;

	pThis->Close() ;

	LQT_RETURN_VOID() ;
}

// boolean pumpMessage( )
IMPL_LOQUATY_FUNC(ExampleWindow_pumpMessage)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;

	LBoolean	valRet = pThis->PumpMessage() ;

	LQT_RETURN_BOOL( valRet ) ;
}

// void fillRectangle( const RGBA* rgba, int x, int y, uint width, uint height )
IMPL_LOQUATY_FUNC(ExampleWindow_fillRectangle)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;
	LQT_FUNC_ARG_STRUCT( LRGBA, rgba ) ;
	LQT_VERIFY_NULL_PTR( rgba ) ;
	LQT_FUNC_ARG_INT( x ) ;
	LQT_FUNC_ARG_INT( y ) ;
	LQT_FUNC_ARG_UINT( width ) ;
	LQT_FUNC_ARG_UINT( height ) ;

	pThis->FillRectangle( *rgba, x, y, width, height ) ;

	LQT_RETURN_VOID() ;
}

// void fillPolygon( const RGBA* rgba, const PointF* pPoints, uint count )
IMPL_LOQUATY_FUNC(ExampleWindow_fillPolygon)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;
	LQT_FUNC_ARG_STRUCT( LRGBA, rgba ) ;
	LQT_VERIFY_NULL_PTR( rgba ) ;
	LQT_FUNC_ARG_STRUCT( LPointF, pPoints ) ;
	LQT_VERIFY_NULL_PTR( pPoints ) ;
	LQT_FUNC_ARG_UINT( count ) ;

	pThis->FillPolygon( *rgba, pPoints, count ) ;

	LQT_RETURN_VOID() ;
}

// void drawLines( const RGBA* rgba, const PointF* pPoints, uint count )
IMPL_LOQUATY_FUNC(ExampleWindow_drawLines)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;
	LQT_FUNC_ARG_STRUCT( LRGBA, rgba ) ;
	LQT_VERIFY_NULL_PTR( rgba ) ;
	LQT_FUNC_ARG_STRUCT( LPointF, pPoints ) ;
	LQT_VERIFY_NULL_PTR( pPoints ) ;
	LQT_FUNC_ARG_UINT( count ) ;

	pThis->DrawLines( *rgba, pPoints, count ) ;

	LQT_RETURN_VOID() ;
}

// void drawText( int x, int y, int size, const RGBA* rgba, String text )
IMPL_LOQUATY_FUNC(ExampleWindow_drawText)
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LExampleWindow, pThis ) ;
	LQT_FUNC_ARG_INT( x ) ;
	LQT_FUNC_ARG_INT( y ) ;
	LQT_FUNC_ARG_INT( size ) ;
	LQT_FUNC_ARG_STRUCT( LRGBA, rgba ) ;
	LQT_VERIFY_NULL_PTR( rgba ) ;
	LQT_FUNC_ARG_STRING( text ) ;

	pThis->DrawText( x, y, size, *rgba, text.c_str() ) ;

	LQT_RETURN_VOID() ;
}



