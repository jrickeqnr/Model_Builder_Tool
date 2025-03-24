#include "gui/PlotWidget.h"
#include "utils/Logger.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_PNG_Image.H>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <sstream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <windows.h>
#include <algorithm>  // For std::min
#include <memory>    // For std::unique_ptr

// Define the LOG_INFO macro if it's not available
#ifndef LOG_INFO
#define LOG_INFO(message, component) std::cout << "[INFO][" << component << "] " << message << std::endl
#endif

#ifndef LOG_ERR
#define LOG_ERR(message, component) std::cerr << "[ERROR][" << component << "] " << message << std::endl
#endif

// Helper function to properly format file paths for Python scripts
std::string formatPathForPython(const std::string& path) {
    std::string result = path;
    // Replace backslashes with forward slashes, which Python accepts even on Windows
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

// Define the static constant
const double PlotWidget::RESIZE_DELAY = 0.5;  // Delay in seconds before regenerating plot after resize

// PlotWidget implementation
PlotWidget::PlotWidget(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), plotImageData(nullptr), plotImageWidth(0), plotImageHeight(0),
      currentPlotType(PlotType::None), resizeTimerActive(false), pendingWidth(0), pendingHeight(0)
{
    box(FL_DOWN_BOX);
    color(FL_WHITE);
    
    // Create a box to display the plot image
    plotBox = new Fl_Box(x, y, w, h);
    plotBox->box(FL_FLAT_BOX);
    
    end();
}

PlotWidget::~PlotWidget() {
    if (plotImageData) {
        delete[] plotImageData;
    }
}

void PlotWidget::draw() {
    Fl_Group::draw();
    
    if (plotImageData) {
        // Draw plot image
        fl_draw_image((const uchar*)plotImageData, x(), y(), plotImageWidth, plotImageHeight, 3, 0);
    } else {
        // Draw message when no plot is available
        fl_color(FL_BLACK);
        fl_font(FL_HELVETICA, 14);
        fl_draw("No plot available", x(), y(), w(), h(), FL_ALIGN_CENTER);
    }
}

void PlotWidget::resizeTimeoutCallback(void* v) {
    PlotWidget* widget = static_cast<PlotWidget*>(v);
    widget->resizeTimerActive = false;
    
    // Call base class resize first
    widget->Fl_Group::resize(widget->x(), widget->y(), widget->pendingWidth, widget->pendingHeight);
    
    // Resize the plot box
    widget->plotBox->resize(widget->x(), widget->y(), widget->pendingWidth, widget->pendingHeight);
    
    // Regenerate the plot with new dimensions if we have data
    if (widget->currentPlotType != PlotType::None) {
        widget->regeneratePlot();
    }
}

void PlotWidget::resize(int x, int y, int w, int h) {
    // Store the pending dimensions
    pendingWidth = w;
    pendingHeight = h;
    
    // If a timer is already active, remove it
    if (resizeTimerActive) {
        Fl::remove_timeout(resizeTimeoutCallback, this);
    }
    
    // Set a new timer
    resizeTimerActive = true;
    Fl::add_timeout(RESIZE_DELAY, resizeTimeoutCallback, this);
    
    // Immediately resize the widget without regenerating the plot
    Fl_Group::resize(x, y, w, h);
    plotBox->resize(x, y, w, h);
    redraw();
}

void PlotWidget::regeneratePlot() {
    switch (currentPlotType) {
        case PlotType::Scatter:
            createScatterPlot(storedActualValues, storedPredictedValues, 
                            storedXLabel, storedYLabel, storedTitle,
                            "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::Timeseries:
            createTimeseriesPlot(storedActualValues, storedPredictedValues, storedTitle,
                                "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::Importance:
            createImportancePlot(storedImportance, storedTitle, 
                                "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::Residual:
            createResidualPlot(storedActualValues, storedPredictedValues, storedTitle,
                              "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::LearningCurve:
            createLearningCurvePlot(storedTrainingScores, storedValidationScores, storedTrainingSizes, storedTitle,
                                   "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::NeuralNetworkArchitecture:
            createNeuralNetworkArchitecturePlot(storedLayerSizes, storedTitle,
                                              "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::TreeVisualization:
            createTreeVisualizationPlot(storedTreeStructure, storedTitle,
                                       "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
            break;
        case PlotType::None:
            break;
    }
}

bool PlotWidget::createTempDataFile(const std::string& data, const std::string& filename)
{
    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return false;
        }

        // Create a unique filename using the process ID and timestamp
        std::string uniqueFilename = std::to_string(GetCurrentProcessId()) + "_" + 
                                   std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                                   "_" + std::filesystem::path(filename).filename().string();

        std::filesystem::path fullPath = std::filesystem::path(tempPath) / uniqueFilename;
        
        std::ofstream file(fullPath);
        if (!file.is_open()) {
            LOG_ERR("ERROR: Failed to create temporary file: " + fullPath.string(), "PlotWidget");
            return false;
        }
        
        file << data;
        file.close();
        
        // Store the full path for later use
        tempFilePaths[filename] = fullPath.string();
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("ERROR: Exception while creating temporary file: " + std::string(e.what()), "PlotWidget");
        return false;
    }
}

bool PlotWidget::executePythonScript(const std::string& scriptPath, const std::string& tempDataPath, const std::string& tempImagePath)
{
    try {
        // Check if Python is available
        int pythonCheck = system("python --version > nul 2>&1");
        if (pythonCheck != 0) {
            LOG_ERR("ERROR: Python is not available", "PlotWidget");
            fl_alert("Python is not available. Please install Python and required libraries (matplotlib, pandas, numpy).");
            return false;
        }

        // Execute the Python script with proper path handling
        std::string command = "python \"" + formatPathForPython(std::filesystem::path(scriptPath).string()) + "\" 2>&1";
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            LOG_ERR("ERROR: Failed to execute Python script", "PlotWidget");
            return false;
        }

        // Read and log the output
        char buffer[128];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        LOG_INFO("Python script output:\n" + result, "PlotWidget");

        // Close the pipe
        int status = _pclose(pipe);
        if (status != 0) {
            LOG_ERR("ERROR: Python script failed with status " + std::to_string(status), "PlotWidget");
            fl_alert("Failed to generate plot. Check if Python and required libraries are installed.");
            return false;
        }

        // Check if the output file was created
        if (!std::filesystem::exists(tempImagePath)) {
            LOG_ERR("ERROR: Plot image file was not created: " + tempImagePath, "PlotWidget");
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("ERROR: Exception while executing Python script: " + std::string(e.what()), "PlotWidget");
        return false;
    }
}

// Helper function to format paths for use with Python
std::string PlotWidget::formatPathForPython(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

void PlotWidget::createTempFilePaths(const std::string& baseName,
                           std::string& dataPath,
                           std::string& imagePath, 
                           std::string& scriptPath) {
    // Generate unique names
    std::string uniqueId = std::to_string(reinterpret_cast<uintptr_t>(this)) + "_" + 
                         std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                         
    // Set output parameter file names
    dataPath = "temp_" + baseName + "_data_" + uniqueId + ".csv";
    imagePath = "temp_" + baseName + "_image_" + uniqueId + ".png";
    scriptPath = "temp_" + baseName + "_script_" + uniqueId + ".py";
    
    // Register the files in the temp directory
    createTempDataFile("", dataPath);
    createTempDataFile("", imagePath);
    createTempDataFile("", scriptPath);
    
    // Return the full paths via output parameters
    dataPath = tempFilePaths[dataPath];
    imagePath = tempFilePaths[imagePath];
    scriptPath = tempFilePaths[scriptPath];
}

void PlotWidget::createScatterPlot(const std::vector<double>& actualValues,
                                 const std::vector<double>& predictedValues,
                                 const std::string& xLabel,
                                 const std::string& yLabel,
                                 const std::string& title,
                                 const std::string& tempDataPath,
                                 const std::string& tempImagePath,
                                 const std::string& tempScriptPath)
{
    // Store data for regeneration
    currentPlotType = PlotType::Scatter;
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedXLabel = xLabel;
    storedYLabel = yLabel;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "scatter_data.csv";
        std::filesystem::path modelFile = tempDir / "scatter_model.csv";
        std::filesystem::path outputFile = tempDir / "scatter_plot.png";
        
        // Create original data CSV file
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        dataFileStream << "index," << yLabel << "\n";
        size_t minSize = actualValues.size() < predictedValues.size() ? actualValues.size() : predictedValues.size();
        for (size_t i = 0; i < minSize; ++i) {
            dataFileStream << i << "," << actualValues[i] << "\n";
        }
        dataFileStream.close();
        
        // Create model data CSV file
        std::ofstream modelFileStream(modelFile);
        if (!modelFileStream.is_open()) {
            LOG_ERR("Failed to create model file", "PlotWidget");
            return;
        }
        
        modelFileStream << "predicted\n";
        for (size_t i = 0; i < minSize; ++i) {
            modelFileStream << predictedValues[i] << "\n";
        }
        modelFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type scatter";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --model_file \"" << formatPathForPython(modelFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --x_column \"" << xLabel << "\"";
        cmd << " --y_column \"" << yLabel << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Load the generated image
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            // Store the image dimensions
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            // Create a new buffer for the image data
            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format

            // Copy the image data
            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        // RGB data
                        plotImageData[dstIdx] = imageData[srcIdx];     // R
                        plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                        plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
                    } else if (depth == 1) {
                        // Grayscale
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        }
        
        // Clean up temporary files (optional - can keep them for debugging)
        // std::filesystem::remove_all(tempDir);
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating scatter plot: " + std::string(e.what()), "PlotWidget");
    }
}

void PlotWidget::createTimeseriesPlot(const std::vector<double>& actualValues,
                                    const std::vector<double>& predictedValues,
                                    const std::string& title,
                                    const std::string& tempDataPath,
                                    const std::string& tempImagePath,
                                    const std::string& tempScriptPath) {
    // Store data for regeneration
    currentPlotType = PlotType::Timeseries;
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "timeseries_data.csv";
        std::filesystem::path outputFile = tempDir / "timeseries_plot.png";
        
        // Create data CSV file
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        dataFileStream << "index,actual,predicted\n";
        size_t minSize = actualValues.size() < predictedValues.size() ? actualValues.size() : predictedValues.size();
        for (size_t i = 0; i < minSize; ++i) {
            dataFileStream << i << "," << actualValues[i] << "," << predictedValues[i] << "\n";
        }
        dataFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type timeseries";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Load the generated image
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            // Store the image dimensions
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            // Create a new buffer for the image data
            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format

            // Copy the image data
            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        // RGB data
                        plotImageData[dstIdx] = imageData[srcIdx];     // R
                        plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                        plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
                    } else if (depth == 1) {
                        // Grayscale
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        } else {
            LOG_ERR("Failed to load the generated plot image", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating timeseries plot: " + std::string(e.what()), "PlotWidget");
    }
}

void PlotWidget::createImportancePlot(const std::unordered_map<std::string, double>& importance,
                                    const std::string& title,
                                    const std::string& tempDataPath,
                                    const std::string& tempImagePath,
                                    const std::string& tempScriptPath)
{
    // Store data for regeneration
    currentPlotType = PlotType::Importance;
    storedImportance = importance;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "importance_data.csv";
        std::filesystem::path outputFile = tempDir / "importance_plot.png";
        
        // Create data CSV file
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        dataFileStream << "feature,importance\n";
        for (const auto& pair : importance) {
            dataFileStream << pair.first << "," << pair.second << "\n";
        }
        dataFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type importance";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Load the generated image
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            // Store the image dimensions
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            // Create a new buffer for the image data
            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format

            // Copy the image data
            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        // RGB data
                        plotImageData[dstIdx] = imageData[srcIdx];     // R
                        plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                        plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
                    } else if (depth == 1) {
                        // Grayscale
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        } else {
            LOG_ERR("Failed to load the generated plot image", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating importance plot: " + std::string(e.what()), "PlotWidget");
    }
}

bool PlotWidget::savePlot(const std::string& filename) {
    // Create temporary file paths with properly managed temp directory paths
    std::string tempFileBaseName = std::to_string(reinterpret_cast<uintptr_t>(this));
    std::string tempDataPath = "temp_plot_data_" + tempFileBaseName + ".csv";
    std::string tempImagePath = "temp_plot_image_" + tempFileBaseName + ".png";
    std::string tempScriptPath = "temp_plot_script_" + tempFileBaseName + ".py";
    
    // Create empty temporary files to generate proper paths
    if (!createTempDataFile("", tempDataPath) || 
        !createTempDataFile("", tempImagePath) || 
        !createTempDataFile("", tempScriptPath)) {
        LOG_ERR("Failed to create temporary files for saving plot", "PlotWidget");
        return false;
    }
    
    // Use the full paths from the tempFilePaths map
    std::string fullTempDataPath = tempFilePaths[tempDataPath];
    std::string fullTempImagePath = tempFilePaths[tempImagePath];
    std::string fullTempScriptPath = tempFilePaths[tempScriptPath];

    // Regenerate the plot with these specific temporary files
    switch (currentPlotType) {
        case PlotType::Scatter:
            createScatterPlot(storedActualValues, storedPredictedValues, 
                            storedXLabel, storedYLabel, storedTitle,
                            fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
        case PlotType::Timeseries:
            createTimeseriesPlot(storedActualValues, storedPredictedValues, storedTitle,
                                fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
        case PlotType::Importance:
            createImportancePlot(storedImportance, storedTitle, fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
        case PlotType::None:
            return false;
        case PlotType::Residual:
            createResidualPlot(storedActualValues, storedPredictedValues, storedTitle,
                               fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
        case PlotType::LearningCurve:
            createLearningCurvePlot(storedTrainingScores, storedValidationScores, storedTrainingSizes, storedTitle,
                                    fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
        case PlotType::NeuralNetworkArchitecture:
            createNeuralNetworkArchitecturePlot(storedLayerSizes, storedTitle,
                                               fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
        case PlotType::TreeVisualization:
            createTreeVisualizationPlot(storedTreeStructure, storedTitle,
                                        fullTempDataPath, fullTempImagePath, fullTempScriptPath);
            break;
    }

    // Copy the generated plot to the target location
    try {
        std::filesystem::path sourcePath(fullTempImagePath);
        std::filesystem::path targetPath(filename);
        
        // Create the target directory if it doesn't exist
        std::filesystem::create_directories(targetPath.parent_path());
        
        // Copy the file
        std::filesystem::copy_file(sourcePath, targetPath, 
                                 std::filesystem::copy_options::overwrite_existing);

        // Clean up temporary files only after successful copy
        std::remove(fullTempScriptPath.c_str());
        std::remove(fullTempDataPath.c_str());
        std::remove(fullTempImagePath.c_str());
        
        return true;
    }
    catch (const std::exception& e) {
        fl_alert("Failed to save plot: %s", e.what());
        return false;
    }
}

void PlotWidget::createResidualPlot(const std::vector<double>& actualValues,
                                  const std::vector<double>& predictedValues,
                                  const std::string& title,
                                  const std::string& tempDataPath,
                                  const std::string& tempImagePath,
                                  const std::string& tempScriptPath)
{
    // Store data for regeneration
    currentPlotType = PlotType::Residual;
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "residual_data.csv";
        std::filesystem::path outputFile = tempDir / "residual_plot.png";
        
        // Create data CSV file
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        // Calculate residuals (actual - predicted)
        dataFileStream << "actual,predicted,residual\n";
        size_t minSize = actualValues.size() < predictedValues.size() ? actualValues.size() : predictedValues.size();
        for (size_t i = 0; i < minSize; ++i) {
            double residual = actualValues[i] - predictedValues[i];
            dataFileStream << actualValues[i] << "," << predictedValues[i] << "," << residual << "\n";
        }
        dataFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type residual";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Load the generated image
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            // Store the image dimensions
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            // Create a new buffer for the image data
            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format

            // Copy the image data
            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        // RGB data
                        plotImageData[dstIdx] = imageData[srcIdx];     // R
                        plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                        plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
                    } else if (depth == 1) {
                        // Grayscale
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        } else {
            LOG_ERR("Failed to load the generated plot image", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating residual plot: " + std::string(e.what()), "PlotWidget");
    }
}

void PlotWidget::createLearningCurvePlot(const std::vector<double>& trainingScores,
                                       const std::vector<double>& validationScores,
                                       const std::vector<int>& trainingSizes,
                                       const std::string& title,
                                       const std::string& tempDataPath,
                                       const std::string& tempImagePath,
                                       const std::string& tempScriptPath)
{
    // Store data for regeneration
    currentPlotType = PlotType::LearningCurve;
    storedTrainingScores = trainingScores;
    storedValidationScores = validationScores;
    storedTrainingSizes = trainingSizes;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "learning_curve_data.csv";
        std::filesystem::path outputFile = tempDir / "learning_curve_plot.png";
        
        // Create data CSV file
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        // Store learning curve data
        dataFileStream << "training_size,training_score,validation_score\n";
        size_t minSize = std::min(trainingSizes.size(), std::min(trainingScores.size(), validationScores.size()));
        for (size_t i = 0; i < minSize; ++i) {
            dataFileStream << trainingSizes[i] << "," << trainingScores[i] << "," << validationScores[i] << "\n";
        }
        dataFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type learning_curve";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Load the generated image
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            // Processing image code (same as other methods)
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];

            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        plotImageData[dstIdx] = imageData[srcIdx];
                        plotImageData[dstIdx+1] = imageData[srcIdx+1];
                        plotImageData[dstIdx+2] = imageData[srcIdx+2];
                    } else if (depth == 1) {
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        } else {
            LOG_ERR("Failed to load the generated plot image", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating learning curve plot: " + std::string(e.what()), "PlotWidget");
    }
}

void PlotWidget::createNeuralNetworkArchitecturePlot(const std::vector<int>& layerSizes,
                                                   const std::string& title,
                                                   const std::string& tempDataPath,
                                                   const std::string& tempImagePath,
                                                   const std::string& tempScriptPath)
{
    // Store data for regeneration
    currentPlotType = PlotType::NeuralNetworkArchitecture;
    storedLayerSizes = layerSizes;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "nn_data.csv";
        std::filesystem::path outputFile = tempDir / "nn_plot.png";
        
        // Create data CSV file
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        // Store neural network architecture data
        dataFileStream << "layer_index,layer_size\n";
        for (size_t i = 0; i < layerSizes.size(); ++i) {
            dataFileStream << i << "," << layerSizes[i] << "\n";
        }
        dataFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type neural_network";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Load image - same code pattern as other methods
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            // Process the image - same as other plot methods
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];

            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        plotImageData[dstIdx] = imageData[srcIdx];
                        plotImageData[dstIdx+1] = imageData[srcIdx+1];
                        plotImageData[dstIdx+2] = imageData[srcIdx+2];
                    } else if (depth == 1) {
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        } else {
            LOG_ERR("Failed to load the generated plot image", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating neural network architecture plot: " + std::string(e.what()), "PlotWidget");
    }
}

void PlotWidget::createTreeVisualizationPlot(const std::string& treeStructure,
                                           const std::string& title,
                                           const std::string& tempDataPath,
                                           const std::string& tempImagePath,
                                           const std::string& tempScriptPath)
{
    // Store data for regeneration
    currentPlotType = PlotType::TreeVisualization;
    storedTreeStructure = treeStructure;
    storedTitle = title;

    try {
        // Get Windows temp directory
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_ERR("Failed to get temp directory", "PlotWidget");
            return;
        }

        // Create a unique subdirectory for temporary files
        std::string uniqueDir = std::to_string(GetCurrentProcessId()) + "_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path tempDir = std::filesystem::path(tempPath) / "Model_Builder_Tool" / uniqueDir;
        std::filesystem::create_directories(tempDir);
        
        // Create paths for temporary files
        std::filesystem::path dataFile = tempDir / "tree_data.txt";
        std::filesystem::path outputFile = tempDir / "tree_plot.png";
        
        // Create data file - tree structure may be in a special format
        std::ofstream dataFileStream(dataFile);
        if (!dataFileStream.is_open()) {
            LOG_ERR("Failed to create data file", "PlotWidget");
            return;
        }
        
        // Write tree structure data
        dataFileStream << treeStructure;
        dataFileStream.close();
        
        // Get the path to the plotting script
        std::filesystem::path scriptPath = PlottingUtility::getPlottingScriptPath();
        if (scriptPath.empty()) {
            LOG_ERR("Could not find plotting script", "PlotWidget");
            return;
        }
        
        // Build the command to run the Python script
        std::stringstream cmd;
        cmd << "python \"" << formatPathForPython(scriptPath.string()) << "\"";
        cmd << " --plot_type tree";
        cmd << " --data_file \"" << formatPathForPython(dataFile.string()) << "\"";
        cmd << " --output_file \"" << formatPathForPython(outputFile.string()) << "\"";
        cmd << " --title \"" << title << "\"";
        cmd << " --width " << (plotBox->w() / 100.0);
        cmd << " --height " << (plotBox->h() / 100.0);
        
        // Store path for later use (needed for saving plots)
        tempFilePaths[tempDataPath] = dataFile.string();
        tempFilePaths[tempImagePath] = outputFile.string();
        
        // Run the command
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            LOG_ERR("Failed to run plotting script", "PlotWidget");
            return;
        }
        
        // Process the image
        Fl_PNG_Image* pngImage = new Fl_PNG_Image(outputFile.string().c_str());
        if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
            plotImageWidth = pngImage->w();
            plotImageHeight = pngImage->h();

            if (plotImageData) {
                delete[] plotImageData;
            }
            plotImageData = new char[plotImageWidth * plotImageHeight * 3];

            const char* imageData = (const char*)pngImage->data()[0];
            int depth = pngImage->d();

            for (int y = 0; y < plotImageHeight; y++) {
                for (int x = 0; x < plotImageWidth; x++) {
                    int srcIdx = (y * plotImageWidth + x) * depth;
                    int dstIdx = (y * plotImageWidth + x) * 3;

                    if (depth >= 3) {
                        plotImageData[dstIdx] = imageData[srcIdx];
                        plotImageData[dstIdx+1] = imageData[srcIdx+1];
                        plotImageData[dstIdx+2] = imageData[srcIdx+2];
                    } else if (depth == 1) {
                        plotImageData[dstIdx] = 
                        plotImageData[dstIdx+1] = 
                        plotImageData[dstIdx+2] = imageData[srcIdx];
                    }
                }
            }
            
            delete pngImage;
            redraw();
        } else {
            LOG_ERR("Failed to load the generated plot image", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Error creating tree visualization plot: " + std::string(e.what()), "PlotWidget");
    }
} 