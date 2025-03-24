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
#include "utils/PlottingUtility.h"

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
    static void exportButtonCallback_static(Fl_Widget* widget, void* userData);
    
    // Member functions
    void handleBackButton();
    void handleExportButton();
    void updateParametersDisplay();
    void updateStatisticsDisplay();
    void createPlots();
    void exportResults(const ExportDialog::ExportOptions& options);
    
    // Model-specific display methods
    void updateLinearRegressionDisplay();
    void updateElasticNetDisplay();
    void updateRandomForestDisplay();
    void updateXGBoostDisplay();
    void updateGradientBoostingDisplay();
    void updateNeuralNetworkDisplay();
    
    // Export methods for different model types
    void exportLinearRegressionResults(const ExportDialog::ExportOptions& options);
    void exportElasticNetResults(const ExportDialog::ExportOptions& options);
    void exportRandomForestResults(const ExportDialog::ExportOptions& options);
    void exportXGBoostResults(const ExportDialog::ExportOptions& options);
    void exportGradientBoostingResults(const ExportDialog::ExportOptions& options);
    void exportNeuralNetworkResults(const ExportDialog::ExportOptions& options);

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
    Fl_Button* exportButton;
    DataTable* parametersTable;
    DataTable* statisticsTable;
    std::unique_ptr<ExportDialog> exportDialog;
    Fl_Box* equationBox;
};

/**
 * @brief Simple widget for creating plots
 * 
 * This class provides functionality for creating plots using matplotlib-cpp.
 */
class PlotWidget : public Fl_Group, public PlottingUtility {
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
     * @brief Create a residual plot
     * 
     * @param actualValues Actual values
     * @param predictedValues Predicted values
     * @param title Title of the plot
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createResidualPlot(const std::vector<double>& actualValues,
                           const std::vector<double>& predictedValues,
                           const std::string& title,
                           const std::string& tempDataPath = "temp_plot_data.csv",
                           const std::string& tempImagePath = "temp_plot_image.png",
                           const std::string& tempScriptPath = "temp_plot_script.py");
    
    /**
     * @brief Create a learning curve plot 
     * 
     * @param trainingScores Vector of training scores at different sizes
     * @param validationScores Vector of validation scores at different sizes
     * @param trainingSizes Vector of training set sizes
     * @param title Title of the plot
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createLearningCurvePlot(const std::vector<double>& trainingScores,
                                const std::vector<double>& validationScores,
                                const std::vector<int>& trainingSizes,
                                const std::string& title,
                                const std::string& tempDataPath = "temp_plot_data.csv",
                                const std::string& tempImagePath = "temp_plot_image.png",
                                const std::string& tempScriptPath = "temp_plot_script.py");
    
    /**
     * @brief Create a neural network architecture visualization
     * 
     * @param layerSizes Vector of layer sizes (including input and output layers)
     * @param title Title of the plot
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createNeuralNetworkArchitecturePlot(const std::vector<int>& layerSizes,
                                            const std::string& title,
                                            const std::string& tempDataPath = "temp_plot_data.csv",
                                            const std::string& tempImagePath = "temp_plot_image.png",
                                            const std::string& tempScriptPath = "temp_plot_script.py");
    
    /**
     * @brief Create a tree visualization for tree-based models
     * 
     * @param treeStructure String representation of tree structure
     * @param title Title of the plot
     * @param tempDataPath Path for temporary data file
     * @param tempImagePath Path for temporary image file
     * @param tempScriptPath Path for temporary script file
     */
    void createTreeVisualizationPlot(const std::string& treeStructure,
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
    bool executePythonScript(const std::string& scriptPath, const std::string& tempDataPath, const std::string& tempImagePath);

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

    // Storage for plot image
    Fl_Box* plotBox;
    char* plotImageData;
    int plotImageWidth;
    int plotImageHeight;
    
    // Resize handling
    bool resizeTimerActive;
    int pendingWidth;
    int pendingHeight;
    static const double RESIZE_DELAY;  // Delay in seconds before regenerating plot after resize

    // Current plot type
    PlotType currentPlotType;

    // Stored data for regeneration
    std::vector<double> storedActualValues;
    std::vector<double> storedPredictedValues;
    std::string storedXLabel;
    std::string storedYLabel;
    std::string storedTitle;
    std::unordered_map<std::string, double> storedImportance;
    
    // For learning curves
    std::vector<double> storedTrainingScores;
    std::vector<double> storedValidationScores;
    std::vector<int> storedTrainingSizes;
    
    // For neural network architecture
    std::vector<int> storedLayerSizes;
    
    // For tree visualization
    std::string storedTreeStructure;

    std::unordered_map<std::string, std::string> tempFilePaths;  // Store full paths of temporary files
};