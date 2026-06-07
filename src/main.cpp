#include "update_agent.h"
#include "logger.h"
#include "config.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

std::atomic<bool> g_running{true};

void signal_handler(int sig) {
    Logger::info("Signal " + std::to_string(sig) + " received. Shutting down...");
    g_running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    Logger::init("update_agent.log");
    Logger::info("=== C++ Update Agent v1.0 starting ===");

    std::string config_path = "config/agent.conf";
    if (argc > 1) config_path = argv[1];

    Config config;
    if (!config.load(config_path)) {
        Logger::error("Failed to load config: " + config_path);
        return 1;
    }

    UpdateAgent agent(config);

    if (!agent.init()) {
        Logger::error("Agent initialization failed.");
        return 1;
    }

    Logger::info("Agent initialized. Poll interval: " +
                 std::to_string(config.poll_interval_seconds) + "s");

    while (g_running) {
        Logger::info("--- Checking for updates ---");
        agent.run_cycle();

        for (int i = 0; i < config.poll_interval_seconds && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    Logger::info("=== Update Agent stopped ===");
    return 0;
}
