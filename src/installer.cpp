#include "installer.h"
#include "logger.h"
#include <filesystem>
#include <cstdlib>
#include <stdexcept>

namespace fs = std::filesystem;

Installer::Installer(const std::string& install_dir, const std::string& backup_dir)
    : m_install_dir(install_dir), m_backup_dir(backup_dir)
{}

void Installer::backup(const std::string& pkg_name) {
    std::string pkg_dir = m_install_dir + "/" + pkg_name;
    if (!fs::exists(pkg_dir)) return;  // nothing to back up

    std::string backup_dest = m_backup_dir + "/" + pkg_name + ".bak";
    std::error_code ec;
    // Remove old backup first
    fs::remove_all(backup_dest, ec);
    fs::copy(pkg_dir, backup_dest,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec)
        Logger::warn("Backup incomplete for " + pkg_name + ": " + ec.message());
    else
        Logger::info("Backup created: " + backup_dest);
}

bool Installer::install(const Package& pkg, const std::string& tarball_path) {
    std::string pkg_dir = m_install_dir + "/" + pkg.name;
    std::error_code ec;
    fs::create_directories(pkg_dir, ec);

    // Extract tarball into pkg_dir
    if (!extract_tarball(tarball_path, pkg_dir)) {
        return false;
    }

    // Run optional post-install script
    if (!pkg.install_script.empty()) {
        std::string script_path = pkg_dir + "/" + pkg.install_script;
        if (fs::exists(script_path)) {
            Logger::info("Running install script: " + script_path);
            if (!run_script(script_path)) {
                Logger::error("Install script failed: " + script_path);
                return false;
            }
        } else {
            Logger::warn("Install script not found after extraction: " + script_path);
        }
    }

    return true;
}

bool Installer::rollback(const std::string& pkg_name) {
    std::string backup_src = m_backup_dir + "/" + pkg_name + ".bak";
    if (!fs::exists(backup_src)) {
        Logger::warn("No backup to restore for: " + pkg_name);
        return false;
    }

    std::string pkg_dir = m_install_dir + "/" + pkg_name;
    std::error_code ec;
    fs::remove_all(pkg_dir, ec);
    fs::copy(backup_src, pkg_dir,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) {
        Logger::error("Rollback failed for " + pkg_name + ": " + ec.message());
        return false;
    }
    Logger::info("Rolled back: " + pkg_name);
    return true;
}

bool Installer::extract_tarball(const std::string& tarball, const std::string& dest_dir) {
    // Use system tar — widely available on Linux; --strip-components=1 removes top dir
    std::string cmd = "tar -xzf \"" + tarball + "\" -C \"" + dest_dir +
                      "\" --strip-components=1 2>&1";
    Logger::info("Extracting: " + cmd);
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        Logger::error("tar extraction failed with code: " + std::to_string(ret));
        return false;
    }
    return true;
}

bool Installer::run_script(const std::string& script_path) {
    // Make executable and run it
    std::string cmd = "chmod +x \"" + script_path + "\" && bash \"" + script_path + "\" 2>&1";
    int ret = std::system(cmd.c_str());
    return (ret == 0);
}
