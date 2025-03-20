#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <string>
#include <functional>

/**
 * @brief Widget for CSV file selection
 * 
 * This widget provides UI for selecting a CSV file from the file system.
 */
class FileSelector : public Fl_Group {
public:
    FileSelector(int x, int y, int w, int h);
    ~FileSelector() = default;

    /**
     * @brief Set the callback function for file selection
     * 
     * @param callback Function to call when a file is selected
     */
    void setFileSelectedCallback(std::function<void(const std::string&)> callback);

private:
    Fl_Input* filePathInput;
    Fl_Button* browseButton;
    Fl_Button* loadButton;
    Fl_Box* descriptionBox;
    
    std::function<void(const std::string&)> fileSelectedCallback;

    /**
     * @brief Static callback for FLTK buttons
     */
    static void browseButtonCallback(Fl_Widget* widget, void* userData);
    static void loadButtonCallback(Fl_Widget* widget, void* userData);
    
    /**
     * @brief Handle browse button click to open file dialog
     */
    void handleBrowseButtonClick();
    
    /**
     * @brief Handle load button click
     */
    void handleLoadButtonClick();
};