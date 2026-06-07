#pragma once
#include "config.h"
#include "package.h"
#include "downloader.h"
#include "verifier.h"
#include "installer.h"
#include "logger.h"
#include <vector>
#include <string>

class UpdateAgent {
public:
    explicit UpdateAgent(const Config& config);

    bool init();
    void run_cycle();

private:
    const Config& m_config;
    Downloader     m_downloader;
    Verifier       m_verifier;
    Installer      m_installer;

    std::vector<Package> fetch_available_updates();
    bool needs_update(const Package& pkg);
    void report_results(const std::vector<std::string>& updated,
                        const std::vector<std::string>& failed);
};
