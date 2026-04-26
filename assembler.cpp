#include "assembler.h"
#include <sstream>
#include <iomanip>
#include <vector>

Assembler::Assembler() : locctr(0), startAddress(0), programLength(0), baseRegisterValue(0), baseActive(false), hasError(false), isXE(false) {}

bool Assembler::run(const std::string& sourceFile, const std::string& outputFile) {
    std::cout << "Starting Pass 1..." << std::endl;
    if (!pass1(sourceFile)) {
        std::cout << "Pass 1 failed due to errors." << std::endl;
        return false;
    }
    dumpIntermediate("intermediate.txt");

    std::cout << "Starting Pass 2..." << std::endl;
    if (!pass2(outputFile)) {
        std::cout << "Pass 2 failed due to errors." << std::endl;
        return false;
    }

    std::cout << "Assembly completed successfully. Output: " << outputFile << std::endl;
    return true;
}

ParsedLine Assembler::parseLine(const std::string& line, int lineNumber) {
    ParsedLine parsed;
    parsed.lineNumber = lineNumber;
    parsed.rawLine = line;
    parsed.address = -1;
    parsed.isDirective = false;
    parsed.isFormat4 = false;
    parsed.length = 0;

    if (line.empty() || line[0] == '.') return parsed; // Comment or empty

    std::string cleanLine = line;
    size_t commentPos = cleanLine.find('.');
    if (commentPos != std::string::npos) {
        parsed.comment = cleanLine.substr(commentPos);
        cleanLine = cleanLine.substr(0, commentPos);
    }

    // Special handling for labels starting at col 0
    std::string label, mnemonic, operand;
    if (!line.empty() && !isspace(line[0])) {
        // Find first space
        size_t firstSpace = cleanLine.find_first_of(" \t");
        if (firstSpace != std::string::npos) {
            label = cleanLine.substr(0, firstSpace);
            size_t nextTokenStart = cleanLine.find_first_not_of(" \t", firstSpace);
            if (nextTokenStart != std::string::npos) {
                size_t secondSpace = cleanLine.find_first_of(" \t", nextTokenStart);
                mnemonic = cleanLine.substr(nextTokenStart, secondSpace - nextTokenStart);
                if (secondSpace != std::string::npos) {
                    size_t thirdTokenStart = cleanLine.find_first_not_of(" \t", secondSpace);
                    if (thirdTokenStart != std::string::npos) {
                        operand = cleanLine.substr(thirdTokenStart);
                        // Trim trailing whitespace from operand
                        operand.erase(operand.find_last_not_of(" \t") + 1);
                    }
                }
            }
        } else {
            label = cleanLine;
        }
    } else {
        size_t nextTokenStart = cleanLine.find_first_not_of(" \t");
        if (nextTokenStart != std::string::npos) {
            size_t secondSpace = cleanLine.find_first_of(" \t", nextTokenStart);
            mnemonic = cleanLine.substr(nextTokenStart, secondSpace - nextTokenStart);
            if (secondSpace != std::string::npos) {
                size_t thirdTokenStart = cleanLine.find_first_not_of(" \t", secondSpace);
                if (thirdTokenStart != std::string::npos) {
                    operand = cleanLine.substr(thirdTokenStart);
                    operand.erase(operand.find_last_not_of(" \t") + 1);
                }
            }
        }
    }

    parsed.label = label;
    parsed.mnemonic = mnemonic;
    parsed.operand = operand;

    if (!parsed.mnemonic.empty() && parsed.mnemonic[0] == '+') {
        parsed.isFormat4 = true;
        parsed.mnemonic = parsed.mnemonic.substr(1);
    }

    return parsed;
}

void Assembler::reportError(int lineNumber, const std::string& message) {
    std::cerr << "Error at line " << lineNumber << ": " << message << std::endl;
    hasError = true;
}

int Assembler::getByteLength(const std::string& operand) {
    if (operand.length() < 3) return 0;
    if (operand[0] == 'C') return operand.length() - 3;
    if (operand[0] == 'X') return (operand.length() - 3 + 1) / 2;
    return 0;
}

bool Assembler::pass1(const std::string& sourceFile) {
    std::ifstream file(sourceFile);
    if (!file.is_open()) {
        std::cerr << "Could not open source file: " << sourceFile << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    bool endEncountered = false;

    while (std::getline(file, line)) {
        lineNumber++;
        ParsedLine parsed = parseLine(line, lineNumber);

        if (parsed.mnemonic.empty() && parsed.label.empty()) continue; // Skip comments/empty

        if (parsed.mnemonic == "START") {
            isXE = false; // Reset for new run, detect later
            programName = parsed.label;
            try {
                startAddress = std::stoi(parsed.operand, nullptr, 16);
            } catch (...) {
                startAddress = 0;
            }
            locctr = startAddress;
            parsed.address = locctr;
            intermediateCode.push_back(parsed);
            continue;
        }

        if (endEncountered) {
            reportError(lineNumber, "Instructions found after END directive");
            continue;
        }

        parsed.address = locctr;

        if (!parsed.label.empty()) {
            if (!symtab.insert(parsed.label, locctr)) {
                reportError(lineNumber, "Duplicate label: " + parsed.label);
            }
        }

        if (parsed.mnemonic == "END") {
            endEncountered = true;
            intermediateCode.push_back(parsed);
            // Handle remaining literals
            auto unassigned = littab.getUnassigned();
            for (auto lit : unassigned) {
                lit->address = locctr;
                lit->isAssigned = true;
                ParsedLine litLine;
                litLine.address = locctr;
                litLine.mnemonic = "*";
                litLine.operand = lit->value;
                litLine.length = lit->length;
                intermediateCode.push_back(litLine);
                locctr += litLine.length;
            }
            break;
        }

        if (parsed.mnemonic == "EQU") {
            parsed.isDirective = true;
            if (parsed.operand == "*") {
                // Address already set to locctr
            } else {
                try {
                    // int val = std::stoi(parsed.operand);
                    // Update symtab if it was already inserted
                } catch (...) {
                    // Symbolic EQU
                }
            }
            intermediateCode.push_back(parsed);
        } else if (parsed.mnemonic == "ORG") {
            parsed.isDirective = true;
            try {
                locctr = std::stoi(parsed.operand, nullptr, 16);
            } catch (...) {
                reportError(lineNumber, "Invalid ORG operand");
            }
            intermediateCode.push_back(parsed);
        } else if (parsed.mnemonic == "LTORG") {
            parsed.isDirective = true;
            isXE = true;
            intermediateCode.push_back(parsed);
            auto unassigned = littab.getUnassigned();
            for (auto lit : unassigned) {
                lit->address = locctr;
                lit->isAssigned = true;
                ParsedLine litLine;
                litLine.address = locctr;
                litLine.mnemonic = "*";
                litLine.operand = lit->value;
                litLine.length = lit->length;
                intermediateCode.push_back(litLine);
                locctr += litLine.length;
            }
            continue;
        } else if (parsed.mnemonic == "BASE" || parsed.mnemonic == "NOBASE") {
            parsed.isDirective = true;
            isXE = true;
            intermediateCode.push_back(parsed);
            continue;
        } else {
            // Instruction or Data allocation
            if (optab.exists(parsed.mnemonic)) {
                auto info = optab.get(parsed.mnemonic);
                if (parsed.isFormat4) { parsed.length = 4; isXE = true; }
                else if (info.format == Format::F1) { parsed.length = 1; isXE = true; }
                else if (info.format == Format::F2) { parsed.length = 2; isXE = true; }
                else parsed.length = 3;
                
                if (!parsed.operand.empty()) {
                    if (parsed.operand[0] == '#' || parsed.operand[0] == '@') isXE = true;
                }
            } else if (parsed.mnemonic == "WORD") {
                parsed.length = 3;
            } else if (parsed.mnemonic == "RESW") {
                try {
                    parsed.length = 3 * std::stoi(parsed.operand);
                } catch (...) { reportError(lineNumber, "Invalid RESW operand"); }
            } else if (parsed.mnemonic == "RESB") {
                try {
                    parsed.length = std::stoi(parsed.operand);
                } catch (...) { reportError(lineNumber, "Invalid RESB operand"); }
            } else if (parsed.mnemonic == "BYTE") {
                parsed.length = getByteLength(parsed.operand);
            } else {
                reportError(lineNumber, "Invalid opcode: " + parsed.mnemonic);
            }

            // Literal check in operand
            if (!parsed.operand.empty() && parsed.operand[0] == '=') {
                littab.add(parsed.operand);
            }

            intermediateCode.push_back(parsed);
            locctr += parsed.length;
        }
    }

    programLength = locctr - startAddress;
    return !hasError;
}

std::string Assembler::hexString(int value, int width) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << (static_cast<unsigned int>(value) & ((1LL << (width * 4)) - 1));
    return ss.str();
}

int Assembler::getRegisterNumber(const std::string& reg) {
    if (reg == "A") return 0;
    if (reg == "X") return 1;
    if (reg == "L") return 2;
    if (reg == "B") return 3;
    if (reg == "S") return 4;
    if (reg == "T") return 5;
    if (reg == "F") return 6;
    if (reg == "PC") return 8;
    if (reg == "SW") return 9;
    return 0; // Default to A if not found
}

bool Assembler::pass2(const std::string& outputFile) {
    std::ofstream objFile(outputFile);
    if (!objFile.is_open()) return false;

    // Header
    objFile << "H" << std::left << std::setw(6) << (programName.length() > 6 ? programName.substr(0, 6) : programName) 
            << std::right << std::setfill('0') << hexString(startAddress, 6) << hexString(programLength, 6) << std::endl;

    std::string currentTRecord = "";
    int tStartAddr = -1;
    std::vector<std::string> mRecords;

    for (size_t i = 0; i < intermediateCode.size(); ++i) {
        auto& line = intermediateCode[i];
        std::string objCode = "";

        if (line.mnemonic == "BASE") {
            if (symtab.exists(line.operand)) baseRegisterValue = symtab.getAddress(line.operand);
            else try { baseRegisterValue = std::stoi(line.operand, nullptr, 16); } catch(...) {}
            baseActive = true;
            continue;
        } else if (line.mnemonic == "NOBASE") {
            baseActive = false;
            continue;
        } else if (line.mnemonic == "START" || line.mnemonic == "EQU" || line.mnemonic == "ORG" || line.mnemonic == "LTORG") {
            continue;
        } else if (line.mnemonic == "END") {
             break;
        }

        // std::cout << "Processing: " << line.mnemonic << " [" << line.operand << "]" << std::endl;

        if (line.mnemonic == "RESW" || line.mnemonic == "RESB") {
            if (!currentTRecord.empty()) {
                objFile << "T" << hexString(tStartAddr, 6) << hexString(currentTRecord.length() / 2, 2) << currentTRecord << std::endl;
                currentTRecord = ""; tStartAddr = -1;
            }
            continue;
        }

        if (line.mnemonic == "BYTE") {
            if (line.operand.length() >= 3) {
                if (line.operand[0] == 'C') {
                    for (size_t j = 2; j < line.operand.length() - 1; ++j) objCode += hexString(line.operand[j], 2);
                } else if (line.operand[0] == 'X') {
                    objCode = line.operand.substr(2, line.operand.length() - 3);
                }
            }
        } else if (line.mnemonic == "WORD") {
            try { objCode = hexString(std::stoi(line.operand), 6); } catch(...) { objCode = "000000"; }
        } else if (line.mnemonic == "*") { // Literal data
            if (line.operand.length() >= 4) {
                if (line.operand[1] == 'C') {
                    for (size_t j = 3; j < line.operand.length() - 1; ++j) objCode += hexString(line.operand[j], 2);
                } else if (line.operand[1] == 'X') {
                    objCode = line.operand.substr(3, line.operand.length() - 4);
                }
            }
        } else if (line.mnemonic == "RSUB") {
            objCode = isXE ? "4F0000" : "4C0000";
        } else if (optab.exists(line.mnemonic)) {
            auto info = optab.get(line.mnemonic);
            if (info.format == Format::F1) {
                objCode = hexString(info.opcode, 2);
            } else if (info.format == Format::F2) {
                objCode = hexString(info.opcode, 2);
                size_t comma = line.operand.find(',');
                if (comma != std::string::npos) {
                    objCode += hexString(getRegisterNumber(line.operand.substr(0, comma)), 1);
                    objCode += hexString(getRegisterNumber(line.operand.substr(comma + 1)), 1);
                } else {
                    objCode += hexString(getRegisterNumber(line.operand), 1) + "0";
                }
            } else if (!isXE) {
                // SIC Standard Format
                int x = 0;
                std::string op = line.operand;
                size_t comma = op.find(",X");
                if (comma != std::string::npos) { x = 1; op = op.substr(0, comma); }
                
                int targetAddr = 0;
                if (symtab.exists(op)) targetAddr = symtab.getAddress(op);
                else if (!op.empty() && isdigit(op[0])) { try { targetAddr = std::stoi(op); } catch(...) {} }

                int firstByte = info.opcode;
                int secondByte = (x << 7) | ((targetAddr >> 8) & 0x7F);
                int thirdByte = targetAddr & 0xFF;
                objCode = hexString(firstByte, 2) + hexString(secondByte, 2) + hexString(thirdByte, 2);
            } else {
                int n = 1, i_flag = 1, x = 0, b = 0, p = 0, e = (line.isFormat4 ? 1 : 0);
                std::string op = line.operand;
                if (!op.empty()) {
                    if (op[0] == '#') { n = 0; i_flag = 1; op = op.substr(1); }
                    else if (op[0] == '@') { n = 1; i_flag = 0; op = op.substr(1); }
                    size_t comma = op.find(",X");
                    if (comma != std::string::npos) { x = 1; op = op.substr(0, comma); }
                }

                int targetAddr = 0;
                bool isImmediateValue = false;
                bool symbolFound = false;
                if (symtab.exists(op)) {
                    targetAddr = symtab.getAddress(op);
                    symbolFound = true;
                } else if (!op.empty() && littab.getAddress(op) != -1) {
                    targetAddr = littab.getAddress(op);
                    symbolFound = true;
                } else if (!op.empty() && isdigit(op[0])) { 
                    try { targetAddr = std::stoi(op); isImmediateValue = true; symbolFound = true; } catch(...) {}
                }

                if (!symbolFound && !op.empty()) {
                    reportError(line.lineNumber, "Undefined symbol: " + op);
                }

                int disp = 0;
                if (line.isFormat4) {
                    disp = targetAddr;
                    if (n == 1 && i_flag == 1) mRecords.push_back("M" + hexString(line.address + 1, 6) + "05");
                } else {
                    if (isImmediateValue && n == 0 && i_flag == 1) {
                        disp = targetAddr;
                    } else {
                        int pc = line.address + 3;
                        disp = targetAddr - pc;
                        if (disp >= -2048 && disp <= 2047) { p = 1; b = 0; }
                        else if (baseActive) {
                            disp = targetAddr - baseRegisterValue;
                            if (disp >= 0 && disp <= 4095) { p = 0; b = 1; }
                            else { reportError(line.lineNumber, "Displacement out of range for PC and Base relative"); }
                        } else {
                            // Try SIC absolute addressing
                            if (n == 1 && i_flag == 1) {
                                disp = targetAddr;
                                p = 0; b = 0;
                            } else if (n == 0 && i_flag == 1) {
                                disp = targetAddr;
                            } else {
                                reportError(line.lineNumber, "Displacement out of range (Base relative not active)");
                            }
                        }
                    }
                }

                int firstByte = (info.opcode & 0xFC) | (n << 1) | i_flag;
                int secondByte = (x << 7) | (b << 6) | (p << 5) | (e << 4);
                if (line.isFormat4) {
                   secondByte |= ((disp >> 16) & 0x0F);
                   objCode = hexString(firstByte, 2) + hexString(secondByte, 2) + hexString(disp & 0xFFFF, 4);
                } else {
                   secondByte |= ((disp >> 8) & 0x0F);
                   objCode = hexString(firstByte, 2) + hexString(secondByte, 2) + hexString(disp & 0xFF, 2);
                }
            }
        }

        if (!objCode.empty()) {
            if (tStartAddr == -1) tStartAddr = line.address;
            if (currentTRecord.length() + objCode.length() > 60) {
                objFile << "T" << hexString(tStartAddr, 6) << hexString(currentTRecord.length() / 2, 2) << currentTRecord << std::endl;
                currentTRecord = objCode; tStartAddr = line.address;
            } else {
                currentTRecord += objCode;
            }
        }
    }

    if (!currentTRecord.empty()) {
        objFile << "T" << hexString(tStartAddr, 6) << hexString(currentTRecord.length() / 2, 2) << currentTRecord << std::endl;
    }

    for (const auto& m : mRecords) objFile << m << std::endl;
    
    int execAddr = startAddress;
    for (auto& line : intermediateCode) if (line.mnemonic == "END" && !line.operand.empty()) if (symtab.exists(line.operand)) execAddr = symtab.getAddress(line.operand);
    objFile << "E" << hexString(execAddr, 6) << std::endl;

    return !hasError;
}

void Assembler::dumpIntermediate(const std::string& fileName) {
    std::ofstream file(fileName);
    if (!file.is_open()) return;
    file << std::left << std::setw(10) << "Address" << std::setw(10) << "Label" << std::setw(10) << "Mnemonic" << std::setw(20) << "Operand" << std::endl;
    for (const auto& line : intermediateCode) {
        file << std::left << std::setw(10) << hexString(line.address, 4) << std::setw(10) << line.label << std::setw(10) << (line.isFormat4 ? "+" : "") + line.mnemonic << std::setw(20) << line.operand << std::endl;
    }
}
