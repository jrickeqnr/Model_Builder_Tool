#pragma once

#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "data/DataFrame.h"
#include "models/Model.h"
#include "gui/FileSelector.h"
#include "gui/ModelSelector.h"
#include "gui/HyperparameterSelector.h"
#include "gui/VariableSelector.h"
#include "gui/ResultsView.h"

/**
 * @brief Main application window
 * 
 * This class manages the main application window and the workflow from
 * file selection to model fitting and result visualization.
 */
class MainWindow : public Fl_Window {
public:
    MainWindow(int width, int height, const char* title);
    ~MainWindow();

    // Callback handlers
    void handleFileSelected(const std::string& filePath);
    void handleModelSelected(const std::string& modelType);
    void handleHyperparametersSelected(const std::unordered_map<std::string, std::string>& hyperparams);
    void handleVariablesSelected(const std::vector<std::string>& inputVariables, 
                               const std::string& targetVariable);
    void handleModelFitted();
    void handleBackButton();
    void handleStartOver();
    
    // Static callback functions for FLTK - moved to public
    static void menuCallback(Fl_Widget* widget, void* userData);

private:
    // Workflow state management
    enum class State {
        FileSelection,
        ModelSelection,
        HyperparameterSelection,
        VariableSelection,
        Results
    };

    // UI components
    Fl_Group* currentPanel;
    Fl_Box* headerLabel;
    Fl_Box* statusBar;
    Fl_Menu_Bar* menuBar;
    
    // Screens
    FileSelector* fileSelector;
    ModelSelector* modelSelector;
    HyperparameterSelector* hyperparameterSelector;
    VariableSelector* variableSelector;
    ResultsView* resultsView;
    
    // Data and model
    std::shared_ptr<DataFrame> dataFrame;
    std::shared_ptr<Model> model;
    
    // Current state
    State currentState;
    std::string currentFilePath;
    std::string currentModelType;
    std::unordered_map<std::string, std::string> currentHyperparameters;
    std::vector<std::string> selectedInputVariables;
    std::string selectedTargetVariable;
    
    /**
     * @brief Update the UI based on the current state
     */
    void updateUI();
    
    /**
     * @brief Create a model based on the selected model type
     * 
     * @param modelType Type of model to create
     * @return std::shared_ptr<Model> Created model
     */
    std::shared_ptr<Model> createModel(const std::string& modelType);
    
    /**
     * @brief Initialize the menu bar
     */
    void setupMenuBar();
};