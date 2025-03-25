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
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Gl_Window.H>
#include "imgui.h"
#include "implot.h"

// Project headers
#include "utils/Logger.h"

// Forward declarations for ImGui/ImPlot
namespace ImGui { class ImGuiContext; }
namespace ImPlot { class ImPlotContext; }

/**
 * @brief Custom FLTK window for plots with integrated ImGui/ImPlot
 * 
 * This class handles the FLTK window creation and initialization
 * for plotting functionality. It directly manages ImGui/ImPlot rendering
 * using the FLTK backend.
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
     * @brief Draw the window content - overridden from Fl_Box
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
        redraw();
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
     * @brief Draw an error message
     */
    void drawErrorMessage(const std::string& message);
    
    // Plot rendering functions
    void renderScatterPlot();
    void renderTimeSeriesPlot();
    void renderResidualPlot();
    void renderImportancePlot();
    void renderLearningCurvePlot();
    
    // Member variables
    bool initialized;
    PlotType currentPlotType;
    
    // Plot data
    std::string plotTitle;
    std::string xLabel;
    std::string yLabel;
    std::vector<double> xValues;
    std::vector<double> yValues;
    std::vector<double> y2Values;
    std::unordered_map<std::string, double> importanceValues;
    std::vector<double> trainingSizes;
    std::vector<double> trainingScores;
    std::vector<double> validationScores;
}; 