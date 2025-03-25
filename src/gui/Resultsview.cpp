#include "gui/ResultsView.h"
#include "utils/Logger.h"
#include "../utils/PlottingUtility.h"
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
    
    // Create a box for the entire view
    box(FL_DOWN_BOX);
    color(FL_BACKGROUND_COLOR);
    
    // Create a group for the left side (stats and params)
    leftPanel = new Fl_Group(x + 10, y + 10, w/2 - 15, h - 20);
    leftPanel->box(FL_DOWN_BOX);
    leftPanel->color(FL_BACKGROUND_COLOR);
    leftPanel->begin();
    
    // Statistics panel
    statisticsPanel = new Fl_Group(leftPanel->x() + 10, leftPanel->y() + 10, 
                                  leftPanel->w() - 20, leftPanel->h()/2 - 15);
    statisticsPanel->box(FL_DOWN_BOX);
    statisticsPanel->color(FL_BACKGROUND_COLOR);
    statisticsPanel->begin();
    
    // Statistics title
    statisticsTitle = new Fl_Box(statisticsPanel->x() + 10, statisticsPanel->y() + 10,
                                statisticsPanel->w() - 20, 25, "Model Statistics");
    statisticsTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    statisticsTitle->labelfont(FL_BOLD);
    
    // Statistics display
    statisticsDisplay = new Fl_Text_Display(statisticsPanel->x() + 10, 
                                          statisticsTitle->y() + statisticsTitle->h() + 10,
                                          statisticsPanel->w() - 20, 
                                          statisticsPanel->h() - statisticsTitle->h() - 30);
    statisticsBuffer = new Fl_Text_Buffer();
    statisticsDisplay->buffer(statisticsBuffer);
    
    statisticsPanel->end();
    
    // Parameters panel
    parametersPanel = new Fl_Group(leftPanel->x() + 10, 
                                  statisticsPanel->y() + statisticsPanel->h() + 10,
                                  leftPanel->w() - 20, 
                                  leftPanel->h()/2 - 15);
    parametersPanel->box(FL_DOWN_BOX);
    parametersPanel->color(FL_BACKGROUND_COLOR);
    parametersPanel->begin();
    
    // Parameters title
    parametersTitle = new Fl_Box(parametersPanel->x() + 10, parametersPanel->y() + 10,
                                parametersPanel->w() - 20, 25, "Model Parameters");
    parametersTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    parametersTitle->labelfont(FL_BOLD);
    
    // Parameters display
    parametersDisplay = new Fl_Text_Display(parametersPanel->x() + 10,
                                          parametersTitle->y() + parametersTitle->h() + 10,
                                          parametersPanel->w() - 20,
                                          parametersPanel->h() - parametersTitle->h() - 30);
    parametersBuffer = new Fl_Text_Buffer();
    parametersDisplay->buffer(parametersBuffer);
    
    parametersPanel->end();
    
    leftPanel->end();
    
    // Create a group for the right side (plots)
    rightPanel = new Fl_Group(x + w/2 + 5, y + 10, w/2 - 15, h - 20);
    rightPanel->box(FL_DOWN_BOX);
    rightPanel->color(FL_BACKGROUND_COLOR);
    rightPanel->begin();
    
    // Create navigation buttons group
    navigationGroup = new Fl_Group(rightPanel->x() + 10, rightPanel->y() + 10,
                                  rightPanel->w() - 20, 40);
    navigationGroup->box(FL_FLAT_BOX);
    navigationGroup->color(FL_BACKGROUND_COLOR);
    navigationGroup->begin();
    
    // Previous button
    prevButton = new Fl_Button(navigationGroup->x(), navigationGroup->y(),
                              100, 30, "Previous");
    prevButton->callback([](Fl_Widget* w, void* v) {
        auto* view = static_cast<ResultsView*>(v);
        if (view) view->cyclePlot(-1);
    }, this);
    
    // Next button
    nextButton = new Fl_Button(prevButton->x() + prevButton->w() + 10,
                              navigationGroup->y(),
                              100, 30, "Next");
    nextButton->callback([](Fl_Widget* w, void* v) {
        auto* view = static_cast<ResultsView*>(v);
        if (view) view->cyclePlot(1);
    }, this);
    
    // Plot type label
    plotTypeLabel = new Fl_Box(nextButton->x() + nextButton->w() + 10,
                              navigationGroup->y(),
                              navigationGroup->w() - (nextButton->x() + nextButton->w() + 20),
                              30, "Plot Type");
    plotTypeLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    navigationGroup->end();
    
    // Create plots panel as an OpenGL window
    plotsPanel = new PlotGLWindow(rightPanel->x() + 10,
                                 navigationGroup->y() + navigationGroup->h() + 10,
                                 rightPanel->w() - 20,
                                 rightPanel->h() - navigationGroup->h() - 60);
    plotsPanel->box(FL_DOWN_BOX);
    plotsPanel->color(FL_BACKGROUND_COLOR);
    
    // Create export button
    exportButton = new Fl_Button(rightPanel->x() + rightPanel->w() - 110,
                                rightPanel->y() + rightPanel->h() - 40,
                                100, 30, "Export");
    exportButton->callback(exportButtonCallback, this);
    
    rightPanel->end();
    
    end();
    
    // Initialize member variables
    model = nullptr;
    dataFrame = nullptr;
    plottingInitialized = false;
    currentPlotType = 0;
    currentModelIndex = 0;
    showModelNavigation = false;
    
    // Update the plot type label
    updatePlotTypeLabel();
    
    LOG_INFO("ResultsView construction complete", "ResultsView");
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
    
    // Resize left panel
    leftPanel->resize(x() + 10, y() + 10, w()/2 - 15, h() - 20);
    
    // Resize statistics panel
    statisticsPanel->resize(leftPanel->x() + 10, leftPanel->y() + 10,
                           leftPanel->w() - 20, leftPanel->h()/2 - 15);
    
    // Resize parameters panel
    parametersPanel->resize(leftPanel->x() + 10,
                           statisticsPanel->y() + statisticsPanel->h() + 10,
                           leftPanel->w() - 20,
                           leftPanel->h()/2 - 15);
    
    // Resize right panel
    rightPanel->resize(x() + w()/2 + 5, y() + 10, w()/2 - 15, h() - 20);
    
    // Resize navigation group
    navigationGroup->resize(rightPanel->x() + 10, rightPanel->y() + 10,
                           rightPanel->w() - 20, 40);
    
    // Resize navigation buttons
    prevButton->resize(navigationGroup->x(), navigationGroup->y(),
                      100, 30);
    nextButton->resize(prevButton->x() + prevButton->w() + 10,
                      navigationGroup->y(),
                      100, 30);
    plotTypeLabel->resize(nextButton->x() + nextButton->w() + 10,
                         navigationGroup->y(),
                         navigationGroup->w() - (nextButton->x() + nextButton->w() + 20),
                         30);
    
    // Resize plots panel
    plotsPanel->resize(rightPanel->x() + 10,
                      navigationGroup->y() + navigationGroup->h() + 10,
                      rightPanel->w() - 20,
                      rightPanel->h() - navigationGroup->h() - 60);
    
    // Resize export button
    exportButton->resize(rightPanel->x() + rightPanel->w() - 110,
                        rightPanel->y() + rightPanel->h() - 40,
                        100, 30);
    
    // Force redraw of plots if we have data
    if (model && dataFrame) {
        createPlots();
    }
    
    // Redraw everything
    redraw();
}

void ResultsView::cyclePlot(int direction) {
    LOG_INFO("Cycling plot with direction: " + std::to_string(direction), "ResultsView");
    
    if (!model || !dataFrame) {
        LOG_WARN("Cannot cycle plot: no model or data available", "ResultsView");
        return;
    }
    
    // Calculate new plot type
    int numPlotTypes = 5;  // Total number of plot types
    currentPlotType = (currentPlotType + direction + numPlotTypes) % numPlotTypes;
    
    // Create the new plot
    createPlots();
    
    // Update the plot type label
    updatePlotTypeLabel();
}

void ResultsView::updatePlotTypeLabel() {
    if (!plotTypeLabel) return;
    
    std::string label;
    switch (currentPlotType) {
        case 0:
            label = "Actual vs Predicted";
            break;
        case 1:
            label = "Residuals";
            break;
        case 2:
            label = "Time Series";
            break;
        case 3:
            label = "Feature Importance";
            break;
        case 4:
            label = "Learning Curve";
            break;
        default:
            label = "Unknown Plot Type";
            break;
    }
    
    plotTypeLabel->copy_label(label.c_str());
    plotTypeLabel->redraw();
}

void ResultsView::draw() {
    LOG_DEBUG("ResultsView::draw called", "ResultsView");
    Fl_Group::draw();
    
    // The PlotGLWindow will handle its own rendering in its draw() method
    
    LOG_DEBUG("ResultsView::draw completed", "ResultsView");
}

void ResultsView::render() {
    if (!plotsPanel || !plotsPanel->isInitialized()) {
        LOG_WARN("Cannot render: plot panel not initialized", "ResultsView");
        return;
    }
    
    try {
        LOG_DEBUG("Rendering plots", "ResultsView");
        
        // Simply redraw the plot panel - it will handle its own rendering
        plotsPanel->redraw();
        
        LOG_DEBUG("Plots rendered successfully", "ResultsView");
    } catch (const std::exception& e) {
        LOG_ERR("Error in render(): " + std::string(e.what()), "ResultsView");
        drawFallbackPlotDisplay();
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
        LOG_ERR("Cannot create plots: model or dataFrame is null", "ResultsView");
        return;
    }
    
    if (!plotsPanel) {
        LOG_ERR("Cannot create plots: PlotGLWindow is null", "ResultsView");
        return;
    }
    
    try {
        // Get actual and predicted values
        std::vector<double> actual = dataFrame->getColumn(model->getTargetName());
        
        // Get feature matrix and make predictions
        Eigen::MatrixXd featureMatrix = dataFrame->toMatrix(model->getVariableNames());
        Eigen::VectorXd predictedEigen = model->predict(featureMatrix);
        
        // Convert predictions to vector
        std::vector<double> predicted(predictedEigen.data(), predictedEigen.data() + predictedEigen.size());
        
        if (actual.empty() || predicted.empty()) {
            LOG_ERR("No data available for plotting", "ResultsView");
            return;
        }
        
        LOG_INFO("Creating plots with " + std::to_string(actual.size()) + " data points", "ResultsView");
        
        switch (currentPlotType) {
            case 0: // Scatter plot
                plotsPanel->createScatterPlot(
                    actual, predicted,
                    "Actual vs Predicted Values",
                    "Actual Values",
                    "Predicted Values"
                );
                break;
                
            case 1: // Residual plot
                {
                    std::vector<double> residuals;
                    residuals.reserve(actual.size());
                    for (size_t i = 0; i < actual.size(); ++i) {
                        residuals.push_back(actual[i] - predicted[i]);
                    }
                    plotsPanel->createResidualPlot(
                        predicted, residuals,
                        "Residual Plot"
                    );
                }
                break;
                
            case 2: // Time series plot
                plotsPanel->createTimeSeriesPlot(
                    actual, predicted,
                    "Time Series Comparison"
                );
                break;
                
            case 3: // Feature importance plot
                if (auto importanceMap = model->getFeatureImportance(); !importanceMap.empty()) {
                    plotsPanel->createImportancePlot(
                        importanceMap,
                        "Feature Importance"
                    );
                } else {
                    LOG_WARN("Feature importance not available for this model", "ResultsView");
                    // Fall back to scatter plot
                    currentPlotType = 0;
                    createPlots();
                }
                break;
                
            case 4: // Learning curve plot
                if (model->supportsLearningCurve()) {
                    std::vector<double> trainSizes, trainScores, valScores;
                    model->getLearningCurve(trainSizes, trainScores, valScores);
                    
                    if (!trainSizes.empty() && !trainScores.empty() && !valScores.empty()) {
                        plotsPanel->createLearningCurvePlot(
                            trainSizes, trainScores, valScores,
                            "Learning Curve"
                        );
                    } else {
                        LOG_WARN("Learning curve data is empty", "ResultsView");
                        // Fall back to scatter plot
                        currentPlotType = 0;
                        createPlots();
                    }
                } else {
                    LOG_WARN("Learning curve not supported by this model", "ResultsView");
                    // Fall back to scatter plot
                    currentPlotType = 0;
                    createPlots();
                }
                break;
                
            default:
                LOG_ERR("Unknown plot type: " + std::to_string(currentPlotType), "ResultsView");
                break;
        }
        
        updatePlotTypeLabel();
        plotsPanel->redraw();
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception during plot creation: " + std::string(e.what()), "ResultsView");
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

void ResultsView::updatePlots() {
    LOG_INFO("Updating plots in ResultsView", "ResultsView");
    
    try {
        if (!plotsPanel) {
            LOG_ERR("Plot panel is null", "ResultsView");
            return;
        }
        
        // Update the plot based on current data
        if (currentData.empty()) {
            LOG_WARN("No data available for plotting", "ResultsView");
            return;
        }
        
        // Create scatter plot
        plotsPanel->createScatterPlot(
            currentData["actual"],
            currentData["predicted"],
            "Actual vs Predicted Values",
            "Actual Values",
            "Predicted Values"
        );
        
        // Force a redraw of the plot panel
        plotsPanel->redraw();
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception in updatePlots: " + std::string(e.what()), "ResultsView");
    }
}

void ResultsView::exportPlot() {
    LOG_INFO("Exporting plot", "ResultsView");
    
    try {
        if (!plotsPanel) {
            LOG_ERR("Plot panel is null", "ResultsView");
            return;
        }
        
        // Get the current plot type
        auto plotType = plotsPanel->getPlotType();
        
        // Create export dialog
        ExportDialog* dialog = new ExportDialog(plotType);
        dialog->show();
        
        // Wait for user input
        while (dialog->shown()) {
            Fl::wait();
        }
        
        // If dialog was cancelled, clean up and return
        if (!dialog->wasConfirmed()) {
            LOG_INFO("Plot export cancelled by user", "ResultsView");
            delete dialog;
            return;
        }
        
        // Get export settings from dialog
        std::string filename = dialog->getFilename();
        int width = dialog->getWidth();
        int height = dialog->getHeight();
        
        // Clean up dialog
        delete dialog;
        
        // Force a redraw of the plot panel with the new dimensions
        plotsPanel->size(width, height);
        plotsPanel->redraw();
        
        LOG_INFO("Plot exported successfully to: " + filename, "ResultsView");
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception in exportPlot: " + std::string(e.what()), "ResultsView");
    }
}