
#ifndef	__LOQUATY_FUNCTION_H__
#define	__LOQUATY_FUNCTION_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 関数の引数の型配列
	//////////////////////////////////////////////////////////////////////////

	class	LArgumentListType	: public std::vector<LType>
	{
	public:
		LArgumentListType( void ) {}
		LArgumentListType( const LArgumentListType& argList )
			: vector<LType>( argList ) {}

	public:
		// 引数型取得
		LType GetArgTypeAt( size_t index ) const ;
		// 引数型設定（配列長自動伸長）
		void SetArgTypeAt( size_t index, const LType& type ) ;

		// 引数リストの一致判定
		bool IsEqualArgmentList( const LArgumentListType& arglist ) const ;
		// 暗黙の型変換可能な引数リスト判定
		bool DoesMatchArgmentListWith( const LArgumentListType& arglist ) const ;

		// 引数型リスト文字列化
		LString ToString( void ) const ;
	} ;


	//////////////////////////////////////////////////////////////////////////
	// 関数の引数の名前と型配列
	//////////////////////////////////////////////////////////////////////////

	class	LNamedArgumentListType	: public LArgumentListType
	{
	protected:
		std::vector< std::shared_ptr<LString> >	m_argNames ;

	public:
		LNamedArgumentListType( void ) {}
		LNamedArgumentListType( const LNamedArgumentListType& argList )
			: LArgumentListType( argList ),
				m_argNames( argList.m_argNames ) {}

		const LNamedArgumentListType& operator = ( const LNamedArgumentListType& argList )
		{
			LArgumentListType::operator = ( argList ) ;
			m_argNames = argList.m_argNames ;
			return	*this ;
		}

	public:
		// 引数名取得（省略されている場合は nullptr）
		const wchar_t * GetArgNameAt( size_t index ) const ;
		// 引数名設定（配列長自動伸長）
		void SetArgNameAt( size_t index, const wchar_t * pwszName ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 関数プロトタイプ
	//////////////////////////////////////////////////////////////////////////

	class	LPrototype	: public Object
	{
	protected:
		LClass *				m_pThisClass ;
		LType::Modifiers		m_modifiers ;
		LType					m_typeReturn ;
		LNamedArgumentListType	m_argListType ;
		std::vector<LValue>		m_argDefault ;

		LNamedArgumentListType	m_refObjTypes ;	// LFunctionObj::m_refObjects の型情報

		std::shared_ptr
				<LCodeBuffer>	m_codeBuf ;		// function 式の場合プロトタイプが
		size_t					m_iCodePoint ;	// コードを所有する

		LType::LComment *		m_pComment ;

	public:
		LPrototype( LClass * pThisClass = nullptr,
					LType::Modifiers modifiers = LType::modifierDefault )
			: m_pThisClass( pThisClass ),
				m_modifiers( modifiers ),
				m_iCodePoint( 0 ), m_pComment( nullptr ) {}
		LPrototype( const LPrototype& proto )
			: m_pThisClass( proto.m_pThisClass ),
				m_modifiers( proto.m_modifiers ),
				m_typeReturn( proto.m_typeReturn ),
				m_argListType( proto.m_argListType ),
				m_argDefault( proto.m_argDefault ),
				m_refObjTypes( proto.m_refObjTypes ),
				m_codeBuf( proto.m_codeBuf ),
				m_iCodePoint( proto.m_iCodePoint ),
				m_pComment( proto.m_pComment ) {}

		const LPrototype& operator = ( const LPrototype& proto )
		{
			m_pThisClass = proto.m_pThisClass ;
			m_modifiers = proto.m_modifiers ;
			m_typeReturn = proto.m_typeReturn ;
			m_argListType = proto.m_argListType ;
			m_argDefault = proto.m_argDefault ;
			m_refObjTypes = proto.m_refObjTypes ;
			m_codeBuf = proto.m_codeBuf ;
			m_iCodePoint = proto.m_iCodePoint ;
			m_pComment = proto.m_pComment ;
			return	*this ;
		}

	public:
		// クラス
		LClass * GetThisClass( void ) const
		{
			return	m_pThisClass ;
		}
		bool IsConstThis( void ) const
		{
			return	(m_modifiers & LType::modifierConst) != 0 ;
		}
		void SetThisClass( LClass * pThisClass, bool constThis = false )
		{
			m_pThisClass = pThisClass ;
			m_modifiers = (m_modifiers & ~LType::modifierConst)
							| (constThis ? LType::modifierConst : 0) ;
		}
		// アクセス修飾子
		LType::Modifiers GetModifiers( void ) const
		{
			return	m_modifiers ;
		}
		void SetModifiers( LType::Modifiers mod )
		{
			m_modifiers = mod ;
		}
		bool IsEnableAccess( LType::Modifiers accMod ) const
		{
			return	((m_modifiers & LType::accessMask) <= (accMod & LType::accessMask)) ;
		}
		// private を不可視に設定する
		void MakePrivateInvisible( void )
		{
			if ( (m_modifiers & LType::accessMask) == LType::modifierPrivate )
			{
				MakeInvisible() ;
			}
		}
		void MakeInvisible( void )
		{
			m_modifiers = (m_modifiers & ~LType::accessMask)
								| LType::modifierPrivateInvisible ;
		}
		// 返り値
		const LType& GetReturnType( void ) const
		{
			return	m_typeReturn ;
		}
		LType& ReturnType( void )
		{
			return	m_typeReturn ;
		}
		// 引数型
		const LNamedArgumentListType& GetArgListType( void ) const
		{
			return	m_argListType ;
		}
		LNamedArgumentListType& ArgListType( void )
		{
			return	m_argListType ;
		}
		// 引数デフォルト値
		const std::vector<LValue>& GetDefaultArgList( void ) const
		{
			return	m_argDefault ;
		}
		LValue GetDefaultArgAt( size_t iArg ) const
		{
			return	(iArg < m_argDefault.size()) ? m_argDefault.at(iArg) : LValue() ;
		}
		std::vector<LValue>& DefaultArgList( void )
		{
			return	m_argDefault ;
		}
		// コメント
		LType::LComment * GetComment( void ) const
		{
			return	m_pComment ;
		}
		void SetComment( LType::LComment * pComment )
		{
			m_pComment = pComment ;
		}

	public:
		// プロトタイプの一致判定
		bool IsEqualPrototype( const LPrototype& proto ) const
		{
			return	m_typeReturn.IsEqual( proto.m_typeReturn )
					&& (IsConstThis() == proto.IsConstThis())
					&& IsEqualArgmentList( proto.m_argListType ) ;
		}
		// 引数一致判定
		bool IsEqualArgmentList( const LArgumentListType& arglist ) const
		{
			return	m_argListType.IsEqualArgmentList( arglist ) ;
		}
		// 暗黙の型変換可能な引数リスト判定
		bool DoesMatchArgmentListWith( const LArgumentListType& arglist ) const ;

		// プロトタイプ文字列化
		LString TypeToString( void ) const ;

	public:
		// 関数キャプチャー引数型情報を追加
		void AddCaptureObjectType
			( const wchar_t * pwszName, const LType& type ) ;
		// 関数参照キャプチャー引数数取得
		size_t GetCaptureObjectCount( void ) const
		{
			return	m_refObjTypes.size() ;
		}
		// 関数参照キャプチャー引数リスト型情報
		const LNamedArgumentListType& GetCaptureObjListTypes( void ) const
		{
			return	m_refObjTypes ;
		}
		// 関数実装コード
		void SetFuncCode( std::shared_ptr<LCodeBuffer> buf, size_t pos )
		{
			m_codeBuf = buf ;
			m_iCodePoint = pos ;
		}
		std::shared_ptr<LCodeBuffer> GetCodeBuffer( void ) const
		{
			return	m_codeBuf ;
		}
		size_t GetCodePoint( void ) const
		{
			return	m_iCodePoint ;
		}

	} ;



	//////////////////////////////////////////////////////////////////////////
	// 関数オブジェクト
	//////////////////////////////////////////////////////////////////////////

	class	LFunctionObj	: public LObject
	{
	protected:
		LString							m_name ;
		size_t							m_iVariation ;
		LNamespace *					m_namespace ;
		std::shared_ptr<LPrototype>		m_prototype ;
		std::shared_ptr<LCodeBuffer>	m_codeBuf ;
		size_t							m_iCodePoint ;
		std::function<void(LContext&)>	m_funcNative ;
		bool							m_callableInConstExpr ;
		std::vector<LValue>				m_refObjects ;

	public:
		LFunctionObj
			( LClass * pClass,
				std::shared_ptr<LPrototype> proto = nullptr )
			: LObject( pClass ),
				m_iVariation( 0 ),
				m_namespace( nullptr ), m_prototype( proto ),
				m_iCodePoint( 0 ), m_callableInConstExpr( false )
		{
			if ( (proto != nullptr) && (proto->GetCodeBuffer() != nullptr) )
			{
				SetFuncCode( proto->GetCodeBuffer(), proto->GetCodePoint() ) ;
			}
		}
		LFunctionObj( const LFunctionObj& func )
			: LObject( func ),
				m_name( func.m_name ),
				m_iVariation( func.m_iVariation ),
				m_namespace( func.m_namespace ),
				m_prototype( func.m_prototype ),
				m_codeBuf( func.m_codeBuf ),
				m_iCodePoint( func.m_iCodePoint ),
				m_funcNative( func.m_funcNative ),
				m_callableInConstExpr( func.m_callableInConstExpr ),
				m_refObjects( func.m_refObjects ) {}

	public:
		// 複製する（要素も全て複製処理する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		// （返り値は呼び出し側の責任で ReleaseRef）
		virtual LObject * DuplicateObject( void ) const ;
		// 内部リソースを解放する
		virtual void DisposeObject( void ) ;

	public:
		// 関数名
		const LString& GetName( void ) const
		{
			return	m_name ;
		}
		LString GetFullName( void ) const ;
		void SetName( const wchar_t * pwszName )
		{
			m_name = pwszName ;
		}
		// 関数名（表記用）
		LString GetPrintName( void ) const ;
		LString GetFullPrintName( void ) const ;

		// 定義された名前空間
		LNamespace * GetParentNamespace( void ) const
		{
			return	m_namespace ;
		}
		void SetParentNamespace( LNamespace * pParent )
		{
			m_namespace = pParent ;
		}

		// 関数プロトタイプ
		void SetPrototype( std::shared_ptr<LPrototype> proto )
		{
			m_prototype = proto ;
		}
		std::shared_ptr<LPrototype> GetPrototype( void ) const
		{
			return	m_prototype ;
		}
		LClass * GetThisClass( void ) const
		{
			return	m_prototype->GetThisClass() ;
		}
		bool IsConstThis( void ) const
		{
			return	m_prototype->IsConstThis() ;
		}
		bool IsEnableAccess( LType::Modifiers accMod ) const
		{
			return	m_prototype->IsEnableAccess( accMod ) ;
		}
		const LType& GetReturnType( void ) const
		{
			return	m_prototype->GetReturnType() ;
		}
		const LArgumentListType& GetArgListType( void ) const
		{
			return	m_prototype->GetArgListType() ;
		}
		// private を不可視に設定する
		void MakePrivateInvisible( void ) ;
		// 無条件に不可視に設定する
		void MakeInvisible( void ) ;
		// プロトタイプの一致判定
		bool IsEqualPrototype( const LPrototype& proto ) const
		{
			return	m_prototype->IsEqualPrototype( proto ) ;
		}
		// 引数一致判定
		bool IsEqualArgmentList( const LArgumentListType& arglist ) const
		{
			return	m_prototype->IsEqualArgmentList( arglist ) ;
		}
		// 暗黙の型変換可能な引数リスト判定
		bool DoesMatchArgmentListWith( const LArgumentListType& arglist ) const
		{
			return	m_prototype->DoesMatchArgmentListWith( arglist ) ;
		}

	public:
		// 関数実装コード
		void SetFuncCode( std::shared_ptr<LCodeBuffer> buf, size_t pos )
		{
			m_codeBuf = buf ;
			m_iCodePoint = pos ;
		}
		std::shared_ptr<LCodeBuffer> GetCodeBuffer( void ) const
		{
			return	m_codeBuf ;
		}
		size_t GetCodePoint( void ) const
		{
			return	m_iCodePoint ;
		}

		// ネイティブ関数
		void SetNative( std::function<void(LContext&)> func, bool callableInConstExpr = false )
		{
			m_funcNative = func ;
			m_callableInConstExpr = callableInConstExpr ;
		}
		bool IsNativeFunc( void ) const
		{
			return	(m_funcNative != nullptr) ;
		}
		bool IsCallableInConstExpr( void ) const
		{
			return	m_callableInConstExpr ;
		}
		void CallNativeFunc( LContext& context )
		{
			m_funcNative( context ) ;
		}

		// 実装されているか？
		bool IsImplemented( void ) const
		{
			return	IsNativeFunc() || (m_codeBuf != nullptr) ;
		}

		// 同名関数のインデックス
		size_t GetVariationIndex( void ) const
		{
			return	m_iVariation ;
		}
		void SetVariationIndex( size_t iVar )
		{
			m_iVariation = iVar ;
		}

	public:
		// 関数キャプチャー引数を追加
		void SetCaptureObjectList( const std::vector<LValue>& refObjs ) ;
		// 関数キャプチャー引数の数を取得
		size_t GetCaptureObjectCount( void ) const ;
		// 関数キャプチャー引数を取得
		LValue GetCaptureObjectAt( size_t index ) const ;

	} ;



	//////////////////////////////////////////////////////////////////////////
	// （同名関数の）異なるプロトタイプ関数の集合
	//////////////////////////////////////////////////////////////////////////

	class	LFunctionVariation	: public std::vector< LPtr<LFunctionObj> >
	{
	public:
		LFunctionVariation( void ) {}
		LFunctionVariation( const LFunctionVariation& funcvar )
			: std::vector< LPtr<LFunctionObj> >( funcvar ) {}

	public:
		// 関数追加、又はオーバーロード
		// （返り値はオーバーロードされた以前の関数）
		LPtr<LFunctionObj> OverloadFunction( LFunctionObj * pFunc ) ;

		// 適合関数取得
		// （※返却されたポインタを ReleaseRef してはならない）
		LPtr<LFunctionObj> GetCallableFunction
			( const LArgumentListType& argListType ) const ;

		// 関数名取得
		LString GetFunctionName( void ) const ;
	} ;



	//////////////////////////////////////////////////////////////////////////
	// 仮想関数ベクタ
	//////////////////////////////////////////////////////////////////////////

	class	LVirtualFuncVector	: public std::vector< LPtr<LFunctionObj> >
	{
	private:
		std::map< std::wstring, std::vector<size_t> >	m_mapIndexByName ;

	public:
		LVirtualFuncVector( void ) {}
		LVirtualFuncVector( const LVirtualFuncVector& vfv )
			: std::vector< LPtr<LFunctionObj> >( vfv ),
				m_mapIndexByName( vfv.m_mapIndexByName ) {}

	public:
		// 関数追加（追加された関数インデックスを返す）
		// （※ pFunc は AddRef されたポインタ）
		size_t AddFunction( const wchar_t * pwszName, LPtr<LFunctionObj> pFunc ) ;

		// 関数追加、またはオーバーライド
		// （返り値は追加された関数インデックスと、オーバーライドされた以前の関数）
		std::tuple< size_t, LPtr<LFunctionObj> >
			OverrideFunction
				( const wchar_t * pwszName,
					LPtr<LFunctionObj> pFunc,
					const LPrototype * pAsProto = nullptr,
					bool mustBeOverride = false ) ;

		// 関数検索
		const std::vector<size_t> * FindFunction( const wchar_t * pwszName ) const ;
		ssize_t FindFunction
			( const wchar_t * pwszName,
				const LArgumentListType& argListType, LClass * pThisClass,
				LType::AccessModifier accScope = LType::modifierPublic ) const ;
		ssize_t FindCallableFunction
			( const wchar_t * pwszName,
				const LArgumentListType& argListType, LClass * pThisClass,
				LType::AccessModifier accScope = LType::modifierPublic ) const ;
		ssize_t FindCallableFunction
			( const std::vector<size_t> * pFuncs,
				const LArgumentListType& argListType, LClass * pThisClass,
				LType::AccessModifier accScope = LType::modifierPublic ) const ;
		// private 関数を不可視関数に変更する
		void MakePrivateInvisible( void ) ;

		// 関数取得
		LPtr<LFunctionObj> GetFunctionAt( size_t index ) const ;

		// 関数候補リストを関数リストへ変換
		std::shared_ptr<LFunctionVariation>
			MakeFuncVariationOf( const std::vector<size_t> * pVirtFuncs ) const ;

		// 関数名リストを取得
		std::shared_ptr< std::vector<LString> > GetFuncNameList( void ) const ;

		// 関数名取得
		LString GetFunctionNameOf( const std::vector<size_t> * pFuncs ) const ;

	} ;



	//////////////////////////////////////////////////////////////////////////
	// 関数の引数を受け取り用ランタイム・ヘルパー
	//////////////////////////////////////////////////////////////////////////

	class	LRuntimeArgList
	{
	private:
		LContext&				m_context ;
		size_t					m_next ;
		std::vector<LObjPtr>	m_objPool ;	// String, NativeObject, ポインタを保持

	public:
		LRuntimeArgList( LContext& context )
			: m_context( context ), m_next( 0 ) {}

		LStackBuffer::Word NextPrimitive( void ) ;
		LBoolean NextBoolean( void ) ;
		LInt NextInt( void ) ;
		LLong NextLong( void ) ;
		LFloat NextFloat( void ) ;
		LDouble NextDouble( void ) ;
		LString NextString( void ) ;
		LObjPtr NextObject( void ) ;
		std::shared_ptr<Object> NextNativeObject( void ) ;
		std::uint8_t * NextPointer( size_t nBytes ) ;
	
		template <class T> T * NextObjectAs( void )
		{
			LObjPtr	pObj = NextObject() ;
			if ( pObj != nullptr )
			{
				T *	pTObj = dynamic_cast<T*>( pObj.Ptr() ) ;
				LObject::AddRef( pTObj ) ;
				return	pTObj ;
			}
			return	nullptr ;
		}

		void SetNativeObject( size_t index, std::shared_ptr<Object> pObj ) ;
	} ;



	//////////////////////////////////////////////////////////////////////////
	// ネイティブ関数定義リスト
	//////////////////////////////////////////////////////////////////////////

	// ネイティブ関数
	typedef	void (*PFN_NativeProc)( LContext& _context ) ;

	// ネイティブ関数簡易実装用
	struct	NativeFuncDesc
	{
		const wchar_t *			pwszFuncName ;		// クラス名 _ メンバ名 ...
		PFN_NativeProc			pfnNativeProc ;		// 関数
		const NativeFuncDesc *	pnfdNext ;			// 次の NativeFuncDesc
	} ;

	struct	NativeFuncDeclList
	{
		const NativeFuncDesc *		pnfdDesc ;
		const NativeFuncDeclList *	pNext ;

		NativeFuncDeclList
			( const NativeFuncDesc * pnfd, const NativeFuncDeclList*& pFirst )
		{
			pnfdDesc = pnfd ;
			pNext = pFirst ;
			pFirst = this ;
		}
	} ;

}

static const Loquaty::NativeFuncDeclList*	s_pnfdlFirst = nullptr ;

// ネイティブ関数簡易宣言
#define	DECL_LOQUATY_FUNC(func_name)	\
	extern const Loquaty::NativeFuncDesc	s_loquaty_desc_##func_name ;	\
	static const NativeFuncDeclList	s_loquaty_decl_##func_name( &s_loquaty_desc_##func_name, s_pnfdlFirst ) ;	\
	void loquaty_func_##func_name( Loquaty::LContext& _context )

#define	DECL_LOQUATY_CONSTRUCTOR(class_name)	DECL_LOQUATY_FUNC(class_name)
#define	DECL_LOQUATY_CONSTRUCTOR_N(class_name,n)	DECL_LOQUATY_FUNC(class_name##_##n)

// 宣言されたネイティブ関数を実装する
#define	DEF_LOQUATY_FUNC_LIST(vm)	(vm)->AddNativeFuncDefinitions(s_pnfdlFirst)

// ネイティブ関数簡易実装
// func_name_str は Loquaty での名前
// 先頭が '_' から始まる場合には、先頭の1文字を取り除いたのち、'__' を '.' に置き換える
#define	IMPL_LOQUATY_FUNC_NAME(func_name,func_name_str)	\
	const Loquaty::NativeFuncDesc	s_loquaty_desc_##func_name =	\
	{	\
		func_name_str,	\
		&loquaty_func_##func_name,	\
		Loquaty::LVirtualMachine::AddNativeFuncDesc( &s_loquaty_desc_##func_name ),	\
	} ;	\
	void loquaty_func_##func_name( Loquaty::LContext& _context )

// ネイティブ関数簡易実装
// func_name は Loquaty 上のフルパスの '.' を '_' に置き換えたもの
// クラス名や関数名に '_' を含む場合には先頭を '_' にし、'.' は '__' にする
#define	IMPL_LOQUATY_FUNC(func_name)	\
			IMPL_LOQUATY_FUNC_NAME(func_name,L###func_name)

#define	IMPL_LOQUATY_CONSTRUCTOR(class_name)	\
			IMPL_LOQUATY_FUNC_NAME(class_name,L###class_name L"_<init>")
#define	IMPL_LOQUATY_CONSTRUCTOR_N(class_name,n)	\
			IMPL_LOQUATY_FUNC_NAME(class_name##_##n,L###class_name L"_<init>_" L###n)

// 引数を受け取るには関数の先頭で LQT_FUNC_ARG_LIST; を記述し、
// 続けてメンバ関数の場合には LQT_FUNC_THIS_{OBJ|NOBJ}(); マクロを
// 更に LQT_FUNC_ARG_xxx(); マクロを引数の数だけ記述する。
// 全ての引数を受け取らないとオブジェクトの解放漏れが発生するので注意！

#define	LQT_FUNC_ARG_LIST	Loquaty::LRuntimeArgList	_arglist( _context )
#define	LQT_ARG_OBJECT(i)	LObjPtr( LObject::AddRef( _context.GetArgAt(i).pObject ) )
#define	LQT_ARG_LONG(i)		(_context.GetArgAt(i).longValue)
#define	LQT_ARG_DOUBLE(i)	(_context.GetArgAt(i).dblValue)
#define	LQT_GET_CLASS(cls)	_context.VM().GetClassPathAs( L###cls )

#define	LQT_VERIFY_NULL_PTR(obj)	\
		if ( (obj) == nullptr ) \
			return _context.ThrowException( exceptionNullPointer )

#define	LQT_FUNC_THIS_OBJ(type,name)	\
		Loquaty::LPtr<type>	name = _arglist.NextObjectAs<type>() ;	\
		assert( name != nullptr ) ;	\
		LQT_VERIFY_NULL_PTR(name)

#define	LQT_FUNC_THIS_POINTER(type,name)	\
		type*	name = (type*) _arglist.NextPointer( sizeof(type) ) ;	\
		assert( name != nullptr ) ;	\
		LQT_VERIFY_NULL_PTR(name)

#define	LQT_FUNC_THIS_NOBJ(type,name)	\
		std::shared_ptr<type>	name = std::dynamic_pointer_cast<type>( _arglist.NextNativeObject() ) ;	\
		assert( name != nullptr ) ;	\
		LQT_VERIFY_NULL_PTR(name)

#define	LQT_FUNC_THIS_INIT_NOBJ(type,name,init_new_arg)	\
		_arglist.SetNativeObject( 0, std::make_shared<type> init_new_arg ) ;	\
		LQT_FUNC_THIS_NOBJ(type,name)

#define	LQT_FUNC_ARG_INT(name)	\
		Loquaty::LInt		name = _arglist.NextInt()

#define	LQT_FUNC_ARG_UINT(name)	\
		Loquaty::LUint		name = (LUint) _arglist.NextInt()

#define	LQT_FUNC_ARG_LONG(name)	\
		Loquaty::LLong		name = _arglist.NextLong()

#define	LQT_FUNC_ARG_ULONG(name)	\
		Loquaty::LUlong		name = (LUlong) _arglist.NextLong()

#define	LQT_FUNC_ARG_BOOL(name)	\
		Loquaty::LBoolean	name = _arglist.NextBoolean()

#define	LQT_FUNC_ARG_FLOAT(name)	\
		Loquaty::LFloat		name = _arglist.NextFloat()

#define	LQT_FUNC_ARG_DOUBLE(name)	\
		Loquaty::LDouble	name = _arglist.NextDouble()

#define	LQT_FUNC_ARG_STRING(name)	\
		Loquaty::LString	name = _arglist.NextString()

#define	LQT_FUNC_ARG_OBJECT(type,name)	\
		Loquaty::LPtr<type>	name = _arglist.NextObjectAs<type>()

#define	LQT_FUNC_ARG_NOBJ(type,name)	\
		std::shared_ptr<type>	name = std::dynamic_pointer_cast<type>( _arglist.NextNativeObject() )

#define	LQT_FUNC_ARG_POINTER(type,name)	\
		type*	name = (type*) _arglist.NextPointer( sizeof(type) )

#define	LQT_FUNC_ARG_POINTER_N(type,name,count)	\
		type*	name = (type*) _arglist.NextPointer( sizeof(type) * (size_t)(count) )

#define	LQT_FUNC_ARG_STRUCT(type,name)	LQT_FUNC_ARG_POINTER(type,name)

#define	LQT_FUNC_ARG_STRUCT_N(type,name,count)	LQT_FUNC_ARG_POINTER_N(type,name,count)


// 関数の返り値は LQT_RETURN_xxx(); マクロで記述する。

#define	LQT_RETURN_INT(expr)	_context.SetReturnValue( LValue(LType::typeInt64, LValue::MakeLong(expr)) ) ; return

#define	LQT_RETURN_UINT(expr)	_context.SetReturnValue( LValue(LType::typeInt64, LValue::MakeLong((LUint)(expr))) ) ; return

#define	LQT_RETURN_LONG(expr)	_context.SetReturnValue( LValue(LType::typeInt64, LValue::MakeLong(expr)) ) ; return

#define	LQT_RETURN_ULONG(expr)	_context.SetReturnValue( LValue(LType::typeInt64, LValue::MakeLong((LLong)(expr))) ) ; return

#define	LQT_RETURN_FLOAT(expr)	_context.SetReturnValue( LValue(LType::typeDouble, LValue::MakeDouble(expr)) ) ; return

#define	LQT_RETURN_DOUBLE(expr)	_context.SetReturnValue( LValue(LType::typeDouble, LValue::MakeDouble(expr)) ) ; return

#define	LQT_RETURN_BOOL(expr)	_context.SetReturnValue( LValue(LType::typeBoolean, LValue::MakeBool(expr)) ) ; return

#define	LQT_RETURN_OBJECT(expr)	_context.SetReturnValue( LValue(LObjPtr(expr)) ) ; return

#define	LQT_RETURN_STRING(expr)	_context.SetReturnValue( LValue(LObjPtr(_context.new_String(expr))) ) ; return

#define	LQT_RETURN_POINTER(expr,bytes)	_context.SetReturnValue( LValue(LObjPtr(_context.new_Pointer(expr,bytes))) ) ; return
#define	LQT_RETURN_POINTER_STRUCT(expr)	_context.SetReturnValue( LValue(LObjPtr(_context.new_Pointer(&(expr),sizeof(expr)))) ) ; return
#define	LQT_RETURN_POINTER_BUF(buf)	_context.SetReturnValue( LValue(LObjPtr(_context.new_Pointer(buf))) ) ; return

#define	LQT_RETURN_VOID()		return


/* for example;
	// in Loquaty source;
	class	ExampleClass	extends NativeObject
	{
		public native ExampleClass( void ) ;
		public native String exampleFunc( String str, int count ) ;
	} ;

	// in C++ source;
	class	ExampleClass	: public Loquaty::Object
	{
	public:
		int	m_count ;
	public:
		ExampleClass( void ) : m_count(0) { }
	} ;

	DECL_LOQUATY_CONSTRUCTOR(ExampleClass) ;
	DECL_LOQUATY_FUNC(ExampleClass_exampleFunc) ;

	IMPL_LOQUATY_CONSTRUCTOR(ExampleClass)
	{
		LQT_FUNC_ARG_LIST ;
		LQT_FUNC_THIS_INIT_NOBJ(ExampleClass,pThis,())

		LQT_RETURN_VOID() ;
	}

	IMPL_LOQUATY_FUNC(ExampleClass_exampleFunc)
	{
		LQT_FUNC_ARG_LIST ;
		LQT_FUNC_THIS_NOBJ(ExampleClass,pThis) ;
		LQT_FUNC_ARG_STRING(str) ;
		LQT_FUNC_ARG_INT(count) ;

		pThis->m_count += count ;

		LString	strCount ;
		str += LIntegerObj::ToString( strCount, pThis->m_count ) ;

		LQT_RETURN_STRING(str) ;
	}
*/

#endif

