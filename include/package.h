#pragma once
#include <string>

struct Package {
    std::string name;
    std::string version;
    std::string url;
    std::string checksum;       // SHA-256 hex string
    std::string install_script; // optional post-install script
};
