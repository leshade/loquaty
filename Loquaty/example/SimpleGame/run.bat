@echo off
rem サンプル簡易ゲームを実行します
rem ..\..\..\build\Loquaty\Loquaty.sln の全プロジェクトをバッチビルドしてから

set "path=..\..\bin\win64;%path%"

loquaty.exe /I ..\..\bin\library SimpleGame.lqs

