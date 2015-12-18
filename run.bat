@echo off
IF NOT EXIST bin mkdir bin
cd bin

set PORT=8000

taskkill /f /im python.exe

start chrome --incognito http://localhost:%PORT%/dolly.html
python -m SimpleHTTPServer %PORT%
cd ..