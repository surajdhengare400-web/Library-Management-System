@echo off
setlocal enabledelayedexpansion
where g++ >nul 2>nul
if errorlevel 1 (
    echo ERROR: g++ was not found. Install MinGW-w64 and add its bin folder to PATH.
    exit /b 1
)
set CORE=
for %%f in (src\core\*.cpp) do set CORE=!CORE! %%f
if not exist build mkdir build

echo Compiling core tests...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc\core -Isrc\console tests\test_core.cpp !CORE! src\console\Console.cpp src\console\ReportManager.cpp -o build\test_core.exe
if errorlevel 1 goto failed
echo Compiling web tests...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc\core -Isrc\web tests\test_web.cpp !CORE! src\web\HttpServer.cpp src\web\Html.cpp src\web\WebApp.cpp src\web\WebCatalog.cpp -lws2_32 -o build\test_web.exe
if errorlevel 1 goto failed

echo.
build\test_core.exe
if errorlevel 1 goto failed
build\test_web.exe
if errorlevel 1 goto failed
echo.
echo All tests passed.
exit /b 0

:failed
echo.
echo FAILED.
exit /b 1
