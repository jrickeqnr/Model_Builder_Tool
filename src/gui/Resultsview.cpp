#include <iostream>
#include "gui/ResultsView.h"
#include "utils/Logger.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_PNG_Image.H>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <sstream>
#include <chrono>
#include <ctime>
#include <filesystem>

// Define the LOG_INFO macro if it's not available
#ifndef LOG_INFO
#define LOG_INFO(message, component) std::cout << "[INFO][" << component << "] " << message << std::endl
#endif

// PlotWidget implementation
PlotWidget::PlotWidget(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), plotImageData(nullptr), plotImageWidth(0), plotImageHeight(0),
      currentPlotType(PlotType::None), resizeTimerActive(false), pendingWidth(0), pendingHeight(0)
{
    box(FL_DOWN_BOX);
    color(FL_WHITE);
    
    // Create a box to display the plot image
    plotBox = new Fl_Box(x, y, w, h);
    plotBox->box(FL_FLAT_BOX);
    
    end();
}

PlotWidget::~PlotWidget() {
    if (plotImageData) {
        delete[] plotImageData;
    }
}

void PlotWidget::draw() {
    Fl_Group::draw();
    
    if (plotImageData) {
        // Draw plot image
        fl_draw_image((const uchar*)plotImageData, x(), y(), plotImageWidth, plotImageHeight, 3, 0);
    } else {
        // Draw message when no plot is available
        fl_color(FL_BLACK);
        fl_font(FL_HELVETICA, 14);
        fl_draw("No plot available", x() + 10, y() + 20);
    }
}

void PlotWidget::resizeTimeoutCallback(void* v) {
    PlotWidget* widget = static_cast<PlotWidget*>(v);
    widget->resizeTimerActive = false;
    
    // Call base class resize first
    widget->Fl_Group::resize(widget->x(), widget->y(), widget->pendingWidth, widget->pendingHeight);
    
    // Resize the plot box
    widget->plotBox->resize(widget->x(), widget->y(), widget->pendingWidth, widget->pendingHeight);
    
    // Regenerate the plot with new dimensions if we have data
    if (widget->currentPlotType != PlotType::None) {
        widget->regeneratePlot();
    }
}

void PlotWidget::resize(int x, int y, int w, int h) {
    // Store the pending dimensions
    pendingWidth = w;
    pendingHeight = h;
    
    // If a timer is already active, remove it
    if (resizeTimerActive) {
        Fl::remove_timeout(resizeTimeoutCallback, this);
    }
    
    // Set a new timer
    resizeTimerActive = true;
    Fl::add_timeout(RESIZE_DELAY, resizeTimeoutCallback, this);
    
    // Immediately resize the widget without regenerating the plot
    Fl_Group::resize(x, y, w, h);
    plotBox->resize(x, y, w, h);
    redraw();
}

void PlotWidget::regeneratePlot() {
    switch (currentPlotType) {
        case PlotType::Scatter:
            createScatterPlot(storedActualValues, storedPredictedValues, 
                            storedXLabel, storedYLabel, storedTitle);
            break;
        case PlotType::Timeseries:
            createTimeseriesPlot(storedActualValues, storedPredictedValues, storedTitle);
            break;
        case PlotType::Importance:
            createImportancePlot(storedImportance, storedTitle);
            break;
        case PlotType::Residual:
            createResidualPlot(storedActualValues, storedPredictedValues, storedTitle);
            break;
        case PlotType::LearningCurve:
            createLearningCurvePlot(storedTrainingScores, storedValidationScores, storedTrainingSizes, storedTitle);
            break;
        case PlotType::NeuralNetworkArchitecture:
            createNeuralNetworkArchitecturePlot(storedLayerSizes, storedTitle);
            break;
        case PlotType::TreeVisualization:
            createTreeVisualizationPlot(storedTreeStructure, storedTitle);
            break;
        case PlotType::None:
            break;
    }
}

void PlotWidget::createScatterPlot(const std::vector<double>& actualValues,
                          const std::vector<double>& predictedValues,
                          const std::string& xLabel,
                          const std::string& yLabel,
                          const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::Scatter;
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedXLabel = xLabel;
    storedYLabel = yLabel;
    storedTitle = title;
    
    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Create the scatter plot
    plt::scatter(actualValues, predictedValues);
    
    // Add a diagonal reference line (y=x)
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    
    for (const auto& val : actualValues) {
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
    }
    
    for (const auto& val : predictedValues) {
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
    }
    
    std::vector<double> line = {min_val, max_val};
    plt::plot(line, line, "r--");
    
    // Add labels and title
    plt::xlabel(xLabel);
    plt::ylabel(yLabel);
    plt::title(title);
    plt::grid(true);
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

void PlotWidget::createTimeseriesPlot(const std::vector<double>& actualValues,
                                     const std::vector<double>& predictedValues,
                                     const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::Timeseries;
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedTitle = title;

    // Create index values for the X-axis
    std::vector<double> indices(actualValues.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = static_cast<double>(i);
    }

    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Create the line plots
    plt::named_plot("Actual", indices, actualValues, "b-");
    plt::named_plot("Predicted", indices, predictedValues, "r-");
    
    // Add labels and title
    plt::xlabel("Time Index");
    plt::ylabel("Values");
    plt::title(title);
    plt::legend();
    plt::grid(true);
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

void PlotWidget::createImportancePlot(const std::unordered_map<std::string, double>& importance,
                                 const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::Importance;
    storedImportance = importance;
    storedTitle = title;

    // Convert the map to vectors for plotting
    std::vector<std::string> features;
    std::vector<double> values;
    
    for (const auto& pair : importance) {
        features.push_back(pair.first);
        values.push_back(pair.second);
    }

    // Sort by importance values
    std::vector<size_t> indices(values.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&values](size_t a, size_t b) { return values[a] < values[b]; });
    
    std::vector<std::string> sorted_features;
    std::vector<double> sorted_values;
    
    for (size_t idx : indices) {
        sorted_features.push_back(features[idx]);
        sorted_values.push_back(values[idx]);
    }
    
    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Create numeric indices for the y-axis
    std::vector<double> y_indices(sorted_features.size());
    std::iota(y_indices.begin(), y_indices.end(), 0.0);
    
    // Create the bar plot
    plt::barh(y_indices, sorted_values);
    
    // Set y-axis labels
    plt::yticks(y_indices, sorted_features);
    
    // Add labels and title
    plt::xlabel("Relative Importance");
    plt::title(title);
    plt::grid(true);
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

void PlotWidget::createResidualPlot(const std::vector<double>& actualValues,
                           const std::vector<double>& predictedValues,
                           const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::Residual;
    storedActualValues = actualValues;
    storedPredictedValues = predictedValues;
    storedTitle = title;

    // Calculate residuals
    std::vector<double> residuals;
    for (size_t i = 0; i < std::min(actualValues.size(), predictedValues.size()); ++i) {
        residuals.push_back(actualValues[i] - predictedValues[i]);
    }

    // Create index values for the X-axis
    std::vector<double> indices(residuals.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = static_cast<double>(i);
    }

    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Create the line plot for residuals
    plt::plot(indices, residuals, "b.");
    
    // Add a horizontal line at y=0
    std::vector<double> zero_line(indices.size(), 0.0);
    plt::plot(indices, zero_line, "r--");
    
    // Add labels and title
    plt::xlabel("Index");
    plt::ylabel("Residual (Actual - Predicted)");
    plt::title(title);
    plt::grid(true);
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

void PlotWidget::createLearningCurvePlot(const std::vector<double>& trainingScores,
                                const std::vector<double>& validationScores,
                                const std::vector<int>& trainingSizes,
                                const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::LearningCurve;
    storedTrainingScores = trainingScores;
    storedValidationScores = validationScores;
    storedTrainingSizes = trainingSizes;
    storedTitle = title;

    // Convert training sizes to doubles for plotting
    std::vector<double> trainingSizesDouble;
    for (int size : trainingSizes) {
        trainingSizesDouble.push_back(static_cast<double>(size));
    }

    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Create the line plots
    plt::named_plot("Training Score", trainingSizesDouble, trainingScores, "bo-");
    plt::named_plot("Validation Score", trainingSizesDouble, validationScores, "ro-");
    
    // Add labels and title
    plt::xlabel("Training Set Size");
    plt::ylabel("Score");
    plt::title(title);
    plt::legend();
    plt::grid(true);
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

void PlotWidget::createNeuralNetworkArchitecturePlot(const std::vector<int>& layerSizes,
                                            const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::NeuralNetworkArchitecture;
    storedLayerSizes = layerSizes;
    storedTitle = title;

    // Convert layer sizes to doubles for plotting
    std::vector<double> layerIndices;
    std::vector<double> sizes;
    
    for (size_t i = 0; i < layerSizes.size(); ++i) {
        layerIndices.push_back(static_cast<double>(i));
        sizes.push_back(static_cast<double>(layerSizes[i]));
    }

    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Create the bar plot
    plt::bar(layerIndices, sizes);
    
    // Add labels and title
    plt::xlabel("Layer");
    plt::ylabel("Number of Neurons");
    plt::title(title);
    plt::grid(true);
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

void PlotWidget::createTreeVisualizationPlot(const std::string& treeStructure,
                                    const std::string& title) {
    // Store data for regeneration
    currentPlotType = PlotType::TreeVisualization;
    storedTreeStructure = treeStructure;
    storedTitle = title;

    // Tree visualization is complex to do with matplotlibcpp directly
    // We'll use a simplified approach and just display the text description
    
    // Clear any existing figure
    plt::clf();
    
    // Create a new figure with dimensions based on widget size
    plt::figure_size(w() / 100, h() / 100);
    
    // Set title
    plt::title(title);
    
    // Add tree structure as text
    // This is a simplified approach - proper tree visualization
    // would require more complex drawing logic
    plt::text(0.5, 0.5, "Tree structure available as text description only");
    
    // Use a temporary file for the image
    std::string tempImagePath = "temp_plot_image.png";
    
    // Save the plot to the temporary file
    plt::save(tempImagePath);
    
    // Load the generated image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());
    if (!pngImage || pngImage->w() <= 0 || pngImage->h() <= 0) {
        LOG_ERR("ERROR: Failed to load the generated plot image: " + tempImagePath, "Resultsview");
        return;
    }
    
    // Store the image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();
    
    // Create a new buffer for the image data
    if (plotImageData) {
        delete[] plotImageData;
    }
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];  // RGB format
    
    // Copy the image data
    const char* imageData = (const char*)pngImage->data()[0];
    int depth = pngImage->d();
    
    for (int y = 0; y < plotImageHeight; y++) {
        for (int x = 0; x < plotImageWidth; x++) {
            int srcIdx = (y * plotImageWidth + x) * depth;
            int dstIdx = (y * plotImageWidth + x) * 3;
            
            if (depth >= 3) {
                // RGB data
                plotImageData[dstIdx] = imageData[srcIdx];     // R
                plotImageData[dstIdx+1] = imageData[srcIdx+1]; // G
                plotImageData[dstIdx+2] = imageData[srcIdx+2]; // B
            } else if (depth == 1) {
                // Grayscale
                plotImageData[dstIdx] = 
                plotImageData[dstIdx+1] = 
                plotImageData[dstIdx+2] = imageData[srcIdx];
            }
        }
    }
    
    // Clean up the PNG image
    delete pngImage;
    
    // Clean up temporary file
    std::remove(tempImagePath.c_str());
    
    // Redraw the widget
    redraw();
}

bool PlotWidget::savePlot(const std::string& filename) {
    if (currentPlotType == PlotType::None) {
        return false;
    }
    
    try {
        // First regenerate the plot using direct matplotlibcpp calls
        regeneratePlot();
        
        // Then save directly to the requested filename
        std::filesystem::path filePath(filename);
        std::filesystem::create_directories(filePath.parent_path());
        
        // Get file extension
        std::string ext = filePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        // Temporarily save to PNG (matplotlib-cpp does best with PNG)
        std::string tempPath = "temp_plot_save.png";
        plt::save(tempPath);
        
        // Copy from temp location to target file
        std::filesystem::copy_file(tempPath, filePath, 
                                  std::filesystem::copy_options::overwrite_existing);
        
        // Clean up temp file
        std::remove(tempPath.c_str());
        
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("ERROR: Exception while saving plot: " + std::string(e.what()), "Resultsview");
        return false;
    }
}

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
    LOG_INFO("Creating plot of type: " + plotType, "Resultsview");
    
    int plotX = x() + 5;
    int plotY = y() + 5;
    int plotW = w() - 10;
    int plotH = h() - 40;  // Leave space for navigation buttons
    
    LOG_INFO("Plot dimensions - X: " + std::to_string(plotX) + 
              ", Y: " + std::to_string(plotY) + 
              ", W: " + std::to_string(plotW) + 
              ", H: " + std::to_string(plotH), "Resultsview");
    
    PlotWidget* plot = new PlotWidget(plotX, plotY, plotW, plotH);
    plots.push_back(plot);
    
    // Create the plot based on type
    if (plotType == "scatter") {
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        LOG_INFO("Scatter plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()),"Resultsview");
        
        plot->createScatterPlot(actual, predictedVec, "Actual Values", "Predicted Values", title);
    }
    else if (plotType == "timeseries") {
        std::vector<double> actual = data->getColumn(model->getTargetName());
        Eigen::MatrixXd X = data->toMatrix(model->getVariableNames());
        Eigen::VectorXd predicted = model->predict(X);
        std::vector<double> predictedVec(predicted.data(), predicted.data() + predicted.size());
        
        LOG_INFO("Time series plot data size - Actual: " + std::to_string(actual.size()) + 
                 ", Predicted: " + std::to_string(predictedVec.size()),"Resultsview");
        
        plot->createTimeseriesPlot(actual, predictedVec, title);
    }
    else if (plotType == "importance") {
        auto importance = model->getFeatureImportance();
        
        LOG_INFO("Feature importance plot - Number of features: " + std::to_string(importance.size()),"Resultsview");
        
        plot->createImportancePlot(importance, title);
    }
    
    LOG_INFO("Adding plot to navigator","Resultsview");
    add(plot);
    
    LOG_INFO("Plot widget dimensions after add - X: " + std::to_string(plot->x()) + 
              ", Y: " + std::to_string(plot->y()) + 
              ", W: " + std::to_string(plot->w()) + 
              ", H: " + std::to_string(plot->h()),"Resultsview");
    
    updateVisibility();
    updateNavigationButtons();
    LOG_INFO("Plot creation completed","Resultsview");
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
    
    return plots[index]->savePlot(filename);
}

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