
#ifndef	__LOQUATY_NAMESPACE_H__
#define	__LOQUATY_NAMESPACE_H__	1

namespace	Loquaty
{
	class	LPackage ;

	//////////////////////////////////////////////////////////////////////////
	// 名前空間
	//////////////////////////////////////////////////////////////////////////

	class	LNamespace	: public LGenericObj
	{
	public:
		// ジェネリック型定義情報
		class	GenericDef
		{
		public:
			LPtr<LNamespace>			m_parent ;		// 親空間
			std::vector<LString>		m_params ;		// ジェネリック引数名
			LString						m_name ;		// ジェネリック名
			Symbol::ReservedWordIndex	m_kind ;		// class, struct, namespace
			LSourceFilePtr				m_source ;		// ソースファイル
			size_t						m_srcFirst ;	// ソースファイル参照位置
			size_t						m_srcEnd ;

		public:
			GenericDef( void ) ;
			GenericDef( const GenericDef& gendef ) ;

			// インスタンス化する
			LPtr<LNamespace> Instantiate
				( LCompiler& compiler, const std::vector<LType>& arg ) const ;
		} ;

	protected:
		LVirtualMachine&	m_vm ;			// 仮想マシン
		LPtr<LNamespace>	m_parent ;		// より大域の名前空間へのチェーン
		LString				m_name ;		// 空間名
		LPackage *			m_pPackage ;		// 含まれているパッケージ

		LArrangementBuffer	m_arrangement ;	// 静的なメンバ変数配置

		std::map<std::wstring,LFunctionVariation>
							m_funcs ;		// 静的なローカル関数

		std::map< std::wstring, LPtr<LNamespace> >
							m_namespaces ;	// サブ名前空間

		std::map<std::wstring,LType>
							m_typedefs ;	// 型名定義

		std::map<std::wstring,GenericDef>
							m_generics ;	// ジェネリック型定義

		std::vector< std::shared_ptr<LType::LComment> >
							m_comments ;	// コメントデータの所有権を保持
		std::shared_ptr<LType::LComment>
							m_selfComment ;	// これ自身のコメント

	public:
		LNamespace
			( LVirtualMachine& vm,
				LPtr<LNamespace> pParent,
				LClass * pClass, const wchar_t * pwszName = nullptr ) ;
		LNamespace( const LNamespace& ns ) ;

	public:
		// 仮想マシン取得
		LVirtualMachine& VM( void ) const
		{
			return	m_vm ;
		}
		// 名前取得
		const LString& GetName( void ) const
		{
			return	m_name ;
		}
		virtual LString GetFullName( void ) const ;

		// 親名前空間
		LPtr<LNamespace> GetParentNamespace( void ) const
		{
			return	m_parent ;
		}

		// パッケージ情報
		LPackage * GetPackage( void ) const
		{
			return	m_pPackage ;
		}
		void SetPackage( LPackage * pPackage )
		{
			m_pPackage = pPackage ;
		}

	public:
		// 文字列として評価
		virtual bool AsString( LString& str ) const ;

		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

		// 全要素の複製（複製はせず参照の複製）
		virtual void DuplicateFrom( const LNamespace& nspace ) ;

		// ローカルクラス・名前空間以外の全ての要素を解放
		virtual void DisposeAllObjects( void ) ;
		// 全ての要素を解放
		virtual void ReleaseAll( void ) ;

	public:
		// ローカル変数を定義
		virtual bool DeclareObjectAs
			( const wchar_t * pwszName, const LType& type, LObjPtr pObj ) ;
		// ローカル変数の型を取得
		virtual const LType * GetLocalObjectTypeAs( const wchar_t * pwszName ) ;
		// ローカル変数を取得
		virtual LObjPtr GetLocalObjectAs( const wchar_t * pwszName ) ;

		// ローカルメンバ変数配置情報
		LArrangementBuffer& StaticArrangement( void )
		{
			return	m_arrangement ;
		}
		const LArrangementBuffer& GetStaticArrangement( void ) const
		{
			return	m_arrangement ;
		}

	public:
		// ローカルクラスを追加
		virtual bool AddClassAs( const wchar_t * pwszName, LPtr<LClass> pClass ) ;
		// ローカルクラスを取得
		// （※返却されたポインタを ReleaseRef してはならない）
		virtual LClass * GetLocalClassAs( const wchar_t * pwszName ) ;
		// クラスを取得（親の名前空間も探す）
		// （※返却されたポインタを ReleaseRef してはならない）
		virtual LClass * GetClassAs( const wchar_t * pwszName ) ;

	public:
		// 静的なローカル関数を追加
		// （返り値はオーバーロードされた以前の関数）
		virtual LPtr<LFunctionObj> AddStaticFunctionAs
			( const wchar_t * pwszName, LPtr<LFunctionObj> pFunc ) ;
		// 静的なローカル関数群を取得
		virtual const LFunctionVariation *
					GetLocalStaticFunctionsAs( const wchar_t * pwszName ) ;
		// 静的なローカル関数を取得
		// （※返却されたポインタを ReleaseRef してはならない）
		virtual LPtr<LFunctionObj> GetLocalStaticCallableFunctionAs
			( const wchar_t * pwszName, const LArgumentListType& argListType ) ;
		// 静的なローカル関数配列
		const std::map<std::wstring,LFunctionVariation>&
						GetStaticFunctionList( void ) const ;

	public:
		// 名前空間を追加
		virtual bool AddNamespace
			( const wchar_t * pwszName, LPtr<LNamespace> pNamespace ) ;
		// 名前空間を取得
		LPtr<LNamespace> GetLocalNamespaceAs( const wchar_t * pwszName ) ;
		// 名前空間を取得（親の名前空間も探す）
		LPtr<LNamespace> GetNamespaceAs( const wchar_t * pwszName ) ;
		// 名前空間配列
		const std::map< std::wstring, LPtr<LNamespace> >&
						GetNamespaceList( void ) const ;

	public:
		// 型定義を追加
		virtual bool DefineTypeAs
			( const wchar_t * pwszName, const LType& type,
				bool flagPackage, LClass * pAliasType = nullptr ) ;
		// 定義された型を取得
		const LType * GetTypeAs( const wchar_t * pwszName ) ;

	public:
		// ジェネリック型定義を追加
		virtual bool DefineGenericType
			( const wchar_t * pwszName, const GenericDef& gendef ) ;
		// ジェネリック型情報を取得
		const LNamespace::GenericDef * GetGenericTypeAs( const wchar_t * pwszName ) ;

	public:
		// コメントデータ作成
		LType::LComment * MakeComment( const wchar_t * pwszComment ) ;

		// これ自体のコメント
		void SetSelfComment( const wchar_t * pwszComment ) ;
		LType::LComment * GetSelfComment( void ) const
		{
			return	m_selfComment.get() ;
		}

	protected:
		std::recursive_mutex	m_mutex ;

	public:
		// 同期処理 locker を取得
		std::unique_lock<std::recursive_mutex> GetLock( void )
		{
			return	std::unique_lock<std::recursive_mutex>( m_mutex ) ;
		}
	} ;



	//////////////////////////////////////////////////////////////////////////
	// 名前空間リスト
	//////////////////////////////////////////////////////////////////////////

	class	LNamespaceList	: public std::vector<LNamespace*>
	{
	public:
		LNamespaceList( void ) {}
		LNamespaceList( const LNamespaceList& nsl ) ;
		const LNamespaceList& operator = ( const LNamespaceList& nsl ) ;
		const LNamespaceList& operator += ( const LNamespaceList& nsl ) ;


		// 名前空間を取得（親の名前空間も探す）
		LPtr<LNamespace> GetNamespaceAs( const wchar_t * pwszName ) const ;
		// クラスを取得（親の名前空間も探す）
		LClass * GetClassAs( const wchar_t * pwszName ) const ;
		// 型を取得（親の名前空間も探す）
		const LType * GetTypeAs( const wchar_t * pwszName ) const ;
		// ジェネリック型情報を取得（親の名前空間も探す）
		const LNamespace::GenericDef *
					GetGenericTypeAs( const wchar_t * pwszName ) const ;

		// 名前空間追加
		void AddNamespace( LNamespace * pNamespace ) ;
		void AddNamespaceList( const LNamespaceList& list ) ;

	} ;



	//////////////////////////////////////////////////////////////////////////
	// パッケージ（主にドキュメント生成時のため）
	//////////////////////////////////////////////////////////////////////////

	class	LPackage	: public Object
	{
	public:
		enum	Type
		{
			typeSystemDefault,
			typeSourceFile,
			typeImportingModule,
			typeIncludingScript,
		} ;
		class	TypeDefDesc
		{
		public:
			LNamespace *	m_pNamespace ;
			LString			m_strName ;

			TypeDefDesc( LNamespace * pNamespace, const wchar_t * pwszName )
				: m_pNamespace( pNamespace ), m_strName( pwszName ) { }
		} ;

	protected:
		Type						m_type ;
		LString						m_name ;
		std::vector<LNamespace*>	m_classes ;
		std::vector<TypeDefDesc>	m_types ;

		static thread_local LPackage *	t_pCurrent ;

	public:
		LPackage( Type type, const wchar_t * name )
			: m_type( type ), m_name( name ) { }

		// タイプ
		Type GetType( void ) const
		{
			return	m_type ;
		}
		// 名前
		const LString& GetName( void ) const
		{
			return	m_name ;
		}
		// クラス追加
		void AddClass( LNamespace* pClass )
		{
			m_classes.push_back( pClass ) ;
		}
		// クラスリスト
		const std::vector<LNamespace*>& GetClasses( void ) const
		{
			return	m_classes ;
		}
		// 型定義追加
		void AddTypeDef
			( LNamespace * pNamespace, const wchar_t * pwszName )
		{
			m_types.push_back( TypeDefDesc( pNamespace, pwszName ) ) ;
		}
		// 型定義リスト
		const std::vector<TypeDefDesc>& GetTypes( void ) const
		{
			return	m_types ;
		}

	public:
		// 現在のスレッドに設定された LContext 取得
		static LPackage * GetCurrent( void )
		{
			return	t_pCurrent ;
		}

		class	Current
		{
		private:
			LPackage *	m_prev ;
		public:
			Current( LPackage * pPackage )
			{
				m_prev = t_pCurrent ;
				t_pCurrent = pPackage ;
			}
			~Current( void )
			{
				t_pCurrent = m_prev ;
			}
		} ;
	} ;

	typedef	std::shared_ptr<LPackage>	LPackagePtr ;

}

#endif

