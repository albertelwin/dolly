@echo off
IF NOT EXIST bin mkdir bin
cd bin

set PORT=8000

taskkill /f /im python.exe

start chrome --incognito http://localhost:%PORT%/atom.html
python -m SimpleHTTPServer %PORT%
cd ..