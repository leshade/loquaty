
#include <loquaty.h>

using namespace	Loquaty ;


//////////////////////////////////////////////////////////////////////////
// ファイル・ステート
//////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LFileStateStructure::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::VariableDesc		LFileStateStructure::s_Variables[7] =
{
	{
		L"fields", L"public", L"uint32", L"0", nullptr,
		L"<desc>取得した有効なファイル状態を表すフラグ集合<br/>\n"
		L"<div class=\"indent1\">stateHasAttribute : attributes にファイル属性を取得した</div>"
		L"<div class=\"indent1\">stateHasFileSize : fileSizeInBytes にファイルサイズを取得した</div>"
		L"<div class=\"indent1\">stateHasModifiedDate : dateModified に更新時刻を取得した</div>"
		L"<div class=\"indent1\">stateHasCreatedDate : dateCreated に作成日時を取得した</div>"
		L"<div class=\"indent1\">stateHasAccessedDate : dateAccessed にアクセス日時を取得した</div>"
		L"</desc>"
	},
	{
		L"attributes", L"public", L"uint32", L"0", nullptr,
		L"<desc>ファイル属性を表すフラグ集合<br/>\n"
		L"<div class=\"indent1\">attrDirectory : ディレクトリ</div>"
		L"<div class=\"indent1\">attrHidden : 隠しファイル</div>"
		L"<div class=\"indent1\">permissionXOTH : その他の実行権限</div>"
		L"<div class=\"indent1\">permissionWOTH : その他の書き込み権限</div>"
		L"<div class=\"indent1\">permissionROTH : その他の読み込み権限</div>"
		L"<div class=\"indent1\">permissionXGRP : グループの実行権限</div>"
		L"<div class=\"indent1\">permissionWGRP : グループの書き込み権限</div>"
		L"<div class=\"indent1\">permissionRGRP : グループの読み込み権限</div>"
		L"<div class=\"indent1\">permissionXUSR : ユーザーの実行権限</div>"
		L"<div class=\"indent1\">permissionWUSR : ユーザーの書き込み権限</div>"
		L"<div class=\"indent1\">permissionRUSR : ユーザーの読み込み権限</div>"
		L"</desc>"
	},
	{
		L"fileSizeInBytes", L"public", L"uint64", L"0", nullptr, L"ファイルサイズ"
	},
	{
		L"dateModified", L"public", L"DateTime", nullptr, nullptr, L"最終更新日時"
	},
	{
		L"dateCreated", L"public", L"DateTime", nullptr, nullptr, L"ファイル作成日時"
	},
	{
		L"dateAccessed", L"public", L"DateTime", nullptr, nullptr, L"最終アクセス日時"
	},
	{
		nullptr, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LFileStateStructure::s_MemberDesc =
{
	nullptr,
	nullptr,
	LFileStateStructure::s_Variables,
	nullptr,
	L"File.queryState 関数で取得できるファイルの状態を格納する構造体です。"
} ;




//////////////////////////////////////////////////////////////////////////
// ファイル
//////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LFileClass::ImplementClass( void )
{
	LBatchClassImplementor	bci( *this ) ;
	bci.AddClass
		( LPtr<LClass>
			( new LFileStateStructure
				( m_vm, LPtr<LNamespace>(),
					m_vm.GetClassClass(), L"State" ) ),
			m_vm.GetStructureClass() ) ;

	AddClassMemberDefinitions( s_MemberDesc ) ;

	bci.Implement() ;
}

const LClass::NativeFuncDesc	LFileClass::s_Virtuals[16] =
{
	{	// public File()
		LClass::s_Constructor,
		&LFileClass::method_init, false,
		L"public", L"void", L"",
		L"空の File オブジェクトを構築します。", nullptr
	},
	{	// public File( String path, long flags = modeRead )
		LClass::s_Constructor,
		&LFileClass::method_init2, false,
		L"public", L"void", L"String path, long flags = File.modeRead",
		L"ファイルを開いて File オブジェクトを構築します。\n"
		L"ファイルを開けなかった場合には IOException 例外が送出されます。\n"
		L"<param name=\"path\">開くファイルパス</param>\n"
		L"<param name=\"flags\">開くファイルのモードとフラグの組み合わせ\n"
		L"<div class=\"indent1\">modeRead : 読み込み用として開く</div>"
		L"<div class=\"indent1\">modeWrite : 書き込み用として開く</div>"
		L"<div class=\"indent1\">modeReadWrite : 読み書き両用として開く</div>"
		L"<div class=\"indent1\">modeCreate : ファイルを新規作成して書き込み用として開く</div>"
		L"<div class=\"indent1\">modeStreaming : ストリーミングの場合、メモリへのバッファを抑制する</div>"
		L"</param>", nullptr
	},
	{	// public boolean open( String path, long flags = modeRead )
		L"open",
		&LFileClass::method_oepn, false,
		L"public", L"boolean", L"String path, long flags = File.modeRead",
		L"ファイルを開きます。\n"
		L"<param name=\"path\">開くファイルパス</param>\n"
		L"<param name=\"flags\">開くファイルのモードとフラグの組み合わせ\n"
		L"<div class=\"indent1\">modeRead : 読み込み用として開く</div>"
		L"<div class=\"indent1\">modeWrite : 書き込み用として開く</div>"
		L"<div class=\"indent1\">modeReadWrite : 読み書き両用として開く</div>"
		L"<div class=\"indent1\">modeCreate : ファイルを新規作成して書き込み用として開く</div>"
		L"<div class=\"indent1\">modeStreamingFlag : ストリーミングの場合、メモリへのバッファを抑制する</div>"
		L"</param>\n"
		L"<return>成功した場合には true、失敗した場合には false</return>", nullptr
	},
	{	// public void close()
		L"close",
		&LFileClass::method_close, false,
		L"public", L"void", L"",
		L"ファイルを閉じます。", nullptr
	},
	{	// public uint64 read( void* buf, uint64 bytes )
		L"read",
		&LFileClass::method_read, false,
		L"public", L"uint64", L"void* buf, uint64 bytes",
		L"ファイルから読み込みます。\n"
		L"<param name=\"buf\">読み込んだデータを格納するメモリの先頭</param>\n"
		L"<param name=\"bytes\">読み込みたいバイト数。"
		L"バッファの有効なサイズを超えている場合にはクリップされます。</param>\n"
		L"<return>実際に読み込まれたバイト数。0 の場合にはファイルの終端。</return>", nullptr
	},
	{	// public uint64 write( const void* buf, uint64 bytes )
		L"write",
		&LFileClass::method_write, false,
		L"public", L"uint64", L"const void* buf, uint64 bytes",
		L"ファイルへ書き込みます。\n"
		L"<param name=\"buf\">書き込むデータを格納したメモリの先頭</param>\n"
		L"<param name=\"bytes\">書き込みたいバイト数。"
		L"バッファの有効なサイズを超えている場合にはクリップされます。</param>\n"
		L"<return>実際に書き込まれたバイト数</return>", nullptr
	},
	{	// public void flush()
		L"flush",
		&LFileClass::method_flush, false,
		L"public", L"void", L"",
		L"書き出しがバッファされている場合には実際に書き出します。", nullptr
	},
	{	// public void seek( long pos )
		L"seek",
		&LFileClass::method_seek, false,
		L"public", L"void", L"long pos",
		L"ファイルポインタを移動します。\n"
		L"<param name=\"pos\">新しいファイルポインタ</param>", nullptr
	},
	{	// public void skip( long bytes )
		L"skip",
		&LFileClass::method_skip, false,
		L"public", L"void", L"long bytes",
		L"ファイルポインタを現在の位置から移動します。\n"
		L"<param name=\"bytes\">ポインタを移動するバイト数。<br/>"
		L"プラスの場合には位置を進め、マイナスの場合には位置を戻します。</param>", nullptr
	},
	{	// public boolean isSeekable() const
		L"isSeekable",
		&LFileClass::method_isSeekable, false,
		L"public const", L"boolean", L"",
		L"シーク可能か判定します。", nullptr
	},
	{	// public long getLength() const
		L"getLength",
		&LFileClass::method_getLength, false,
		L"public const", L"long", L"",
		L"ファイルの全長を取得します。\n"
		L"<return>ファイルの全長（バイト単位）。ストリーム等取得できない場合には -1</return>", nullptr
	},
	{	// public long getPosition() const
		L"getPosition",
		&LFileClass::method_getPosition, false,
		L"public const", L"long", L"",
		L"現在のファイルポインタを取得します。\n"
		L"<return>ファイルポインタの位置。ストリーム等取得できない場合には -1</return>", nullptr
	},
	{	// public void truncate()
		L"truncate",
		&LFileClass::method_truncate, false,
		L"public", L"void", L"",
		L"ファイルを現在のファイルポインタの位置まで切り詰めます。", nullptr
	},
	{	// public OutputStream outStream( String enc = "utf-8" ) const
		L"outStream",
		&LFileClass::method_outStream, false,
		L"public", L"OutputStream", L"String enc = \"utf-8\"",
		L"ファイルへ出力する OutputStream を取得します。\n"
		L"<param name=\"enc\">文字エンコーディングを指定します。<br/>"
		L"<div class=\"indent1\">&quot;utf-8&quot; : UTF-8</div>"
		L"<div class=\"indent1\">&quot;utf-16&quot; : UTF-16</div>"
		L"<div class=\"indent1\">&quot;utf-32&quot; : UTF-32</div>"
		L"<div class=\"indent1\">null : デフォルトのエンコーディング</div>"
		L"</param>\n"
		L"<return>ファイルへ出力する OutputStream オブジェクト</return>", nullptr
	},
	{	// public String getFilePath() const
		L"getFilePath",
		&LFileClass::method_getFilePath, false,
		L"public", L"String", L"",
		L"開いているファイルのパスを取得します。\n"
		L"<return>開いているファイルのパス。<br/>"
		L"多くの場合コンストラクタや open 関数に渡したパスがそのまま返されます。"
		L"実際のファイルでない場合 null を返す場合もあることに注意してください。</return>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::NativeFuncDesc	LFileClass::s_Functions[10] =
{
	{	// public static boolean queryState( File.State* state, String path )
		L"queryState",
		&LFileClass::method_queryState, false,
		L"public static", L"boolean", L"File.State* state, String path",
		L"ファイルのステート情報を取得します。\n"
		L"<param name=\"state\">情報を受け取る File.State 構造体</param>\n"
		L"<param name=\"path\">対象のファイルパス</param>\n"
		L"<return>成功した場合には true</return>", nullptr
	},
	{	// public static boolean isExisting( String path )
		L"isExisting",
		&LFileClass::method_isExisting, false,
		L"public static", L"boolean", L"String path",
		L"ファイルが存在するか問い合わせます。\n"
		L"<param name=\"path\">対象のファイルパス</param>\n"
		L"<return>ファイル／ディレクトリが存在する場合には true</return>", nullptr
	},
	{	// public static boolean isDirectory( String path )
		L"isDirectory",
		&LFileClass::method_isDirectory, false,
		L"public static", L"boolean", L"String path",
		L"ディレクトリか問い合わせます。\n"
		L"<param name=\"path\">対象のファイルパス</param>\n"
		L"<return>対象がディレクトリが存在する場合には true</return>", nullptr
	},
	{	// public static String[] listFiles( String path )
		L"listFiles",
		&LFileClass::method_listFiles, false,
		L"public static", L"String[]", L"String path",
		L"指定ディレクトリ内の全てのファイル名とディレクトリ名を列挙します。\n"
		L"<param name=\"path\">対象のディレクトリパス</param>\n"
		L"<return>ディレクトリ内に存在するファイル名とディレクトリ名の配列。<br/>"
		L"各配列の要素はパスではなくディレクトリ内のファイル名</return>", nullptr
	},
	{	// public static boolean deleteFile( String path )
		L"deleteFile",
		&LFileClass::method_deleteFile, false,
		L"public static", L"boolean", L"String path",
		L"指定のファイルを削除します。\n"
		L"<param name=\"path\">対象のファイルパス</param>\n"
		L"<return>削除に成功した場合には true</return>", nullptr
	},
	{	// public static boolean renameFile( String pathOld, String pathNew )
		L"renameFile",
		&LFileClass::method_renameFile, false,
		L"public static", L"boolean", L"String pathOld, String pathNew",
		L"ファイル名を変更／移動します。\n"
		L"<param name=\"pathOld\">対象の元のファイルパス</param>\n"
		L"<param name=\"pathNew\">対象の変更後のファイルパス</param>\n"
		L"<return>変更に成功した場合には true</return>", nullptr
	},
	{	// public static boolean createDirectory( String path )
		L"createDirectory",
		&LFileClass::method_createDirectory, false,
		L"public static", L"boolean", L"String path",
		L"ディレクトリを作成します。デフォルトのスキーマでは途中のディレクトリが存在しない場合にも併せて作成されます。\n"
		L"<param name=\"path\">作成するディレクトリパス</param>\n"
		L"<return>作成に成功した場合には true</return>", nullptr
	},
	{	// public static boolean deleteDirectory( String path )
		L"deleteDirectory",
		&LFileClass::method_deleteDirectory, false,
		L"public static", L"boolean", L"String path",
		L"ディレクトリを削除します。デフォルトのスキーマではディレクトリ内にファイルが残っている場合しない場合には削除できません。\n"
		L"<param name=\"path\">削除するディレクトリパス</param>\n"
		L"<return>削除に成功した場合には true</return>", nullptr
	},
	{	// public static File makeBuffer( long flags = File.modeRead, const void* buf = null, uint64 bytes = 0 )
		L"makeBuffer",
		&LFileClass::method_makeBuffer, false,
		L"public static", L"File",
		L"long flags = File.modeRead, const void* buf = null, uint64 bytes = 0",
		L"実際のファイルではないメモリ上の File オブジェクトを作成します。\n"
		L"<param name=\"flags\">ファイルモードフラグ\n"
		L"<div class=\"indent1\">modeRead : 読み込み用として作成</div>"
		L"<div class=\"indent1\">modeWrite : 書き込み用として作成</div>"
		L"<div class=\"indent1\">modeReadWrite : 読み書き両用として作成</div></param>"
		L"<param name=\"buf\">File オブジェクトの初期イメージ</param>\n"
		L"<param name=\"bytes\">File オブジェクトの初期イメージのバイト数</param>\n"
		L"<return>作成された File オブジェクト</return>", nullptr
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LValue	LFileClass::s_VarInitValue[41] =
{
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeRead ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeWrite ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeReadWrite ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeCreate ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeCreateFlag ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeCreateDirsFlag ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeStreamingFlag ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeXOTH ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeWOTH ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeROTH ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeRWXO ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeXGRP ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeWGRP ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeRGRP ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeRWXG ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeXUSR ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeWUSR ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeRUSR ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modeRWXU ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::modePermissionMask ) ),

	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::stateHasAttribute ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::stateHasFileSize ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::stateHasModifiedDate ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::stateHasCreatedDate ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::stateHasAccessedDate ) ),

	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::attrDirectory ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::attrHidden ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionXOTH ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionWOTH ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionROTH ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionRWXO ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionXGRP ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionWGRP ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionRGRP ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionRWXG ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionXUSR ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionWUSR ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionRUSR ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionRWXU ) ),
	LValue( LType::typeInt64, LValue::MakeLong( LDirectory::permissionMask ) ),
} ;

const LClass::VariableDesc		LFileClass::s_Variables[41] =
{
	{
		L"modeRead", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[0],
		L"読み込み用として開く。"
	},
	{
		L"modeWrite", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[1],
		L"書き込み用として開く。"
	},
	{
		L"modeReadWrite", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[2],
		L"読み書き両用として開く。"
	},
	{
		L"modeCreate", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[3],
		L"ファイルを新規作成して書き出し用として開く。\n"
		L"ディレクトリが存在しない場合にはディレクトリも作成する。"
	},
	{
		L"modeCreateFlag", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[4],
		L"ファイルを新規作成して開く。"
	},
	{
		L"modeCreateDirsFlag", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[5],
		L"ディレクトリが存在しない場合にはディレクトリも作成する。"
	},
	{
		L"modeStreamingFlag", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[6],
		L"圧縮ストリームなどのストリーム形式で、"
		L"デフォルトでは一旦メモリにバッファするような場合にも、バッファリングせずに"
		L"ストリームとして開く。"
	},
	{
		L"modeXOTH", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[7],
		L"その他の実行権限。"
	},
	{
		L"modeWOTH", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[8],
		L"その他の書き込み権限。"
	},
	{
		L"modeROTH", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[9],
		L"その他の読み込み権限。"
	},
	{
		L"modeRWXO", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[10],
		L"その他の実行読み書き権限。"
	},
	{
		L"modeXGRP", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[11],
		L"グループの実行権限。"
	},
	{
		L"modeWGRP", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[12],
		L"グループの書き込み権限。"
	},
	{
		L"modeRGRP", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[13],
		L"グループの読み込み権限。"
	},
	{
		L"modeRWXG", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[14],
		L"グループの実行読み書き権限。"
	},
	{
		L"modeXUSR", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[15],
		L"ユーザーの実行権限。"
	},
	{
		L"modeWUSR", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[16],
		L"ユーザーの書き込み権限。"
	},
	{
		L"modeRUSR", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[17],
		L"ユーザーの読み込み権限。"
	},
	{
		L"modeRWXU", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[18],
		L"ユーザーの実行読み書き権限。"
	},
	{
		L"modePermissionMask", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[19],
		L"権限フラグマスク。"
	},

	{
		L"stateHasAttribute", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[20],
		L"File.State の attributes へ値を取得した。"
	},
	{
		L"stateHasFileSize", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[21],
		L"File.State の fileSizeInBytes へ値を取得した。"
	},
	{
		L"stateHasModifiedDate", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[22],
		L"File.State の dateModified へ値を取得した。"
	},
	{
		L"stateHasCreatedDate", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[23],
		L"File.State の dateCreated へ値を取得した。"
	},
	{
		L"stateHasAccessedDate", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[24],
		L"File.State の dateAccessed へ値を取得した。"
	},

	{
		L"attrDirectory", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[25],
		L"ディレクトリ属性フラグ。"
	},
	{
		L"attrHidden", L"public static const",
		L"uint32", nullptr, &LFileClass::s_VarInitValue[26],
		L"隠しファイル属性フラグ。"
	},
	{
		L"permissionXOTH", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[27],
		L"その他の実行権限。"
	},
	{
		L"permissionWOTH", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[28],
		L"その他の書き込み権限。"
	},
	{
		L"permissionROTH", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[29],
		L"その他の読み込み権限。"
	},
	{
		L"permissionRWXO", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[30],
		L"その他の実行読み書き権限。"
	},
	{
		L"permissionXGRP", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[31],
		L"グループの実行権限。"
	},
	{
		L"permissionWGRP", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[32],
		L"グループの書き込み権限。"
	},
	{
		L"permissionRGRP", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[33],
		L"グループの読み込み権限。"
	},
	{
		L"permissionRWXG", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[34],
		L"グループの実行読み書き権限。"
	},
	{
		L"permissionXUSR", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[35],
		L"ユーザーの実行権限。"
	},
	{
		L"permissionWUSR", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[36],
		L"ユーザーの書き込み権限。"
	},
	{
		L"permissionRUSR", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[37],
		L"ユーザーの読み込み権限。"
	},
	{
		L"permissionRWXU", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[38],
		L"ユーザーの実行読み書き権限。"
	},
	{
		L"permissionMask", L"public static const",
		L"long", nullptr, &LFileClass::s_VarInitValue[39],
		L"権限フラグマスク。"
	},

	{
		nullptr, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LFileClass::s_MemberDesc =
{
	LFileClass::s_Virtuals,
	LFileClass::s_Functions,
	LFileClass::s_Variables,
	nullptr,
	L"ファイルを保持、またファイルシステムへのインターフェースを備えたクラスです。"
} ;


// File()
void LFileClass::method_init( LContext& _context )
{
}

// File( String path, long flags )
void LFileClass::method_init2( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_STRING( path ) ;
	LQT_FUNC_ARG_LONG( flags ) ;

	LFilePtr	file = LURLSchemer::Open( path.c_str(), (long) flags ) ;
	if ( file == nullptr )
	{
		_context.ThrowException( exceptionCannotOpenFile ) ;
		return ;
	}
	pNObj->SetNative( file ) ;

	LQT_RETURN_VOID() ;
}

// boolean oepn( String path, long flags )
void LFileClass::method_oepn( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_STRING( path ) ;
	LQT_FUNC_ARG_LONG( flags ) ;

	LFilePtr	file = LURLSchemer::Open( path.c_str(), (long) flags ) ;
	if ( file == nullptr )
	{
		LQT_RETURN_BOOL( false ) ;
	}
	else
	{
		pNObj->SetNative( file ) ;
		LQT_RETURN_BOOL( true ) ;
	}
}

// void close()
void LFileClass::method_close( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LNativeObj::SetNativeObject( pNObj.Ptr(), nullptr ) ;

	LQT_RETURN_VOID() ;
}

// uint64 read( void* buf, uint64 bytes )
void LFileClass::method_read( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, buf ) ;
	LQT_FUNC_ARG_LONG( bytes ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile == nullptr )
	{
		LQT_RETURN_LONG( 0 ) ;
	}

	size_t	nBytes = std::min( (size_t) bytes, buf->GetBytes() ) ;
	nBytes = pfile->Read( buf->GetPointer( 0, nBytes ), nBytes ) ;

	LQT_RETURN_LONG( (LLong) nBytes ) ;
}

// uint64 write( const void* buf, uint64 bytes )
void LFileClass::method_write( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, buf ) ;
	LQT_FUNC_ARG_LONG( bytes ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile == nullptr )
	{
		LQT_RETURN_LONG( 0 ) ;
	}

	size_t	nBytes = std::min( (size_t) bytes, buf->GetBytes() ) ;
	nBytes = pfile->Write( buf->GetPointer( 0, nBytes ), nBytes ) ;

	LQT_RETURN_LONG( (LLong) nBytes ) ;
}

// void flush()
void LFileClass::method_flush( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile != nullptr )
	{
		pfile->Flush() ;
	}
	LQT_RETURN_VOID() ;
}

// void seek( long pos )
void LFileClass::method_seek( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_LONG( pos ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile != nullptr )
	{
		pfile->Seek( pos ) ;
	}
	LQT_RETURN_VOID() ;
}

// void skip( long bytes )
void LFileClass::method_skip( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_LONG( bytes ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile != nullptr )
	{
		pfile->Skip( bytes ) ;
	}
	LQT_RETURN_VOID() ;
}

// boolean isSeekable() const
void LFileClass::method_isSeekable( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	LQT_RETURN_BOOL( (pfile != nullptr) && pfile->IsSeekable() ) ;
}

// long getLength() const
void LFileClass::method_getLength( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	LQT_RETURN_LONG( (pfile != nullptr) ? pfile->GetLength() : -1 ) ;
}

// long getPosition() const
void LFileClass::method_getPosition( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	LQT_RETURN_LONG( (pfile != nullptr) ? pfile->GetPosition() : -1 ) ;
}

// void truncate()
void LFileClass::method_truncate( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile != nullptr )
	{
		pfile->Truncate() ;
	}
	LQT_RETURN_VOID() ;
}

// OutputStream outStream( String enc = "utf-8" ) const
void LFileClass::method_outStream( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;
	LQT_FUNC_ARG_STRING( enc ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile == nullptr )
	{
		LQT_RETURN_OBJECT( nullptr ) ;
	}

	LPtr<LNativeObj>
		pOutStream( new LNativeObj( LQT_GET_CLASS(OutputStream) ) ) ;
	if ( enc == L"utf-8" )
	{
		pOutStream->SetNative
			( std::make_shared<LOutputUTF8Stream>( pfile ) ) ;
	}
	else if ( enc == L"utf-16" )
	{
		pOutStream->SetNative
			( std::make_shared<LOutputUTF16Stream>( pfile ) ) ;
	}
	else if ( enc == L"utf-32" )
	{
		pOutStream->SetNative
			( std::make_shared<LOutputUTF32Stream>( pfile ) ) ;
	}
	else
	{
		pOutStream->SetNative
			( std::make_shared<LOutputStringStream>( pfile ) ) ;
	}

	LQT_RETURN_OBJECT( pOutStream ) ;

}

// String getFilePath() const
void LFileClass::method_getFilePath( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_OBJ( LNativeObj, pNObj ) ;

	LFilePtr	pfile = pNObj->Get<LPureFile>() ;
	if ( pfile == nullptr )
	{
		LQT_RETURN_STRING( L"" ) ;
	}
	LQT_RETURN_STRING( pfile->GetFilePath() ) ;
}

// static boolean queryState( State state, String path )
void LFileClass::method_queryState( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRUCT( LDirectory::State, pState ) ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.QueryFileState( *pState, path.c_str() ) ) ;
}

// static boolean isExisting( String path )
void LFileClass::method_isExisting( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.IsExisting( path.c_str() ) ) ;
}

// static boolean isDirectory( String path )
void LFileClass::method_isDirectory( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.IsDirectory( path.c_str() ) ) ;
}

// static String[] listFiles( String path )
void LFileClass::method_listFiles( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( path ) ;

	std::vector<LString>	files ;
	LURLSchemer::s_schemer.ListFiles( files, path.c_str() ) ;

	LPtr<LArrayObj>
		pArray( _context.new_Array( _context.VM().GetStringClass() ) ) ;
	for ( size_t i = 0; i < files.size(); i ++ )
	{
		pArray->m_array.push_back
			( LObjPtr( _context.new_String( files.at(i) ) ) ) ;
	}
	LQT_RETURN_OBJECT( pArray ) ;
}

// static boolean deleteFile( String path )
void LFileClass::method_deleteFile( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.DeleteFile( path.c_str() ) ) ;
}

// static boolean renameFile( String pathOld, String pathNew )
void LFileClass::method_renameFile( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( pathOld ) ;
	LQT_FUNC_ARG_STRING( pathNew ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.RenameFile
				( pathOld.c_str(), pathNew.c_str() ) ) ;
}

// static boolean createDirectory( String path )
void LFileClass::method_createDirectory( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.CreateDirectory( path.c_str() ) ) ;
}

// static boolean deleteDirectory( String path )
void LFileClass::method_deleteDirectory( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_STRING( path ) ;

	LQT_RETURN_BOOL
		( LURLSchemer::s_schemer.DeleteDirectory( path.c_str() ) ) ;
}

// static File makeBuffer( long flags = File.modeRead, const void* buf = null, uint64 bytes = 0 )
void LFileClass::method_makeBuffer( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_ARG_LONG( flags ) ;
	LQT_FUNC_ARG_OBJECT( LPointerObj, pBuf ) ;
	LQT_FUNC_ARG_ULONG( bytes ) ;

	std::shared_ptr<LArrayBufStorage>	buf ;
	if ( pBuf != nullptr )
	{
		size_t	nBytes = std::min( (size_t) bytes, pBuf->GetBytes() ) ;
		buf = std::make_shared<LArrayBufStorage>
					( pBuf->GetPointer( 0, nBytes ), nBytes ) ;
	}
	else
	{
		buf = std::make_shared<LArrayBufStorage>() ;
	}

	std::shared_ptr<LBufferedFile>
			pFile = std::make_shared<LBufferedFile>( (long) flags, buf ) ;

	LPtr<LNativeObj>	pFileObj( new LNativeObj( LQT_GET_CLASS(File) ) ) ;
	pFileObj->SetNative( pFile ) ;

	LQT_RETURN_OBJECT( pFileObj ) ;
}




//////////////////////////////////////////////////////////////////////////////
// 出力ストリーム
//////////////////////////////////////////////////////////////////////////////

// クラス定義処理（ネイティブな実装）
void LOutputStreamClass::ImplementClass( void )
{
	AddClassMemberDefinitions( s_MemberDesc ) ;
}

const LClass::NativeFuncDesc		LOutputStreamClass::s_Virtuals[3] =
{
	{	// public void writeString( String str )
		L"writeString",
		&LOutputStreamClass::method_writeString, false,
		L"public", L"void", L"String str",
		L"ファイルへ文字列を出力します。\n"
		L"文字列は所定の文字コードにエンコードされます。", nullptr,
	},
	{	// public File getFile() const
		L"getFile",
		&LOutputStreamClass::method_getFile, false,
		L"public const", L"File", L"",
		L"ストリームの出力先のファイルを取得します。", nullptr,
	},
	{
		nullptr, nullptr, false,
		nullptr, nullptr, nullptr, nullptr, nullptr
	},
} ;

const LClass::BinaryOperatorDesc	LOutputStreamClass::s_BinaryOps[5] =
{
	{	// OutputStream operator << ( String str )
		L"<<", &LOutputStreamClass::operator_sadd_str, false, false,
		L"OutputStream", L"String str",
		L"ファイルへ文字列を出力します。", nullptr,
	},
	{	// OutputStream operator << ( Object obj )
		L"<<", &LOutputStreamClass::operator_sadd_str, false, false,
		L"OutputStream", L"Object obj",
		L"obj.toString() をファイルへ出力します。", nullptr,
	},
	{	// OutputStream operator << ( long num )
		L"<<", &LOutputStreamClass::operator_sadd_int, false, false,
		L"OutputStream", L"long num",
		L"long を文字列化しファイルへ出力します。", nullptr,
	},
	{	// OutputStream operator << ( double num )
		L"<<", &LOutputStreamClass::operator_sadd_num, false, false,
		L"OutputStream", L"double num",
		L"double を文字列化しファイルへ出力します。", nullptr,
	},
	{
		nullptr, nullptr, false, false, nullptr, nullptr, nullptr, nullptr,
	},
} ;

const LClass::ClassMemberDesc	LOutputStreamClass::s_MemberDesc =
{
	LOutputStreamClass::s_Virtuals,
	nullptr,
	nullptr,
	LOutputStreamClass::s_BinaryOps,
	L"OutputStream クラスはファイルへ文字列をエンコードして出力するクラスです。",
} ;

// void writeString( String str )
void LOutputStreamClass::method_writeString( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LOutputStream, strm ) ;
	LQT_FUNC_ARG_STRING( str ) ;

	strm->WriteString( str ) ;

	LQT_RETURN_VOID() ;
}

// File getFile() const
void LOutputStreamClass::method_getFile( LContext& _context )
{
	LQT_FUNC_ARG_LIST ;
	LQT_FUNC_THIS_NOBJ( LOutputStream, strm ) ;

	LPtr<LNativeObj>	pFileObj( new LNativeObj( LQT_GET_CLASS(File) ) ) ;
	pFileObj->SetNative( strm->GetFile() ) ;

	LQT_RETURN_OBJECT( pFileObj ) ;
}

// OutputStream operator << ( String str )
LValue::Primitive LOutputStreamClass::operator_sadd_str
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	std::shared_ptr<LOutputStream>
			strm = LNativeObj::GetNative<LOutputStream>( val1.pObject ) ;
	if ( strm == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	if ( val2.pObject != nullptr )
	{
		LString	str ;
		if ( val2.pObject->AsString( str ) )
		{
			strm->WriteString( str ) ;
		}
	}
	val1.pObject->AddRef() ;
	return	LValue::MakeObjectPtr( val1.pObject ) ;
}

// OutputStream operator << ( long num )
LValue::Primitive LOutputStreamClass::operator_sadd_int
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	std::shared_ptr<LOutputStream>
			strm = LNativeObj::GetNative<LOutputStream>( val1.pObject ) ;
	if ( strm == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	*strm << (long) val2.longValue ;

	val1.pObject->AddRef() ;
	return	LValue::MakeObjectPtr( val1.pObject ) ;
}

// OutputStream operator << ( double num )
LValue::Primitive LOutputStreamClass::operator_sadd_num
	( LValue::Primitive val1, LValue::Primitive val2, void * instance )
{
	std::shared_ptr<LOutputStream>
			strm = LNativeObj::GetNative<LOutputStream>( val1.pObject ) ;
	if ( strm == nullptr )
	{
		LContext::ThrowExceptionError( exceptionNullPointer ) ;
		return	LValue::MakeObjectPtr( nullptr ) ;
	}
	*strm << (double) val2.dblValue ;

	val1.pObject->AddRef() ;
	return	LValue::MakeObjectPtr( val1.pObject ) ;
}



