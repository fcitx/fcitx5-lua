#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }

    if (strlen(argv[1]) == 0) {
        return 1;
    }

    std::ifstream in(argv[2], std::ios::in | std::ios::binary);
    if (!in) {
        return 1;
    }

    char buffer[1024];
    size_t totalBytes = 0;

    std::cout << "char " << argv[1] << "[] = {";
    const size_t lineLength = 16;
    while (!in.eof()) {
        in.read(buffer, sizeof(buffer));
        auto bytes = in.gcount();
        for (auto i = 0; i < bytes; i++) {
            if (i % lineLength == 0) {
                std::cout << std::endl;
            }
            std::cout << " 0x" << std::hex << static_cast<int>(buffer[i])
                      << ",";
        }
        totalBytes += bytes;
    }

    std::cout << std::endl
              << "0x00};" << std::endl
              << "unsigned int " << argv[1] << "Length = " << std::dec
              << totalBytes << ";" << std::endl;

    return 0;
}
