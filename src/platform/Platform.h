#pragma once

#include "Logger.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

fs::path getApplicationDirectory();

std::tm getLocalTime(std::time_t time);

std::string httpGet(
    const std::string& url,
    Logger& logger
);

std::string httpPost(
    const std::string& url,
    const std::string& postData,
    Logger& logger
);

bool downloadItemWithSteamCMD(
    const std::vector<std::pair<std::string, std::string>>& items,
    const fs::path& appDirectory,
    Logger& logger
);

std::string detectAppID(
    const std::string& workshopID,
    Logger& logger
);
