﻿
#include <loquaty.h>
#include <loquaty_lib.h>

using namespace Loquaty ;



//////////////////////////////////////////////////////////////////////////////
// Loquaty アプリケーション
//////////////////////////////////////////////////////////////////////////////

class	LoquatyApp
{
private:
	enum	Verb
	{
		verbNo,
		verbHelp,
		verbRun,
		verbDumpFunc,
		verbMakeDocClass,
		verbMakeDocPackage,
		verbMakeDocAll,
		verbMakeCppStub,
	} ;
	Verb					m_verb ;
	bool					m_optNologo ;
	LString					m_strDumpFunc ;
	LString					m_strMakeOutput ;
	LString					m_strMakeTarget ;
	LString					m_strCppClass ;
	LString					m_strSourceFile ;
	std::vector<LString>	m_argsScript ;

	LPtr<LVirtualMachine>	m_vm ;
	int						m_warnLevel ;

public:
	LoquatyApp( void ) ;
	~LoquatyApp( void ) ;

	// コマンドライン解釈
	int ParseCmdLine( int argc, char* argv[], char* envp[] ) ;
	void AddEnvIncludePath( char* envp[] ) ;

	// 実行
	int Run( void ) ;

	// ロゴ表示
	void PrintLogo( void ) ;

	// ヘルプ表示
	void PrintHelp( void ) ;

	// ソースを読み込んでコンパイルする
	int LoadSource( void ) ;

	// main 関数を実行する
	int RunMain( void ) ;

	// 関数をダンプする
	int DumpFunction( void ) ;
	int DumpFunction( LPtr<LFunctionObj> pFunc ) ;

	// クラスを文書化する
	int MakeDocClass( void ) ;

	LString MakeClassDocFileName( LClass * pClass, LClass * pFromClass ) ;
	LString MakeClassDocFileDir( LClass * pClass, LClass * pFromClass ) ;
	std::shared_ptr<LOutputStream> OpenClassDocFile( LClass * pClass ) ;

	int MakeDocClass( LOutputStream& strm, LClass * pClass ) ;
	LString MakeDocSuperClass
		( LOutputStream& strm, LClass * pClass, LClass * pFromClass ) ;

	void MakeDocVariableList
		( LOutputStream& strm, const wchar_t * pwszBase,
			LObjPtr pVar, const LArrangementBuffer& arrange ) ;
	void MakeDocVariableDesc
		( LOutputStream& strm, const wchar_t * pwszBase,
			LObjPtr pVar, const LArrangementBuffer& arrange ) ;
	void MakeDocVariableDesc
		( LOutputStream& strm, const wchar_t * pwszName,
			const LType& typeVar, LObjPtr pVarInit ) ;
	void MakeDocFunctionDesc
		( LOutputStream& strm, LPtr<LFunctionObj> pFunc ) ;
	void MakeDocBinaryOperatorDesc
		( LOutputStream& strm, const Symbol::BinaryOperatorDef& bodef ) ;

	LString MakeTypeModifiers( LType::Modifiers modifiers ) ;

	void MakeComment( LType::LComment& comment ) ;
	bool HasCommentSummary( LType::LComment& comment ) ;
	void MakeDocXMLSummary( LOutputStream& strm, LType::LComment& comment ) ;
	void MakeDocXMLParams( LOutputStream& strm, LType::LComment& comment ) ;
	void MakeDocXMLDescription( LOutputStream& strm, LType::LComment& comment ) ;

	// css ファイルを出力する
	bool WriteCssFile( const wchar_t * pwszFilePath ) ;

	// パッケージに含まれるクラスのインデックスを文書化する
	int MakeDocIndexInPackage( void ) ;
	int MakeDocIndexInPackage( LPackagePtr pPackage ) ;
	int MakeDocClassesInPackage
		( LOutputStream& strm, LPackagePtr pPackage, bool flagRootIndex ) ;

	// 全てのパッケージのクラスを文書化する
	int MakeDocAllPackages( void ) ;

	// クラスの native 関数宣言・実装のテンプレートを出力する
	int MakeNativeFuncStubClass( void ) ;
	int MakeNativeFuncStubClass
		( LOutputStream& osHeader, LOutputStream& osCpp,
			LClass * pClass, const wchar_t * pwszCppClass ) ;
	int MakeNativeFunctionStub
		( LOutputStream& osHeader, LOutputStream& osCpp,
			LPtr<LFunctionObj> pFunc, const wchar_t * pwszCppClass ) ;
	void OutputStubFuncArgList
		( LOutputStream& osCpp, LPtr<LFunctionObj> pFunc ) ;

	static bool IsNativeClass( LClass * pClass ) ;
	static LString GetPrimitiveTypeName( LType::Primitive type ) ;
	static LString MakeStubFunctionName( const LString& strFuncName, int& nExit ) ;

} ;



//////////////////////////////////////////////////////////////////////////////
// エラーを標準出力するコンパイラ
//////////////////////////////////////////////////////////////////////////////

class	CLICompiler	: public LCompiler
{
public:
	CLICompiler( LVirtualMachine& vm, LStringParser * src = nullptr )
		: LCompiler( vm, src ) { }

	// 文字列出力
	virtual void PrintString( const LString& str ) ;

} ;



