@echo off
g++ -o sicxe_asm.exe main.cpp assembler.cpp sicxe_common.cpp
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    exit /b %errorlevel%
)
echo [SUCCESS] Compilation successful.

if "%1"=="" (
    echo Usage: run.bat <source_file.asm> [output_file.obj]
) else (
    .\sicxe_asm.exe %*
)
