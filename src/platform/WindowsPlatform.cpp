#ifdef _WIN32

#define NOMINMAX

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Platform.h"

fs::path getApplicationDirectory() {
  wchar_t buffer[MAX_PATH];

  DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

  if (length == 0) {
    throw std::runtime_error("Unable to determine application path.");
  }

  return fs::path(buffer).parent_path();
}

std::tm getLocalTime(std::time_t time) {
  std::tm local{};

  if (localtime_s(&local, &time) != 0) {
    throw std::runtime_error("Unable to convert time.");
  }

  return local;
}

static std::string httpRequest(const std::wstring& host, const std::wstring& path, const std::string& method, const std::string& postData, Logger& logger) {
  HINTERNET session = WinHttpOpen(L"SteamWorkshopDownloader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);

  if (!session) {
    logger.write("WinHTTP session creation failed.", level::ERR);
    return "";
  }

  HINTERNET connection = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);

  if (!connection) {
    logger.write(" WinHTTP connection failed.", level::ERR);
    WinHttpCloseHandle(session);

    return "";
  }

  HINTERNET request =
      WinHttpOpenRequest(connection, method == "POST" ? L"POST" : L"GET", path.c_str(), nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE);

  if (!request) {
    logger.write("WinHTTP request creation failed.", level::ERR);

    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return "";
  }

  BOOL result = FALSE;

  if (method == "POST") {
    const wchar_t* headers =
        L"Content-Type: "
        L"application/x-www-form-urlencoded\r\n";

    result = WinHttpSendRequest(
        request, headers, -1, postData.empty() ? nullptr : const_cast<char*>(postData.data()),
        static_cast<DWORD>(postData.size()), static_cast<DWORD>(postData.size()), 0);

  } else {
    result = WinHttpSendRequest(request, nullptr, 0, nullptr, 0, 0, 0);
  }

  if (!result || !WinHttpReceiveResponse(request, nullptr)) {
    logger.write("WinHTTP request failed.", level::ERR);

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
    if (!WinHttpReadData(request, buffer.data(), available, &downloaded)) {
      logger.write("WinHTTP read failed.", level::ERR);
      break;
    }
    response.append(buffer.data(), downloaded);
  }

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);

  return response;
}

// URL Parser
static std::pair<std::wstring, std::wstring> parseUrl(const std::string& url) {
  const std::string prefix = "https://";

  if (url.rfind(prefix, 0) != 0) {
    throw std::runtime_error("Only HTTPS URLs are supported.");
  }

  std::string remainder = url.substr(prefix.size());
  size_t slash = remainder.find('/');

  if (slash == std::string::npos) {
    throw std::runtime_error("Invalid URL.");
  }

  std::string host = remainder.substr(0, slash);
  std::string path = remainder.substr(slash);

  int hostLength = MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, nullptr, 0);
  std::wstring wideHost(hostLength, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, wideHost.data(), hostLength);

  int pathLength = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  std::wstring widePath(pathLength, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath.data(), pathLength);

  return {wideHost, widePath};
}

// GET
std::string httpGet(const std::string& url, Logger& logger) {
  try {
    auto [host, path] = parseUrl(url);
    return httpRequest(host, path, "GET", "", logger);
  } catch (const std::exception& exception) {
    logger.write(exception.what(), level::ERR);
    return "";
  }
}

// POST
std::string httpPost(const std::string& url, const std::string& postData, Logger& logger) {
  try {
    auto [host, path] = parseUrl(url);
    return httpRequest(host, path, "POST", postData, logger);
  } catch (const std::exception& exception) {
    logger.write(exception.what(), level::ERR);
    return "";
  }
}

bool downloadItemWithSteamCMD(const std::vector<std::pair<std::string, std::string>>& items, const fs::path& appDirectory, Logger& logger) {
  fs::path steamcmd = appDirectory / "Steamcmd" / "steamcmd.exe";

  if (!fs::exists(steamcmd)) {
    logger.write("SteamCMD executable not found: " + steamcmd.string(), level::ERR);
    return false;
  }

  std::stringstream command;
  command << "\"" << steamcmd.string() << "\" " << "+login anonymous ";

  for (const auto& item : items) {
    const std::string& workshopID = item.first;
    const std::string& appID = item.second;
    command << "+workshop_download_item " << appID << " " << workshopID << " ";
    logger.write(
        "Added download task: "
        "AppID=" +
        appID + " WorkshopID=" + workshopID);
  }

  command << "+quit";

  logger.write("Starting SteamCMD download.");
  logger.write("SteamCMD command: " + command.str());

  int result = std::system(command.str().c_str());

  if (result != 0) {
    logger.write("SteamCMD failed. Exit code: " + std::to_string(result), level::ERR);
    return false;
  }
  logger.write("SteamCMD completed successfully.");

  return true;
}

std::string detectAppID(const std::string& workshopID, Logger& logger) {
  const std::string url = "https://api.steampowered.com/" "ISteamRemoteStorage/" "GetPublishedFileDetails/v1/";

  const std::string postData = "itemcount=1&publishedfileids[0]=" + workshopID;
  std::string response = httpPost(url, postData, logger);

  if (response.empty()) {
    logger.write("Empty Steam API response for " + workshopID, level::ERR);
    return "";
  }

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

  logger.write("Unable to extract AppID from Steam response for " + workshopID, level::WARN);
  logger.write("Response preview: " + response.substr(0, std::min<size_t>(response.size(), 500)));
  return "";
}

#endif
