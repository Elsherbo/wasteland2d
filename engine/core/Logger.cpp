#include "Logger.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

namespace engine {

// Static member initialization
LoggerConfig Logger::config_;
bool Logger::initialized_ = false;
FILE* Logger::logFile_ = nullptr;

// Cross-platform color codes
namespace ConsoleColors {
#ifdef _WIN32
    // Windows console colors
    inline void setColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }
    
    inline void resetColor() {
        setColor(15); // White
    }
    
    const int Gray = 8;
    const int Blue = 9;
    const int Green = 10;
    const int Yellow = 14;
    const int Red = 12;
    const int Magenta = 13;
    const int White = 15;
    const int BrightRed = 12; // Can be enhanced with intense colors
#else
    // Unix ANSI color codes
    const char* Reset = "\033[0m";
    const char* Gray = "\033[90m";
    const char* Blue = "\033[34m";
    const char* Green = "\033[32m";
    const char* Yellow = "\033[33m";
    const char* Red = "\033[31m";
    const char* Magenta = "\033[35m";
    const char* White = "\033[37m";
    const char* BrightRed = "\033[91m";
    
    inline void setColor(const char* color) {
        std::cout << color;
    }
    
    inline void resetColor() {
        std::cout << Reset;
    }
#endif
}

// Get current timestamp as string
std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") << "." 
        << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// Get color for log level
int getLevelColor(LogLevel level) {
#ifdef _WIN32
    switch (level) {
        case LogLevel::Debug:   return ConsoleColors::Gray;
        case LogLevel::Info:    return ConsoleColors::Green;
        case LogLevel::Warning: return ConsoleColors::Yellow;
        case LogLevel::Error:   return ConsoleColors::Red;
        case LogLevel::Fatal:   return ConsoleColors::BrightRed;
        default:               return ConsoleColors::White;
    }
#else
    switch (level) {
        case LogLevel::Debug:   return reinterpret_cast<int>(ConsoleColors::Gray);
        case LogLevel::Info:    return reinterpret_cast<int>(ConsoleColors::Green);
        case LogLevel::Warning: return reinterpret_cast<int>(ConsoleColors::Yellow);
        case LogLevel::Error:   return reinterpret_cast<int>(ConsoleColors::Red);
        case LogLevel::Fatal:   return reinterpret_cast<int>(ConsoleColors::BrightRed);
        default:               return reinterpret_cast<int>(ConsoleColors::White);
    }
#endif
}

// Get level name as string
const char* getLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:               return "UNKN ";
    }
}

void Logger::init(const LoggerConfig& config) {
    config_ = config;
    initialized_ = true;
    
    // Open log file if enabled
    if (config_.enableFileOutput) {
#ifdef _WIN32
        fopen_s(&logFile_, config_.logFilePath.c_str(), "w");
#else
        logFile_ = fopen(config_.logFilePath.c_str(), "w");
#endif
        if (logFile_) {
            std::fprintf(logFile_, "=== Wasteland2D Log Started ===\n");
            std::fflush(logFile_);
        }
    }
    
    // Enable Windows console colors if needed
#ifdef _WIN32
    if (config_.enableColors) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hConsole, &mode);
        SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void Logger::shutdown() {
    if (logFile_) {
        std::fprintf(logFile_, "=== Wasteland2d Log Ended ===\n");
        std::fflush(logFile_);
        fclose(logFile_);
        logFile_ = nullptr;
    }
    initialized_ = false;
}

bool Logger::shouldLog(LogLevel level) {
    if (!initialized_) return false;
    return static_cast<int>(level) >= static_cast<int>(config_.level);
}

void Logger::log(LogLevel level, const char* category, const char* message) {
    if (!shouldLog(level)) return;
    
    std::ostringstream output;
    
    // Add timestamp if enabled
    if (config_.enableTimestamps) {
        output << "[" << getTimestamp() << "] ";
    }
    
    // Add level
    if (config_.enableColors) {
        ConsoleColors::setColor(getLevelColor(level));
    }
    output << "[" << getLevelName(level) << "] ";
    if (config_.enableColors) {
        ConsoleColors::resetColor();
    }
    
    // Add category if enabled
    if (config_.enableCategories) {
        output << "[" << category << "] ";
    }
    
    // Add message
    output << message;
    
    // Reset color if needed
    if (config_.enableColors) {
        ConsoleColors::resetColor();
    }
    
    // Output to console
    std::cout << output.str() << std::endl;
    
    // Output to file if enabled
    if (logFile_) {
        std::fprintf(logFile_, "%s\n", output.str().c_str());
        std::fflush(logFile_);
    }
    
    // Fatal errors should exit
    if (level == LogLevel::Fatal) {
        shutdown();
        std::exit(1);
    }
}

void Logger::flush() {
    std::cout << std::flush;
    if (logFile_) {
        std::fflush(logFile_);
    }
}

} // namespace engine