#include "sicxe_common.h"
#include <algorithm>

OpcodeTable::OpcodeTable() {
    // Format 1
    table["FIX"]   = {"FIX",   0xC4, Format::F1};
    table["FLOAT"] = {"FLOAT", 0xC0, Format::F1};
    table["HIO"]   = {"HIO",   0xC8, Format::F1};
    table["SIO"]   = {"SIO",   0xF0, Format::F1};
    table["TIO"]   = {"TIO",   0xF4, Format::F1};

    // Format 2
    table["ADDR"]   = {"ADDR",   0x90, Format::F2};
    table["CLEAR"]  = {"CLEAR",  0xB4, Format::F2};
    table["COMPR"]  = {"COMPR",  0xA0, Format::F2};
    table["DIVR"]   = {"DIVR",   0x9C, Format::F2};
    table["MULR"]   = {"MULR",   0x98, Format::F2};
    table["RMO"]    = {"RMO",    0xAC, Format::F2};
    table["SHIFTL"] = {"SHIFTL", 0xA4, Format::F2};
    table["SHIFTR"] = {"SHIFTR", 0xA8, Format::F2};
    table["SUBR"]   = {"SUBR",   0x94, Format::F2};
    table["SVC"]    = {"SVC",    0xB0, Format::F2};
    table["TIXR"]   = {"TIXR",   0xB8, Format::F2};

    // Format 3/4
    table["ADD"]   = {"ADD",   0x18, Format::F3_4};
    table["ADDF"]  = {"ADDF",  0x58, Format::F3_4};
    table["AND"]   = {"AND",   0x40, Format::F3_4};
    table["COMP"]  = {"COMP",  0x28, Format::F3_4};
    table["COMPF"] = {"COMPF", 0x88, Format::F3_4};
    table["DIV"]   = {"DIV",   0x24, Format::F3_4};
    table["DIVF"]  = {"DIVF",  0x64, Format::F3_4};
    table["J"]     = {"J",     0x3C, Format::F3_4};
    table["JEQ"]   = {"JEQ",   0x30, Format::F3_4};
    table["JGT"]   = {"JGT",   0x34, Format::F3_4};
    table["JLT"]   = {"JLT",   0x38, Format::F3_4};
    table["JSUB"]  = {"JSUB",  0x48, Format::F3_4};
    table["LDA"]   = {"LDA",   0x00, Format::F3_4};
    table["LDB"]   = {"LDB",   0x68, Format::F3_4};
    table["LDCH"]  = {"LDCH",  0x50, Format::F3_4};
    table["LDF"]   = {"LDF",   0x70, Format::F3_4};
    table["LDL"]   = {"LDL",   0x08, Format::F3_4};
    table["LDS"]   = {"LDS",   0x6C, Format::F3_4};
    table["LDT"]   = {"LDT",   0x74, Format::F3_4};
    table["LDX"]   = {"LDX",   0x04, Format::F3_4};
    table["LPS"]   = {"LPS",   0xD0, Format::F3_4};
    table["MUL"]   = {"MUL",   0x20, Format::F3_4};
    table["MULF"]  = {"MULF",  0x60, Format::F3_4};
    table["OR"]    = {"OR",    0x44, Format::F3_4};
    table["RD"]    = {"RD",    0xE8, Format::F3_4};
    table["RSUB"]  = {"RSUB",  0x4C, Format::F3_4};
    table["SSK"]   = {"SSK",   0xD8, Format::F3_4};
    table["STA"]   = {"STA",   0x0C, Format::F3_4};
    table["STB"]   = {"STB",   0x78, Format::F3_4};
    table["STCH"]  = {"STCH",  0x54, Format::F3_4};
    table["STF"]   = {"STF",   0x80, Format::F3_4};
    table["STI"]   = {"STI",   0xD4, Format::F3_4};
    table["STL"]   = {"STL",   0x14, Format::F3_4};
    table["STS"]   = {"STS",   0x7C, Format::F3_4};
    table["STT"]   = {"STT",   0x84, Format::F3_4};
    table["STX"]   = {"STX",   0x10, Format::F3_4};
    table["SUB"]   = {"SUB",   0x1C, Format::F3_4};
    table["SUBF"]  = {"SUBF",  0x5C, Format::F3_4};
    table["TD"]    = {"TD",    0xE0, Format::F3_4};
    table["TIX"]   = {"TIX",   0x2C, Format::F3_4};
    table["WD"]    = {"WD",    0xEC, Format::F3_4};
}

bool OpcodeTable::exists(const std::string& mnemonic) const {
    return table.find(mnemonic) != table.end();
}

const OpcodeInfo& OpcodeTable::get(const std::string& mnemonic) const {
    return table.at(mnemonic);
}

bool SymbolTable::insert(const std::string& label, int address, bool isAbsolute) {
    if (exists(label)) return false;
    table[label] = {address, isAbsolute};
    return true;
}

bool SymbolTable::exists(const std::string& label) const {
    return table.find(label) != table.end();
}

int SymbolTable::getAddress(const std::string& label) const {
    return table.at(label).address;
}

void LiteralTable::add(const std::string& literal) {
    if (table.find(literal) == table.end()) {
        int len = 0;
        if (literal.length() >= 4) {
            if (literal[1] == 'C') len = literal.length() - 4;
            else if (literal[1] == 'X') len = (literal.length() - 4 + 1) / 2;
        }
        table[literal] = {literal, -1, len, false};
    }
}

std::vector<LiteralInfo*> LiteralTable::getUnassigned() {
    std::vector<LiteralInfo*> unassigned;
    for (auto& pair : table) {
        if (!pair.second.isAssigned) {
            unassigned.push_back(&pair.second);
        }
    }
    return unassigned;
}

int LiteralTable::getAddress(const std::string& literal) const {
    auto it = table.find(literal);
    if (it == table.end()) return -1;
    return it->second.address;
}
