#include "gui/PlotNavigator.h"
#include "utils/Logger.h"
#include <FL/fl_ask.H>
#include <algorithm>
#include <filesystem>

// PlotNavigator implementation
PlotNavigator::PlotNavigator(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), currentPlotIndex(0)
{
    box(FL_DOWN_BOX);
    color(FL_WHITE);

    // Create navigation buttons at the bottom
    int buttonWidth = 30;
    int buttonHeight = 25;
    int buttonY = y + h - buttonHeight - 5;
    
    prevButton = new Fl_Button(x + 5, buttonY, buttonWidth, buttonHeight, "@<");
    prevButton->callback(prevButtonCallback, this);
    
    nextButton = new Fl_Button(x + w - buttonWidth - 5, buttonY, buttonWidth, buttonHeight, "@>");
    nextButton->callback(nextButtonCallback, this);
    
    // Create plot label
    plotLabel = new Fl_Box(x + buttonWidth + 10, buttonY, w - 2*buttonWidth - 20, buttonHeight);
    plotLabel->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    
    end();
    updateNavigationButtons();
}

PlotNavigator::~PlotNavigator()
{
    clearPlots();
}

void PlotNavigator::createPlot(const std::shared_ptr<DataFrame>& data,
                       const std::shared_ptr<Model>& model,
                       const std::string& plotType,
                       const std::string& title)
{
    // Define a logging macro if not available
    #ifndef LOG_INFO
    #define LOG_INFO(message, component) std::cout << "[INFO][" << component << "] " << message << std::endl
    #endif
    
    LOG_INFO("Creating plot of type: " + plotType, "PlotNavigator");
    
    // Initialize plot widget with proper dimensions
    int plotX = x() + 5;
    int plotY = y() + 5;
    int plotW = w() - 10;
    int plotH = h() - 40;  // Leave space for navigation buttons
    
    PlotWidget* plot = new PlotWidget(plotX, plotY, plotW, plotH);
    plots.push_back(plot);
    
    // Variables to hold the temp file paths
    std::string tempDataPath, tempImagePath, tempScriptPath;
    
    // Create the appropriate plot based on type
    if (plotType == "scatter") {
        // Create temporary files with proper paths
        plot->createTempFilePaths("scatter", tempDataPath, tempImagePath, tempScriptPath);
        
        // Load the data for the plot
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        LOG_INFO("Scatter plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()), "PlotNavigator");
        
        plot->createScatterPlot(actual, predictedVec, "Actual Values", "Predicted Values", title,
                               tempDataPath, tempImagePath, tempScriptPath);
    }
    else if (plotType == "timeseries") {
        // Create temporary files with proper paths
        plot->createTempFilePaths("timeseries", tempDataPath, tempImagePath, tempScriptPath);
        
        // Load the data for the plot
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        LOG_INFO("Time series plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()), "PlotNavigator");
        
        plot->createTimeseriesPlot(actual, predictedVec, title,
                                  tempDataPath, tempImagePath, tempScriptPath);
    }
    else if (plotType == "importance") {
        // Create temporary files with proper paths
        plot->createTempFilePaths("importance", tempDataPath, tempImagePath, tempScriptPath);
        
        // Load feature importance data
        std::unordered_map<std::string, double> importance = model->getFeatureImportance();
        
        LOG_INFO("Feature importance plot with " + std::to_string(importance.size()) + " features", "PlotNavigator");
        
        plot->createImportancePlot(importance, title, tempDataPath, tempImagePath, tempScriptPath);
    }
    else if (plotType == "residual") {
        // Create temporary files with proper paths
        plot->createTempFilePaths("residual", tempDataPath, tempImagePath, tempScriptPath);
        
        // Load the data for the plot
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        LOG_INFO("Residual plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()), "PlotNavigator");
        
        plot->createResidualPlot(actual, predictedVec, title,
                                tempDataPath, tempImagePath, tempScriptPath);
    }
    
    // Update plot navigator
    add(plot);
    plot->hide();  // Hide initially, will be shown based on currentPlotIndex
    currentPlotIndex = plots.size() - 1;
    updateVisibility();
    updateNavigationButtons();
    
    LOG_INFO("Plot creation completed", "PlotNavigator");
}

void PlotNavigator::nextPlot()
{
    if (currentPlotIndex < plots.size() - 1) {
        currentPlotIndex++;
        updateVisibility();
        updateNavigationButtons();
    }
}

void PlotNavigator::prevPlot()
{
    if (currentPlotIndex > 0) {
        currentPlotIndex--;
        updateVisibility();
        updateNavigationButtons();
    }
}

void PlotNavigator::clearPlots()
{
    for (auto plot : plots) {
        remove(plot);
        delete plot;
    }
    plots.clear();
    currentPlotIndex = 0;
    updateNavigationButtons();
}

void PlotNavigator::prevButtonCallback(Fl_Widget*, void* v)
{
    ((PlotNavigator*)v)->prevPlot();
}

void PlotNavigator::nextButtonCallback(Fl_Widget*, void* v)
{
    ((PlotNavigator*)v)->nextPlot();
}

void PlotNavigator::updateVisibility()
{
    for (size_t i = 0; i < plots.size(); i++) {
        if (i == currentPlotIndex) {
            plots[i]->show();
        } else {
            plots[i]->hide();
        }
    }
    
    // Update plot label
    if (!plots.empty()) {
        char label[32];
        snprintf(label, sizeof(label), "Plot %zu of %zu", currentPlotIndex + 1, plots.size());
        plotLabel->copy_label(label);
    } else {
        plotLabel->copy_label("No plots available");
    }
    
    redraw();
}

void PlotNavigator::updateNavigationButtons()
{
    prevButton->activate();
    nextButton->activate();
    
    if (plots.empty() || currentPlotIndex == 0) {
        prevButton->deactivate();
    }
    if (plots.empty() || currentPlotIndex == plots.size() - 1) {
        nextButton->deactivate();
    }
}

bool PlotNavigator::savePlotToFile(size_t index, const std::string& filename) {
    if (index >= plots.size()) {
        return false;
    }

    try {
        // Create the directory if it doesn't exist
        std::filesystem::path filePath(filename);
        std::filesystem::create_directories(filePath.parent_path());

        // Save the plot using matplotlib's savefig
        plots[index]->savePlot(filename);
        return true;
    }
    catch (const std::exception& e) {
        fl_alert("Failed to save plot: %s", e.what());
        return false;
    }
} 