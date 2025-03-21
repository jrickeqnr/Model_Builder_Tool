#include "gui/MainWindow.h"
#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include "models/LinearRegression.h"
#include "data/CSVReader.h"

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
    
    // Create status bar
    statusBar = new Fl_Box(0, height - 30, width, 30, "Ready");
    statusBar->box(FL_FLAT_BOX);
    statusBar->color(fl_rgb_color(240, 240, 240));
    statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Create panels for each stage
    int panelX = 0;
    int panelY = 70;
    int panelW = width;
    int panelH = height - 100;
    
    fileSelector = new FileSelector(panelX, panelY, panelW, panelH);
    fileSelector->setFileSelectedCallback([this](const std::string& filePath) {
        this->handleFileSelected(filePath);
    });
    
    modelSelector = new ModelSelector(panelX, panelY, panelW, panelH);
    modelSelector->setModelSelectedCallback([this](const std::string& modelType) {
        this->handleModelSelected(modelType);
    });
    modelSelector->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
    hyperparameterSelector = new HyperparameterSelector(panelX, panelY, panelW, panelH);
    hyperparameterSelector->setHyperparametersSelectedCallback(
        [this](const std::unordered_map<std::string, std::string>& hyperparams) {
            this->handleHyperparametersSelected(hyperparams);
        }
    );
    hyperparameterSelector->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
    variableSelector = new VariableSelector(panelX, panelY, panelW, panelH);
    variableSelector->setVariablesSelectedCallback(
        [this](const std::vector<std::string>& inputVars, const std::string& targetVar) {
            this->handleVariablesSelected(inputVars, targetVar);
        }
    );
    variableSelector->setBackButtonCallback([this]() {
        this->handleBackButton();
    });
    
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
    
    // Set resizable
    resizable(this);
    
    end();
}

MainWindow::~MainWindow() {
    // Clean up resources
}

void MainWindow::handleFileSelected(const std::string& filePath) {
    currentFilePath = filePath;
    statusBar->label("Loading CSV file...");
    
    try {
        // Read the CSV file
        CSVReader reader;
        dataFrame = std::make_shared<DataFrame>(reader.readCSV(filePath));
        
        // Update status
        char statusMsg[256];
        snprintf(statusMsg, sizeof(statusMsg), "CSV file loaded successfully: %zu rows, %zu columns", 
                dataFrame->rowCount(), dataFrame->columnCount());
        statusBar->label(statusMsg);
        
        // Move to next step
        currentState = State::ModelSelection;
        updateUI();
    } catch (const std::exception& e) {
        fl_alert("Failed to load CSV file: %s", e.what());
        statusBar->label("Failed to load CSV file");
    }
}

void MainWindow::handleModelSelected(const std::string& modelType) {
    currentModelType = modelType;
    
    // Move to hyperparameter selection for models that have hyperparameters
    if (modelType != "Linear Regression") {
        currentState = State::HyperparameterSelection;
        hyperparameterSelector->setModelType(modelType);
    } else {
        // Linear Regression has no hyperparameters, skip to variable selection
        currentState = State::VariableSelection;
    }
    
    updateUI();
}

void MainWindow::handleHyperparametersSelected(const std::unordered_map<std::string, std::string>& hyperparams) {
    currentHyperparameters = hyperparams;
    
    // Move to variable selection
    currentState = State::VariableSelection;
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
    statusBar->label(statusMsg.c_str());
    
    // Fit model
    try {
        // Prepare data
        Eigen::MatrixXd X = dataFrame->toMatrix(inputVariables);
        Eigen::VectorXd y = Eigen::Map<Eigen::VectorXd>(
            dataFrame->getColumn(targetVariable).data(),
            dataFrame->getColumn(targetVariable).size()
        );
        
        // Fit model
        statusBar->label("Fitting model...");
        Fl::check();  // Update the UI to show the status message
        
        // Pass variable names to the model when fitting
        bool success = model->fit(X, y, inputVariables, targetVariable);
        
        if (success) {
            // Update results view
            resultsView->setModel(model);
            resultsView->setData(dataFrame, inputVariables, targetVariable);
            
            // Move to results step
            currentState = State::Results;
            updateUI();
            
            statusBar->label("Model fitted successfully");
            handleModelFitted();
        } else {
            fl_alert("Failed to fit model");
            statusBar->label("Failed to fit model");
        }
    } catch (const std::exception& e) {
        fl_alert("Error fitting model: %s", e.what());
        statusBar->label("Error fitting model");
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
    statusBar->label("Started new analysis");
}

void MainWindow::updateUI() {
    // Hide all panels
    fileSelector->hide();
    modelSelector->hide();
    hyperparameterSelector->hide();
    variableSelector->hide();
    resultsView->hide();
    
    // Show the panel for the current state
    switch (currentState) {
        case State::FileSelection:
            headerLabel->label("Step 1: Select CSV File");
            fileSelector->show();
            currentPanel = fileSelector;
            break;
            
        case State::ModelSelection:
            headerLabel->label("Step 2: Select Model Type");
            modelSelector->show();
            currentPanel = modelSelector;
            break;
            
        case State::HyperparameterSelection:
            headerLabel->label("Step 3: Configure Hyperparameters");
            hyperparameterSelector->show();
            currentPanel = hyperparameterSelector;
            break;
            
        case State::VariableSelection:
            if (currentModelType != "Linear Regression") {
                headerLabel->label("Step 4: Select Variables");
            } else {
                headerLabel->label("Step 3: Select Variables");
            }
            variableSelector->show();
            if (dataFrame) {
                variableSelector->setAvailableVariables(dataFrame->getColumnNames());
            }
            currentPanel = variableSelector;
            break;
            
        case State::Results:
            headerLabel->label("Results");
            resultsView->show();
            currentPanel = resultsView;
            break;
    }
    
    // Redraw the window
    redraw();
}

std::shared_ptr<Model> MainWindow::createModel(const std::string& modelType) {
    if (modelType == "Linear Regression") {
        return std::make_shared<LinearRegression>();
    }
    else if (modelType == "ElasticNet") {
        // In a real implementation, we would create an ElasticNet model class
        // and pass the hyperparameters to the constructor
        // For now, we'll return a LinearRegression as a placeholder
        auto linearModel = std::make_shared<LinearRegression>();
        // In a real implementation, we would set the hyperparameters here
        return linearModel;
    }
    else if (modelType == "XGBoost") {
        // In a real implementation, we would create an XGBoost model class
        // and pass the hyperparameters to the constructor
        // For now, we'll return a LinearRegression as a placeholder
        auto linearModel = std::make_shared<LinearRegression>();
        // In a real implementation, we would set the hyperparameters here
        return linearModel;
    }
    else if (modelType == "Random Forest") {
        // In a real implementation, we would create a RandomForest model class
        // and pass the hyperparameters to the constructor
        // For now, we'll return a LinearRegression as a placeholder
        auto linearModel = std::make_shared<LinearRegression>();
        // In a real implementation, we would set the hyperparameters here
        return linearModel;
    }
    else if (modelType == "Neural Network") {
        // In a real implementation, we would create a NeuralNetwork model class
        // and pass the hyperparameters to the constructor
        // For now, we'll return a LinearRegression as a placeholder
        auto linearModel = std::make_shared<LinearRegression>();
        // In a real implementation, we would set the hyperparameters here
        return linearModel;
    }
    else if (modelType == "Gradient Boosting") {
        // In a real implementation, we would create a GradientBoosting model class
        // and pass the hyperparameters to the constructor
        // For now, we'll return a LinearRegression as a placeholder
        auto linearModel = std::make_shared<LinearRegression>();
        // In a real implementation, we would set the hyperparameters here
        return linearModel;
    }
    
    // Unknown model type
    return nullptr;
}

void MainWindow::menuCallback(Fl_Widget* widget, void* userData) {
    MainWindow* window = static_cast<MainWindow*>(widget->window());
    const char* action = static_cast<const char*>(userData);
    
    if (strcmp(action, "new") == 0) {
        window->handleStartOver();
    } else if (strcmp(action, "exit") == 0) {
        window->hide();
    } else if (strcmp(action, "about") == 0) {
        fl_message("Linear Regression Tool v1.0.0\n\n"
                  "A simple tool for performing linear regression analysis on CSV data.");
    }
}