#include "gui/ResultsView.h"
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

// DataTable implementation
DataTable::DataTable(int x, int y, int w, int h, const char* label)
    : Fl_Table(x, y, w, h, label) 
{
    // Setup the table
    rows(0);
    row_header(0);
    row_height_all(25);
    row_resize(0);
    
    cols(2);
    col_header(1);
    col_width(0, w/2);
    col_width(1, w/2);
    col_resize(1);
    
    end();
}

DataTable::~DataTable() {
    // No explicit cleanup needed
}

void DataTable::setData(const std::unordered_map<std::string, double>& data) {
    // Clear existing data
    names.clear();
    values.clear();
    
    // Copy data to vectors for table display
    for (const auto& pair : data) {
        names.push_back(pair.first);
        values.push_back(pair.second);
    }
    
    // Update table rows
    rows(names.size());
    row_height_all(25);
    
    // Force redraw
    redraw();
}

void DataTable::draw_cell(TableContext context, int row, int col, int x, int y, int w, int h) {
    switch (context) {
        case CONTEXT_CELL: {
            // Draw cell content
            fl_push_clip(x, y, w, h);
            fl_color(FL_WHITE);
            fl_rectf(x, y, w, h);
            fl_color(FL_GRAY0);
            fl_rect(x, y, w, h);
            
            fl_color(FL_BLACK);
            fl_font(FL_HELVETICA, 14);
            if (col == 0 && row < names.size()) {
                fl_draw(names[row].c_str(), x + 5, y, w - 10, h, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            } else if (col == 1 && row < values.size()) {
                char valStr[50];
                snprintf(valStr, sizeof(valStr), "%.6f", values[row]);
                fl_draw(valStr, x + 5, y, w - 10, h, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
            }
            fl_pop_clip();
            break;
        }
        
        case CONTEXT_COL_HEADER: {
            // Draw column headers
            fl_push_clip(x, y, w, h);
            fl_draw_box(FL_THIN_UP_BOX, x, y, w, h, FL_BACKGROUND_COLOR);
            fl_color(FL_BLACK);
            fl_font(FL_HELVETICA_BOLD, 14);
            if (col == 0) {
                fl_draw("Parameter", x + 5, y, w - 10, h, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            } else {
                fl_draw("Value", x + 5, y, w - 10, h, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
            }
            fl_pop_clip();
            break;
        }
        
        default:
            break;
    }
}

// PlotWidget implementation
PlotWidget::PlotWidget(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), plotImageData(nullptr), plotImageWidth(0), plotImageHeight(0)
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

void PlotWidget::createScatterPlot(const std::vector<double>& actualValues,
        const std::vector<double>& predictedValues,
        const std::string& xLabel,
        const std::string& yLabel,
        const std::string& title) {
    // Create temporary file paths
    std::string tempDataPath = "temp_plot_data.csv";
    std::string tempImagePath = "temp_plot_image.png"; // Changed from .ppm to .png
    std::string tempScriptPath = "temp_plot_script.py";

    // Write data to temporary file
    std::ofstream dataFile(tempDataPath);
    if (!dataFile.is_open()) {
    fl_alert("Failed to create temporary data file");
    return;
    }

    dataFile << "actual,predicted\n";
    for (size_t i = 0; i < std::min(actualValues.size(), predictedValues.size()); ++i) {
    dataFile << actualValues[i] << "," << predictedValues[i] << "\n";
    }
    dataFile.close();

    // Create plotting script
    std::ofstream scriptFile(tempScriptPath);
    if (!scriptFile.is_open()) {
    fl_alert("Failed to create temporary script file");
    return;
    }

    scriptFile << "import matplotlib.pyplot as plt\n";
    scriptFile << "import pandas as pd\n";
    scriptFile << "import numpy as np\n\n";

    // Read data
    scriptFile << "# Read data\n";
    scriptFile << "data = pd.read_csv('" << tempDataPath << "')\n";
    scriptFile << "actual = data['actual']\n";
    scriptFile << "predicted = data['predicted']\n\n";

    // Create plot
    scriptFile << "# Create plot\n";
    scriptFile << "plt.figure(figsize=(8, 6), dpi=100)\n";
    scriptFile << "plt.scatter(actual, predicted, alpha=0.7)\n\n";

    // Add perfect prediction line
    scriptFile << "# Add perfect prediction line\n";
    scriptFile << "min_val = min(actual.min(), predicted.min())\n";
    scriptFile << "max_val = max(actual.max(), predicted.max())\n";
    scriptFile << "plt.plot([min_val, max_val], [min_val, max_val], 'r--')\n\n";

    // Set labels and title
    scriptFile << "# Set labels and title\n";
    scriptFile << "plt.xlabel('" << xLabel << "')\n";
    scriptFile << "plt.ylabel('" << yLabel << "')\n";
    scriptFile << "plt.title('" << title << "')\n";
    scriptFile << "plt.grid(True, linestyle='--', alpha=0.7)\n\n";

    // Save plot - using PNG format instead of PPM
    scriptFile << "# Save plot\n";
    scriptFile << "plt.tight_layout()\n";
    scriptFile << "plt.savefig('" << tempImagePath << "', format='png')\n"; // Changed to png
    scriptFile << "print(f'Plot saved as: {'" << tempImagePath << "'}')\n";
    scriptFile << "plt.close()\n";

    scriptFile.close();

    // Execute Python script
    std::string command = "python " + tempScriptPath;
    int result = std::system(command.c_str());

    if (result == 0) {
    // Load the generated PNG image
    Fl_PNG_Image* pngImage = new Fl_PNG_Image(tempImagePath.c_str());

    if (pngImage && pngImage->w() > 0 && pngImage->h() > 0) {
    // Clean up old image data if it exists
    if (plotImageData) {
    delete[] plotImageData;
    plotImageData = nullptr;
    }

    // Update image dimensions
    plotImageWidth = pngImage->w();
    plotImageHeight = pngImage->h();

    // Copy image data
    plotImageData = new char[plotImageWidth * plotImageHeight * 3];

    // Access the RGB data from the Fl_PNG_Image
    const uchar* imageData = reinterpret_cast<const uchar*>(pngImage->data()[0]);
    int depth = pngImage->d();

    // Copy the image data with appropriate conversion
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

    // Update the plotBox if it exists
    if (plotBox) {
    plotBox->redraw();
    }

    // Redraw the widget
    redraw();
    } else {
    fl_alert("Failed to load the generated plot image");
    }
    } else {
    fl_alert("Failed to generate plot. Check if Python and matplotlib are installed.");
    }

    // Clean up temporary files
    std::remove(tempScriptPath.c_str());
    std::remove(tempDataPath.c_str());
    // Don't delete the image immediately, as it might still be in use
    // Optionally add code to delete it when the app closes
    std::remove(tempImagePath.c_str());
}

bool PlotWidget::generatePlot(const std::string& command) {
    // This method is deprecated in favor of createScatterPlot
    return false;
}

// ResultsView implementation
ResultsView::ResultsView(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h)
{
    begin();
    
    // Set group properties
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
    
    // Constants for layout
    int margin = 20;
    int headerHeight = 40;
    int bottomButtonsHeight = 40;
    
    // Create title label
    modelTitleLabel = new Fl_Box(x + margin, y + margin, w - 2*margin, headerHeight, "Model Results");
    modelTitleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    modelTitleLabel->labelsize(18);
    modelTitleLabel->labelfont(FL_BOLD);
    
    // Create main display area
    int contentY = y + margin + headerHeight + 10;
    int contentHeight = h - margin*2 - headerHeight - 10 - bottomButtonsHeight - 10;
    int tableWidth = (w - margin*3) / 2;
    
    // Parameters group (left side)
    parametersGroup = new Fl_Group(x + margin, contentY, tableWidth, contentHeight/2);
    parametersGroup->box(FL_BORDER_BOX);
    parametersGroup->begin();
    
    Fl_Box* parametersLabel = new Fl_Box(x + margin + 10, contentY + 10, tableWidth - 20, 30, "Model Parameters");
    parametersLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    parametersLabel->labelsize(14);
    parametersLabel->labelfont(FL_BOLD);
    
    DataTable* paramsTable = new DataTable(
        x + margin + 10, contentY + 50, 
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
    
    DataTable* statsTable = new DataTable(
        x + margin + 10, contentY + contentHeight/2 + 60, 
        tableWidth - 20, contentHeight/2 - 70);
    
    statisticsGroup->end();
    
    // Plot group (right side)
    plotWidget = new PlotWidget(x + margin*2 + tableWidth, contentY, tableWidth, contentHeight);
    
    // Create bottom buttons
    int buttonY = y + h - margin - bottomButtonsHeight;
    
    backButton = new Fl_Button(x + margin, buttonY, 100, bottomButtonsHeight, "Back");
    backButton->callback(backButtonCallback_static, this);
    
    exportButton = new Fl_Button(x + w - margin - 150, buttonY, 150, bottomButtonsHeight, "Export Results");
    exportButton->callback(exportButtonCallback_static, this);
    
    end();
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

void ResultsView::updateResults() {
    if (!model || !dataFrame) {
        return;
    }
    
    // Update model title
    modelTitleLabel->copy_label((model->getName() + " Model Results").c_str());
    
    // Update parameters and statistics
    updateParametersDisplay();
    updateStatisticsDisplay();
    
    // Create visualization
    createScatterPlot();
    
    // Redraw the widget
    redraw();
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
    exportResults();
}

void ResultsView::updateParametersDisplay() {
    if (!model) {
        return;
    }
    
    // Get parameters table
    Fl_Group* group = parametersGroup;
    DataTable* table = nullptr;
    
    // Find the DataTable in the group
    for (int i = 0; i < group->children(); i++) {
        if (dynamic_cast<DataTable*>(group->child(i))) {
            table = dynamic_cast<DataTable*>(group->child(i));
            break;
        }
    }
    
    if (!table) {
        return;
    }
    
    // Update table with parameter values
    table->setData(model->getParameters());
}

void ResultsView::updateStatisticsDisplay() {
    if (!model) {
        return;
    }
    
    // Get statistics table
    Fl_Group* group = statisticsGroup;
    DataTable* table = nullptr;
    
    // Find the DataTable in the group
    for (int i = 0; i < group->children(); i++) {
        if (dynamic_cast<DataTable*>(group->child(i))) {
            table = dynamic_cast<DataTable*>(group->child(i));
            break;
        }
    }
    
    if (!table) {
        return;
    }
    
    // Update table with statistic values
    table->setData(model->getStatistics());
}

void ResultsView::createScatterPlot() {
    if (!model || !dataFrame) {
        return;
    }
    
    // Get actual values
    std::vector<double> actualValues = dataFrame->getColumn(targetVariable);
    
    // Get input data for prediction
    Eigen::MatrixXd X = dataFrame->toMatrix(inputVariables);
    
    // Get predicted values
    Eigen::VectorXd predictedValuesEigen = model->predict(X);
    
    // Convert to std::vector
    std::vector<double> predictedValues(predictedValuesEigen.data(), 
                                      predictedValuesEigen.data() + predictedValuesEigen.size());
    
    // Create scatter plot
    plotWidget->createScatterPlot(actualValues, predictedValues,
                                "Actual Values", "Predicted Values",
                                "Actual vs. Predicted Values");
}

void ResultsView::exportResults() {
    if (!model || !dataFrame) {
        return;
    }
    
    // Show file dialog to get save location
    const char* fileName = fl_file_chooser("Export Results", "*.txt", "regression_results.txt");
    if (!fileName) {
        return;
    }
    
    try {
        // Open file for writing
        std::ofstream file(fileName);
        
        if (!file.is_open()) {
            fl_alert("Failed to open file for writing");
            return;
        }
        
        // Write model information
        file << "Model: " << model->getName() << std::endl;
        file << "Description: " << model->getDescription() << std::endl;
        file << std::endl;
        
        // Write parameters
        file << "Model Parameters:" << std::endl;
        file << "----------------" << std::endl;
        auto parameters = model->getParameters();
        for (const auto& param : parameters) {
            file << param.first << ": " << param.second << std::endl;
        }
        file << std::endl;
        
        // Write statistics
        file << "Model Statistics:" << std::endl;
        file << "---------------" << std::endl;
        auto statistics = model->getStatistics();
        for (const auto& stat : statistics) {
            file << stat.first << ": " << stat.second << std::endl;
        }
        file << std::endl;
        
        // Write variable information
        file << "Variables:" << std::endl;
        file << "----------" << std::endl;
        file << "Target Variable: " << targetVariable << std::endl;
        file << "Input Variables: ";
        for (size_t i = 0; i < inputVariables.size(); ++i) {
            file << inputVariables[i];
            if (i < inputVariables.size() - 1) {
                file << ", ";
            }
        }
        file << std::endl;
        
        // Close file
        file.close();
        
        fl_message("Results exported successfully");
    } catch (const std::exception& e) {
        fl_alert("Failed to export results: %s", e.what());
    }
}