#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Platform.h"

namespace fs = std::filesystem;

std::string extractWorkshopID(const std::string& input) {
  std::regex idPattern(R"(id=(\d+))");
  std::smatch match;

  if (std::regex_search(input, match, idPattern)) {
    return match[1];
  }

  std::regex numberPattern(R"((\d+)$)");
  if (std::regex_search(input, match, numberPattern)) {
    return match[1];
  }

  return "";
}

std::vector<std::string> readWorkshopList(const fs::path& listFile, Logger& logger) {
  std::vector<std::string> result;
  std::ifstream file(listFile);

  if (!file) {
    logger.write("list.txt not found.");
    throw std::runtime_error("list.txt not found.");
  }

  std::string line;

  while (std::getline(file, line)) {
    std::string id = extractWorkshopID(line);
    if (!id.empty()) {
      result.push_back(id);
      logger.write("Detected workshop ID: " + id);
    }
  }

  logger.write("Loaded " + std::to_string(result.size()) + " workshop items.");
  return result;
}

int main() {
  try {
    fs::path appDirectory = getApplicationDirectory();  // where the executable is located
    fs::path logDirectory = appDirectory / "logs";   // logs
    fs::path listFile = appDirectory / "list.txt";   // seek for list file in executable is located
    fs::path archiveDirectory = appDirectory / "listArchive";

    Logger logger(logDirectory);
    logger.write("SteamWorkshopDownloader started.");
    auto workshopItems = readWorkshopList(listFile, logger);  // get all ids from list file

    if (workshopItems.empty()) {
      logger.write("No workshop items found.");
      return EXIT_FAILURE;
    }

    std::vector<std::pair<std::string, std::string> > items;

    for (const auto& workshopID : workshopItems) {
      std::string appID = detectAppID(workshopID, logger);

      if (!appID.empty()) {
        items.emplace_back(workshopID, appID);
      } else {
        logger.write("Skipped workshop item: " + workshopID);
      }
    }

    logger.write("Resolved " + std::to_string(items.size()) + " workshop items.");

    if (items.empty()) {
      logger.write("No valid workshop items found.");
      return EXIT_FAILURE;
    }

    logger.write("Workshop item detection phase completed.");
    bool downloadSuccess = downloadItemWithSteamCMD(items, appDirectory, logger);

    if (!downloadSuccess) {
      logger.write("Download failed. Application stopped.");
      return EXIT_FAILURE;
    }

    logger.write("All Workshop items downloaded successfully.");
    fs::create_directories(archiveDirectory);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm local = getLocalTime(time);
    std::stringstream filename;

    filename << "list_" << std::put_time(&local, "%Y-%m-%d_%H-%M-%S") << ".txt";

    fs::path destination = archiveDirectory / filename.str();
    std::error_code error;

    fs::rename(listFile, destination, error);

    if (error) {
      logger.write("Unable to archive list file: " + error.message(), level::ERR);
    } else {
      logger.write("Archived list file: " + destination.string());
    }

    logger.write("Source list archived.");
    logger.write("Downloader finished successfully.");

    return EXIT_SUCCESS;

  } catch (const std::exception& exception) {
    std::cerr << "Fatal error: " << exception.what() << '\n';

    return EXIT_FAILURE;
  }
}
