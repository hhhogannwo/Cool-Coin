@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "ROOT=%CD%"
set "DATA=%ROOT%\data"
set "CONF=%DATA%\coolcoin.conf"
set "DAEMON=%ROOT%\bin\coolcoind.exe"
set "CLI=%ROOT%\bin\coolcoin-cli.exe"
set "GUI=%ROOT%\CoolWallet.exe"

if not exist "%DATA%" mkdir "%DATA%"
if not exist "%DATA%\wallets" mkdir "%DATA%\wallets"

if not exist "%DAEMON%" (
  echo ERROR: Missing %DAEMON%
  pause
  exit /b 1
)

if not exist "%CLI%" (
  echo ERROR: Missing %CLI%
  pause
  exit /b 1
)

if not exist "%GUI%" (
  echo ERROR: Missing %GUI%
  pause
  exit /b 1
)

echo Starting Cool Coin daemon...
start "" /b "%DAEMON%" -datadir="%DATA%" -conf="%CONF%" -server=1 -listen=1

echo Waiting for RPC...
set /a COUNT=0

:WAIT_RPC
timeout /t 2 /nobreak >nul
"%CLI%" -datadir="%DATA%" -conf="%CONF%" getblockchaininfo >nul 2>&1
if %errorlevel%==0 goto RPC_READY

set /a COUNT+=1
if %COUNT% GEQ 45 (
  echo ERROR: RPC did not become ready.
  echo Check data\debug.log
  pause
  exit /b 1
)
goto WAIT_RPC

:RPC_READY
echo RPC ready.

"%CLI%" -datadir="%DATA%" -conf="%CONF%" listwallets | findstr /i /c:"coolwallet" >nul
if errorlevel 1 (
  echo Loading or creating coolwallet...
  "%CLI%" -datadir="%DATA%" -conf="%CONF%" loadwallet coolwallet >nul 2>&1
  if errorlevel 1 (
    "%CLI%" -datadir="%DATA%" -conf="%CONF%" createwallet coolwallet
  )
)

echo Starting CoolWallet...
start "" "%GUI%"

endlocal
