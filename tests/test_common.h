#pragma once

#include <string>

// Cross-platform temporary directory for test files
// Uses TEST_TMP_DIR defined by CMake, with fallbacks
inline std::string getTestTempDir() {
#ifdef TEST_TMP_DIR
    // On Windows, TEST_TMP_DIR from CMake might have backslashes
    // On Unix, it will have forward slashes
    return TEST_TMP_DIR;
#else
#ifdef _WIN32
    return "C:\\Temp";  // Windows fallback
#else
    return "/tmp";     // Unix fallback
#endif
#endif
}

// Helper to get full path for a test file
inline std::string getTestTempPath(const std::string& filename) {
    std::string dir = getTestTempDir();
    // Ensure directory path ends with separator
    if (!dir.empty() && dir.back() != '\\' && dir.back() != '/') {
#ifdef _WIN32
        dir += "\\";
#else
        dir += "/";
#endif
    }
    return dir + filename;
}
