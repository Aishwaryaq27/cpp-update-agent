#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <ctime>

class Logger {
public:
    enum class Level { INFO, WARN, ERROR };

    static void init(const std::string& log_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_file_.open(log_path, std::ios::app);
        if (!log_file_.is_open())
            std::cerr << "[WARN] Could not open log file: " << log_path << "\n";
    }

    static void info (const std::string& msg) { log(Level::INFO,  msg); }
    static void warn (const std::string& msg) { log(Level::WARN,  msg); }
    static void error(const std::string& msg) { log(Level::ERROR, msg); }

private:
    static void log(Level level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string ts   = timestamp();
        std::string tag  = (level == Level::INFO  ? "[INFO] " :
                            level == Level::WARN  ? "[WARN] " : "[ERROR]");
        std::string line = ts + " " + tag + " " + msg;

        std::cout << line << "\n";
        if (log_file_.is_open()) {
            log_file_ << line << "\n";
            log_file_.flush();
        }
    }

    static std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }

    static std::ofstream log_file_;
    static std::mutex    mutex_;
};

inline std::ofstream Logger::log_file_;
inline std::mutex    Logger::mutex_;
