#include "assembler.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <source_file.asm> [output_file.obj]" << std::endl;
        return 1;
    }

    std::string sourceFile = argv[1];
    std::string outputFile = (argc > 2) ? argv[2] : sourceFile.substr(0, sourceFile.find_last_of('.')) + ".obj";

    Assembler assem;
    if (assem.run(sourceFile, outputFile)) {
        return 0;
    } else {
        return 1;
    }
}
