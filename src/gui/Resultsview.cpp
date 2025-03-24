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
    plotsPanel = new Fl_Gl_Window(x + padding, statisticsPanel->y() + statisticsPanel->h() + padding, 
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
    
    // Get current dimensions
    int w = this->w();
    int h = this->h();
    LOG_DEBUG("ResultsView dimensions: " + std::to_string(w) + "x" + std::to_string(h), "ResultsView");
    
    // Position UI components
    LOG_DEBUG("Positioning UI components", "ResultsView");
    if (statisticsPanel) {
        statisticsPanel->resize(10, 10, w/3 - 20, h/2 - 20);
    }
    if (parametersPanel) {
        parametersPanel->resize(w/3 + 10, 10, w/3 - 20, h/2 - 20);
    }
    if (plotsPanel) {
        plotsPanel->resize(2*w/3 + 10, 10, w/3 - 20, h/2 - 20);
    }
    
    // Initialize PlottingUtility if needed
    if (!plottingInitialized && model && plotsPanel) {
        LOG_INFO("Initializing PlottingUtility", "ResultsView");
        bool initSuccess = PlottingUtility::getInstance().initialize(plotsPanel);
        if (initSuccess) {
            plottingInitialized = true;
            LOG_INFO("PlottingUtility initialized successfully", "ResultsView");
            
            // Create plots
            LOG_INFO("Creating plots for model", "ResultsView");
            createPlots();
        } else {
            LOG_ERR("Failed to initialize PlottingUtility", "ResultsView");
        }
    }
    
    LOG_INFO("ResultsView::layout() completed", "ResultsView");
}

void ResultsView::draw() {
    LOG_DEBUG("ResultsView::draw called", "ResultsView");
    
    // Call parent's draw method first
    Fl_Group::draw();
    
    // Render plots if initialized
    if (plottingInitialized) {
        PlottingUtility::getInstance().render();
    }
    
    LOG_DEBUG("ResultsView::draw completed", "ResultsView");
}

void ResultsView::render() {
    if (plottingInitialized) {
        try {
            PlottingUtility::getInstance().render();
            LOG_DEBUG("Plots rendered in render()", "ResultsView");
        } catch (const std::exception& e) {
            LOG_ERR("Error in render(): " + std::string(e.what()), "ResultsView");
        }
    }
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
    LOG_INFO("Setting new model in ResultsView", "ResultsView");
    
    if (!newModel) {
        LOG_ERR("Attempted to set null model", "ResultsView");
        return;
    }
    
    model = newModel;
    
    // Get the DataFrame from the model
    dataFrame = model->getDataFrame();
    if (!dataFrame) {
        LOG_ERR("Model does not have a DataFrame set", "ResultsView");
        return;
    }
    LOG_INFO("Successfully retrieved DataFrame from model with " + 
             std::to_string(dataFrame->getNumRows()) + " rows and " +
             std::to_string(dataFrame->columnCount()) + " columns", "ResultsView");
    
    // Update displays
    updateStatisticsDisplay();
    updateParametersDisplay();
    
    // Force a layout update to create plots
    layout();
    
    LOG_INFO("Model set successfully in ResultsView", "ResultsView");
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
    LOG_INFO("Updating statistics display", "ResultsView");
    
    if (!model || !statisticsBuffer) {
        LOG_ERR("Cannot update statistics: model or buffer is null", "ResultsView");
        return;
    }
    
    try {
        std::stringstream ss;
        auto statistics = model->getStatistics();
        
        // Format statistics nicely
        ss << "Model Type: " << model->getName() << "\n\n";
        
        for (const auto& [key, value] : statistics) {
            ss << key << ": " << std::fixed << std::setprecision(4) << value << "\n";
        }
        
        statisticsBuffer->text(ss.str().c_str());
        LOG_INFO("Statistics display updated successfully", "ResultsView");
    } catch (const std::exception& e) {
        LOG_ERR("Error updating statistics display: " + std::string(e.what()), "ResultsView");
    }
}

void ResultsView::updateParametersDisplay() {
    LOG_INFO("Updating parameters display", "ResultsView");
    
    if (!model || !parametersBuffer) {
        LOG_ERR("Cannot update parameters: model or buffer is null", "ResultsView");
        return;
    }
    
    try {
        std::stringstream ss;
        auto parameters = model->getParameters();
        
        // Format parameters nicely
        ss << "Model Parameters:\n\n";
        
        for (const auto& [key, value] : parameters) {
            ss << key << ": " << value << "\n";
        }
        
        // Add feature information
        ss << "\nFeatures:\n";
        for (const auto& feature : model->getVariableNames()) {
            ss << "- " << feature << "\n";
        }
        
        ss << "\nTarget Variable: " << model->getTargetName() << "\n";
        
        parametersBuffer->text(ss.str().c_str());
        LOG_INFO("Parameters display updated successfully", "ResultsView");
    } catch (const std::exception& e) {
        LOG_ERR("Error updating parameters display: " + std::string(e.what()), "ResultsView");
    }
}

void ResultsView::createPlots() {
    LOG_INFO("Creating plots for model", "ResultsView");
    
    if (!model || !dataFrame) {
        LOG_ERR("Cannot create plots: model or DataFrame is null", "ResultsView");
        return;
    }
    
    if (!plottingInitialized) {
        LOG_ERR("Cannot create plots: PlottingUtility not initialized", "ResultsView");
        return;
    }
    
    try {
        // Get actual and predicted values
        std::vector<double> actual = dataFrame->getColumn(model->getTargetName());
        std::vector<double> predicted;
        Eigen::VectorXd predictedEigen = model->predict(dataFrame->toMatrix(model->getVariableNames()));
        predicted.resize(predictedEigen.size());
        Eigen::VectorXd::Map(&predicted[0], predictedEigen.size()) = predictedEigen;
        
        LOG_INFO("Creating plots with " + std::to_string(actual.size()) + " data points", "ResultsView");
        
        // Create scatter plot of actual vs predicted values
        PlottingUtility::getInstance().createScatterPlot(
            actual, predicted,
            "Actual vs Predicted Values",
            "Actual Values",
            "Predicted Values"
        );
        
        // Create time series plot
        PlottingUtility::getInstance().createTimeSeriesPlot(
            actual, predicted,
            "Time Series Plot"
        );
        
        // Calculate and create residual plot
        std::vector<double> residuals;
        residuals.reserve(actual.size());
        for (size_t i = 0; i < actual.size(); ++i) {
            residuals.push_back(actual[i] - predicted[i]);
        }
        
        PlottingUtility::getInstance().createResidualPlot(
            predicted, residuals,
            "Residual Plot"
        );
        
        // Create feature importance plot if available
        auto importance = model->getFeatureImportance();
        if (!importance.empty()) {
            PlottingUtility::getInstance().createImportancePlot(
                importance,
                "Feature Importance"
            );
        }
        
        LOG_INFO("All plots created successfully", "ResultsView");
    } catch (const std::exception& e) {
        LOG_ERR("Error creating plots: " + std::string(e.what()), "ResultsView");
        throw;
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