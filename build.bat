@echo off
g++ -O2 -DFREEGLUT_STATIC -Wall -Iinclude AudioEngine.cpp Starfield.cpp Planet.cpp Spacecraft.cpp PostProcess.cpp SceneDirector.cpp Scene1.cpp Scene2.cpp Scene3.cpp main.cpp -Llib -lfreeglut_static -lopengl32 -lglu32 -lwinmm -lgdi32 -static -s -o Martian.exe
if %ERRORLEVEL% EQU 0 (
    copy /Y Martian.exe bin\Release\Martian.exe >nul
    copy /Y Martian.exe bin\Debug\Martian.exe >nul
    echo BUILD SUCCESSFUL
) else (
    echo BUILD FAILED
)
