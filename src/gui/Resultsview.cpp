#include <iostream>
#include "gui/ResultsView.h"
#include "gui/PlotWidget.h"
#include "gui/PlotNavigator.h"
#include "utils/Logger.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <sstream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <algorithm>  // For std::min
#include <memory>    // For std::unique_ptr

// ResultsView implementation
ResultsView::ResultsView(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h),
      exportDialog(std::make_unique<ExportDialog>(400, 300, "Export Options"))
{
    begin();
    
    // Set group properties
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
    
    // Constants for layout
    int margin = 20;
    int headerHeight = 40;
    int bottomButtonsHeight = 40;
    int equationHeight = 60;
    int subtitleHeight = 25;  // Height for the model type subtitle
    
    // Create title label
    modelTitleLabel = new Fl_Box(x + margin, y + margin, w - 2*margin, headerHeight, "Model Results");
    modelTitleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    modelTitleLabel->labelsize(18);
    modelTitleLabel->labelfont(FL_BOLD);
    
    // Create subtitle label
    modelSubtitleLabel = new Fl_Box(x + margin, y + margin + headerHeight, w - 2*margin, subtitleHeight, "");
    modelSubtitleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    modelSubtitleLabel->labelsize(14);
    modelSubtitleLabel->labelfont(FL_ITALIC);
    
    // Create equation display box (moved down by subtitleHeight)
    Fl_Box* equationLabel = new Fl_Box(x + margin, y + margin + headerHeight + subtitleHeight + 5, 
                                      w - 2*margin, equationHeight, "Regression Equation:");
    equationLabel->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    equationLabel->labelsize(14);
    equationLabel->labelfont(FL_BOLD);
    
    equationDisplay = new Fl_Box(x + margin + 20, y + margin + headerHeight + subtitleHeight + 30, 
                                w - 2*margin - 40, equationHeight - 20, "");
    equationDisplay->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    equationDisplay->labelsize(14);
    equationDisplay->box(FL_BORDER_BOX);
    
    // Create main display area (moved down by subtitleHeight)
    int contentY = y + margin + headerHeight + subtitleHeight + equationHeight + 15;
    int contentHeight = h - margin*2 - headerHeight - subtitleHeight - equationHeight - 15 - bottomButtonsHeight - 10;
    int tableWidth = (w - margin*3) / 2;
    
    // Parameters group (left side)
    parametersGroup = new Fl_Group(x + margin, contentY, tableWidth, contentHeight/2);
    parametersGroup->box(FL_BORDER_BOX);
    parametersGroup->begin();
    
    Fl_Box* parametersLabel = new Fl_Box(x + margin + 10, contentY + 10, tableWidth - 20, 30, "Model Parameters");
    parametersLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    parametersLabel->labelsize(14);
    parametersLabel->labelfont(FL_BOLD);
    
    parametersTable = new DataTable(x + margin + 10, contentY + 50, 
                                  tableWidth - 20, contentHeight/2 - 60);
    
    parametersGroup->end();
    
    // Statistics group (left side, bottom half)
    statisticsGroup = new Fl_Group(x + margin, contentY + contentHeight/2 + 10, tableWidth, contentHeight/2 - 10);
    statisticsGroup->box(FL_BORDER_BOX);
    statisticsGroup->begin();
    
    Fl_Box* statisticsLabel = new Fl_Box(x + margin + 10, contentY + contentHeight/2 + 20, tableWidth - 20, 30, "Model Statistics");
    statisticsLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    statisticsLabel->labelsize(14);
    statisticsLabel->labelfont(FL_BOLD);
    
    statisticsTable = new DataTable(x + margin + 10, contentY + contentHeight/2 + 60, 
                                  tableWidth - 20, contentHeight/2 - 70);
    
    statisticsGroup->end();
    
    // Plot navigator (right side)
    plotNavigator = new PlotNavigator(x + margin*2 + tableWidth, contentY, tableWidth, contentHeight);
    
    // Create bottom buttons
    int buttonY = y + h - margin - bottomButtonsHeight;
    
    backButton = new Fl_Button(x + margin, buttonY, 100, bottomButtonsHeight, "Back");
    backButton->callback(backButtonCallback_static, this);
    
    exportButton = new Fl_Button(x + w - margin - 150, buttonY, 150, bottomButtonsHeight, "Export Results");
    exportButton->callback(exportButtonCallback_static, this);
    
    end();

    // Initialize the export dialog
    exportDialog->onExport = [this](const ExportDialog::ExportOptions& options) {
        exportResults(options);
    };
}

ResultsView::~ResultsView() {
    // FLTK handles widget destruction through parent-child relationship
}

void ResultsView::setModel(std::shared_ptr<Model> model) {
    this->model = model;
}

void ResultsView::setData(std::shared_ptr<DataFrame> dataFrame, 
                         const std::vector<std::string>& inputVariables, 
                         const std::string& targetVariable) {
    this->dataFrame = dataFrame;
    this->inputVariables = inputVariables;
    this->targetVariable = targetVariable;
}

void ResultsView::setModelType(const std::string& modelType) {
    this->modelType = modelType;
    if (modelSubtitleLabel) {
        modelSubtitleLabel->copy_label(("Model Type: " + modelType).c_str());
    }
}

void ResultsView::setHyperparameters(const std::unordered_map<std::string, std::string>& hyperparams) {
    this->hyperparameters = hyperparams;
}

void ResultsView::updateResults() {
    if (!model || !dataFrame) {
        return;
    }
    
    // Clear existing plots
    plotNavigator->clearPlots();
    
    // Update displays based on the model type
    if (modelType == "Linear Regression") {
        updateLinearRegressionDisplay();
    } else {
        // For now, all models use the same display until they are implemented
        // This will be updated as each model is implemented
        // Replaced LOG_INFO with comment to fix build error
        updateLinearRegressionDisplay();
    }
    
    // Redraw the widget
    redraw();
}

void ResultsView::updateParametersDisplay() {
    if (!model || !parametersTable) {
        return;
    }
    
    // Get parameters
    auto parameters = model->getParameters();
    
    // Reorganize parameters to show intercept first, then coefficients in order
    std::unordered_map<std::string, double> orderedParams;
    
    // Add intercept first
    auto interceptIt = parameters.find("intercept");
    if (interceptIt != parameters.end()) {
        orderedParams["Intercept"] = interceptIt->second;
    }
    
    // Then add coefficients with variable names
    for (const auto& varName : model->getVariableNames()) {
        auto coefIt = parameters.find(varName);
        if (coefIt != parameters.end()) {
            orderedParams[varName + " (coefficient)"] = coefIt->second;
        }
    }
    
    // Update table with ordered parameter values
    parametersTable->setData(orderedParams);
}

void ResultsView::updateStatisticsDisplay() {
    if (!model || !statisticsTable) {
        return;
    }
    
    // Get statistics
    auto statistics = model->getStatistics();
    
    // Create a nicer representation of statistics with better names
    std::unordered_map<std::string, double> formattedStats;
    
    // R-squared
    auto r2 = statistics.find("r_squared");
    if (r2 != statistics.end()) {
        formattedStats["R² (coefficient of determination)"] = r2->second;
    }
    
    // Adjusted R-squared
    auto adjR2 = statistics.find("adjusted_r_squared");
    if (adjR2 != statistics.end()) {
        formattedStats["Adjusted R²"] = adjR2->second;
    }
    
    // RMSE
    auto rmse = statistics.find("rmse");
    if (rmse != statistics.end()) {
        formattedStats["RMSE (root mean squared error)"] = rmse->second;
    }
    
    // Number of samples
    auto samples = statistics.find("n_samples");
    if (samples != statistics.end()) {
        formattedStats["Number of observations"] = samples->second;
    }
    
    // Number of features
    auto features = statistics.find("n_features");
    if (features != statistics.end()) {
        formattedStats["Number of variables"] = features->second;
    }
    
    // Update table with formatted statistics
    statisticsTable->setData(formattedStats);
}

std::string ResultsView::getEquationString() const {
    if (!model) {
        return "No model available";
    }
    
    std::ostringstream equation;
    equation << std::fixed << std::setprecision(4);
    
    // Get target name
    std::string targetName = model->getTargetName();
    if (targetName.empty()) {
        targetName = "Y";
    }
    
    // Get parameters
    auto parameters = model->getParameters();
    
    // Start with the target variable
    equation << targetName << " = ";
    
    // Add intercept
    bool firstTerm = true;
    auto interceptIt = parameters.find("intercept");
    if (interceptIt != parameters.end()) {
        equation << interceptIt->second;
        firstTerm = false;
    }
    
    // Add coefficients with variable names
    for (const auto& varName : model->getVariableNames()) {
        auto coefIt = parameters.find(varName);
        if (coefIt != parameters.end()) {
            double coef = coefIt->second;
            if (coef >= 0 && !firstTerm) {
                equation << " + ";
            } else if (coef < 0) {
                equation << " - ";
                coef = -coef; // Make positive for display
            }
            
            equation << coef << " * " << varName;
            firstTerm = false;
        }
    }
    
    return equation.str();
}

void ResultsView::createPlots() {
    if (!model || !dataFrame) {
        return;
    }

    // Get actual and predicted values
    Eigen::MatrixXd X = dataFrame->toMatrix(inputVariables);
    Eigen::VectorXd y = Eigen::Map<Eigen::VectorXd>(
        dataFrame->getColumn(targetVariable).data(),
        dataFrame->getColumn(targetVariable).size()
    );
    Eigen::VectorXd predictions = model->predict(X);

    // Convert Eigen vectors to std::vector
    std::vector<double> actualValues(y.data(), y.data() + y.size());
    std::vector<double> predictedValues(predictions.data(), predictions.data() + predictions.size());

    // Clear existing plots
    plotNavigator->clearPlots();

    // Create standard plots for all models
    plotNavigator->createPlot(
        dataFrame, model,
        "scatter", 
        "Actual vs. Predicted Values"
    );

    plotNavigator->createPlot(
        dataFrame, model,
        "timeseries", 
        "Time Series Plot"
    );

    plotNavigator->createPlot(
        dataFrame, model,
        "importance", 
        "Feature Importance"
    );
    
    // Only create residual plot for non-linear regression models
    if (model->getName() != "Linear Regression") {
        plotNavigator->createPlot(
            dataFrame, model,
            "residual", 
            "Residual Plot"
        );
    }

    // Add model-specific plots
    std::string modelName = model->getName();
    
    if (modelName == "Neural Network") {
        // For neural networks, add architecture diagram
        // We would need to extract layer sizes from the model
        std::vector<int> layerSizes;
        
        // Check if we can get the layer sizes from hyperparameters
        auto params = model->getParameters();
        auto hiddenLayersIt = params.find("hidden_layer_sizes");
        
        if (hiddenLayersIt != params.end()) {
            // Parse the hidden layer sizes
            std::string hiddenLayers = std::to_string(hiddenLayersIt->second);
            std::stringstream ss(hiddenLayers);
            std::string layer;
            
            // Add input layer size (number of features)
            layerSizes.push_back(inputVariables.size());
            
            // Parse comma-separated hidden layer sizes
            while (std::getline(ss, layer, ',')) {
                try {
                    layerSizes.push_back(std::stoi(layer));
                } catch (...) {
                    // If we can't parse, just use a default
                    layerSizes.push_back(10);
                }
            }
            
            // Add output layer (always 1 for regression)
            layerSizes.push_back(1);
            
            plotNavigator->createPlot(
                dataFrame, model,
                "neural_network_architecture", 
                "Neural Network Architecture"
            );
        }
    }
    else if (modelName == "Random Forest" || modelName == "Gradient Boosting" || modelName == "XGBoost") {
        // For tree-based models, add a tree visualization (if available)
        plotNavigator->createPlot(
            dataFrame, model,
            "tree_visualization", 
            "Tree Visualization"
        );
    }
    
    // For all models except Linear Regression, add learning curves
    if (modelName != "Linear Regression") {
        plotNavigator->createPlot(
            dataFrame, model,
            "learning_curve", 
            "Learning Curves"
        );
    }
}

void ResultsView::exportResults(const ExportDialog::ExportOptions& options) {
    if (!model || !dataFrame) {
        return;
    }
    
    // For now, use the original export options until the new fields are properly implemented
    std::string exportPath = options.exportPath;
    bool exportSummary = options.modelSummary;
    bool exportCSV = options.predictedValues;
    bool exportPlots = options.scatterPlot || options.linePlot || options.importancePlot;
    
    // Original export code
    try {
        // Export model summary if selected
        if (exportSummary) {
            std::string summaryPath = exportPath + "/model_summary.txt";
            std::ofstream file(summaryPath);
            if (file.is_open()) {
                file << "Model Summary\n";
                file << "============\n\n";
                file << "Model Type: " << model->getName() << "\n\n";
                
                file << "Parameters:\n";
                for (const auto& param : model->getParameters()) {
                    file << "  " << param.first << ": " << param.second << "\n";
                }
                file << "\n";
                
                file << "Statistics:\n";
                for (const auto& stat : model->getStatistics()) {
                    file << "  " << stat.first << ": " << stat.second << "\n";
                }
                file << "\n";
                
                file << "Input Variables:\n";
                for (const auto& var : inputVariables) {
                    file << "  " << var << "\n";
                }
                file << "\n";
                
                file << "Target Variable: " << targetVariable << "\n";
                file.close();
                
                fl_message("Model summary exported to %s", summaryPath.c_str());
            } else {
                fl_alert("Error: Failed to open file for writing: %s", summaryPath.c_str());
            }
        }
        
        if (exportCSV) {
            // Generate CSV with predictions
            std::string csvPath = exportPath + "/predictions.csv";
            std::ofstream file(csvPath);
            if (file.is_open()) {
                // Write header
                file << targetVariable << ",Predicted\n";
                
                // Generate predictions
                Eigen::MatrixXd X = dataFrame->toMatrix(inputVariables);
                Eigen::VectorXd predictions = model->predict(X);
                std::vector<double> targetData = dataFrame->getColumn(targetVariable);
                
                // Write data
                for (int i = 0; i < predictions.size(); ++i) {
                    file << targetData[i] << "," << predictions(i) << "\n";
                }
                
                file.close();
                fl_message("Predictions exported to %s", csvPath.c_str());
            } else {
                fl_alert("Error: Failed to open file for writing: %s", csvPath.c_str());
            }
        }
        
        if (exportPlots) {
            // Export all plots
            for (size_t i = 0; i < plotNavigator->getPlotCount(); ++i) {
                std::string plotPath = exportPath + "/plot_" + std::to_string(i+1) + ".png";
                if (!plotNavigator->savePlotToFile(i, plotPath)) {
                    fl_alert("Error: Failed to save plot to %s", plotPath.c_str());
                }
            }
            fl_message("Plots exported to %s", exportPath.c_str());
        }
    }
    catch (const std::exception& e) {
        fl_alert("Error exporting results: %s", e.what());
    }
}

// Model-specific display methods
void ResultsView::updateLinearRegressionDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create specific plots for linear regression
    createPlots();
    
    // Additional model-specific displays
    if (model && equationDisplay) {  // Add null check for equationDisplay
        // Add equation display
        std::string equation = getEquationString();
        equationDisplay->copy_label(equation.c_str());
    }
}

void ResultsView::updateElasticNetDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create common plots
    createPlots();
    
    // Add specific plots for ElasticNet
    if (model && dataFrame) {
        // Add regularization path plot if available
        // For now, use standard plots
    }
}

void ResultsView::updateRandomForestDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for Random Forest
    if (model && dataFrame) {
        // Add feature importance plot
        auto importance = model->getFeatureImportance();
        plotNavigator->createPlot(dataFrame, model, "importance", "Feature Importance");
    }
}

void ResultsView::updateXGBoostDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for XGBoost
    if (model && dataFrame) {
        // Add feature importance plot
        auto importance = model->getFeatureImportance();
        plotNavigator->createPlot(dataFrame, model, "importance", "Feature Importance");
    }
}

void ResultsView::updateGradientBoostingDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for Gradient Boosting
    if (model && dataFrame) {
        // Add feature importance plot
        auto importance = model->getFeatureImportance();
        plotNavigator->createPlot(dataFrame, model, "importance", "Feature Importance");
    }
}

void ResultsView::updateNeuralNetworkDisplay() {
    // Update parameters and statistics displays
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create standard plots
    createPlots();
    
    // Add specific visualizations for Neural Network
    if (model && dataFrame) {
        // Add neural network architecture visualization if available
        if (hyperparameters.count("hiddenLayerSizes")) {
            std::string hiddenLayers = hyperparameters.at("hiddenLayerSizes");
            std::vector<int> layerSizes;
            
            // Parse hidden layer sizes
            std::stringstream ss(hiddenLayers);
            std::string item;
            while (std::getline(ss, item, ',')) {
                try {
                    layerSizes.push_back(std::stoi(item));
                } catch (...) {
                    // Skip invalid values
                }
            }
            
            // Add input and output layer sizes
            layerSizes.insert(layerSizes.begin(), inputVariables.size());
            layerSizes.push_back(1); // Regression has 1 output
            
            // Create architecture plot
            plotNavigator->createPlot(dataFrame, model, "nn_architecture", "Neural Network Architecture");
        }
    }
}

// Export methods for different model types
void ResultsView::exportLinearRegressionResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportElasticNetResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportRandomForestResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportXGBoostResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportGradientBoostingResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::exportNeuralNetworkResults(const ExportDialog::ExportOptions& options) {
    // Base implementation just calls the general export
    exportResults(options);
}

void ResultsView::setBackButtonCallback(std::function<void()> callback) {
    backButtonCallback = callback;
}

void ResultsView::backButtonCallback_static(Fl_Widget* widget, void* userData) {
    ResultsView* self = static_cast<ResultsView*>(userData);
    self->handleBackButton();
}

void ResultsView::exportButtonCallback_static(Fl_Widget* widget, void* userData) {
    ResultsView* self = static_cast<ResultsView*>(userData);
    self->handleExportButton();
}

void ResultsView::handleBackButton() {
    if (backButtonCallback) {
        backButtonCallback();
    }
}

void ResultsView::handleExportButton() {
    if (!model || !dataFrame) {
        fl_alert("No model or data available to export!");
        return;
    }

    exportDialog->setModel(model);
    exportDialog->show();
}