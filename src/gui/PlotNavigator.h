#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <vector>
#include <string>
#include <memory>
#include "gui/PlotWidget.h"
#include "data/DataFrame.h"
#include "models/Model.h"

/**
 * @brief Class for managing multiple plots with navigation
 */
class PlotNavigator : public Fl_Group {
public:
    PlotNavigator(int x, int y, int w, int h);
    ~PlotNavigator();

    /**
     * @brief Create a new plot
     * 
     * @param data DataFrame containing the data
     * @param model Model used for predictions
     * @param plotType Type of plot to create
     * @param title Plot title
     */
    void createPlot(const std::shared_ptr<DataFrame>& data,
                   const std::shared_ptr<Model>& model,
                   const std::string& plotType,
                   const std::string& title);

    /**
     * @brief Navigate to the next plot
     */
    void nextPlot();

    /**
     * @brief Navigate to the previous plot
     */
    void prevPlot();

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
    size_t getPlotCount() const { return plots.size(); }

private:
    std::vector<PlotWidget*> plots;
    size_t currentPlotIndex;
    Fl_Button* prevButton;
    Fl_Button* nextButton;
    Fl_Box* plotLabel;

    static void prevButtonCallback(Fl_Widget* w, void* v);
    static void nextButtonCallback(Fl_Widget* w, void* v);

    void updateVisibility();
    void updateNavigationButtons();
}; 