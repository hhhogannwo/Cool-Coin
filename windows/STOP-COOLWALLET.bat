@echo off
setlocal
cd /d "%~dp0"

set "DATA=%CD%\data"
set "CONF=%DATA%\coolcoin.conf"
set "CLI=%CD%\bin\coolcoin-cli.exe"

if exist "%CLI%" (
  "%CLI%" -datadir="%DATA%" -conf="%CONF%" stop
)

taskkill /IM CoolWallet.exe /F >nul 2>&1
taskkill /IM coolcoind.exe /F >nul 2>&1

echo CoolWallet stopped.
endlocal
