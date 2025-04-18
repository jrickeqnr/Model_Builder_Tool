#include "gui/MainWindow.h"
#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <iostream>
#include <fstream>
#include <memory>

// Add includes for CSV handling and data
#include "data/CSVReader.h"
#include "data/DataFrame.h"

// Include model headers
#include "models/LinearRegression.h"
#include "models/ElasticNet.h"
#include "models/XGBoost.h"
#include "models/RandomForest.h"
#include "models/NeuralNetwork.h"
#include "models/GradientBoosting.h"

// Include utilities
#include "utils/Logger.h"

// Define the menu items
static Fl_Menu_Item menuItems[] = {
    {"&File", 0, 0, 0, FL_SUBMENU},
        {"&New Analysis", 0, MainWindow::menuCallback, (void*)"new"},
        {"&Exit", 0, MainWindow::menuCallback, (void*)"exit"},
        {0},
    {"&Help", 0, 0, 0, FL_SUBMENU},
        {"&About", 0, MainWindow::menuCallback, (void*)"about"},
        {0},
    {0}
};

MainWindow::MainWindow(int width, int height, const char* title)
    : Fl_Window(width, height, title), currentState(State::FileSelection)
{
    LOG_INFO("Creating MainWindow", "MainWindow");
    
    // Set window properties
    size_range(800, 600);
    
    // Create menu bar
    menuBar = new Fl_Menu_Bar(0, 0, width, 30);
    menuBar->copy(menuItems);
    menuBar->box(FL_FLAT_BOX);
    menuBar->color(fl_rgb_color(240, 240, 240));
    
    // Create header
    headerLabel = new Fl_Box(0, 30, width, 40, "Step 1: Select CSV File");
    headerLabel->box(FL_FLAT_BOX);
    headerLabel->labelfont(FL_BOLD);
    headerLabel->labelsize(18);
    
    // Create panels for each stage
    int panelX = 0;
    int panelY = 70;
    int panelW = width;
    int panelH = height - 100;
    
    // Create status bar - ensure it's fully initialized with a proper label and covers the bottom area
    statusBar = new Fl_Box(0, height - 30, width, 30);
    statusBar->box(FL_FLAT_BOX);
    statusBar->color(fl_rgb_color(240, 240, 240));
    statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    statusBar->copy_label("Ready");
    
    // Set status bar as a resizable component to ensure it fills the bottom
    resizable(statusBar);
    
    LOG_INFO("Creating FileSelector", "MainWindow");
    fileSelector = new FileSelector(panelX, panelY, panelW, panelH);
    fileSelector->setFileSelectedCallback([this](const std::string& filePath) {
        this->handleFileSelected(filePath);
    });
    
    LOG_INFO("Creating ModelSelector", "MainWindow");
    modelSelector = new ModelSelector(panelX, panelY, panelW, panelH);
    modelSelector->setModelSelectedCallback([this](const std::string& modelType) {
        this->handleModelSelected(modelType);
    });
    modelSelector->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
    LOG_INFO("Creating HyperparameterSelector", "MainWindow");
    hyperparameterSelector = new HyperparameterSelector(panelX, panelY, panelW, panelH);
    hyperparameterSelector->setHyperparametersSelectedCallback(
        [this](const std::unordered_map<std::string, std::string>& hyperparams) {
            this->handleHyperparametersSelected(hyperparams);
        }
    );
    hyperparameterSelector->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
    LOG_INFO("Creating VariableSelector", "MainWindow");
    variableSelector = new VariableSelector(panelX, panelY, panelW, panelH);
    variableSelector->setVariablesSelectedCallback(
        [this](const std::vector<std::string>& inputVars, const std::string& targetVar) {
            this->handleVariablesSelected(inputVars, targetVar);
        }
    );
    variableSelector->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
    LOG_INFO("Creating ResultsView", "MainWindow");
    resultsView = new ResultsView(panelX, panelY, panelW, panelH);
    resultsView->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
    // Hide all panels except file selector initially
    modelSelector->hide();
    hyperparameterSelector->hide();
    variableSelector->hide();
    resultsView->hide();
    
    // Set current panel
    currentPanel = fileSelector;
    
    // Initialize UI based on current state
    updateUI();
    
    end();
    LOG_INFO("MainWindow created", "MainWindow");
}

MainWindow::~MainWindow() {
    LOG_INFO("Destroying MainWindow", "MainWindow");
    // Clean up resources
}

void MainWindow::handleFileSelected(const std::string& filePath) {
    LOG_INFO("File selected: " + filePath, "MainWindow");
    currentFilePath = filePath;
    statusBar->copy_label("Loading CSV file...");
    
    try {
        // Read the CSV file
        CSVReader reader;
        LOG_INFO("Reading CSV file", "MainWindow");
        dataFrame = std::make_shared<DataFrame>(reader.readCSV(filePath));
        
        // Update status
        char statusMsg[256];
        snprintf(statusMsg, sizeof(statusMsg), "CSV file loaded successfully: %zu rows, %zu columns", 
                dataFrame->getNumRows(), dataFrame->columnCount());
        statusBar->copy_label(statusMsg);
        LOG_INFO(statusMsg, "MainWindow");
        
        // Move to next step
        currentState = State::ModelSelection;
        updateUI();
    } catch (const std::exception& e) {
        LOG_ERR("Failed to load CSV file: " + std::string(e.what()), "MainWindow");
        fl_alert("Failed to load CSV file: %s", e.what());
        statusBar->copy_label("Failed to load CSV file");
    }
}

void MainWindow::handleModelSelected(const std::string& modelType) {
    LOG_INFO("Model selected: " + modelType, "MainWindow");
    currentModelType = modelType;
    
    // Move to hyperparameter selection for models that have hyperparameters
    if (modelType != "Linear Regression") {
        LOG_INFO("Moving to hyperparameter selection", "MainWindow");
        currentState = State::HyperparameterSelection;
        
        try {
            LOG_INFO("Setting model type on hyperparameter selector: " + modelType, "MainWindow");
            hyperparameterSelector->setModelType(modelType);
            LOG_INFO("Model type set successfully", "MainWindow");
        } catch (const std::exception& e) {
            LOG_ERR("Exception when setting model type: " + std::string(e.what()), "MainWindow");
        } catch (...) {
            LOG_ERR("Unknown exception when setting model type", "MainWindow");
        }
    } else {
        // Linear Regression has no hyperparameters, skip to variable selection
        LOG_INFO("Skipping hyperparameter selection for Linear Regression", "MainWindow");
        currentState = State::VariableSelection;
    }
    
    updateUI();
}

void MainWindow::handleHyperparametersSelected(const std::unordered_map<std::string, std::string>& hyperparams) {
    LOG_INFO("Hyperparameters selected for model: " + currentModelType, "MainWindow");
    for (const auto& param : hyperparams) {
        LOG_INFO("  " + param.first + " = " + param.second, "MainWindow");
    }
    
    currentHyperparameters = hyperparams;
    
    // Move to variable selection
    currentState = State::VariableSelection;
    LOG_INFO("Moving to variable selection", "MainWindow");
    updateUI();
}

void MainWindow::handleVariablesSelected(const std::vector<std::string>& inputVariables, 
                                        const std::string& targetVariable) {
    selectedInputVariables = inputVariables;
    selectedTargetVariable = targetVariable;
    
    // Create model
    model = createModel(currentModelType);
    if (!model) {
        fl_alert("Failed to create model");
        return;
    }
    
    // Update status message with model type and hyperparameters if any
    std::string statusMsg = "Using " + currentModelType;
    if (currentHyperparameters.size() > 0 && currentModelType != "Linear Regression") {
        statusMsg += " with custom hyperparameters";
    }
    statusBar->copy_label(statusMsg.c_str());
    
    // Fit model and move to results view
    fitModelAndShowResults();
}

void MainWindow::fitModelAndShowResults() {
    try {
        // Prepare data
        Eigen::MatrixXd X = dataFrame->toMatrix(selectedInputVariables);
        Eigen::VectorXd y = Eigen::Map<Eigen::VectorXd>(
            dataFrame->getColumn(selectedTargetVariable).data(),
            dataFrame->getColumn(selectedTargetVariable).size()
        );
        
        // Fit model
        statusBar->copy_label("Fitting model...");
        Fl::check();  // Update the UI to show the status message
        
        // Pass variable names to the model when fitting
        bool success = model->fit(X, y, selectedInputVariables, selectedTargetVariable);
        
        if (success) {
            // Configure results view based on model type
            configureResultsView();
            
            // Move to results step
            currentState = State::Results;
            updateUI();
            
            statusBar->copy_label("Model fitted successfully");
            handleModelFitted();
        } else {
            fl_alert("Failed to fit model");
            statusBar->copy_label("Failed to fit model");
        }
    } catch (const std::exception& e) {
        fl_alert("Error fitting model: %s", e.what());
        statusBar->copy_label("Error fitting model");
    }
}

void MainWindow::configureResultsView() {
    // Update results view with model and data
    resultsView->setModel(model);
    resultsView->setData(dataFrame, selectedInputVariables, selectedTargetVariable);
    
    // Configure results view based on model type
    resultsView->setModelType(currentModelType);
    
    // Pass hyperparameters to results view if needed
    if (!currentHyperparameters.empty()) {
        resultsView->setHyperparameters(currentHyperparameters);
    }
}

void MainWindow::handleModelFitted() {
    // Display model results
    resultsView->updateResults();
}

void MainWindow::handleBackButton() {
    // Move back to previous state
    switch (currentState) {
        case State::FileSelection:
            // Already at first step
            break;
            
        case State::ModelSelection:
            currentState = State::FileSelection;
            break;
        
        case State::HyperparameterSelection:
            currentState = State::ModelSelection;
            break;
            
        case State::VariableSelection:
            if (currentModelType != "Linear Regression") {
                currentState = State::HyperparameterSelection;
            } else {
                currentState = State::ModelSelection;
            }
            break;
            
        case State::Results:
            currentState = State::VariableSelection;
            break;
    }
    
    updateUI();
}

void MainWindow::handleStartOver() {
    // Reset state
    currentState = State::FileSelection;
    dataFrame.reset();
    model.reset();
    
    // Clear selections
    currentFilePath.clear();
    currentModelType.clear();
    currentHyperparameters.clear();
    selectedInputVariables.clear();
    selectedTargetVariable.clear();
    
    // Update UI
    updateUI();
    statusBar->copy_label("Started new analysis");
}

void MainWindow::updateUI() {
    LOG_INFO("Updating UI for state: " + std::to_string(static_cast<int>(currentState)), "MainWindow");
    
    // Hide all panels
    fileSelector->hide();
    modelSelector->hide();
    hyperparameterSelector->hide();
    variableSelector->hide();
    resultsView->hide();
    
    // Show the panel for the current state
    switch (currentState) {
        case State::FileSelection:
            LOG_INFO("Showing FileSelector", "MainWindow");
            headerLabel->copy_label("Step 1: Select CSV File");
            fileSelector->show();
            currentPanel = fileSelector;
            break;
            
        case State::ModelSelection:
            LOG_INFO("Showing ModelSelector", "MainWindow");
            headerLabel->copy_label("Step 2: Select Model Type");
            modelSelector->show();
            currentPanel = modelSelector;
            break;
            
        case State::HyperparameterSelection:
            LOG_INFO("Showing HyperparameterSelector", "MainWindow");
            headerLabel->copy_label("Step 3: Configure Hyperparameters");
            hyperparameterSelector->show();
            currentPanel = hyperparameterSelector;
            break;
            
        case State::VariableSelection:
            LOG_INFO("Showing VariableSelector", "MainWindow");
            if (currentModelType != "Linear Regression") {
                headerLabel->copy_label("Step 4: Select Variables");
            } else {
                headerLabel->copy_label("Step 3: Select Variables");
            }
            variableSelector->show();
            if (dataFrame) {
                variableSelector->setAvailableVariables(dataFrame->getColumnNames());
            }
            currentPanel = variableSelector;
            break;
            
        case State::Results:
            LOG_INFO("Showing ResultsView", "MainWindow");
            headerLabel->copy_label("Results");
            resultsView->show();
            currentPanel = resultsView;
            break;
    }
    
    // Redraw the entire window to ensure all components are properly displayed
    this->redraw();
    Fl::check();  // Process any pending events
    LOG_INFO("UI updated", "MainWindow");
}

std::shared_ptr<Model> MainWindow::createModel(const std::string& modelType) {
    LOG_INFO("Creating model of type: " + modelType, "MainWindow");
    
    std::shared_ptr<Model> result = nullptr;
    
    try {
        if (modelType == "Linear Regression") {
            result = std::make_shared<LinearRegression>();
        }
        else if (modelType == "ElasticNet") {
            // Parse hyperparameters for ElasticNet
            double alpha = 0.5;
            double lambda = 1.0;
            int max_iter = 1000;
            double tol = 0.0001;

            // Parse hyperparameters if they exist
            if (!currentHyperparameters.empty()) {
                try {
                    // Check if the parameter exists before trying to access it
                    if (currentHyperparameters.find("alpha") != currentHyperparameters.end() && 
                        currentHyperparameters.at("alpha") != "auto") {
                        alpha = std::stod(currentHyperparameters.at("alpha"));
                    }
                    if (currentHyperparameters.find("lambda") != currentHyperparameters.end() && 
                        currentHyperparameters.at("lambda") != "auto") {
                        lambda = std::stod(currentHyperparameters.at("lambda"));
                    }
                    if (currentHyperparameters.find("max_iter") != currentHyperparameters.end() && 
                        currentHyperparameters.at("max_iter") != "auto") {
                        max_iter = std::stoi(currentHyperparameters.at("max_iter"));
                    }
                    if (currentHyperparameters.find("tol") != currentHyperparameters.end() && 
                        currentHyperparameters.at("tol") != "auto") {
                        tol = std::stod(currentHyperparameters.at("tol"));
                    }
                } catch (const std::exception& e) {
                    LOG_ERR("Error parsing ElasticNet hyperparameters: " + std::string(e.what()), "MainWindow");
                }
            }

            LOG_INFO("Creating ElasticNet with alpha=" + std::to_string(alpha) + 
                     ", lambda=" + std::to_string(lambda) + 
                     ", max_iter=" + std::to_string(max_iter) + 
                     ", tol=" + std::to_string(tol), "MainWindow");
                     
            result = std::make_shared<ElasticNet>(alpha, lambda, max_iter, tol);
        }
        else if (modelType == "XGBoost") {
            // Create XGBoost with selected hyperparameters
            double learning_rate = 0.1;
            int max_depth = 6;
            int n_estimators = 100;
            double subsample = 1.0;
            double colsample_bytree = 1.0;
            int min_child_weight = 1;
            double gamma = 0.0;

            // Parse hyperparameters if they exist
            if (!currentHyperparameters.empty()) {
                try {
                    if (currentHyperparameters.find("learning_rate") != currentHyperparameters.end() && 
                        currentHyperparameters.at("learning_rate") != "auto") {
                        learning_rate = std::stod(currentHyperparameters.at("learning_rate"));
                    }
                    if (currentHyperparameters.find("max_depth") != currentHyperparameters.end() && 
                        currentHyperparameters.at("max_depth") != "auto") {
                        max_depth = std::stoi(currentHyperparameters.at("max_depth"));
                    }
                    if (currentHyperparameters.find("n_estimators") != currentHyperparameters.end() && 
                        currentHyperparameters.at("n_estimators") != "auto") {
                        n_estimators = std::stoi(currentHyperparameters.at("n_estimators"));
                    }
                    if (currentHyperparameters.find("subsample") != currentHyperparameters.end() && 
                        currentHyperparameters.at("subsample") != "auto") {
                        subsample = std::stod(currentHyperparameters.at("subsample"));
                    }
                    if (currentHyperparameters.find("colsample_bytree") != currentHyperparameters.end() && 
                        currentHyperparameters.at("colsample_bytree") != "auto") {
                        colsample_bytree = std::stod(currentHyperparameters.at("colsample_bytree"));
                    }
                    if (currentHyperparameters.find("min_child_weight") != currentHyperparameters.end() && 
                        currentHyperparameters.at("min_child_weight") != "auto") {
                        min_child_weight = std::stoi(currentHyperparameters.at("min_child_weight"));
                    }
                    if (currentHyperparameters.find("gamma") != currentHyperparameters.end() && 
                        currentHyperparameters.at("gamma") != "auto") {
                        gamma = std::stod(currentHyperparameters.at("gamma"));
                    }
                } catch (const std::exception& e) {
                    LOG_ERR("Error parsing XGBoost hyperparameters: " + std::string(e.what()), "MainWindow");
                }
            }

            LOG_INFO("Creating XGBoost with learning_rate=" + std::to_string(learning_rate) + 
                     ", max_depth=" + std::to_string(max_depth) + 
                     ", n_estimators=" + std::to_string(n_estimators) + 
                     ", subsample=" + std::to_string(subsample) + 
                     ", colsample_bytree=" + std::to_string(colsample_bytree) + 
                     ", min_child_weight=" + std::to_string(min_child_weight) + 
                     ", gamma=" + std::to_string(gamma), "MainWindow");
                     
            result = std::make_shared<XGBoost>(learning_rate, max_depth, n_estimators,
                                             subsample, colsample_bytree, min_child_weight, gamma);
        }
        else if (modelType == "Random Forest") {
            // Parse hyperparameters for Random Forest
            int n_estimators = 100;
            int max_depth = 10;
            int min_samples_split = 2;
            int min_samples_leaf = 1;
            std::string max_features = "auto";
            bool bootstrap = true;

            // Parse hyperparameters if they exist
            if (!currentHyperparameters.empty()) {
                try {
                    if (currentHyperparameters.find("n_estimators") != currentHyperparameters.end() && 
                        currentHyperparameters.at("n_estimators") != "auto") {
                        n_estimators = std::stoi(currentHyperparameters.at("n_estimators"));
                    }
                    if (currentHyperparameters.find("max_depth") != currentHyperparameters.end() && 
                        currentHyperparameters.at("max_depth") != "auto") {
                        max_depth = std::stoi(currentHyperparameters.at("max_depth"));
                    }
                    if (currentHyperparameters.find("min_samples_split") != currentHyperparameters.end() && 
                        currentHyperparameters.at("min_samples_split") != "auto") {
                        min_samples_split = std::stoi(currentHyperparameters.at("min_samples_split"));
                    }
                    if (currentHyperparameters.find("min_samples_leaf") != currentHyperparameters.end() && 
                        currentHyperparameters.at("min_samples_leaf") != "auto") {
                        min_samples_leaf = std::stoi(currentHyperparameters.at("min_samples_leaf"));
                    }
                    if (currentHyperparameters.find("max_features") != currentHyperparameters.end() && 
                        currentHyperparameters.at("max_features") != "auto") {
                        max_features = currentHyperparameters.at("max_features");
                    }
                    if (currentHyperparameters.find("bootstrap") != currentHyperparameters.end() && 
                        currentHyperparameters.at("bootstrap") != "auto") {
                        bootstrap = (currentHyperparameters.at("bootstrap") == "true");
                    }
                } catch (const std::exception& e) {
                    LOG_ERR("Error parsing Random Forest hyperparameters: " + std::string(e.what()), "MainWindow");
                }
            }

            LOG_INFO("Creating Random Forest with n_estimators=" + std::to_string(n_estimators) + 
                     ", max_depth=" + std::to_string(max_depth) + 
                     ", min_samples_split=" + std::to_string(min_samples_split) + 
                     ", min_samples_leaf=" + std::to_string(min_samples_leaf) + 
                     ", max_features=" + max_features + 
                     ", bootstrap=" + (bootstrap ? "true" : "false"), "MainWindow");
                     
            result = std::make_shared<RandomForest>(n_estimators, max_depth, min_samples_split,
                                                  min_samples_leaf, max_features, bootstrap);
        }
        else if (modelType == "Neural Network") {
            // Create Neural Network with selected hyperparameters
            std::vector<int> hidden_layer_sizes = {10};  // Default
            std::string activation = "relu";
            double learning_rate = 0.001;
            int max_iter = 200;
            int batch_size = 32;
            std::string solver = "adam";
            double alpha = 0.0001;

            // Parse hyperparameters if they exist
            if (!currentHyperparameters.empty()) {
                try {
                    if (currentHyperparameters.find("hidden_layer_sizes") != currentHyperparameters.end() && 
                        currentHyperparameters.at("hidden_layer_sizes") != "auto") {
                        // Parse hidden layer sizes from comma-separated string
                        std::string hidden_layers = currentHyperparameters.at("hidden_layer_sizes");
                        std::stringstream ss(hidden_layers);
                        std::string layer;
                        hidden_layer_sizes.clear();
                        while (std::getline(ss, layer, ',')) {
                            hidden_layer_sizes.push_back(std::stoi(layer));
                        }
                    }

                    if (currentHyperparameters.find("activation") != currentHyperparameters.end() && 
                        currentHyperparameters.at("activation") != "auto") {
                        activation = currentHyperparameters.at("activation");
                    }
                    if (currentHyperparameters.find("learning_rate") != currentHyperparameters.end() && 
                        currentHyperparameters.at("learning_rate") != "auto") {
                        learning_rate = std::stod(currentHyperparameters.at("learning_rate"));
                    }
                    if (currentHyperparameters.find("max_iter") != currentHyperparameters.end() && 
                        currentHyperparameters.at("max_iter") != "auto") {
                        max_iter = std::stoi(currentHyperparameters.at("max_iter"));
                    }
                    if (currentHyperparameters.find("batch_size") != currentHyperparameters.end() && 
                        currentHyperparameters.at("batch_size") != "auto") {
                        batch_size = std::stoi(currentHyperparameters.at("batch_size"));
                    }
                    if (currentHyperparameters.find("solver") != currentHyperparameters.end() && 
                        currentHyperparameters.at("solver") != "auto") {
                        solver = currentHyperparameters.at("solver");
                    }
                    if (currentHyperparameters.find("alpha") != currentHyperparameters.end() && 
                        currentHyperparameters.at("alpha") != "auto") {
                        alpha = std::stod(currentHyperparameters.at("alpha"));
                    }
                } catch (const std::exception& e) {
                    LOG_ERR("Error parsing Neural Network hyperparameters: " + std::string(e.what()), "MainWindow");
                }
            }

            // Prepare hidden layer sizes string for logging
            std::string hiddenLayersStr = "{";
            for (size_t i = 0; i < hidden_layer_sizes.size(); ++i) {
                hiddenLayersStr += std::to_string(hidden_layer_sizes[i]);
                if (i < hidden_layer_sizes.size() - 1) {
                    hiddenLayersStr += ",";
                }
            }
            hiddenLayersStr += "}";

            LOG_INFO("Creating Neural Network with hidden_layer_sizes=" + hiddenLayersStr + 
                     ", activation=" + activation + 
                     ", learning_rate=" + std::to_string(learning_rate) + 
                     ", max_iter=" + std::to_string(max_iter) + 
                     ", batch_size=" + std::to_string(batch_size) + 
                     ", solver=" + solver + 
                     ", alpha=" + std::to_string(alpha), "MainWindow");
                     
            result = std::make_shared<NeuralNetwork>(hidden_layer_sizes, activation,
                                                   learning_rate, max_iter, batch_size,
                                                   solver, alpha);
        }
        else if (modelType == "Gradient Boosting") {
            // Create Gradient Boosting with selected hyperparameters
            double learning_rate = 0.1;
            int n_estimators = 100;
            int max_depth = 3;
            int min_samples_split = 2;
            int min_samples_leaf = 1;
            double subsample = 1.0;
            std::string loss = "squared_error";

            // Parse hyperparameters if they exist
            if (!currentHyperparameters.empty()) {
                try {
                    if (currentHyperparameters.find("learning_rate") != currentHyperparameters.end() && 
                        currentHyperparameters.at("learning_rate") != "auto") {
                        learning_rate = std::stod(currentHyperparameters.at("learning_rate"));
                    }
                    if (currentHyperparameters.find("n_estimators") != currentHyperparameters.end() && 
                        currentHyperparameters.at("n_estimators") != "auto") {
                        n_estimators = std::stoi(currentHyperparameters.at("n_estimators"));
                    }
                    if (currentHyperparameters.find("max_depth") != currentHyperparameters.end() && 
                        currentHyperparameters.at("max_depth") != "auto") {
                        max_depth = std::stoi(currentHyperparameters.at("max_depth"));
                    }
                    if (currentHyperparameters.find("min_samples_split") != currentHyperparameters.end() && 
                        currentHyperparameters.at("min_samples_split") != "auto") {
                        min_samples_split = std::stoi(currentHyperparameters.at("min_samples_split"));
                    }
                    if (currentHyperparameters.find("min_samples_leaf") != currentHyperparameters.end() && 
                        currentHyperparameters.at("min_samples_leaf") != "auto") {
                        min_samples_leaf = std::stoi(currentHyperparameters.at("min_samples_leaf"));
                    }
                    if (currentHyperparameters.find("subsample") != currentHyperparameters.end() && 
                        currentHyperparameters.at("subsample") != "auto") {
                        subsample = std::stod(currentHyperparameters.at("subsample"));
                    }
                    if (currentHyperparameters.find("loss") != currentHyperparameters.end() && 
                        currentHyperparameters.at("loss") != "auto") {
                        loss = currentHyperparameters.at("loss");
                    }
                } catch (const std::exception& e) {
                    LOG_ERR("Error parsing Gradient Boosting hyperparameters: " + std::string(e.what()), "MainWindow");
                }
            }

            LOG_INFO("Creating Gradient Boosting with learning_rate=" + std::to_string(learning_rate) + 
                     ", n_estimators=" + std::to_string(n_estimators) + 
                     ", max_depth=" + std::to_string(max_depth) + 
                     ", min_samples_split=" + std::to_string(min_samples_split) + 
                     ", min_samples_leaf=" + std::to_string(min_samples_leaf) + 
                     ", subsample=" + std::to_string(subsample) + 
                     ", loss=" + loss, "MainWindow");
                     
            result = std::make_shared<GradientBoosting>(learning_rate, n_estimators,
                                                      max_depth, min_samples_split,
                                                      min_samples_leaf, subsample, loss);
        }
        else {
            LOG_ERR("Unknown model type: " + modelType, "MainWindow");
            fl_alert("Unknown model type selected. Please try again.");
            return nullptr;
        }
        
        if (result) {
            LOG_INFO("Model created successfully", "MainWindow");
        } else {
            LOG_ERR("Failed to create model", "MainWindow");
        }
    } catch (const std::exception& e) {
        LOG_ERR("Exception creating model: " + std::string(e.what()), "MainWindow");
    } catch (...) {
        LOG_ERR("Unknown exception creating model", "MainWindow");
    }
    
    return result;
}

void MainWindow::menuCallback(Fl_Widget* widget, void* userData) {
    try {
        MainWindow* window = static_cast<MainWindow*>(widget->window());
        const char* action = static_cast<const char*>(userData);
        
        if (!window) {
            LOG_ERR("Null window pointer in menu callback", "MainWindow");
            return;
        }
        
        LOG_INFO("Menu action: " + std::string(action), "MainWindow");
        
        if (strcmp(action, "new") == 0) {
            window->handleStartOver();
        } else if (strcmp(action, "exit") == 0) {
            window->hide();
        } else if (strcmp(action, "about") == 0) {
            fl_message("Linear Regression Tool v1.0.0\n\n"
                      "A simple tool for performing linear regression analysis on CSV data.");
        }
    } catch (const std::exception& e) {
        LOG_ERR("Exception in menu callback: " + std::string(e.what()), "MainWindow");
    } catch (...) {
        LOG_ERR("Unknown exception in menu callback", "MainWindow");
    }
}