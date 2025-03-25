#include "gui/ExportDialog.h"
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <sstream>
#include <iomanip>

ExportDialog::ExportDialog(int w, int h, const char* title)
    : Fl_Window(w, h, title)
{
    begin();

    // Set window properties
    set_modal();  // Make it modal
    position(Fl::w()/2 - w/2, Fl::h()/2 - h/2);  // Center the window

    int padding = 10;
    int checkboxWidth = w - 2 * padding;
    int checkboxHeight = 25;
    int y = padding;

    // Create checkboxes for export options
    scatterPlotCheck = new Fl_Check_Button(padding, y, checkboxWidth, checkboxHeight, "Scatter Plot");
    scatterPlotCheck->value(1); // Default to checked
    y += checkboxHeight + padding;

    linePlotCheck = new Fl_Check_Button(padding, y, checkboxWidth, checkboxHeight, "Line Plot");
    linePlotCheck->value(1);
    y += checkboxHeight + padding;

    importancePlotCheck = new Fl_Check_Button(padding, y, checkboxWidth, checkboxHeight, "Variable Importance Plot");
    importancePlotCheck->value(1);
    y += checkboxHeight + padding;

    predictedValuesCheck = new Fl_Check_Button(padding, y, checkboxWidth, checkboxHeight, "Predicted Values (CSV)");
    predictedValuesCheck->value(1);
    y += checkboxHeight + padding;

    modelSummaryCheck = new Fl_Check_Button(padding, y, checkboxWidth, checkboxHeight, "Model Summary (TXT)");
    modelSummaryCheck->value(1);
    y += checkboxHeight + padding;

    // Create path selection components
    pathDisplay = new Fl_Box(padding, y, w - 3 * padding - 80, checkboxHeight, "No directory selected");
    pathDisplay->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    pathDisplay->box(FL_DOWN_BOX);

    browseButton = new Fl_Button(w - padding - 80, y, 80, checkboxHeight, "Browse");
    browseButton->callback(browseCallback, this);
    y += checkboxHeight + padding;

    // Create action buttons
    int buttonWidth = 80;
    int buttonSpacing = (w - 2 * padding - 2 * buttonWidth) / 3;
    
    exportButton = new Fl_Button(padding + buttonSpacing, y, buttonWidth, checkboxHeight, "Export");
    exportButton->callback(exportCallback, this);
    exportButton->deactivate(); // Disabled until directory is selected

    cancelButton = new Fl_Button(padding + buttonWidth + 2 * buttonSpacing, y, buttonWidth, checkboxHeight, "Cancel");
    cancelButton->callback(cancelCallback, this);

    end();
}

ExportDialog::~ExportDialog() {
    // FLTK will handle widget cleanup
}

void ExportDialog::setModel(std::shared_ptr<Model> model) {
    currentModel = model;
}

ExportDialog::ExportOptions ExportDialog::getExportOptions() const {
    ExportOptions options;
    // Set new fields for compatibility with updated ResultsView
    options.exportSummary = modelSummaryCheck->value();
    options.exportCSV = predictedValuesCheck->value();
    options.exportPlots = scatterPlotCheck->value() || linePlotCheck->value() || importancePlotCheck->value();
    options.outputDir = selectedPath;
    
    // Set legacy fields for backward compatibility
    options.scatterPlot = scatterPlotCheck->value();
    options.linePlot = linePlotCheck->value();
    options.importancePlot = importancePlotCheck->value();
    options.predictedValues = predictedValuesCheck->value();
    options.modelSummary = modelSummaryCheck->value();
    options.exportPath = selectedPath;
    
    return options;
}

void ExportDialog::browseCallback(Fl_Widget*, void* v) {
    ExportDialog* dialog = static_cast<ExportDialog*>(v);
    
    const char* dir = fl_dir_chooser("Select Export Directory", "", 0);
    if (dir) {
        dialog->selectedPath = dir;
        dialog->pathDisplay->copy_label(dir);
        dialog->exportButton->activate();
    }
}

void ExportDialog::exportCallback(Fl_Widget*, void* v) {
    ExportDialog* dialog = static_cast<ExportDialog*>(v);
    
    if (dialog->selectedPath.empty()) {
        fl_alert("Please select a directory first!");
        return;
    }

    dialog->createDirectory();
    
    if (dialog->onExport) {
        dialog->onExport(dialog->getExportOptions());
    }
    
    dialog->hide();
}

void ExportDialog::cancelCallback(Fl_Widget*, void* v) {
    ExportDialog* dialog = static_cast<ExportDialog*>(v);
    dialog->hide();
}

std::string ExportDialog::generateExportPath() const {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S");
    
    std::string modelName = currentModel ? currentModel->getName() : "unknown_model";
    return modelName + "_" + ss.str();
}

void ExportDialog::createDirectory() {
    if (!selectedPath.empty()) {
        std::filesystem::path basePath(selectedPath);
        std::filesystem::path exportDir = basePath / generateExportPath();
        
        try {
            std::filesystem::create_directories(exportDir);
            selectedPath = exportDir.string();
        } catch (const std::filesystem::filesystem_error& e) {
            fl_alert("Failed to create export directory: %s", e.what());
        }
    }
} 