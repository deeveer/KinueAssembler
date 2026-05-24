@echo off
setlocal

g++ -o sicxe_loader.exe loader_main.cpp linking_loader.cpp
if %errorlevel% neq 0 (
    echo [ERROR] Loader compilation failed.
    exit /b %errorlevel%
)

if not exist loader_tests\actual mkdir loader_tests\actual

.\sicxe_loader.exe --addr 4000 --map loader_tests\actual\single_section.map --mem loader_tests\actual\single_section.mem loader_tests\single_section.obj
if %errorlevel% neq 0 exit /b %errorlevel%
fc loader_tests\expected\single_section.map loader_tests\actual\single_section.map >nul
if %errorlevel% neq 0 (
    echo [ERROR] single_section.map differs.
    exit /b 1
)
fc loader_tests\expected\single_section.mem loader_tests\actual\single_section.mem >nul
if %errorlevel% neq 0 (
    echo [ERROR] single_section.mem differs.
    exit /b 1
)

.\sicxe_loader.exe --addr 4000 --map loader_tests\actual\two_sections.map --mem loader_tests\actual\two_sections.mem loader_tests\two_sections_main.obj loader_tests\two_sections_sub.obj
if %errorlevel% neq 0 exit /b %errorlevel%
fc loader_tests\expected\two_sections.map loader_tests\actual\two_sections.map >nul
if %errorlevel% neq 0 (
    echo [ERROR] two_sections.map differs.
    exit /b 1
)
fc loader_tests\expected\two_sections.mem loader_tests\actual\two_sections.mem >nul
if %errorlevel% neq 0 (
    echo [ERROR] two_sections.mem differs.
    exit /b 1
)

.\sicxe_loader.exe --addr 4000 loader_tests\duplicate_symbol_main.obj loader_tests\duplicate_symbol_sub.obj >nul 2>nul
if %errorlevel% equ 0 (
    echo [ERROR] duplicate symbol fixture should fail.
    exit /b 1
)

.\sicxe_loader.exe --addr 4000 loader_tests\undefined_symbol.obj >nul 2>nul
if %errorlevel% equ 0 (
    echo [ERROR] undefined symbol fixture should fail.
    exit /b 1
)

echo [SUCCESS] Loader tests passed.
