@echo off

set BUILD_TYPE=Release

:: Reset build directory
rd /s /q build 2>nul
mkdir build & cd build

:: Configure
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..

:: Build
cmake --build . --config %BUILD_TYPE%

cd ..
