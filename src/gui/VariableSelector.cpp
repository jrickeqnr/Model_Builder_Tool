#include "VariableSelector.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <algorithm>

namespace {
    constexpr int MARGIN = 10;
    constexpr int SPACING = 10;
    constexpr int BUTTON_HEIGHT = 25;
    constexpr int HEADER_HEIGHT = 30;
    constexpr int BOTTOM_BUTTONS_HEIGHT = 30;
    constexpr int LABEL_HEIGHT = 20;
    constexpr int DESC_HEIGHT = 25;
}

struct VariableSelector::Layout {
    int x, y, w, h;
    int browserWidth;
    int componentHeight;
    int componentY;
    int buttonsX;
    int selectedX;
    int targetX;
    int bottomY;

    Layout(int x_, int y_, int w_, int h_) 
        : x(x_), y(y_), w(w_), h(h_) {
        browserWidth = (w - 2 * MARGIN - 2 * SPACING) / 3;
        componentHeight = h - 2 * MARGIN - HEADER_HEIGHT - DESC_HEIGHT - BOTTOM_BUTTONS_HEIGHT - SPACING;
        componentY = y + MARGIN + HEADER_HEIGHT + DESC_HEIGHT;
        buttonsX = x + MARGIN + browserWidth + SPACING/2;
        selectedX = buttonsX + BUTTON_HEIGHT + SPACING/2;
        targetX = selectedX + browserWidth + SPACING;
        bottomY = y + h - MARGIN - BOTTOM_BUTTONS_HEIGHT;
    }
};

VariableSelector::VariableSelector(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), layout(std::make_unique<Layout>(x, y, w, h))
{
    begin();
    initUI();
    end();
    resizable(this);
}

VariableSelector::~VariableSelector() = default;

void VariableSelector::initUI() {
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
    createHeader();
    createBrowsers();
    createNavigationButtons();
}

void VariableSelector::createHeader() {
    auto* title = new Fl_Box(x(), y() + MARGIN, w(), HEADER_HEIGHT, "Step 3: Select Variables");
    title->align(FL_ALIGN_CENTER);
    title->labelsize(16);
    title->labelfont(FL_BOLD);

    auto* desc = new Fl_Box(x() + MARGIN, y() + MARGIN + HEADER_HEIGHT,
                          w() - 2 * MARGIN, DESC_HEIGHT,
                          "Select input and target variables for your regression model:");
    desc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    desc->labelsize(12);
}

void VariableSelector::createBrowsers() {
    createLabeledBrowser(layout->x + MARGIN, "Available Variables",
                        availableVariablesBrowser, availableBrowserCallback,
                        layout->componentY, layout->componentHeight);

    addButton = createButton(layout->buttonsX, layout->componentY + layout->componentHeight/3,
                           ">", addButtonCallback);
    removeButton = createButton(layout->buttonsX, layout->componentY + 2*layout->componentHeight/3,
                              "<", removeButtonCallback);

    createLabeledBrowser(layout->selectedX, "Selected Input Variables",
                        selectedVariablesBrowser, selectedBrowserCallback,
                        layout->componentY, layout->componentHeight);

    createTargetBrowser();

    variableInfoBox = new Fl_Box(layout->x + MARGIN,
                                layout->componentY + layout->componentHeight - 80,
                                layout->browserWidth, 80, "");
    variableInfoBox->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    variableInfoBox->box(FL_BORDER_BOX);
    variableInfoBox->labelsize(11);
}

void VariableSelector::createLabeledBrowser(int x, const char* label, Fl_Browser*& browser,
                                          Fl_Callback* cb, int y, int h) {
    auto* lbl = new Fl_Box(x, y, layout->browserWidth, LABEL_HEIGHT, label);
    lbl->align(FL_ALIGN_LEFT);
    lbl->labelsize(12);

    browser = new Fl_Browser(x, y + LABEL_HEIGHT, layout->browserWidth, h - LABEL_HEIGHT);
    browser->type(FL_HOLD_BROWSER);
    browser->callback(cb, this);
}

void VariableSelector::createTargetBrowser() {
    auto* label = new Fl_Box(layout->targetX, layout->componentY,
                           layout->browserWidth, LABEL_HEIGHT, "Target Variable");
    label->align(FL_ALIGN_LEFT);
    label->labelsize(12);

    auto* desc = new Fl_Box(layout->targetX, layout->componentY + LABEL_HEIGHT,
                          layout->browserWidth, DESC_HEIGHT - 5,
                          "Select dependent variable:");
    desc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    desc->labelsize(11);

    targetVariableBrowser = new Fl_Browser(layout->targetX,
                                         layout->componentY + LABEL_HEIGHT + DESC_HEIGHT - 5,
                                         layout->browserWidth,
                                         layout->componentHeight - LABEL_HEIGHT - DESC_HEIGHT + 5);
    targetVariableBrowser->type(FL_HOLD_BROWSER);
    targetVariableBrowser->callback(targetBrowserCallback, this);
}

Fl_Button* VariableSelector::createButton(int x, int y, const char* label, Fl_Callback* cb) {
    auto* btn = new Fl_Button(x, y, BUTTON_HEIGHT, BUTTON_HEIGHT, label);
    btn->callback(cb, this);
    btn->deactivate();
    return btn;
}

void VariableSelector::createNavigationButtons() {
    backButton = new Fl_Button(layout->x + MARGIN, layout->bottomY,
                              100, BOTTOM_BUTTONS_HEIGHT, "Back");
    backButton->callback(backButtonCallback_static, this);

    runButton = new Fl_Button(layout->x + layout->w - MARGIN - 150, layout->bottomY,
                             150, BOTTOM_BUTTONS_HEIGHT, "Run Regression");
    runButton->callback(runButtonCallback, this);
    runButton->deactivate();
}

void VariableSelector::setAvailableVariables(const std::vector<std::string>& variables) {
    availableVariablesBrowser->clear();
    selectedVariablesBrowser->clear();
    targetVariableBrowser->clear();

    for (const auto& var : variables) {
        availableVariablesBrowser->add(var.c_str());
        targetVariableBrowser->add(var.c_str());
    }

    variableInfoBox->label("");
    updateRunButtonState();
}

void VariableSelector::setVariablesSelectedCallback(
    std::function<void(const std::vector<std::string>&, const std::string&)> callback) {
    variablesSelectedCallback = std::move(callback);
}

void VariableSelector::setBackButtonCallback(std::function<void()> callback) {
    backButtonCallback = std::move(callback);
}

void VariableSelector::addButtonCallback(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleAddVariableClick();
}

void VariableSelector::removeButtonCallback(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleRemoveVariableClick();
}

void VariableSelector::runButtonCallback(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleRunButtonClick();
}

void VariableSelector::backButtonCallback_static(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleBackButtonClick();
}

void VariableSelector::availableBrowserCallback(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleAvailableVariableSelectionChange();
}

void VariableSelector::selectedBrowserCallback(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleSelectedVariableSelectionChange();
}

void VariableSelector::targetBrowserCallback(Fl_Widget*, void* data) {
    static_cast<VariableSelector*>(data)->handleTargetVariableChange();
}

void VariableSelector::handleAddVariableClick() {
    int selected = availableVariablesBrowser->value();
    if (!selected) return;

    const char* var = availableVariablesBrowser->text(selected);
    if (!var) return;

    for (int i = 1; i <= selectedVariablesBrowser->size(); ++i) {
        if (strcmp(selectedVariablesBrowser->text(i), var) == 0) return;
    }

    selectedVariablesBrowser->add(var);
    updateRunButtonState();
}

void VariableSelector::handleRemoveVariableClick() {
    int selected = selectedVariablesBrowser->value();
    if (!selected) return;

    selectedVariablesBrowser->remove(selected);
    updateRunButtonState();
    removeButton->deactivate();
}

void VariableSelector::handleRunButtonClick() {
    std::vector<std::string> inputs;
    for (int i = 1; i <= selectedVariablesBrowser->size(); ++i) {
        if (const char* text = selectedVariablesBrowser->text(i)) {
            inputs.emplace_back(text);
        }
    }

    int targetIdx = targetVariableBrowser->value();
    if (targetIdx <= 0) return;

    const char* target = targetVariableBrowser->text(targetIdx);
    if (!target) return;

    if (std::find(inputs.begin(), inputs.end(), target) != inputs.end()) {
        fl_alert("Target variable cannot be an input variable.");
        return;
    }

    if (variablesSelectedCallback) {
        variablesSelectedCallback(inputs, target);
    }
}

void VariableSelector::handleBackButtonClick() {
    if (backButtonCallback) backButtonCallback();
}

void VariableSelector::handleAvailableVariableSelectionChange() {
    int selected = availableVariablesBrowser->value();
    if (selected) {
        addButton->activate();
        if (const char* var = availableVariablesBrowser->text(selected)) {
            showVariableInfo(var);
        }
    } else {
        addButton->deactivate();
        variableInfoBox->label("");
    }
}

void VariableSelector::handleSelectedVariableSelectionChange() {
    int selected = selectedVariablesBrowser->value();
    if (selected) {
        removeButton->activate();
        if (const char* var = selectedVariablesBrowser->text(selected)) {
            showVariableInfo(var);
        }
    } else {
        removeButton->deactivate();
    }
}

void VariableSelector::handleTargetVariableChange() {
    updateRunButtonState();
    int selected = targetVariableBrowser->value();
    if (selected) {
        if (const char* var = targetVariableBrowser->text(selected)) {
            showVariableInfo(var);
        }
    }
}

void VariableSelector::updateRunButtonState() {
    bool hasInputs = selectedVariablesBrowser->size() > 0;
    bool hasTarget = targetVariableBrowser->value() > 0;
    (hasInputs && hasTarget) ? runButton->activate() : runButton->deactivate();
}

void VariableSelector::showVariableInfo(const std::string& variableName) {
    std::string info = "Variable: " + variableName;
    variableInfoBox->copy_label(info.c_str());
    variableInfoBox->redraw();
}