#include "gui/HyperparameterSelector.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <sstream>

HyperparameterSelector::HyperparameterSelector(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), currentModelType("")
{
    begin();
    
    // Set user_data to this, needed for callbacks
    user_data(this);
    
    int margin = 20;
    
    // Create title label with copy_label
    titleLabel = new Fl_Box(x + margin, y + margin, w - 2*margin, 40);
    titleLabel->copy_label("Model Hyperparameters");
    titleLabel->labelfont(FL_BOLD);
    titleLabel->labelsize(16);
    titleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    
    // Create description label with copy_label
    descriptionLabel = new Fl_Box(x + margin, y + margin + 40, w - 2*margin, 40);
    descriptionLabel->copy_label("Configure the hyperparameters for the selected model:");
    descriptionLabel->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_WRAP);
    
    // Create parameters group
    int paramY = y + margin + 90;
    int paramH = h - paramY - margin - 60;
    parametersGroup = new Fl_Group(x + margin, paramY, w - 2*margin, paramH);
    parametersGroup->box(FL_BORDER_BOX);
    parametersGroup->end();
    
    // Create navigation buttons with copy_label
    backButton = new Fl_Button(x + margin, y + h - margin - 40, 100, 40);
    backButton->copy_label("Back");
    backButton->callback(backButtonCallback, this);
    
    nextButton = new Fl_Button(x + w - margin - 100, y + h - margin - 40, 100, 40);
    nextButton->copy_label("Next");
    nextButton->callback(nextButtonCallback, this);
    
    end();
    
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
}

HyperparameterSelector::~HyperparameterSelector()
{
    // Free allocated parameter name strings in autoToggle widgets
    for (auto& param : paramWidgets) {
        if (param.autoToggle && param.autoToggle->user_data()) {
            free(param.autoToggle->user_data());
        }
    }
    
    clearUI();
}

void HyperparameterSelector::setModelType(const std::string& modelType)
{
    if (currentModelType != modelType) {
        currentModelType = modelType;
        clearUI();
        buildUIForModelType();
    }
}

void HyperparameterSelector::clearUI()
{
    parametersGroup->begin();
    
    // Remove all parameter widgets
    for (auto& param : paramWidgets) {
        if (param.widget) {
            parametersGroup->remove(param.widget);
            delete param.widget;
        }
        if (param.autoToggle) {
            // Free the parameter name string if it exists
            if (param.autoToggle->user_data()) {
                free(param.autoToggle->user_data());
            }
            parametersGroup->remove(param.autoToggle);
            delete param.autoToggle;
        }
    }
    
    paramWidgets.clear();
    parametersGroup->end();
    parametersGroup->redraw();
}

void HyperparameterSelector::buildUIForModelType()
{
    // Use a persistent string for the title label
    std::string titleStr = "Configure " + currentModelType + " Hyperparameters";
    titleLabel->copy_label(titleStr.c_str());
    
    if (currentModelType == "Linear Regression") {
        createLinearRegressionUI();
    } else if (currentModelType == "ElasticNet") {
        createElasticNetUI();
    } else if (currentModelType == "XGBoost") {
        createXGBoostUI();
    } else if (currentModelType == "Random Forest") {
        createRandomForestUI();
    } else if (currentModelType == "Neural Network") {
        createNeuralNetworkUI();
    } else if (currentModelType == "Gradient Boosting") {
        createGradientBoostingUI();
    }
    
    redraw();
}

void HyperparameterSelector::createLinearRegressionUI()
{
    // Linear regression doesn't have hyperparameters to configure
    parametersGroup->begin();
    
    Fl_Box* infoBox = new Fl_Box(parametersGroup->x() + 20, parametersGroup->y() + 20, 
                                parametersGroup->w() - 40, 40);
    infoBox->copy_label("Linear Regression has no hyperparameters to configure.");
    infoBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    
    parametersGroup->end();
}

void HyperparameterSelector::createElasticNetUI()
{
    parametersGroup->begin();
    
    int yPos = parametersGroup->y() + 20;
    int xPos = parametersGroup->x() + 20;
    int width = parametersGroup->w() - 40;
    
    // Add alpha parameter (controls L1 vs L2 ratio)
    addSliderParam("alpha", "Alpha (L1 ratio):", 0.0, 1.0, 0.5);
    addAutoToggle("alpha");
    
    // Add lambda parameter (regularization strength)
    addSliderParam("lambda", "Lambda (regularization strength):", 0.0, 10.0, 1.0);
    addAutoToggle("lambda");
    
    // Add max iterations
    addIntSliderParam("max_iter", "Maximum Iterations:", 100, 10000, 1000);
    addAutoToggle("max_iter");
    
    // Add tolerance
    addSliderParam("tol", "Tolerance:", 0.0001, 0.1, 0.0001, 0.0001);
    addAutoToggle("tol");
    
    parametersGroup->end();
}

void HyperparameterSelector::createXGBoostUI()
{
    parametersGroup->begin();
    
    // Add learning rate
    addSliderParam("learning_rate", "Learning Rate:", 0.01, 1.0, 0.1);
    addAutoToggle("learning_rate");
    
    // Add max depth
    addIntSliderParam("max_depth", "Maximum Tree Depth:", 1, 15, 6);
    addAutoToggle("max_depth");
    
    // Add n_estimators
    addIntSliderParam("n_estimators", "Number of Estimators:", 50, 1000, 100);
    addAutoToggle("n_estimators");
    
    // Add subsample
    addSliderParam("subsample", "Subsample Ratio:", 0.1, 1.0, 1.0);
    addAutoToggle("subsample");
    
    // Add colsample_bytree
    addSliderParam("colsample_bytree", "Column Sample by Tree:", 0.1, 1.0, 1.0);
    addAutoToggle("colsample_bytree");
    
    // Add min_child_weight
    addIntSliderParam("min_child_weight", "Minimum Child Weight:", 1, 10, 1);
    addAutoToggle("min_child_weight");
    
    // Add gamma (min_split_loss)
    addSliderParam("gamma", "Gamma (Minimum Split Loss):", 0.0, 10.0, 0.0);
    addAutoToggle("gamma");
    
    parametersGroup->end();
}

void HyperparameterSelector::createRandomForestUI()
{
    parametersGroup->begin();
    
    // Add n_estimators
    addIntSliderParam("n_estimators", "Number of Trees:", 10, 500, 100);
    addAutoToggle("n_estimators");
    
    // Add max_depth
    addIntSliderParam("max_depth", "Maximum Tree Depth:", 1, 30, 10);
    addAutoToggle("max_depth");
    
    // Add min_samples_split
    addIntSliderParam("min_samples_split", "Minimum Samples to Split:", 2, 20, 2);
    addAutoToggle("min_samples_split");
    
    // Add min_samples_leaf
    addIntSliderParam("min_samples_leaf", "Minimum Samples per Leaf:", 1, 20, 1);
    addAutoToggle("min_samples_leaf");
    
    // Add max_features
    std::vector<std::string> featureOptions = {"auto", "sqrt", "log2", "all"};
    addChoiceParam("max_features", "Maximum Features to Consider:", featureOptions, 0);
    addAutoToggle("max_features");
    
    // Add bootstrap option
    addCheckParam("bootstrap", "Use Bootstrap Sampling", true);
    addAutoToggle("bootstrap");
    
    parametersGroup->end();
}

void HyperparameterSelector::createNeuralNetworkUI()
{
    parametersGroup->begin();
    
    // Add hidden layers structure
    addTextParam("hidden_layer_sizes", "Hidden Layer Sizes (comma-separated):", "10,10");
    addAutoToggle("hidden_layer_sizes");
    
    // Add activation function
    std::vector<std::string> activationOptions = {"relu", "tanh", "sigmoid", "identity"};
    addChoiceParam("activation", "Activation Function:", activationOptions, 0);
    addAutoToggle("activation");
    
    // Add learning rate
    addSliderParam("learning_rate", "Learning Rate:", 0.001, 0.1, 0.01, 0.001);
    addAutoToggle("learning_rate");
    
    // Add max iterations
    addIntSliderParam("max_iter", "Maximum Iterations:", 100, 10000, 1000);
    addAutoToggle("max_iter");
    
    // Add batch size
    addIntSliderParam("batch_size", "Batch Size:", 8, 256, 32);
    addAutoToggle("batch_size");
    
    // Add solver
    std::vector<std::string> solverOptions = {"adam", "sgd", "lbfgs"};
    addChoiceParam("solver", "Solver:", solverOptions, 0);
    addAutoToggle("solver");
    
    // Add alpha (L2 penalty)
    addSliderParam("alpha", "Alpha (L2 penalty):", 0.0001, 0.01, 0.0001, 0.0001);
    addAutoToggle("alpha");
    
    parametersGroup->end();
}

void HyperparameterSelector::createGradientBoostingUI()
{
    parametersGroup->begin();
    
    // Add learning rate
    addSliderParam("learning_rate", "Learning Rate:", 0.01, 1.0, 0.1);
    addAutoToggle("learning_rate");
    
    // Add n_estimators
    addIntSliderParam("n_estimators", "Number of Estimators:", 50, 500, 100);
    addAutoToggle("n_estimators");
    
    // Add max_depth
    addIntSliderParam("max_depth", "Maximum Tree Depth:", 1, 15, 3);
    addAutoToggle("max_depth");
    
    // Add min_samples_split
    addIntSliderParam("min_samples_split", "Minimum Samples to Split:", 2, 20, 2);
    addAutoToggle("min_samples_split");
    
    // Add min_samples_leaf
    addIntSliderParam("min_samples_leaf", "Minimum Samples per Leaf:", 1, 20, 1);
    addAutoToggle("min_samples_leaf");
    
    // Add subsample
    addSliderParam("subsample", "Subsample Ratio:", 0.1, 1.0, 1.0);
    addAutoToggle("subsample");
    
    // Add loss function
    std::vector<std::string> lossOptions = {"squared_error", "absolute_error", "huber", "quantile"};
    addChoiceParam("loss", "Loss Function:", lossOptions, 0);
    addAutoToggle("loss");
    
    parametersGroup->end();
}

void HyperparameterSelector::addSliderParam(const std::string& name, const std::string& label, 
                                         double min, double max, double value, double step)
{
    int paramCount = paramWidgets.size();
    int y = parametersGroup->y() + 20 + (paramCount * 60);
    int labelWidth = 200;
    int sliderWidth = parametersGroup->w() - labelWidth - 60;
    
    // Create label - make a copy of the label string to ensure it remains valid
    Fl_Box* paramLabel = new Fl_Box(parametersGroup->x() + 20, y, labelWidth, 25);
    paramLabel->copy_label(label.c_str());  // Use copy_label instead of label
    paramLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Create slider
    Fl_Value_Slider* slider = new Fl_Value_Slider(parametersGroup->x() + 20 + labelWidth, y, sliderWidth, 25);
    slider->type(FL_HOR_NICE_SLIDER);
    slider->minimum(min);
    slider->maximum(max);
    slider->value(value);
    slider->step(step);
    slider->precision(5);
    
    ParamWidgets param;
    param.name = name;
    param.widget = slider;
    paramWidgets.push_back(param);
}

void HyperparameterSelector::addIntSliderParam(const std::string& name, const std::string& label, 
                                           int min, int max, int value)
{
    int paramCount = paramWidgets.size();
    int y = parametersGroup->y() + 20 + (paramCount * 60);
    int labelWidth = 200;
    int sliderWidth = parametersGroup->w() - labelWidth - 60;
    
    // Create label - make a copy of the label string to ensure it remains valid
    Fl_Box* paramLabel = new Fl_Box(parametersGroup->x() + 20, y, labelWidth, 25);
    paramLabel->copy_label(label.c_str());  // Use copy_label instead of label
    paramLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Create slider
    Fl_Value_Slider* slider = new Fl_Value_Slider(parametersGroup->x() + 20 + labelWidth, y, sliderWidth, 25);
    slider->type(FL_HOR_NICE_SLIDER);
    slider->minimum(static_cast<double>(min));
    slider->maximum(static_cast<double>(max));
    slider->value(static_cast<double>(value));
    slider->step(1);
    slider->precision(0);
    
    ParamWidgets param;
    param.name = name;
    param.widget = slider;
    paramWidgets.push_back(param);
}

void HyperparameterSelector::addChoiceParam(const std::string& name, const std::string& label, 
                                        const std::vector<std::string>& options, int defaultIndex)
{
    int paramCount = paramWidgets.size();
    int y = parametersGroup->y() + 20 + (paramCount * 60);
    int labelWidth = 200;
    int choiceWidth = 200;
    
    // Create label - make a copy of the label string
    Fl_Box* paramLabel = new Fl_Box(parametersGroup->x() + 20, y, labelWidth, 25);
    paramLabel->copy_label(label.c_str());
    paramLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Create choice
    Fl_Choice* choice = new Fl_Choice(parametersGroup->x() + 20 + labelWidth, y, choiceWidth, 25);
    
    // Add options
    for (const auto& option : options) {
        choice->add(option.c_str());
    }
    
    // Set default
    if (defaultIndex >= 0 && defaultIndex < static_cast<int>(options.size())) {
        choice->value(defaultIndex);
    }
    
    ParamWidgets param;
    param.name = name;
    param.widget = choice;
    paramWidgets.push_back(param);
}

void HyperparameterSelector::addCheckParam(const std::string& name, const std::string& label, bool defaultValue)
{
    int paramCount = paramWidgets.size();
    int y = parametersGroup->y() + 20 + (paramCount * 60);
    
    // Create checkbox - make a copy of the label
    Fl_Check_Button* check = new Fl_Check_Button(parametersGroup->x() + 20, y, 
                                              parametersGroup->w() - 40, 25);
    check->copy_label(label.c_str());
    check->value(defaultValue ? 1 : 0);
    
    ParamWidgets param;
    param.name = name;
    param.widget = check;
    paramWidgets.push_back(param);
}

void HyperparameterSelector::addTextParam(const std::string& name, const std::string& label, const std::string& defaultValue)
{
    int paramCount = paramWidgets.size();
    int y = parametersGroup->y() + 20 + (paramCount * 60);
    int labelWidth = 200;
    int inputWidth = 200;
    
    // Create label - make a copy of the label
    Fl_Box* paramLabel = new Fl_Box(parametersGroup->x() + 20, y, labelWidth, 25);
    paramLabel->copy_label(label.c_str());
    paramLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Create input
    Fl_Input* input = new Fl_Input(parametersGroup->x() + 20 + labelWidth, y, inputWidth, 25);
    input->value(defaultValue.c_str());
    
    ParamWidgets param;
    param.name = name;
    param.widget = input;
    paramWidgets.push_back(param);
}

void HyperparameterSelector::addAutoToggle(const std::string& paramName)
{
    // Find the parameter widget
    for (auto& param : paramWidgets) {
        if (param.name == paramName && !param.autoToggle) {
            int y = param.widget->y();
            int x = param.widget->x() + param.widget->w() + 10;
            
            // Create the auto checkbox with copy_label
            Fl_Check_Button* autoToggle = new Fl_Check_Button(x, y, 100, 25);
            autoToggle->copy_label("Auto");
            autoToggle->callback(autoToggleCallback, this);
            
            // Store parameter name in a way that won't cause memory corruption
            // We use a copy of the string to ensure it remains valid
            char* paramNameCopy = strdup(paramName.c_str());
            autoToggle->user_data(paramNameCopy);
            
            param.autoToggle = autoToggle;
            break;
        }
    }
}

void HyperparameterSelector::updateParamVisibility(const std::string& paramName, bool autoEnabled)
{
    for (auto& param : paramWidgets) {
        if (param.name == paramName) {
            if (autoEnabled) {
                param.widget->deactivate();
            } else {
                param.widget->activate();
            }
            break;
        }
    }
}

void HyperparameterSelector::handleAutoToggle(const std::string& paramName)
{
    for (auto& param : paramWidgets) {
        if (param.name == paramName && param.autoToggle) {
            bool isAuto = (param.autoToggle->value() != 0);
            updateParamVisibility(paramName, isAuto);
            break;
        }
    }
}

void HyperparameterSelector::setHyperparametersSelectedCallback(std::function<void(const std::unordered_map<std::string, std::string>&)> callback)
{
    hyperparametersSelectedCallback = callback;
}

void HyperparameterSelector::setBackButtonCallback(std::function<void()> callback)
{
    backButtonCallbackFn = callback;
}

void HyperparameterSelector::handleNextButtonClick()
{
    if (hyperparametersSelectedCallback) {
        auto params = collectParameters();
        hyperparametersSelectedCallback(params);
    }
}

void HyperparameterSelector::handleBackButtonClick()
{
    if (backButtonCallbackFn) {
        backButtonCallbackFn();
    }
}

std::unordered_map<std::string, std::string> HyperparameterSelector::collectParameters()
{
    std::unordered_map<std::string, std::string> params;
    
    for (const auto& param : paramWidgets) {
        bool isAuto = (param.autoToggle && param.autoToggle->value() != 0);
        
        // Add parameter with special "auto" value if auto is selected
        if (isAuto) {
            params[param.name] = "auto";
            continue;
        }
        
        // Otherwise get the actual value
        if (auto slider = dynamic_cast<Fl_Value_Slider*>(param.widget)) {
            std::ostringstream oss;
            oss << slider->value();
            params[param.name] = oss.str();
        }
        else if (auto choice = dynamic_cast<Fl_Choice*>(param.widget)) {
            params[param.name] = choice->text() ? choice->text() : "";
        }
        else if (auto check = dynamic_cast<Fl_Check_Button*>(param.widget)) {
            params[param.name] = check->value() ? "true" : "false";
        }
        else if (auto input = dynamic_cast<Fl_Input*>(param.widget)) {
            params[param.name] = input->value() ? input->value() : "";
        }
    }
    
    return params;
}

void HyperparameterSelector::nextButtonCallback(Fl_Widget* widget, void* userData)
{
    HyperparameterSelector* self = static_cast<HyperparameterSelector*>(userData);
    self->handleNextButtonClick();
}

void HyperparameterSelector::backButtonCallback(Fl_Widget* widget, void* userData)
{
    HyperparameterSelector* self = static_cast<HyperparameterSelector*>(userData);
    self->handleBackButtonClick();
}

void HyperparameterSelector::autoToggleCallback(Fl_Widget* widget, void* userData)
{
    // The userData here is the HyperparameterSelector instance
    HyperparameterSelector* self = static_cast<HyperparameterSelector*>(userData);
    Fl_Check_Button* checkButton = static_cast<Fl_Check_Button*>(widget);
    
    // Extract the parameter name from checkButton's user_data
    if (self && checkButton && checkButton->user_data()) {
        const char* paramName = static_cast<const char*>(checkButton->user_data());
        self->handleAutoToggle(paramName);
    }
} 