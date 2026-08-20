#include <windows.h>
#include <winhttp.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

fs::path getApplicationDirectory() {
  wchar_t buf[MAX_PATH];
  DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  if (len == 0) {
    throw std::runtime_error("Unable to determine application path.");
  }
  return fs::path(buf).parent_path();
}

class Logger {
  std::ofstream file;

  public:
  Logger(const fs::path& directory) {
    fs::create_directories(directory);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
    localtime_s(&local, &time);

    std::stringstream name;
    name << "ModDownloader_" << std::put_time(&local, "%Y-%m-%d_%H-%M-%S") << ".log";

    file.open(directory / name.str());

    if (!file) {
      throw std::runtime_error("Cannot create log file.");
    }

    write("Logger initialized.");
  }

  void write(const std::string& message) {
    if (!file) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
    localtime_s(&local, &time);

    file << "[" << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << "] " << message<< "\n";

    file.flush();
  }
};

std::string extractWorkshopID(const std::string& input) {
  const std::string& value = input;

  std::regex idPattern(R"(id=(\d+))");

  std::smatch match;

  if (std::regex_search(value, match, idPattern)) {
    return match[1];
  }

  std::regex numberPattern(R"((\d+)$)"); //regex

  if (std::regex_search(value, match, numberPattern)) {
    return match[1];
  }

  return "";
}

std::vector<std::string> readModList(const fs::path& listFile, Logger& logger) {
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

void archiveList(const fs::path& listFile, const fs::path& archiveDirectory, Logger& logger) {
  if (!fs::exists(listFile)) {
    return;
  }

  fs::create_directories(archiveDirectory);

  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::tm local{};
  localtime_s(&local, &time);

  std::stringstream name;
  name << "list_" << std::put_time(&local, "%Y-%m-%d_%H-%M-%S") << ".txt";

  fs::path destination = archiveDirectory / name.str();
  fs::rename(listFile, destination);

  logger.write("Archived list file: " + destination.string());
}

// Helper function for http connection
std::string httpGet(const std::wstring& host, const std::wstring& path, Logger& logger) {
  HINTERNET session =
      WinHttpOpen(L"ModDownloader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                  nullptr, nullptr, 0);

  if (!session) {
    logger.write("ERROR: WinHTTP session creation failed.");

    return "";
  }

  HINTERNET connection =
      WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);

  if (!connection) {
    WinHttpCloseHandle(session);

    logger.write("ERROR: WinHTTP connection failed.");

    return "";
  }

  HINTERNET request = WinHttpOpenRequest(
      connection, L"GET", path.c_str(), nullptr, nullptr,
      nullptr, WINHTTP_FLAG_SECURE);

  if (!request) {
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    logger.write("ERROR: WinHTTP request failed.");

    return "";
  }

  BOOL result = WinHttpSendRequest(request, nullptr, 0,
                                   nullptr, 0, 0, 0);

  if (!result || !WinHttpReceiveResponse(request, nullptr)) {
    logger.write("ERROR: Steam Workshop request failed.");

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return "";
  }

  std::string response;

  DWORD available = 0;

  while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
    std::vector<char> buffer(available);

    DWORD downloaded = 0;

    if (WinHttpReadData(request, buffer.data(), available, &downloaded)) {
      response.append(buffer.data(), downloaded);
    }
  }

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);

  return response;
}

// Steam application finder
// This is for finding the steam ID of the application
// It's necessary for Steamcmd to properly download item
std::string detectAppID(const std::string& workshopID, Logger& logger) {
  HINTERNET session = WinHttpOpen(L"ModDownloader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);

  if (!session) {
    logger.write("ERROR: Cannot create WinHTTP session.");
    return "";
  }

  HINTERNET connection = WinHttpConnect(session, L"api.steampowered.com", INTERNET_DEFAULT_HTTPS_PORT, 0);

  if (!connection) {
    logger.write("ERROR: Cannot connect to Steam API.");

    WinHttpCloseHandle(session);

    return "";
  }

  HINTERNET request = WinHttpOpenRequest(
      connection, L"POST", L"/ISteamRemoteStorage/GetPublishedFileDetails/v1/",
      nullptr, nullptr, nullptr,
      WINHTTP_FLAG_SECURE);

  if (!request) {
    logger.write("ERROR: Cannot create Steam API request.");

    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return "";
  }

  std::string postData = "itemcount=1&publishedfileids[0]=" + workshopID;

  std::wstring headers = L"Content-Type: application/x-www-form-urlencoded\r\n";

  BOOL sent = WinHttpSendRequest(request,headers.c_str(),-1, static_cast<LPVOID>(const_cast<char*>(postData.c_str())),
                         static_cast<DWORD>(postData.size()),
                         static_cast<DWORD>(postData.size()), 0);

  if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
    logger.write("ERROR: Steam API request failed.");

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return "";
  }

  std::string response;
  DWORD available = 0;

  do {
    available = 0;

    if (!WinHttpQueryDataAvailable(request, &available)) {
      break;
    }

    if (available == 0) break;

    std::vector<char> buffer(available);

    DWORD downloaded = 0;

    if (WinHttpReadData(request, buffer.data(), available, &downloaded)) {
      response.append(buffer.data(), downloaded);
    }

  } while (available > 0);

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);

  if (response.empty()) {
    logger.write("ERROR: Empty Steam API response for " + workshopID);

    return "";
  }

  /*
      Example response:

      {
        "publishedfiledetails": [
          {
            "result":1,
            "consumer_app_id":294100,
            "creator_app_id":294100
          }
        ]
      }
  */

  logger.write("Steam API response received for " + workshopID);

  std::regex consumerRegex(R"("consumer_app_id"\s*:\s*(\d+))");
  std::smatch match;

  if (std::regex_search(response, match, consumerRegex)) {
    std::string appID = match[1];

    logger.write("Detected AppID " + appID + " for workshop item " + workshopID);

    return appID;
  }

  std::regex creatorRegex(R"("creator_app_id"\s*:\s*(\d+))");

  if (std::regex_search(response, match, creatorRegex)) {
    std::string appID = match[1];

    logger.write("Detected AppID using creator_app_id: " + appID + " for workshop item " + workshopID);

    return appID;
  }

  logger.write("Unable to extract AppID from Steam response for " + workshopID);

  logger.write("Response preview: " + response.substr(0, std::min<size_t>(response.size(), 500)));

  return "";
}

//Structure for keeping the item info
struct WorkshopItem {
  std::string workshopID;
  std::string appID;
};
bool downloadModsWithSteamCMD(const std::vector<WorkshopItem>& items,const fs::path& appDirectory, Logger& logger) {
  fs::path steamcmd = appDirectory / "Steamcmd" / "steamcmd.exe";

  if (!fs::exists(steamcmd)) {
    logger.write("ERROR: SteamCMD executable not found: " + steamcmd.string());

    return false;
  }

  // Command construction
  // Login anonymously
  // Then send all commends to download items from workshop
  // Finally quits the SteamCmd terminal
  std::stringstream command;

  command << "\"" << steamcmd.string() << "\" " << "+login anonymous ";
  for (const auto& item : items) {
    command << "+workshop_download_item " << item.appID << " " << item.workshopID << " ";

    logger.write(
        "Added download task: "
        "AppID=" +
        item.appID + " WorkshopID=" + item.workshopID);
  }

  command << "+quit";

  logger.write("Starting SteamCMD download.");
  logger.write("SteamCMD command: " + command.str());

  int result = std::system(command.str().c_str());

  if (result != 0) {
    logger.write("ERROR: SteamCMD failed. Exit code: " + std::to_string(result));

    return false;
  }

  logger.write("SteamCMD completed successfully.");
  return true;
}

int main() {
  try {
    fs::path appDir = getApplicationDirectory();
    fs::path logDir = appDir / "logs";
    Logger logger(logDir);
    logger.write("ModDownloader started.");
    fs::path listFile = appDir / "list.txt";
    fs::path archiveDirectory = appDir / "listArchive";

    auto mods = readModList(listFile, logger);
    std::vector<WorkshopItem> items;

    for (const auto& id : mods) {
      std::string appID = detectAppID(id, logger);

      if (!appID.empty()) {
        items.push_back({id, appID});
      } else {
        logger.write("Skipped workshop item: " + id);
      }
    }

    logger.write("Resolved " + std::to_string(items.size()) + " workshop items.");

    if (mods.empty()) {
      logger.write("No workshop items found.");

      return EXIT_FAILURE;
    }

    logger.write("Mod detection phase completed.");

    bool downloadSuccess = downloadModsWithSteamCMD(items, appDir, logger);

    if (!downloadSuccess) {
      logger.write("Download failed. Application stopped.");

      return EXIT_FAILURE;
    }

    logger.write("All mods downloaded successfully.");
    archiveList(listFile, archiveDirectory, logger);
    logger.write("Source list archived.");
    logger.write("Downloader finished successfully.");

    return EXIT_SUCCESS;
  }

  catch (const std::exception& ) {
    return EXIT_FAILURE;
  }
}