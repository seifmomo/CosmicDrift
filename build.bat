@echo off
echo.
echo === Building Cosmic Drift ===
echo.
g++ -o CosmicDrift.exe main.cpp -lfreeglut -lopengl32 -lglu32 -lm
if %ERRORLEVEL% EQU 0 (
    echo BUILD SUCCESSFUL!
    echo Run: CosmicDrift.exe
) else (
    echo BUILD FAILED!
)
pause
