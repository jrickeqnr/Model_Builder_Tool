#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Button.H>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
#include <unordered_map>

#include "data/DataFrame.h"
#include "models/Model.h"
#include "gui/ExportDialog.h"
#include "gui/DataTable.h"

// Forward declarations
class PlotWidget;

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

    /**
     * @brief Get a formatted equation string representing the model
     * 
     * @return std::string Formatted equation string
     */
    std::string getEquationString() const;

private:
    // Static callbacks for FLTK
    static void backButtonCallback_static(Fl_Widget* widget, void* userData);
    static void exportButtonCallback_static(Fl_Widget* widget, void* userData);
    
    // Member functions
    void handleBackButton();
    void handleExportButton();
    void updateParametersDisplay();
    void updateStatisticsDisplay();
    void createPlots();
    void exportResults(const ExportDialog::ExportOptions& options);

    // Data members
    std::shared_ptr<Model> model;
    std::shared_ptr<DataFrame> dataFrame;
    std::vector<std::string> inputVariables;
    std::string targetVariable;
    std::function<void()> backButtonCallback;

    // UI Elements
    Fl_Box* modelTitleLabel;
    Fl_Box* equationDisplay;
    Fl_Group* parametersGroup;
    Fl_Group* statisticsGroup;
    PlotNavigator* plotNavigator;
    Fl_Button* backButton;
    Fl_Button* exportButton;
    DataTable* parametersTable;
    DataTable* statisticsTable;
    std::unique_ptr<ExportDialog> exportDialog;
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
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createScatterPlot(const std::vector<double>& actualValues,
                          const std::vector<double>& predictedValues,
                          const std::string& xLabel,
                          const std::string& yLabel,
                          const std::string& title,
                          const std::string& tempDataPath = "temp_plot_data.csv",
                          const std::string& tempImagePath = "temp_plot_image.png",
                          const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a time series plot
     * 
     * @param actualValues Actual values
     * @param predictedValues Predicted values
     * @param title Title of the plot
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createTimeseriesPlot(const std::vector<double>& actualValues,
                             const std::vector<double>& predictedValues,
                             const std::string& title,
                             const std::string& tempDataPath = "temp_plot_data.csv",
                             const std::string& tempImagePath = "temp_plot_image.png",
                             const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a feature importance plot
     * 
     * @param importance Map of feature names to importance scores
     * @param title Title of the plot
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createImportancePlot(const std::unordered_map<std::string, double>& importance,
                             const std::string& title,
                             const std::string& tempDataPath = "temp_plot_data.csv",
                             const std::string& tempImagePath = "temp_plot_image.png",
                             const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Save the current plot to a file
     * 
     * @param filename Path where to save the plot
     * @return bool True if successful, false otherwise
     */
    bool savePlot(const std::string& filename);

protected:
    /**
     * @brief Draw the widget
     */
    void draw() override;

    /**
     * @brief Handle resize events
     */
    void resize(int x, int y, int w, int h) override;

private:
    static void resizeTimeoutCallback(void* v);
    void regeneratePlot();
    bool createTempDataFile(const std::string& data, const std::string& filename);

    Fl_Box* plotBox;
    char* plotImageData;
    int plotImageWidth;
    int plotImageHeight;
    bool resizeTimerActive;
    int pendingWidth;
    int pendingHeight;
    static constexpr double RESIZE_DELAY = 0.2; // seconds

    // Store plot data for regeneration on resize
    enum class PlotType {
        None,
        Scatter,
        Timeseries,
        Importance
    };
    PlotType currentPlotType;
    std::vector<double> storedActualValues;
    std::vector<double> storedPredictedValues;
    std::string storedXLabel;
    std::string storedYLabel;
    std::string storedTitle;
    std::unordered_map<std::string, double> storedImportance;
};