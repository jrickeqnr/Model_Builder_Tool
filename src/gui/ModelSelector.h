#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <string>
#include <functional>
#include <vector>

/**
 * @brief Widget for regression model selection
 * 
 * This widget provides UI for selecting the type of regression model to use.
 */
class ModelSelector : public Fl_Group {
public:
    ModelSelector(int x, int y, int w, int h);
    ~ModelSelector() = default;

    /**
     * @brief Set the callback for model selection
     * 
     * @param callback Function to call when a model is selected
     */
    void setModelSelectedCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief Set the callback for back button
     * 
     * @param callback Function to call when back button is clicked
     */
    void setBackButtonCallback(std::function<void()> callback);

private:
    Fl_Choice* modelChoice;
    Fl_Box* modelDescriptionBox;
    Fl_Button* nextButton;
    Fl_Button* backButton;
    
    std::function<void(const std::string&)> modelSelectedCallback;
    std::function<void()> backButtonCallbackFn; // Renamed to avoid conflict
    
    // Available models
    std::vector<std::string> models;
    
    // Static callbacks for FLTK
    static void choiceCallback(Fl_Widget* widget, void* userData);
    static void nextButtonCallback(Fl_Widget* widget, void* userData);
    static void backButtonCallback(Fl_Widget* widget, void* userData);
    
    /**
     * @brief Handle model selection change
     */
    void handleModelSelectionChange();
    
    /**
     * @brief Handle next button click
     */
    void handleNextButtonClick();
    
    /**
     * @brief Handle back button click
     */
    void handleBackButtonClick();
    
    /**
     * @brief Update the description based on selected model
     * 
     * @param modelName Name of the selected model
     */
    void updateModelDescription(const std::string& modelName);
};