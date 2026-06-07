#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "logger.h"

struct Config {
    std::string manifest_url;
    std::string download_dir        = "/tmp/update_agent/downloads";
    std::string install_dir         = "/tmp/update_agent/installed";
    std::string backup_dir          = "/tmp/update_agent/backups";
    int         poll_interval_seconds = 60;
    int         timeout_seconds       = 30;
    bool        dry_run               = false;

    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            Logger::warn("Config file not found: " + path + " — using defaults.");
            return true; // defaults are fine
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and blank lines
            if (line.empty() || line[0] == '#') continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key   = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            if      (key == "manifest_url")           manifest_url             = value;
            else if (key == "download_dir")           download_dir             = value;
            else if (key == "install_dir")            install_dir              = value;
            else if (key == "backup_dir")             backup_dir               = value;
            else if (key == "poll_interval_seconds")  poll_interval_seconds    = std::stoi(value);
            else if (key == "timeout_seconds")        timeout_seconds          = std::stoi(value);
            else if (key == "dry_run")                dry_run                  = (value == "true");
            else Logger::warn("Unknown config key: " + key);
        }

        Logger::info("Config loaded from: " + path);
        return true;
    }

private:
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end   = s.find_last_not_of (" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }
};
