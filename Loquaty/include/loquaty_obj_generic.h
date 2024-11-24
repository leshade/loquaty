
#ifndef	__LOQUATY_OBJ_GENERIC_H__
#define	__LOQUATY_OBJ_GENERIC_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// ユーザー定義オブジェクト基底
	//////////////////////////////////////////////////////////////////////////

	class	LGenericObj	: public LMapObj
	{
	public:
		std::vector<LType>					m_types ;
		std::shared_ptr<LArrayBufStorage>	m_pBuf ;
		LPtr<LPointerObj>					m_pPtr ;

	public:
		LGenericObj( LClass * pClass ) ;
		LGenericObj( const LGenericObj& obj ) ;

	public:	// プリミティブな操作の定義
		// 要素型情報取得
		virtual LType GetElementTypeAt( size_t index ) const ;
		virtual LType GetElementTypeAs( const wchar_t * name ) const ;
		// 要素の型情報設定
		virtual void SetElementTypeAt( size_t index, const LType& type ) ;
		virtual void SetElementTypeAs( const wchar_t * name, const LType& type ) ;

		// 保持するバッファへのポインタを返す
		virtual LPointerObj * GetBufferPoiner( void ) ;

		// 文字列として評価
		virtual bool AsString( LString& str ) const ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		void CloneFrom( const LGenericObj& obj ) ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

	public:
		// 要素の削除
		virtual void RemoveElementAt( size_t index ) ;
		virtual void RemoveAll( void ) ;

	public:
		// データの配置情報に基づいてバッファを確保する
		// （既にバッファを持っている場合、必要に応じて拡張する）
		void AllocateDataBuffer( void ) ;

	} ;

}

#endif

