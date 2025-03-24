#include "gui/ResultsView.h"
#include "utils/Logger.h"
#include "utils/PlottingUtility.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/fl_message.H>
#include <sstream>
#include <iomanip>
#include <algorithm>

// ResultsView implementation
ResultsView::ResultsView(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h)
{
    LOG_INFO("Constructing ResultsView", "ResultsView");
    
    // Set up the main layout
    begin();
    
    int padding = 10;
    int topHeight = 150;  // Height for statistics and parameters
    int bottomHeight = h - topHeight - 3 * padding;
    
    LOG_DEBUG("Creating statistics panel", "ResultsView");
    // Statistics panel
    statisticsPanel = new Fl_Group(x + padding, y + padding, w / 2 - padding * 1.5, topHeight);
    statisticsPanel->box(FL_DOWN_BOX);
    statisticsPanel->color(FL_WHITE);
    statisticsPanel->begin();
    
    // Statistics title
    statisticsTitle = new Fl_Box(padding, padding, statisticsPanel->w() - 2 * padding, 25, "Model Statistics");
    statisticsTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    statisticsTitle->labelfont(FL_BOLD);
    
    // Statistics display
    statisticsDisplay = new Fl_Text_Display(padding, statisticsTitle->y() + statisticsTitle->h() + padding,
                                         statisticsPanel->w() - 2 * padding, statisticsPanel->h() - statisticsTitle->h() - 3 * padding);
    statisticsBuffer = new Fl_Text_Buffer();
    statisticsDisplay->buffer(statisticsBuffer);
    
    statisticsPanel->end();
    
    LOG_DEBUG("Creating parameters panel", "ResultsView");
    // Parameters panel
    parametersPanel = new Fl_Group(statisticsPanel->x() + statisticsPanel->w() + padding, y + padding, 
                                 w / 2 - padding * 1.5, topHeight);
    parametersPanel->box(FL_DOWN_BOX);
    parametersPanel->color(FL_WHITE);
    parametersPanel->begin();
    
    // Parameters title
    parametersTitle = new Fl_Box(padding, padding, parametersPanel->w() - 2 * padding, 25, "Model Parameters");
    parametersTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    parametersTitle->labelfont(FL_BOLD);
    
    // Parameters display
    parametersDisplay = new Fl_Text_Display(padding, parametersTitle->y() + parametersTitle->h() + padding,
                                          parametersPanel->w() - 2 * padding, parametersPanel->h() - parametersTitle->h() - 3 * padding);
    parametersBuffer = new Fl_Text_Buffer();
    parametersDisplay->buffer(parametersBuffer);
    
    parametersPanel->end();
    
    LOG_DEBUG("Creating plots panel", "ResultsView");
    // Plots panel
    plotsPanel = new Fl_Group(x + padding, statisticsPanel->y() + statisticsPanel->h() + padding, 
                            w - 2 * padding, bottomHeight);
    plotsPanel->box(FL_DOWN_BOX);
    plotsPanel->color(FL_WHITE);
    plotsPanel->begin();
    
    // Create the export button
    exportButton = new Fl_Button(plotsPanel->w() - 110, plotsPanel->h() - 40,
                               100, 30, "Export Results");
    exportButton->callback(exportButtonCallback, this);
    
    plotsPanel->end();
    
    end();
    
    // Set resize behavior
    resizable(plotsPanel);
    
    // Initialize member variables
    model = nullptr;
    dataFrame = nullptr;
    plottingInitialized = false;
    currentModelIndex = 0;
    showModelNavigation = false;
    
    LOG_INFO("ResultsView construction complete, size: " + 
             std::to_string(w) + "x" + std::to_string(h), "ResultsView");
}

ResultsView::~ResultsView() {
    LOG_INFO("Destroying ResultsView", "ResultsView");
    
    delete statisticsBuffer;
    delete parametersBuffer;
    
    // Clean up PlottingUtility if initialized
    if (plottingInitialized) {
        try {
            LOG_DEBUG("Cleaning up PlottingUtility", "ResultsView");
            PlottingUtility::getInstance().cleanup();
            LOG_DEBUG("PlottingUtility cleanup successful", "ResultsView");
        } catch (const std::exception& e) {
            LOG_ERR("Error cleaning up PlottingUtility: " + std::string(e.what()), "ResultsView");
        } catch (...) {
            LOG_ERR("Unknown error cleaning up PlottingUtility", "ResultsView");
        }
    }
    
    LOG_INFO("ResultsView destroyed", "ResultsView");
}

void ResultsView::layout() {
    LOG_INFO("ResultsView::layout() called", "ResultsView");
    
    // Do not call the base layout() method since it doesn't exist in Fl_Group
    
    // Get dimensions for layout
    int padding = 10;
    int w = this->w();
    int h = this->h();
    LOG_DEBUG("ResultsView dimensions: " + std::to_string(w) + "x" + std::to_string(h), "ResultsView");
    
    try {
        // Initialize PlottingUtility if not already done
        if (!plottingInitialized && plotsPanel) {
            LOG_INFO("Initializing PlottingUtility", "ResultsView");
            try {
                PlottingUtility::getInstance().initialize(plotsPanel);
                plottingInitialized = true;
                LOG_INFO("PlottingUtility initialized successfully", "ResultsView");
            } catch (const std::exception& e) {
                LOG_ERR("Failed to initialize PlottingUtility: " + std::string(e.what()), "ResultsView");
                drawFallbackPlotDisplay();
                return;
            } catch (...) {
                LOG_ERR("Unknown error initializing PlottingUtility", "ResultsView");
                drawFallbackPlotDisplay();
                return;
            }
        }
        
        // Position UI components
        LOG_DEBUG("Positioning UI components", "ResultsView");
        
        // Calculate panel sizes
        int topHeight = 150;
        int bottomHeight = h - topHeight - 3 * padding;
        
        // Position statistics and parameters panels
        if (statisticsPanel && parametersPanel) {
            statisticsPanel->resize(x() + padding, y() + padding, w / 2 - padding * 1.5, topHeight);
            parametersPanel->resize(statisticsPanel->x() + statisticsPanel->w() + padding, y() + padding, 
                                  w / 2 - padding * 1.5, topHeight);
        }
        
        // Position plots panel
        if (plotsPanel) {
            plotsPanel->resize(x() + padding, y() + topHeight + 2 * padding, 
                             w - 2 * padding, bottomHeight);
            
            // Position the export button in the bottom right of the plots panel
            if (exportButton) {
                exportButton->resize(plotsPanel->x() + plotsPanel->w() - 110, 
                                   plotsPanel->y() + plotsPanel->h() - 40,
                                   100, 30);
            }
        }
        
        // Create plots if model is set
        if (model) {
            LOG_DEBUG("Model is set, creating plots", "ResultsView");
            try {
                createPlots();
                LOG_INFO("Plots created successfully", "ResultsView");
            } catch (const std::exception& e) {
                LOG_ERR("Failed to create plots: " + std::string(e.what()), "ResultsView");
                drawFallbackPlotDisplay();
            } catch (...) {
                LOG_ERR("Unknown error creating plots", "ResultsView");
                drawFallbackPlotDisplay();
            }
        } else {
            LOG_ERR("No model available for plotting", "ResultsView");
            drawFallbackPlotDisplay();
        }
    } catch (const std::exception& e) {
        LOG_ERR("Exception in ResultsView::layout(): " + std::string(e.what()), "ResultsView");
        drawFallbackPlotDisplay();
    } catch (...) {
        LOG_ERR("Unknown exception in ResultsView::layout()", "ResultsView");
        drawFallbackPlotDisplay();
    }
    
    LOG_INFO("ResultsView::layout() completed", "ResultsView");
}

void ResultsView::draw() {
    LOG_DEBUG("ResultsView::draw called", "ResultsView");
    
    // Draw the regular Fl_Group elements first
    Fl_Group::draw();
    
    // No need to render plots here, as they are drawn by the PlottingUtility
    // during the render() calls in createPlots.
    
    LOG_DEBUG("ResultsView::draw completed", "ResultsView");
}

// Helper method to draw a simple fallback when plotting fails
void ResultsView::drawFallbackPlotDisplay() {
    LOG_INFO("Drawing fallback plot display", "ResultsView");
    
    if (!plotsPanel) {
        LOG_ERR("No plot area available for fallback display", "ResultsView");
        return;
    }
    
    // Save current drawing color
    Fl_Color oldColor = fl_color();
    
    // Draw a background for the error message
    fl_color(FL_BACKGROUND_COLOR);
    fl_rectf(plotsPanel->x(), plotsPanel->y(), plotsPanel->w(), plotsPanel->h());
    
    // Draw a border
    fl_color(FL_DARK3);
    fl_rect(plotsPanel->x(), plotsPanel->y(), plotsPanel->w(), plotsPanel->h());
    
    // Draw the error message
    fl_color(FL_RED);
    fl_font(FL_HELVETICA_BOLD, 14);
    const char* message = "Error displaying plots";
    fl_draw(message, 
            plotsPanel->x() + plotsPanel->w()/2 - fl_width(message)/2, 
            plotsPanel->y() + plotsPanel->h()/2);
    
    fl_font(FL_HELVETICA, 12);
    const char* submessage = "See log for details";
    fl_draw(submessage, 
            plotsPanel->x() + plotsPanel->w()/2 - fl_width(submessage)/2, 
            plotsPanel->y() + plotsPanel->h()/2 + 20);
    
    // Restore previous color
    fl_color(oldColor);
    
    LOG_DEBUG("Fallback plot display drawn", "ResultsView");
}

void ResultsView::setModel(std::shared_ptr<Model> newModel) {
    if (!newModel) {
        LOG_WARN("Attempted to set null model", "ResultsView");
        return;
    }
    
    LOG_INFO("Setting model: " + newModel->getName(), "ResultsView");
    model = newModel;
    
    // Get DataFrame from model
    if (model) {
        dataFrame = model->getDataFrame();
        if (dataFrame) {
            LOG_INFO("Got DataFrame with " + std::to_string(dataFrame->getNumRows()) + 
                     " rows and " + std::to_string(dataFrame->columnCount()) + " columns", "ResultsView");
        } else {
            LOG_WARN("Model has no associated DataFrame", "ResultsView");
        }
        
        // Update displays
        LOG_DEBUG("Updating statistics display", "ResultsView");
        updateStatisticsDisplay();
        
        LOG_DEBUG("Updating parameters display", "ResultsView");
        updateParametersDisplay();
        
        // Create plots for the model
        LOG_DEBUG("Creating plots", "ResultsView");
        try {
            createPlots();
            LOG_INFO("Plots created successfully", "ResultsView");
        } catch (const std::exception& e) {
            LOG_ERR("Error creating plots: " + std::string(e.what()), "ResultsView");
            drawFallbackPlotDisplay();
        } catch (...) {
            LOG_ERR("Unknown error creating plots", "ResultsView");
            drawFallbackPlotDisplay();
        }
        
        LOG_INFO("Model set successfully", "ResultsView");
    }
}

void ResultsView::onModelComparisonSelected(const std::vector<std::shared_ptr<Model>>& models) {
    if (models.empty()) {
        LOG_WARN("No models provided for comparison", "ResultsView");
        return;
    }
    
    LOG_INFO("Model comparison selected with " + 
             std::to_string(models.size()) + " models", "ResultsView");
    
    // Store the models for comparison
    comparisonModels = models;
    
    // Set the current model to the first one
    currentModelIndex = 0;
    showModelNavigation = (models.size() > 1);
    
    // Set the active model
    setModel(models[0]);
}

void ResultsView::updateStatisticsDisplay() {
    if (!model) {
        LOG_WARN("Cannot update statistics display: no model", "ResultsView");
        return;
    }
    
    if (!statisticsBuffer) {
        LOG_ERR("Cannot update statistics display: no buffer", "ResultsView");
        return;
    }
    
    LOG_DEBUG("Updating statistics display for model: " + model->getName(), "ResultsView");
    
    std::stringstream ss;
    
    // Get statistics from the model
    auto statistics = model->getStatistics();
    
    // Format the statistics with labels
    ss << "Model Type: " << model->getName() << "\n\n";
    
    // Add metrics based on what's available
    if (statistics.count("r_squared")) {
        ss << "R² Score: " << std::fixed << std::setprecision(4) << statistics["r_squared"] << "\n";
    }
    
    if (statistics.count("adjusted_r_squared")) {
        ss << "Adjusted R²: " << std::fixed << std::setprecision(4) << statistics["adjusted_r_squared"] << "\n";
    }
    
    if (statistics.count("rmse")) {
        ss << "Root Mean Squared Error (RMSE): " << std::fixed << std::setprecision(4) << statistics["rmse"] << "\n";
    }
    
    if (statistics.count("mae")) {
        ss << "Mean Absolute Error (MAE): " << std::fixed << std::setprecision(4) << statistics["mae"] << "\n";
    }
    
    if (statistics.count("n_samples")) {
        ss << "Number of Samples: " << static_cast<int>(statistics["n_samples"]) << "\n";
    }
    
    if (statistics.count("n_features")) {
        ss << "Number of Features: " << static_cast<int>(statistics["n_features"]) << "\n";
    }
    
    // Set the text
    statisticsBuffer->text(ss.str().c_str());
    
    LOG_DEBUG("Statistics display updated", "ResultsView");
}

void ResultsView::updateParametersDisplay() {
    if (!model) {
        LOG_WARN("Cannot update parameters display: no model", "ResultsView");
        return;
    }
    
    if (!parametersBuffer) {
        LOG_ERR("Cannot update parameters display: no buffer", "ResultsView");
        return;
    }
    
    LOG_DEBUG("Updating parameters display for model: " + model->getName(), "ResultsView");
    
    std::stringstream ss;
    
    // Get parameters from the model
    auto params = model->getParameters();
    
    ss << "Model Parameters:\n\n";
    
    // Special handling for linear regression - using model's type name instead of enum value
    if (model->getName() == "Linear Regression") {
        // Check if we have an intercept in the parameters
        double intercept = 0.0;
        if (params.count("intercept")) {
            intercept = params["intercept"];
        }
        
        // Add intercept
        ss << "Intercept: " << std::fixed << std::setprecision(4) << intercept << "\n\n";
        
        // Add coefficients
        ss << "Coefficients:\n";
        
        // Get variable names from model
        auto variableNames = model->getVariableNames();
        
        // Match variable names to parameter values
        for (const auto& varName : variableNames) {
            if (params.count(varName)) {
                ss << varName << ": " << std::fixed << std::setprecision(4) << params[varName] << "\n";
            }
        }
    } else {
        // For other model types, just show parameters in name:value format
        for (const auto& param : params) {
            ss << param.first << ": " << std::fixed << std::setprecision(4) << param.second << "\n";
        }
    }
    
    // Set the text
    parametersBuffer->text(ss.str().c_str());
    
    LOG_DEBUG("Parameters display updated", "ResultsView");
}

void ResultsView::createPlots() {
    LOG_INFO("Creating plots for model: " + (model ? model->getName() : "null"), "ResultsView");
    
    if (!model) {
        LOG_ERR("No model available for plotting", "ResultsView");
        return;
    }
    
    if (!plottingInitialized) {
        LOG_ERR("PlottingUtility not initialized", "ResultsView");
        return;
    }
    
    // Get model data for plotting
    LOG_DEBUG("Getting model data for plots", "ResultsView");
    
    try {
        // Get actual and predicted values
        auto statistics = model->getStatistics();
        auto parameters = model->getParameters();
        
        // Need to get the data differently for each model
        if (!dataFrame) {
            LOG_ERR("No DataFrame available for plotting", "ResultsView");
            return;
        }
        
        // Get target values (actual)
        std::vector<double> actual;
        std::string targetColumn = model->getTargetName();
        
        if (dataFrame->hasColumn(targetColumn)) {
            actual = dataFrame->getColumn(targetColumn);
            LOG_DEBUG("Retrieved " + std::to_string(actual.size()) + " actual values", "ResultsView");
        } else {
            LOG_ERR("Target column not found in DataFrame", "ResultsView");
            return;
        }
        
        // Get predictions by letting the model predict on its training data
        auto featureNames = model->getVariableNames();
        Eigen::MatrixXd X = dataFrame->toMatrix(featureNames);
        Eigen::VectorXd predictions = model->predict(X);
        
        std::vector<double> predicted(predictions.data(), predictions.data() + predictions.size());
        LOG_DEBUG("Retrieved " + std::to_string(predicted.size()) + " predicted values", "ResultsView");
        
        if (actual.empty() || predicted.empty()) {
            LOG_ERR("Empty actual or predicted values", "ResultsView");
            return;
        }
        
        // Ensure equal sizes (in case of any mismatch)
        size_t minSize = std::min(actual.size(), predicted.size());
        if (actual.size() != predicted.size()) {
            LOG_WARN("Size mismatch between actual and predicted values, truncating to " + 
                    std::to_string(minSize), "ResultsView");
            actual.resize(minSize);
            predicted.resize(minSize);
        }
        
        // Create scatter plot for actual vs predicted
        LOG_INFO("Creating scatter plot", "ResultsView");
        try {
            PlottingUtility::getInstance().createScatterPlot(
                actual, 
                predicted,
                "Actual vs Predicted Values",
                "Actual", 
                "Predicted"
            );
            LOG_INFO("Rendering scatter plot", "ResultsView");
            PlottingUtility::getInstance().render();
        } catch (const std::exception& e) {
            LOG_ERR("Failed to create or render scatter plot: " + std::string(e.what()), "ResultsView");
        } catch (...) {
            LOG_ERR("Unknown error creating or rendering scatter plot", "ResultsView");
        }
        
        // Only create time series plot if we have enough data points
        if (actual.size() > 2) {
            LOG_INFO("Creating time series plot", "ResultsView");
            try {
                PlottingUtility::getInstance().createTimeSeriesPlot(
                    actual,
                    predicted,
                    "Actual vs Predicted Over Time"
                );
                LOG_INFO("Rendering time series plot", "ResultsView");
                PlottingUtility::getInstance().render();
            } catch (const std::exception& e) {
                LOG_ERR("Failed to create or render time series plot: " + std::string(e.what()), "ResultsView");
            } catch (...) {
                LOG_ERR("Unknown error creating or rendering time series plot", "ResultsView");
            }
        } else {
            LOG_WARN("Not enough data points for time series plot", "ResultsView");
        }
        
        // Calculate residuals
        LOG_DEBUG("Calculating residuals", "ResultsView");
        std::vector<double> residuals;
        for (size_t i = 0; i < actual.size() && i < predicted.size(); ++i) {
            residuals.push_back(actual[i] - predicted[i]);
        }
        
        if (!residuals.empty()) {
            LOG_INFO("Creating residual plot", "ResultsView");
            try {
                PlottingUtility::getInstance().createResidualPlot(
                    predicted,
                    residuals,
                    "Residual Plot"
                );
                LOG_INFO("Rendering residual plot", "ResultsView");
                PlottingUtility::getInstance().render();
            } catch (const std::exception& e) {
                LOG_ERR("Failed to create or render residual plot: " + std::string(e.what()), "ResultsView");
            } catch (...) {
                LOG_ERR("Unknown error creating or rendering residual plot", "ResultsView");
            }
        } else {
            LOG_WARN("No residuals available for plotting", "ResultsView");
        }
        
        // Create feature importance plot if supported
        if (model->supportsFeatureImportance()) {
            try {
                auto importance = model->getFeatureImportance();
                if (!importance.empty()) {
                    LOG_INFO("Creating feature importance plot", "ResultsView");
                    PlottingUtility::getInstance().createImportancePlot(
                        importance,
                        "Feature Importance"
                    );
                    LOG_INFO("Rendering feature importance plot", "ResultsView");
                    PlottingUtility::getInstance().render();
                }
            } catch (const std::exception& e) {
                LOG_ERR("Failed to create feature importance plot: " + std::string(e.what()), "ResultsView");
            } catch (...) {
                LOG_ERR("Unknown error creating feature importance plot", "ResultsView");
            }
        }
        
        LOG_INFO("All plots created successfully", "ResultsView");
    } catch (const std::exception& e) {
        LOG_ERR("Exception in createPlots: " + std::string(e.what()), "ResultsView");
        throw; // Rethrow to allow calling function to handle
    } catch (...) {
        LOG_ERR("Unknown exception in createPlots", "ResultsView");
        throw; // Rethrow to allow calling function to handle
    }
}

void ResultsView::exportResults() {
    LOG_INFO("Exporting results", "ResultsView");
    // Implementation for exporting results can be added here
    // For now, just show a message
    fl_message("Export functionality not implemented yet");
}

void ResultsView::exportButtonCallback(Fl_Widget* w, void* data) {
    ResultsView* view = static_cast<ResultsView*>(data);
    if (view) {
        LOG_INFO("Export button clicked", "ResultsView");
        view->exportResults();
    }
}