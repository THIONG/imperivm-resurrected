@echo off
rem Builds the installer stub as build\installer-stub.exe (x86). publish-version.js appends the release zip to it. Usage: installer\build.bat
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>nul
where cl >nul 2>nul || (echo The x86 C++ compiler was not found & exit /b 1)
cd /d "%~dp0.."
if not exist build\installer mkdir build\installer
set MZ=..\tools\miniz
if exist third_party\miniz\miniz.c set MZ=third_party\miniz
rc /nologo /Isrc /Iinstaller /fo build\installer\installer.res installer\installer.rc || exit /b 1
cl /nologo /c /O2 /MT /W0 /DNDEBUG /I%MZ% /Fobuild\installer\ %MZ%\miniz.c || exit /b 1
cl /nologo /c /O2 /MT /W3 /EHsc /std:c++17 /utf-8 /DNDEBUG /DUNICODE /D_UNICODE /I%MZ% /Fobuild\installer\ installer\installer.cpp || exit /b 1
link /nologo /SUBSYSTEM:WINDOWS /OUT:build\installer-stub.exe build\installer\*.obj build\installer\installer.res user32.lib gdi32.lib shell32.lib ole32.lib advapi32.lib comctl32.lib bcrypt.lib || exit /b 1
echo OK: build\installer-stub.exe
