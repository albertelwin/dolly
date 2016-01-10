@echo off

IF NOT EXIST bin mkdir bin
cd bin

REM cl -MTd -Od -Z7 -nologo -Feasset_packer -EHa- -Gm- -GR- -fp:fast -Oi -WX -W4 -wd4996 -wd4100 -wd4189 -wd4127 -wd4201 -DWIN32=1 -I../lib -I../src ../src/asset_packer.cpp shell32.lib user32.lib gdi32.lib -link
REM cd ../dat && "../bin/asset_packer.exe" && cd ../bin

set COMMON_COMPILER_FLAGS=-s TOTAL_MEMORY=67108864 -std=c++11 -Werror -Wall -Wno-missing-braces -Wno-unused-variable

set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS%
REM set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% -O3

em++ %COMPILER_FLAGS% -I../src --js-library ../src/web_audio.js ../src/asm_js_main.cpp -o dolly.html --shell-file ../src/template.html --preload-file ../dat/@/
cd ..
