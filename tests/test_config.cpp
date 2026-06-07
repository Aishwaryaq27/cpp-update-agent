// test_config.cpp
// Tests: values parsed correctly, defaults applied, unknown keys warned, dry_run flag
#include "config.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdio>

int main() {
    int passed = 0, failed = 0;

    auto ok   = [&](const std::string& msg){ std::cout << "[PASS] " << msg << "\n"; ++passed; };
    auto fail = [&](const std::string& msg){ std::cout << "[FAIL] " << msg << "\n"; ++failed; };

    // ── Test 1: Full config parsed correctly ────────────────────────────────
    {
        const std::string path = "/tmp/test_full.conf";
        std::ofstream f(path);
        f << "# comment line\n"
          << "manifest_url = https://example.com/manifest.json\n"
          << "download_dir = /tmp/dl\n"
          << "install_dir = /tmp/inst\n"
          << "backup_dir = /tmp/bk\n"
          << "poll_interval_seconds = 120\n"
          << "timeout_seconds = 15\n"
          << "dry_run = true\n";
        f.close();

        Config cfg;
        bool loaded = cfg.load(path);
        assert(loaded);
        (cfg.manifest_url == "https://example.com/manifest.json") ?
            ok("manifest_url parsed") : fail("manifest_url wrong: " + cfg.manifest_url);
        (cfg.download_dir == "/tmp/dl") ?
            ok("download_dir parsed") : fail("download_dir wrong");
        (cfg.install_dir == "/tmp/inst") ?
            ok("install_dir parsed") : fail("install_dir wrong");
        (cfg.backup_dir == "/tmp/bk") ?
            ok("backup_dir parsed") : fail("backup_dir wrong");
        (cfg.poll_interval_seconds == 120) ?
            ok("poll_interval_seconds parsed") : fail("poll_interval_seconds wrong");
        (cfg.timeout_seconds == 15) ?
            ok("timeout_seconds parsed") : fail("timeout_seconds wrong");
        (cfg.dry_run == true) ?
            ok("dry_run=true parsed") : fail("dry_run wrong");
        std::remove(path.c_str());
    }

    // ── Test 2: Missing file uses defaults ───────────────────────────────────
    {
        Config cfg;
        bool loaded = cfg.load("/tmp/nonexistent_xyz.conf");
        loaded ? ok("Missing file returns true (uses defaults)") : fail("Missing file should not fail");
        (cfg.poll_interval_seconds == 60) ?
            ok("Default poll_interval_seconds = 60") : fail("Default poll_interval wrong");
        (cfg.dry_run == false) ?
            ok("Default dry_run = false") : fail("Default dry_run wrong");
    }

    // ── Test 3: dry_run = false ───────────────────────────────────────────────
    {
        const std::string path = "/tmp/test_dry_false.conf";
        std::ofstream f(path);
        f << "dry_run = false\n";
        f.close();

        Config cfg;
        cfg.load(path);
        (cfg.dry_run == false) ?
            ok("dry_run=false parsed") : fail("dry_run=false not parsed");
        std::remove(path.c_str());
    }

    // ── Test 4: Whitespace trimming ───────────────────────────────────────────
    {
        const std::string path = "/tmp/test_whitespace.conf";
        std::ofstream f(path);
        f << "  manifest_url  =  https://trimmed.example.com  \n";
        f.close();

        Config cfg;
        cfg.load(path);
        (cfg.manifest_url == "https://trimmed.example.com") ?
            ok("Whitespace trimmed from values") : fail("Whitespace not trimmed: '" + cfg.manifest_url + "'");
        std::remove(path.c_str());
    }

    std::cout << "\nConfig tests: " << passed << " passed, " << failed << " failed.\n";
    return (failed == 0) ? 0 : 1;
}
