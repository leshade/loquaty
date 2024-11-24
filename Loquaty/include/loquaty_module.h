
#ifndef	__LOQUATY_MODULE_H__
#define	__LOQUATY_MODULE_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 抽象モジュール
	//////////////////////////////////////////////////////////////////////////

	class	LModule	: public Object
	{
	public:
		// 仮想マシンにクラス群を追加する
		virtual void ImportTo( LVirtualMachine& vm ) = 0 ;

	} ;

	typedef	std::shared_ptr<LModule>	LModulePtr ;



	//////////////////////////////////////////////////////////////////////////
	// モジュール・プロデューサー
	//////////////////////////////////////////////////////////////////////////

	class	LModuleProducer	: public Object
	{
	public:
		// 名前を受け取って対応する LModulePtr を返す関数
		// 知らない名前なら nullptr を返す
		typedef	std::function<LModulePtr(const wchar_t*)>	Producer ;

	protected:
		// 未知のモジュールを取得するプロデューサー
		std::vector<Producer>				m_producers ;

		// 既知のモジュール
		std::map<std::wstring,LModulePtr>	m_mapModules ;

		std::mutex							m_mutex ;

	public:
		// モジュールを取得する
		virtual LModulePtr GetModule( const wchar_t * pwszName ) ;
		// モジュールを生成／取得する
		virtual LModulePtr ProduceModule( const wchar_t * pwszName ) ;

		// プロデューサーを追加する
		void AddProducer( Producer producer ) ;
		void AddProducer( std::shared_ptr<LModuleProducer> producer ) ;

		// 複製
		const LModuleProducer& operator = ( const LModuleProducer& mp ) ;
	} ;



	//////////////////////////////////////////////////////////////////////////
	// プラグイン・モジュール・プロデューサー
	//////////////////////////////////////////////////////////////////////////

	class	LPluginModuleProducer	: public LModuleProducer
	{
	protected:
		LString	m_strPluginPath ;

	public:
		LPluginModuleProducer( const wchar_t * pwszPath = nullptr )
			: m_strPluginPath( pwszPath ) {}

		// モジュールを生成／取得する
		virtual LModulePtr ProduceModule( const wchar_t * pwszName ) ;
	} ;



	//////////////////////////////////////////////////////////////////////////
	// プラグイン・エクスポート・モジュール
	//////////////////////////////////////////////////////////////////////////

	class	LPluginExporter	: public LModule
	{
	private:
		std::vector<LModulePtr>	m_modules ;

	public:
		// モジュール追加
		void AddModule( LModulePtr module ) ;

		// 仮想マシンにクラス群を追加する
		virtual void ImportTo( LVirtualMachine& vm ) ;

	public:
		// シングルトン（DLL 側用：DLL 自体がインスタンスみたいなものなので）
		static LPluginExporter	m_exporter ;

	public:
		// DLL エントリポイントの DLL_PROCESS_ATTACH 等で追加しておく
		static void DLLAddModule( LModulePtr module ) ;

	} ;


}



#endif

