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
        // Initialize ImGui context
        LOG_DEBUG("Initializing ImGui context", "PlottingUtility");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        // Configure ImGui IO
        LOG_DEBUG("Configuring ImGui IO", "PlottingUtility");
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Initialize ImPlot context
        LOG_DEBUG("Initializing ImPlot context", "PlottingUtility");
        ImPlot::CreateContext();
        
        // Get window handle
        LOG_DEBUG("Getting window handle", "PlottingUtility");
        Fl_Window* window = parent->window();
        if (!window) {
            LOG_ERR("Failed to get window handle", "PlottingUtility");
            return false;
        }
        
        // Check for GL window
        LOG_DEBUG("Checking for GL window", "PlottingUtility");
        Fl_Gl_Window* gl_window = dynamic_cast<Fl_Gl_Window*>(window);
        if (!gl_window) {
            LOG_WARN("Parent widget is not in a GL window, rendering may fail", "PlottingUtility");
            return false;
        }
        
        // Initialize ImGui FLTK backend
        LOG_DEBUG("Initializing ImGui FLTK backend", "PlottingUtility");
        if (!ImGui_ImplFLTK_Init(window, gl_window)) {
            LOG_ERR("Failed to initialize ImGui FLTK backend", "PlottingUtility");
            return false;
        }
        
        // Initialize ImGui OpenGL3 backend
        LOG_DEBUG("Initializing ImGui OpenGL3 backend", "PlottingUtility");
        if (!ImGui_ImplOpenGL3_Init("#version 130")) {
            LOG_ERR("Failed to initialize ImGui OpenGL3 backend", "PlottingUtility");
            return false;
        }
        
        // Set up plot style
        LOG_DEBUG("Setting up plot style", "PlottingUtility");
        ImGui::StyleColorsDark();
        
        // Set up ImPlot style
        LOG_DEBUG("Setting up ImPlot style", "PlottingUtility");
        ImPlot::StyleColorsDark();
        
        // Build font atlas
        LOG_DEBUG("Building font atlas", "PlottingUtility");
        if (!io.Fonts->IsBuilt()) {
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            // The font atlas will be built by the OpenGL3 renderer
        }
        
        LOG_DEBUG("ImPlot style setup complete", "PlottingUtility");
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
    LOG_INFO("Cleaning up PlottingUtility", "PlottingUtility");
    
    // Shutdown ImGui backends
    LOG_DEBUG("Shutting down ImGui OpenGL3 backend", "PlottingUtility");
    ImGui_ImplOpenGL3_Shutdown();
    
    LOG_DEBUG("Shutting down ImGui FLTK backend", "PlottingUtility");
    ImGui_ImplFLTK_Shutdown();
    
    // Destroy ImPlot and ImGui contexts
    LOG_DEBUG("Destroying ImPlot context", "PlottingUtility");
    ImPlot::DestroyContext();
    
    LOG_DEBUG("Destroying ImGui context", "PlottingUtility");
    ImGui::DestroyContext();
    
    LOG_INFO("PlottingUtility cleanup complete", "PlottingUtility");
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
    
    // Start ImGui frame
    LOG_DEBUG("Starting new ImGui frame", "PlottingUtility");
    try {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplFLTK_NewFrame();
        ImGui::NewFrame();
        
        // Create a full-size window without decorations
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(w), static_cast<float>(h)));
        
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | 
                                      ImGuiWindowFlags_NoResize | 
                                      ImGuiWindowFlags_NoMove | 
                                      ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoInputs;
        
        ImGui::Begin("PlotWindow", nullptr, windowFlags);
        
        // Center the title
        float windowWidth = ImGui::GetWindowWidth();
        float textWidth = ImGui::CalcTextSize(title.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("%s", title.c_str());
        ImGui::Spacing();
        
        LOG_DEBUG("ImGui plot window created successfully", "PlottingUtility");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during renderPlotWindow: " + std::string(e.what()), "PlottingUtility");
        return false;
    }
    catch (...) {
        LOG_ERR("Unknown exception during renderPlotWindow", "PlottingUtility");
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
    if (!initialized || !parentWidget) return;
    
    LOG_DEBUG("PlottingUtility::render() called", "PlottingUtility");
    
    try {
        // Start new frame for both backends
        ImGui_ImplFLTK_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        
        // Set up the window
        if (!renderPlotWindow(currentPlot.title, currentPlot.width, currentPlot.height)) {
            LOG_ERR("Failed to render plot window", "PlottingUtility");
            return;
        }
        
        LOG_DEBUG("Rendering plot type: " + std::to_string(static_cast<int>(currentPlot.type)), "PlottingUtility");
        
        // Actual plotting code based on the type
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
                LOG_WARN("Unknown plot type: " + std::to_string(static_cast<int>(currentPlot.type)), "PlottingUtility");
                break;
        }
        
        // End the window
        ImGui::End();
        
        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        Fl_Window* window = parentWidget->window();
        if (window) {
            display_w = window->w();
            display_h = window->h();
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        
        LOG_DEBUG("Plot rendering completed successfully", "PlottingUtility");
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during PlottingUtility rendering: " + std::string(e.what()), "PlottingUtility");
        throw; // Re-throw to let caller handle with fallback
    }
    catch (...) {
        LOG_ERR("Unknown exception during PlottingUtility rendering", "PlottingUtility");
        throw; // Re-throw to let caller handle with fallback
    }
}

void PlottingUtility::renderScatterPlot() {
    LOG_DEBUG("Rendering scatter plot", "PlottingUtility");
    
    try {
        if (ImPlot::BeginPlot(currentPlot.title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(currentPlot.xLabel.c_str(), currentPlot.yLabel.c_str());
            
            // Add scatter plot
            if (currentPlot.xValues.size() > 0 && currentPlot.yValues.size() > 0) {
                ImPlot::PlotScatter("Data", 
                                  currentPlot.xValues.data(), 
                                  currentPlot.yValues.data(), 
                                  static_cast<int>(currentPlot.xValues.size()));
            }
            
            // Add identity line (y=x) if data range allows
            if (!currentPlot.xValues.empty() && !currentPlot.yValues.empty()) {
                double minX = *std::min_element(currentPlot.xValues.begin(), currentPlot.xValues.end());
                double maxX = *std::max_element(currentPlot.xValues.begin(), currentPlot.xValues.end());
                double minY = *std::min_element(currentPlot.yValues.begin(), currentPlot.yValues.end());
                double maxY = *std::max_element(currentPlot.yValues.begin(), currentPlot.yValues.end());
                
                double min = std::min(minX, minY);
                double max = std::max(maxX, maxY);
                
                std::vector<double> lineX = {min, max};
                std::vector<double> lineY = {min, max};
                
                ImPlot::PlotLine("y=x", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
                
                LOG_DEBUG("Added identity line from " + std::to_string(min) + " to " + std::to_string(max), "PlottingUtility");
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Scatter plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for scatter plot", "PlottingUtility");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during scatter plot rendering: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::renderTimeSeriesPlot() {
    LOG_DEBUG("Rendering time series plot", "PlottingUtility");
    
    try {
        if (ImPlot::BeginPlot(currentPlot.title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(currentPlot.xLabel.c_str(), currentPlot.yLabel.c_str());
            
            // Add actual values line
            if (currentPlot.xValues.size() > 0 && currentPlot.yValues.size() > 0) {
                ImPlot::PlotLine("Actual", 
                               currentPlot.xValues.data(), 
                               currentPlot.yValues.data(), 
                               static_cast<int>(currentPlot.xValues.size()));
            }
            
            // Add predicted values line
            if (currentPlot.xValues.size() > 0 && currentPlot.y2Values.size() > 0) {
                ImPlot::PlotLine("Predicted", 
                               currentPlot.xValues.data(), 
                               currentPlot.y2Values.data(), 
                               static_cast<int>(currentPlot.xValues.size()));
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Time series plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for time series plot", "PlottingUtility");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during time series plot rendering: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::renderResidualPlot() {
    LOG_DEBUG("Rendering residual plot", "PlottingUtility");
    
    try {
        if (ImPlot::BeginPlot(currentPlot.title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(currentPlot.xLabel.c_str(), currentPlot.yLabel.c_str());
            
            // Add residual scatter plot
            if (currentPlot.xValues.size() > 0 && currentPlot.yValues.size() > 0) {
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
                
                ImPlot::PlotLine("Zero", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
                
                LOG_DEBUG("Added zero line from x=" + std::to_string(minX) + " to x=" + std::to_string(maxX), "PlottingUtility");
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Residual plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for residual plot", "PlottingUtility");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during residual plot rendering: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::renderImportancePlot() {
    LOG_DEBUG("Rendering importance plot", "PlottingUtility");
    
    try {
        if (ImPlot::BeginPlot(currentPlot.title.c_str(), ImVec2(-1, -1))) {
            // For feature importance, we create a horizontal bar chart
            ImPlot::SetupAxes(currentPlot.xLabel.c_str(), currentPlot.yLabel.c_str(), 
                            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            
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
                ImPlot::PlotBars("Importance", values.data(), static_cast<int>(values.size()), 0.75, 0.0, ImPlotBarsFlags_Horizontal);
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Importance plot rendered successfully", "PlottingUtility");
        } else {
            LOG_WARN("ImPlot::BeginPlot failed for importance plot", "PlottingUtility");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during importance plot rendering: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::renderLearningCurvePlot() {
    LOG_DEBUG("Rendering learning curve plot", "PlottingUtility");
    
    try {
        if (ImPlot::BeginPlot(currentPlot.title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes(currentPlot.xLabel.c_str(), currentPlot.yLabel.c_str());
            
            // Add training score line
            if (currentPlot.trainingSizes.size() > 0 && currentPlot.trainingScores.size() > 0) {
                ImPlot::PlotLine("Training Score", 
                               currentPlot.trainingSizes.data(), 
                               currentPlot.trainingScores.data(), 
                               static_cast<int>(currentPlot.trainingSizes.size()));
            }
            
            // Add validation score line
            if (currentPlot.trainingSizes.size() > 0 && currentPlot.validationScores.size() > 0) {
                ImPlot::PlotLine("Validation Score", 
                               currentPlot.trainingSizes.data(), 
                               currentPlot.validationScores.data(), 
                               static_cast<int>(currentPlot.trainingSizes.size()));
            }
            
            ImPlot::EndPlot();
            LOG_DEBUG("Learning curve plot rendering completed", "PlottingUtility");
        } else {
            LOG_ERR("Failed to begin learning curve plot", "PlottingUtility");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during learning curve plot rendering: " + std::string(e.what()), "PlottingUtility");
    }
}

void PlottingUtility::clear() {
    LOG_INFO("Clearing current plot data", "PlottingUtility");
    currentPlot = PlotData();
}