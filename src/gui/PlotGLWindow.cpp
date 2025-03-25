#include "gui/PlotGLWindow.h"

// Standard library headers
#include <algorithm>
#include <string>
#include <cmath>
#include <stdexcept>

// ImGui backend headers
#include "../backends/imgui_impl_fltk.h"
#include "../backends/imgui_impl_opengl3.h"

PlotGLWindow::PlotGLWindow(int x, int y, int w, int h, const char* label)
    : Fl_Gl_Window(x, y, w, h, label), initialized(false) {
    // Set up proper OpenGL mode for modern usage
    mode(FL_RGB | FL_DOUBLE | FL_DEPTH | FL_OPENGL3);
    
    LOG_INFO("PlotGLWindow constructor", "PlotGLWindow");
}

PlotGLWindow::~PlotGLWindow() {
    LOG_INFO("PlotGLWindow destructor - cleaning up resources", "PlotGLWindow");
    
    if (initialized) {
        try {
            cleanupImGui();
        } catch (const std::exception& e) {
            LOG_ERR("Exception during ImGui cleanup: " + std::string(e.what()), "PlotGLWindow");
        }
    }
}

void PlotGLWindow::draw() {
    if (!valid()) {
        valid(1);
        glViewport(0, 0, w(), h());
        
        // Initialize ImGui if not already done
        if (!initialized) {
            initialized = initializeImGui();
            if (!initialized) {
                LOG_ERR("Failed to initialize ImGui/ImPlot", "PlotGLWindow");
            }
        }
    }
    
    // Clear the background
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Log OpenGL version for debugging
    if (!openGLVersionLogged) {
        const char* version = (const char*)glGetString(GL_VERSION);
        if (version) {
            LOG_INFO("OpenGL Version: " + std::string(version), "PlotGLWindow");
        } else {
            LOG_WARN("Could not get OpenGL version", "PlotGLWindow");
        }
        openGLVersionLogged = true;
    }
    
    // Draw a test rectangle to verify OpenGL is working
    drawTestRectangle();
    
    // Render the plot if initialized
    if (initialized) {
        try {
            // Use try-catch for each rendering step to provide better error reporting
            try {
                // Start new ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplFLTK_NewFrame();
                ImGui::NewFrame();
                
                // Create an ImGui window for our plot
                ImGui::Begin("##PlotWindow", nullptr, ImGuiWindowFlags_NoTitleBar | 
                                                     ImGuiWindowFlags_NoResize | 
                                                     ImGuiWindowFlags_NoScrollbar | 
                                                     ImGuiWindowFlags_NoCollapse);
                
                // Render the current plot
                renderPlot();
                
                // End the ImGui window
                ImGui::End();
                
                // Render
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            } catch (const std::exception& e) {
                LOG_ERR("Exception during ImGui rendering: " + std::string(e.what()), "PlotGLWindow");
                drawErrorMessage("ImGui rendering error: " + std::string(e.what()));
            }
        } catch (const std::exception& e) {
            LOG_ERR("Exception during plot rendering: " + std::string(e.what()), "PlotGLWindow");
            drawErrorMessage("Rendering error: " + std::string(e.what()));
        }
    } else {
        // Draw a message if not initialized
        drawErrorMessage("ImGui/ImPlot not initialized");
    }
    
    // We need to explicitly swap the buffers for OpenGL double-buffering
    swap_buffers();
}

bool PlotGLWindow::initializeImGui() {
    LOG_INFO("Initializing ImGui and ImPlot", "PlotGLWindow");
    
    try {
        // Ensure OpenGL context is current
        make_current();
        
        // Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        // Initialize ImPlot context
        ImPlot::CreateContext();
        
        // Configure ImGui IO and build fonts
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.Fonts->Build();  // Build fonts before backend init
        
        // Initialize ImGui OpenGL3 backend first
        if (!ImGui_ImplOpenGL3_Init("#version 120")) {
            LOG_ERR("Failed to initialize ImGui OpenGL3 backend", "PlotGLWindow");
            return false;
        }
        
        // Initialize ImGui FLTK backend second
        if (!ImGui_ImplFLTK_Init(window(), this)) {
            LOG_ERR("Failed to initialize ImGui FLTK backend", "PlotGLWindow");
            return false;
        }
        
        // Set up plot style with light theme
        ImGui::StyleColorsLight();
        ImPlot::StyleColorsLight();
        
        // Customize plot style
        ImPlot::GetStyle().LineWeight = 2.0f;
        ImPlot::GetStyle().MarkerSize = 6.0f;
        ImPlot::GetStyle().MarkerWeight = 2.0f;
        ImPlot::GetStyle().FillAlpha = 1.0f;
        
        LOG_INFO("ImGui and ImPlot initialization successful", "PlotGLWindow");
        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Exception during ImGui initialization: " + std::string(e.what()), "PlotGLWindow");
        return false;
    }
}

void PlotGLWindow::cleanupImGui() {
    LOG_INFO("Cleaning up ImGui and ImPlot resources", "PlotGLWindow");
    
    // End any in-progress ImGui frame
    if (ImGui::GetCurrentContext() && ImGui::GetFrameCount() > 0) {
        try {
            ImGui::EndFrame();
            ImGui::Render();
        } catch (const std::exception& e) {
            LOG_ERR("Error ending ImGui frame: " + std::string(e.what()), "PlotGLWindow");
        }
    }
    
    // Shutdown ImGui backends
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplFLTK_Shutdown();
    
    // Destroy ImPlot context
    if (ImPlot::GetCurrentContext()) {
        ImPlot::DestroyContext();
    }
    
    // Destroy ImGui context
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
    
    initialized = false;
}

void PlotGLWindow::createScatterPlot(
    const std::vector<double>& xValues,
    const std::vector<double>& yValues,
    const std::string& title,
    const std::string& xLabel,
    const std::string& yLabel
) {
    LOG_INFO("Creating scatter plot: " + title, "PlotGLWindow");
    
    if (xValues.size() != yValues.size()) {
        LOG_ERR("X and Y data must have the same size for scatter plots", "PlotGLWindow");
        return;
    }
    
    if (xValues.empty()) {
        LOG_ERR("Cannot create scatter plot with empty data", "PlotGLWindow");
        return;
    }
    
    // Store plot data
    this->plotTitle = title.empty() ? "Scatter Plot" : title;
    this->xLabel = xLabel;
    this->yLabel = yLabel;
    this->xValues = xValues;
    this->yValues = yValues;
    
    // Set plot type
    currentPlotType = PlotType::Scatter;
    
    LOG_INFO("Scatter plot created successfully", "PlotGLWindow");
    redraw();  // Request a redraw to show the new plot
}

void PlotGLWindow::createTimeSeriesPlot(
    const std::vector<double>& actual,
    const std::vector<double>& predicted,
    const std::string& title
) {
    LOG_INFO("Creating time series plot: " + title, "PlotGLWindow");
    
    if (actual.size() != predicted.size()) {
        LOG_ERR("Actual and predicted data must have the same size for time series plots", "PlotGLWindow");
        return;
    }
    
    if (actual.empty()) {
        LOG_ERR("Cannot create time series plot with empty data", "PlotGLWindow");
        return;
    }
    
    // Create x values as indices
    std::vector<double> indices(actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        indices[i] = static_cast<double>(i);
    }
    
    // Store plot data
    this->plotTitle = title.empty() ? "Time Series Plot" : title;
    this->xLabel = "Time";
    this->yLabel = "Value";
    this->xValues = indices;
    this->yValues = actual;
    this->y2Values = predicted;
    
    // Set plot type
    currentPlotType = PlotType::TimeSeries;
    
    LOG_INFO("Time series plot created successfully", "PlotGLWindow");
    redraw();  // Request a redraw to show the new plot
}

void PlotGLWindow::createResidualPlot(
    const std::vector<double>& predicted,
    const std::vector<double>& residuals,
    const std::string& title
) {
    LOG_INFO("Creating residual plot: " + title, "PlotGLWindow");
    
    if (predicted.size() != residuals.size()) {
        LOG_ERR("Predicted and residual data must have the same size", "PlotGLWindow");
        return;
    }
    
    if (predicted.empty()) {
        LOG_ERR("Cannot create residual plot with empty data", "PlotGLWindow");
        return;
    }
    
    // Store plot data
    this->plotTitle = title.empty() ? "Residual Plot" : title;
    this->xLabel = "Predicted";
    this->yLabel = "Residual";
    this->xValues = predicted;
    this->yValues = residuals;
    
    // Set plot type
    currentPlotType = PlotType::Residual;
    
    LOG_INFO("Residual plot created successfully", "PlotGLWindow");
    redraw();  // Request a redraw to show the new plot
}

void PlotGLWindow::createImportancePlot(
    const std::unordered_map<std::string, double>& importance,
    const std::string& title
) {
    LOG_INFO("Creating importance plot: " + title, "PlotGLWindow");
    
    if (importance.empty()) {
        LOG_ERR("Cannot create importance plot with empty data", "PlotGLWindow");
        return;
    }
    
    // Store plot data
    this->plotTitle = title.empty() ? "Feature Importance" : title;
    this->xLabel = "Importance";
    this->yLabel = "Feature";
    this->importanceValues = importance;
    
    // Set plot type
    currentPlotType = PlotType::Importance;
    
    LOG_INFO("Importance plot created successfully", "PlotGLWindow");
    redraw();  // Request a redraw to show the new plot
}

void PlotGLWindow::createLearningCurvePlot(
    const std::vector<double>& trainingSizes,
    const std::vector<double>& trainingScores,
    const std::vector<double>& validationScores,
    const std::string& title
) {
    LOG_INFO("Creating learning curve plot: " + title, "PlotGLWindow");
    
    if (trainingSizes.empty() || trainingScores.empty() || validationScores.empty()) {
        LOG_ERR("Cannot create learning curve plot with empty data", "PlotGLWindow");
        return;
    }
    
    if (trainingSizes.size() != trainingScores.size() || trainingSizes.size() != validationScores.size()) {
        LOG_ERR("Training sizes, training scores, and validation scores must have the same length", "PlotGLWindow");
        return;
    }
    
    // Store plot data
    this->plotTitle = title.empty() ? "Learning Curve" : title;
    this->xLabel = "Training Examples";
    this->yLabel = "Score";
    this->trainingSizes = trainingSizes;
    this->trainingScores = trainingScores;
    this->validationScores = validationScores;
    
    // Set plot type
    currentPlotType = PlotType::LearningCurve;
    
    LOG_INFO("Learning curve plot created successfully", "PlotGLWindow");
    redraw();  // Request a redraw to show the new plot
}

void PlotGLWindow::renderPlot() {
    LOG_DEBUG("Rendering plot of type: " + std::to_string(static_cast<int>(currentPlotType)), "PlotGLWindow");
    
    // Display the current plot title at the top
    ImGui::Text("%s", plotTitle.c_str());
    ImGui::Separator();
    
    // Render the appropriate plot based on the current plot type
    switch (currentPlotType) {
        case PlotType::Scatter:
            renderScatterPlot();
            break;
        case PlotType::TimeSeries:
            renderTimeSeriesPlot();
            break;
        case PlotType::Residual:
            renderResidualPlot();
            break;
        case PlotType::Importance:
            renderImportancePlot();
            break;
        case PlotType::LearningCurve:
            renderLearningCurvePlot();
            break;
        default:
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown plot type");
            break;
    }
}

void PlotGLWindow::renderScatterPlot() {
    LOG_DEBUG("Rendering scatter plot", "PlotGLWindow");
    
    try {
        // Set up plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 6.0f);
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        ImPlotAxisFlags axisFlags = ImPlotAxisFlags_AutoFit;
        
        // Begin plot with explicit size - use nearly the full available area
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty (ImPlot requires a valid, non-empty ID)
        std::string plotID = !plotTitle.empty() ? plotTitle : "Scatter Plot";
        
        if (ImPlot::BeginPlot(plotID.c_str(), plotSize, flags)) {
            // Set up axes with labels
            ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, yLabel.c_str(), axisFlags);
            
            // Add scatter plot points
            if (!xValues.empty() && !yValues.empty() && xValues.size() == yValues.size()) {
                // Set marker style for data points
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 6.0f, ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 2.0f);
                
                // Plot scatter points
                ImPlot::PlotScatter("Data Points", 
                                   xValues.data(), 
                                   yValues.data(), 
                                   static_cast<int>(xValues.size()));
                
                // Add identity line (y=x)
                double minX = *std::min_element(xValues.begin(), xValues.end());
                double maxX = *std::max_element(xValues.begin(), xValues.end());
                double minY = *std::min_element(yValues.begin(), yValues.end());
                double maxY = *std::max_element(yValues.begin(), yValues.end());
                
                double min = std::min(minX, minY);
                double max = std::max(maxX, maxY);
                
                std::vector<double> lineX = {min, max};
                std::vector<double> lineY = {min, max};
                
                // Set line style for identity line - dark gray
                ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), 2.0f);
                ImPlot::PlotLine("y=x", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No data to plot");
            }
            
            ImPlot::EndPlot();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create plot");
        }
        
        ImPlot::PopStyleVar(2);
    } catch (const std::exception& e) {
        LOG_ERR("Exception during scatter plot rendering: " + std::string(e.what()), "PlotGLWindow");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlotGLWindow::renderTimeSeriesPlot() {
    LOG_DEBUG("Rendering time series plot", "PlotGLWindow");
    
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
        std::string plotID = !plotTitle.empty() ? plotTitle : "Time Series Plot";
        
        if (ImPlot::BeginPlot(plotID.c_str(), plotSize, flags)) {
            ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, yLabel.c_str(), axisFlags);
            
            // Add actual values line
            if (!xValues.empty() && !yValues.empty() && xValues.size() == yValues.size()) {
                ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 2.0f);
                ImPlot::PlotLine("Actual", 
                                xValues.data(), 
                                yValues.data(), 
                                static_cast<int>(xValues.size()));
            }
            
            // Add predicted values line
            if (!xValues.empty() && !y2Values.empty() && xValues.size() == y2Values.size()) {
                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), 2.0f);
                ImPlot::PlotLine("Predicted", 
                                xValues.data(), 
                                y2Values.data(), 
                                static_cast<int>(xValues.size()));
            }
            
            ImPlot::EndPlot();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create time series plot");
        }
        
        ImPlot::PopStyleVar();
    } catch (const std::exception& e) {
        LOG_ERR("Exception during time series plot rendering: " + std::string(e.what()), "PlotGLWindow");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlotGLWindow::renderResidualPlot() {
    LOG_DEBUG("Rendering residual plot", "PlotGLWindow");
    
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
        std::string plotID = !plotTitle.empty() ? plotTitle : "Residual Plot";
        
        if (ImPlot::BeginPlot(plotID.c_str(), plotSize, flags)) {
            ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, yLabel.c_str(), axisFlags);
            
            // Add residual scatter plot
            if (!xValues.empty() && !yValues.empty() && xValues.size() == yValues.size()) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4.0f, ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 1.5f);
                ImPlot::PlotScatter("Residuals", 
                                   xValues.data(), 
                                   yValues.data(), 
                                   static_cast<int>(xValues.size()));
                
                // Add zero line
                double minX = *std::min_element(xValues.begin(), xValues.end());
                double maxX = *std::max_element(xValues.begin(), xValues.end());
                std::vector<double> lineX = {minX, maxX};
                std::vector<double> lineY = {0.0, 0.0};
                
                ImPlot::SetNextLineStyle(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), 1.5f);
                ImPlot::PlotLine("Zero", lineX.data(), lineY.data(), static_cast<int>(lineX.size()));
            }
            
            ImPlot::EndPlot();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create residual plot");
        }
        
        ImPlot::PopStyleVar(2);
    } catch (const std::exception& e) {
        LOG_ERR("Exception during residual plot rendering: " + std::string(e.what()), "PlotGLWindow");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlotGLWindow::renderImportancePlot() {
    LOG_DEBUG("Rendering importance plot", "PlotGLWindow");
    
    try {
        // Set plot styling
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(10, 10));
        
        // Set plot flags
        ImPlotFlags flags = ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus;
        
        // Get available size and set plot dimensions
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 plotSize(availSize.x * 0.95f, availSize.y * 0.75f);
        
        // Ensure plot title is not empty
        std::string plotID = !plotTitle.empty() ? plotTitle : "Feature Importance";
        
        if (ImPlot::BeginPlot(plotID.c_str(), plotSize, flags)) {
            // For feature importance, we create a horizontal bar chart
            ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str(), ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxis(ImAxis_Y1, yLabel.c_str(), ImPlotAxisFlags_AutoFit);
            
            if (!importanceValues.empty()) {
                // Convert unordered_map to vectors and sort by importance
                std::vector<std::pair<std::string, double>> sortedImportance;
                for (const auto& pair : importanceValues) {
                    sortedImportance.push_back(pair);
                }
                
                std::sort(sortedImportance.begin(), sortedImportance.end(),
                        [](const auto& a, const auto& b) {
                            return a.second > b.second;
                        });
                
                // Prepare data for plotting
                std::vector<double> values;
                std::vector<const char*> labels;
                
                for (const auto& pair : sortedImportance) {
                    values.push_back(pair.second);
                    labels.push_back(pair.first.c_str());
                }
                
                // Bar chart
                ImPlot::SetNextFillStyle(ImVec4(0.0f, 0.45f, 0.85f, 0.8f));
                ImPlot::PlotBars("Importance", values.data(), static_cast<int>(values.size()), 0.75, 0.0, ImPlotBarsFlags_Horizontal);
            }
            
            ImPlot::EndPlot();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create importance plot");
        }
        
        ImPlot::PopStyleVar();
    } catch (const std::exception& e) {
        LOG_ERR("Exception during importance plot rendering: " + std::string(e.what()), "PlotGLWindow");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlotGLWindow::renderLearningCurvePlot() {
    LOG_DEBUG("Rendering learning curve plot", "PlotGLWindow");
    
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
        std::string plotID = !plotTitle.empty() ? plotTitle : "Learning Curve";
        
        if (ImPlot::BeginPlot(plotID.c_str(), plotSize, flags)) {
            ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str(), axisFlags);
            ImPlot::SetupAxis(ImAxis_Y1, yLabel.c_str(), axisFlags);
            
            // Add training score line
            if (!trainingSizes.empty() && !trainingScores.empty() && 
                trainingSizes.size() == trainingScores.size()) {
                ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.45f, 0.85f, 1.0f), 2.0f);
                ImPlot::PlotLine("Training Score", 
                               trainingSizes.data(), 
                               trainingScores.data(), 
                               static_cast<int>(trainingSizes.size()));
            }
            
            // Add validation score line
            if (!trainingSizes.empty() && !validationScores.empty() && 
                trainingSizes.size() == validationScores.size()) {
                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), 2.0f);
                ImPlot::PlotLine("Validation Score", 
                               trainingSizes.data(), 
                               validationScores.data(), 
                               static_cast<int>(trainingSizes.size()));
            }
            
            ImPlot::EndPlot();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create learning curve plot");
        }
        
        ImPlot::PopStyleVar();
    } catch (const std::exception& e) {
        LOG_ERR("Exception during learning curve plot rendering: " + std::string(e.what()), "PlotGLWindow");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }
}

void PlotGLWindow::drawErrorMessage(const std::string& message) {
    // Draw a red rectangle with text for error messages
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(10.0f, 10.0f);
    glVertex2f(w() - 10.0f, 10.0f);
    glVertex2f(w() - 10.0f, 50.0f);
    glVertex2f(10.0f, 50.0f);
    glEnd();
    
    // We can't easily draw text with raw OpenGL, so we just draw the rectangle
    // The error message is logged but not displayed
    LOG_ERR("Error message: " + message, "PlotGLWindow");
}

void PlotGLWindow::drawTestRectangle() {
    // Draw a red test rectangle to verify OpenGL is working
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w(), h(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glColor3f(1.0f, 0.0f, 0.0f);  // Red color
    glBegin(GL_QUADS);
    glVertex2f(50.0f, 50.0f);
    glVertex2f(150.0f, 50.0f);
    glVertex2f(150.0f, 150.0f);
    glVertex2f(50.0f, 150.0f);
    glEnd();
    
    // Check if rectangle drawing had errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERR("OpenGL error after drawing rectangle: " + std::to_string(err), "PlotGLWindow");
    }
} 