@echo off
set workingDir=C:\Projects\diana\build
SET build = -b
SET run = -r

cmake -G "Visual Studio 16 2019" -S "./src" -B "./build"

pushd %workingDir%
if %1%==build (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    msbuild.exe Diana.sln /property:Configuration=Debug
)
else if %1%==run (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    msbuild.exe Diana.sln /property:Configuration=Debug
    devenv Diana.sln
)
popd