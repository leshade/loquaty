@echo off

@echo PATH=%~dp0bin\win64;%PATH%
@echo LOQUATY_HOME=%~dp0..
@echo LOQUATY_INCLUDE_PATH=%~dp0bin\library

@echo 環境変数を更新します。

set /P YN=よろしいですか？ (Y/N) 

if "%YN%" == "Y" goto :LabelSetx
if "%YN%" == "y" goto :LabelSetx

@echo 環境変数は更新しませんでした。
goto :LabelExit

:LabelSetx

call :strlen

if %len% GEQ 1024 (
	echo 1024 文字を超えるため PATH は更新しません。
) else (
	setx PATH "%~dp0bin\win64;%PATH%"
)
@echo PATH=%PATH%

setx LOQUATY_HOME "%~dp0.."
@echo LOQUATY_HOME=%~dp0..

setx LOQUATY_INCLUDE_PATH "%~dp0bin\library"
@echo LOQUATY_INCLUDE_PATH=%~dp0bin\library

@echo 環境変数を更新しました。
goto :LabelExit


:strlen
	set len=0
	set "temp=%~dp0bin\win64;%PATH%"

	:label_loop
		set /a len=len+1
		set "temp=%temp:~1%"
		if not "%temp%"=="" goto :label_loop

	exit /b


:LabelExit
pause
