#include "linking_loader.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName
              << " --addr <hex_address> [--map <map_file>] [--mem <memory_file>] <object_file>...\n";
}

bool parseHexAddress(const std::string& text, int& value) {
    try {
        size_t consumed = 0;
        value = std::stoi(text, &consumed, 16);
        return consumed == text.length();
    } catch (...) {
        return false;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    int loadAddress = 0;
    bool hasLoadAddress = false;
    std::string mapFile = "loading_map.txt";
    std::string memoryFile = "memory_dump.txt";
    std::vector<std::string> objectFiles;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--addr") {
            if (i + 1 >= argc || !parseHexAddress(argv[++i], loadAddress)) {
                std::cerr << "Invalid or missing --addr value.\n";
                return 1;
            }
            hasLoadAddress = true;
        } else if (arg == "--map") {
            if (i + 1 >= argc) {
                std::cerr << "Missing --map output file.\n";
                return 1;
            }
            mapFile = argv[++i];
        } else if (arg == "--mem") {
            if (i + 1 >= argc) {
                std::cerr << "Missing --mem output file.\n";
                return 1;
            }
            memoryFile = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            objectFiles.push_back(arg);
        }
    }

    if (!hasLoadAddress) {
        std::cerr << "--addr is required.\n";
        printUsage(argv[0]);
        return 1;
    }

    if (objectFiles.empty()) {
        std::cerr << "At least one object file is required.\n";
        printUsage(argv[0]);
        return 1;
    }

    LinkingLoader loader;
    if (!loader.load(objectFiles, loadAddress)) {
        return 1;
    }

    if (!loader.writeLoadingMap(mapFile)) {
        std::cerr << "Could not write loading map: " << mapFile << "\n";
        return 1;
    }

    if (!loader.writeMemoryDump(memoryFile)) {
        std::cerr << "Could not write memory dump: " << memoryFile << "\n";
        return 1;
    }

    std::cout << "Load completed successfully.\n";
    std::cout << "Loading map: " << mapFile << "\n";
    std::cout << "Memory dump: " << memoryFile << "\n";
    std::cout << "Execution address: " << std::uppercase << std::hex << loader.getExecutionAddress() << "\n";
    return 0;
}
