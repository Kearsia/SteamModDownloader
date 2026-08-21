#ifdef __linux__

#include <openssl/bio.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Platform.h"

fs::path getApplicationDirectory() {
  std::error_code error;
  fs::path executable = fs::read_symlink("/proc/self/exe", error);

  if (error) {
    throw std::runtime_error("Unable to determine application path: " + error.message());
  }

  return executable.parent_path();
}

std::tm getLocalTime(std::time_t time) {
  std::tm local{};
  if (localtime_r(&time, &local) == nullptr) {
    throw std::runtime_error("Unable to convert time.");
  }

  return local;
}

static std::string decodeChunked(const std::string& body) {
  std::string result;
  size_t position = 0;

  while (position < body.size()) {
    size_t lineEnd = body.find("\r\n", position);

    if (lineEnd == std::string::npos) {
      break;
    }

    std::string sizeText = body.substr(position, lineEnd - position);
    size_t semicolon = sizeText.find(';');

    if (semicolon != std::string::npos) {
      sizeText = sizeText.substr(0, semicolon);
    }

    size_t chunkSize = 0;
    std::stringstream stream;

    stream << std::hex << sizeText;
    stream >> chunkSize;

    position = lineEnd + 2;
    if (chunkSize == 0) {
      break;
    }

    if (position + chunkSize > body.size()) {
      break;
    }

    result.append(body, position, chunkSize);
    position += chunkSize;

    if (position + 2 <= body.size()) {
      position += 2;
    }
  }

  return result;
}

static std::pair<std::string, std::string> parseUrl(const std::string& url) {
  const std::string prefix = "https://";

  if (url.rfind(prefix, 0) != 0) {
    throw std::runtime_error("Only HTTPS URLs are supported.");
  }

  std::string remainder = url.substr(prefix.size());
  size_t slash = remainder.find('/');

  if (slash == std::string::npos) {
    throw std::runtime_error("Invalid URL.");
  }
  return {remainder.substr(0, slash),remainder.substr(slash)};
}

static std::string httpsRequest(const std::string& host, const std::string& path, const std::string& method, const std::string& postData, Logger& logger) {
  SSL_CTX* context = SSL_CTX_new(TLS_client_method());

  if (!context) {
    logger.write("ERROR: Unable to create OpenSSL context.");

    return "";
  }

  SSL_CTX_set_default_verify_paths(context);
  BIO* bio = BIO_new_ssl_connect(context);

  if (!bio) {
    logger.write("ERROR: Unable to create OpenSSL BIO.");
    SSL_CTX_free(context);
    return "";
  }

  SSL* ssl = nullptr;
  BIO_get_ssl(bio, &ssl);

  if (!ssl) {
    logger.write("ERROR: Unable to initialize SSL.");
    BIO_free_all(bio);
    SSL_CTX_free(context);

    return "";
  }

  SSL_set_verify(ssl, SSL_VERIFY_PEER, nullptr);
  SSL_set_tlsext_host_name(ssl, host.c_str());
  BIO_set_conn_hostname(bio, (host + ":443").c_str());

  if (BIO_do_connect(bio) <= 0) {
    logger.write("ERROR: Unable to connect to " + host);

    BIO_free_all(bio);
    SSL_CTX_free(context);

    return "";
  }

  if (BIO_do_handshake(bio) <= 0) {
    logger.write("ERROR: TLS handshake failed.");
    BIO_free_all(bio);
    SSL_CTX_free(context);
    return "";
  }

  // HTTP Build

  std::stringstream request;
  request << method << " " << path << " HTTP/1.1\r\n";
  request << "Host: " << host << "\r\n";
  request << "User-Agent: ModDownloader/1.0\r\n";
  request << "Connection: close\r\n";

  if (method == "POST") {
    request << "Content-Type: " << "application/x-www-form-urlencoded\r\n";
    request << "Content-Length: " << postData.size() << "\r\n";
  }

  request << "\r\n";

  if (method == "POST") {
    request << postData;
  }

  std::string requestData = request.str();

  // Send
  int written = BIO_write(bio, requestData.data(), static_cast<int>(requestData.size()));
  if (written <= 0) {
    logger.write("ERROR: Failed to send HTTPS request.");
    BIO_free_all(bio);
    SSL_CTX_free(context);
    return "";
  }

  // Recieve
  std::string response;
  char buffer[8192];
  while (true) {
    int bytesRead = BIO_read(bio, buffer, sizeof(buffer));
    if (bytesRead > 0) {
      response.append(buffer, bytesRead);
      continue;
    }

    if (BIO_should_retry(bio)) {
      continue;
    }
    break;
  }

  BIO_free_all(bio);
  SSL_CTX_free(context);

  // Split headers/body
  size_t headerEnd = response.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    logger.write("ERROR: Invalid HTTP response.");
    return "";
  }
  std::string headers = response.substr(0, headerEnd);
  std::string body = response.substr(headerEnd + 4);

  // Status
  size_t firstSpace = headers.find(' ');
  if (firstSpace != std::string::npos) {
    size_t secondSpace = headers.find(' ', firstSpace + 1);
    if (secondSpace != std::string::npos) {
      std::string status = headers.substr(firstSpace + 1, secondSpace - firstSpace - 1);
      try {
        int statusCode = std::stoi(status);
        if (statusCode < 200 || statusCode >= 300) {
          logger.write("ERROR: HTTP status " + std::to_string(statusCode));
          return "";
        }
      } catch (...) {
        logger.write("ERROR: Invalid HTTP status.");
        return "";
      }
    }
  }

  // Response in chunks
  std::string lowerHeaders = headers;
  std::transform(
      lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(),
      [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
  if (lowerHeaders.find("transfer-encoding: chunked") != std::string::npos) {
    body = decodeChunked(body);
  }
  return body;
}

// GET
std::string httpGet(const std::string& url, Logger& logger) {
  try {
    auto [host, path] = parseUrl(url);
    return httpsRequest(host, path, "GET", "", logger);

  } catch (const std::exception& exception) {
    logger.write(std::string("ERROR: ") + exception.what());
    return "";
  }
}

// POST
std::string httpPost(const std::string& url, const std::string& postData, Logger& logger) {
  try {
    auto [host, path] = parseUrl(url);
    return httpsRequest(host, path, "POST", postData, logger);

  } catch (const std::exception& exception) {
    logger.write(std::string("ERROR: ") + exception.what());
    return "";
  }
}

bool downloadModsWithSteamCMD(const std::vector<std::pair<std::string, std::string>>& items, const fs::path& appDirectory, Logger& logger) {
  fs::path steamcmd = appDirectory / "Steamcmd" / "steamcmd.sh";

  if (!fs::exists(steamcmd)) {
    logger.write("ERROR: SteamCMD executable not found: " + steamcmd.string());
    return false;
  }

  std::stringstream command;
  command << "bash " << "'" << steamcmd.string() << "' " << "+login anonymous ";

  for (const auto& item : items) {
    const std::string& workshopID = item.first;
    const std::string& appID = item.second;
    command << "+workshop_download_item " << appID << " " << workshopID << " ";
    logger.write("Added download task: " "AppID=" + appID + " WorkshopID=" + workshopID);
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

std::string detectAppID(const std::string& workshopID, Logger& logger) {
  const std::string url = "https://api.steampowered.com/" "ISteamRemoteStorage/" "GetPublishedFileDetails/v1/";
  const std::string postData = "itemcount=1&publishedfileids[0]=" + workshopID;

  std::string response = httpPost(url, postData, logger);
  if (response.empty()) {
    logger.write("ERROR: Empty Steam API response for " + workshopID);

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
  logger.write("Unable to extract AppID from Steam response for " + workshopID);
  logger.write("Response preview: " + response.substr(0, std::min<size_t>(response.size(), 500)));

  return "";
}

#endif
