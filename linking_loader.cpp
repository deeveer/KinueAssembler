#include "linking_loader.h"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

LinkingLoader::LinkingLoader()
    : programAddress(0), executionAddress(0), executionAddressSet(false), hasError(false) {}

bool LinkingLoader::load(const std::vector<std::string>& objectFiles, int loadAddress) {
    reset();
    programAddress = loadAddress;
    executionAddress = loadAddress;

    if (objectFiles.empty()) {
        reportError("No object files were provided.");
        return false;
    }

    if (!pass1(objectFiles)) return false;
    if (!pass2(objectFiles)) return false;

    return !hasError;
}

bool LinkingLoader::pass1(const std::vector<std::string>& objectFiles) {
    int csaddr = programAddress;

    for (const auto& objectFile : objectFiles) {
        std::vector<ObjectLine> lines;
        if (!readObjectFile(objectFile, lines)) return false;

        bool inSection = false;
        int cslth = 0;
        int inputStart = 0;

        for (const auto& line : lines) {
            Record record;
            if (!parseRecord(line, record)) continue;
            if (!handlePass1Record(record, inSection, csaddr, cslth, inputStart)) continue;
        }

        if (inSection) {
            reportError(lines.back(), "Object file ended before an E record.");
        }
    }

    return !hasError;
}

bool LinkingLoader::pass2(const std::vector<std::string>& objectFiles) {
    int csaddr = programAddress;

    for (const auto& objectFile : objectFiles) {
        std::vector<ObjectLine> lines;
        if (!readObjectFile(objectFile, lines)) return false;

        bool inSection = false;
        int cslth = 0;
        int inputStart = 0;

        for (const auto& line : lines) {
            Record record;
            if (!parseRecord(line, record)) continue;
            if (!handlePass2Record(record, inSection, csaddr, cslth, inputStart)) continue;
        }

        if (inSection) {
            reportError(lines.back(), "Object file ended before an E record.");
        }
    }

    return !hasError;
}

bool LinkingLoader::readObjectFile(const std::string& objectFile, std::vector<ObjectLine>& lines) {
    std::ifstream file(objectFile);
    if (!file.is_open()) {
        reportError("Could not open object file: " + objectFile);
        return false;
    }

    std::string text;
    int lineNumber = 0;
    while (std::getline(file, text)) {
        lineNumber++;
        if (!text.empty() && text.back() == '\r') text.pop_back();
        if (trim(text).empty()) continue;
        lines.push_back({objectFile, lineNumber, text});
    }

    if (lines.empty()) {
        reportError("Object file is empty: " + objectFile);
        return false;
    }

    return true;
}

bool LinkingLoader::parseRecord(const ObjectLine& line, Record& record) {
    std::string text = trim(line.text);
    record = Record();
    record.type = text.empty() ? '\0' : text[0];
    record.address = 0;
    record.length = 0;
    record.source = line;

    if (text.empty()) {
        reportError(line, "Empty record.");
        return false;
    }

    switch (record.type) {
    case 'H': {
        if (text.length() < 19) {
            reportError(line, "Header record must contain name, start address, and length.");
            return false;
        }
        record.name = trim(text.substr(1, 6));
        if (!parseHex(text.substr(7, 6), record.address, line, "header start address")) return false;
        if (!parseHex(text.substr(13, 6), record.length, line, "control section length")) return false;
        if (record.name.empty()) {
            reportError(line, "Control section name is empty.");
            return false;
        }
        return true;
    }
    case 'D': {
        std::string rest = text.substr(1);
        if (rest.length() % 12 != 0) {
            reportError(line, "Define record must contain 12-character symbol/address groups.");
            return false;
        }
        for (size_t i = 0; i < rest.length(); i += 12) {
            SymbolEntry entry;
            entry.name = trim(rest.substr(i, 6));
            if (entry.name.empty()) {
                reportError(line, "Define record contains an empty symbol name.");
                return false;
            }
            if (!parseHex(rest.substr(i + 6, 6), entry.address, line, "external symbol address")) return false;
            record.definitions.push_back(entry);
        }
        return true;
    }
    case 'R':
        // Reference records are parsed as known records for forward compatibility.
        return true;
    case 'T': {
        if (text.length() < 9) {
            reportError(line, "Text record must contain start address and byte length.");
            return false;
        }
        if (!parseHex(text.substr(1, 6), record.address, line, "text start address")) return false;
        if (!parseHex(text.substr(7, 2), record.length, line, "text byte length")) return false;
        record.objectCode = text.substr(9);
        if (record.objectCode.length() != static_cast<size_t>(record.length * 2)) {
            reportError(line, "Text record byte length does not match object code length.");
            return false;
        }
        if (!isHexString(record.objectCode)) {
            reportError(line, "Text record contains non-hex object code.");
            return false;
        }
        return true;
    }
    case 'M': {
        if (text.length() < 9) {
            reportError(line, "Modification record must contain address and half-byte length.");
            return false;
        }
        if (!parseHex(text.substr(1, 6), record.address, line, "modification address")) return false;
        if (!parseHex(text.substr(7, 2), record.length, line, "modification half-byte length")) return false;
        record.modificationExpression = trim(text.substr(9));
        if (!record.modificationExpression.empty()) {
            char sign = record.modificationExpression[0];
            std::string symbol = trim(record.modificationExpression.substr(1));
            if ((sign != '+' && sign != '-') || symbol.empty()) {
                reportError(line, "Modification expression must be +SYMBOL or -SYMBOL.");
                return false;
            }
        }
        return true;
    }
    case 'E': {
        std::string operand = trim(text.substr(1));
        if (!operand.empty() && !parseHex(operand, record.address, line, "execution address")) return false;
        return true;
    }
    default:
        reportError(line, std::string("Invalid object record type: ") + record.type);
        return false;
    }
}

bool LinkingLoader::handlePass1Record(const Record& record, bool& inSection, int& csaddr, int& cslth, int& inputStart) {
    if (record.type != 'H' && !inSection) {
        reportError(record.source, "Record appears before a header record.");
        return false;
    }

    switch (record.type) {
    case 'H': {
        if (inSection) {
            reportError(record.source, "New header appears before previous control section ended.");
            return false;
        }
        if (estab.find(record.name) != estab.end()) {
            reportError(record.source, "Duplicate control section or external symbol: " + record.name);
            return false;
        }

        cslth = record.length;
        inputStart = record.address;
        inSection = true;
        controlSections.push_back({record.name, csaddr, cslth});
        estab[record.name] = {record.name, csaddr, record.name};
        return true;
    }
    case 'D':
        for (const auto& definition : record.definitions) {
            if (estab.find(definition.name) != estab.end()) {
                reportError(record.source, "Duplicate external symbol: " + definition.name);
                continue;
            }
            if (definition.address > cslth) {
                reportError(record.source, "External symbol address exceeds control section length: " + definition.name);
                continue;
            }
            const std::string sectionName = controlSections.empty() ? "" : controlSections.back().name;
            estab[definition.name] = {definition.name, csaddr + definition.address, sectionName};
        }
        return true;
    case 'E':
        csaddr += cslth;
        cslth = 0;
        inSection = false;
        return true;
    default:
        return true;
    }
}

bool LinkingLoader::handlePass2Record(const Record& record, bool& inSection, int& csaddr, int& cslth, int& inputStart) {
    if (record.type != 'H' && !inSection) {
        reportError(record.source, "Record appears before a header record.");
        return false;
    }

    switch (record.type) {
    case 'H':
        if (inSection) {
            reportError(record.source, "New header appears before previous control section ended.");
            return false;
        }
        inSection = true;
        cslth = record.length;
        inputStart = record.address;
        return true;
    case 'T':
        return loadTextRecord(record, csaddr, cslth, inputStart);
    case 'M':
        return applyModificationRecord(record, csaddr, cslth, inputStart);
    case 'E':
        if (!executionAddressSet && !trim(record.source.text.substr(1)).empty()) {
            executionAddress = csaddr + (record.address - inputStart);
            executionAddressSet = true;
        }
        csaddr += cslth;
        cslth = 0;
        inSection = false;
        return true;
    default:
        return true;
    }
}

bool LinkingLoader::loadTextRecord(const Record& record, int csaddr, int cslth, int inputStart) {
    int relativeAddress = record.address - inputStart;
    if (relativeAddress < 0 || relativeAddress + record.length > cslth) {
        reportError(record.source, "Text record exceeds current control section length.");
        return false;
    }

    int absoluteAddress = csaddr + relativeAddress;
    for (int i = 0; i < record.length; ++i) {
        int byteValue = 0;
        if (!parseHex(record.objectCode.substr(i * 2, 2), byteValue, record.source, "text byte")) return false;
        if (!putMemoryByte(absoluteAddress + i, static_cast<unsigned char>(byteValue), record.source, false)) {
            return false;
        }
    }
    return true;
}

bool LinkingLoader::applyModificationRecord(const Record& record, int csaddr, int cslth, int inputStart) {
    int byteCount = (record.length + 1) / 2;
    int relativeAddress = record.address - inputStart;
    if (relativeAddress < 0 || relativeAddress + byteCount > cslth) {
        reportError(record.source, "Modification field exceeds current control section length.");
        return false;
    }

    int fieldAddress = csaddr + relativeAddress;
    int fieldValue = 0;
    if (!readModificationField(fieldAddress, record.length, fieldValue)) {
        reportError(record.source, "Modification field refers to unloaded memory.");
        return false;
    }

    bool ok = true;
    int delta = resolveModificationValue(record.modificationExpression, csaddr, record.source, ok);
    if (!ok) return false;

    int mask = (record.length >= 8) ? -1 : ((1 << (record.length * 4)) - 1);
    int modifiedValue = (fieldValue + delta) & mask;
    return writeModificationField(fieldAddress, record.length, modifiedValue);
}

bool LinkingLoader::readModificationField(int byteAddress, int halfByteLength, int& value) const {
    value = 0;
    bool startsAtLowNibble = (halfByteLength % 2) == 1;
    int startNibble = byteAddress * 2 + (startsAtLowNibble ? 1 : 0);

    for (int i = 0; i < halfByteLength; ++i) {
        int nibbleIndex = startNibble + i;
        unsigned char byteValue = 0;
        if (!getMemoryByte(nibbleIndex / 2, byteValue)) return false;

        int nibble = (nibbleIndex % 2 == 0) ? ((byteValue >> 4) & 0x0F) : (byteValue & 0x0F);
        value = (value << 4) | nibble;
    }

    return true;
}

bool LinkingLoader::writeModificationField(int byteAddress, int halfByteLength, int value) {
    bool startsAtLowNibble = (halfByteLength % 2) == 1;
    int startNibble = byteAddress * 2 + (startsAtLowNibble ? 1 : 0);

    for (int i = halfByteLength - 1; i >= 0; --i) {
        int nibbleIndex = startNibble + i;
        int nibble = value & 0x0F;
        value >>= 4;

        unsigned char byteValue = 0;
        if (!getMemoryByte(nibbleIndex / 2, byteValue)) return false;

        if (nibbleIndex % 2 == 0) {
            byteValue = static_cast<unsigned char>((byteValue & 0x0F) | (nibble << 4));
        } else {
            byteValue = static_cast<unsigned char>((byteValue & 0xF0) | nibble);
        }

        memory[nibbleIndex / 2] = byteValue;
    }

    return true;
}

int LinkingLoader::resolveModificationValue(const std::string& expression, int csaddr, const ObjectLine& source, bool& ok) {
    ok = true;
    if (expression.empty()) {
        return csaddr;
    }

    char sign = expression[0];
    std::string symbol = trim(expression.substr(1));
    auto it = estab.find(symbol);
    if (it == estab.end()) {
        reportError(source, "Undefined external symbol: " + symbol);
        ok = false;
        return 0;
    }

    return sign == '-' ? -it->second.address : it->second.address;
}

bool LinkingLoader::putMemoryByte(int address, unsigned char value, const ObjectLine& source, bool allowOverwrite) {
    if (!allowOverwrite && memory.find(address) != memory.end()) {
        reportError(source, "Text record overlaps previously loaded memory at " + hexString(address, 6) + ".");
        return false;
    }
    memory[address] = value;
    return true;
}

bool LinkingLoader::getMemoryByte(int address, unsigned char& value) const {
    auto it = memory.find(address);
    if (it == memory.end()) return false;
    value = it->second;
    return true;
}

bool LinkingLoader::writeLoadingMap(const std::string& outputFile) const {
    std::ofstream file(outputFile);
    if (!file.is_open()) return false;

    file << "Control Section   Symbol   Address   Length\n";
    for (const auto& section : controlSections) {
        file << std::left << std::setw(18) << section.name
             << std::setw(9) << ""
             << hexString(section.startAddress, 6) << "    "
             << hexString(section.length, 6) << "\n";

        std::vector<ExternalSymbol> symbols;
        for (const auto& pair : estab) {
            const auto& symbol = pair.second;
            if (symbol.controlSection == section.name && symbol.name != section.name) {
                symbols.push_back(symbol);
            }
        }

        for (size_t i = 0; i < symbols.size(); ++i) {
            for (size_t j = i + 1; j < symbols.size(); ++j) {
                if (symbols[j].address < symbols[i].address ||
                    (symbols[j].address == symbols[i].address && symbols[j].name < symbols[i].name)) {
                    ExternalSymbol temp = symbols[i];
                    symbols[i] = symbols[j];
                    symbols[j] = temp;
                }
            }
        }

        for (const auto& symbol : symbols) {
            file << std::left << std::setw(18) << ""
                 << std::setw(9) << symbol.name
                 << hexString(symbol.address, 6) << "\n";
        }
    }
    file << "Execution Address          " << hexString(executionAddress, 6) << "\n";

    return true;
}

bool LinkingLoader::writeMemoryDump(const std::string& outputFile) const {
    std::ofstream file(outputFile);
    if (!file.is_open()) return false;

    file << "Address  Bytes\n";
    if (memory.empty()) return true;

    auto it = memory.begin();
    while (it != memory.end()) {
        int rowStart = it->first;
        int rowEnd = rowStart;
        std::vector<unsigned char> bytes;

        while (it != memory.end() && it->first == rowEnd && bytes.size() < 16) {
            bytes.push_back(it->second);
            ++rowEnd;
            ++it;
        }

        file << hexString(rowStart, 6) << "   ";
        for (size_t i = 0; i < bytes.size(); ++i) {
            if (i > 0) file << " ";
            file << hexString(bytes[i], 2);
        }
        file << "\n";
    }

    return true;
}

std::string LinkingLoader::trim(const std::string& value) const {
    size_t first = 0;
    while (first < value.length() && std::isspace(static_cast<unsigned char>(value[first]))) first++;

    size_t last = value.length();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) last--;

    return value.substr(first, last - first);
}

std::string LinkingLoader::hexString(int value, int width) const {
    std::stringstream ss;
    int shift = width * 4;
    unsigned int mask = shift >= 32 ? 0xFFFFFFFFu : ((1u << shift) - 1u);
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(width)
       << (static_cast<unsigned int>(value) & mask);
    return ss.str();
}

bool LinkingLoader::parseHex(const std::string& text, int& value, const ObjectLine& source, const std::string& fieldName) {
    if (text.empty() || !isHexString(text)) {
        reportError(source, "Invalid hexadecimal " + fieldName + ": " + text);
        return false;
    }

    try {
        value = std::stoi(text, nullptr, 16);
        return true;
    } catch (...) {
        reportError(source, "Invalid hexadecimal " + fieldName + ": " + text);
        return false;
    }
}

bool LinkingLoader::isHexString(const std::string& text) const {
    for (char ch : text) {
        if (!std::isxdigit(static_cast<unsigned char>(ch))) return false;
    }
    return true;
}

void LinkingLoader::reportError(const ObjectLine& source, const std::string& message) {
    std::stringstream ss;
    ss << source.fileName << ":" << source.lineNumber << ": " << message;
    errors.push_back(ss.str());
    std::cerr << "Error: " << ss.str() << std::endl;
    hasError = true;
}

void LinkingLoader::reportError(const std::string& message) {
    errors.push_back(message);
    std::cerr << "Error: " << message << std::endl;
    hasError = true;
}

void LinkingLoader::reset() {
    estab.clear();
    controlSections.clear();
    memory.clear();
    errors.clear();
    programAddress = 0;
    executionAddress = 0;
    executionAddressSet = false;
    hasError = false;
}
