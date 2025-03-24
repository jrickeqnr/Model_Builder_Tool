#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>  // For SHGetFolderPathA and CSIDL_LOCAL_APPDATA

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) {
        m_logLevel = level;
    }

    LogLevel getLogLevel() const {
        return m_logLevel;
    }

    void debug(const std::string& message, const std::string& component = "") {
        log(LogLevel::DEBUG, message, component);
    }

    void info(const std::string& message, const std::string& component = "") {
        log(LogLevel::INFO, message, component);
    }

    void warn(const std::string& message, const std::string& component = "") {
        log(LogLevel::WARNING, message, component);
    }

    void error(const std::string& message, const std::string& component = "") {
        log(LogLevel::ERR, message, component);
    }

    void fatal(const std::string& message, const std::string& component = "") {
        log(LogLevel::FATAL, message, component);
    }

    void log(LogLevel level, const std::string& message, const std::string& component = "") {
        if (level < m_logLevel) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::ofstream logFile;
        static bool firstLog = true;

        // Get the application data directory for Windows
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
            std::filesystem::path logDir = std::filesystem::path(appDataPath) / "Model_Builder_Tool" / "logs";
            std::filesystem::create_directories(logDir);
            
            std::filesystem::path logPath = logDir / "application.log";
            
            if (firstLog) {
                // Clear log file on first use
                logFile.open(logPath, std::ios::out | std::ios::trunc);
                firstLog = false;
            } else {
                // Append to log file
                logFile.open(logPath, std::ios::app);
            }

            if (!logFile.is_open()) {
                // Fallback to current directory if app data path fails
                logFile.open("application.log", std::ios::app);
            }
        } else {
            // Fallback to current directory if app data path fails
            logFile.open("application.log", std::ios::app);
        }

        if (!logFile.is_open()) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::tm now_tm;
        localtime_s(&now_tm, &now_time_t);
        
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

        std::string levelStr;
        switch (level) {
            case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
            case LogLevel::INFO:    levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARNING"; break;
            case LogLevel::ERR:   levelStr = "ERROR"; break;
            case LogLevel::FATAL:   levelStr = "FATAL"; break;
            default:                levelStr = "UNKNOWN";
        }

        std::string componentInfo = component.empty() ? "" : "[" + component + "] ";
        logFile << ss.str() << " | " << std::setw(7) << std::left << levelStr << " | " 
                << componentInfo << message << std::endl;
        logFile.close();
        
        // Also output to console for immediate feedback
        std::cout << ss.str() << " | " << std::setw(7) << std::left << levelStr << " | " 
                  << componentInfo << message << std::endl;
    }

private:
    Logger() : m_logLevel(LogLevel::DEBUG) {
        // Initialize logging directory on construction
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
            std::filesystem::path logDir = std::filesystem::path(appDataPath) / "Model_Builder_Tool" / "logs";
            std::filesystem::create_directories(logDir);
        }
    }
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_logLevel;
    std::mutex m_mutex;
};

#define LOG_DEBUG(message, component) Logger::getInstance().debug(message, component)
#define LOG_INFO(message, component) Logger::getInstance().info(message, component)
#define LOG_WARN(message, component) Logger::getInstance().warn(message, component)
#define LOG_ERR(message, component) Logger::getInstance().error(message, component)
#define LOG_FATAL(message, component) Logger::getInstance().fatal(message, component) 