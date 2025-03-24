#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <algorithm>  // Add algorithm before Windows.h
#include <unordered_map>
#include <FL/Fl_Widget.H>

// ImGui headers
#include "imgui.h"
#include "../backends/imgui_impl_fltk.h"
#include "../backends/imgui_impl_opengl3.h"

// Prevent Windows from defining min/max macros
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>  // For SHGetFolderPathA and CSIDL_LOCAL_APPDATA

#include <Eigen/Dense>
#include "data/DataFrame.h"

namespace fs = std::filesystem;

// Forward declaration
class Fl_Widget;

/**
 * @brief C++ plotting utility using ImGui for direct rendering
 */
class PlottingUtility {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static PlottingUtility& getInstance();
    
    /**
     * @brief Initialize the plotting utility with a parent widget
     * @param parentWidget The parent widget to render within
     */
    void initialize(Fl_Widget* parentWidget);
    
    /**
     * @brief Clean up plotting utility resources
     */
    void cleanup();
    
    /**
     * @brief Create a scatter plot
     * @param actual Actual values
     * @param predicted Predicted values
     * @param title Plot title
     * @param xLabel X-axis label
     * @param yLabel Y-axis label
     * @param width Plot width
     * @param height Plot height
     */
    void createScatterPlot(
        const std::vector<double>& actual,
        const std::vector<double>& predicted,
        const std::string& title,
        const std::string& xLabel,
        const std::string& yLabel,
        int width = 0,
        int height = 0
    );
    
    /**
     * @brief Create a time series plot
     * @param actual Actual values
     * @param predicted Predicted values
     * @param title Plot title
     * @param width Plot width
     * @param height Plot height
     */
    void createTimeSeriesPlot(
        const std::vector<double>& actual,
        const std::vector<double>& predicted,
        const std::string& title,
        int width = 0,
        int height = 0
    );
    
    /**
     * @brief Create a residual plot
     * @param predicted Predicted values
     * @param residuals Residual values
     * @param title Plot title
     * @param width Plot width
     * @param height Plot height
     */
    void createResidualPlot(
        const std::vector<double>& predicted,
        const std::vector<double>& residuals,
        const std::string& title,
        int width = 0,
        int height = 0
    );
    
    /**
     * @brief Create a feature importance plot
     * @param importance Map of feature names to importance values
     * @param title Plot title
     * @param width Plot width
     * @param height Plot height
     */
    void createImportancePlot(
        const std::unordered_map<std::string, double>& importance,
        const std::string& title,
        int width = 0,
        int height = 0
    );
    
    /**
     * @brief Create a learning curve plot
     * @param trainingSizes Training set sizes
     * @param trainingScores Training scores
     * @param validationScores Validation scores
     * @param title Plot title
     * @param width Plot width
     * @param height Plot height
     */
    void createLearningCurvePlot(
        const std::vector<double>& trainingSizes,
        const std::vector<double>& trainingScores,
        const std::vector<double>& validationScores,
        const std::string& title,
        int width = 0,
        int height = 0
    );
    
    /**
     * @brief Render the current plot
     */
    void render();
    
    /**
     * @brief Clear the current plot data
     */
    void clear();

private:
    // Parent widget to render within
    Fl_Widget* parentWidget;
    
    /**
     * @brief Set up the ImPlot style
     */
    void setupPlotStyle();
    
    /**
     * @brief Render a plot window with the given title and dimensions
     * @param title Plot title
     * @param width Plot width (0 = use parent width)
     * @param height Plot height (0 = use parent height)
     * @return True if the window was created successfully, false otherwise
     */
    bool renderPlotWindow(const std::string& title, int width, int height);
    
    /**
     * @brief Calculate plot dimensions based on the parent widget or provided values
     * @param width Desired width (0 = use parent width)
     * @param height Desired height (0 = use parent height)
     * @return std::pair<int, int> Calculated width and height
     */
    std::pair<int, int> calculatePlotDimensions(int width, int height);
    
    /**
     * @brief Render scatter plot using ImPlot
     */
    void renderScatterPlot();
    
    /**
     * @brief Render time series plot using ImPlot
     */
    void renderTimeSeriesPlot();
    
    /**
     * @brief Render residual plot using ImPlot
     */
    void renderResidualPlot();
    
    /**
     * @brief Render feature importance plot using ImPlot
     */
    void renderImportancePlot();
    
    /**
     * @brief Render learning curve plot using ImPlot
     */
    void renderLearningCurvePlot();
    
    /**
     * @brief Data structure for storing plot data
     */
    struct PlotData {
        enum class Type {
            None,
            Scatter,
            TimeSeries,
            Residual,
            Importance,
            LearningCurve
        } type = Type::None;
        
        std::string title;
        std::string xLabel;
        std::string yLabel;
        
        std::vector<double> xValues;
        std::vector<double> yValues;
        std::vector<double> y2Values;
        
        std::unordered_map<std::string, double> importanceValues;
        
        std::vector<double> trainingSizes;
        std::vector<double> trainingScores;
        std::vector<double> validationScores;
        
        int width = 0;
        int height = 0;
    };
    
    // Current plot data
    PlotData currentPlot;
    
    /**
     * @brief Get the path to the application's executable directory
     * @return std::string The executable directory path
     */
    static std::string getExecutableDir() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        return std::string(buffer).substr(0, pos);
    }
    
    /**
     * @brief Get the path to the application's data directory
     * @return std::string The application data directory path
     */
    static std::string getAppDataDir() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
            return std::string(path) + "\\Model_Builder_Tool";
        }
        return "";
    }
};