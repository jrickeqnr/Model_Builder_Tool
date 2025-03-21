#include <iostream>
#include "gui/ResultsView.h"
#include "util/Logger.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
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

// Define the LOG_INFO macro if it's not available
#ifndef LOG_INFO
#define LOG_INFO(message, component) std::cout << "[INFO][" << component << "] " << message << std::endl
#endif

// Add logging function with file clearing on first use
void log_debug(const std::string& message) {
    static bool first_call = true;
    std::ofstream logFile;
    
    if (first_call) {
        // Clear the file on first use
        logFile.open("debug.log", std::ios::out | std::ios::trunc);
        first_call = false;
    } else {
        logFile.open("debug.log", std::ios::app);
    }
    
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    logFile << std::ctime(&now_c) << message << std::endl;
    logFile.close();
}

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
        fl_draw("No plot available", x() + 10, y() + 20);
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
        std::ofstream file(filename);
        if (!file.is_open()) {
            log_debug("ERROR: Failed to create temporary file: " + filename);
            return false;
        }
        
        file << data;
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        log_debug("ERROR: Exception while creating temporary file: " + std::string(e.what()));
        return false;
    }
}

bool PlotWidget::executePythonScript(const std::string& scriptPath, const std::string& tempDataPath, const std::string& tempImagePath)
{
    try {
        // Check if Python is available
        int pythonCheck = system("python --version > nul 2>&1");
        if (pythonCheck != 0) {
            log_debug("ERROR: Python is not available");
            fl_alert("Python is not available. Please install Python and required libraries (matplotlib, pandas, numpy).");
            return false;
        }

        // Execute the Python script
        std::string command = "python \"" + scriptPath + "\" 2>&1";
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            log_debug("ERROR: Failed to execute Python script");
            return false;
        }

        // Read and log the output
        char buffer[128];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        log_debug("Python script output:\n" + result);

        // Close the pipe
        int status = _pclose(pipe);
        if (status != 0) {
            log_debug("ERROR: Python script failed with status " + std::to_string(status));
            fl_alert("Failed to generate plot. Check if Python and required libraries are installed.");
            return false;
        }

        // Check if the output file was created
        std::ifstream checkFile(tempImagePath);
        if (!checkFile.good()) {
            log_debug("ERROR: Plot image file was not created: " + tempImagePath);
            return false;
        }
        checkFile.close();

        return true;
    }
    catch (const std::exception& e) {
        log_debug("ERROR: Exception while executing Python script: " + std::string(e.what()));
        return false;
    }
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

    log_debug("Creating scatter plot - Data file: " + tempDataPath + ", Image file: " + tempImagePath);

    // Write data to temporary file
    if (!createTempDataFile("actual,predicted\n", tempDataPath)) {
        return;
    }

    std::ofstream dataFile(tempDataPath, std::ios::app);
    if (!dataFile.is_open()) {
        log_debug("ERROR: Failed to append to temporary data file: " + tempDataPath);
        return;
    }

    for (size_t i = 0; i < std::min(actualValues.size(), predictedValues.size()); ++i) {
        dataFile << actualValues[i] << "," << predictedValues[i] << "\n";
    }
    dataFile.close();

    // Calculate figure size based on widget dimensions
    double figWidth = w() / 100.0;  // Convert pixels to inches (assuming 100 DPI)
    double figHeight = h() / 100.0;

    // Create plotting script
    std::stringstream scriptContent;
    scriptContent << "import matplotlib\n"
                 << "matplotlib.use('Agg')\n"
                 << "import matplotlib.pyplot as plt\n"
                 << "import pandas as pd\n"
                 << "import numpy as np\n\n"
                 << "try:\n"
                 << "    # Read data\n"
                 << "    data = pd.read_csv('" << tempDataPath << "')\n"
                 << "    actual = data['actual']\n"
                 << "    predicted = data['predicted']\n\n"
                 << "    # Create plot\n"
                 << "    plt.figure(figsize=(" << figWidth << ", " << figHeight << "), dpi=100)\n"
                 << "    plt.scatter(actual, predicted, alpha=0.7, s=30)\n\n"
                 << "    # Add perfect prediction line\n"
                 << "    min_val = min(actual.min(), predicted.min())\n"
                 << "    max_val = max(actual.max(), predicted.max())\n"
                 << "    plt.plot([min_val, max_val], [min_val, max_val], 'r--', linewidth=1)\n\n"
                 << "    # Set labels and title\n"
                 << "    plt.xlabel('" << xLabel << "', fontsize=8)\n"
                 << "    plt.ylabel('" << yLabel << "', fontsize=8)\n"
                 << "    plt.title('" << title << "', fontsize=10)\n"
                 << "    plt.xticks(fontsize=8)\n"
                 << "    plt.yticks(fontsize=8)\n"
                 << "    plt.grid(True, linestyle='--', alpha=0.7)\n\n"
                 << "    # Save plot\n"
                 << "    plt.tight_layout()\n"
                 << "    plt.savefig('" << tempImagePath << "', format='png', bbox_inches='tight')\n"
                 << "    plt.close()\n"
                 << "except Exception as e:\n"
                 << "    print('Error in Python script:', str(e))\n"
                 << "    exit(1)\n";

    if (!createTempDataFile(scriptContent.str(), tempScriptPath)) {
        return;
    }

    // Execute the Python script
    if (!executePythonScript(tempScriptPath, tempDataPath, tempImagePath)) {
        return;
    }

    // Load the generated image
    std::unique_ptr<Fl_PNG_Image> pngImage(new Fl_PNG_Image(tempImagePath.c_str()));
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        log_debug("ERROR: Failed to load the generated plot image: " + tempImagePath);
        return;
    }

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

    // Clean up temporary files
    std::remove(tempScriptPath.c_str());
    std::remove(tempDataPath.c_str());

    // Redraw the widget
    redraw();
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

    // Write data to temporary file
    std::ofstream dataFile(tempDataPath);
    if (!dataFile.is_open()) {
        fl_alert("Failed to create temporary data file");
        return;
    }

    dataFile << "index,actual,predicted\n";
    for (size_t i = 0; i < actualValues.size(); ++i) {
        dataFile << i << "," << actualValues[i] << "," << predictedValues[i] << "\n";
    }
    dataFile.close();

    // Calculate figure size based on widget dimensions
    double figWidth = w() / 100.0;  // Convert pixels to inches (assuming 100 DPI)
    double figHeight = h() / 100.0;

    // Create plotting script
    std::ofstream scriptFile(tempScriptPath);
    if (!scriptFile.is_open()) {
        fl_alert("Failed to create temporary script file");
        return;
    }

    scriptFile << "import matplotlib.pyplot as plt\n";
    scriptFile << "import pandas as pd\n";
    scriptFile << "import numpy as np\n\n";

    // Read data
    scriptFile << "# Read data\n";
    scriptFile << "data = pd.read_csv('" << tempDataPath << "')\n\n";

    // Create plot with dimensions based on widget size
    scriptFile << "# Create plot\n";
    scriptFile << "plt.figure(figsize=(" << figWidth << ", " << figHeight << "), dpi=100)\n";
    scriptFile << "plt.plot(data['index'], data['actual'], label='Actual', alpha=0.7, linewidth=1)\n";
    scriptFile << "plt.plot(data['index'], data['predicted'], label='Predicted', alpha=0.7, linewidth=1)\n\n";

    // Set labels and title with adjusted font sizes
    scriptFile << "# Set labels and title\n";
    scriptFile << "plt.xlabel('Time Index', fontsize=8)\n";
    scriptFile << "plt.ylabel('Values', fontsize=8)\n";
    scriptFile << "plt.title('" << title << "', fontsize=10)\n";
    scriptFile << "plt.xticks(fontsize=8)\n";
    scriptFile << "plt.yticks(fontsize=8)\n";
    scriptFile << "plt.legend(fontsize=8)\n";
    scriptFile << "plt.grid(True, linestyle='--', alpha=0.7)\n\n";

    // Save plot
    scriptFile << "# Save plot\n";
    scriptFile << "plt.tight_layout()\n";
    scriptFile << "plt.savefig('" << tempImagePath << "', format='png', bbox_inches='tight')\n";
    scriptFile << "plt.close()\n";

    scriptFile.close();

    // Execute Python script
    std::string command = "python " + tempScriptPath;
    int result = std::system(command.c_str());
    if (result != 0) {
        fl_alert("Failed to generate plot. Check if Python and matplotlib are installed.");
        return;
    }

    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
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

        // Clean up the PNG image
        delete pngImage;

        // Redraw the widget
        redraw();
    } else {
        fl_alert("Failed to load the generated plot image: %s", tempImagePath.c_str());
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

    // Write data to temporary file
    std::ofstream dataFile(tempDataPath);
    if (!dataFile.is_open()) {
        fl_alert("Failed to create temporary data file");
        return;
    }

    dataFile << "feature,importance\n";
    for (const auto& pair : importance) {
        dataFile << pair.first << "," << pair.second << "\n";
    }
    dataFile.close();

    // Calculate figure size based on widget dimensions
    double figWidth = w() / 100.0;  // Convert pixels to inches (assuming 100 DPI)
    double figHeight = h() / 100.0;

    // Create plotting script
    std::ofstream scriptFile(tempScriptPath);
    if (!scriptFile.is_open()) {
        fl_alert("Failed to create temporary script file");
        return;
    }

    scriptFile << "import matplotlib.pyplot as plt\n";
    scriptFile << "import pandas as pd\n";
    scriptFile << "import numpy as np\n\n";

    // Read data
    scriptFile << "# Read data\n";
    scriptFile << "data = pd.read_csv('" << tempDataPath << "')\n";
    scriptFile << "data = data.sort_values('importance', ascending=True)\n\n";

    // Create plot with dimensions based on widget size
    scriptFile << "# Create plot\n";
    scriptFile << "plt.figure(figsize=(" << figWidth << ", " << figHeight << "), dpi=100)\n";
    scriptFile << "y_pos = np.arange(len(data['feature']))\n";
    scriptFile << "plt.barh(y_pos, data['importance'], align='center', height=0.5)\n";
    scriptFile << "plt.yticks(y_pos, data['feature'], fontsize=8)\n\n";

    // Set labels and title with adjusted font sizes
    scriptFile << "# Set labels and title\n";
    scriptFile << "plt.xlabel('Relative Importance', fontsize=8)\n";
    scriptFile << "plt.title('" << title << "', fontsize=10)\n";
    scriptFile << "plt.xticks(fontsize=8)\n";
    scriptFile << "plt.grid(True, linestyle='--', alpha=0.7)\n\n";

    // Save plot
    scriptFile << "# Save plot\n";
    scriptFile << "plt.tight_layout(pad=0.5)\n";
    scriptFile << "plt.savefig('" << tempImagePath << "', format='png', bbox_inches='tight')\n";
    scriptFile << "plt.close()\n";

    scriptFile.close();

    // Execute Python script
    std::string command = "python " + tempScriptPath;
    int result = std::system(command.c_str());
    if (result != 0) {
        fl_alert("Failed to generate plot. Check if Python and matplotlib are installed.");
        return;
    }

    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
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

        // Clean up the PNG image
        delete pngImage;

        // Redraw the widget
        redraw();
    } else {
        fl_alert("Failed to load the generated plot image: %s", tempImagePath.c_str());
    }
}

bool PlotWidget::savePlot(const std::string& filename) {
    // Create temporary file paths with unique names using the current plot type
    std::string tempDataPath = "temp_plot_data_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".csv";
    std::string tempImagePath = "temp_plot_image_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".png";
    std::string tempScriptPath = "temp_plot_script_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".py";

    // Regenerate the plot with these specific temporary files
    switch (currentPlotType) {
        case PlotType::Scatter:
            createScatterPlot(storedActualValues, storedPredictedValues, 
                            storedXLabel, storedYLabel, storedTitle,
                            tempDataPath, tempImagePath, tempScriptPath);
            break;
        case PlotType::Timeseries:
            createTimeseriesPlot(storedActualValues, storedPredictedValues, storedTitle,
                                tempDataPath, tempImagePath, tempScriptPath);
            break;
        case PlotType::Importance:
            createImportancePlot(storedImportance, storedTitle, tempDataPath, tempImagePath, tempScriptPath);
            break;
        case PlotType::None:
            return false;
    }

    // Copy the generated plot to the target location
    try {
        std::filesystem::path sourcePath(tempImagePath);
        std::filesystem::path targetPath(filename);
        
        // Create the target directory if it doesn't exist
        std::filesystem::create_directories(targetPath.parent_path());
        
        // Copy the file
        std::filesystem::copy_file(sourcePath, targetPath, 
                                 std::filesystem::copy_options::overwrite_existing);

        // Clean up temporary files only after successful copy
        std::remove(tempScriptPath.c_str());
        std::remove(tempDataPath.c_str());
        std::remove(tempImagePath.c_str());
        
        return true;
    }
    catch (const std::exception& e) {
        fl_alert("Failed to save plot: %s", e.what());
        return false;
    }
}

// PlotNavigator implementation
PlotNavigator::PlotNavigator(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), currentPlotIndex(0)
{
    box(FL_DOWN_BOX);
    color(FL_WHITE);

    // Create navigation buttons at the bottom
    int buttonWidth = 30;
    int buttonHeight = 25;
    int buttonY = y + h - buttonHeight - 5;
    
    prevButton = new Fl_Button(x + 5, buttonY, buttonWidth, buttonHeight, "@<");
    prevButton->callback(prevButtonCallback, this);
    
    nextButton = new Fl_Button(x + w - buttonWidth - 5, buttonY, buttonWidth, buttonHeight, "@>");
    nextButton->callback(nextButtonCallback, this);
    
    // Create plot label
    plotLabel = new Fl_Box(x + buttonWidth + 10, buttonY, w - 2*buttonWidth - 20, buttonHeight);
    plotLabel->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    
    end();
    updateNavigationButtons();
}

PlotNavigator::~PlotNavigator()
{
    clearPlots();
}

void PlotNavigator::createPlot(const std::shared_ptr<DataFrame>& data,
                             const std::shared_ptr<Model>& model,
                             const std::string& plotType,
                             const std::string& title)
{
    log_debug("Creating plot of type: " + plotType);
    
    int plotX = x() + 5;
    int plotY = y() + 5;
    int plotW = w() - 10;
    int plotH = h() - 40;  // Leave space for navigation buttons
    
    log_debug("Plot dimensions - X: " + std::to_string(plotX) + 
              ", Y: " + std::to_string(plotY) + 
              ", W: " + std::to_string(plotW) + 
              ", H: " + std::to_string(plotH));
    
    PlotWidget* plot = new PlotWidget(plotX, plotY, plotW, plotH);
    plots.push_back(plot);
    
    // Create the plot based on type
    if (plotType == "scatter") {
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        log_debug("Scatter plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()));
        
        plot->createScatterPlot(actual, predictedVec, "Actual Values", "Predicted Values", title,
                               "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
    }
    else if (plotType == "timeseries") {
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        log_debug("Time series plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()));
        
        plot->createTimeseriesPlot(actual, predictedVec, title,
                                   "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
    }
    else if (plotType == "importance") {
        auto importance = model->getFeatureImportance();
        
        log_debug("Feature importance plot - Number of features: " + std::to_string(importance.size()));
        
        plot->createImportancePlot(importance, title, "temp_plot_data.csv", "temp_plot_image.png", "temp_plot_script.py");
    }
    
    log_debug("Adding plot to navigator");
    add(plot);
    
    log_debug("Plot widget dimensions after add - X: " + std::to_string(plot->x()) + 
              ", Y: " + std::to_string(plot->y()) + 
              ", W: " + std::to_string(plot->w()) + 
              ", H: " + std::to_string(plot->h()));
    
    updateVisibility();
    updateNavigationButtons();
    log_debug("Plot creation completed");
}

void PlotNavigator::nextPlot()
{
    if (currentPlotIndex < plots.size() - 1) {
        currentPlotIndex++;
        updateVisibility();
        updateNavigationButtons();
    }
}

void PlotNavigator::prevPlot()
{
    if (currentPlotIndex > 0) {
        currentPlotIndex--;
        updateVisibility();
        updateNavigationButtons();
    }
}

void PlotNavigator::clearPlots()
{
    for (auto plot : plots) {
        remove(plot);
        delete plot;
    }
    plots.clear();
    currentPlotIndex = 0;
    updateNavigationButtons();
}

void PlotNavigator::prevButtonCallback(Fl_Widget*, void* v)
{
    ((PlotNavigator*)v)->prevPlot();
}

void PlotNavigator::nextButtonCallback(Fl_Widget*, void* v)
{
    ((PlotNavigator*)v)->nextPlot();
}

void PlotNavigator::updateVisibility()
{
    for (size_t i = 0; i < plots.size(); i++) {
        if (i == currentPlotIndex) {
            plots[i]->show();
        } else {
            plots[i]->hide();
        }
    }
    
    // Update plot label
    if (!plots.empty()) {
        char label[32];
        snprintf(label, sizeof(label), "Plot %zu of %zu", currentPlotIndex + 1, plots.size());
        plotLabel->copy_label(label);
    } else {
        plotLabel->copy_label("No plots available");
    }
    
    redraw();
}

void PlotNavigator::updateNavigationButtons()
{
    prevButton->activate();
    nextButton->activate();
    
    if (plots.empty() || currentPlotIndex == 0) {
        prevButton->deactivate();
    }
    if (plots.empty() || currentPlotIndex == plots.size() - 1) {
        nextButton->deactivate();
    }
}

bool PlotNavigator::savePlotToFile(size_t index, const std::string& filename) {
    if (index >= plots.size()) {
        return false;
    }

    try {
        // Create the directory if it doesn't exist
        std::filesystem::path filePath(filename);
        std::filesystem::create_directories(filePath.parent_path());

        // Save the plot using matplotlib's savefig
        plots[index]->savePlot(filename);
        return true;
    }
    catch (const std::exception& e) {
        fl_alert("Failed to save plot: %s", e.what());
        return false;
    }
}

// ResultsView implementation
ResultsView::ResultsView(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h),
      exportDialog(std::make_unique<ExportDialog>(400, 300, "Export Options"))
{
    begin();
    
    // Set group properties
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
    
    // Constants for layout
    int margin = 20;
    int headerHeight = 40;
    int bottomButtonsHeight = 40;
    int equationHeight = 60;
    
    // Create title label
    modelTitleLabel = new Fl_Box(x + margin, y + margin, w - 2*margin, headerHeight, "Model Results");
    modelTitleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    modelTitleLabel->labelsize(18);
    modelTitleLabel->labelfont(FL_BOLD);
    
    // Create subtitle label
    modelSubtitleLabel = new Fl_Box(x + margin, y + margin + headerHeight, w - 2*margin, 25, "");
    modelSubtitleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    modelSubtitleLabel->labelsize(14);
    modelSubtitleLabel->labelfont(FL_ITALIC);
    
    // Create equation display box
    Fl_Box* equationLabel = new Fl_Box(x + margin, y + margin + headerHeight + 30, 
                                      w - 2*margin, equationHeight, "Regression Equation:");
    equationLabel->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    equationLabel->labelsize(14);
    equationLabel->labelfont(FL_BOLD);
    
    equationDisplay = new Fl_Box(x + margin + 20, y + margin + headerHeight + 55, 
                                w - 2*margin - 40, equationHeight - 20, "");
    equationDisplay->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    equationDisplay->labelsize(14);
    equationDisplay->box(FL_BORDER_BOX);
    
    // Create main display area
    int contentY = y + margin + headerHeight + equationHeight + 10;
    int contentHeight = h - margin*2 - headerHeight - equationHeight - 10 - bottomButtonsHeight - 10;
    int tableWidth = (w - margin*3) / 2;
    
    // Parameters group (left side)
    parametersGroup = new Fl_Group(x + margin, contentY, tableWidth, contentHeight/2);
    parametersGroup->box(FL_BORDER_BOX);
    parametersGroup->begin();
    
    Fl_Box* parametersLabel = new Fl_Box(x + margin + 10, contentY + 10, tableWidth - 20, 30, "Model Parameters");
    parametersLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    parametersLabel->labelsize(14);
    parametersLabel->labelfont(FL_BOLD);
    
    parametersTable = new DataTable(x + margin + 10, contentY + 50, 
                                  tableWidth - 20, contentHeight/2 - 60);
    
    parametersGroup->end();
    
    // Statistics group (left side, bottom half)
    statisticsGroup = new Fl_Group(x + margin, contentY + contentHeight/2 + 10, tableWidth, contentHeight/2 - 10);
    statisticsGroup->box(FL_BORDER_BOX);
    statisticsGroup->begin();
    
    Fl_Box* statisticsLabel = new Fl_Box(x + margin + 10, contentY + contentHeight/2 + 20, tableWidth - 20, 30, "Model Statistics");
    statisticsLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    statisticsLabel->labelsize(14);
    statisticsLabel->labelfont(FL_BOLD);
    
    statisticsTable = new DataTable(x + margin + 10, contentY + contentHeight/2 + 60, 
                                  tableWidth - 20, contentHeight/2 - 70);
    
    statisticsGroup->end();
    
    // Plot navigator (right side)
    plotNavigator = new PlotNavigator(x + margin*2 + tableWidth, contentY, tableWidth, contentHeight);
    
    // Create bottom buttons
    int buttonY = y + h - margin - bottomButtonsHeight;
    
    backButton = new Fl_Button(x + margin, buttonY, 100, bottomButtonsHeight, "Back");
    backButton->callback(backButtonCallback_static, this);
    
    exportButton = new Fl_Button(x + w - margin - 150, buttonY, 150, bottomButtonsHeight, "Export Results");
    exportButton->callback(exportButtonCallback_static, this);
    
    end();

    // Initialize the export dialog
    exportDialog->onExport = [this](const ExportDialog::ExportOptions& options) {
        exportResults(options);
    };
}

ResultsView::~ResultsView() {
    // FLTK handles widget destruction through parent-child relationship
}

void ResultsView::setModel(std::shared_ptr<Model> model) {
    this->model = model;
}

void ResultsView::setData(std::shared_ptr<DataFrame> dataFrame, 
                         const std::vector<std::string>& inputVariables, 
                         const std::string& targetVariable) {
    this->dataFrame = dataFrame;
    this->inputVariables = inputVariables;
    this->targetVariable = targetVariable;
}

void ResultsView::setModelType(const std::string& modelType) {
    this->modelType = modelType;
    if (modelSubtitleLabel) {
        modelSubtitleLabel->copy_label(("Model Type: " + modelType).c_str());
    }
}

void ResultsView::setHyperparameters(const std::unordered_map<std::string, std::string>& hyperparams) {
    this->hyperparameters = hyperparams;
}

void ResultsView::updateResults() {
    if (!model || !dataFrame) {
        return;
    }
    
    // Clear existing plots
    plotNavigator->clearPlots();
    
    // Update displays based on the model type
    if (modelType == "Linear Regression") {
        updateLinearRegressionDisplay();
    } else {
        // For now, all models use the same display until they are implemented
        // This will be updated as each model is implemented
        // Replaced LOG_INFO with comment to fix build error
        updateLinearRegressionDisplay();
    }
    
    // Redraw the widget
    redraw();
}

void ResultsView::updateParametersDisplay() {
    if (!model || !parametersTable) {
        return;
    }
    
    // Get parameters
    auto parameters = model->getParameters();
    
    // Reorganize parameters to show intercept first, then coefficients in order
    std::unordered_map<std::string, double> orderedParams;
    
    // Add intercept first
    auto interceptIt = parameters.find("intercept");
    if (interceptIt != parameters.end()) {
        orderedParams["Intercept"] = interceptIt->second;
    }
    
    // Then add coefficients with variable names
    for (const auto& varName : model->getVariableNames()) {
        auto coefIt = parameters.find(varName);
        if (coefIt != parameters.end()) {
            orderedParams[varName + " (coefficient)"] = coefIt->second;
        }
    }
    
    // Update table with ordered parameter values
    parametersTable->setData(orderedParams);
}

void ResultsView::updateStatisticsDisplay() {
    if (!model || !statisticsTable) {
        return;
    }
    
    // Get statistics
    auto statistics = model->getStatistics();
    
    // Create a nicer representation of statistics with better names
    std::unordered_map<std::string, double> formattedStats;
    
    // R-squared
    auto r2 = statistics.find("r_squared");
    if (r2 != statistics.end()) {
        formattedStats["R² (coefficient of determination)"] = r2->second;
    }
    
    // Adjusted R-squared
    auto adjR2 = statistics.find("adjusted_r_squared");
    if (adjR2 != statistics.end()) {
        formattedStats["Adjusted R²"] = adjR2->second;
    }
    
    // RMSE
    auto rmse = statistics.find("rmse");
    if (rmse != statistics.end()) {
        formattedStats["RMSE (root mean squared error)"] = rmse->second;
    }
    
    // Number of samples
    auto samples = statistics.find("n_samples");
    if (samples != statistics.end()) {
        formattedStats["Number of observations"] = samples->second;
    }
    
    // Number of features
    auto features = statistics.find("n_features");
    if (features != statistics.end()) {
        formattedStats["Number of variables"] = features->second;
    }
    
    // Update table with formatted statistics
    statisticsTable->setData(formattedStats);
}

std::string ResultsView::getEquationString() const {
    if (!model) {
        return "No model available";
    }
    
    std::ostringstream equation;
    equation << std::fixed << std::setprecision(4);
    
    // Get target name
    std::string targetName = model->getTargetName();
    if (targetName.empty()) {
        targetName = "Y";
    }
    
    // Get parameters
    auto parameters = model->getParameters();
    
    // Start with the target variable
    equation << targetName << " = ";
    
    // Add intercept
    bool firstTerm = true;
    auto interceptIt = parameters.find("intercept");
    if (interceptIt != parameters.end()) {
        equation << interceptIt->second;
        firstTerm = false;
    }
    
    // Add coefficients with variable names
    for (const auto& varName : model->getVariableNames()) {
        auto coefIt = parameters.find(varName);
        if (coefIt != parameters.end()) {
            double coef = coefIt->second;
            if (coef >= 0 && !firstTerm) {
                equation << " + ";
            } else if (coef < 0) {
                equation << " - ";
                coef = -coef; // Make positive for display
            }
            
            equation << coef << " * " << varName;
            firstTerm = false;
        }
    }
    
    return equation.str();
}

void ResultsView::createPlots() {
    if (!model || !dataFrame) {
        return;
    }

    // Get actual and predicted values
    Eigen::MatrixXd X = dataFrame->toMatrix(inputVariables);
    Eigen::VectorXd y = Eigen::Map<Eigen::VectorXd>(
        dataFrame->getColumn(targetVariable).data(),
        dataFrame->getColumn(targetVariable).size()
    );
    Eigen::VectorXd predictions = model->predict(X);

    // Convert Eigen vectors to std::vector
    std::vector<double> actualValues(y.data(), y.data() + y.size());
    std::vector<double> predictedValues(predictions.data(), predictions.data() + predictions.size());

    // Clear existing plots
    plotNavigator->clearPlots();

    // Create standard plots for all models
    plotNavigator->createPlot(
        dataFrame, model,
        "scatter", 
        "Actual vs. Predicted Values"
    );

    plotNavigator->createPlot(
        dataFrame, model,
        "importance", 
        "Feature Importance"
    );
    
    plotNavigator->createPlot(
        dataFrame, model,
        "residual", 
        "Residual Plot"
    );

    // Add model-specific plots
    std::string modelName = model->getName();
    
    if (modelName == "Neural Network") {
        // For neural networks, add architecture diagram
        // We would need to extract layer sizes from the model
        std::vector<int> layerSizes;
        
        // Check if we can get the layer sizes from hyperparameters
        auto params = model->getParameters();
        auto hiddenLayersIt = params.find("hidden_layer_sizes");
        
        if (hiddenLayersIt != params.end()) {
            // Parse the hidden layer sizes
            std::string hiddenLayers = std::to_string(hiddenLayersIt->second);
            std::stringstream ss(hiddenLayers);
            std::string layer;
            
            // Add input layer size (number of features)
            layerSizes.push_back(inputVariables.size());
            
            // Parse comma-separated hidden layer sizes
            while (std::getline(ss, layer, ',')) {
                try {
                    layerSizes.push_back(std::stoi(layer));
                } catch (...) {
                    // If we can't parse, just use a default
                    layerSizes.push_back(10);
                }
            }
            
            // Add output layer (always 1 for regression)
            layerSizes.push_back(1);
            
            plotNavigator->createPlot(
                dataFrame, model,
                "neural_network_architecture", 
                "Neural Network Architecture"
            );
        }
    }
    else if (modelName == "Random Forest" || modelName == "Gradient Boosting" || modelName == "XGBoost") {
        // For tree-based models, add a tree visualization (if available)
        plotNavigator->createPlot(
            dataFrame, model,
            "tree_visualization", 
            "Tree Visualization"
        );
    }
    
    // For all models except Linear Regression, add learning curves
    if (modelName != "Linear Regression") {
        plotNavigator->createPlot(
            dataFrame, model,
            "learning_curve", 
            "Learning Curves"
        );
    }
}

void ResultsView::exportResults(const ExportDialog::ExportOptions& options) {
    if (!model || !dataFrame) {
        return;
    }
    
    // For now, use the original export options until the new fields are properly implemented
    std::string exportPath = options.exportPath;
    bool exportSummary = options.modelSummary;
    bool exportCSV = options.predictedValues;
    bool exportPlots = options.scatterPlot || options.linePlot || options.importancePlot;
    
    // Original export code
    try {
        // Export model summary if selected
        if (exportSummary) {
            std::string summaryPath = exportPath + "/model_summary.txt";
            std::ofstream file(summaryPath);
            if (file.is_open()) {
                file << "Model Summary\n";
                file << "============\n\n";
                file << "Model Type: " << model->getName() << "\n\n";
                
                file << "Parameters:\n";
                for (const auto& param : model->getParameters()) {
                    file << "  " << param.first << ": " << param.second << "\n";
                }
                file << "\n";
                
                file << "Statistics:\n";
                for (const auto& stat : model->getStatistics()) {
                    file << "  " << stat.first << ": " << stat.second << "\n";
                }
                file << "\n";
                
                file << "Input Variables:\n";
                for (const auto& var : inputVariables) {
                    file << "  " << var << "\n";
                }
                file << "\n";
                
                file << "Target Variable: " << targetVariable << "\n";
                file.close();
                
                fl_message("Model summary exported to %s", summaryPath.c_str());
            } else {
                fl_alert("Error: Failed to open file for writing: %s", summaryPath.c_str());
            }
        }
        
        if (exportCSV) {
            // Generate CSV with predictions
            std::string csvPath = exportPath + "/predictions.csv";
            std::ofstream file(csvPath);
            if (file.is_open()) {
                // Write header
                file << targetVariable << ",Predicted\n";
                
                // Generate predictions
                Eigen::MatrixXd X = dataFrame->toMatrix(inputVariables);
                Eigen::VectorXd predictions = model->predict(X);
                std::vector<double> targetData = dataFrame->getColumn(targetVariable);
                
                // Write data
                for (int i = 0; i < predictions.size(); ++i) {
                    file << targetData[i] << "," << predictions(i) << "\n";
                }
                
                file.close();
                fl_message("Predictions exported to %s", csvPath.c_str());
            } else {
                fl_alert("Error: Failed to open file for writing: %s", csvPath.c_str());
            }
        }
        
        if (exportPlots) {
            // Export all plots
            for (size_t i = 0; i < plotNavigator->getPlotCount(); ++i) {
                std::string plotPath = exportPath + "/plot_" + std::to_string(i+1) + ".png";
                if (!plotNavigator->savePlotToFile(i, plotPath)) {
                    fl_alert("Error: Failed to save plot to %s", plotPath.c_str());
                }
            }
            fl_message("Plots exported to %s", exportPath.c_str());
        }
    }
    catch (const std::exception& e) {
        fl_alert("Error exporting results: %s", e.what());
    }
}

// Model-specific display methods
void ResultsView::updateLinearRegressionDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create specific plots for linear regression
    createPlots();
    
    // Additional model-specific displays
    if (model && equationDisplay) {  // Add null check for equationDisplay
        // Add equation display
        std::string equation = getEquationString();
        equationDisplay->copy_label(equation.c_str());
    }
}

void ResultsView::updateElasticNetDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create common plots
    createPlots();
    
    // Add specific plots for ElasticNet
    if (model && dataFrame) {
        // Add regularization path plot if available
        // For now, use standard plots
    }
}

void ResultsView::updateRandomForestDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for Random Forest
    if (model && dataFrame) {
        // Add feature importance plot
        auto importance = model->getFeatureImportance();
        plotNavigator->createPlot(dataFrame, model, "importance", "Feature Importance");
    }
}

void ResultsView::updateXGBoostDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for XGBoost
    if (model && dataFrame) {
        // Add feature importance plot
        auto importance = model->getFeatureImportance();
        plotNavigator->createPlot(dataFrame, model, "importance", "Feature Importance");
    }
}

void ResultsView::updateGradientBoostingDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for Gradient Boosting
    if (model && dataFrame) {
        // Add feature importance plot
        auto importance = model->getFeatureImportance();
        plotNavigator->createPlot(dataFrame, model, "importance", "Feature Importance");
    }
}

void ResultsView::updateNeuralNetworkDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for Neural Network
    if (model && dataFrame) {
        // Add neural network architecture visualization if available
        if (hyperparameters.count("hiddenLayerSizes")) {
            std::string hiddenLayers = hyperparameters.at("hiddenLayerSizes");
            std::vector<int> layerSizes;
            
            // Parse hidden layer sizes
            std::stringstream ss(hiddenLayers);
            std::string item;
            while (std::getline(ss, item, ',')) {
                try {
                    layerSizes.push_back(std::stoi(item));
                } catch (...) {
                    // Skip invalid values
                }
            }
            
            // Add input and output layer sizes
            layerSizes.insert(layerSizes.begin(), inputVariables.size());
            layerSizes.push_back(1); // Regression has 1 output
            
            // Create architecture plot
            plotNavigator->createPlot(dataFrame, model, "nn_architecture", "Neural Network Architecture");
        }
    }
}

// Export methods for different model types
void ResultsView::exportLinearRegressionResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportElasticNetResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportRandomForestResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportXGBoostResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportGradientBoostingResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportNeuralNetworkResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::setBackButtonCallback(std::function<void()> callback) {
    backButtonCallback = callback;
}

void ResultsView::backButtonCallback_static(Fl_Widget* widget, void* userData) {
    ResultsView* self = static_cast<ResultsView*>(userData);
    self->handleBackButton();
}

void ResultsView::exportButtonCallback_static(Fl_Widget* widget, void* userData) {
    ResultsView* self = static_cast<ResultsView*>(userData);
    self->handleExportButton();
}

void ResultsView::handleBackButton() {
    if (backButtonCallback) {
        backButtonCallback();
    }
}

void ResultsView::handleExportButton() {
    if (!model || !dataFrame) {
        fl_alert("No model or data available to export!");
        return;
    }

    exportDialog->setModel(model);
    exportDialog->show();
}

void PlotWidget::createResidualPlot(const std::vector<double>& actualValues,
                                  const std::vector<double>& predictedValues,
                                  const std::string& title,
                                  const std::string& tempDataPath,
                                  const std::string& tempImagePath,
                                  const std::string& tempScriptPath)
{
    if (actualValues.size() != predictedValues.size() || actualValues.empty()) {
        return;
    }

    // Store the data for regeneration on resize
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedTitle = title;
    currentPlotType = PlotType::Residual;

    // Create a data file with actual and predicted values
    std::stringstream dataContent;
    dataContent << "actual,predicted,residual\n";
    
    for (size_t i = 0; i < actualValues.size(); ++i) {
        double residual = actualValues[i] - predictedValues[i];
        dataContent << actualValues[i] << "," << predictedValues[i] << "," << residual << "\n";
    }
    
    if (!createTempDataFile(dataContent.str(), tempDataPath)) {
        return;
    }
    
    // Create Python script for plotting
    std::stringstream scriptContent;
    scriptContent << "import matplotlib\n"
                 << "matplotlib.use('Agg')\n"
                 << "import matplotlib.pyplot as plt\n"
                 << "import pandas as pd\n"
                 << "import numpy as np\n\n"
                 << "# Set figure size based on widget dimensions\n"
                 << "plt.figure(figsize=(" << plotBox->w() / 100.0 << ", " << plotBox->h() / 100.0 << "), dpi=100)\n\n"
                 << "# Read data\n"
                 << "data = pd.read_csv('" << tempDataPath << "')\n\n"
                 << "# Create residual plot\n"
                 << "plt.scatter(data['predicted'], data['residual'], alpha=0.6)\n"
                 << "plt.axhline(y=0, color='r', linestyle='-')\n"
                 << "plt.xlabel('Predicted Values')\n"
                 << "plt.ylabel('Residuals')\n"
                 << "plt.title('" << title << "')\n"
                 << "plt.grid(True, linestyle='--', alpha=0.7)\n\n"
                 << "# Save the plot\n"
                 << "plt.tight_layout()\n"
                 << "plt.savefig('" << tempImagePath << "', dpi=100)\n"
                 << "plt.close()\n";
                 
    if (!createTempDataFile(scriptContent.str(), tempScriptPath)) {
        return;
    }
    
    // Run the Python script to generate the plot
    std::string command = "python \"" + tempScriptPath + "\"";
    int result = system(command.c_str());
    
    if (result != 0) {
        return;
    }
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
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
}

void PlotWidget::createLearningCurvePlot(const std::vector<double>& trainingScores,
                                       const std::vector<double>& validationScores,
                                       const std::vector<int>& trainingSizes,
                                       const std::string& title,
                                       const std::string& tempDataPath,
                                       const std::string& tempImagePath,
                                       const std::string& tempScriptPath)
{
    if (trainingScores.size() != validationScores.size() || 
        trainingScores.size() != trainingSizes.size() || 
        trainingScores.empty()) {
        return;
    }

    // Store data for regeneration (store all data for learning curves)
    storedTitle = title;
    storedTrainingScores = trainingScores;
    storedValidationScores = validationScores;
    storedTrainingSizes = trainingSizes;
    currentPlotType = PlotType::LearningCurve;

    // Create a data file with training and validation scores
    std::stringstream dataContent;
    dataContent << "training_size,training_score,validation_score\n";
    
    for (size_t i = 0; i < trainingScores.size(); ++i) {
        dataContent << trainingSizes[i] << "," << trainingScores[i] << "," << validationScores[i] << "\n";
    }
    
    if (!createTempDataFile(dataContent.str(), tempDataPath)) {
        return;
    }
    
    // Create Python script for plotting
    std::stringstream scriptContent;
    scriptContent << "import matplotlib\n"
                 << "matplotlib.use('Agg')\n"
                 << "import matplotlib.pyplot as plt\n"
                 << "import pandas as pd\n"
                 << "import numpy as np\n\n"
                 << "# Set figure size based on widget dimensions\n"
                 << "plt.figure(figsize=(" << plotBox->w() / 100.0 << ", " << plotBox->h() / 100.0 << "), dpi=100)\n\n"
                 << "# Read data\n"
                 << "data = pd.read_csv('" << tempDataPath << "')\n\n"
                 << "# Create learning curve plot\n"
                 << "plt.plot(data['training_size'], data['training_score'], 'o-', label='Training score')\n"
                 << "plt.plot(data['training_size'], data['validation_score'], 'o-', label='Validation score')\n"
                 << "plt.xlabel('Training examples')\n"
                 << "plt.ylabel('Score')\n"
                 << "plt.title('" << title << "')\n"
                 << "plt.legend(loc='best')\n"
                 << "plt.grid(True, linestyle='--', alpha=0.7)\n\n"
                 << "# Save the plot\n"
                 << "plt.tight_layout()\n"
                 << "plt.savefig('" << tempImagePath << "', dpi=100)\n"
                 << "plt.close()\n";
                 
    if (!createTempDataFile(scriptContent.str(), tempScriptPath)) {
        return;
    }
    
    // Run the Python script to generate the plot
    std::string command = "python \"" + tempScriptPath + "\"";
    int result = system(command.c_str());
    
    if (result != 0) {
        return;
    }
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
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
}

void PlotWidget::createNeuralNetworkArchitecturePlot(const std::vector<int>& layerSizes,
                                                   const std::string& title,
                                                   const std::string& tempDataPath,
                                                   const std::string& tempImagePath,
                                                   const std::string& tempScriptPath)
{
    if (layerSizes.size() < 2) {
        return;
    }

    // Store data for regeneration
    storedTitle = title;
    storedLayerSizes = layerSizes;
    currentPlotType = PlotType::NeuralNetworkArchitecture;

    // Create a data file with layer sizes
    std::stringstream dataContent;
    dataContent << "layer_index,layer_size\n";
    
    for (size_t i = 0; i < layerSizes.size(); ++i) {
        dataContent << i << "," << layerSizes[i] << "\n";
    }
    
    if (!createTempDataFile(dataContent.str(), tempDataPath)) {
        return;
    }
    
    // Create Python script for plotting
    std::stringstream scriptContent;
    scriptContent << "import matplotlib\n"
                 << "matplotlib.use('Agg')\n"
                 << "import matplotlib.pyplot as plt\n"
                 << "import pandas as pd\n"
                 << "import numpy as np\n\n"
                 << "# Set figure size based on widget dimensions\n"
                 << "plt.figure(figsize=(" << plotBox->w() / 100.0 << ", " << plotBox->h() / 100.0 << "), dpi=100)\n\n"
                 << "# Read data\n"
                 << "data = pd.read_csv('" << tempDataPath << "')\n\n"
                 << "# Neural network visualization function\n"
                 << "def draw_neural_net(ax, left, right, bottom, top, layer_sizes):\n"
                 << "    n_layers = len(layer_sizes)\n"
                 << "    v_spacing = (top - bottom)/float(max(layer_sizes))\n"
                 << "    h_spacing = (right - left)/float(n_layers - 1)\n"
                 << "    \n"
                 << "    # Nodes\n"
                 << "    for n, layer_size in enumerate(layer_sizes):\n"
                 << "        layer_top = v_spacing*(layer_size - 1)/2. + (top + bottom)/2.\n"
                 << "        for m in range(layer_size):\n"
                 << "            circle = plt.Circle((n*h_spacing + left, layer_top - m*v_spacing), v_spacing/4.,\n"
                 << "                              color='w', ec='k', zorder=4)\n"
                 << "            ax.add_artist(circle)\n"
                 << "            \n"
                 << "    # Edges\n"
                 << "    for n, (layer_size_a, layer_size_b) in enumerate(zip(layer_sizes[:-1], layer_sizes[1:])):\n"
                 << "        layer_top_a = v_spacing*(layer_size_a - 1)/2. + (top + bottom)/2.\n"
                 << "        layer_top_b = v_spacing*(layer_size_b - 1)/2. + (top + bottom)/2.\n"
                 << "        for m in range(layer_size_a):\n"
                 << "            for o in range(layer_size_b):\n"
                 << "                line = plt.Line2D([n*h_spacing + left, (n + 1)*h_spacing + left],\n"
                 << "                               [layer_top_a - m*v_spacing, layer_top_b - o*v_spacing], c='k')\n"
                 << "                ax.add_artist(line)\n"
                 << "\n"
                 << "# Get layer sizes\n"
                 << "layer_sizes = data['layer_size'].tolist()\n\n"
                 << "# Create neural network architecture visualization\n"
                 << "fig = plt.figure(figsize=(" << plotBox->w() / 100.0 << ", " << plotBox->h() / 100.0 << "))\n"
                 << "ax = fig.gca()\n"
                 << "ax.axis('off')\n"
                 << "draw_neural_net(ax, .1, .9, .1, .9, layer_sizes)\n\n"
                 << "# Add layer labels\n"
                 << "layer_names = ['Input'] + ['Hidden ' + str(i+1) for i in range(len(layer_sizes)-2)] + ['Output']\n"
                 << "for i, name in enumerate(layer_names):\n"
                 << "    plt.text(i/float(len(layer_sizes)-1), 0.01, name, ha='center', va='center')\n\n"
                 << "plt.title('" << title << "')\n\n"
                 << "# Save the plot\n"
                 << "plt.tight_layout()\n"
                 << "plt.savefig('" << tempImagePath << "', dpi=100)\n"
                 << "plt.close()\n";
                 
    if (!createTempDataFile(scriptContent.str(), tempScriptPath)) {
        return;
    }
    
    // Run the Python script to generate the plot
    std::string command = "python \"" + tempScriptPath + "\"";
    int result = system(command.c_str());
    
    if (result != 0) {
        return;
    }
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
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
}

void PlotWidget::createTreeVisualizationPlot(const std::string& treeStructure,
                                           const std::string& title,
                                           const std::string& tempDataPath,
                                           const std::string& tempImagePath,
                                           const std::string& tempScriptPath)
{
    if (treeStructure.empty()) {
        return;
    }

    // Store data for regeneration
    storedTitle = title;
    storedTreeStructure = treeStructure;
    currentPlotType = PlotType::TreeVisualization;

    // Create a data file with tree structure
    std::stringstream dataContent;
    dataContent << treeStructure;
    
    if (!createTempDataFile(dataContent.str(), tempDataPath)) {
        return;
    }
    
    // Create Python script for plotting a simplified tree visualization
    std::stringstream scriptContent;
    scriptContent << "import matplotlib\n"
                 << "matplotlib.use('Agg')\n"
                 << "import matplotlib.pyplot as plt\n"
                 << "import numpy as np\n\n"
                 << "# Set figure size based on widget dimensions\n"
                 << "plt.figure(figsize=(" << plotBox->w() / 100.0 << ", " << plotBox->h() / 100.0 << "), dpi=100)\n\n"
                 << "# For a real implementation, we would use libraries like scikit-learn's tree.export_graphviz\n"
                 << "# and graphviz to visualize the tree. For this simplified version, we'll create a placeholder.\n\n"
                 << "# Create a placeholder tree visualization\n"
                 << "plt.text(0.5, 0.5, 'Tree Visualization Placeholder\\n\\nIn a real implementation, this would show\\n"
                 << "an actual decision tree diagram using libraries\\nlike graphviz.', ha='center', va='center', fontsize=12)\n"
                 << "plt.title('" << title << "')\n"
                 << "plt.axis('off')\n\n"
                 << "# Save the plot\n"
                 << "plt.tight_layout()\n"
                 << "plt.savefig('" << tempImagePath << "', dpi=100)\n"
                 << "plt.close()\n";
                 
    if (!createTempDataFile(scriptContent.str(), tempScriptPath)) {
        return;
    }
    
    // Run the Python script to generate the plot
    std::string command = "python \"" + tempScriptPath + "\"";
    int result = system(command.c_str());
    
    if (result != 0) {
        return;
    }
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
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
}