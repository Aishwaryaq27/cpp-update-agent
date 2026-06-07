#pragma once
#include "package.h"
#include <string>

class Installer {
public:
    Installer(const std::string& install_dir, const std::string& backup_dir);

    // Backs up the currently installed version of pkg.name (if any)
    void backup(const std::string& pkg_name);

    // Extracts the tarball and runs optional install_script
    bool install(const Package& pkg, const std::string& tarball_path);

    // Restores the previous backup (called on install failure)
    bool rollback(const std::string& pkg_name);

private:
    std::string m_install_dir;
    std::string m_backup_dir;

    bool extract_tarball(const std::string& tarball, const std::string& dest_dir);
    bool run_script(const std::string& script_path);
};
