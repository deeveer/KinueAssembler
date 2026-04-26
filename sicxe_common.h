#ifndef SICXE_COMMON_H
#define SICXE_COMMON_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <sstream>

enum class Format {
    F1, F2, F3_4, SIC_ONLY
};

struct OpcodeInfo {
    std::string mnemonic;
    unsigned char opcode;
    Format format;
};

class OpcodeTable {
public:
    OpcodeTable();
    bool exists(const std::string& mnemonic) const;
    const OpcodeInfo& get(const std::string& mnemonic) const;

private:
    std::unordered_map<std::string, OpcodeInfo> table;
};

struct SymbolInfo {
    int address;
    bool isAbsolute;
};

class SymbolTable {
public:
    bool insert(const std::string& label, int address, bool isAbsolute = false);
    bool exists(const std::string& label) const;
    int getAddress(const std::string& label) const;
    const std::unordered_map<std::string, SymbolInfo>& getTable() const { return table; }

private:
    std::unordered_map<std::string, SymbolInfo> table;
};

struct LiteralInfo {
    std::string value;
    int address;
    int length;
    bool isAssigned;
};

class LiteralTable {
public:
    void add(const std::string& literal);
    std::vector<LiteralInfo*> getUnassigned();
    int getAddress(const std::string& literal) const;
    const std::unordered_map<std::string, LiteralInfo>& getTable() const { return table; }

private:
    std::unordered_map<std::string, LiteralInfo> table;
};

struct ParsedLine {
    int address;
    std::string label;
    std::string mnemonic;
    std::string operand;
    std::string comment;
    std::string objectCode;
    bool isDirective;
    bool isFormat4;
    int length;
    int lineNumber;
    std::string rawLine;
};

#endif
