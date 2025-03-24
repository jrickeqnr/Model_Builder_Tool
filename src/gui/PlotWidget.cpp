#include "gui/PlotWidget.h"
#include "utils/Logger.h"
#include "utils/PlottingUtility.h"
#include <algorithm>
#include <string>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>

#include "imgui.h"
#include "implot.h"

// Define the LOG_INFO macro if it's not available
#ifndef LOG_INFO
#define LOG_INFO(message, component) std::cout << "[INFO][" << component << "] " << message << std::endl
#endif

#ifndef LOG_ERR
#define LOG_ERR(message, component) std::cerr << "[ERROR][" << component << "] " << message << std::endl
#endif

#ifndef LOG_WARNING
#define LOG_WARNING(message, component) std::cerr << "[WARNING][" << component << "] " << message << std::endl
#endif

// PlotWidget implementation
PlotWidget::PlotWidget()
    : title("Plot"), xLabel("X"), yLabel("Y"), plotType(PlotType::NONE) {
    LOG_DEBUG("PlotWidget constructor called", "PlotWidget");
}

PlotWidget::~PlotWidget() {
    LOG_DEBUG("PlotWidget destructor called", "PlotWidget");
    // Nothing to clean up
}

void PlotWidget::regeneratePlot() {
    // Nothing to do here, as rendering is done on-demand in the render() method
    LOG_INFO("Plot regeneration requested for plot: " + title, "PlotWidget");
}

void PlotWidget::createScatterPlot(
    const std::vector<double>& xValues,
    const std::vector<double>& yValues,
    const std::string& xLabel,
    const std::string& yLabel
) {
    LOG_DEBUG("Creating scatter plot with " + std::to_string(xValues.size()) + " points", "PlotWidget");
    
    if (xValues.empty() || yValues.empty()) {
        LOG_ERR("Cannot create scatter plot with empty data", "PlotWidget");
        return;
    }
    
    if (xValues.size() != yValues.size()) {
        LOG_ERR("Cannot create scatter plot with mismatched data sizes: X=" + 
                std::to_string(xValues.size()) + ", Y=" + std::to_string(yValues.size()), "PlotWidget");
        return;
    }
    
    // Store plot data
    this->xValues = xValues;
    this->yValues = yValues;
    this->xLabel = xLabel;
    this->yLabel = yLabel;
    this->plotType = PlotType::SCATTER;
    
    LOG_INFO("Scatter plot created successfully with " + std::to_string(xValues.size()) + " points", "PlotWidget");
}

void PlotWidget::createTimeSeriesPlot(
    const std::vector<double>& actual, 
    const std::vector<double>& predicted
) {
    LOG_DEBUG("Creating time series plot with " + std::to_string(actual.size()) + " points", "PlotWidget");
    
    if (actual.empty() || predicted.empty()) {
        LOG_ERR("Cannot create time series plot with empty data", "PlotWidget");
        return;
    }
    
    if (actual.size() != predicted.size()) {
        LOG_ERR("Cannot create time series plot with mismatched data sizes: Actual=" + 
                std::to_string(actual.size()) + ", Predicted=" + std::to_string(predicted.size()), "PlotWidget");
        return;
    }
    
    // Create x values as indices
    std::vector<double> indices(actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        indices[i] = static_cast<double>(i);
    }
    
    // Store plot data
    this->xValues = indices;
    this->yValues = actual;
    this->y2Values = predicted;
    this->xLabel = "Time";
    this->yLabel = "Value";
    this->plotType = PlotType::TIME_SERIES;
    
    LOG_INFO("Time series plot created successfully with " + std::to_string(actual.size()) + " points", "PlotWidget");
}

void PlotWidget::createResidualPlot(
    const std::vector<double>& predicted,
    const std::vector<double>& residuals
) {
    LOG_DEBUG("Creating residual plot with " + std::to_string(predicted.size()) + " points", "PlotWidget");
    
    if (predicted.empty() || residuals.empty()) {
        LOG_ERR("Cannot create residual plot with empty data", "PlotWidget");
        return;
    }
    
    if (predicted.size() != residuals.size()) {
        LOG_ERR("Cannot create residual plot with mismatched data sizes: Predicted=" + 
                std::to_string(predicted.size()) + ", Residuals=" + std::to_string(residuals.size()), "PlotWidget");
        return;
    }
    
    // Store plot data
    this->xValues = predicted;
    this->yValues = residuals;
    this->xLabel = "Predicted";
    this->yLabel = "Residual";
    this->plotType = PlotType::RESIDUAL;
    
    LOG_INFO("Residual plot created successfully with " + std::to_string(residuals.size()) + " points", "PlotWidget");
}

void PlotWidget::createImportancePlot(
    const std::unordered_map<std::string, double>& importance
) {
    LOG_DEBUG("Creating importance plot with " + std::to_string(importance.size()) + " features", "PlotWidget");
    
    if (importance.empty()) {
        LOG_ERR("Cannot create importance plot with empty data", "PlotWidget");
        return;
    }
    
    // Store plot data
    this->importanceValues = importance;
    this->xLabel = "Importance";
    this->yLabel = "Feature";
    this->plotType = PlotType::IMPORTANCE;
    
    // Log feature names and their importance values for debugging
    std::stringstream ss;
    ss << "Features: ";
    for (const auto& [feature, value] : importance) {
        ss << feature << "=" << value << ", ";
    }
    LOG_DEBUG(ss.str(), "PlotWidget");
    
    LOG_INFO("Importance plot created successfully with " + std::to_string(importance.size()) + " features", "PlotWidget");
}

void PlotWidget::createLearningCurvePlot(
    const std::vector<double>& trainSizes,
    const std::vector<double>& trainScores,
    const std::vector<double>& validScores
) {
    LOG_DEBUG("Creating learning curve plot with " + std::to_string(trainSizes.size()) + " points", "PlotWidget");
    
    if (trainSizes.empty() || trainScores.empty()) {
        LOG_ERR("Cannot create learning curve plot with empty data", "PlotWidget");
        return;
    }
    
    if (trainSizes.size() != trainScores.size()) {
        LOG_ERR("Cannot create learning curve plot with mismatched data sizes: TrainSizes=" + 
                std::to_string(trainSizes.size()) + ", TrainScores=" + std::to_string(trainScores.size()), "PlotWidget");
        return;
    }
    
    if (!validScores.empty() && trainSizes.size() != validScores.size()) {
        LOG_ERR("Cannot create learning curve plot with mismatched validation data size: TrainSizes=" + 
                std::to_string(trainSizes.size()) + ", ValidScores=" + std::to_string(validScores.size()), "PlotWidget");
        return;
    }
    
    // Store plot data
    this->trainingSizes = trainSizes;
    this->trainingScores = trainScores;
    this->validationScores = validScores;
    this->xLabel = "Training Examples";
    this->yLabel = "Score";
    this->plotType = PlotType::LEARNING_CURVE;
    
    LOG_INFO("Learning curve plot created successfully with " + std::to_string(trainSizes.size()) + " points", "PlotWidget");
}

void PlotWidget::createLossCurvePlot(
    const std::vector<double>& epochs,
    const std::vector<double>& trainLoss,
    const std::vector<double>& validLoss
) {
    LOG_DEBUG("Creating loss curve plot with " + std::to_string(epochs.size()) + " epochs", "PlotWidget");
    
    if (epochs.empty() || trainLoss.empty()) {
        LOG_ERR("Cannot create loss curve plot with empty data", "PlotWidget");
        return;
    }
    
    if (epochs.size() != trainLoss.size()) {
        LOG_ERR("Cannot create loss curve plot with mismatched data sizes: Epochs=" + 
                std::to_string(epochs.size()) + ", TrainLoss=" + std::to_string(trainLoss.size()), "PlotWidget");
        return;
    }
    
    if (!validLoss.empty() && epochs.size() != validLoss.size()) {
        LOG_ERR("Cannot create loss curve plot with mismatched validation data size: Epochs=" + 
                std::to_string(epochs.size()) + ", ValidLoss=" + std::to_string(validLoss.size()), "PlotWidget");
        return;
    }
    
    // Store plot data
    this->epochs = epochs;
    this->trainLoss = trainLoss;
    this->validLoss = validLoss;
    this->xLabel = "Epoch";
    this->yLabel = "Loss";
    this->plotType = PlotType::LOSS_CURVE;
    
    LOG_INFO("Loss curve plot created successfully with " + std::to_string(epochs.size()) + " epochs", "PlotWidget");
}

bool PlotWidget::savePlotToFile(const std::string& filename) {
    LOG_DEBUG("Attempting to save plot to file: " + (filename.empty() ? "auto-generated filename" : filename), "PlotWidget");
    
    std::string outputFile = filename;
    
    if (outputFile.empty()) {
        // Generate a default filename based on plot type and title
        std::string safeTitle = title;
        // Replace spaces and special characters
        std::replace(safeTitle.begin(), safeTitle.end(), ' ', '_');
        std::replace(safeTitle.begin(), safeTitle.end(), '/', '_');
        std::replace(safeTitle.begin(), safeTitle.end(), '\\', '_');
        
        outputFile = safeTitle + ".png";
    }
    
    // TODO: Implement actual plot saving using ImGui screenshot functionality
    // Currently this would require additional implementation for ImGui/ImPlot
    
    LOG_WARNING("Plot saving to file not yet implemented: " + outputFile, "PlotWidget");
    return false;
}

void PlotWidget::setTitle(const std::string& title) {
    LOG_DEBUG("Setting plot title to: " + title, "PlotWidget");
    this->title = title;
}

void PlotWidget::render() {
    LOG_DEBUG("Rendering plot of type: " + std::to_string(static_cast<int>(plotType)) + 
              ", title: " + title, "PlotWidget");
    
    try {
        // Set flags for plot rendering
        ImPlotFlags plotFlags = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMenus;
        
        // Render the appropriate plot based on type
        switch (plotType) {
            case PlotType::SCATTER:
                renderScatterPlot();
                break;
                
            case PlotType::TIME_SERIES:
                renderTimeSeriesPlot();
                break;
                
            case PlotType::RESIDUAL:
                renderResidualPlot();
                break;
                
            case PlotType::IMPORTANCE:
                renderImportancePlot();
                break;
                
            case PlotType::LEARNING_CURVE:
                renderLearningCurvePlot();
                break;
                
            case PlotType::LOSS_CURVE:
                renderLossCurvePlot();
                break;
                
            case PlotType::NONE:
            default:
                LOG_WARNING("No plot data available to render", "PlotWidget");
                ImGui::TextWrapped("No plot data available.");
                break;
        }
        
        LOG_DEBUG("Plot rendering completed successfully", "PlotWidget");
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during plot rendering: " + std::string(e.what()), "PlotWidget");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering plot: %s", e.what());
    }
    catch (...) {
        LOG_ERR("Unknown exception during plot rendering", "PlotWidget");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown error rendering plot");
    }
}

void PlotWidget::renderScatterPlot() {
    LOG_DEBUG("Rendering scatter plot with " + std::to_string(xValues.size()) + " points", "PlotWidget");
    
    try {
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str());
            
            // Add scatter plot
            ImPlot::PlotScatter("Data", 
                              xValues.data(), 
                              yValues.data(), 
                              static_cast<int>(xValues.size()));
            
            // Add identity line (y=x) if data range allows
            if (!xValues.empty() && !yValues.empty()) {
                double minX = *std::min_element(xValues.begin(), xValues.end());
                double maxX = *std::max_element(xValues.begin(), xValues.end());
                double minY = *std::min_element(yValues.begin(), yValues.end());
                double maxY = *std::max_element(yValues.begin(), yValues.end());
                
                double min = std::min(minX, minY);
                double max = std::max(maxX, maxY);
                
                std::vector<double> lineX = {min, max};
                std::vector<double> lineY = {min, max};
                
                ImPlot::PlotLine("y=x", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
                
                LOG_DEBUG("Added identity line from " + std::to_string(min) + " to " + std::to_string(max), "PlotWidget");
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Scatter plot rendering completed", "PlotWidget");
        } else {
            LOG_ERR("Failed to begin scatter plot", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during scatter plot rendering: " + std::string(e.what()), "PlotWidget");
        throw;
    }
}

void PlotWidget::renderTimeSeriesPlot() {
    LOG_DEBUG("Rendering time series plot with " + std::to_string(yValues.size()) + 
              " actual and " + std::to_string(y2Values.size()) + " predicted points", "PlotWidget");
    
    try {
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str());
            
            // Add actual values line
            ImPlot::PlotLine("Actual", 
                           xValues.data(), 
                           yValues.data(), 
                           static_cast<int>(xValues.size()));
            
            // Add predicted values line
            ImPlot::PlotLine("Predicted", 
                           xValues.data(), 
                           y2Values.data(), 
                           static_cast<int>(xValues.size()));
            
            ImPlot::EndPlot();
            LOG_DEBUG("Time series plot rendering completed", "PlotWidget");
        } else {
            LOG_ERR("Failed to begin time series plot", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during time series plot rendering: " + std::string(e.what()), "PlotWidget");
        throw;
    }
}

void PlotWidget::renderResidualPlot() {
    LOG_DEBUG("Rendering residual plot with " + std::to_string(xValues.size()) + " points", "PlotWidget");
    
    try {
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str());
            
            // Add residual scatter plot
            ImPlot::PlotScatter("Residuals", 
                              xValues.data(), 
                              yValues.data(), 
                              static_cast<int>(xValues.size()));
            
            // Add zero line
            if (!xValues.empty()) {
                double minX = *std::min_element(xValues.begin(), xValues.end());
                double maxX = *std::max_element(xValues.begin(), xValues.end());
                std::vector<double> lineX = {minX, maxX};
                std::vector<double> lineY = {0.0, 0.0};
                
                ImPlot::PlotLine("Zero", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
                
                LOG_DEBUG("Added zero line from x=" + std::to_string(minX) + " to x=" + std::to_string(maxX), "PlotWidget");
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Residual plot rendering completed", "PlotWidget");
        } else {
            LOG_ERR("Failed to begin residual plot", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during residual plot rendering: " + std::string(e.what()), "PlotWidget");
        throw;
    }
}

void PlotWidget::renderImportancePlot() {
    LOG_DEBUG("Rendering importance plot with " + std::to_string(importanceValues.size()) + " features", "PlotWidget");
    
    try {
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1))) {
            // For feature importance, we create a horizontal bar chart
            ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str(), 
                            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            
            // Convert unordered_map to vectors and sort by importance
            std::vector<std::pair<std::string, double>> sortedImportance;
            for (const auto& pair : importanceValues) {
                sortedImportance.push_back(pair);
            }
            
            std::sort(sortedImportance.begin(), sortedImportance.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    });
            
            LOG_DEBUG("Sorted " + std::to_string(sortedImportance.size()) + " features by importance", "PlotWidget");
            
            // Prepare data for plotting
            std::vector<double> values;
            std::vector<const char*> labels;
            
            for (const auto& pair : sortedImportance) {
                values.push_back(pair.second);
                labels.push_back(pair.first.c_str());
            }
            
            // Bar chart
            ImPlot::PlotBars("Importance", values.data(), static_cast<int>(values.size()), 0.75, 0.0, ImPlotBarsFlags_Horizontal);
            
            ImPlot::EndPlot();
            LOG_DEBUG("Importance plot rendering completed", "PlotWidget");
        } else {
            LOG_ERR("Failed to begin importance plot", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during importance plot rendering: " + std::string(e.what()), "PlotWidget");
        throw;
    }
}

void PlotWidget::renderLearningCurvePlot() {
    LOG_DEBUG("Rendering learning curve plot with " + std::to_string(trainingSizes.size()) + 
              " training points and " + std::to_string(validationScores.size()) + " validation points", "PlotWidget");
    
    try {
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str());
            
            // Add training score line
            ImPlot::PlotLine("Training Score", 
                           trainingSizes.data(), 
                           trainingScores.data(), 
                           static_cast<int>(trainingSizes.size()));
            
            // Add validation score line
            if (!validationScores.empty()) {
                ImPlot::PlotLine("Validation Score", 
                               trainingSizes.data(), 
                               validationScores.data(), 
                               static_cast<int>(trainingSizes.size()));
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Learning curve plot rendering completed", "PlotWidget");
        } else {
            LOG_ERR("Failed to begin learning curve plot", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during learning curve plot rendering: " + std::string(e.what()), "PlotWidget");
        throw;
    }
}

void PlotWidget::renderLossCurvePlot() {
    LOG_DEBUG("Rendering loss curve plot with " + std::to_string(epochs.size()) + 
              " epochs, " + std::to_string(trainLoss.size()) + " training loss points, and " + 
              std::to_string(validLoss.size()) + " validation loss points", "PlotWidget");
    
    try {
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str());
            
            // Add training loss line
            ImPlot::PlotLine("Training Loss", 
                           epochs.data(), 
                           trainLoss.data(), 
                           static_cast<int>(epochs.size()));
            
            // Add validation loss line
            if (!validLoss.empty()) {
                ImPlot::PlotLine("Validation Loss", 
                               epochs.data(), 
                               validLoss.data(), 
                               static_cast<int>(epochs.size()));
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Loss curve plot rendering completed", "PlotWidget");
        } else {
            LOG_ERR("Failed to begin loss curve plot", "PlotWidget");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during loss curve plot rendering: " + std::string(e.what()), "PlotWidget");
        throw;
    }
} 