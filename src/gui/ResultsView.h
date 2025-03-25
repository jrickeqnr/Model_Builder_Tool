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
#include "gui/PlotGLWindow.h"  // Include the custom window header
#include "gui/ExportDialog.h"  // Include the export dialog header

/**
 * @brief View for displaying model results
 * 
 * This view displays the results of a fitted model, including statistics,
 * plots, and visualizations of the model's performance.
 */
class ResultsView : public Fl_Group {
public:
    ResultsView(int x, int y, int w, int h);
    ~ResultsView() override;
    
    /**
     * @brief Set the model to display results for
     * 
     * @param model The fitted model to display
     */
    void setModel(std::shared_ptr<Model> newModel);
    
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
    // Left panel (stats and params)
    Fl_Group* leftPanel;
    Fl_Group* statisticsPanel;
    Fl_Box* statisticsTitle;
    Fl_Text_Display* statisticsDisplay;
    Fl_Text_Buffer* statisticsBuffer;
    
    Fl_Group* parametersPanel;
    Fl_Box* parametersTitle;
    Fl_Text_Display* parametersDisplay;
    Fl_Text_Buffer* parametersBuffer;
    
    // Right panel (plots)
    Fl_Group* rightPanel;
    Fl_Group* navigationGroup;
    Fl_Button* prevButton;
    Fl_Button* nextButton;
    Fl_Box* plotTypeLabel;
    PlotGLWindow* plotsPanel;  // Changed to our custom class
    Fl_Button* exportButton;
    
    // Model data
    std::shared_ptr<Model> model;
    std::shared_ptr<DataFrame> dataFrame;
    
    // Plot state
    bool plottingInitialized;
    int currentPlotType;
    std::map<std::string, std::vector<double>> currentData;  // Added currentData member
    
    // Methods
    void createPlots();
    void updateStatisticsDisplay();
    void updateParametersDisplay();
    void cyclePlot(int direction);
    void updatePlotTypeLabel();
    void exportResults();
    void updatePlots();  // Added updatePlots declaration
    void exportPlot();   // Added exportPlot declaration
    static void exportButtonCallback(Fl_Widget* w, void* data);
};