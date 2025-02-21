
#ifndef	__LOQUATY_VALUE_H__
#define	__LOQUATY_VALUE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 型と値
	//////////////////////////////////////////////////////////////////////////

	class	LValue
	{
	public:
		union	Primitive
		{
			LBoolean	boolValue ;
			LInt		intValue ;
			LLong		longValue ;
			LUint64		ulongValue ;
			LFloat		flValue ;
			LDouble		dblValue ;
			LObject *	pObject ;
			void *		pVoidPtr ;
		} ;
		static Primitive MakeBool( LBoolean value )
		{
			Primitive	prm ;
			#ifdef	_LOQUATY_BIG_ENDIAN
			prm.longValue = 0 ;
			prm.boolValue = value ;
			#else
			prm.longValue = value ;
			#endif
			return	prm ;
		}
		static Primitive MakeLong( LLong value )
		{
			Primitive	prm ;
			prm.longValue = value ;
			return	prm ;
		}
		static Primitive MakeUint64( LUint64 value )
		{
			Primitive	prm ;
			prm.ulongValue = value ;
			return	prm ;
		}
		static Primitive MakeDouble( LDouble value )
		{
			Primitive	prm ;
			prm.dblValue = value ;
			return	prm ;
		}
		static Primitive MakeObjectPtr( LObject * pObject )
		{
			Primitive	prm ;
			prm.pObject = pObject ;
			return	prm ;
		}
		static Primitive MakeVoidPtr( void * pVoidPtr )
		{
			Primitive	prm ;
			prm.pVoidPtr = pVoidPtr ;
			return	prm ;
		}

	protected:
		LType		m_type ;
		Primitive	m_value ;
		LObjPtr		m_pObject ;

	public:
		LValue( void )
			: m_value( MakeObjectPtr(nullptr) ) {}
		LValue( const LType& type, LObjPtr pObj = nullptr )
			: m_type( type ), m_pObject( pObj )
		{
			m_value.pObject = pObj.Ptr() ;
		}
		LValue( LType::Primitive type, const Primitive& value )
			: m_type( type ), m_value( value ) {}
		LValue( LObjPtr pObj ) ;
		LValue( const LValue& value )
			: m_type( value.m_type ),
				m_value( value.m_value ),
				m_pObject( value.m_pObject ) {}

		const LValue& operator = ( const LValue& value )
		{
			m_type = value.m_type ;
			m_value = value.m_value ;
			m_pObject = value.m_pObject ;
			return	*this ;
		}

		// 型情報
		const LType& GetType( void ) const
		{
			return	m_type ;
		}
		LType& Type( void )
		{
			return	m_type ;
		}

		// 値
		const Primitive& Value( void ) const
		{
			return	m_value ;
		}
		Primitive& Value( void )
		{
			return	m_value ;
		}
		const LObjPtr& GetObject( void ) const
		{
			return	m_pObject ;
		}

		// void か？
		bool IsVoid( void ) const
		{
			return	m_type.IsVoid() && (m_pObject == nullptr) ;
		}

		// null か？
		bool IsNull( void ) const
		{
			return	m_type.IsObject() && (m_pObject == nullptr) ;
		}

		// オブジェクトなら AddRef する
		LObject * AddRef( void ) const ;

		// 複製を作成
		LValue Clone( void ) const ;

		// 値を評価
		LBoolean AsBoolean( void ) const ;
		LLong AsInteger( void ) const ;
		LDouble AsDouble( void ) const ;
		LString AsString( void ) const ;

		// ポインタの参照先やオブジェクトに値を設定
		bool PutInteger( LLong val ) ;
		bool PutDouble( LDouble val ) ;
		bool PutString( const wchar_t * str ) ;

		// クラスのメンバやポインタの参照先の構造体に値を設定
		// ※AsExpression で文字列化した値を LCompiler::EvaluateConstExpr で
		// 　解釈し PutMembers でリストアすることができる
		bool PutMembers( const LValue& val ) ;

		// 要素取得
		LValue GetElementAt
			( LVirtualMachine& vm,
				size_t index, bool flagRef = false ) const ;
		LValue GetMemberAs
			( LVirtualMachine& vm,
				const wchar_t * name, bool flagRef = false ) const ;
		// 要素数取得
		size_t GetElementCount( void ) const ;
		// 要素名取得
		const wchar_t * GetElementNameAt
				( LString& strName, size_t index ) const ;

		// ポインタの参照先がプリミティブ型／
		// ボックス化されたオブジェクトの場合評価する
		LValue UnboxingData( void ) const ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 演算子関数
	//////////////////////////////////////////////////////////////////////////

	namespace	Symbol
	{
		// 演算子関数
		// ※ LObject* を引数に受け取る関数は、呼び出し元の責任で ReleaseRef する。
		//    呼び出された関数内で ReleaseRef しない。
		// ※ LObject* を引数に受け取る場合、演算子関数は nullptr が渡される可能性が
		//    あることに注意する必要がある。
		//    nullptr が渡されても必ず安全に処理できなければならない。
		//    （NullPointerException を投げるのも関数側の責任）
		// ※ 評価値が LObject* の演算子は呼び出し元の責任で解放する。
		//   （ランタイムで判定せずコンパイル時の型で解放するか決定する）
		//   （instance には演算子が必要とする拡張データを受け渡すことができる）
		//   （スクリプト上で operator を記述する場合には instance 経由で型情報などを渡す）
		typedef	LValue::Primitive
					(*PFN_OPERATOR1)
						( LValue::Primitive val1, void * instance ) ;
		typedef	LValue::Primitive
					(*PFN_OPERATOR2)
						( LValue::Primitive val1,
							LValue::Primitive val2, void * instance ) ;

		// 演算子種類
		enum	OperatorClass
		{
			operatorBinary,		// 二項演算子
			operatorUnary,		// 前置単項演算子
			operatorUnaryPost,	// 後置単項演算子
		} ;

		// 演算子定義基底
		struct	OperatorDef
		{
			OperatorClass	m_opClass ;		// 演算子種類
			OperatorIndex	m_opIndex ;		// 演算子
			const wchar_t *	m_pwszComment ;	// コメント
			bool			m_constExpr ;	// 定数式で実行できる
			bool			m_constThis ;	// 左辺オブジェクトは const か？
			LType			m_typeRet ;		// 演算子評価型
			LClass *		m_pThisClass ;	// 左辺オブジェクトのクラス
		} ;

		// 単項演算子実装定義
		struct	UnaryOperatorDef	: public OperatorDef
		{
			PFN_OPERATOR1	m_pfnOp ;		// 実行関数
			void *			m_pInstance ;
			LFunctionObj *	m_pOpFunc ;
		} ;

		// 二項演算子実装定義
		struct	BinaryOperatorDef	: public OperatorDef
		{
			PFN_OPERATOR2	m_pfnOp ;		// 実行関数
			LType			m_typeRight ;	// 右項型
			void *			m_pInstance ;
			LFunctionObj *	m_pOpFunc ;
		} ;
	}

}


#endif

