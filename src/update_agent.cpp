#include "update_agent.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <future> 
#include <algorithm>
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

UpdateAgent::UpdateAgent(const Config& config)
    : m_config(config),
      m_downloader(config.download_dir, config.timeout_seconds),
      m_verifier(),
      m_installer(config.install_dir, config.backup_dir)
{}

bool UpdateAgent::init() {
    for (const auto& dir : {m_config.download_dir,
                             m_config.install_dir,
                             m_config.backup_dir}) {
        if (!fs::exists(dir)) {
            std::error_code ec;
            fs::create_directories(dir, ec);
            if (ec) {
                Logger::error("Cannot create directory: " + dir + " — " + ec.message());
                return false;
            }
        }
    }
    Logger::info("Directories verified.");
    return true;
}

void UpdateAgent::run_cycle() {
    auto packages = fetch_available_updates();
    if (packages.empty()) {
        Logger::info("No packages found in manifest.");
        return;
    }

    // Separate packages that need updating from those that don't
    std::vector<Package> to_update;
    for (const auto& pkg : packages) {
        if (!needs_update(pkg)) {
            Logger::info("[SKIP] " + pkg.name + " is up-to-date (v" + pkg.version + ")");
        } else {
            Logger::info("[QUEUED] " + pkg.name + " -> v" + pkg.version);
            to_update.push_back(pkg);
        }
    }

    if (to_update.empty()) {
        Logger::info("All packages up-to-date.");
        return;
    }

    // ── Phase 1: Parallel download + verify ──────────────────────────────
    // Each future returns: { package, local_tarball_path, success }
    struct DownloadResult {
        Package     pkg;
        std::string tarball_path;
        bool        success;
    };

    std::vector<std::future<DownloadResult>> futures;
    futures.reserve(to_update.size());

    for (const auto& pkg : to_update) {
        futures.push_back(
            std::async(std::launch::async, [this, pkg]() -> DownloadResult {
                std::string dest = m_config.download_dir + "/" +
                                   pkg.name + "-" + pkg.version + ".tar.gz";

                Logger::info("[DOWNLOAD] " + pkg.name + " from " + pkg.url);
                if (!m_downloader.download_file(pkg.url, dest)) {
                    Logger::error("[DOWNLOAD FAILED] " + pkg.name);
                    return {pkg, dest, false};
                }

                if (!pkg.checksum.empty()) {
                    if (!m_verifier.verify_sha256(dest, pkg.checksum)) {
                        Logger::error("[CHECKSUM FAILED] " + pkg.name);
                        std::filesystem::remove(dest);
                        return {pkg, dest, false};
                    }
                    Logger::info("[VERIFIED] " + pkg.name);
                }

                return {pkg, dest, true};
            })
        );
    }

    // Collect download results (wait for all to finish)
    std::vector<DownloadResult> ready;
    for (auto& f : futures) {
        auto result = f.get();
        if (result.success) {
            ready.push_back(result);
        }
    }

    Logger::info("Downloads complete. " + std::to_string(ready.size()) +
                 "/" + std::to_string(to_update.size()) + " succeeded.");

    // ── Phase 2: Serial install (filesystem operations must not race) ─────
    std::vector<std::string> updated, failed;

    for (const auto& res : ready) {
        Logger::info("[INSTALL] " + res.pkg.name);
        m_installer.backup(res.pkg.name);

        if (m_installer.install(res.pkg, res.tarball_path)) {
            // Write version stamp
            std::string stamp = m_config.install_dir + "/" + res.pkg.name + ".version";
            std::ofstream vf(stamp);
            vf << res.pkg.version << "\n";
            updated.push_back(res.pkg.name + "@" + res.pkg.version);
            Logger::info("[OK] " + res.pkg.name + " -> v" + res.pkg.version);
        } else {
            Logger::error("[FAILED] " + res.pkg.name + " — rolling back");
            m_installer.rollback(res.pkg.name);
            failed.push_back(res.pkg.name);
        }

        std::filesystem::remove(res.tarball_path);
    }

    // Mark packages that failed to download
    for (const auto& pkg : to_update) {
        bool was_ready = std::any_of(ready.begin(), ready.end(),
            [&](const DownloadResult& r){ return r.pkg.name == pkg.name; });
        if (!was_ready) failed.push_back(pkg.name + " (download/verify failed)");
    }

    report_results(updated, failed);
}

std::vector<Package> UpdateAgent::fetch_available_updates() {
    std::vector<Package> packages;
    std::string manifest_path = m_config.download_dir + "/manifest.json";

    if (!m_config.manifest_url.empty()) {
        if (!m_downloader.download_file(m_config.manifest_url, manifest_path)) {
            Logger::error("Failed to download manifest from: " + m_config.manifest_url);
            return packages;
        }
    }

    std::ifstream file(manifest_path);
    if (!file.is_open()) {
        Logger::error("Manifest not found: " + manifest_path);
        return packages;
    }

    try {
        nlohmann::json doc = nlohmann::json::parse(file);

        if (!doc.is_array()) {
            Logger::error("Manifest JSON root must be an array.");
            return packages;
        }

        for (const auto& item : doc) {
            Package pkg;
            pkg.name           = item.value("name",           "");
            pkg.version        = item.value("version",        "");
            pkg.url            = item.value("url",            "");
            pkg.checksum       = item.value("checksum",       "");
            pkg.install_script = item.value("install_script", "");

            if (pkg.name.empty() || pkg.version.empty() || pkg.url.empty()) {
                Logger::warn("Skipping incomplete package entry in manifest.");
                continue;
            }
            packages.push_back(pkg);
        }
    } catch (const nlohmann::json::parse_error& e) {
        Logger::error("Manifest JSON parse error: " + std::string(e.what()));
        return packages;
    }

    Logger::info("Manifest loaded. Packages found: " + std::to_string(packages.size()));
    return packages;
}

bool UpdateAgent::needs_update(const Package& pkg) {
    std::string version_file = m_config.install_dir + "/" + pkg.name + ".version";
    std::ifstream vf(version_file);
    if (!vf.is_open()) return true;   // Not installed

    std::string installed_version;
    std::getline(vf, installed_version);
    return installed_version != pkg.version;
}

void UpdateAgent::report_results(const std::vector<std::string>& updated,
                                  const std::vector<std::string>& failed) {
    Logger::info("--- Cycle complete ---");
    Logger::info("Updated : " + std::to_string(updated.size()));
    Logger::info("Failed  : " + std::to_string(failed.size()));

    for (const auto& u : updated) Logger::info("  [OK]   " + u);
    for (const auto& f : failed)  Logger::error("  [FAIL] " + f);

    // Write status report
    std::ofstream report(m_config.install_dir + "/last_run.txt", std::ios::trunc);
    if (report.is_open()) {
        auto now = std::time(nullptr);
        report << "Last run: " << std::ctime(&now);
        report << "Updated: " << updated.size() << "\n";
        report << "Failed:  " << failed.size()  << "\n";
        for (const auto& u : updated) report << "  OK   " << u << "\n";
        for (const auto& f : failed)  report << "  FAIL " << f << "\n";
    }
}
