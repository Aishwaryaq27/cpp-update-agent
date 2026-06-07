// test_manifest_parser.cpp
// Tests the nlohmann/json manifest parsing via a mock manifest file
#include "nlohmann/json.hpp"
#include "package.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdio>

// Inline the parsing logic (mirrors UpdateAgent::fetch_available_updates)
std::vector<Package> parse_manifest(const std::string& path) {
    std::vector<Package> packages;
    std::ifstream file(path);
    if (!file.is_open()) return packages;

    try {
        nlohmann::json doc = nlohmann::json::parse(file);
        if (!doc.is_array()) return packages;

        for (const auto& item : doc) {
            Package pkg;
            pkg.name           = item.value("name",           "");
            pkg.version        = item.value("version",        "");
            pkg.url            = item.value("url",            "");
            pkg.checksum       = item.value("checksum",       "");
            pkg.install_script = item.value("install_script", "");
            if (pkg.name.empty() || pkg.version.empty() || pkg.url.empty()) continue;
            packages.push_back(pkg);
        }
    } catch (...) {}
    return packages;
}

int main() {
    int passed = 0, failed = 0;
    auto ok   = [&](const std::string& msg){ std::cout << "[PASS] " << msg << "\n"; ++passed; };
    auto fail = [&](const std::string& msg){ std::cout << "[FAIL] " << msg << "\n"; ++failed; };

    // ── Test 1: Standard manifest ──────────────────────────────────────────
    {
        const std::string path = "/tmp/test_manifest_standard.json";
        std::ofstream f(path);
        f << R"([
    {
        "name": "myapp",
        "version": "2.1.0",
        "url": "https://example.com/myapp-2.1.0.tar.gz",
        "checksum": "abc123",
        "install_script": "install.sh"
    }
])";
        f.close();
        auto pkgs = parse_manifest(path);
        (pkgs.size() == 1)              ? ok("Single package parsed")          : fail("Expected 1 package");
        (pkgs[0].name    == "myapp")    ? ok("name parsed correctly")          : fail("name wrong");
        (pkgs[0].version == "2.1.0")    ? ok("version parsed correctly")       : fail("version wrong");
        (pkgs[0].checksum == "abc123")  ? ok("checksum parsed correctly")      : fail("checksum wrong");
        (pkgs[0].install_script == "install.sh") ?
            ok("install_script parsed") : fail("install_script wrong");
        std::remove(path.c_str());
    }

    // ── Test 2: Multiple packages ──────────────────────────────────────────
    {
        const std::string path = "/tmp/test_manifest_multi.json";
        std::ofstream f(path);
        f << R"([
    {"name":"app-a","version":"1.0","url":"https://x.com/a.tar.gz","checksum":"","install_script":""},
    {"name":"app-b","version":"2.0","url":"https://x.com/b.tar.gz","checksum":"","install_script":""}
])";
        f.close();
        auto pkgs = parse_manifest(path);
        (pkgs.size() == 2) ? ok("Two packages parsed") : fail("Expected 2 packages, got " + std::to_string(pkgs.size()));
        std::remove(path.c_str());
    }

    // ── Test 3: Incomplete entry skipped ──────────────────────────────────
    {
        const std::string path = "/tmp/test_manifest_incomplete.json";
        std::ofstream f(path);
        f << R"([
    {"name":"good-app","version":"1.0","url":"https://x.com/good.tar.gz"},
    {"name":"no-url-app","version":"1.0"}
])";
        f.close();
        auto pkgs = parse_manifest(path);
        (pkgs.size() == 1) ? ok("Incomplete entry skipped") : fail("Expected 1 package after skip");
        std::remove(path.c_str());
    }

    // ── Test 4: Invalid JSON handled gracefully ────────────────────────────
    {
        const std::string path = "/tmp/test_manifest_bad.json";
        std::ofstream f(path);
        f << "this is not json at all {{{";
        f.close();
        auto pkgs = parse_manifest(path);
        (pkgs.empty()) ? ok("Invalid JSON returns empty list") : fail("Invalid JSON should return empty");
        std::remove(path.c_str());
    }

    // ── Test 5: Empty array ────────────────────────────────────────────────
    {
        const std::string path = "/tmp/test_manifest_empty.json";
        std::ofstream f(path);
        f << "[]";
        f.close();
        auto pkgs = parse_manifest(path);
        (pkgs.empty()) ? ok("Empty array returns empty list") : fail("Empty array should be empty");
        std::remove(path.c_str());
    }

    std::cout << "\nManifest parser tests: " << passed << " passed, " << failed << " failed.\n";
    return (failed == 0) ? 0 : 1;
}
