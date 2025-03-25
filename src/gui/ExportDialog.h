#pragma once

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <string>
#include "gui/PlotGLWindow.h"

class ExportDialog : public Fl_Window {
public:
    ExportDialog(PlotGLWindow::PlotType plotType);
    ~ExportDialog();

    bool wasConfirmed() const { return confirmed; }
    std::string getFilename() const { return filename; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    static void okCallback(Fl_Widget* w, void* v);
    static void cancelCallback(Fl_Widget* w, void* v);
    static void browseCallback(Fl_Widget* w, void* v);

    void initialize();

    // UI Elements
    Fl_Input* filenameInput;
    Fl_Int_Input* widthInput;
    Fl_Int_Input* heightInput;
    Fl_Button* browseButton;
    Fl_Button* okButton;
    Fl_Button* cancelButton;

    // State
    bool confirmed;
    std::string filename;
    int width;
    int height;
    PlotGLWindow::PlotType plotType;
}; 