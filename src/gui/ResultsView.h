#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Button.H>
#include <memory>
#include <vector>
#include <string>
#include <functional>

#include "data/DataFrame.h"
#include "models/Model.h"

// Forward declaration of the plotting widget
class PlotWidget;

/**
 * @brief Widget for displaying regression results
 * 
 * This widget provides UI for displaying the results of a regression analysis,
 * including model parameters, statistics, and visualizations.
 */
class ResultsView : public Fl_Group {
public:
    ResultsView(int x, int y, int w, int h);
    ~ResultsView();

    /**
     * @brief Set the model to display results for
     * 
     * @param model Pointer to the model
     */
    void setModel(std::shared_ptr<Model> model);

    /**
     * @brief Set the data used for the model
     * 
     * @param dataFrame Pointer to the data frame
     * @param inputVariables Names of input variables
     * @param targetVariable Name of target variable
     */
    void setData(std::shared_ptr<DataFrame> dataFrame, 
                const std::vector<std::string>& inputVariables, 
                const std::string& targetVariable);

    /**
     * @brief Update the results display
     */
    void updateResults();
    
    /**
     * @brief Set the callback function for back button
     * 
     * @param callback Function to call when back button is clicked
     */
    void setBackButtonCallback(std::function<void()> callback);

private:
    std::shared_ptr<Model> model;
    std::shared_ptr<DataFrame> dataFrame;
    std::vector<std::string> inputVariables;
    std::string targetVariable;

    Fl_Box* modelTitleLabel;
    Fl_Group* parametersGroup;
    Fl_Group* statisticsGroup;
    PlotWidget* plotWidget;
    Fl_Button* backButton;
    Fl_Button* exportButton;
    
    std::function<void()> backButtonCallback;

    // Static callbacks for FLTK
    static void backButtonCallback_static(Fl_Widget* widget, void* userData);
    static void exportButtonCallback_static(Fl_Widget* widget, void* userData);
    
    /**
     * @brief Handle back button click
     */
    void handleBackButton();
    
    /**
     * @brief Handle export button click
     */
    void handleExportButton();

    /**
     * @brief Update the parameters display
     */
    void updateParametersDisplay();

    /**
     * @brief Update the statistics display
     */
    void updateStatisticsDisplay();

    /**
     * @brief Create scatter plot of actual vs predicted values
     */
    void createScatterPlot();

    /**
     * @brief Export results to a file
     */
    void exportResults();
};

/**
 * @brief Simple widget for creating plots
 * 
 * This class provides functionality for creating plots using matplotlib-cpp.
 */
class PlotWidget : public Fl_Group {
public:
    PlotWidget(int x, int y, int w, int h);
    ~PlotWidget();

    /**
     * @brief Create a scatter plot
     * 
     * @param actualValues Actual values
     * @param predictedValues Predicted values
     * @param xLabel Label for x-axis
     * @param yLabel Label for y-axis
     * @param title Title of the plot
     */
    void createScatterPlot(const std::vector<double>& actualValues,
                           const std::vector<double>& predictedValues,
                           const std::string& xLabel,
                           const std::string& yLabel,
                           const std::string& title);

protected:
    /**
     * @brief Draw the widget
     */
    void draw() override;

private:
    Fl_Box* plotBox;
    char* plotImageData;
    int plotImageWidth;
    int plotImageHeight;

    /**
     * @brief Generate a plot using matplotlib-cpp
     * 
     * @param command Python command to generate the plot
     * @return bool True if plot was generated successfully
     */
    bool generatePlot(const std::string& command);
};

/**
 * @brief Custom table to display parameters and statistics
 */
class DataTable : public Fl_Table {
public:
    DataTable(int x, int y, int w, int h, const char* label = 0);
    ~DataTable();
    
    /**
     * @brief Set the data for the table
     * 
     * @param data Map of names to values
     */
    void setData(const std::unordered_map<std::string, double>& data);
    
protected:
    /**
     * @brief Draw a cell
     */
    void draw_cell(TableContext context, int row, int col, int x, int y, int w, int h) override;
    
private:
    std::vector<std::string> names;
    std::vector<double> values;
};