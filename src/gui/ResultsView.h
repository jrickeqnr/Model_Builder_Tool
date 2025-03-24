#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Multiline_Output.H>
#include <map>
#include <string>
#include <memory>
#include <iomanip>
#include <sstream>
#include <functional>
#include <vector>

#include "models/Model.h"
#include "data/DataFrame.h"
#include "utils/PlottingUtility.h"

/**
 * @brief View for displaying model results
 * 
 * This view displays the results of a fitted model, including statistics,
 * plots, and visualizations of the model's performance.
 */
class ResultsView : public Fl_Group {
public:
    ResultsView(int x, int y, int w, int h);
    ~ResultsView();
    
    /**
     * @brief Set the model to display results for
     * 
     * @param model The fitted model to display
     */
    void setModel(std::shared_ptr<Model> model);
    
    /**
     * @brief Set multiple models for comparison
     * 
     * @param models Vector of models to compare
     */
    void onModelComparisonSelected(const std::vector<std::shared_ptr<Model>>& models);
    
    /**
     * @brief FLTK layout method
     */
    void layout();
    
    /**
     * @brief FLTK draw method
     */
    void draw() override;
    
    /**
     * @brief Draw a fallback display when plotting fails
     */
    void drawFallbackPlotDisplay();
    
    void render();  // Add render method declaration

private:
    // UI components - match the names used in the CPP file
    Fl_Group* statisticsPanel;
    Fl_Box* statisticsTitle;
    Fl_Text_Display* statisticsDisplay;
    Fl_Text_Buffer* statisticsBuffer;
    
    Fl_Group* parametersPanel;
    Fl_Box* parametersTitle;
    Fl_Text_Display* parametersDisplay;
    Fl_Text_Buffer* parametersBuffer;
    
    Fl_Group* plotsPanel;
    Fl_Button* exportButton;
    
    // Data
    std::shared_ptr<Model> model;
    std::shared_ptr<DataFrame> dataFrame;
    std::vector<std::shared_ptr<Model>> comparisonModels;
    
    // Plot state
    bool plottingInitialized;
    size_t currentModelIndex;
    bool showModelNavigation;
    
    // Callbacks
    static void exportButtonCallback(Fl_Widget* w, void* data);
    
    // Helper methods for creating displays
    void updateStatisticsDisplay();
    void updateParametersDisplay();
    void createPlots();
    void exportResults();
};