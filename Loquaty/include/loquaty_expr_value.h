
#ifndef	__LOQUATY_EXPR_VALUE_H__
#define	__LOQUATY_EXPR_VALUE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// コンパイル時の数式の評価値
	//////////////////////////////////////////////////////////////////////////

	class	LExprValue	: public LValue
	{
	public:
		enum	ExprOptionType
		{
			exprSingle,			// 通常 LExprValue
								// ※スタック上の確保情報のため m_optValue1 にダミーを保持する場合があることに留意

			exprRefPointer,		// LType は CanArrangeOnBuf()==true
								// LObject* は LPointerObj* の静的変数へのポインタ
								// 基本的に定数式の中で使用（実行時式では exprRefPtrOffset を使用）
			exprFuncVar,		// m_pFuncVar (静的な関数の候補)
								// LType は基底 Function 型
			exprRefVirtual,		// class::func 表現 m_pVirtClass, m_pVirtuals
								// LType は基底 Function 型
			exprOperator,		// m_pOperatorDef
								// LType は基底 Function 型
			exprNamespace,		// m_namespace

			exprPtrOffset,		// LValue:=pointer-type, opt1:=pointer, opt1:=offset(in bytes) : pointer + offset
			exprRefPtrOffset,	// LValue:=element-type, opt1:=pointer, opt1:=offset(in bytes) : pointer + offset
			exprRefByIndex,		// LValue:=element-type, opt1:=ref_obj, opt2:=index            : ref_obj[index]
			exprRefByStr,		// LValue:=element-type, opt1:=ref_obj, opt2:=str              : ref_obj[str]
			exprRefCallThis,	// LValue:=func-type, opt1:=call_obj, opt2:func                : call_obj.*func

			exprFirstHasOptions	= exprPtrOffset,
		} ;

	protected:
		bool				m_constExpr ;	// 定数値か？
		bool				m_uniqueObj ;	// 定数オブジェクトはユニークオブジェクトか？
		bool				m_localVar ;	// ローカルスタック上か？
		ExprOptionType		m_optType ;		// 式のオプションタイプ

		const LFunctionVariation *
							m_pFuncVar ;	// 静的な関数群
		std::shared_ptr<LFunctionVariation>
							m_ownFuncVar ;

		LClass *			m_pVirtClass ;	// 仮想関数のクラス
		const std::vector<size_t> *
							m_pVirtuals ;	// 仮想関数エントリ

		const Symbol::OperatorDef *
							m_pOperatorDef ;// 演算子関数

		LPtr<LNamespace>	m_namespace ;	// 名前空間

		std::shared_ptr<LExprValue>
							m_optValue1 ;	// オプション値
		std::shared_ptr<LExprValue>
							m_optValue2 ;

	public:
		LExprValue( void )
			: m_constExpr( true ), m_localVar( false ), m_uniqueObj( false ),
				m_optType( exprSingle ),
				m_pFuncVar( nullptr ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( const LExprValue& expr )
			: LValue( expr ),
				m_constExpr( expr.m_constExpr ),
				m_uniqueObj( expr.m_uniqueObj ),
				m_localVar( false ),
				m_optType( expr.m_optType ),
				m_pFuncVar( expr.m_pFuncVar ),
				m_ownFuncVar( expr.m_ownFuncVar ),
				m_pVirtClass( expr.m_pVirtClass ),
				m_pVirtuals( expr.m_pVirtuals ),
				m_pOperatorDef( expr.m_pOperatorDef ),
				m_namespace( expr.m_namespace ),
				m_optValue1( expr.m_optValue1 ),
				m_optValue2( expr.m_optValue2 ) {}

		LExprValue( const LType& type,
					LObjPtr pObj, bool constExpr,
					bool uniqueObj,
					ExprOptionType optType = exprSingle )
			: LValue( type, pObj ), m_constExpr( constExpr ),
				m_uniqueObj( uniqueObj ), m_localVar( false ),
				m_optType( optType ), m_pFuncVar( nullptr ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( LType::Primitive type,
					const Primitive& value,
					bool constExpr,
					ExprOptionType optType = exprSingle )
			: LValue( type, value ), m_constExpr( constExpr ),
				m_uniqueObj( false ), m_localVar( false ),
				m_optType( optType ), m_pFuncVar( nullptr ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( const LValue& value, ExprOptionType optType = exprSingle )
			: LValue( value ), m_constExpr( true ),
				m_uniqueObj( false ), m_localVar( false ),
				m_optType( optType ), m_pFuncVar( nullptr ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( LClass * pFuncClass, const LFunctionVariation * pFuncVar )
			: LValue( LType(pFuncClass), nullptr ),
				m_constExpr( true ), m_uniqueObj( false ), m_localVar( false ),
				m_optType( exprFuncVar ), m_pFuncVar( pFuncVar ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( LClass * pFuncClass, std::shared_ptr<LFunctionVariation> pFuncVar )
			: LValue( LType(pFuncClass), nullptr ),
				m_constExpr( true ), m_uniqueObj( false ), m_localVar( false ),
				m_optType( exprFuncVar ),
				m_pFuncVar( pFuncVar.get() ), m_ownFuncVar( pFuncVar ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( LClass * pFuncClass,
					LClass * pVirtClass, const std::vector<size_t> * pVirtVec )
			: LValue( LType(pFuncClass), nullptr ),
				m_constExpr( false ), m_uniqueObj( false ), m_localVar( false ),
				m_optType( exprRefVirtual ),
				m_pFuncVar( nullptr ),
				m_pVirtClass( pVirtClass ), m_pVirtuals( pVirtVec ),
				m_pOperatorDef( nullptr ) {}

		LExprValue( LClass * pFuncClass,
					const Symbol::OperatorDef * pOperatorDef )
			: LValue( LType(pFuncClass), nullptr ),
				m_constExpr( true ), m_uniqueObj( false ), m_localVar( false ),
				m_optType( exprOperator ),
				m_pFuncVar( nullptr ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( pOperatorDef ) {}

		LExprValue( LPtr<LNamespace> pns )
			: m_constExpr( true ), m_uniqueObj( false ), m_localVar( false ),
				m_optType( exprNamespace ),
				m_namespace( pns ),
				m_pFuncVar( nullptr ),
				m_pVirtClass( nullptr ), m_pVirtuals( nullptr ),
				m_pOperatorDef( nullptr ) {}

		const LExprValue& operator = ( const LExprValue& expr )
		{
			LValue::operator = ( expr ) ;
			m_constExpr = expr.m_constExpr ;
			m_uniqueObj = expr.m_uniqueObj ;
			m_optType = expr.m_optType ;
			m_pFuncVar = expr.m_pFuncVar ;
			m_ownFuncVar = expr.m_ownFuncVar ;
			m_pVirtClass = expr.m_pVirtClass ;
			m_pVirtuals = expr.m_pVirtuals ;
			m_pOperatorDef = expr.m_pOperatorDef ;
			m_namespace = expr.m_namespace ;
			m_optValue1 = expr.m_optValue1 ;
			m_optValue2 = expr.m_optValue2 ;
			return	*this ;
		}

		// 型情報設定
		void SetType( const LType& type, bool constExpr = false )
		{
			m_type = type ;
			m_constExpr = constExpr ;
		}

		// 定数値設定
		void SetConstValue( const LValue& value )
		{
			LValue::operator = ( value ) ;
			m_constExpr = true ;
		}

		// 単体要素の評価値として設定
		void SetSingle( ExprOptionType optType = exprSingle ) ;
		// 単体要素か？
		bool IsSingle( void ) const ;

		// 第二、第三要素の評価値を設定
		void SetOption
			( ExprOptionType type,
				std::shared_ptr<LExprValue> val1,
				std::shared_ptr<LExprValue> val2 ) ;
		void SetOptionPointerOffset
			( std::shared_ptr<LExprValue> valPtr,
				std::shared_ptr<LExprValue> valOffset ) ;
		void SetOptionRefPointerOffset
			( std::shared_ptr<LExprValue> valPtr,
				std::shared_ptr<LExprValue> valOffset ) ;
		void SetOptionRefByIndexOf
			( std::shared_ptr<LExprValue> valObj,
				std::shared_ptr<LExprValue> valIndex ) ;
		void SetOptionRefByStrOf
			( std::shared_ptr<LExprValue> valObj,
				std::shared_ptr<LExprValue> valStr ) ;
		void SetOptionRefCallThisOf
			( std::shared_ptr<LExprValue> valThisObj,
				std::shared_ptr<LExprValue> valFunc ) ;

		// オプション・タイプ
		ExprOptionType OptionType( void ) const ;

		// 参照型として修飾されているか？
		// (IsRefPointer() || IsOnLocal() || IsObjReference())
		bool IsReference( void ) const ;
		// ローカル上のエントリか？
		bool IsOnLocal( void ) const ;
		bool IsPointerOnLocal( void ) const ;
		bool IsPtrOffsetOnLocal( void ) const ;
		// 参照ポインタか？（実態はポインタでコンパイル時の型情報が参照）
		bool IsRefPointer( void ) const ;
		// ポインタを参照ポインタに変更する
		void MakeIntoRefPointer( void ) ;
		// 参照ポインタをポインタに変更する
		void MakeRefIntoPointer( void ) ;

		// オフセット付きポインタか？
		bool IsOffsetPointer( void ) const ;
		bool IsRefOffsetPointer( void ) const ;

		// fetch_addr ポインタか？
		bool IsFetchAddrPointer( void ) const ;

		// 解放が必要な型か？
		bool IsTypeNeededToRelease( void ) ;

		// オブジェクト要素参照か？
		bool IsObjReference( void ) const ;
		bool IsRefByIndex( void ) const ;
		bool IsRefByString( void ) const ;

		// 関数間接呼び出しか？
		bool IsRefCallThis( void ) const ;

		// 定数値判定
		bool IsConstExpr( void ) const ;
		bool IsUniqueObject( void ) const ;
		bool IsFunctionVariation( void ) const ;
		bool IsOperatorFunction( void ) const ;
		bool IsRefCallFunction( void ) const ;
		bool IsRefVirtualFunction( void ) const ;
		bool IsNamespace( void ) const ;
		bool IsConstExprClass( void ) const ;
		bool IsConstExprFunction( void ) const ;

		// 定数式評価値取得
		LClass * GetConstExprClass( void ) const ;
		LPtr<LFunctionObj> GetConstExprFunction( void ) const ;

		// 静的な関数群
		const LFunctionVariation * GetFuncVariation( void ) const ;
		// 仮想関数
		LClass * GetVirtFuncClass( void ) const ;
		const std::vector<size_t> * GetVirtFunctions( void ) const ;
		// 演算子関数
		const Symbol::OperatorDef * GetOperatorFunction( void ) const ;
		// 名前空間
		LPtr<LNamespace> GetNamespace( void ) const ;
		// 第二要素
		std::shared_ptr<LExprValue> GetOption1( void ) const ;
		std::shared_ptr<LExprValue> GetNonOffsetPointer( void ) ;
		std::shared_ptr<LExprValue> MoveOption1( void ) ;
		void SetOption1( std::shared_ptr<LExprValue> pOpt1 ) ;
		// 第三要素
		std::shared_ptr<LExprValue> GetOption2( void ) const ;
		std::shared_ptr<LExprValue> MoveOption2( void ) ;
		void SetOption2( std::shared_ptr<LExprValue> pOpt2 ) ;

		// 即値生成
		static std::shared_ptr<LExprValue> MakeConstExprInt( LLong val ) ;
		static std::shared_ptr<LExprValue> MakeConstExprFloat( LDouble val ) ;
		static std::shared_ptr<LExprValue> MakeConstExprDouble( LDouble val ) ;
		static std::shared_ptr<LExprValue> MakeConstExprObject( LObjPtr pObject ) ;
		static std::shared_ptr<LExprValue>
			MakeConstExprString( LVirtualMachine& vm, const LString& str ) ;
	} ;

	typedef	std::shared_ptr<LExprValue>	LExprValuePtr ;


	//////////////////////////////////////////////////////////////////////////
	// 実行時の型と値の配列（関数の引数や、数式の実行時スタック等）
	//////////////////////////////////////////////////////////////////////////

	class	LExprValueArray	: public std::vector<LExprValuePtr>
	{
	public:
		// プッシュ
		LExprValuePtr Push( LExprValuePtr expr )
		{
			std::vector<LExprValuePtr>::push_back( expr ) ;
			return	expr ;
		}
		LExprValuePtr PushBool( LBoolean val ) ;
		LExprValuePtr PushLong( LLong val ) ;
		LExprValuePtr PushDouble( LDouble val ) ;
		LExprValuePtr PushObject( const LType& type, LObject* pObj ) ;
		LExprValuePtr PushRuntimeType( const LType& type, bool responsible ) ;

		// 個数
		size_t GetLength( void ) const
		{
			return	std::vector<LExprValuePtr>::size() ;
		}

		// 要素取得
		LExprValuePtr GetAt( size_t index ) const
		{
			return	(index < std::vector<LExprValuePtr>::size())
					? std::vector<LExprValuePtr>::at(index) : nullptr ;
		}
		LExprValuePtr GetBackAt( size_t index ) const
		{
			return	(index < std::vector<LExprValuePtr>::size())
					? std::vector<LExprValuePtr>::at(BackIndex(index)) : nullptr ;
		}
		size_t BackIndex( size_t index ) const
		{
			assert( index < std::vector<LExprValuePtr>::size() ) ;
			return	std::vector<LExprValuePtr>::size() - 1 - index ;
		}

		// 代入
		const LExprValueArray& operator =
					( const LExprValueArray& xvalArray )
		{
			std::vector<LExprValuePtr>::operator = ( xvalArray ) ;
			return	*this ;
		}

		// 比較
		bool operator == ( const LExprValueArray& xvalArray ) const
		{
			return	IsEqual( xvalArray ) ;
		}
		bool operator != ( const LExprValueArray& xvalArray ) const
		{
			return	!IsEqual( xvalArray ) ;
		}
		bool IsEqual( const LExprValueArray& xvalArray ) const ;

		// 要素検索
		ssize_t Find( LExprValuePtr expr ) const ;
		ssize_t FindBack( LExprValuePtr expr ) const ;

		// 解放可能なスタック数を数える
		size_t CountBackTemporaries( void ) ;

		// スタックを開放する
		void FreeStack( size_t nCount ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// ローカル変数の型と値とその他の情報
	//////////////////////////////////////////////////////////////////////////

	class	LLocalVar	: public LExprValue
	{
	public:
		enum	AllocType
		{
			allocSpace,
			allocPrimitive,		// [0]:プリミティブ型
			allocObject,		// [0]:LObject*, [1]:yp back link
			allocPointer,		// [0]:LPointerObj*, [1]:yp back link, [2]:index
			allocLocalArray,	// [2]:領域...
		} ;
		AllocType	m_allocType ;		// フレームバッファ上の確保状況
		size_t		m_iLocation ;		// fp[m_iLocation] 位置
		bool		m_flagReadOnly ;
		size_t		m_maxFetchRange ;	// fetch_addr ポインタ型で参照された最大の範囲
		size_t		m_iFetchCodePos ;	// codeLoadFetchAddr を実行したコードポインタ

	public:
		LLocalVar( const LType& type,
					AllocType allocType, size_t iLoc = 0 )
			: LExprValue( type, nullptr, false, false ),
				m_allocType( allocType ), m_iLocation( iLoc ),
				m_flagReadOnly( false ),
				m_maxFetchRange( 0 ), m_iFetchCodePos( SIZE_MAX )
		{
			m_localVar = true ;
		}
		LLocalVar( const LLocalVar& lvar )
			: LExprValue( lvar ),
				m_allocType( lvar.m_allocType ),
				m_iLocation( lvar.m_iLocation ),
				m_flagReadOnly( lvar.m_flagReadOnly ),
				m_maxFetchRange( lvar.m_maxFetchRange ),
				m_iFetchCodePos( lvar.m_iFetchCodePos )
		{
			m_localVar = true ;
		}

		const LLocalVar& operator = ( const LLocalVar& lvar )
		{
			LExprValue::operator = ( lvar ) ;
			m_allocType = lvar.m_allocType ;
			m_iLocation = lvar.m_iLocation ;
			m_flagReadOnly = lvar.m_flagReadOnly ;
			m_maxFetchRange = lvar.m_maxFetchRange ;
			m_iFetchCodePos = lvar.m_iFetchCodePos ;
			return	*this ;
		}

		// 配置タイプ
		AllocType GetAllocType( void ) const
		{
			return	m_allocType ;
		}
		bool HasDestructChain( void ) const
		{
			return	(m_allocType == allocObject)
					|| (m_allocType == allocPointer) ;
		}

		// 配置ロケーション
		size_t GetLocation( void ) const
		{
			return	m_iLocation ;
		}

		// 読み取り専用
		bool IsReadOnly( void ) const
		{
			return	m_flagReadOnly ;
		}
		void SetReadOnly( void )
		{
			m_flagReadOnly = true ;
		}

		// fetch_addr ポインタの参照指標
		void HasFetchPointerRange( size_t nBytes )
		{
			m_maxFetchRange = std::max( m_maxFetchRange, nBytes ) ;
		}
		size_t GetMaxFetchPointerRange( void ) const
		{
			return	m_maxFetchRange ;
		}

	} ;

	typedef	std::shared_ptr<LLocalVar>	LLocalVarPtr ;


	//////////////////////////////////////////////////////////////////////////
	// 実行時のフレームバッファの型と値の配置
	//////////////////////////////////////////////////////////////////////////

	class	LLocalVarArray
	{
	protected:
		std::shared_ptr<LLocalVarArray>	m_pParent ;
		size_t							m_nUsed ;
		std::map<size_t,LLocalVarPtr>	m_mapVars ;
		std::map<std::wstring,size_t>	m_mapNames ;

	public:
		LLocalVarArray( std::shared_ptr<LLocalVarArray> pParent = nullptr )
			: m_pParent( pParent ), m_nUsed( 0 ) {}
		LLocalVarArray( const LLocalVarArray& lva )
			: m_nUsed( lva.m_nUsed ),
				m_mapVars( lva.m_mapVars ),
				m_mapNames( lva.m_mapNames ) {}

		// 使用領域サイズ
		size_t GetUsedSize( void ) const
		{
			return	m_nUsed ;
		}
		void AddUsedSize( size_t nUsed )
		{
			m_nUsed += nUsed ;
		}
		void TruncateUsedSize( size_t nUsed ) ;

		// 変数追加
		LLocalVarPtr AddLocalVar
			( const wchar_t * pwszName, const LType& type ) ;
		LLocalVarPtr AllocLocalVar( const LType& type ) ;
		LLocalVarPtr PutLocalVar( size_t iLoc, const LType& type ) ;
		void SetLocalVarNameAs
			( const wchar_t * pwszName, LLocalVarPtr pVar ) ;

		// 変数取得（この配列のみ）
		LLocalVarPtr GetLocalVarAs( const wchar_t * pwszName ) const ;
		LLocalVarPtr GetLocalVarAt( size_t iLoc ) const ;

		// 変数名取得
		const std::wstring * GetLocalVarNameAt( size_t iLoc ) const ;

		// 変数名配列
		const std::map<std::wstring,size_t>& GetLocalNameList( void ) const
		{
			return	m_mapNames ;
		}

		// 変数を検索（この配列のみ）
		ssize_t FindLocalVar( LExprValuePtr xval ) const ;

		// fetch_addr 修飾された変数が存在するか？（この配列のみ）
		bool AreThereAnyFetchAddr( void ) const ;

		// 適合するか？（この配列のみ）
		bool DoesMatchWith( const LLocalVarArray& lvaFrom, bool flagEqueal ) const ;

		// 結合
		const LLocalVarArray& operator += ( const LLocalVarArray& lva ) ;

		// 親のフレーム
		std::shared_ptr<LLocalVarArray> GetParent( void ) const
		{
			return	m_pParent ;
		}
		void DetachParent( void )
		{
			m_pParent = nullptr ;
		}
	} ;

	typedef	std::shared_ptr<LLocalVarArray>	LLocalVarArrayPtr ;

}


#endif

