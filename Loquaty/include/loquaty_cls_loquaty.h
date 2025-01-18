
#ifndef	__LOQUATY_CLS_LOQUATY_H__
#define	__LOQUATY_CLS_LOQUATY_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// Loquaty クラス
	//////////////////////////////////////////////////////////////////////////

	class	LLoquatyClass	: public LClass
	{
	public:
		LLoquatyClass
			( LVirtualMachine& vm, LClass * pClass,
				const wchar_t * pwszName = nullptr )
			: LClass( vm, vm.Global(), pClass, pwszName )
		{
			m_pPrototype = new LVirtualMachine() ;
		}

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[8] ;
		static const NativeFuncDesc		s_Functions[5] ;
		static const VariableDesc		s_VariableDesc[2] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// public Loquaty( Loquaty ref = null )
		static void method_init( LContext& context ) ;
		// public void release()
		static void method_release( LContext& context ) ;
		// public int includeScript( String file, StringBuf errmsg = null )
		static void method_includeScript( LContext& context ) ;
		// public Function<long()> compileAsInteger( String expr, StringBuf errmsg = null ) const
		static void method_compileAsInteger( LContext& context ) ;
		// public Function<double()> compileAsNumber( String expr, StringBuf errmsg = null ) const
		static void method_compileAsNumber( LContext& context ) ;
		// public Function<Object()> compileAsObject( String expr, StringBuf errmsg = null ) const
		static void method_compileAsObject( LContext& context ) ;
		// public Function<void()> compileAsVoid( String expr, StringBuf errmsg = null ) const
		static void method_compileAsVoid( LContext& context ) ;

		static LPtr<LFunctionObj>
			CompileFunc
				( LVirtualMachine& vmMaster,
					LVirtualMachine& vm,
					const LString& strExpr,
					LStringBufObj * pErrMsg, const LType& typeRet ) ;
		static LPtr<LFunctionObj>
			CompileFunc
				( LVirtualMachine& vmMaster,
					LVirtualMachine& vm,
					const LString& strExpr,
					LString * pErrMsg, const LType& typeRet ) ;

		// public static Loquaty getCurrent()
		static void method_getCurrent( LContext& context ) ;
		// public static String express( Object obj )
		static void method_express( LContext& context ) ;
		// public static Object evalConstExpr( String expr )
		static void method_evalConstExpr( LContext& context ) ;
		// public static void traceLocalVars()
		static void method_traceLocalVars( LContext& context ) ;

	public:
		class	Compiler	: public LCompiler
		{
		protected:
			LString *	m_pErrMsg ;

		public:
			Compiler( LVirtualMachine& vm, LString * pErrMsg )
				: LCompiler( vm ), m_pErrMsg( pErrMsg ) { }
			Compiler( LVirtualMachine& vm, LStringBufObj * pErrMsg )
				: LCompiler( vm ),
					m_pErrMsg( (pErrMsg != nullptr)
								? &(pErrMsg->m_string) : nullptr ) { }

			// 文字列出力
			virtual void PrintString( const LString& str ) ;

		} ;
	} ;

}

#endif


