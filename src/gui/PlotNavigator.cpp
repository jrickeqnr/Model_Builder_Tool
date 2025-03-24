#include "gui/PlotNavigator.h"
#include "gui/PlotWidget.h"
#include "utils/Logger.h"
#include <algorithm>
#include <filesystem>

// Define the LOG_WARNING macro if it's not available
#ifndef LOG_WARNING
#define LOG_WARNING(message, component) std::cerr << "[WARNING][" << component << "] " << message << std::endl
#endif

// PlotNavigator implementation
PlotNavigator::PlotNavigator() 
    : currentPlotIndex(0), numPlots(0) {
    LOG_INFO("PlotNavigator initialized", "PlotNavigator");
}

PlotNavigator::~PlotNavigator() {
    clearPlots();
}

void PlotNavigator::createPlot(PlotWidget::PlotType plotType, const std::string& title,
                             const std::vector<double>& xValues, 
                             const std::vector<double>& yValues) {
    LOG_INFO("Creating plot: " + title, "PlotNavigator");
    
    if (plotType == PlotWidget::PlotType::SCATTER) {
        LOG_INFO("Creating scatter plot with " + std::to_string(xValues.size()) + " points", "PlotNavigator");
        
        if (xValues.size() != yValues.size()) {
            LOG_WARNING("X and Y data must have the same size for scatter plots", "PlotNavigator");
            return;
        }
        
        std::unique_ptr<PlotWidget> plot = std::make_unique<PlotWidget>();
        plot->setTitle(title);
        plot->createScatterPlot(xValues, yValues, "Actual", "Predicted");
        plots.push_back(std::move(plot));
    }
    else if (plotType == PlotWidget::PlotType::TIME_SERIES) {
        LOG_INFO("Creating time series plot with " + std::to_string(xValues.size()) + " points", "PlotNavigator");
        
        if (xValues.size() != yValues.size()) {
            LOG_WARNING("Actual and predicted data must have the same size for time series plots", "PlotNavigator");
            return;
        }
        
        std::unique_ptr<PlotWidget> plot = std::make_unique<PlotWidget>();
        plot->setTitle(title);
        plot->createTimeSeriesPlot(xValues, yValues);
        plots.push_back(std::move(plot));
    }
    else if (plotType == PlotWidget::PlotType::RESIDUAL) {
        LOG_INFO("Creating residual plot with " + std::to_string(xValues.size()) + " points", "PlotNavigator");
        
        if (xValues.size() != yValues.size()) {
            LOG_WARNING("Predicted and residual data must have the same size", "PlotNavigator");
            return;
        }
        
        std::unique_ptr<PlotWidget> plot = std::make_unique<PlotWidget>();
        plot->setTitle(title);
        plot->createResidualPlot(xValues, yValues);
        plots.push_back(std::move(plot));
    }
    
    updatePlotCount();
}

void PlotNavigator::createPlot(PlotWidget::PlotType plotType, const std::string& title,
                             const std::unordered_map<std::string, double>& values) {
    LOG_INFO("Creating feature importance plot with " + std::to_string(values.size()) + " features", "PlotNavigator");
    
    if (values.empty()) {
        LOG_WARNING("Cannot create importance plot with empty data", "PlotNavigator");
        return;
    }
    
    std::unique_ptr<PlotWidget> plot = std::make_unique<PlotWidget>();
    plot->setTitle(title);
    plot->createImportancePlot(values);
    plots.push_back(std::move(plot));
    
    updatePlotCount();
}

void PlotNavigator::createPlot(PlotWidget::PlotType plotType, const std::string& title,
                             const std::vector<double>& xValues, 
                             const std::vector<double>& yValues,
                             const std::vector<double>& y2Values) {
    LOG_INFO("Creating learning curve or loss curve plot", "PlotNavigator");
    
    if (xValues.empty() || yValues.empty()) {
        LOG_WARNING("Cannot create plot with empty data", "PlotNavigator");
        return;
    }
    
    if (xValues.size() != yValues.size() || 
        (!y2Values.empty() && xValues.size() != y2Values.size())) {
        LOG_WARNING("Data arrays must have the same size", "PlotNavigator");
        return;
    }
    
    std::unique_ptr<PlotWidget> plot = std::make_unique<PlotWidget>();
    plot->setTitle(title);
    
    if (plotType == PlotWidget::PlotType::LEARNING_CURVE) {
        plot->createLearningCurvePlot(xValues, yValues, y2Values);
    }
    else if (plotType == PlotWidget::PlotType::LOSS_CURVE) {
        plot->createLossCurvePlot(xValues, yValues, y2Values);
    }
    
    plots.push_back(std::move(plot));
    updatePlotCount();
}

void PlotNavigator::showPlot(size_t index) {
    if (index < plots.size()) {
        currentPlotIndex = index;
    }
}

bool PlotNavigator::nextPlot() {
    if (plots.empty()) return false;
    
    if (currentPlotIndex < plots.size() - 1) {
        currentPlotIndex++;
        return true;
    }
    return false;
}

bool PlotNavigator::previousPlot() {
    if (plots.empty()) return false;
    
    if (currentPlotIndex > 0) {
        currentPlotIndex--;
        return true;
    }
    return false;
}

void PlotNavigator::clearPlots() {
    plots.clear();
    currentPlotIndex = 0;
    updatePlotCount();
    LOG_INFO("All plots cleared", "PlotNavigator");
}

size_t PlotNavigator::getPlotCount() const {
    return plots.size();
}

void PlotNavigator::render() {
    if (plots.empty()) {
        ImGui::TextWrapped("No plots available. Create a model first.");
        return;
    }
    
    // Navigation buttons
    ImGui::BeginGroup();
    
    // Only show navigation if we have multiple plots
    if (plots.size() > 1) {
        ImGui::Text("Plot %zu of %zu:", currentPlotIndex + 1, plots.size());
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        
        if (ImGui::Button("Previous")) {
            previousPlot();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Next")) {
            nextPlot();
        }
        
        ImGui::Separator();
    }
    
    // Render the current plot
    if (currentPlotIndex < plots.size()) {
        plots[currentPlotIndex]->render();
        
        // Plot export options
        ImGui::Separator();
        
        if (ImGui::Button("Save Plot Image")) {
            plots[currentPlotIndex]->savePlotToFile();
        }
    }
    
    ImGui::EndGroup();
}

void PlotNavigator::updatePlotCount() {
    numPlots = plots.size();
}

bool PlotNavigator::savePlotToFile(size_t index, const std::string& filename) {
    if (index >= plots.size()) {
        return false;
    }

    try {
        // Create the directory if it doesn't exist
        std::filesystem::path filePath(filename);
        std::filesystem::create_directories(filePath.parent_path());

        // Use the plot widget's save function
        return plots[index]->savePlotToFile(filename);
    }
    catch (const std::exception& e) {
        LOG_WARNING("Failed to save plot: " + std::string(e.what()), "PlotNavigator");
        return false;
    }
} 