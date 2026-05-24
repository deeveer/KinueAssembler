#ifndef LINKING_LOADER_H
#define LINKING_LOADER_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct ControlSection {
    std::string name;
    int startAddress;
    int length;
};

struct ExternalSymbol {
    std::string name;
    int address;
    std::string controlSection;
};

class LinkingLoader {
public:
    LinkingLoader();

    bool load(const std::vector<std::string>& objectFiles, int loadAddress);
    bool writeLoadingMap(const std::string& outputFile) const;
    bool writeMemoryDump(const std::string& outputFile) const;

    int getExecutionAddress() const { return executionAddress; }
    bool hasErrors() const { return hasError; }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    struct ObjectLine {
        std::string fileName;
        int lineNumber;
        std::string text;
    };

    struct SymbolEntry {
        std::string name;
        int address;
    };

    struct Record {
        char type;
        std::string name;
        int address;
        int length;
        std::string objectCode;
        std::vector<SymbolEntry> definitions;
        std::string modificationExpression;
        ObjectLine source;
    };

    bool pass1(const std::vector<std::string>& objectFiles);
    bool pass2(const std::vector<std::string>& objectFiles);
    bool readObjectFile(const std::string& objectFile, std::vector<ObjectLine>& lines);
    bool parseRecord(const ObjectLine& line, Record& record);

    bool handlePass1Record(const Record& record, bool& inSection, int& csaddr, int& cslth, int& inputStart);
    bool handlePass2Record(const Record& record, bool& inSection, int& csaddr, int& cslth, int& inputStart);

    bool loadTextRecord(const Record& record, int csaddr, int cslth, int inputStart);
    bool applyModificationRecord(const Record& record, int csaddr, int cslth, int inputStart);
    bool readModificationField(int byteAddress, int halfByteLength, int& value) const;
    bool writeModificationField(int byteAddress, int halfByteLength, int value);

    int resolveModificationValue(const std::string& expression, int csaddr, const ObjectLine& source, bool& ok);
    bool putMemoryByte(int address, unsigned char value, const ObjectLine& source, bool allowOverwrite);
    bool getMemoryByte(int address, unsigned char& value) const;

    std::string trim(const std::string& value) const;
    std::string hexString(int value, int width) const;
    bool parseHex(const std::string& text, int& value, const ObjectLine& source, const std::string& fieldName);
    bool isHexString(const std::string& text) const;
    void reportError(const ObjectLine& source, const std::string& message);
    void reportError(const std::string& message);
    void reset();

    std::unordered_map<std::string, ExternalSymbol> estab;
    std::vector<ControlSection> controlSections;
    std::map<int, unsigned char> memory;
    std::vector<std::string> errors;

    int programAddress;
    int executionAddress;
    bool executionAddressSet;
    bool hasError;
};

#endif
