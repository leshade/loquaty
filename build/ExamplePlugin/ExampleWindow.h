
#ifndef __EXAMPLE_WINDOW_H__
#define __EXAMPLE_WINDOW_H__

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment( lib, "winmm.lib" )


// RGB 構造体
struct	LRGBA
{
	LUint8	r, g, b, a ;
} ;

// PointF 構造体
struct	LPointF
{
	LFloat	x, y ;
} ;


//////////////////////////////////////////////////////////////////////////////
// サンプル・ウィンドウ
//////////////////////////////////////////////////////////////////////////////

class	LExampleWindow	: public Object
{
protected:
	LObjPtr		m_pObj ;		// オブジェクト（循環参照になることに注意）
	HWND		m_hwnd ;		// Win32 ウィンドウハンドル
	bool		m_closing ;
	bool		m_painted ;
	LUint		m_width ;		// 表示バッファのサイズ
	LUint		m_height ;
	LUint		m_scale ;

	// GDI+ 初期化
	ULONG_PTR							m_gdipToken ;
	Gdiplus::GdiplusStartupInput		m_gdipStartupInput ;

	// 表示用バッファ
	std::unique_ptr<Gdiplus::Graphics>	m_grphView ;
	std::unique_ptr<Gdiplus::Bitmap>	m_imgView ;

	// 表示用バッファ（DIB）※Gdiplus::Bitmap ⇒ DC への描画が遅すぎるので…
	HDC			m_hDCView ;
	HBITMAP		m_hBitmap ;
	HBITMAP		m_hDefBitmap ;
	BITMAPINFO	m_bmiView ;
	uint8_t *	m_pPixels ;

	// 以前のタイマー
	DWORD		m_dwLastTimer ;

	// コールバック関数呼び出し用スレッド
	LPtr<LThreadObj>	m_threadCallback ;

public:
	LExampleWindow( LVirtualMachine& vm, LObjPtr pObj ) ;
	virtual ~LExampleWindow( void ) ;

	// ウィンドウ作成
	bool Create
		( const wchar_t * pwszCaption, LUint width, LUint height, LUint scale ) ;
	// ウィンドウを閉じる
	void Close( void ) ;
	// メッセージポンプ
	bool PumpMessage( void ) ;
	// 塗りつぶし矩形描画
	void FillRectangle( const LRGBA& rgba, int x, int y, LUint width, LUint height ) ;
	// 塗りつぶし多角形描画
	void FillPolygon( const LRGBA& rgba, const LPointF* pPoints, LUint count ) ;
	// 連続直線描画
	void DrawLines( const LRGBA& rgba, const LPointF* pPoints, LUint count ) ;
	// テキスト描画
	void DrawText( int x, int y, int size,
					const LRGBA& rgba, const wchar_t * pwszText ) ;

private:
	// ウィンドウ関数
	static LRESULT CALLBACK ExampleWindowProc
			( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) ;
	virtual LRESULT OnWindowProc
			( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) ;

	// タイマー処理
	virtual void OnTimer( void ) ;
	// 描画処理
	virtual void OnPaint( void ) ;
	// キー押下
	virtual void OnKeyDown( int vkey ) ;
	// キー解放
	virtual void OnKeyUp( int vkey ) ;

} ;



// native 関数宣言
//////////////////////////////////////////////////////////////////////////////

DECL_LOQUATY_CONSTRUCTOR(ExampleWindow);
DECL_LOQUATY_FUNC(ExampleWindow_createWindow);
DECL_LOQUATY_FUNC(ExampleWindow_closeWindow);
DECL_LOQUATY_FUNC(ExampleWindow_pumpMessage);
DECL_LOQUATY_FUNC(ExampleWindow_fillRectangle);
DECL_LOQUATY_FUNC(ExampleWindow_fillPolygon);
DECL_LOQUATY_FUNC(ExampleWindow_drawLines);
DECL_LOQUATY_FUNC(ExampleWindow_drawText);



#endif


