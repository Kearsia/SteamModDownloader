#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

enum class level {
    INFO = 0,
    WARN = 1,
    ERR = 2,
    FATAL = 3
};

class Logger {
    std::ofstream file;

public:
    explicit Logger(const fs::path& logDirectory);
    void write(const std::string& message, level l = level::INFO);
    static std::string printLevel(level l);
};
