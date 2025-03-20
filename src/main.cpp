#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "gui/MainWindow.h"

int main(int argc, char *argv[]) {
    // Initialize FLTK
    Fl::scheme("gtk+");  // Use a modern look and feel
    Fl::visual(FL_DOUBLE | FL_RGB);
    
    // Create and show the main window
    MainWindow *mainWindow = new MainWindow(900, 700, "Linear Regression Tool");
    mainWindow->show(argc, argv);
    
    // Run the application event loop
    return Fl::run();
}