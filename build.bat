@echo off
cd /d "%~dp0"
echo Building OpenGL 2D Physics Simulator...
g++ -std=c++17 -O2 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8 -mwindows ^
    src\main.cpp ^
    src\app.cpp ^
    src\scene.cpp ^
    src\physics.cpp ^
    src\renderer.cpp ^
    src\ui.cpp ^
    src\function1d.cpp ^
    -lopengl32 -lgdi32 -luser32 -lcomctl32 -lcomdlg32 ^
    -o physics_sim.exe 2>&1
if %errorlevel% == 0 (
    echo Build successful! Run physics_sim.exe
) else (
    echo Build failed.
)
pause
