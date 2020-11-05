@echo off
set workingDir=C:\Projects\diana\build
SET build = -b

cmake -G "Visual Studio 15 2017 Win64" -S "./src" -B "./build"

pushd %workingDir%
if %1%==build (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    msbuild.exe Diana.sln /property:Configuration=Debug
)
popd