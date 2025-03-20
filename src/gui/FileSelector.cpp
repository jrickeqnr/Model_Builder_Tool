#include "gui/FileSelector.h"
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_draw.H>

FileSelector::FileSelector(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h) 
{
    begin();
    
    // Create description box
    int margin = 20;
    descriptionBox = new Fl_Box(x + margin, y + margin, w - 2*margin, 60);
    descriptionBox->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_WRAP);
    descriptionBox->label("Select a CSV file containing the data you want to analyze. "
                         "The file should have headers in the first row and contain "
                         "numeric columns suitable for regression analysis.");
    
    // Create file input field
    filePathInput = new Fl_Input(x + margin + 90, y + margin + 80, w - 2*margin - 90 - 100, 30, "CSV File:");
    filePathInput->align(FL_ALIGN_LEFT);
    filePathInput->readonly(1);
    
    // Create browse button
    browseButton = new Fl_Button(x + w - margin - 100, y + margin + 80, 100, 30, "Browse...");
    browseButton->callback(browseButtonCallback, this);
    
    // Create load button
    loadButton = new Fl_Button(x + w - margin - 120, y + h - margin - 40, 120, 40, "Load File");
    loadButton->callback(loadButtonCallback, this);
    loadButton->deactivate();  // Disable until a file is selected
    
    end();
    
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
}

void FileSelector::setFileSelectedCallback(std::function<void(const std::string&)> callback) {
    fileSelectedCallback = callback;
}

void FileSelector::browseButtonCallback(Fl_Widget* widget, void* userData) {
    FileSelector* self = static_cast<FileSelector*>(userData);
    self->handleBrowseButtonClick();
}

void FileSelector::loadButtonCallback(Fl_Widget* widget, void* userData) {
    FileSelector* self = static_cast<FileSelector*>(userData);
    self->handleLoadButtonClick();
}

void FileSelector::handleBrowseButtonClick() {
    const char* filename = fl_file_chooser("Select CSV File", "CSV Files (*.csv)", "");
    if (filename) {
        filePathInput->value(filename);
        loadButton->activate();
    }
}

void FileSelector::handleLoadButtonClick() {
    const char* filePath = filePathInput->value();
    if (filePath && *filePath && fileSelectedCallback) {
        fileSelectedCallback(filePath);
    }
}