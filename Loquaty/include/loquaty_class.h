
#ifndef	__LOQUATY_CLASS_H__
#define	__LOQUATY_CLASS_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Loquaty オブジェクトクラス
	//////////////////////////////////////////////////////////////////////////

	class	LClass	: public LNamespace
	{
	protected:
		bool					m_flagCompleted ;	// 定義済みか？
		bool					m_flagGenInstance ;	// ジェネリック・インスタンス

		LClass *				m_pSuperClass ;		// 継承元クラス
		std::vector<LClass*>	m_vecImplements ;

		LVirtualFuncVector		m_vfvVirtualFuncs ;	// 仮想関数ベクタ
		std::map<LClass*, LVirtualFuncVector>
								m_mapVirtualFuncs ;	// 実装仮想関数ベクタ

		LFunctionObj *			m_pFuncFinalize ;	// finalize() 関数

		LObjPtr					m_pPrototype ;		// プロトタイプ・オブジェクト
		LArrangementBuffer		m_protoBuffer ;		// プロトタイプ・バッファ
		size_t					m_nSuperBufSize ;	// 派生元クラスの合計バッファサイズ

		LClass *				m_pGenArrayType ;	// Type[] : 配列ジェネリック型
		LClass *				m_pGenMapType ;		// Map<Type> : 辞書配列ジェネリック型

		std::vector<Symbol::UnaryOperatorDef>	m_vecUnaryOperators ;
		std::vector<Symbol::BinaryOperatorDef>	m_vecBinaryOperators ;

		friend class	LStructureClass ;
		friend class	LVirtualMachine ;

	public:
		static constexpr const wchar_t *	s_Constructor = L"<init>" ;	// 構築関数名

	public:
		LClass( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LNamespace( vm, pNamespace, pClass, pwszName ),
				m_flagCompleted( false ),
				m_flagGenInstance( false ),
				m_pSuperClass( nullptr ),
				m_pFuncFinalize( nullptr ),
				m_protoBuffer( 1 ),
				m_nSuperBufSize( 0 ),
				m_pGenArrayType( nullptr ), m_pGenMapType( nullptr ) {}
		LClass( const LClass& cls )
			: LNamespace( cls ),
				m_flagCompleted( false ),
				m_flagGenInstance( false ),
				m_pSuperClass( cls.m_pSuperClass ),
				m_vecImplements( cls.m_vecImplements ),
				m_vfvVirtualFuncs( cls.m_vfvVirtualFuncs ),
				m_mapVirtualFuncs( cls.m_mapVirtualFuncs ),
				m_pFuncFinalize( cls.m_pFuncFinalize ),
				m_pPrototype( cls.m_pPrototype ),
				m_protoBuffer( cls.m_protoBuffer ),
				m_nSuperBufSize( cls.m_nSuperBufSize ),
				m_pGenArrayType( cls.m_pGenArrayType ),
				m_pGenMapType( cls.m_pGenMapType ),
				m_vecUnaryOperators( cls.m_vecUnaryOperators ),
				m_vecBinaryOperators( cls.m_vecBinaryOperators ) {}

		// クラス名取得
		const LString& GetClassName( void ) const
		{
			return	LNamespace::GetName() ;
		}
		LString GetFullClassName( void ) const
		{
			return	LNamespace::GetFullName() ;
		}
		// 構築関数表記名
		LString GetConstructerName( void ) const ;

		// ジェネリッククラスか？
		bool IsGenericInstance( void ) const
		{
			return	m_flagGenInstance ;
		}
		void SetGenericInstanceFlag( void )
		{
			m_flagGenInstance = true ;
		}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// ローカルクラス・名前空間以外の全ての要素を解放
		virtual void DisposeAllObjects( void ) ;

	public:
		// pClass へキャスト可能か？（データの変換なしのキャスト）
		//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
		enum	ResultInstanceOf
		{
			instanceUnavailable,	// false 評価
			instanceAvailable,		// true 評価
			instanceConstCast,		// false 評価
			instanceStrangerPtr,	// false 評価
		} ;
		virtual ResultInstanceOf TestInstanceOf( LClass * pClass ) const ;
		bool IsInstanceOf( LClass * pClass ) const
		{
			return	(TestInstanceOf(pClass) == instanceAvailable) ;
		}

		// 仮想関数ベクタ取得
		virtual const LVirtualFuncVector *
					GetVirtualVectorOf( LClass * pClass ) const ;
		const LVirtualFuncVector& GetVirtualVector( void ) const ;

		// オブジェクト生成（コンストラクタは後で呼び出し元責任で呼び出す）
		virtual LObject * CreateInstance( void ) ;

		// finalize() を呼び出す
		virtual void InvokeFinalizer( LObject * pObj ) ;

	public:
		// 親クラス追加
		virtual bool AddSuperClass( LClass * pClass ) ;
		// インターフェース追加
		virtual bool AddImplementClass( LClass * pClass ) ;
	protected:
		bool TestExtendingClass( LClass * pClass ) ;
		void ImplementVirtuals( LClass * pClass ) ;

	public:
		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;
		// クラス定義完成時処理
		virtual void CompleteClass( void ) ;
		// クラス定義済みか？
		bool IsClassCompleted( void ) const ;
		// 抽象クラスか？
		bool IsAbstractClass( void ) const ;

	public:
		// 親クラス取得
		LClass * GetSuperClass( void ) const ;
		const std::vector<LClass*>& GetImplementClasses( void ) const ;

	public:
		// プロトタイプオブジェクト設定
		virtual void SetPrototypeObject( LObjPtr pPrototype ) ;
		// プロトタイプオブジェクト取得
		LObjPtr GetPrototypeObject( void ) const ;

	public:
		// メンバ変数配置情報取得
		LArrangementBuffer& ProtoArrangemenet( void )
		{
			return	m_protoBuffer ;
		}
		const LArrangementBuffer& GetProtoArrangemenet( void ) const
		{
			return	m_protoBuffer ;
		}
		// 派生元のメンバ変数の配置サイズ
		//（新しく追加されるこのクラスのメンバ変数の初めの位置とは
		//  必ずしも一致しないことに注意）
		size_t GetProtoSuperArrangeBufBytes( void ) const
		{
			return	m_nSuperBufSize ;
		}
		// 配置情報からメンバ変数を追加
		static void AddMemberArrangement
			( LArrangementBuffer& arrangeDst,
				const LArrangementBuffer& arrangeSrc ) ;

	public:
		// 仮想関追加、またはオーバーライド
		// （返り値は追加された関数インデックスと、オーバーライドされた以前の関数）
		std::tuple< size_t, LPtr<LFunctionObj> >
			OverrideVirtualFunction
				( const wchar_t * pwszName,
					LPtr<LFunctionObj> pFunc, const LPrototype * pAsProto = nullptr ) ;

		// native 関数定義用
		struct	NativeFuncDesc
		{
			const wchar_t *	m_pwszFuncName ;		// nullptr の時リストの終端
			void (*m_pfnNativeFunc)( LContext& ) ;
			bool			m_callableInConstExpr ;
			const wchar_t *	m_pwszModifiers ;
			const wchar_t *	m_pwszRetType ;
			const wchar_t *	m_pwszArgList ;
			const wchar_t *	m_pwszComment ;
			const wchar_t *	m_pwszAsArgList ;		// ジェネリック派生クラスで
													// 基本クラスの関数を不可視に
													// したい場合、その引数
		} ;
		// ネイティブ仮想関数群を追加・又はオーバーライド
		void OverrideVirtuals
			( const NativeFuncDesc * pnfDescs,
				const LNamespaceList * pnslLocal = nullptr ) ;
		// ネイティブ静関数群を追加・又はオーバーライド
		void OverloadStaticFunctions
			( const NativeFuncDesc * pnfDescs,
				const LNamespaceList * pnslLocal = nullptr ) ;
		// NativeFuncDesc から LFunctionObj を生成
		LPtr<LFunctionObj> CreateFunctionObj
			( const NativeFuncDesc& nfDesc,
				LCompiler& compiler, const LNamespaceList& nsList ) ;
		std::shared_ptr<LPrototype> GetOverridePrototype
			( const NativeFuncDesc& nfDesc,
				LCompiler& compiler, const LNamespaceList& nsList ) ;

	public:
		// 変数定義用
		struct	VariableDesc
		{
			const wchar_t *	m_pwszName ;		// nullptr の時リストの終端
			const wchar_t *	m_pwszModifiers ;
			const wchar_t *	m_pwszType ;
			const wchar_t *	m_pwszInitExpr ;
			const LValue *	m_pInitValue ;
			const wchar_t *	m_pwszComment ;
		} ;
		// 変数定義を追加
		void AddVariableDefinitions
			( const VariableDesc * pvDescs,
				const LNamespaceList * pnslLocal = nullptr ) ;
		void AddVariableDefinition
			( const VariableDesc& varDesc,
				LCompiler& compiler, const LNamespaceList& nsList ) ;

	public:
		// 二項演算子簡易定義
		struct	BinaryOperatorDesc
		{
			const wchar_t *			m_pwszOperator ;		// nullptr の時リストの終端
			Symbol::PFN_OPERATOR2	m_pfnNativeFunc ;
			bool					m_evalInConstExpr ;		// 定数式で実行できるか？
			bool					m_constLeft ;			// 左辺は const か？
			const wchar_t *			m_pwszRetType ;			// 評価型
			const wchar_t *			m_pwszRightType ;		// 右辺型
			const wchar_t *			m_pwszComment ;
			const wchar_t *			m_pwszAsRightType ;		// ジェネリック型の時に
															// オーバーロードしたい右辺型
		} ;
		// 二項演算子を追加
		void AddBinaryOperatorDefinitions
			( const BinaryOperatorDesc * pboDescs,
				const LNamespaceList * pnslLocal = nullptr ) ;
		void AddBinaryOperatorDefinition
			( const BinaryOperatorDesc& boDesc,
				LCompiler& compiler, const LNamespaceList& nsList ) ;

	public:
		// 適合単項演算子取得
		virtual const Symbol::UnaryOperatorDef *
				GetMatchUnaryOperator
					( Symbol::OperatorIndex opIndex,
						LType::AccessModifier accScope = LType::modifierPublic) const ;
		static const Symbol::UnaryOperatorDef *
				FindMatchUnaryOperator
					( Symbol::OperatorIndex opIndex,
						const Symbol::UnaryOperatorDef * pList, size_t nCount,
						LType::AccessModifier accScope = LType::modifierPublic ) ;
		// 適合二単項演算子取得
		virtual const Symbol::BinaryOperatorDef *
				GetMatchBinaryOperator
					( Symbol::OperatorIndex opIndex, const LType& typeVal2,
						LType::AccessModifier accScope = LType::modifierPublic ) const ;
		static const Symbol::BinaryOperatorDef *
				FindMatchBinaryOperator
					( Symbol::OperatorIndex opIndex,
						const LClass * pThisClass, const LType& typeVal2,
						const Symbol::BinaryOperatorDef * pList, size_t nCount,
						LType::AccessModifier accScope = LType::modifierPublic ) ;
		// 二単項演算子リスト
		const std::vector<Symbol::BinaryOperatorDef>& GetBinaryOperators( void ) const
		{
			return	m_vecBinaryOperators ;
		}

	public:
		// 単項演算子定義追加
		virtual void AddUnaryOperatorDef
			( Symbol::OperatorIndex opIndex, const LType& typeRet,
				Symbol::PFN_OPERATOR1 pfnOp, void * pInstance = nullptr,
				bool constExpr = true, bool constThis = true,
				const wchar_t * pwszComment = nullptr ) ;
		void AddUnaryOperatorDef
			( const Symbol::UnaryOperatorDef & uopDef ) ;
		void AddUnaryOperatorDefList
			( const Symbol::UnaryOperatorDef * pList, size_t nCount = SIZE_MAX ) ;
		// 二項演算子定義追加
		virtual void AddBinaryOperatorDef
			( Symbol::OperatorIndex opIndex, const LType& typeRet,
				const LType& typeRight,
				Symbol::PFN_OPERATOR2 pfnOp, void * pInstance = nullptr,
				bool constExpr = true, bool constThis = true,
				const wchar_t * pwszComment = nullptr ) ;
		void AddBinaryOperatorDef
			( const Symbol::BinaryOperatorDef & bopDef, const LType& typeAsRight ) ;
		void AddBinaryOperatorDefList
			( const Symbol::BinaryOperatorDef * pList, size_t nCount= SIZE_MAX ) ;

	public:
		// クラスメンバ一括定義
		struct	ClassMemberDesc
		{
			const NativeFuncDesc *		m_pnfdVirtuals ;	// 仮想関数
			const NativeFuncDesc *		m_pnfdFunctions ;	// 静関数
			const VariableDesc *		m_pvdVariables ;	// 変数
			const BinaryOperatorDesc *	m_pbodBinaryOps ;	// 二項演算子
			const wchar_t *				m_pwszComment ;
		} ;
		void AddClassMemberDefinitions
			( const ClassMemberDesc& desc,
				const LNamespaceList * pnslLocal = nullptr ) ;

	public:
		// instanceof 演算子 : boolean(LObject*,LClass*)
		static LValue::Primitive operator_instanceof
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// == 演算子 : boolean(LObject*,LObject*)
		static LValue::Primitive operator_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// != 演算子 : boolean(LObject*,LObject*)
		static LValue::Primitive operator_not_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// sizeof 演算子関数 : long(LObject*)
		static LValue::Primitive operator_sizeof
			( LValue::Primitive val1, void * instance ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// クラス一括実装ヘルパー
	//////////////////////////////////////////////////////////////////////////

	class	LBatchClassImplementor
	{
	protected:
		LNamespace&				m_target ;
		std::vector<LClass*>	m_classes ;

	public:
		LBatchClassImplementor( LNamespace& nsTarget ) ;

		// クラス追加
		void AddClass( LPtr<LClass> pClass, LClass * pSuperClass ) ;
		// 実装のために追加
		void DeclareClass( LClass * pClass, LClass * pSuperClass ) ;
		// 一括実装
		void Implement( void ) ;
	} ;


	//////////////////////////////////////////////////////////////////////////
	// Class クラス
	//////////////////////////////////////////////////////////////////////////

	class	LClassClass	: public LClass
	{
	public:
		LClassClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName ) {}
		LClassClass( const LClassClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc	s_Virtuals[2] ;

	public:
		// String getName()
		static void method_getName( LContext& context ) ;
	} ;


	//////////////////////////////////////////////////////////////////////////
	// プリミティブ／構造体配列型
	//////////////////////////////////////////////////////////////////////////

	class	LDataArrayClass	: public LClass
	{
	public:
		LType				m_typeData ;		// 配列装飾されていない型
												//		[const] PritimiveTypes
												//		[const] Structure
		std::vector<size_t>	m_dimArray ;		// 二次元配列以上の場合

	protected:
		LClass *			m_pGenPointerType ;	// この配列へのポインタ型

		friend class	LVirtualMachine ;

	public:
		LDataArrayClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName,
				const LType& typeData, const std::vector<size_t>& dimArray ) ;
		LDataArrayClass( const LDataArrayClass& cls ) ;

	public:
		// 配列装飾されていない型
		const LType& GetDataType( void ) const ;
		// １回要素参照した型
		LType GetElementType( void ) const ;
		// 要素を参照するポインタ型
		LClass * GetElementPtrClass( void ) const ;
		// ストライド
		size_t GetDataSize( void ) const ;
		// アライメント
		size_t GetDataAlign( void ) const ;
		// 配列情報
		const std::vector<size_t>& Dimension( void ) const ;
		// （１回要素参照する際の）配列長
		size_t GetArrayElementCount( void ) const ;

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// pClass へキャスト可能か？（データの変換なしのキャスト）
		//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
		virtual ResultInstanceOf TestInstanceOf( LClass * pClass ) const ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Pointer クラス
	//////////////////////////////////////////////////////////////////////////

	class	LPointerClass	: public LClass
	{
	protected:
		LType	m_typeBuf ;		// ポインタ装飾されていない型
								//		[const] PritimiveTypes
								//		[const] Structure
								//		データ配列型

	public:
		LPointerClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName,
				const LType& typeBuf )
			: LClass( vm, vm.Global(), pClass, pwszName ),
				m_typeBuf( typeBuf )
		{
			m_pPrototype = new LPointerObj( this ) ;
		}
		LPointerClass( const LPointerClass& ptrcls )
			: LClass( ptrcls ),
				m_typeBuf( ptrcls.m_typeBuf ) {}

	public:
		// ポインタ装飾されていない型
		const LType& GetBufferType( void ) const ;
		// ポインタ装飾も配列装飾もされていない型
		LType GetDataType( void ) const ;
		// 要素ストライド
		size_t GetElementStride( void ) const ;
		// 要素アライメント
		size_t GetElementAlign( void ) const ;

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// pClass へキャスト可能か？（データの変換なしのキャスト）
		//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
		virtual ResultInstanceOf TestInstanceOf( LClass * pClass ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc	s_Virtuals[3] ;

	public:
		// ビルトイン定義演算子
		static constexpr const size_t			UnaryOffsetOperatorCount = 2 ;
		static constexpr const size_t			UnaryPtrOperatorCount = 2 ;
		static constexpr const size_t			BinaryOffsetOperatorCount = 6 ;
		static constexpr const size_t			BinaryPtrOperatorCount = 6 ;
		static const Symbol::UnaryOperatorDef	s_UnaryOffsetOperatorDefs[UnaryOffsetOperatorCount+1] ;
		static const Symbol::UnaryOperatorDef	s_UnaryPtrOperatorDefs[UnaryPtrOperatorCount+1] ;
		static const Symbol::BinaryOperatorDef	s_BinaryOffsetOperatorDefs[BinaryOffsetOperatorCount+1] ;
		static const Symbol::BinaryOperatorDef	s_BinaryPtrOperatorDefs[BinaryPtrOperatorCount+1] ;

		// 単項演算子（オフセット/アドレスに対する整数値の操作）
		// （※instance はストライド整数値）
		static LValue::Primitive operator_offset_increment
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_offset_decrement
			( LValue::Primitive val1, void * instance ) ;

		// 単項演算子（ポインタ・オブジェクトに対する操作）
		// （※instance はストライド整数値）
		static LValue::Primitive operator_ptr_increment
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_ptr_decrement
			( LValue::Primitive val1, void * instance ) ;

		// 二項演算子（オフセット/アドレスに対する整数値の操作）
		// （※instance はストライド整数値）
		static LValue::Primitive operator_offset_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_offset_sub
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_offset_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_offset_not_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

		// 二項演算子（ポインタ・オブジェクトに対する操作）
		// （※instance はストライド整数値）
		static LValue::Primitive operator_ptr_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_ptr_sub
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_ptr_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_ptr_not_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
	} ;


	//////////////////////////////////////////////////////////////////////////
	// Integer クラス
	//////////////////////////////////////////////////////////////////////////

	class	LIntegerClass	: public LClass
	{
	public:
		LIntegerClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName )
		{
			m_pPrototype = new LIntegerObj( this ) ;
		}
		LIntegerClass( const LIntegerClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc	s_Virtuals[2] ;

	public:
		// プリミティブ用ビルトイン定義演算子
		static constexpr const size_t			UnaryOperatorCount = 6 ;
		static constexpr const size_t			BinaryOperatorCount = 18 ;
		static const Symbol::UnaryOperatorDef	s_UnaryOperatorDefs[UnaryOperatorCount+1] ;
		static const Symbol::BinaryOperatorDef	s_IntOperatorDefs[BinaryOperatorCount+1] ;
		static const Symbol::BinaryOperatorDef	s_UintOperatorDefs[BinaryOperatorCount+1] ;

	public:
		// long プリミティブ用単項演算子
		static LValue::Primitive operator_unary_plus
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_unary_minus
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_increment
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_decrement
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_bit_not
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_logical_not
			( LValue::Primitive val1, void * instance ) ;

		// long プリミティブ用二項演算子
		static LValue::Primitive operator_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_sub
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_mul
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_div
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_mod
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_bit_and
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_bit_or
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_bit_xor
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_shift_right
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_shift_right_a
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_shift_left
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_not_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_less_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_less_than
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_grater_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_grater_than
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_logical_and
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_logical_or
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Double クラス
	//////////////////////////////////////////////////////////////////////////

	class	LDoubleClass	: public LClass
	{
	public:
		LDoubleClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName )
		{
			m_pPrototype = new LDoubleObj( this ) ;
		}
		LDoubleClass( const LDoubleClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc	s_Virtuals[2] ;

	public:
		// プリミティブ用ビルトイン定義演算子
		static constexpr const size_t			UnaryOperatorCount = 2 ;
		static constexpr const size_t			BinaryOperatorCount = 10 ;
		static const Symbol::UnaryOperatorDef	s_UnaryOperatorDefs[UnaryOperatorCount+1] ;
		static const Symbol::BinaryOperatorDef	s_BinaryOperatorDefs[BinaryOperatorCount+1] ;

	public:
		// double プリミティブ用単項演算子
		static LValue::Primitive operator_unary_plus
			( LValue::Primitive val1, void * instance ) ;
		static LValue::Primitive operator_unary_minus
			( LValue::Primitive val1, void * instance ) ;

		// double プリミティブ用二項演算子
		static LValue::Primitive operator_add
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_sub
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_mul
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_div
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_not_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_less_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_less_than
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_grater_equal
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive comparator_grater_than
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
	} ;


	//////////////////////////////////////////////////////////////////////////
	// String クラス
	//////////////////////////////////////////////////////////////////////////

	class	LStringClass	: public LClass
	{
	public:
		LStringClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName )
		{
			m_pPrototype = new LStringObj( this ) ;
		}
		LStringClass( const LStringClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc			s_Virtuals[22] ;
		static const NativeFuncDesc			s_Functions[3] ;
		static const BinaryOperatorDesc		s_BinaryOps[13] ;
		static const ClassMemberDesc		s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// StringBuf クラス
	//////////////////////////////////////////////////////////////////////////

	class	LStringBufClass	: public LClass
	{
	public:
		LStringBufClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName )
		{
			m_pPrototype = new LStringBufObj( this ) ;
		}
		LStringBufClass( const LStringBufClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc			s_Virtuals[7] ;
		static const BinaryOperatorDesc		s_BinaryOps[3] ;
		static const ClassMemberDesc		s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Array クラス
	//////////////////////////////////////////////////////////////////////////

	class	LArrayClass	: public LClass
	{
	protected:
		LClass *	m_pElementType ;

	public:
		LArrayClass
			( LVirtualMachine& vm, LClass * pClass,
				const wchar_t * pwszName = nullptr,
				LClass * pElementType = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName ),
				m_pElementType( pElementType )
		{
			m_pPrototype = new LArrayObj( this ) ;
		}
		LArrayClass( const LArrayClass& cls )
			: LClass( cls ), m_pElementType( cls.m_pElementType ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// pClass へキャスト可能か？（データの変換なしのキャスト）
		//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
		virtual ResultInstanceOf TestInstanceOf( LClass * pClass ) const ;

	public:
		// 要素型取得
		LClass * GetElementTypeClass( void ) const
		{
			return	m_pElementType ;
		}

	public:
		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[10] ;
		static const BinaryOperatorDesc	s_BinaryOps[5] ;
		static const ClassMemberDesc	s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Map クラス
	//////////////////////////////////////////////////////////////////////////

	class	LMapClass	: public LClass
	{
	protected:
		LClass *	m_pElementType ;

	public:
		LMapClass
			( LVirtualMachine& vm, LClass * pClass,
				const wchar_t * pwszName = nullptr,
				LClass * pElementType = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName ),
				m_pElementType( pElementType )
		{
			m_pPrototype = new LMapObj( this ) ;
		}
		LMapClass( const LMapClass& cls )
			: LClass( cls ), m_pElementType( cls.m_pElementType ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// pClass へキャスト可能か？（データの変換なしのキャスト）
		//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
		virtual ResultInstanceOf TestInstanceOf( LClass * pClass ) const ;

	public:
		// 要素型取得
		LClass * GetElementTypeClass( void ) const
		{
			return	m_pElementType ;
		}

	public:
		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[7] ;
		static const BinaryOperatorDesc	s_BinaryOps[2] ;
		static const ClassMemberDesc	s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Function クラス
	//////////////////////////////////////////////////////////////////////////

	class	LFunctionClass	: public LClass
	{
	protected:
		std::shared_ptr<LPrototype>	m_pFuncProto ;

	public:
		LFunctionClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName = nullptr,
				std::shared_ptr<LPrototype> pProto = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName ),
				m_pFuncProto( pProto )
		{
			m_pPrototype = new LFunctionObj( this, pProto ) ;
		}
		LFunctionClass( const LFunctionClass& cls )
			: LClass( cls ), m_pFuncProto( cls.m_pFuncProto ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;
		// ローカルクラス・名前空間以外の全ての要素を解放
		virtual void DisposeAllObjects( void ) ;
		// pClass へキャスト可能か？（データの変換なしのキャスト）
		//（pClass は派生元か？ あるいは配列要素が派生元の関係か？）
		virtual ResultInstanceOf TestInstanceOf( LClass * pClass ) const ;

	public:
		std::shared_ptr<LPrototype>	GetFuncPrototype( void ) const
		{
			return	m_pFuncProto ;
		}

	public:
		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	public:
		// Function オブジェクトの構築関数
		// （キャプチャーオブジェクトを引数に受け取る）
		static void method_init( LContext& context ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// ユーザー定義用 Object クラス
	//////////////////////////////////////////////////////////////////////////

	class	LGenericObjClass	: public LClass
	{
	public:
		LGenericObjClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, pNamespace, pClass, pwszName )
		{
			m_pPrototype = new LGenericObj( this ) ;
		}
		LGenericObjClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName, LObjPtr pPrototype )
			: LClass( vm, pNamespace, pClass, pwszName )
		{
			m_pPrototype = pPrototype ;
		}
		LGenericObjClass( const LGenericObjClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// 基底 Object メソッドの定義処理
		void ImpletentPureObjectClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[8] ;
		static const BinaryOperatorDesc	s_BinaryOps[3] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// boolean operator == ( Object obj )
		static LValue::Primitive operator_eq
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// boolean operator != ( Object obj )
		static LValue::Primitive operator_ne
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// NativeObject クラス
	//////////////////////////////////////////////////////////////////////////

	class	LNativeObjClass	: public LClass
	{
	public:
		LNativeObjClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, pNamespace, pClass, pwszName )
		{
			m_pPrototype = new LNativeObj( this ) ;
		}
		LNativeObjClass( const LNativeObjClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	public:
		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[2] ;
		static const BinaryOperatorDesc	s_BinaryOps[3] ;
		static const ClassMemberDesc	s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Structure クラス
	//////////////////////////////////////////////////////////////////////////

	class	LStructureClass	: public LClass
	{
	protected:
		std::map<LClass*,size_t>
						m_mapArrangement ;		// 親クラスへのオフセット

		LClass *		m_pGenPointerType ;		// Type* : ポインタ型
		LClass *		m_pGenConstPtrType ;	// const Type* : ポインタ型

		friend class	LVirtualMachine ;

	public:
		LStructureClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, pNamespace, pClass, pwszName ),
				m_pGenPointerType( nullptr ), m_pGenConstPtrType( nullptr ) {}
		LStructureClass( const LStructureClass& cls )
			: LClass( cls ),
				m_mapArrangement( cls.m_mapArrangement ),
				m_pGenPointerType( cls.m_pGenPointerType ),
				m_pGenConstPtrType( cls.m_pGenConstPtrType ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// オブジェクト生成（コンストラクタは後で呼び出し元責任で呼び出す）
		virtual LObject * CreateInstance( void ) ;

		// 親クラス追加
		virtual bool AddSuperClass( LClass * pClass ) ;
		// インターフェース追加（２つめ以降の struct 派生）
		virtual bool AddImplementClass( LClass * pClass ) ;

		// 親クラスへのキャストの際のオフセット
		const size_t * GetOffsetCastTo( LClass * pClass ) const ;

		// 構造体のサイズ
		size_t GetStructBytes( void ) const
		{
			return	GetProtoArrangemenet().GetAlignedBytes() ;
		}

	public:
		// 二項演算子
		static LValue::Primitive operator_store_ptr
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		static LValue::Primitive operator_store_fetch_addr
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Exception クラス
	//////////////////////////////////////////////////////////////////////////

	class	LExceptionClass	: public LClass
	{
	public:
		LExceptionClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LClass( vm, pNamespace, pClass, pwszName )
		{
			m_pPrototype = new LExceptionObj( this ) ;
		}
		LExceptionClass( const LExceptionClass& cls )
			: LClass( cls ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[2] ;
		static const ClassMemberDesc	s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Task クラス
	//////////////////////////////////////////////////////////////////////////

	class	LTaskClass	: public LClass
	{
	protected:
		LType	m_typeReturn ;

	public:
		LTaskClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName,
				const LType& typeReturn )
			: LClass( vm, pNamespace, pClass, pwszName ),
				m_typeReturn( typeReturn )
		{
			m_pPrototype = new LTaskObj( this ) ;
		}
		LTaskClass( const LTaskClass& cls )
			: LClass( cls ),
				m_typeReturn( cls.m_typeReturn ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[11] ;
		static const NativeFuncDesc		s_Functions[3] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// 返り値型取得
		const LType& GetReturnType( void ) const
		{
			return	m_typeReturn ;
		}

	} ;


	//////////////////////////////////////////////////////////////////////////
	// Thread クラス
	//////////////////////////////////////////////////////////////////////////

	class	LThreadClass	: public LClass
	{
	protected:
		LType	m_typeReturn ;

	public:
		LThreadClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName,
				const LType& typeReturn )
			: LClass( vm, pNamespace, pClass, pwszName ),
				m_typeReturn( typeReturn )
		{
			m_pPrototype = new LThreadObj( this ) ;
		}
		LThreadClass( const LThreadClass& cls )
			: LClass( cls ),
				m_typeReturn( cls.m_typeReturn ) {}

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[3] ;
		static const NativeFuncDesc		s_Functions[3] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// 返り値型取得
		const LType& GetReturnType( void ) const
		{
			return	m_typeReturn ;
		}

	} ;

}


#endif


