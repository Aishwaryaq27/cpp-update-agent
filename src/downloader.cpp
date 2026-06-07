#include "downloader.h"
#include "logger.h"
#include <curl/curl.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// libcurl write callback — streams data to an ofstream
static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* file = static_cast<std::ofstream*>(userdata);
    size_t total = size * nmemb;
    file->write(static_cast<char*>(ptr), static_cast<std::streamsize>(total));
    return file->good() ? total : 0;
}

Downloader::Downloader(const std::string& download_dir, int timeout_seconds)
    : m_download_dir(download_dir), m_timeout_seconds(timeout_seconds)
{}

bool Downloader::download_file(const std::string& url, const std::string& dest) const {
    // Ensure parent directory exists
    fs::create_directories(fs::path(dest).parent_path());

    std::ofstream out(dest, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::error("Cannot open destination file: " + dest);
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::error("curl_easy_init() failed");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        static_cast<long>(m_timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);      // follow redirects
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);      // enforce TLS verification
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      "cpp-update-agent/1.0");

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    out.close();

    if (res != CURLE_OK) {
        Logger::error("curl error: " + std::string(curl_easy_strerror(res)));
        fs::remove(dest);
        return false;
    }

    if (http_code < 200 || http_code >= 300) {
        Logger::error("HTTP " + std::to_string(http_code) + " downloading: " + url);
        fs::remove(dest);
        return false;
    }

    Logger::info("Downloaded: " + url + " -> " + dest);
    return true;
}
