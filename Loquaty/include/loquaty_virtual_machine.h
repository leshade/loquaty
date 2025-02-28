
#ifndef	__LOQUATY_VIRTUAL_MACHINE_H__
#define	__LOQUATY_VIRTUAL_MACHINE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Loquaty 仮想マシン
	//////////////////////////////////////////////////////////////////////////

	class	LVirtualMachine	: public LObject
	{
	public:
		// 基本クラス
		enum	BasicClassIndex
		{
			classClass,			// ※ Class クラスのクラス
			classNamespace,
			classObject,
			classNativeObject,
			classStructure,
			classDataArray,
			classPointer,
			classInteger,
			classDouble,
			classArray,
			classMap,
			classString,
			classStringBuf,
			classFunction,
			classException,
			classTask,
			classThread,
			classBasicCount,
		} ;

	protected:
		// 基本的なクラス
		LClass *	m_pBasicClass[classBasicCount] ;
		LClass *	m_pPrimitivePtrClass[LType::typePrimitiveCount] ;
		LClass *	m_pPrimitiveCPtrClass[LType::typePrimitiveCount] ;
		LClass *	m_pVoidPointerClass ;
		LClass *	m_pConstVoidPointerClass ;

		// 大域空間
		LPtr<LNamespace>	m_pGlobal ;
		bool				m_refGlobal ;

		// モジュール
		LModuleProducer		m_producer ;

		// ソースファイル
		LSourceProducer		m_sources ;

		// パッケージ
		std::vector<LPackagePtr>
							m_packages ;

		// 実行中のスレッド・リスト
		LThreadObj *			m_pFirstThread ;
		std::mutex				m_mutexThreads ;

		// デバッガー
		LDebugger *				m_pDebugger ;	

		LOQUATY_DLL_EXPORT
		static LVirtualMachine *	s_pCurrent ;

	public:
		LVirtualMachine( void ) ;
		~LVirtualMachine( void ) ;

	public:
		// 初期化
		void Initialize( void ) ;
		void InitializeRef( LVirtualMachine& vmRef ) ;
		// 解放
		void Release( void ) ;

	public:
		// グローバルに設定された LVirtualMachine
		static LVirtualMachine * GetCurrentVM( void ) ;
		void SetCurrentVM( void ) ;

		// グローバル空間
		LPtr<LNamespace> Global( void ) const ;

		// モジュール・プロデューサー
		LModuleProducer& ModuleProducer( void ) ;

		// ソース・プロデューサー
		LSourceProducer& SourceProducer( void ) ;

		// パッケージ追加
		void AddPackage( LPackagePtr package ) ;

		// パッケージ・リスト
		const std::vector<LPackagePtr>& GetPackageList( void ) const ;

	public:
		// 基本クラス取得
		LClass * GetBasicClass( BasicClassIndex clsIndex ) const ;
		LClassClass * GetClassClass( void ) const ;
		LClass * GetNamespaceClass( void ) const ;
		LGenericObjClass * GetObjectClass( void ) const ;
		LNativeObjClass * GetNativeObjClass( void ) const ;
		LStructureClass * GetStructureClass( void ) const ;
		LDataArrayClass * GetDataArrayClass( void ) const ;
		LPointerClass * GetPointerClass( void ) const ;
		LIntegerClass * GetIntegerObjClass( void ) const ;
		LDoubleClass * GetDoubleObjClass( void ) const ;
		LStringClass * GetStringClass( void ) const ;
		LStringBufClass * GetStringBufClass( void ) const ;
		LArrayClass * GetArrayClass( void ) const ;
		LMapClass * GetMapClass( void ) const ;
		LFunctionClass * GetFunctionClass( void ) const ;
		LExceptionClass * GetExceptionClass( void ) const ;
		LTaskClass * GetTaskClass( void ) const ;
		LThreadClass * GetThreadClass( void ) const ;
		// クラス取得（大域空間のみ）
		LClass * GetGlobalClassAs( const wchar_t * pwszName ) const ;
		// クラス取得（. 記号で階層化された名前）
		LClass * GetClassPathAs( const wchar_t * pwszName ) const ;
		// 配列ジェネリック型取得
		LClass * GetArrayClassAs( LClass * pElementType ) ;
		// 辞書配列ジェネリック型取得
		LClass * GetMapClassAs( LClass * pElementType ) ;
		// データ配列型取得
		LClass * GetDataArrayClassAs
			( const LType& typeData, const std::vector<size_t>& dimArray ) ;
		// ポインタ型取得
		LClass * GetPointerClassAs( const LType& typeData ) ;
		// 例外型取得
		LClass * GetExceptioinClassAs( const wchar_t * pwszClassName ) ;
		// 関数型取得
		LClass * GetFunctionClassAs
			( std::shared_ptr<LPrototype> pProto,
				LFunctionClass * pProtoClass = nullptr ) ;
		// タスク型取得
		LClass * GetTaskClassAs( const LType& typeRet ) ;
		// スレッド型取得
		LClass * GetThreadClassAs( const LType& typeRet ) ;

	public:
		// タスクオブジェクト生成
		LPtr<LTaskObj> new_Task( void ) ;
		LPtr<LTaskObj> new_Task( const LType& typeRet ) ;
		// スレッドオブジェクト生成
		LPtr<LThreadObj> new_Thread( void ) ;
		LPtr<LThreadObj> new_Thread( const LType& typeRet ) ;

	public:
		// 式を関数としてコンパイルする
		// ※より詳細なコンパイルには
		//   LCompiler::CompileExpressionAsFunc や
		//   LCompiler::CompileStatementsAsFunc を利用する
		LPtr<LFunctionObj>
			CompileAsFunc
				( const wchar_t * pwszExpr,
					LString* pErrMsg, const LType& typeRet ) ;

	public:
		// スレッドをリストに追加
		void AttachThread( LThreadObj * pThread ) ;
		// スレッドをリストに追加
		void DetachThread( LThreadObj * pThread ) ;
		// スレッドリストを取得
		void GetThreadList( std::vector< LPtr<LThreadObj> >& listThreads ) ;
		// 実行中のスレッドを強制終了する
		void TerminateAllThreads( void ) ;

	public:
		// デバッガー設定
		void AttachDebugger( LDebugger * pDebugger ) ;
		// デバッガー取得
		LDebugger * GetDebugger( void ) const ;

	protected:
		std::map< std::wstring,
					std::vector< LPtr<LFunctionObj> > >	m_mapSolvedFuncs ;
		std::map< std::wstring, PFN_NativeProc >		m_mapNativeFuncs ;

		static const NativeFuncDesc *	s_pnfdFirstDesc ;

	public:
		// ネイティブ関数定義追加
		void AddNativeFuncDefinitions( void ) ;
		void AddNativeFuncDefinitions( const NativeFuncDesc * pFuncDescs ) ;
		void AddNativeFuncDefinitions( const NativeFuncDeclList * pFuncDeclList ) ;
		void AddNativeFuncDefinition
			( const wchar_t * pwszFullFuncName, PFN_NativeProc pfnNativeProc ) ;

		// ネイティブ関数の実装解決
		void SolveNativeFunction( LPtr<LFunctionObj> pFunc ) ;

		// IMPL_LOQUATY_FUNC マクロで定義した関数名を実際の関数名に変換する
		// ※ '_' を '.' に置き換え。
		//    但し先頭が '_' の場合は '__' を '.' に置き換え '_' はそのまま
		static LString GetActualFunctionName( const wchar_t * pwszFuncName ) ;

	public:
		// s_pnfdFirstDesc チェーンに pnfdDesc を追加（挿入）
		static const NativeFuncDesc *
				AddNativeFuncDesc( const NativeFuncDesc * pnfdDesc ) ;

	public:	// LObject
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	} ;


}

#endif

