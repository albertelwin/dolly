@echo off

set COMPILE_AND_RUN_ASSET_PACKER=0

IF NOT EXIST bin mkdir bin
cd bin

IF %COMPILE_AND_RUN_ASSET_PACKER%==1 (
	cl -MTd -Od -Z7 -nologo -Feasset_packer -EHa- -Gm- -GR- -fp:fast -Oi -WX -W4 -wd4996 -wd4100 -wd4189 -wd4127 -wd4201 -DWIN32=1 -DDEBUG_ENABLED=1 -I../lib -I../src ../src/asset_packer.cpp shell32.lib user32.lib gdi32.lib -link
	cd ../dat
	"../bin/asset_packer.exe"
	"C:\Program Files\7-zip\7z" a pak/preload.zip preload.pak > NUL: && del "preload.pak"
	"C:\Program Files\7-zip\7z" a pak/map.zip map.pak > NUL: && del "map.pak"
	"C:\Program Files\7-zip\7z" a pak/texture.zip texture.pak > NUL: && del "texture.pak"
	rem "C:\Program Files\7-zip\7z" a pak/audio.zip audio.pak > NUL: && del "audio.pak"
	"C:\Program Files\7-zip\7z" a pak/atlas.zip atlas.pak > NUL: && del "atlas.pak"
	cd ../bin
)

set COMMON_COMPILER_FLAGS=-s TOTAL_MEMORY=134217728 -std=c++11 -Werror -Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function -DDEBUG_ENABLED=1 -DDEV_ENABLED=0

rem set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% -s SAFE_HEAP=0
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% -s GL_UNSAFE_OPTS=1 -O3

em++ %COMPILER_FLAGS% -I../src --js-library ../src/web_audio.js ../src/asm_js_main.cpp -o dolly.html --shell-file ../src/template.html --preload-file ../dat/pak@pak --preload-file ../dat/ogg@audio
cd ..
