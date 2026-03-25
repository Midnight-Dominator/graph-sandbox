@echo off
REM C:\Basic Code\Graph Sandbox

REM 1) Cập nhật theo đường dẫn include/lib của bạn
set SDL2_INCLUDE=C:\Basic Code\Graph Sandbox\source\include\SDL2
set SDL2_LIB=C:\Basic Code\Graph Sandbox\source\lib

REM 2) Bản build debug/release
g++ -std=c++23 -O2 main.cpp -I"%SDL2_INCLUDE%" -L"%SDL2_LIB%" -o graph_sandbox.exe ^
    -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

IF ERRORLEVEL 1 (
    echo Bị lỗi build!
    pause
    exit /b 1
)

echo Build thành công: network_guardian.exe
pause