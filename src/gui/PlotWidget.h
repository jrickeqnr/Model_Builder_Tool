#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

// Forward declarations
namespace ImPlot {
    struct ImPlotContext;
}

class PlotWidget {
public:
    enum class PlotType {
        NONE,
        SCATTER,
        TIME_SERIES,
        RESIDUAL,
        IMPORTANCE,
        LEARNING_CURVE,
        LOSS_CURVE
    };

    PlotWidget();
    ~PlotWidget();

    // Set plot title
    void setTitle(const std::string& title);

    // Create various plot types
    void createScatterPlot(
        const std::vector<double>& xValues,
        const std::vector<double>& yValues,
        const std::string& xLabel = "X",
        const std::string& yLabel = "Y"
    );

    void createTimeSeriesPlot(
        const std::vector<double>& actual,
        const std::vector<double>& predicted
    );

    void createResidualPlot(
        const std::vector<double>& predicted,
        const std::vector<double>& residuals
    );

    void createImportancePlot(
        const std::unordered_map<std::string, double>& importance
    );

    void createLearningCurvePlot(
        const std::vector<double>& trainSizes,
        const std::vector<double>& trainScores,
        const std::vector<double>& validScores
    );
    
    void createLossCurvePlot(
        const std::vector<double>& epochs,
        const std::vector<double>& trainLoss,
        const std::vector<double>& validLoss = std::vector<double>()
    );
    
    // Regenerate plot with potentially updated data
    void regeneratePlot();
    
    // Save plot to image file
    bool savePlotToFile(const std::string& filename = "");
    
    // Render the plot using ImGui/ImPlot
    void render();
    
    // Get plot type
    PlotType getType() const { return plotType; }
    
    // Get plot title
    const std::string& getTitle() const { return title; }

private:
    // Plot properties
    std::string title;
    std::string xLabel;
    std::string yLabel;
    PlotType plotType;
    
    // Plot data
    std::vector<double> xValues;
    std::vector<double> yValues;
    std::vector<double> y2Values;  // For plots with a second y series (e.g., predicted values)
    
    // For feature importance plots
    std::unordered_map<std::string, double> importanceValues;
    
    // For learning curve plots
    std::vector<double> trainingSizes;
    std::vector<double> trainingScores;
    std::vector<double> validationScores;
    
    // For loss curve plots
    std::vector<double> epochs;
    std::vector<double> trainLoss;
    std::vector<double> validLoss;
    
    // Rendering
    void renderScatterPlot();
    void renderTimeSeriesPlot();
    void renderResidualPlot();
    void renderImportancePlot();
    void renderLearningCurvePlot();
    void renderLossCurvePlot();
}; 