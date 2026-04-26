#!/bin/bash
g++ -o sicxe_asm main.cpp assembler.cpp sicxe_common.cpp
if [ $? -ne 0 ]; then
    echo "[ERROR] Compilation failed."
    exit 1
fi
echo "[SUCCESS] Compilation successful."

if [ -z "$1" ]; then
    echo "Usage: ./run.sh <source_file.asm> [output_file.obj]"
else
    chmod +x ./sicxe_asm
    ./sicxe_asm "$@"
fi
