#include <iostream>
#include <algorithm>
#include <numeric>
#include <limits>
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
#include <matplotlibcpp.h>
namespace plt = matplotlibcpp;

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

bool PlotNavigator::savePlotToFile(size_t index, const std::string& filename) {
    if (index >= plots.size()) {
        return false;
    }
    
    return plots[index]->savePlot(filename);
}