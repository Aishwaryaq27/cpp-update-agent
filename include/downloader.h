#pragma once
#include <string>

// Downloads files via libcurl (HTTP/HTTPS).
class Downloader {
public:
    Downloader(const std::string& download_dir, int timeout_seconds);

    // Downloads url to local path dest. Returns true on success.
    bool download_file(const std::string& url, const std::string& dest) const;

private:
    std::string m_download_dir;
    int         m_timeout_seconds;
};
