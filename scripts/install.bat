@echo off
setlocal enabledelayedexpansion

echo.
echo ================================================
echo    Luma Compiler v0.1.0 - Windows Installer
echo ================================================
echo.

:: Check if running as administrator
net session >nul 2>&1
if %errorLevel% == 0 (
    set "ADMIN=1"
    echo Running with administrator privileges...
    echo Installing system-wide to: C:\Program Files\luma
    set "INSTALL_DIR=C:\Program Files\luma"
) else (
    set "ADMIN=0"
    echo Running without administrator privileges...
    echo Installing for current user to: %USERPROFILE%\.luma
    set "INSTALL_DIR=%USERPROFILE%\.luma"
)
echo.

:: Create installation directories
echo Creating installation directories...

ver >nul

if not exist "!INSTALL_DIR!\bin" md "!INSTALL_DIR!\bin" 2>nul
if not exist "!INSTALL_DIR!\std" md "!INSTALL_DIR!\std" 2>nul

if not exist "!INSTALL_DIR!\bin\" (
    echo ERROR: Failed to create directories^^!
    echo Please check permissions and try again.
    pause
    exit /b 1
)
if not exist "!INSTALL_DIR!\std\" (
    echo ERROR: Failed to create directories^^!
    echo Please check permissions and try again.
    pause
    exit /b 1
)

:: Find luma.exe - check common build output locations
set "LUMA_SRC="
if exist "build\luma.exe"         set "LUMA_SRC=build\luma.exe"
if exist "build\release\luma.exe" set "LUMA_SRC=build\release\luma.exe"
if exist "build\debug\luma.exe"   set "LUMA_SRC=build\debug\luma.exe"
if exist "target\luma.exe"        set "LUMA_SRC=target\luma.exe"
if exist "luma.exe"               set "LUMA_SRC=luma.exe"

if "!LUMA_SRC!" == "" (
    echo ERROR: luma.exe not found. Searched:
    echo   build\luma.exe
    echo   build\release\luma.exe
    echo   build\debug\luma.exe
    echo   target\luma.exe
    echo   luma.exe
    pause
    exit /b 1
)

echo Copying luma.exe from !LUMA_SRC!...
copy /Y "!LUMA_SRC!" "!INSTALL_DIR!\bin\luma.exe" >nul
if !errorLevel! neq 0 (
    echo ERROR: Failed to copy luma.exe^^!
    pause
    exit /b 1
)

:: Copy standard library
echo Copying standard library...
if exist "std" (
    xcopy /E /I /Y "std\*" "!INSTALL_DIR!\std\" >nul
    if !errorLevel! neq 0 (
        echo ERROR: Failed to copy standard library^^!
        pause
        exit /b 1
    )
) else (
    echo WARNING: std\ directory not found. Standard library not installed.
)

echo.
echo Installation completed successfully^^!
echo.
echo Luma installed to: !INSTALL_DIR!
echo.

:: Check if already in PATH
echo %PATH% | findstr /C:"!INSTALL_DIR!\bin" >nul
if !errorLevel! == 0 (
    echo Path already configured correctly.
    goto :verify
)

:: Add to PATH
echo Configuring PATH...
if !ADMIN! == 1 (
    for /f "skip=2 tokens=3*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path') do set "SYSTEM_PATH=%%a %%b"
    setx /M PATH "!SYSTEM_PATH!;!INSTALL_DIR!\bin" >nul 2>&1
    if !errorLevel! == 0 (
        echo PATH updated successfully ^(system-wide^).
    ) else (
        echo WARNING: Failed to update system PATH automatically.
        echo Please add manually: !INSTALL_DIR!\bin
    )
) else (
    for /f "skip=2 tokens=3*" %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "USER_PATH=%%a %%b"
    if "!USER_PATH!" == "" (
        setx PATH "!INSTALL_DIR!\bin" >nul 2>&1
    ) else (
        setx PATH "!USER_PATH!;!INSTALL_DIR!\bin" >nul 2>&1
    )
    if !errorLevel! == 0 (
        echo PATH updated successfully ^(user-only^).
    ) else (
        echo WARNING: Failed to update user PATH automatically.
        echo Please add manually: !INSTALL_DIR!\bin
    )
)

:verify
echo.
echo ================================================
echo Verifying installation...
echo ================================================
echo.

"!INSTALL_DIR!\bin\luma.exe" --version >nul 2>&1
if !errorLevel! == 0 (
    echo [OK] Luma compiler found
    "!INSTALL_DIR!\bin\luma.exe" --version
) else (
    echo [WARNING] Could not verify luma installation
)
echo.

if exist "!INSTALL_DIR!\std\io.luma" (
    echo [OK] Standard library installed
) else (
    echo [WARNING] Standard library may not be complete
)

echo.
echo ================================================
echo Installation Summary
echo ================================================
echo.
echo Installation directory: !INSTALL_DIR!
echo Compiler binary:        !INSTALL_DIR!\bin\luma.exe
echo Standard library:       !INSTALL_DIR!\std\
echo.
echo IMPORTANT: You may need to restart your terminal
echo            for PATH changes to take effect.
echo.
echo To verify installation, open a NEW terminal and run:
echo   luma --version
echo.
echo To uninstall, delete: !INSTALL_DIR!
echo   and remove from PATH if necessary.
echo.
echo ================================================
echo Installation complete^^!
echo ================================================
echo.
pause