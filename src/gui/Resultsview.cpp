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
    plotsPanel = new Fl_Gl_Window(rightPanel->x() + 10,
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
    
    // Resize plot type label
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
    
    // Initialize plotting if we have a model and plots panel
    if (model && plotsPanel && !plottingInitialized) {
        LOG_INFO("Initializing PlottingUtility", "ResultsView");
        try {
            plotsPanel->make_current();
            
            if (PlottingUtility::getInstance().initialize(plotsPanel)) {
                LOG_INFO("PlottingUtility initialized successfully", "ResultsView");
                plottingInitialized = true;
                createPlots();
                updatePlotTypeLabel();
            } else {
                LOG_ERR("Failed to initialize PlottingUtility", "ResultsView");
            }
        } catch (const std::exception& e) {
            LOG_ERR("Exception initializing PlottingUtility: " + std::string(e.what()), "ResultsView");
        }
    }
    
    LOG_INFO("ResultsView::layout() completed", "ResultsView");
}

void ResultsView::cyclePlot(int direction) {
    if (!plottingInitialized) return;
    
    currentPlotType = (currentPlotType + direction + 5) % 5;
    updatePlotTypeLabel();
    
    // Force a redraw of the label
    plotTypeLabel->redraw();
    
    // Force a redraw of the plots panel
    if (plotsPanel) {
        plotsPanel->make_current();
        plotsPanel->redraw();
    }
    
    // Force a redraw of the entire view
    redraw();
}

void ResultsView::updatePlotTypeLabel() {
    const char* plotTypes[] = {
        "Scatter Plot",
        "Time Series",
        "Residual Plot",
        "Importance Plot",
        "Learning Curve"
    };
    plotTypeLabel->label(plotTypes[currentPlotType]);
    
    // Update the current plot type in PlottingUtility
    PlottingUtility::getInstance().setCurrentPlotType(static_cast<PlottingUtility::PlotData::Type>(currentPlotType));
}

void ResultsView::draw() {
    LOG_DEBUG("ResultsView::draw called", "ResultsView");
    Fl_Group::draw();
    
    if (plottingInitialized && plotsPanel) {
        LOG_DEBUG("Making GL window current and rendering plots", "ResultsView");
        plotsPanel->make_current();
        render();
    } else {
        LOG_WARN("Cannot render plots: plottingInitialized=" + std::to_string(plottingInitialized) + 
                 ", plotsPanel=" + std::to_string(reinterpret_cast<uintptr_t>(plotsPanel)), 
                 "ResultsView");
    }
    
    LOG_DEBUG("ResultsView::draw completed", "ResultsView");
}

void ResultsView::render() {
    if (!plottingInitialized || !plotsPanel) {
        LOG_WARN("Cannot render: plotting not initialized or no plots panel", "ResultsView");
        return;
    }
    
    try {
        LOG_DEBUG("Setting up OpenGL context for rendering", "ResultsView");
        
        // Make sure the GL window is current
        plotsPanel->make_current();
        
        // Set up OpenGL viewport
        glViewport(0, 0, plotsPanel->w(), plotsPanel->h());
        
        // Clear the background
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);  // Light gray background
        glClear(GL_COLOR_BUFFER_BIT);
        
        LOG_DEBUG("Calling PlottingUtility::render()", "ResultsView");
        PlottingUtility::getInstance().render();
        
        // Swap buffers
        plotsPanel->swap_buffers();
        
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
        LOG_ERR("Cannot create plots: model=" + std::to_string(reinterpret_cast<uintptr_t>(model.get())) + 
                ", dataFrame=" + std::to_string(reinterpret_cast<uintptr_t>(dataFrame.get())), 
                "ResultsView");
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
        LOG_DEBUG("Creating scatter plot", "ResultsView");
        PlottingUtility::getInstance().createScatterPlot(
            actual, predicted,
            "Actual vs Predicted Values",
            "Actual Values",
            "Predicted Values"
        );
        
        // Create time series plot
        LOG_DEBUG("Creating time series plot", "ResultsView");
        PlottingUtility::getInstance().createTimeSeriesPlot(
            actual, predicted,
            "Time Series Plot"
        );
        
        // Calculate and create residual plot
        LOG_DEBUG("Creating residual plot", "ResultsView");
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
            LOG_DEBUG("Creating importance plot with " + std::to_string(importance.size()) + " features", "ResultsView");
            PlottingUtility::getInstance().createImportancePlot(
                importance,
                "Feature Importance"
            );
        }
        
        LOG_INFO("All plots created successfully", "ResultsView");
        
        // Force a redraw of the plots panel
        if (plotsPanel) {
            LOG_DEBUG("Forcing plots panel redraw", "ResultsView");
            plotsPanel->redraw();
        }
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