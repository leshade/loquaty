@echo off
rem �T���v���Q�[�������s���܂�
rem ..\..\..\build\Loquaty\Loquaty.sln �̑S�v���W�F�N�g��
rem ..\..\..\build\EntisGLS4Plugin\EntisGLS4Plugin.sln ���r���h���Ă���

set "path=..\..\bin\win64;%path%"

loquaty.exe /I ..\..\bin\library AvoidanceFlight.lqs

pause
