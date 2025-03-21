#include "gui/ModelSelector.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>

ModelSelector::ModelSelector(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h)
{
    begin();
    
    int margin = 20;
    
    // Create description label
    Fl_Box* descriptionLabel = new Fl_Box(x + margin, y + margin, w - 2*margin, 40,
        "Select the type of regression model you want to use for your analysis:");
    descriptionLabel->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_WRAP);
    
    // Create model selection dropdown
    modelChoice = new Fl_Choice(x + margin + 120, y + margin + 60, 200, 30, "Model Type:");
    modelChoice->align(FL_ALIGN_LEFT);
    modelChoice->callback(choiceCallback, this);
    
    // Add model options
    models.push_back("Linear Regression");
    models.push_back("ElasticNet");
    models.push_back("XGBoost");
    models.push_back("Random Forest");
    models.push_back("Neural Network");
    models.push_back("Gradient Boosting");
    
    for (const auto& model : models) {
        modelChoice->add(model.c_str());
    }
    modelChoice->value(0);  // Select the first model by default
    
    // Create model description box with a border
    int descY = y + margin + 110;
    int descH = h - descY - margin - 60;
    modelDescriptionBox = new Fl_Box(x + margin, descY, w - 2*margin, descH);
    modelDescriptionBox->box(FL_BORDER_BOX);
    modelDescriptionBox->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_WRAP | FL_ALIGN_INSIDE);
    
    // Update the description for the default selected model
    updateModelDescription(models[0]);
    
    // Create navigation buttons
    backButton = new Fl_Button(x + margin, y + h - margin - 40, 100, 40, "Back");
    backButton->callback(backButtonCallback, this);
    
    nextButton = new Fl_Button(x + w - margin - 100, y + h - margin - 40, 100, 40, "Next");
    nextButton->callback(nextButtonCallback, this);
    
    end();
    
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
}

void ModelSelector::setModelSelectedCallback(std::function<void(const std::string&)> callback) {
    modelSelectedCallback = callback;
}

void ModelSelector::setBackButtonCallback(std::function<void()> callback) {
    backButtonCallbackFn = callback; // Updated variable name
}

void ModelSelector::choiceCallback(Fl_Widget* widget, void* userData) {
    ModelSelector* self = static_cast<ModelSelector*>(userData);
    self->handleModelSelectionChange();
}

void ModelSelector::nextButtonCallback(Fl_Widget* widget, void* userData) {
    ModelSelector* self = static_cast<ModelSelector*>(userData);
    self->handleNextButtonClick();
}

void ModelSelector::backButtonCallback(Fl_Widget* widget, void* userData) {
    ModelSelector* self = static_cast<ModelSelector*>(userData);
    self->handleBackButtonClick();
}

void ModelSelector::handleModelSelectionChange() {
    int selected = modelChoice->value();
    if (selected >= 0 && selected < static_cast<int>(models.size())) {
        updateModelDescription(models[selected]);
    }
}

void ModelSelector::handleNextButtonClick() {
    int selected = modelChoice->value();
    if (selected >= 0 && selected < static_cast<int>(models.size()) && modelSelectedCallback) {
        modelSelectedCallback(models[selected]);
    }
}

void ModelSelector::handleBackButtonClick() {
    if (backButtonCallbackFn) { // Updated variable name
        backButtonCallbackFn();
    }
}

void ModelSelector::updateModelDescription(const std::string& modelName) {
    if (modelName == "Linear Regression") {
        modelDescriptionBox->label(
            "Linear Regression\n\n"
            "Linear regression is a linear approach to modeling the relationship "
            "between a dependent variable and one or more independent variables.\n\n"
            "Key characteristics:\n"
            "- Simple and interpretable model\n"
            "- Works well for linearly separable data\n"
            "- Provides coefficients that indicate the impact of each feature\n"
            "- Assumes a linear relationship between variables\n\n"
            "Performance metrics:\n"
            "- R-squared (coefficient of determination)\n"
            "- Adjusted R-squared\n"
            "- Root Mean Squared Error (RMSE)"
        );
    }
    else if (modelName == "ElasticNet") {
        modelDescriptionBox->label(
            "ElasticNet Regression\n\n"
            "ElasticNet combines L1 and L2 regularization to handle correlated variables "
            "and prevent overfitting in regression models.\n\n"
            "Key characteristics:\n"
            "- Regularization technique that combines Lasso (L1) and Ridge (L2) penalties\n"
            "- Good for datasets with correlated features\n"
            "- Can perform feature selection by zeroing out less important features\n"
            "- Balances between feature selection and coefficient shrinkage\n\n"
            "Hyperparameters:\n"
            "- Alpha: Controls the L1 vs L2 ratio (1 = Lasso, 0 = Ridge)\n"
            "- Lambda: Overall regularization strength"
        );
    }
    else if (modelName == "XGBoost") {
        modelDescriptionBox->label(
            "XGBoost (Extreme Gradient Boosting)\n\n"
            "A high-performance implementation of gradient boosted decision trees "
            "designed for speed and performance.\n\n"
            "Key characteristics:\n"
            "- Highly efficient and scalable implementation of gradient boosting\n"
            "- Often winning solution in machine learning competitions\n"
            "- Handles missing values automatically\n"
            "- Includes regularization to prevent overfitting\n"
            "- Parallel tree construction for faster training\n\n"
            "Hyperparameters:\n"
            "- Learning rate: Controls the contribution of each tree\n"
            "- Max depth: Maximum depth of trees\n"
            "- Number of estimators: Number of boosting rounds"
        );
    }
    else if (modelName == "Random Forest") {
        modelDescriptionBox->label(
            "Random Forest Regression\n\n"
            "An ensemble learning method that builds multiple decision trees and "
            "merges their predictions to improve accuracy and control overfitting.\n\n"
            "Key characteristics:\n"
            "- Ensemble of decision trees trained on random subsets of data\n"
            "- Handles high-dimensional data well\n"
            "- Robust to outliers and non-linear data\n"
            "- Provides feature importance measures\n"
            "- Less prone to overfitting than single decision trees\n\n"
            "Hyperparameters:\n"
            "- Number of trees: More trees usually means better performance\n"
            "- Max depth: Controls the maximum depth of each tree\n"
            "- Min samples split/leaf: Controls the minimum number of samples required"
        );
    }
    else if (modelName == "Neural Network") {
        modelDescriptionBox->label(
            "Neural Network Regression\n\n"
            "A multilayer perceptron (MLP) for regression that can model complex "
            "non-linear relationships in data.\n\n"
            "Key characteristics:\n"
            "- Can approximate any continuous function\n"
            "- Effective for complex, high-dimensional data\n"
            "- Automatically learns feature interactions\n"
            "- Requires more data than traditional regression models\n"
            "- May be more difficult to interpret\n\n"
            "Hyperparameters:\n"
            "- Hidden layer sizes: Number and size of hidden layers\n"
            "- Activation function: Non-linear function applied at each neuron\n"
            "- Learning rate: Controls the step size during optimization\n"
            "- Batch size: Number of samples processed before model update"
        );
    }
    else if (modelName == "Gradient Boosting") {
        modelDescriptionBox->label(
            "Gradient Boosting Regression\n\n"
            "An ensemble technique that builds regression trees sequentially, with each "
            "tree correcting the errors of its predecessors.\n\n"
            "Key characteristics:\n"
            "- Powerful technique for regression problems\n"
            "- Builds trees sequentially to correct previous trees' errors\n"
            "- Often better performance than random forests\n"
            "- Can capture complex non-linear patterns\n"
            "- Provides feature importance measures\n\n"
            "Hyperparameters:\n"
            "- Learning rate: Controls the contribution of each tree\n"
            "- Number of estimators: Number of sequential trees\n"
            "- Max depth: Maximum depth of each tree\n"
            "- Subsample: Fraction of samples used for tree building"
        );
    }
}