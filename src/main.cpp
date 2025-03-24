#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "gui/MainWindow.h"
#include "utils/Logger.h"
#include "utils/PlottingUtility.h"

int main(int argc, char *argv[]) {
    // Initialize logging
    LOG_INFO("Application starting", "main");
    
    try {
        // Initialize FLTK
        LOG_INFO("Initializing FLTK", "main");
        Fl::scheme("gtk+");  // Use a modern look and feel
        Fl::visual(FL_DOUBLE | FL_RGB);
        LOG_INFO("FLTK initialized", "main");
        
        // Initialize PlottingUtility
        LOG_INFO("Initializing PlottingUtility", "main");
        PlottingUtility::getInstance();
        LOG_INFO("PlottingUtility initialized", "main");
        
        // Create and show the main window
        LOG_INFO("Creating main window", "main");
        MainWindow *mainWindow = new MainWindow(900, 700, "Linear Regression Tool");
        LOG_INFO("Showing main window", "main");
        mainWindow->show(argc, argv);
        
        // Run the application event loop
        LOG_INFO("Starting FLTK event loop", "main");
        int result = Fl::run();
        LOG_INFO("Application exiting with code: " + std::to_string(result), "main");
        return result;
    } catch (const std::exception& e) {
        LOG_FATAL("Fatal error in main: " + std::string(e.what()), "main");
        return 1;
    } catch (...) {
        LOG_FATAL("Unknown fatal error in main", "main");
        return 1;
    }
}