#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <cstdio>

namespace engine {

// Log severity levels - ordered from least to most severe
enum class LogLevel {
    Debug,   // Detailed debugging information
    Info,    // General informational messages
    Warning, // Warning messages for potentially problematic situations
    Error,   // Error events that might still allow the application to continue
    Fatal    // Critical errors that will terminate the application
};

// Configuration for the logging system
struct LoggerConfig {
    LogLevel level = LogLevel::Info;           // Minimum level to log
    bool enableColors = true;                  // Enable colored console output
    bool enableFileOutput = false;            // Enable file logging
    std::string logFilePath = "wasteland2d.log"; // Path to log file
    bool enableTimestamps = true;             // Include timestamps in output
    bool enableCategories = true;             // Include category tags
    size_t maxFileSizeMB = 10;                 // Max log file size before rotation (MB)
};

// Main logging interface
class Logger {
public:
    // Initialize the logger with configuration
    static void init(const LoggerConfig& config);
    
    // Shutdown the logger (flushes and closes files)
    static void shutdown();
    
    // Check if a log level would be logged (for expensive operations)
    static bool shouldLog(LogLevel level);
    
    // Core logging functions
    static void log(LogLevel level, const char* category, const char* message);
    
    // Simple logging functions with variadic template support
    template<typename... Args>
    static void debug(const char* category, Args&&... args) {
        logInternal(LogLevel::Debug, category, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(const char* category, Args&&... args) {
        logInternal(LogLevel::Info, category, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warning(const char* category, Args&&... args) {
        logInternal(LogLevel::Warning, category, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const char* category, Args&&... args) {
        logInternal(LogLevel::Error, category, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void fatal(const char* category, Args&&... args) {
        logInternal(LogLevel::Fatal, category, std::forward<Args>(args)...);
    }
    
    // Flush the log (useful for critical sections)
    static void flush();

private:
    // Template-based internal logging that handles variadic arguments
    template<typename... Args>
    static void logInternal(LogLevel level, const char* category, Args&&... args) {
        if (!shouldLog(level)) return;
        
        std::ostringstream oss;
        // Stream-style concatenation
        formatArgs(oss, std::forward<Args>(args)...);
        log(level, category, oss.str().c_str());
    }
    
    // Helper to concatenate arguments with spaces
    template<typename T, typename... Args>
    static void formatArgs(std::ostringstream& oss, T first, Args&&... rest) {
        oss << first;
        if constexpr (sizeof...(rest) > 0) {
            oss << " ";
            formatArgs(oss, std::forward<Args>(rest)...);
        }
    }
    
    // Base case - single argument
    template<typename T>
    static void formatArgs(std::ostringstream& oss, T last) {
        oss << last;
    }
    
    static LoggerConfig config_;
    static bool initialized_;
    static FILE* logFile_;
};

// Convenience macros for logging with stream-style support
#define LOG_DEBUG(cat, ...)   engine::Logger::debug(cat, __VA_ARGS__)
#define LOG_INFO(cat, ...)    engine::Logger::info(cat, __VA_ARGS__)
#define LOG_WARNING(cat, ...) engine::Logger::warning(cat, __VA_ARGS__)
#define LOG_ERROR(cat, ...)   engine::Logger::error(cat, __VA_ARGS__)
#define LOG_FATAL(cat, ...)   engine::Logger::fatal(cat, __VA_ARGS__)

// Common log categories
namespace LogCategory {
    constexpr const char* General = "General";
    constexpr const char* Core = "Core";
    constexpr const char* ECS = "ECS";
    constexpr const char* Physics = "Physics";
    constexpr const char* Render = "Render";
    constexpr const char* Input = "Input";
    constexpr const char* Audio = "Audio";
    constexpr const char* Assets = "Assets";
    constexpr const char* Combat = "Combat";
    constexpr const char* Inventory = "Inventory";
    constexpr const char* UI = "UI";
    constexpr const char* Network = "Network";
    constexpr const char* FileIO = "FileIO";
}

} // namespace engine