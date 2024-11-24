
# Loquaty, the language for game scripts

Loquaty は汎用的なスクリプト言語で、「ポインタ型のある Java」、「const のある Java」、「演算子をオーバーロードできる Java」、「try 文に finally 句のある C++」、「安全なポインタしかない C++」等々、他の言語との差異を挙げることはできますが、ゲーム用のスクリプト言語として開発されたその目的のために特に以下の特徴を有します。

* 「軽量で」「安全な」軽量スレッド
* C++ との（比較的容易な）インターフェース
* ポータブル性
* そこそこ高速

詳細は、  
⇒ [Loquaty 言語マニュアル](./Loquaty/doc/manual.xhtml)  
⇒ [クラス・リファレンス・マニュアル](./Loquaty/doc/index.xhtml)  


## ファイルとディレクトリ

- `README.md` - このファイル
- `LICENSE` - ライセンス情報
- `build/Loquaty/Loquaty.sln` - Visual Studio 用ソリューション
- [`Loquaty/doc/manual.xhtml`](./Loquaty/doc/manual.xhtml) - 言語マニュアル
- [`Loquaty/doc/index.xhtml`](./Loquaty/doc/index.xhtml) - クラス・リファレンス・マニュアル
- `Loquaty/include/` - C++ 用ヘッダファイル
- `Loquaty/source/` - C++ 用ソースファイル
- `Loquaty/library/win32/` - Win32 用ライブラリ・ビルド出力ディレクトリ
- `Loquaty/library/win64/` - Win64 用ライブラリ・ビルド出力ディレクトリ
- `Loquaty/bin/library/` - Loquaty 用インクルードファイル (LOQUATY_INCLUDE_PATH 環境変数設定推奨)
- `Loquaty/bin/win32/loquaty.exe` - Win32 用 loquaty.exe ビルド出力先
- `Loquaty/bin/win32/plugins/` - Win32 loquaty.exe 用プラグイン・ディレクトリ
- `Loquaty/bin/win64/loquaty.exe` - Win64 用 loquaty.exe ビルド出力先
- `Loquaty/bin/win64/plugins/` - Win64 loquaty.exe 用プラグイン・ディレクトリ
- `Loquaty/example/SimpleGame/SimpleGame.lqs` - サンプル簡易ゲーム・スクリプト
- `Loquaty/example/SimpleGame/run.bat` - サンプル簡易ゲーム実行用 bat ファイル (Win64 用)


## 使い方 (C++)

インクルード・ディレクトリに `Loquaty/include/` , ライブラリ・ディレクトリに `Loquaty/library/win64/` または `Loquaty/library/win32/` を追加し、

```C++
#include <loquaty.h>
#include <loquaty_lib.h>

using namespace Loquaty ;
```

をインクルード

```C++
LPtr<LVirtualMachine> vm = new LVirtualMachine ;
vm->Initialize() ;

// スクリプト 'script_file.lqs' 読み込み
LCompiler compiler( *vm ) ;
compiler.IncludeScript( L"script_file.lqs" ) ;

// 関数 funcName() 実行
LPtr<LThreadObj> thread = new LThreadObj( vm->GetThreadClass() ) ;
auto [value, exception] =
    thread.SyncCallFunctionAs( nullptr, L"funcName", nullptr, 0 ) ;
```

で実行できます。

スクリプトのインクルードパスの設定や、あるいは独自の書庫や圧縮ファイルなどから入力することもできます。

詳細は `build/LoquatyCLI` (loquaty.exe) や `build/ExamplePlugin` を参照。

