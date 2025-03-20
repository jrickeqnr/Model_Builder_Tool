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
    // Add more model descriptions as they are implemented
}