
#ifndef	__LOQUATY_CLS_FILE_H__
#define	__LOQUATY_CLS_FILE_H__	1

namespace	Loquaty
{

	//////////////////////////////////////////////////////////////////////////
	// ファイル・ステート
	//////////////////////////////////////////////////////////////////////////

	class	LFileStateStructure	: public LStructureClass
	{
	public:
		LFileStateStructure
			( LVirtualMachine& vm, LPtr<LNamespace> pNamespace,
				LClass * pClass, const wchar_t * pwszName = nullptr )
			: LStructureClass( vm, pNamespace, pClass, pwszName ) { }

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const VariableDesc		s_Variables[7] ;
		static const ClassMemberDesc	s_MemberDesc ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// ファイル
	//////////////////////////////////////////////////////////////////////////

	class	LFileClass	: public LNativeObjClass
	{
	public:
		LFileClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName )
			: LNativeObjClass( vm, vm.Global(), pClass, pwszName ) { }

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[16] ;
		static const NativeFuncDesc		s_Functions[15] ;
		static const LValue				s_VarInitValue[41] ;
		static const VariableDesc		s_Variables[41] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// File()
		static void method_init( LContext& context ) ;
		// File( String path, long flags )
		static void method_init2( LContext& context ) ;
		// boolean open( String path, long flags )
		static void method_oepn( LContext& context ) ;
		// void close()
		static void method_close( LContext& context ) ;
		// uint64 read( void* buf, uint64 bytes )
		static void method_read( LContext& context ) ;
		// uint64 write( const void* buf, uint64 bytes )
		static void method_write( LContext& context ) ;
		// void flush()
		static void method_flush( LContext& context ) ;
		// void seek( long pos )
		static void method_seek( LContext& context ) ;
		// void skip( long bytes )
		static void method_skip( LContext& context ) ;
		// boolean isSeekable() const
		static void method_isSeekable( LContext& context ) ;
		// long getLength() const
		static void method_getLength( LContext& context ) ;
		// long getPosition() const
		static void method_getPosition( LContext& context ) ;
		// void truncate()
		static void method_truncate( LContext& context ) ;
		// OutputStream outStream( String enc = "utf-8" ) const
		static void method_outStream( LContext& context ) ;
		// String getFilePath() const
		static void method_getFilePath( LContext& context ) ;

		// static boolean queryState( File.State* state, String path )
		static void method_queryState( LContext& context ) ;
		// static boolean isExisting( String path )
		static void method_isExisting( LContext& context ) ;
		// static boolean isDirectory( String path )
		static void method_isDirectory( LContext& context ) ;
		// static String[] listFiles( String path )
		static void method_listFiles( LContext& context ) ;
		// static boolean deleteFile( String path )
		static void method_deleteFile( LContext& context ) ;
		// static boolean renameFile( String pathOld, String pathNew )
		static void method_renameFile( LContext& context ) ;
		// static boolean createDirectory( String path )
		static void method_createDirectory( LContext& context ) ;
		// static boolean deleteDirectory( String path )
		static void method_deleteDirectory( LContext& context ) ;
		// static File makeBuffer( long flags = File.modeRead, const void* buf = null, uint64 bytes = 0 )
		static void method_makeBuffer( LContext& context ) ;
		// static String pathDirectoryOf( String path )
		static void method_pathDirectoryOf( LContext& context ) ;
		// static String pathFileNameOf( String path )
		static void method_pathFileNameOf( LContext& context ) ;
		// static String pathFileTitleOf( String path )
		static void method_pathFileTitleOf( LContext& context ) ;
		// static String pathExtensionOf( String path )
		static void method_pathExtensionOf( LContext& context ) ;
		// static String catenatePath( String pathBase, String pathOffset, char deli = '/' )
		static void method_catenatePath( LContext& context ) ;

	} ;


	//////////////////////////////////////////////////////////////////////////
	// 出力ストリーム
	//////////////////////////////////////////////////////////////////////////

	class	LOutputStreamClass	: public LNativeObjClass
	{
	public:
		LOutputStreamClass
			( LVirtualMachine& vm,
				LClass * pClass, const wchar_t * pwszName )
			: LNativeObjClass( vm, vm.Global(), pClass, pwszName ) { }

		// クラス定義処理（ネイティブな実装）
		virtual void ImplementClass( void ) ;

	private:
		static const NativeFuncDesc		s_Virtuals[3] ;
		static const BinaryOperatorDesc	s_BinaryOps[5] ;
		static const ClassMemberDesc	s_MemberDesc ;

	public:
		// void writeString( String str )
		static void method_writeString( LContext& context ) ;
		// File getFile() const
		static void method_getFile( LContext& context ) ;

		// OutputStream operator << ( String str )
		static LValue::Primitive operator_sadd_str
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// OutputStream operator << ( long num )
		static LValue::Primitive operator_sadd_int
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;
		// OutputStream operator << ( double num )
		static LValue::Primitive operator_sadd_num
			( LValue::Primitive val1, LValue::Primitive val2, void * instance ) ;

	} ;

}

#endif

