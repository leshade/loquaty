@echo off
rem クラス・リファレンス・マニュアルを生成します

set "path=..\bin\win64;%path%"

loquaty.exe /doc_all /out .\ /I ..\bin\library makedoc.lqs

