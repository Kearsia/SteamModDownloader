#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

class Logger {
    std::ofstream file;

public:
    explicit Logger(const fs::path& logDirectory);
    void write(const std::string& message);
};
