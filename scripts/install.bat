@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ================================================================
:: Luma Compiler v0.1.0 - Windows Installer
:: ================================================================
::
:: Expected project layout:
::
::   Luma/
::   ├── bin/
::   │   └── luma.exe
::   ├── src/
::   │   └── *.lx
::   ├── std/
::   │   └── ...
::   └── scripts/
::       └── install.bat
::
:: Installed layout:
::
::   C:\Program Files\luma/
::   ├── bin/
::   │   └── luma.exe
::   └── std/
::       └── ...
::
:: ================================================================

title Luma Compiler Installer

echo.
echo ================================================================
echo              Luma Compiler v0.1.0 - Windows Installer
echo ================================================================
echo.

:: ----------------------------------------------------------------
:: Determine project directory
:: ----------------------------------------------------------------
::
:: install.bat is located in:
::
::   Luma\scripts\install.bat
::
:: Therefore:
::
::   %~dp0       = Luma\scripts\
::   %~dp0..     = Luma\
::
:: ----------------------------------------------------------------

set "PROJECT_DIR=%~dp0.."

:: Convert to absolute/normalized path
for %%A in ("%PROJECT_DIR%") do set "PROJECT_DIR=%%~fA"

set "LUMA_EXE=%PROJECT_DIR%\bin\luma.exe"
set "STD_DIR=%PROJECT_DIR%\std"

:: ----------------------------------------------------------------
:: Installation paths
:: ----------------------------------------------------------------

set "INSTALL_DIR=C:\Program Files\luma"
set "BIN_DIR=%INSTALL_DIR%\bin"
set "INSTALL_STD_DIR=%INSTALL_DIR%\std"

:: ----------------------------------------------------------------
:: Display configuration
:: ----------------------------------------------------------------

echo Project directory:
echo   %PROJECT_DIR%
echo.
echo Compiler source:
echo   %LUMA_EXE%
echo.
echo Standard library source:
echo   %STD_DIR%
echo.
echo Installation directory:
echo   %INSTALL_DIR%
echo.

:: ----------------------------------------------------------------
:: Check administrator privileges
:: ----------------------------------------------------------------

net session >nul 2>&1

if %errorlevel% neq 0 (
    echo Administrator privileges are required.
    echo.
    echo Requesting administrator privileges...
    echo.

    "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" ^
        -NoProfile -ExecutionPolicy Bypass ^
        -Command "Start-Process -FilePath '%~f0' -Verb RunAs"

    if errorlevel 1 (
        echo.
        echo ERROR: Failed to request administrator privileges.
        echo.
        echo Please right-click install.bat and select:
        echo "Run as administrator"
        echo.
        pause
        exit /b 1
    )

    exit /b 0
)

echo [OK] Running with administrator privileges.
echo.

:: ----------------------------------------------------------------
:: Check compiler
:: ----------------------------------------------------------------

echo ================================================================
echo Checking compiler...
echo ================================================================
echo.

if not exist "%LUMA_EXE%" (
    echo ERROR: luma.exe was not found.
    echo.
    echo Expected:
    echo   %LUMA_EXE%
    echo.
    echo Your project should contain:
    echo.
    echo   Luma\
    echo   +-- bin\
    echo   ^|   +-- luma.exe
    echo   +-- src\
    echo   ^|   +-- *.lx
    echo   +-- std\
    echo   ^|   +-- ...
    echo   +-- scripts\
    echo       +-- install.bat
    echo.
    pause
    exit /b 1
)

echo [OK] Found luma.exe
echo.

:: ----------------------------------------------------------------
:: Check standard library
:: ----------------------------------------------------------------

echo ================================================================
echo Checking standard library...
echo ================================================================
echo.

if not exist "%STD_DIR%" (
    echo ERROR: Standard library directory was not found.
    echo.
    echo Expected:
    echo   %STD_DIR%
    echo.
    echo The Luma compiler requires the standard library.
    echo.
    pause
    exit /b 1
)

echo [OK] Found standard library:
echo      %STD_DIR%
echo.

:: ----------------------------------------------------------------
:: Create installation directories
:: ----------------------------------------------------------------

echo ================================================================
echo Creating installation directories...
echo ================================================================
echo.

if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%" >nul 2>&1

    if errorlevel 1 (
        echo ERROR: Failed to create:
        echo   %INSTALL_DIR%
        echo.
        pause
        exit /b 1
    )
)

if not exist "%BIN_DIR%" (
    mkdir "%BIN_DIR%" >nul 2>&1

    if errorlevel 1 (
        echo ERROR: Failed to create:
        echo   %BIN_DIR%
        echo.
        pause
        exit /b 1
    )
)

if not exist "%INSTALL_STD_DIR%" (
    mkdir "%INSTALL_STD_DIR%" >nul 2>&1

    if errorlevel 1 (
        echo ERROR: Failed to create:
        echo   %INSTALL_STD_DIR%
        echo.
        pause
        exit /b 1
    )
)

echo [OK] Installation directories created.
echo.

:: ----------------------------------------------------------------
:: Install compiler
:: ----------------------------------------------------------------

echo ================================================================
echo Installing Luma compiler...
echo ================================================================
echo.

echo Source:
echo   %LUMA_EXE%
echo.
echo Destination:
echo   %BIN_DIR%\luma.exe
echo.

copy /Y "%LUMA_EXE%" "%BIN_DIR%\luma.exe" >nul

if errorlevel 1 (
    echo.
    echo ERROR: Failed to copy luma.exe.
    echo.
    pause
    exit /b 1
)

echo [OK] Luma compiler installed.
echo.

:: ----------------------------------------------------------------
:: Install standard library
:: ----------------------------------------------------------------

echo ================================================================
echo Installing Luma standard library...
echo ================================================================
echo.

echo Source:
echo   %STD_DIR%
echo.
echo Destination:
echo   %INSTALL_STD_DIR%
echo.

:: Copy the entire std directory recursively.
::
:: /E = copy subdirectories, including empty ones
:: /I = assume destination is a directory
:: /Y = overwrite existing files without prompting
:: /Q = quiet mode

xcopy "%STD_DIR%\*" "%INSTALL_STD_DIR%\" /E /I /Y /Q >nul

if errorlevel 1 (
    echo.
    echo ERROR: Failed to copy the standard library.
    echo.
    echo Source:
    echo   %STD_DIR%
    echo.
    echo Destination:
    echo   %INSTALL_STD_DIR%
    echo.
    pause
    exit /b 1
)

echo [OK] Standard library installed.
echo.

:: ----------------------------------------------------------------
:: Verify compiler
:: ----------------------------------------------------------------

echo ================================================================
echo Verifying installation...
echo ================================================================
echo.

if exist "%BIN_DIR%\luma.exe" (
    echo [OK] Compiler:
    echo      %BIN_DIR%\luma.exe
) else (
    echo [ERROR] Compiler was not found after installation.
    echo.
    pause
    exit /b 1
)

:: ----------------------------------------------------------------
:: Verify standard library
:: ----------------------------------------------------------------

if exist "%INSTALL_STD_DIR%" (
    echo [OK] Standard library:
    echo      %INSTALL_STD_DIR%\
) else (
    echo [ERROR] Standard library was not found after installation.
    echo.
    pause
    exit /b 1
)

echo.

:: ----------------------------------------------------------------
:: Add Luma to system PATH
:: ----------------------------------------------------------------

echo ================================================================
echo Configuring system PATH...
echo ================================================================
echo.

set "SYSTEM_PATH="

for /f "tokens=2,*" %%A in (
    'reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul'
) do (
    if /I "%%A"=="Path" (
        set "SYSTEM_PATH=%%B"
    )
)

echo !SYSTEM_PATH! | findstr /I /L /C:"%BIN_DIR%" >nul 2>&1

if not errorlevel 1 (
    echo [OK] Luma is already in the system PATH.
) else (
    echo Adding:
    echo   %BIN_DIR%
    echo.

    setx /M PATH "!SYSTEM_PATH!;%BIN_DIR%" >nul 2>&1

    if errorlevel 1 (
        echo [WARNING] Failed to update the system PATH.
        echo.
        echo Please manually add:
        echo   %BIN_DIR%
    ) else (
        echo [OK] Luma added to the system PATH.
    )
)

echo.

:: ----------------------------------------------------------------
:: Test compiler
:: ----------------------------------------------------------------

echo ================================================================
echo Testing Luma compiler...
echo ================================================================
echo.

"%BIN_DIR%\luma.exe" --version

if errorlevel 1 (
    echo.
    echo [WARNING] Luma was installed but could not be executed.
    echo.
    echo Windows may be blocking the unsigned executable through
    echo Smart App Control or another Code Integrity policy.
    echo.
    echo The installation itself was successful.
) else (
    echo.
    echo [OK] Luma executed successfully.
)

echo.

:: ----------------------------------------------------------------
:: Installation summary
:: ----------------------------------------------------------------

echo ================================================================
echo                 Installation Summary
echo ================================================================
echo.

echo Project:
echo   %PROJECT_DIR%
echo.

echo Compiler source:
echo   %LUMA_EXE%
echo.

echo Installed compiler:
echo   %BIN_DIR%\luma.exe
echo.

echo Standard library source:
echo   %STD_DIR%\
echo.

echo Installed standard library:
echo   %INSTALL_STD_DIR%\
echo.

echo PATH entry:
echo   %BIN_DIR%
echo.

echo ================================================================
echo Important
echo ================================================================
echo.

echo Luma source files remain in:
echo   %PROJECT_DIR%\src\
echo.

echo The source files are NOT copied by this installer.
echo.

echo The standard library IS copied to:
echo   %INSTALL_STD_DIR%\
echo.

echo PATH changes only apply to NEW terminals.
echo.
echo Close this terminal and open a new PowerShell or Command Prompt.
echo.

echo Then run:
echo.
echo   luma --version
echo.
echo   luma --help
echo.

echo ================================================================
echo                    Installation Complete
echo ================================================================
echo.

pause
exit /b 0
