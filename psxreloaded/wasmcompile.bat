@echo off
setlocal EnableDelayedExpansion

set SOURCES=
set PRELOAD=

rem ============================================================================
rem Source files
rem ============================================================================

for %%D in (
    src
    src\core
    src\GUI
    src\platform
    src\psx
    src\psx-utils
    src\xgui\XGUI
    src\xgui\XGUI\impl
) do (
    for %%F in (%%D\*.cpp) do (
        set SOURCES=!SOURCES! "%%F"
    )
)

rem ============================================================================
rem Assets
rem ============================================================================

rem GLSL (GLES3) shaders
for /R shaders\gles3 %%F in (*.glsl) do (
    set PRELOAD=!PRELOAD! --preload-file "%%F"
)

rem for %%F in (*.bin) do (
rem    set PRELOAD=!PRELOAD! --preload-file "%%F"
rem )

echo.
echo Building...
echo.

em++ ^
!SOURCES! ^
src\platform\platform_audio.c ^
src\platform\ControllerInput.c ^
-Isrc ^
-Ilib\include ^
-std=c++17 ^
-O3 ^
-sUSE_WEBGL2=1 ^
-sFULL_ES3=1 ^
-sALLOW_MEMORY_GROWTH=1 ^
-sAUDIO_WORKLET=1 ^
-sWASM_WORKERS=1 ^
-DPSXRELOADED_WASM_AUDIO=1 ^
!PRELOAD! ^
--preload-file "CascadiaCode-Medium.ttf" ^
--preload-file "SCPH1001.bin" ^
-DRELEASE ^
--shell-file wasm\shell.html ^
-o wasm\psxreloaded.html

if errorlevel 1 (
    echo.
    echo Build FAILED.
    exit /b 1
)

echo.
echo Build succeeded.