@echo off
echo Building Coursework Project (Debug x64)...
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0GraphicsTutorials.sln" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo /t:Rebuild
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Executable: %~dp0x64\Debug\Coursework.exe
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    exit /b %ERRORLEVEL%
)
