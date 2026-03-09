@echo off
cd /d "%~dp0"
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" GraphicsTutorials.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo /t:Rebuild
