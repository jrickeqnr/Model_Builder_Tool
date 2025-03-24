#pragma once

#include <FL/Fl_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <filesystem>

#include "models/Model.h"

class ExportDialog : public Fl_Window {
public:
    struct ExportOptions {
        bool exportSummary = false;
        bool exportCSV = false;
        bool exportPlots = false;
        std::string outputDir;
        
        // For backward compatibility
        bool scatterPlot = false;
        bool linePlot = false;
        bool importancePlot = false;
        bool predictedValues = false;
        bool modelSummary = false;
        std::string exportPath;
        
        // Helper method to get string representation
        std::string toString() const {
            std::string result = "ExportOptions{";
            result += "exportSummary=" + std::string(exportSummary ? "true" : "false");
            result += ", exportCSV=" + std::string(exportCSV ? "true" : "false");
            result += ", exportPlots=" + std::string(exportPlots ? "true" : "false");
            result += ", outputDir='" + outputDir + "'";
            result += "}";
            return result;
        }
    };

    ExportDialog();
    ExportDialog(int w, int h, const char* title);
    ~ExportDialog();

    void setModel(std::shared_ptr<Model> model);
    ExportOptions getExportOptions() const;
    
    // Callback for when export is confirmed
    std::function<void(const ExportOptions&)> onExport;

private:
    static void browseCallback(Fl_Widget* w, void* v);
    static void exportCallback(Fl_Widget* w, void* v);
    static void cancelCallback(Fl_Widget* w, void* v);

    void createDirectory();
    std::string generateExportPath() const;
    void initialize();

    // UI Elements
    Fl_Check_Button* scatterPlotCheck;
    Fl_Check_Button* linePlotCheck;
    Fl_Check_Button* importancePlotCheck;
    Fl_Check_Button* predictedValuesCheck;
    Fl_Check_Button* modelSummaryCheck;
    Fl_Button* browseButton;
    Fl_Box* pathDisplay;
    Fl_Button* exportButton;
    Fl_Button* cancelButton;

    std::string selectedPath;
    std::shared_ptr<Model> currentModel;
}; 