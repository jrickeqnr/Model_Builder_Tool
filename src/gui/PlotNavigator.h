#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "imgui.h"
#include "gui/PlotWidget.h"

/**
 * @brief Class for managing multiple plots with navigation (ImGui version)
 */
class PlotNavigator {
public:
    /**
     * @brief Constructor
     */
    PlotNavigator();
    
    /**
     * @brief Destructor
     */
    ~PlotNavigator();

    /**
     * @brief Create a scatter, time series, or residual plot
     * 
     * @param plotType Type of plot to create
     * @param title Plot title
     * @param xValues X values for the plot
     * @param yValues Y values for the plot
     */
    void createPlot(PlotWidget::PlotType plotType, const std::string& title,
                  const std::vector<double>& xValues, 
                  const std::vector<double>& yValues);
    
    /**
     * @brief Create a feature importance plot
     * 
     * @param plotType Should be PlotType::IMPORTANCE
     * @param title Plot title
     * @param values Map of feature names to importance values
     */
    void createPlot(PlotWidget::PlotType plotType, const std::string& title,
                  const std::unordered_map<std::string, double>& values);
    
    /**
     * @brief Create a learning curve or loss curve plot
     * 
     * @param plotType Plot type (LEARNING_CURVE or LOSS_CURVE)
     * @param title Plot title
     * @param xValues X values (training sizes or epochs)
     * @param yValues First Y series (training scores/loss)
     * @param y2Values Second Y series (validation scores/loss)
     */
    void createPlot(PlotWidget::PlotType plotType, const std::string& title,
                  const std::vector<double>& xValues, 
                  const std::vector<double>& yValues,
                  const std::vector<double>& y2Values);

    /**
     * @brief Show a specific plot by index
     * 
     * @param index Index of the plot to show
     */
    void showPlot(size_t index);

    /**
     * @brief Navigate to the next plot
     * 
     * @return true If navigation was successful
     * @return false If at the last plot already
     */
    bool nextPlot();

    /**
     * @brief Navigate to the previous plot
     * 
     * @return true If navigation was successful
     * @return false If at the first plot already
     */
    bool previousPlot();

    /**
     * @brief Clear all plots
     */
    void clearPlots();

    /**
     * @brief Save a specific plot to a file
     * 
     * @param index Index of the plot to save
     * @param filename Path where to save the plot
     * @return bool True if successful, false otherwise
     */
    bool savePlotToFile(size_t index, const std::string& filename);
    
    /**
     * @brief Get the total number of plots
     * 
     * @return size_t Number of plots
     */
    size_t getPlotCount() const;
    
    /**
     * @brief Render the current plot using ImGui
     */
    void render();

private:
    std::vector<std::unique_ptr<PlotWidget>> plots;
    size_t currentPlotIndex;
    size_t numPlots;
    
    /**
     * @brief Update the plot count after adding or removing plots
     */
    void updatePlotCount();
}; 