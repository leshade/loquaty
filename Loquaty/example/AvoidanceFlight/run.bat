@echo off
rem サンプルゲームを実行します
rem ..\..\..\build\Loquaty\Loquaty.sln の全プロジェクトと
rem ..\..\..\build\EntisGLS4Plugin\EntisGLS4Plugin.sln をビルドしてから

set "path=..\..\bin\win64;%path%"

loquaty.exe /I ..\..\bin\library AvoidanceFlight.lqs

pause
