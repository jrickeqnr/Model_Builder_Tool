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
#include <matplotlibcpp.h>

namespace plt = matplotlibcpp;

#include "data/DataFrame.h"
#include "models/Model.h"
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
     * @brief Set the model type for specialized display
     * 
     * @param modelType The type of model being displayed
     */
    void setModelType(const std::string& modelType);
    
    /**
     * @brief Set hyperparameters for specialized display
     * 
     * @param hyperparams Map of hyperparameter names to values
     */
    void setHyperparameters(const std::unordered_map<std::string, std::string>& hyperparams);

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
    
    // Member functions
    void handleBackButton();
    void updateParametersDisplay();
    void updateStatisticsDisplay();
    void createPlots();
    
    // Model-specific display methods
    void updateLinearRegressionDisplay();
    void updateElasticNetDisplay();
    void updateRandomForestDisplay();
    void updateXGBoostDisplay();
    void updateGradientBoostingDisplay();
    void updateNeuralNetworkDisplay();

    // Data members
    std::shared_ptr<Model> model;
    std::shared_ptr<DataFrame> dataFrame;
    std::vector<std::string> inputVariables;
    std::string targetVariable;
    std::string modelType;
    std::unordered_map<std::string, std::string> hyperparameters;
    std::function<void()> backButtonCallback;

    // UI Elements
    Fl_Box* modelTitleLabel;
    Fl_Box* modelSubtitleLabel;
    Fl_Box* equationDisplay;
    Fl_Group* parametersGroup;
    Fl_Group* statisticsGroup;
    PlotNavigator* plotNavigator;
    Fl_Button* backButton;
    DataTable* parametersTable;
    DataTable* statisticsTable;
    Fl_Box* equationBox;
};

/**
 * @brief Simple widget for creating plots
 * 
 * This class provides functionality for creating plots using matplotlib-cpp.
 */
class PlotWidget : public Fl_Group {
public:
    enum class PlotType {
        None,
        Scatter,
        Timeseries,
        Importance,
        Residual,
        LearningCurve,
        NeuralNetworkArchitecture,
        TreeVisualization
    };

    PlotWidget(int x, int y, int w, int h);
    ~PlotWidget();

    void draw() override;
    void resize(int x, int y, int w, int h) override;

    // Direct plotting methods using matplotlibcpp
    void createScatterPlot(const std::vector<double>& actualValues,
                          const std::vector<double>& predictedValues,
                          const std::string& xLabel,
                          const std::string& yLabel,
                          const std::string& title);

    void createTimeseriesPlot(const std::vector<double>& actualValues,
                             const std::vector<double>& predictedValues,
                             const std::string& title);

    void createImportancePlot(const std::unordered_map<std::string, double>& importance,
                             const std::string& title);

    void createResidualPlot(const std::vector<double>& actualValues,
                           const std::vector<double>& predictedValues,
                           const std::string& title);

    void createLearningCurvePlot(const std::vector<double>& trainingScores,
                                const std::vector<double>& validationScores,
                                const std::vector<int>& trainingSizes,
                                const std::string& title);

    void createNeuralNetworkArchitecturePlot(const std::vector<int>& layerSizes,
                                            const std::string& title);

    void createTreeVisualizationPlot(const std::string& treeStructure,
                                    const std::string& title);

    bool savePlot(const std::string& filename);

private:
    static constexpr double RESIZE_DELAY = 0.25; // Seconds
    static void resizeTimeoutCallback(void* v);
    void regeneratePlot();

    Fl_Box* plotBox;
    char* plotImageData;
    int plotImageWidth;
    int plotImageHeight;
    PlotType currentPlotType;

    // Store data for regeneration
    std::vector<double> storedActualValues;
    std::vector<double> storedPredictedValues;
    std::string storedXLabel;
    std::string storedYLabel;
    std::string storedTitle;
    std::unordered_map<std::string, double> storedImportance;
    std::vector<double> storedTrainingScores;
    std::vector<double> storedValidationScores;
    std::vector<int> storedTrainingSizes;
    std::vector<int> storedLayerSizes;
    std::string storedTreeStructure;

    // Resize handling
    bool resizeTimerActive;
    int pendingWidth;
    int pendingHeight;
};