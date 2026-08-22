#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "Platform.h"
#include "Logger.h"

Logger::Logger(const fs::path& logDirectory) {
    fs::create_directories(logDirectory);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime = getLocalTime(time);
    std::stringstream filename;

    filename << "SWD" << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << ".log";
    file.open(logDirectory / filename.str(), std::ios::out);

    if (!file) {
        throw std::runtime_error("Cannot create log file.");
    }
    write("Logger initialized.");
}

void Logger::write(const std::string& message) {
    if (!file) { return; }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime = getLocalTime(time);
    file << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] " << message << '\n';

    file.flush();
}
