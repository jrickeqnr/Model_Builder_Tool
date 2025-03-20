#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>

#include <Eigen/Dense>
#include "data/DataFrame.h"

namespace fs = std::filesystem;

/**
 * @brief Utility class for generating plots using Python
 */
class PlottingUtility {
public:
    /**
     * @brief Generate a regression plot using the Python script
     * 
     * @param data DataFrame containing the original data
     * @param predictions Vector of predicted values
     * @param xColumn Name of the X column
     * @param yColumn Name of the Y column
     * @param title Plot title
     * @return std::string Path to the generated plot file, or empty string on failure
     */
    static std::string generateRegressionPlot(
        const DataFrame& data,
        const Eigen::VectorXd& predictions,
        const std::string& xColumn,
        const std::string& yColumn,
        const std::string& title = "Linear Regression Results") 
    {
        try {
            // Create temporary directory if it doesn't exist
            fs::path tempDir = fs::temp_directory_path() / "linear_regression_tool";
            if (!fs::exists(tempDir)) {
                fs::create_directories(tempDir);
            }
            
            // Create paths for temporary files
            fs::path dataFile = tempDir / "data.csv";
            fs::path modelFile = tempDir / "model.csv";
            fs::path outputFile = tempDir / "regression_plot.png";
            
            // Save the original data to a CSV file
            saveDataToCSV(data, dataFile.string());
            
            // Save the predictions to a CSV file
            savePredictionsToCSV(data, predictions, xColumn, modelFile.string());
            
            // Get the script path
            fs::path scriptPath = getPlottingScriptPath();
            if (scriptPath.empty()) {
                std::cerr << "Error: Could not find plotting script" << std::endl;
                return "";
            }
            
            // Build the command to run the Python script
            std::stringstream cmd;
            cmd << "python \"" << scriptPath.string() << "\"";
            cmd << " --data_file \"" << dataFile.string() << "\"";
            cmd << " --model_file \"" << modelFile.string() << "\"";
            cmd << " --output_file \"" << outputFile.string() << "\"";
            cmd << " --x_column \"" << xColumn << "\"";
            cmd << " --y_column \"" << yColumn << "\"";
            cmd << " --title \"" << title << "\"";
            
            // Run the command
            int result = std::system(cmd.str().c_str());
            if (result != 0) {
                std::cerr << "Error: Failed to run plotting script. Command: " << cmd.str() << std::endl;
                return "";
            }
            
            // Check if the output file was created
            if (!fs::exists(outputFile)) {
                std::cerr << "Error: Plot file was not created" << std::endl;
                return "";
            }
            
            return outputFile.string();
        }
        catch (const std::exception& e) {
            std::cerr << "Error generating plot: " << e.what() << std::endl;
            return "";
        }
    }

private:
    /**
     * @brief Save the DataFrame to a CSV file
     * 
     * @param data The DataFrame to save
     * @param filePath Path to save the CSV file
     * @return bool True if successful, false otherwise
     */
    static bool saveDataToCSV(const DataFrame& data, const std::string& filePath) {
        try {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file for writing: " << filePath << std::endl;
                return false;
            }
            
            // Write the header
            for (size_t i = 0; i < data.getColumnNames().size(); ++i) {
                file << data.getColumnNames()[i];
                if (i < data.getColumnNames().size() - 1) {
                    file << ",";
                }
            }
            file << std::endl;
            
            // Write the data
            for (int row = 0; row < data.getNumRows(); ++row) {
                for (size_t col = 0; col < data.getColumnNames().size(); ++col) {
                    file << data.getValue(row, col);
                    if (col < data.getColumnNames().size() - 1) {
                        file << ",";
                    }
                }
                file << std::endl;
            }
            
            file.close();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving data to CSV: " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * @brief Save the predictions to a CSV file
     * 
     * @param data Original DataFrame (for x values)
     * @param predictions Vector of predicted values
     * @param xColumn Name of the X column
     * @param filePath Path to save the CSV file
     * @return bool True if successful, false otherwise
     */
    static bool savePredictionsToCSV(
        const DataFrame& data,
        const Eigen::VectorXd& predictions,
        const std::string& xColumn,
        const std::string& filePath) 
    {
        try {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file for writing: " << filePath << std::endl;
                return false;
            }
            
            // Get the index of the x column
            int xColIndex = data.getColumnIndex(xColumn);
            if (xColIndex < 0) {
                std::cerr << "Error: Column not found: " << xColumn << std::endl;
                return false;
            }
            
            // Write the header
            file << xColumn << ",predicted" << std::endl;
            
            // Write the data
            for (int row = 0; row < data.getNumRows(); ++row) {
                file << data.getValue(row, xColIndex) << "," << predictions(row) << std::endl;
            }
            
            file.close();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving predictions to CSV: " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * @brief Get the path to the plotting script
     * 
     * @return fs::path Path to the plotting script
     */
    static fs::path getPlottingScriptPath() {
        // Try several possible locations
        std::vector<fs::path> possiblePaths = {
            // Current directory
            fs::path("plot_regression.py"),
            
            // plotting_scripts directory in current directory
            fs::path("plotting_scripts") / "plot_regression.py",
            
            // plotting_scripts directory in the executable directory
            fs::path(getExecutableDir()) / "plotting_scripts" / "plot_regression.py",
            
            // Direct path based on build directory structure
            fs::path(getExecutableDir()) / ".." / ".." / "src" / "plotting_scripts" / "plot_regression.py"
        };
        
        for (const auto& path : possiblePaths) {
            if (fs::exists(path)) {
                return fs::absolute(path);
            }
        }
        
        std::cerr << "Warning: Could not find plot_regression.py in any of the expected locations" << std::endl;
        std::cerr << "Executable directory: " << getExecutableDir() << std::endl;
        std::cerr << "Current directory: " << fs::current_path() << std::endl;
        
        return fs::path();
    }
    
    /**
     * @brief Get the directory of the currently running executable
     * 
     * @return std::string Directory path
     */
    static std::string getExecutableDir() {
        char buffer[1024];
    #ifdef _WIN32
        GetModuleFileNameA(NULL, buffer, sizeof(buffer));
    #else
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
        }
    #endif
        return fs::path(buffer).parent_path().string();
    }
};