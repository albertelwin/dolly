@echo off

set COMPILE_AND_RUN_ASSET_PACKER=0

IF NOT EXIST bin mkdir bin
cd bin

IF %COMPILE_AND_RUN_ASSET_PACKER%==1 (
	cl -MTd -Od -Z7 -nologo -Feasset_packer -EHa- -Gm- -GR- -fp:fast -Oi -WX -W4 -wd4996 -wd4100 -wd4189 -wd4127 -wd4201 -DWIN32=1 -DDEBUG_ENABLED=1 -I../lib -I../src ../src/asset_packer.cpp shell32.lib user32.lib gdi32.lib -link
	cd ../dat
	"../bin/asset_packer.exe"
	"C:\Program Files\7-zip\7z" a asset.zip asset.pak > NUL:
	del "asset.pak"
	cd ../bin
)

rem TODO: Keep memory footprint under 128MB!
set COMMON_COMPILER_FLAGS=-s TOTAL_MEMORY=268435456 -std=c++11 -Werror -Wall -Wno-missing-braces -Wno-unused-variable -DDEBUG_ENABLED=1 -DDEV_ENABLED=1

rem set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% -s SAFE_HEAP=0
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% -s GL_UNSAFE_OPTS=1 -O3

em++ %COMPILER_FLAGS% -I../src --js-library ../src/web_audio.js ../src/asm_js_main.cpp -o dolly.html --shell-file ../src/template.html --preload-file ../dat/asset.zip@asset.zip
cd ..
