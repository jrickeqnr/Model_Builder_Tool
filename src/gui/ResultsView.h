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
#include <chrono>

#include "data/DataFrame.h"
#include "models/Model.h"
#include "gui/ExportDialog.h"
#include "gui/DataTable.h"
#include "gui/PlotNavigator.h"
#include "gui/PlotWidget.h"
#include "utils/PlottingUtility.h"

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