#include "gui/VariableSelector.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <algorithm>

VariableSelector::VariableSelector(int x, int y, int w, int h) 
    : Fl_Group(x, y, w, h)
{
    begin();
    
    // Set group properties
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
    
    // Constants for layout
    int margin = 10;  // Reduced margin
    int spacing = 10;
    int buttonHeight = 25;
    int headerHeight = 40;
    int bottomButtonsHeight = 30;
    
    // Create title label
    Fl_Box* titleLabel = new Fl_Box(
        x, y + margin, 
        w, headerHeight - margin,
        "Step 3: Select Variables");
    titleLabel->align(FL_ALIGN_CENTER);
    titleLabel->labelsize(16);
    titleLabel->labelfont(FL_BOLD);
    
    // Create description label
    Fl_Box* descriptionLabel = new Fl_Box(
        x + margin, y + headerHeight, 
        w - 2*margin, 30,
        "Select the input variables and target variable for your regression model:");
    descriptionLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    descriptionLabel->labelsize(12);
    
    // Calculate dimensions for the main components
    int componentY = y + headerHeight + 30 + spacing;
    int componentHeight = h - headerHeight - 30 - (margin * 2) - bottomButtonsHeight - spacing * 3;
    int browserWidth = (w - 2*margin - 2*spacing) / 3;
    int buttonWidth = 40;  // Fixed width for add/remove buttons
    
    // Create available variables browser
    int availableX = x + margin;
    Fl_Box* availableLabel = new Fl_Box(
        availableX, componentY, 
        browserWidth, 20,
        "Available Variables");
    availableLabel->align(FL_ALIGN_LEFT);
    availableLabel->labelsize(12);
    
    availableVariablesBrowser = new Fl_Browser(
        availableX, componentY + 25,
        browserWidth, componentHeight - 85);  // Reduce height to make room for info box
    availableVariablesBrowser->type(FL_HOLD_BROWSER);
    availableVariablesBrowser->callback(availableBrowserCallback, this);
    
    // Variable info box - taller to show more content
    variableInfoBox = new Fl_Box(
        availableX, componentY + componentHeight - 55,
        browserWidth, 55, "");
    variableInfoBox->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    variableInfoBox->box(FL_BORDER_BOX);
    variableInfoBox->labelsize(11);  // Smaller font to fit more text
    
    // Create add/remove buttons - centered and with fixed width
    int buttonsX = availableX + browserWidth + spacing;
    addButton = new Fl_Button(
        buttonsX + (spacing/2), componentY + (componentHeight/2) - 20,
        buttonWidth, buttonHeight, ">");
    addButton->callback(addButtonCallback, this);
    addButton->deactivate();
    
    removeButton = new Fl_Button(
        buttonsX + (spacing/2), componentY + (componentHeight/2) + 20,
        buttonWidth, buttonHeight, "<");
    removeButton->callback(removeButtonCallback, this);
    removeButton->deactivate();
    
    // Create selected variables browser
    int selectedX = buttonsX + buttonWidth + spacing;
    Fl_Box* selectedLabel = new Fl_Box(
        selectedX, componentY, 
        browserWidth, 20,
        "Selected Input Variables");
    selectedLabel->align(FL_ALIGN_LEFT);
    selectedLabel->labelsize(12);
    
    selectedVariablesBrowser = new Fl_Browser(
        selectedX, componentY + 25,
        browserWidth, componentHeight);
    selectedVariablesBrowser->type(FL_HOLD_BROWSER);
    selectedVariablesBrowser->callback(selectedBrowserCallback, this);
    
    // Create target variable browser
    int targetX = selectedX + browserWidth + spacing;
    Fl_Box* targetLabel = new Fl_Box(
        targetX, componentY, 
        browserWidth, 20,
        "Target Variable");
    targetLabel->align(FL_ALIGN_LEFT);
    targetLabel->labelsize(12);
    
    Fl_Box* targetDescLabel = new Fl_Box(
        targetX, componentY + 25,
        browserWidth, 20,
        "Select the dependent variable:");
    targetDescLabel->align(FL_ALIGN_LEFT);
    targetDescLabel->labelsize(11);
    
    targetVariableBrowser = new Fl_Browser(
        targetX, componentY + 50,
        browserWidth, componentHeight - 25);
    targetVariableBrowser->type(FL_HOLD_BROWSER);
    targetVariableBrowser->callback(targetBrowserCallback, this);
    
    // Create bottom navigation buttons
    int bottomY = y + h - margin - bottomButtonsHeight;
    backButton = new Fl_Button(
        x + margin, bottomY, 
        100, bottomButtonsHeight, 
        "Back");
    backButton->callback(backButtonCallback_static, this);
    
    runButton = new Fl_Button(
        x + w - margin - 150, bottomY, 
        150, bottomButtonsHeight, 
        "Run Regression");
    runButton->callback(runButtonCallback, this);
    runButton->deactivate();
    
    end();
    resizable(this);  // Make the widget resizable
}

VariableSelector::~VariableSelector() {
    // FLTK handles widget destruction through parent-child relationship
}

void VariableSelector::setAvailableVariables(const std::vector<std::string>& variables) {
    // Clear existing items
    availableVariablesBrowser->clear();
    selectedVariablesBrowser->clear();
    targetVariableBrowser->clear();
    
    // Add variables to available list and target browser
    for (const auto& var : variables) {
        availableVariablesBrowser->add(var.c_str());
        targetVariableBrowser->add(var.c_str());
    }
    
    // Clear variable info
    variableInfoBox->label("");
    
    // Disable run button initially
    updateRunButtonState();
}

void VariableSelector::setVariablesSelectedCallback(
    std::function<void(const std::vector<std::string>&, const std::string&)> callback) {
    variablesSelectedCallback = callback;
}

void VariableSelector::setBackButtonCallback(std::function<void()> callback) {
    backButtonCallback = callback;
}

void VariableSelector::addButtonCallback(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleAddVariableClick();
}

void VariableSelector::removeButtonCallback(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleRemoveVariableClick();
}

void VariableSelector::runButtonCallback(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleRunButtonClick();
}

void VariableSelector::backButtonCallback_static(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleBackButtonClick();
}

void VariableSelector::availableBrowserCallback(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleAvailableVariableSelectionChange();
}

void VariableSelector::selectedBrowserCallback(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleSelectedVariableSelectionChange();
}

void VariableSelector::targetBrowserCallback(Fl_Widget* widget, void* userData) {
    VariableSelector* self = static_cast<VariableSelector*>(userData);
    self->handleTargetVariableChange();
}

void VariableSelector::handleAddVariableClick() {
    // Get selected variable
    int selectedIndex = availableVariablesBrowser->value();
    if (selectedIndex == 0) {
        return;
    }
    
    const char* variableName = availableVariablesBrowser->text(selectedIndex);
    if (!variableName) {
        return;
    }
    
    // Check if variable is already in selected list
    for (int i = 1; i <= selectedVariablesBrowser->size(); ++i) {
        if (strcmp(selectedVariablesBrowser->text(i), variableName) == 0) {
            return; // Already exists
        }
    }
    
    // Add to selected list
    selectedVariablesBrowser->add(variableName);
    
    // Update button states
    updateRunButtonState();
}

void VariableSelector::handleRemoveVariableClick() {
    // Get selected variable
    int selectedIndex = selectedVariablesBrowser->value();
    if (selectedIndex == 0) {
        return;
    }
    
    // Remove from selected list
    selectedVariablesBrowser->remove(selectedIndex);
    
    // Update button states
    updateRunButtonState();
    removeButton->deactivate();
}

void VariableSelector::handleTargetVariableChange() {
    // Target variable changed, update run button state
    updateRunButtonState();
    
    // Update variable info if a target is selected
    int selectedIndex = targetVariableBrowser->value();
    if (selectedIndex != 0) {
        const char* variableName = targetVariableBrowser->text(selectedIndex);
        if (variableName) {
            // Highlight that this is selected as target
            // Don't change the variable info box to avoid confusing the user
        }
    }
}

void VariableSelector::handleAvailableVariableSelectionChange() {
    int selectedIndex = availableVariablesBrowser->value();
    
    // If a variable is selected, activate the add button, otherwise deactivate it
    if (selectedIndex != 0) {
        addButton->activate();
        
        const char* variableName = availableVariablesBrowser->text(selectedIndex);
        if (variableName) {
            showVariableInfo(variableName);
        }
    } else {
        addButton->deactivate();
        variableInfoBox->label("");
    }
}

void VariableSelector::handleSelectedVariableSelectionChange() {
    int selectedIndex = selectedVariablesBrowser->value();
    
    // If a variable is selected, activate the remove button, otherwise deactivate it
    if (selectedIndex != 0) {
        removeButton->activate();
        
        // Also show variable info for the selected input variable
        const char* variableName = selectedVariablesBrowser->text(selectedIndex);
        if (variableName) {
            showVariableInfo(variableName);
        }
    } else {
        removeButton->deactivate();
    }
}

void VariableSelector::handleRunButtonClick() {
    // Get selected input variables
    std::vector<std::string> inputVariables;
    for (int i = 1; i <= selectedVariablesBrowser->size(); ++i) {
        const char* text = selectedVariablesBrowser->text(i);
        if (text) {
            inputVariables.push_back(text);
        }
    }
    
    // Get target variable
    int targetIndex = targetVariableBrowser->value();
    if (targetIndex <= 0) {
        return;
    }
    
    const char* targetVariable = targetVariableBrowser->text(targetIndex);
    if (!targetVariable) {
        return;
    }
    
    // Check if target variable is also an input variable
    if (std::find(inputVariables.begin(), inputVariables.end(), targetVariable) != inputVariables.end()) {
        fl_alert("The target variable cannot also be an input variable.");
        return;
    }
    
    // Call the callback
    if (variablesSelectedCallback) {
        variablesSelectedCallback(inputVariables, targetVariable);
    }
}

void VariableSelector::handleBackButtonClick() {
    if (backButtonCallback) {
        backButtonCallback();
    }
}

void VariableSelector::updateRunButtonState() {
    // Run button is enabled if we have at least one input variable and a target variable
    bool hasInputs = selectedVariablesBrowser->size() > 0;
    bool hasTarget = targetVariableBrowser->value() > 0;
    
    if (hasInputs && hasTarget) {
        runButton->activate();
    } else {
        runButton->deactivate();
    }
}

void VariableSelector::showVariableInfo(const std::string& variableName) {
    // Here you could add additional information about the variable
    // For now, just display the variable name
    std::string info = "Variable: " + variableName;
    
    // You could add more information here, like data type, range, etc.
    // info += "\nType: Numeric"; 
    // info += "\nRange: 0-100";
    
    variableInfoBox->copy_label(info.c_str());
    variableInfoBox->redraw();
}