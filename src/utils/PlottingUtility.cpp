#include "utils/PlottingUtility.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <windows.h>
#include <cmath>
#include <numeric>
#include <limits>
#include "utils/Logger.h"

// FLTK headers
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>

// ImGui headers
#include "imgui.h"
#include "implot.h"
#include "../backends/imgui_impl_fltk.h"
#include "../backends/imgui_impl_opengl3.h"

// Define the LOG_INFO macro if it's not available
#ifndef LOG_INFO
#define LOG_INFO(message, component) std::cout << "[INFO][" << component << "] " << message << std::endl
#endif

PlottingUtility& PlottingUtility::getInstance() {
    static PlottingUtility instance;
    return instance;
}

bool PlottingUtility::initialize(Fl_Widget* parent) {
    if (initialized) return true;
    
    LOG_INFO("Initializing PlottingUtility with widget: " + std::to_string(reinterpret_cast<uintptr_t>(parent)), "PlottingUtility");
    
    try {
        // Check for GL window - either the parent itself or its window
        LOG_DEBUG("Checking for GL window", "PlottingUtility");
        Fl_Gl_Window* gl_window = dynamic_cast<Fl_Gl_Window*>(parent);
        if (!gl_window) {
            gl_window = dynamic_cast<Fl_Gl_Window*>(parent->window());
        }
        if (!gl_window) {
            LOG_ERR("No GL window found", "PlottingUtility");
            return false;
        }
        
        // Make GL window current
        gl_window->make_current();
        
        // Store the parent widget
        initialized = true;
        parentWidget = parent;
        
        LOG_INFO("PlottingUtility initialization complete", "PlottingUtility");
        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Exception during PlottingUtility initialization: " + std::string(e.what()), "PlottingUtility");
        return false;
    }
}

void PlottingUtility::cleanup() {
    LOG_INFO("Cleaning up PlottingUtility resources", "PlottingUtility");
    
    if (!initialized) {
        LOG_WARN("PlottingUtility::cleanup called but not initialized", "PlottingUtility");
        return;
    }
    
    try {
        initialized = false;
        parentWidget = nullptr;
        
        LOG_INFO("PlottingUtility cleanup completed successfully", "PlottingUtility");
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during PlottingUtility cleanup: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::safeEndImGuiFrame() {
    if (ImGui::GetCurrentContext()) {
        // Check if we're in a frame that needs ending
        if (ImGui::GetFrameCount() > 0) {
            LOG_WARN("Ending in-progress ImGui frame during cleanup", "PlottingUtility");
            try {
                ImGui::EndFrame();
                ImGui::Render();
            }
            catch (const std::exception& e) {
                LOG_ERR("Error ending ImGui frame: " + std::string(e.what()), "PlottingUtility");
            }
        }
    }
}

void PlottingUtility::setupPlotStyle() {
    LOG_DEBUG("Setting up ImPlot style", "PlottingUtility");
    
    ImPlot::GetStyle().LineWeight = 2.0f;
    ImPlot::GetStyle().MarkerSize = 4.0f;
    ImPlot::GetStyle().MarkerWeight = 1.0f;
    ImPlot::GetStyle().ErrorBarWeight = 1.5f;
    ImPlot::GetStyle().DigitalBitHeight = 8.0f;
    
    // Set grid style
    ImPlot::GetStyle().MajorGridSize = ImVec2(1.0f, 1.0f);
    ImPlot::GetStyle().MinorGridSize = ImVec2(1.0f, 1.0f);
    
    // Make plots more readable with better colors
    ImPlot::GetStyle().UseLocalTime = false;
    ImPlot::GetStyle().UseISO8601 = false;
    ImPlot::GetStyle().Use24HourClock = true;
    
    LOG_DEBUG("ImPlot style setup complete", "PlottingUtility");
}

bool PlottingUtility::renderPlotWindow(const std::string& title, int width, int height) {
    LOG_DEBUG("Rendering plot window: " + title, "PlottingUtility");
    
    if (!parentWidget) {
        LOG_ERR("Cannot render plot window: parentWidget is null", "PlottingUtility");
        return false;
    }
    
    // Calculate appropriate dimensions
    auto [w, h] = calculatePlotDimensions(width, height);
    LOG_DEBUG("Plot dimensions: " + std::to_string(w) + "x" + std::to_string(h), "PlottingUtility");
    
    try {
        // Create a window that fills the entire area
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(w), static_cast<float>(h)));
        
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | 
                                     ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoMove | 
                                     ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoCollapse;
                                     // Removed NoBackground and NoBringToFrontOnFocus flags
        
        if (!ImGui::Begin("PlotWindow", nullptr, windowFlags)) {
            ImGui::End();
            return false;
        }
        
        // Add some padding
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        
        // Center the title
        float windowWidth = ImGui::GetWindowWidth();
        float textWidth = ImGui::CalcTextSize(title.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("%s", title.c_str());
        ImGui::Spacing();
        ImGui::Spacing();
        
        ImGui::PopStyleVar();
        
        LOG_DEBUG("ImGui plot window created successfully", "PlottingUtility");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during renderPlotWindow: " + std::string(e.what()), "PlottingUtility");
        return false;
    }
}

std::pair<int, int> PlottingUtility::calculatePlotDimensions(int width, int height) {
    if (!parentWidget) {
        LOG_WARN("Cannot calculate plot dimensions: parentWidget is null", "PlottingUtility");
        return {0, 0};
    }
    
    int w = (width > 0) ? width : parentWidget->w();
    int h = (height > 0) ? height : parentWidget->h();
    
    LOG_DEBUG("Calculated plot dimensions: " + std::to_string(w) + "x" + std::to_string(h), "PlottingUtility");
    return {w, h};
}

void PlottingUtility::createScatterPlot(
    const std::vector<double>& actual,
    const std::vector<double>& predicted,
    const std::string& title,
    const std::string& xLabel,
    const std::string& yLabel,
    int width,
    int height
) {
    LOG_INFO("Creating scatter plot: " + title, "PlottingUtility");
    
    if (actual.size() != predicted.size()) {
        LOG_ERR("Actual and predicted data must have the same size for scatter plots", "PlottingUtility");
        return;
    }
    
    if (actual.empty()) {
        LOG_ERR("Cannot create scatter plot with empty data", "PlottingUtility");
        return;
    }
    
    LOG_DEBUG("Scatter plot data size: " + std::to_string(actual.size()), "PlottingUtility");
    
    // Store the plot data
    currentPlot = PlotData();
    currentPlot.type = PlotData::Type::Scatter;
    currentPlot.title = title;
    currentPlot.xLabel = xLabel;
    currentPlot.yLabel = yLabel;
    currentPlot.xValues = actual;
    currentPlot.yValues = predicted;
    currentPlot.width = width;
    currentPlot.height = height;
    
    LOG_INFO("Scatter plot created with " + std::to_string(actual.size()) + " points", "PlottingUtility");
    LOG_INFO("Current plot type is now: " + std::to_string(static_cast<int>(currentPlot.type)), "PlottingUtility");
}

void PlottingUtility::createTimeSeriesPlot(
    const std::vector<double>& actual,
    const std::vector<double>& predicted,
    const std::string& title,
    int width,
    int height
) {
    LOG_INFO("Creating time series plot: " + title, "PlottingUtility");
    
    if (actual.size() != predicted.size()) {
        LOG_ERR("Actual and predicted data must have the same size for time series plots", "PlottingUtility");
        return;
    }
    
    if (actual.empty()) {
        LOG_ERR("Cannot create time series plot with empty data", "PlottingUtility");
        return;
    }
    
    LOG_DEBUG("Time series plot data size: " + std::to_string(actual.size()), "PlottingUtility");
    
    // Store the plot data
    currentPlot = PlotData();
    currentPlot.type = PlotData::Type::TimeSeries;
    currentPlot.title = title;
    currentPlot.xLabel = "Time";
    currentPlot.yLabel = "Value";
    
    // Create x values as indices
    std::vector<double> indices(actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        indices[i] = static_cast<double>(i);
    }
    
    currentPlot.xValues = indices;
    currentPlot.yValues = actual;
    currentPlot.y2Values = predicted;
    currentPlot.width = width;
    currentPlot.height = height;
    
    LOG_INFO("Time series plot created with " + std::to_string(actual.size()) + " points", "PlottingUtility");
}

void PlottingUtility::createResidualPlot(
    const std::vector<double>& predicted,
    const std::vector<double>& residuals,
    const std::string& title,
    int width,
    int height
) {
    LOG_INFO("Creating residual plot: " + title, "PlottingUtility");
    
    if (predicted.size() != residuals.size()) {
        LOG_ERR("Predicted and residual data must have the same size", "PlottingUtility");
        return;
    }
    
    if (predicted.empty()) {
        LOG_ERR("Cannot create residual plot with empty data", "PlottingUtility");
        return;
    }
    
    LOG_DEBUG("Residual plot data size: " + std::to_string(predicted.size()), "PlottingUtility");
    
    // Store the plot data
    currentPlot = PlotData();
    currentPlot.type = PlotData::Type::Residual;
    currentPlot.title = title;
    currentPlot.xLabel = "Predicted";
    currentPlot.yLabel = "Residual";
    currentPlot.xValues = predicted;
    currentPlot.yValues = residuals;
    currentPlot.width = width;
    currentPlot.height = height;
    
    LOG_INFO("Residual plot created with " + std::to_string(residuals.size()) + " points", "PlottingUtility");
}

void PlottingUtility::createImportancePlot(
    const std::unordered_map<std::string, double>& importance,
    const std::string& title,
    int width,
    int height
) {
    LOG_INFO("Creating importance plot: " + title, "PlottingUtility");
    
    if (importance.empty()) {
        LOG_ERR("Cannot create importance plot with empty data", "PlottingUtility");
        return;
    }
    
    LOG_DEBUG("Importance plot data size: " + std::to_string(importance.size()) + " features", "PlottingUtility");
    
    // Store the plot data
    currentPlot = PlotData();
    currentPlot.type = PlotData::Type::Importance;
    currentPlot.title = title;
    currentPlot.xLabel = "Importance";
    currentPlot.yLabel = "Feature";
    currentPlot.importanceValues = importance;
    currentPlot.width = width;
    currentPlot.height = height;
    
    LOG_INFO("Importance plot created with " + std::to_string(importance.size()) + " features", "PlottingUtility");
}

void PlottingUtility::createLearningCurvePlot(
    const std::vector<double>& trainingSizes,
    const std::vector<double>& trainingScores,
    const std::vector<double>& validationScores,
    const std::string& title,
    int width,
    int height
) {
    LOG_INFO("Creating learning curve plot: " + title, "PlottingUtility");
    
    if (trainingSizes.empty() || trainingScores.empty() || validationScores.empty()) {
        LOG_ERR("Cannot create learning curve plot with empty data", "PlottingUtility");
        return;
    }
    
    if (trainingSizes.size() != trainingScores.size() || trainingSizes.size() != validationScores.size()) {
        LOG_ERR("Training sizes, training scores, and validation scores must have the same length", "PlottingUtility");
        return;
    }
    
    LOG_DEBUG("Learning curve plot data size: " + std::to_string(trainingSizes.size()) + " points", "PlottingUtility");
    
    // Store the plot data
    currentPlot = PlotData();
    currentPlot.type = PlotData::Type::LearningCurve;
    currentPlot.title = title;
    currentPlot.xLabel = "Training Examples";
    currentPlot.yLabel = "Score";
    currentPlot.trainingSizes = trainingSizes;
    currentPlot.trainingScores = trainingScores;
    currentPlot.validationScores = validationScores;
    currentPlot.width = width;
    currentPlot.height = height;
    
    LOG_INFO("Learning curve plot created with " + std::to_string(trainingSizes.size()) + " points", "PlottingUtility");
}

void PlottingUtility::render() {
    if (!initialized || !parentWidget) {
        LOG_ERR("Cannot render: initialized=" + std::to_string(initialized) + 
                ", parentWidget=" + std::to_string(reinterpret_cast<uintptr_t>(parentWidget)), 
                "PlottingUtility");
        return;
    }
    
    LOG_DEBUG("PlottingUtility::render() called", "PlottingUtility");
    
    try {
        // Get GL window and make it current
        Fl_Gl_Window* gl_window = dynamic_cast<Fl_Gl_Window*>(parentWidget);
        if (!gl_window) {
            LOG_ERR("Parent widget is not a GL window", "PlottingUtility");
            return;
        }
        
        LOG_DEBUG("GL window dimensions: " + std::to_string(gl_window->w()) + "x" + std::to_string(gl_window->h()), 
                  "PlottingUtility");
        
        gl_window->make_current();
        
        // Clear the background
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);  // Light gray background
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Make sure ImGui frame state is clean before starting new frame
        ensureFrameIsClean();
        
        // Start new frame and render with proper exception handling
        try {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplFLTK_NewFrame();
            ImGui::NewFrame();
            
            LOG_DEBUG("ImGui frame started", "PlottingUtility");
            
            // Create an ImGui window for our plot that fills the entire area
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(gl_window->w(), gl_window->h()));
            
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                          ImGuiWindowFlags_NoResize | 
                                          ImGuiWindowFlags_NoMove | 
                                          ImGuiWindowFlags_NoScrollbar | 
                                          ImGuiWindowFlags_NoCollapse;
            
            ImGui::Begin("##PlotWindow", nullptr, window_flags);
            
            // Calculate plot size to fill the entire area
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.95f);
            
            // Set plot style with light theme
            ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            
            // Render the appropriate plot based on type
            switch (currentPlot.type) {
                case PlotData::Type::Scatter:
                    renderScatterPlot();
                    break;
                case PlotData::Type::TimeSeries:
                    renderTimeSeriesPlot();
                    break;
                case PlotData::Type::Residual:
                    renderResidualPlot();
                    break;
                case PlotData::Type::Importance:
                    renderImportancePlot();
                    break;
                case PlotData::Type::LearningCurve:
                    renderLearningCurvePlot();
                    break;
                default:
                    ImGui::Text("No plot data available");
                    break;
            }
            
            ImPlot::PopStyleColor(3);
            ImGui::End();
            
            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
        } catch (const std::exception& e) {
            LOG_ERR("Exception during ImGui rendering: " + std::string(e.what()), "PlottingUtility");
        }
        
        // Swap buffers
        gl_window->swap_buffers();
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception during plot rendering: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::renderScatterPlot() {
    LOG_DEBUG("Rendering scatter plot", "PlottingUtility");
    
    try {
        // Set up plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 6.0f);  // Larger markers
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        ImPlotAxisFlags axisFlags = ImPlotAxisFlags_AutoFit;
        
        // Begin plot with explicit size - use nearly the full available area
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty - ImPlot requires a non-empty ID
        std::string plotTitle = currentPlot.title;
        if (plotTitle.empty()) {
            plotTitle = "Scatter Plot"; // Default title if empty
        }
        
        LOG_DEBUG("Beginning ImPlot with title: '" + plotTitle + "' and size: " + 
                 std::to_string(plotSize.x) + "x" + std::to_string(plotSize.y),
                 "PlottingUtility");
        
        bool plotBegun = ImPlot::BeginPlot(plotTitle.c_str(), plotSize, flags);
        
        if (plotBegun) {
            LOG_DEBUG("ImPlot::BeginPlot succeeded", "PlottingUtility");
            
            // Set up axes with labels
            ImPlot::SetupAxis(ImAxis_X1, currentPlot.xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, currentPlot.yLabel.c_str(), axisFlags);
            
            // Add scatter plot
            if (currentPlot.xValues.size() > 0 && currentPlot.yValues.size() > 0) {
                LOG_DEBUG("Plotting scatter points, count: " + 
                         std::to_string(currentPlot.xValues.size()),
                         "PlottingUtility");
                
                // Set marker style for data points - blue circles
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 6.0f, ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 2.0f);
                
                // Plot scatter points
                ImPlot::PlotScatter("Data Points", 
                                  currentPlot.xValues.data(), 
                                  currentPlot.yValues.data(), 
                                  static_cast<int>(currentPlot.xValues.size()));
                
                // Add identity line (y=x)
                double minX = *std::min_element(currentPlot.xValues.begin(), currentPlot.xValues.end());
                double maxX = *std::max_element(currentPlot.xValues.begin(), currentPlot.xValues.end());
                double minY = *std::min_element(currentPlot.yValues.begin(), currentPlot.yValues.end());
                double maxY = *std::max_element(currentPlot.yValues.begin(), currentPlot.yValues.end());
                
                double min = std::min(minX, minY);
                double max = std::max(maxX, maxY);
                
                std::vector<double> lineX = {min, max};
                std::vector<double> lineY = {min, max};
                
                // Set line style for identity line - dark gray
                ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), 2.0f);
                ImPlot::PlotLine("y=x", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
                
                LOG_DEBUG("Added identity line from " + std::to_string(min) + " to " + std::to_string(max), "PlottingUtility");
            } else {
                LOG_WARN("No data to plot in scatter plot", "PlottingUtility");
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Scatter plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for scatter plot", "PlottingUtility");
            
            // Check for ImGui/ImPlot errors
            if (ImGui::GetIO().WantCaptureMouse) {
                LOG_DEBUG("ImGui wants to capture mouse - might be interacting with UI", "PlottingUtility");
            }
            
            // Get OpenGL error if any
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                LOG_ERR("OpenGL error during plot creation: " + std::to_string(err), "PlottingUtility");
            }
            
            // Render error text directly in ImGui
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create plot");
            ImGui::Text("Check log for details");
        }
        
        ImPlot::PopStyleVar(2);
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during scatter plot rendering: " + std::string(e.what()), "PlottingUtility");
        
        // Display error text in ImGui
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
    catch (...) {
        LOG_ERR("Unknown exception during scatter plot rendering", "PlottingUtility");
        
        // Display generic error text in ImGui
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown error occurred during rendering");
    }
}

void PlottingUtility::renderTimeSeriesPlot() {
    LOG_DEBUG("Rendering time series plot", "PlottingUtility");
    
    try {
        // Set plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        ImPlotAxisFlags axisFlags = ImPlotAxisFlags_AutoFit;
        
        // Get available size and set plot dimensions
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty
        std::string plotTitle = currentPlot.title;
        if (plotTitle.empty()) {
            plotTitle = "Time Series Plot";
        }
        
        LOG_DEBUG("Beginning time series plot with title: '" + plotTitle + "'", "PlottingUtility");
        
        bool plotBegun = ImPlot::BeginPlot(plotTitle.c_str(), plotSize, flags);
        if (plotBegun) {
            ImPlot::SetupAxis(ImAxis_X1, currentPlot.xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, currentPlot.yLabel.c_str(), axisFlags);
            
            // Add actual values line
            if (currentPlot.xValues.size() > 0 && currentPlot.yValues.size() > 0) {
                ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 2.0f);
                ImPlot::PlotLine("Actual", 
                               currentPlot.xValues.data(), 
                               currentPlot.yValues.data(), 
                               static_cast<int>(currentPlot.xValues.size()));
            }
            
            // Add predicted values line
            if (currentPlot.xValues.size() > 0 && currentPlot.y2Values.size() > 0) {
                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), 2.0f);
                ImPlot::PlotLine("Predicted", 
                               currentPlot.xValues.data(), 
                               currentPlot.y2Values.data(), 
                               static_cast<int>(currentPlot.xValues.size()));
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Time series plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for time series plot", "PlottingUtility");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create time series plot");
        }
        
        ImPlot::PopStyleVar();
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during time series plot rendering: " + std::string(e.what()), "PlottingUtility");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlottingUtility::renderResidualPlot() {
    LOG_DEBUG("Rendering residual plot", "PlottingUtility");
    
    try {
        // Set plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 4.0f);
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        ImPlotAxisFlags axisFlags = ImPlotAxisFlags_AutoFit;
        
        // Get available size and set plot dimensions
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty
        std::string plotTitle = currentPlot.title;
        if (plotTitle.empty()) {
            plotTitle = "Residual Plot";
        }
        
        LOG_DEBUG("Beginning residual plot with title: '" + plotTitle + "'", "PlottingUtility");
        
        bool plotBegun = ImPlot::BeginPlot(plotTitle.c_str(), plotSize, flags);
        if (plotBegun) {
            ImPlot::SetupAxis(ImAxis_X1, currentPlot.xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, currentPlot.yLabel.c_str(), axisFlags);
            
            // Add residual scatter plot
            if (currentPlot.xValues.size() > 0 && currentPlot.yValues.size() > 0) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4.0f, ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 1.5f);
                ImPlot::PlotScatter("Residuals", 
                                  currentPlot.xValues.data(), 
                                  currentPlot.yValues.data(), 
                                  static_cast<int>(currentPlot.xValues.size()));
            }
            
            // Add zero line
            if (!currentPlot.xValues.empty()) {
                double minX = *std::min_element(currentPlot.xValues.begin(), currentPlot.xValues.end());
                double maxX = *std::max_element(currentPlot.xValues.begin(), currentPlot.xValues.end());
                std::vector<double> lineX = {minX, maxX};
                std::vector<double> lineY = {0.0, 0.0};
                
                ImPlot::SetNextLineStyle(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), 1.5f);
                ImPlot::PlotLine("Zero", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
                
                LOG_DEBUG("Added zero line from x=" + std::to_string(minX) + " to x=" + std::to_string(maxX), "PlottingUtility");
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Residual plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for residual plot", "PlottingUtility");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create residual plot");
        }
        
        ImPlot::PopStyleVar(2);
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during residual plot rendering: " + std::string(e.what()), "PlottingUtility");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlottingUtility::renderImportancePlot() {
    LOG_DEBUG("Rendering importance plot", "PlottingUtility");
    
    try {
        // Set plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        
        // Get available size and set plot dimensions
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty
        std::string plotTitle = currentPlot.title;
        if (plotTitle.empty()) {
            plotTitle = "Feature Importance";
        }
        
        LOG_DEBUG("Beginning importance plot with title: '" + plotTitle + "'", "PlottingUtility");
        
        bool plotBegun = ImPlot::BeginPlot(plotTitle.c_str(), plotSize, flags);
        if (plotBegun) {
            // For feature importance, we create a horizontal bar chart
            ImPlot::SetupAxis(ImAxis_X1, currentPlot.xLabel.c_str(), ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxis(ImAxis_Y1, currentPlot.yLabel.c_str(), ImPlotAxisFlags_AutoFit);
            
            // Convert unordered_map to vectors and sort by importance
            std::vector<std::pair<std::string, double>> sortedImportance;
            for (const auto& pair : currentPlot.importanceValues) {
                sortedImportance.push_back(pair);
            }
            
            std::sort(sortedImportance.begin(), sortedImportance.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    });
            
            LOG_DEBUG("Sorted " + std::to_string(sortedImportance.size()) + " features by importance", "PlottingUtility");
            
            // Prepare data for plotting
            std::vector<double> values;
            std::vector<const char*> labels;
            
            for (const auto& pair : sortedImportance) {
                values.push_back(pair.second);
                labels.push_back(pair.first.c_str());
            }
            
            // Bar chart
            if (!values.empty()) {
                ImPlot::SetNextFillStyle(ImVec4(0.0f, 0.45f, 0.85f, 0.8f));
                ImPlot::PlotBars("Importance", values.data(), static_cast<int>(values.size()), 0.75, 0.0, ImPlotBarsFlags_Horizontal);
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Importance plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for importance plot", "PlottingUtility");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create importance plot");
        }
        
        ImPlot::PopStyleVar();
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during importance plot rendering: " + std::string(e.what()), "PlottingUtility");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlottingUtility::renderLearningCurvePlot() {
    LOG_DEBUG("Rendering learning curve plot", "PlottingUtility");
    
    try {
        // Set plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        ImPlotAxisFlags axisFlags = ImPlotAxisFlags_AutoFit;
        
        // Get available size and set plot dimensions
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty
        std::string plotTitle = currentPlot.title;
        if (plotTitle.empty()) {
            plotTitle = "Learning Curve";
        }
        
        LOG_DEBUG("Beginning learning curve plot with title: '" + plotTitle + "'", "PlottingUtility");
        
        bool plotBegun = ImPlot::BeginPlot(plotTitle.c_str(), plotSize, flags);
        if (plotBegun) {
            ImPlot::SetupAxis(ImAxis_X1, currentPlot.xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, currentPlot.yLabel.c_str(), axisFlags);
            
            // Add training score line
            if (currentPlot.trainingSizes.size() > 0 && currentPlot.trainingScores.size() > 0) {
                ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 2.0f);
                ImPlot::PlotLine("Training Score", 
                               currentPlot.trainingSizes.data(), 
                               currentPlot.trainingScores.data(), 
                               static_cast<int>(currentPlot.trainingSizes.size()));
            }
            
            // Add validation score line
            if (currentPlot.trainingSizes.size() > 0 && currentPlot.validationScores.size() > 0) {
                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), 2.0f);
                ImPlot::PlotLine("Validation Score", 
                               currentPlot.trainingSizes.data(), 
                               currentPlot.validationScores.data(), 
                               static_cast<int>(currentPlot.trainingSizes.size()));
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Learning curve plot rendering completed", "PlottingUtility");
        } else {
            LOG_WARN("Failed to begin learning curve plot", "PlottingUtility");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create learning curve plot");
        }
        
        ImPlot::PopStyleVar();
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during learning curve plot rendering: " + std::string(e.what()), "PlottingUtility");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlottingUtility::clear() {
    LOG_INFO("Clearing current plot data", "PlottingUtility");
    currentPlot = PlotData();
}

bool PlottingUtility::ensureFrameIsClean() {
    if (ImGui::GetCurrentContext() && ImGui::GetFrameCount() > 0) {
        LOG_WARN("ImGui frame might be in progress, ending it to be safe", "PlottingUtility");
        ImGui::EndFrame();
        return true;
    }
    return false;
}