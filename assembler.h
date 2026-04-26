#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "sicxe_common.h"
#include <fstream>
#include <iostream>

class Assembler {
public:
    Assembler();
    bool run(const std::string& sourceFile, const std::string& outputFile);
    void dumpIntermediate(const std::string& fileName);

private:
    bool pass1(const std::string& sourceFile);
    bool pass2(const std::string& outputFile);

    void reportError(int lineNumber, const std::string& message);
    ParsedLine parseLine(const std::string& line, int lineNumber);
    int calculateInstructionLength(ParsedLine& parsed);
    int evaluateExpression(const std::string& expr);
    
    // Helper for BYTE length
    int getByteLength(const std::string& operand);

    std::string hexString(int value, int width);
    int getRegisterNumber(const std::string& reg);

    OpcodeTable optab;
    SymbolTable symtab;
    LiteralTable littab;
    std::vector<ParsedLine> intermediateCode;

    int locctr;
    int startAddress;
    int programLength;
    int baseRegisterValue;
    bool baseActive;
    std::string programName;

    bool hasError;
    bool isXE;
};

#endif
