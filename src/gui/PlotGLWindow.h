#pragma once

// Include dependency configuration
#include "include/DependencyConfig.h"

// Standard library headers
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>

// Third-party library headers
#include <FL/Fl_Gl_Window.H>
#include "imgui.h"
#include "implot.h"

// Project headers
#include "utils/Logger.h"

// Forward declarations for ImGui/ImPlot
namespace ImGui { class ImGuiContext; }
namespace ImPlot { class ImPlotContext; }

/**
 * @brief Custom OpenGL window for plots with integrated ImGui/ImPlot
 * 
 * This class handles the OpenGL context creation and initialization
 * for plotting functionality. It directly manages ImGui/ImPlot rendering
 * without needing an external utility class.
 */
class PlotGLWindow : public Fl_Gl_Window {
public:
    /**
     * @brief Plot types that can be displayed
     */
    enum class PlotType {
        Scatter = 0,
        TimeSeries = 1,
        Residual = 2,
        Importance = 3,
        LearningCurve = 4
    };

    /**
     * @brief Constructor
     * 
     * @param x X position
     * @param y Y position
     * @param w Width
     * @param h Height
     * @param label Window label
     */
    PlotGLWindow(int x, int y, int w, int h, const char* label = nullptr);
    
    /**
     * @brief Destructor - cleans up ImGui/ImPlot resources
     */
    ~PlotGLWindow();
    
    /**
     * @brief Draw the window content - overridden from Fl_Gl_Window
     */
    void draw() override;
    
    /**
     * @brief Check if the plotting system is initialized
     * 
     * @return true if ImGui/ImPlot is initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief Set the current plot type
     * 
     * @param type Type of plot to display
     */
    void setPlotType(PlotType type) { 
        currentPlotType = type;
        LOG_INFO("Plot type set to: " + std::to_string(static_cast<int>(currentPlotType)), "PlotGLWindow");
    }
    
    /**
     * @brief Get the current plot type
     * 
     * @return Current plot type
     */
    PlotType getPlotType() const { return currentPlotType; }
    
    /**
     * @brief Create a scatter plot
     * 
     * @param xValues X values
     * @param yValues Y values
     * @param title Plot title
     * @param xLabel X-axis label
     * @param yLabel Y-axis label
     */
    void createScatterPlot(
        const std::vector<double>& xValues,
        const std::vector<double>& yValues,
        const std::string& title = "Scatter Plot",
        const std::string& xLabel = "X",
        const std::string& yLabel = "Y"
    );
    
    /**
     * @brief Create a time series plot
     * 
     * @param actual Actual values
     * @param predicted Predicted values
     * @param title Plot title
     */
    void createTimeSeriesPlot(
        const std::vector<double>& actual,
        const std::vector<double>& predicted,
        const std::string& title = "Time Series Plot"
    );
    
    /**
     * @brief Create a residual plot
     * 
     * @param predicted Predicted values
     * @param residuals Residual values
     * @param title Plot title
     */
    void createResidualPlot(
        const std::vector<double>& predicted,
        const std::vector<double>& residuals,
        const std::string& title = "Residual Plot"
    );
    
    /**
     * @brief Create a feature importance plot
     * 
     * @param importance Map of feature names to importance values
     * @param title Plot title
     */
    void createImportancePlot(
        const std::unordered_map<std::string, double>& importance,
        const std::string& title = "Feature Importance"
    );
    
    /**
     * @brief Create a learning curve plot
     * 
     * @param trainingSizes Training set sizes
     * @param trainingScores Training scores
     * @param validationScores Validation scores
     * @param title Plot title
     */
    void createLearningCurvePlot(
        const std::vector<double>& trainingSizes,
        const std::vector<double>& trainingScores,
        const std::vector<double>& validationScores,
        const std::string& title = "Learning Curve"
    );
    
private:
    /**
     * @brief Initialize ImGui and ImPlot systems
     * 
     * @return true if initialization succeeded, false otherwise
     */
    bool initializeImGui();
    
    /**
     * @brief Clean up ImGui and ImPlot resources
     */
    void cleanupImGui();
    
    /**
     * @brief Render the current plot
     */
    void renderPlot();
    
    /**
     * @brief Render a scatter plot
     */
    void renderScatterPlot();
    
    /**
     * @brief Render a time series plot
     */
    void renderTimeSeriesPlot();
    
    /**
     * @brief Render a residual plot
     */
    void renderResidualPlot();
    
    /**
     * @brief Render a feature importance plot
     */
    void renderImportancePlot();
    
    /**
     * @brief Render a learning curve plot
     */
    void renderLearningCurvePlot();
    
    /**
     * @brief Draw an error message on the window
     * 
     * @param message The error message to display
     */
    void drawErrorMessage(const std::string& message);
    
    /**
     * @brief Draw a test rectangle to verify OpenGL is working
     */
    void drawTestRectangle();
    
    // State variables
    bool initialized = false;
    bool openGLVersionLogged = false;
    
    // Plot data
    PlotType currentPlotType = PlotType::Scatter;
    std::string plotTitle;
    std::string xLabel;
    std::string yLabel;
    
    // Data for various plot types
    std::vector<double> xValues;
    std::vector<double> yValues;
    std::vector<double> y2Values;  // Used for time series (predicted values)
    std::vector<double> trainingSizes;
    std::vector<double> trainingScores;
    std::vector<double> validationScores;
    std::unordered_map<std::string, double> importanceValues;
}; 