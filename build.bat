@echo off

REM  Build script for graph_sandbox using g++
REM  Make sure you have SDL2 and SDL2_ttf installed and properly linked
g++ -std=c++23 -O2 main.cpp -o graph_sandbox.exe ^
    -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

IF ERRORLEVEL 1 (
    echo Build unsuccessful. Please check for errors in the output above.
    pause
    exit /b 1
)

echo Build successful: graph_sandbox.exe
pause