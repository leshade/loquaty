
#ifndef	__LOQUATY_TYPE_H__
#define	__LOQUATY_TYPE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 基本的な型情報
	//////////////////////////////////////////////////////////////////////////

	class	LType
	{
	public:
		// プリミティブ型
		enum	Primitive
		{
			typeBoolean,
			typeInt8, typeUint8,
			typeInt16, typeUint16,
			typeInt32, typeUint32,
			typeInt64, typeUint64,
			typeFloat, typeDouble,
			typeObject,				// ※以降、基本オブジェクト
			typeNativeObject,
			typeDataArray,
			typeStructure,
			typePointer,
			typeIntegerObj,
			typeDoubleObj,
			typeString,
			typeStringBuf,
			typeArray,
			typeMap,
			typeFunction,
			typeAllCount,
			typePrimitiveCount = typeObject,
		} ;

		// アクセス修飾子フラグ
		enum	AccessModifier
		{
			modifierDefault,
			modifierPublic,
			modifierProtected,
			modifierPrivate,
			modifierPrivateInvisible,
			accessMask				= 0x00000007,
			modifierStatic			= 0x00000100,
			modifierAbstract		= 0x00000200,
			modifierConst			= 0x00000400,
			modifierNative			= 0x00000800,
			modifierSynchronized	= 0x00001000,
			modifierFetchAddr		= 0x00002000,
			modifierOverride		= 0x00010000,
			modifierDeprecated		= 0x00020000,
		} ;
		typedef	std::uint32_t	Modifiers ;

		// コメント情報
		class	LComment	: public LString
		{
		public:
			std::shared_ptr<LXMLDocument>	m_xmlDoc ;
		public:
			LComment( const wchar_t * pwszStr = nullptr ) : LString( pwszStr ) {}
		} ;

	protected:
		Primitive	m_type ;
		Modifiers	m_accMod ;	// アクセス修飾子 (enum AccessModifier の組み合わせ）
		LClass *	m_pClass ;
		LComment *	m_pComment ;

	public:
		// メモリ配置上の整列サイズ
		static const size_t		s_bytesAligned[typeAllCount] ;

		// プリミティブ型名
		static const wchar_t *	s_pwszPrimitiveTypeName[typePrimitiveCount] ;

		// プリミティブ型名判定（一致しない場合は typeObject を返却）
		static Primitive AsPrimitive( const wchar_t * name ) ;

		// 整数型か？
		static bool IsIntegerPrimitive( Primitive type )
		{
			return	(type >= typeInt8) && (type <= typeUint64) ;
		}
		static bool IsUnsignedIntegerPrimitive( Primitive type )
		{
			return	(type == typeUint8)
					|| (type == typeUint16)
					|| (type == typeUint32)
					|| (type == typeUint64) ;
		}

		// 浮動週数点型か？
		static bool IsFloatingPointPrimitive( Primitive type )
		{
			return	(type == typeFloat) || (type == typeDouble) ;
		}

		// プリミティブ型の合成
		static Primitive MaxPrimitiveOf( Primitive type1, Primitive type2 ) ;

		// プリミティブ型か？
		static bool IsPrimitiveType( Primitive type )
		{
			return	(type < typeObject) ;
		}
		// オブジェクト型か？
		static bool IsObjectType( Primitive type )
		{
			return	(type >= typeObject) ;
		}

	public:
		LType( LClass * pClass = nullptr, LType::Modifiers accMod = 0 )
			: m_type( typeObject ), m_accMod( accMod ),
				m_pClass( pClass ), m_pComment( nullptr ) { }
		LType( Primitive type, LType::Modifiers accMod = 0 )
			: m_type( type ), m_accMod( accMod ),
				m_pClass( nullptr ), m_pComment( nullptr ) { }
		LType( Primitive type, LClass * pClass, LType::Modifiers accMod = 0 )
			: m_type( type ), m_accMod( accMod ),
				m_pClass( pClass ), m_pComment( nullptr ) { }
		LType( const LType& type, bool constModify = false )
			: m_type( type.m_type ),
				m_accMod( type.m_accMod
						| (constModify ? modifierConst : 0) ),
				m_pClass( type.m_pClass ), m_pComment( type.m_pComment ) { }
		const LType& operator = ( const LType& type )
		{
			m_type = type.m_type ;
			m_accMod = type.m_accMod ;
			m_pClass = type.m_pClass ;
			m_pComment = type.m_pComment ;
			return	*this ;
		}

		// void か？
		bool IsVoid( void ) const
		{
			return	(m_type == typeObject) && (m_pClass == nullptr) ;
		}

		// プリミティブ型判定
		bool IsPrimitive( void ) const
		{
			return	IsPrimitiveType( m_type ) ;
		}
		bool IsBoolean( void ) const
		{
			return	(m_type == typeBoolean) ;
		}
		bool IsInteger( void ) const
		{
			return	IsIntegerPrimitive( m_type ) ;
		}
		bool IsUnsignedInteger( void ) const
		{
			return	IsUnsignedIntegerPrimitive( m_type ) ;
		}
		bool IsFloatingPointNumber( void ) const
		{
			return	IsFloatingPointPrimitive( m_type ) ;
		}

		// Object 型判定
		bool IsObject( void ) const
		{
			return	IsObjectType( m_type ) ;
		}
		bool IsArray( void ) const ;
		bool IsMap( void ) const ;
		bool IsFunction( void ) const ;
		bool IsStructure( void ) const ;
		bool IsStructurePtr( void ) const ;
		bool IsDataArray( void ) const ;
		bool IsPointer( void ) const ;
		bool IsFetchAddr( void ) const ;
		bool IsIntegerObj( void ) const ;
		bool IsDoubleObj( void ) const ;
		bool IsString( void ) const ;
		bool IsStringBuf( void ) const ;
		bool IsTask( void ) const ;
		bool IsThread( void ) const ;

		// バッファ配列可能な型か？（ポインタ型にできるか？）
		bool CanArrangeOnBuf( void ) const ;

		// 実行時にオブジェクト表現される型か？
		bool IsRuntimeObject( void ) const ;
		// 解放が必要な型か？
		bool IsNeededToRelease( void ) const ;

		// 型情報取得
		Primitive GetPrimitive( void ) const
		{
			return	m_type ;
		}
		LClass * GetClass( void ) const ;
		LArrayClass * GetArrayClass( void ) const ;
		LMapClass * GetMapClass( void ) const ;
		LFunctionClass * GetFunctionClass( void ) const ;
		LTaskClass * GetTaskClass( void ) const ;
		LThreadClass * GetThreadClass( void ) const ;
		LStructureClass * GetStructureClass( void ) const ;
		LStructureClass * GetPtrStructureClass( void ) const ;	// struct* 型の struct
		LDataArrayClass * GetDataArrayClass( void ) const ;
		LPointerClass * GetPointerClass( void ) const ;

		// ポインタ型かデータ配列型で装飾されていないベースの型を取得する
		LType GetBaseDataType( void ) const ;

		// ポインタ型の場合、ポインタ装飾を外した型を取得する
		// データ配列型の場合、１回要素参照した型を取得する
		// 構造体型の場合、そのままを取得する
		// Map, Array の場合は要素型を取得する
		// String, StringBuf の場合は Integer 型を取得する
		// それ以外の場合は void を取得する
		LType GetRefElementType( void ) const ;

		// 配置の際のアライメント
		size_t GetAlignBytes( void ) const ;
		// 配置の際のデータサイズ
		size_t GetDataBytes( void ) const ;

		// 同値判定
		bool IsEqual( const LType& type ) const
		{
			return	(m_type == type.m_type)
					&& (m_pClass == type.m_pClass)
					&& (m_accMod == type.m_accMod) ;
		}
		bool operator == ( const LType& type ) const
		{
			return	IsEqual( type ) ;
		}
		bool operator != ( const LType& type ) const
		{
			return	!IsEqual( type ) ;
		}

		// 型名取得
		LString GetTypeName( void ) const ;

		// コメントデータ
		LComment * GetComment( void ) const ;
		void SetComment( LComment * pComment ) ;

	public:
		// キャスト・メソッド
		enum	CastMethod
		{
			castImpossible,			// キャスト不可能
			castableJust,			// 変換不要
			castableUpCast,			// アップキャスト（データ変換不要・型のみ変換）
			castableUpCastPtr,		// ポインタのアップキャスト（オフセットが必要な場合あり）
			castableArrayToPtr,		// データ配列のポインタ化
			castableDataToPtr,		// データ（プリミティブ/構造体）のポインタ化
			castableObjectToPtr,	// オブジェクトのポインタ化
			castableBoxing,			// 数値のオブジェクト化
			castableUnboxing,		// オブジェクトの数値化
			castablePrecision,		// 数値変換（精度の拡張：暗黙変換）
			castableConvertNum,		// 数値変換（明示的なキャストが必要）
			castableNumToStr,		// 文字列化（明示的なキャストが必要）
			castableObjToStr,		// 文字列化（明示的なキャストが必要）
			castableStrToNum,		// 文字列の数値化（明示的なキャストが必要）
			castableConstDataToPtr,	// const データ（プリミティブ/構造体）のポインタ化
			castableConstCast,		// const キャスト（明示的なキャストが必要）
			castableConstCastPtr,	// const ポインタキャスト（明示的なキャストが必要）
			castableDownCast,		// ダウンキャスト（明示的なキャストが必要）
			castableDownCastPtr,	// ポインタのダウンキャスト（明示的なキャストが必要）（オフセットが必要な場合あり）
			castableCrossCast,		// クロスキャスト（明示的なキャストが必要）
			castableCastStrangerPtr,// ポインタの強制キャスト（明示的なキャストが必要）
			castableExplicitly 	= castableConvertNum,
			castableDangerous 	= castableCrossCast,
		} ;
		// 暗黙のキャスト可能か？（データの変換を含む）
		CastMethod CanCastTo( const LType& type ) const ;
		// 暗黙のキャスト可能か？（データの変換を含む）
		CastMethod CanImplicitCastTo( const LType& type ) const ;
		// 明示的なキャスト可能か？（データの変換を含む）
		CastMethod CanExplicitCastTo( const LType& type ) const ;

	public:	// アクセス修飾子
		// 修飾フラグ
		LType::Modifiers GetModifiers( void ) const
		{
			return	m_accMod ;
		}
		bool IsModifiered( LType::Modifiers mask ) const
		{
			return	(m_accMod & mask) != 0 ;
		}
		void SetModifiers( LType::Modifiers accMod )
		{
			m_accMod = accMod ;
		}

		// const 修飾
		bool IsConst( void ) const
		{
			return	(m_accMod & modifierConst) != 0 ;
		}
		LType ConstWith( const LType& type ) const
		{
			return	LType( *this, type.IsConst() ) ;
		}
		LType ConstType( void ) const
		{
			return	LType( *this, true ) ;
		}
		LType ExConst( void ) const
		{
			return	LType( m_type, m_pClass,
							(m_accMod & ~modifierConst) ) ;
		}

		// アクセス可能判定
		bool IsEnableAccess( LType::Modifiers accMod ) const
		{
			return	((m_accMod & accessMask)
						<= (accMod & accessMask)) ;
		}
		// アクセス修飾子
		AccessModifier GetAccessModifier( void ) const
		{
			return	(AccessModifier) (m_accMod & accessMask) ;
		}
		void SetAccessModifier( LType::AccessModifier accMod )
		{
			m_accMod = (m_accMod & ~accessMask) | accMod ;
		}
		// private を不可視に設定する
		void MakePrivateInvisible( void )
		{
			if ( (m_accMod & accessMask) == modifierPrivate )
			{
				m_accMod = (m_accMod & ~accessMask)
							| modifierPrivateInvisible ;
			}
		}
	} ;

}


#endif

