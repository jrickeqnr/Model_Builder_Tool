#include "utils/PlottingUtility.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <windows.h>

// Static method to get the path to the plotting script
std::filesystem::path PlottingUtility::getPlottingScriptPath() {
    // Get the executable path
    char execPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, execPath, MAX_PATH) == 0) {
        return {};
    }
    
    // Get the directory containing the executable
    std::filesystem::path appDir = std::filesystem::path(execPath).parent_path();
    
    // Check for the plotting script in various locations
    std::vector<std::filesystem::path> possibleLocations = {
        appDir / "plotting_scripts" / "plot_regression .py",
        appDir / "plotting_scripts" / "plot_regression.py",
        appDir / ".." / "plotting_scripts" / "plot_regression.py",
        appDir / ".." / "resources" / "plot_regression.py"
    };
    
    for (const auto& location : possibleLocations) {
        if (std::filesystem::exists(location)) {
            return location;
        }
    }
    
    // If not found, return empty path
    return {};
}

// Helper function to properly format file paths for Python scripts
std::string PlottingUtility::formatPathForPython(const std::string& path) {
    std::string result = path;
    // Replace backslashes with forward slashes, which Python accepts even on Windows
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}