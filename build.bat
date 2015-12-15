@echo off
IF NOT EXIST bin mkdir bin
cd bin

set COMMON_COMPILER_FLAGS=-s TOTAL_MEMORY=67108864 -std=c++11 -Werror

REM set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS%
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% -O3

em++ %COMPILER_FLAGS% -I../src --js-library ../src/web_audio.js ../src/asm_js_main.cpp -o atom.html --shell-file ../src/template.html --preload-file ../dat/@/
cd ..
