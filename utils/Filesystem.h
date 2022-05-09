#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

inline std::vector<char> ReadFileContent(const std::string& fileName)
{
    std::ifstream file;
    if (!std::filesystem::exists(fileName))
    {
        throw std::runtime_error("File " + fileName + " doesn't exist");
    }

    file.open(fileName, std::ifstream::binary);

    file.seekg(0, file.end);
    long bytecodeLength = file.tellg();
    file.seekg(0, file.beg);

    std::vector<char> bytecode(bytecodeLength);
    file.read(bytecode.data(), bytecodeLength);
    return bytecode;
}
