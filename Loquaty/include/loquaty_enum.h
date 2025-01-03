
#ifndef	__LOQUATY_ENUM_H__
#define	__LOQUATY_ENUM_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Loquaty 列挙型
	//////////////////////////////////////////////////////////////////////////

	class	LEnumerativeClass	: public LClass
	{
	protected:
		LType			m_typeElement ;		// 列挙子型
		bool			m_flagUsing ;		// using 指定
		std::uint64_t	m_maskEnum ;		// 整数型のビットマスク

	public:
		LEnumerativeClass
			( LVirtualMachine& vm,
				LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName,
				const LType& typeElement )
			: LClass( vm, pNamespace, pClass, pwszName ),
				m_typeElement( typeElement ),
				m_flagUsing( false ), m_maskEnum( 0 ) { }
		LEnumerativeClass( const LEnumerativeClass& cls )
			: LClass( cls ),
				m_typeElement( cls.m_typeElement ),
				m_flagUsing( cls.m_flagUsing ),
				m_maskEnum( cls.m_maskEnum ) { }

	public:
		// 要素型取得
		const LType& GetEnumElementType( void ) const
		{
			return	m_typeElement ;
		}
		// using 指定されたか？
		bool IsUsingEnumerator( void ) const
		{
			return	m_flagUsing ;
		}
		// using 指定
		void SetUsingEnumFlag( bool flagUsing )
		{
			m_flagUsing = flagUsing ;
		}
		// 整数型のビットマスク取得
		std::uint64_t GetIntEnumMask( void ) const
		{
			return	m_maskEnum ;
		}
		// 整数型のビットマスク設定
		void SetIntEnumMask( std::uint64_t mask )
		{
			m_maskEnum = mask ;
		}
		// 列挙子に適合する値か？
		bool IsConformableValue( const LValue& value ) const ;

	public:
		// 複製する（要素も全て複製処理する）
		virtual LObject * CloneObject( void ) const ;
		// 複製する（要素は参照する形で複製処理する）
		virtual LObject * DuplicateObject( void ) const ;

	} ;

}

#endif

