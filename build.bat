@echo off
setlocal enabledelayedexpansion
where g++ >nul 2>nul
if errorlevel 1 (
    echo ERROR: g++ was not found. Install MinGW-w64 and add its bin folder to PATH.
    exit /b 1
)
set CORE=
for %%f in (src\core\*.cpp) do set CORE=!CORE! %%f
set CONSOLE=
for %%f in (src\console\*.cpp) do set CONSOLE=!CONSOLE! %%f
set WEB=
for %%f in (src\web\*.cpp) do set WEB=!WEB! %%f

echo Compiling console application...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc\core -Isrc\console !CORE! !CONSOLE! -o LibraryConsole.exe
if errorlevel 1 goto failed

echo Compiling web application...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc\core -Isrc\web !CORE! !WEB! -lws2_32 -o LibraryWeb.exe
if errorlevel 1 goto failed

echo.
echo Build successful.
echo   Website : .\LibraryWeb.exe     then open http://127.0.0.1:8080/
echo   Console : .\LibraryConsole.exe
exit /b 0

:failed
echo.
echo BUILD FAILED.
exit /b 1
