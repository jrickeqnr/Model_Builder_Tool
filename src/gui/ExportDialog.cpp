#include "gui/ExportDialog.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>
#include "utils/Logger.h"

ExportDialog::ExportDialog(PlotGLWindow::PlotType plotType)
    : Fl_Window(400, 250, "Export Plot"),
      confirmed(false),
      filename("plot.png"),
      width(800),
      height(600),
      plotType(plotType)
{
    initialize();
}

ExportDialog::~ExportDialog()
{
    LOG_INFO("ExportDialog destructor called", "ExportDialog");
}

void ExportDialog::initialize()
{
    LOG_INFO("Initializing ExportDialog", "ExportDialog");
    
    begin();
    
    // Create filename input
    filenameInput = new Fl_Input(100, 20, 200, 25, "Filename:");
    filenameInput->value(filename.c_str());
    
    // Create browse button
    browseButton = new Fl_Button(310, 20, 70, 25, "Browse");
    browseButton->callback(browseCallback, this);
    
    // Create width input
    widthInput = new Fl_Int_Input(100, 60, 100, 25, "Width:");
    widthInput->value(std::to_string(width).c_str());
    
    // Create height input
    heightInput = new Fl_Int_Input(100, 100, 100, 25, "Height:");
    heightInput->value(std::to_string(height).c_str());
    
    // Create OK button
    okButton = new Fl_Button(220, 200, 70, 30, "OK");
    okButton->callback(okCallback, this);
    
    // Create Cancel button
    cancelButton = new Fl_Button(300, 200, 70, 30, "Cancel");
    cancelButton->callback(cancelCallback, this);
    
    end();
    
    set_modal();
}

void ExportDialog::okCallback(Fl_Widget* w, void* v)
{
    ExportDialog* dialog = static_cast<ExportDialog*>(v);
    if (dialog) {
        LOG_INFO("OK button clicked in ExportDialog", "ExportDialog");
        
        // Get values from inputs
        dialog->filename = dialog->filenameInput->value();
        try {
            dialog->width = std::stoi(dialog->widthInput->value());
            dialog->height = std::stoi(dialog->heightInput->value());
        } catch (const std::exception& e) {
            LOG_ERR("Invalid dimensions: " + std::string(e.what()), "ExportDialog");
            fl_alert("Please enter valid dimensions");
            return;
        }
        
        // Validate dimensions
        if (dialog->width <= 0 || dialog->height <= 0) {
            LOG_ERR("Invalid dimensions: width and height must be positive", "ExportDialog");
            fl_alert("Width and height must be positive numbers");
            return;
        }
        
        dialog->confirmed = true;
        dialog->hide();
    }
}

void ExportDialog::cancelCallback(Fl_Widget* w, void* v)
{
    ExportDialog* dialog = static_cast<ExportDialog*>(v);
    if (dialog) {
        LOG_INFO("Cancel button clicked in ExportDialog", "ExportDialog");
        dialog->confirmed = false;
        dialog->hide();
    }
}

void ExportDialog::browseCallback(Fl_Widget* w, void* v)
{
    ExportDialog* dialog = static_cast<ExportDialog*>(v);
    if (dialog) {
        LOG_INFO("Browse button clicked in ExportDialog", "ExportDialog");
        
        Fl_Native_File_Chooser chooser;
        chooser.title("Save Plot As");
        chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
        chooser.filter("PNG Files\t*.png\n");
        chooser.preset_file(dialog->filename.c_str());
        
        if (chooser.show() == 0) {
            dialog->filename = chooser.filename();
            dialog->filenameInput->value(dialog->filename.c_str());
            LOG_INFO("Selected file: " + dialog->filename, "ExportDialog");
        }
    }
} 