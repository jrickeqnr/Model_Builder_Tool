#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

/**
 * @brief Widget for setting model hyperparameters
 * 
 * This widget provides UI for configuring hyperparameters for different model types.
 */
class HyperparameterSelector : public Fl_Group {
public:
    HyperparameterSelector(int x, int y, int w, int h);
    ~HyperparameterSelector();

    /**
     * @brief Set the model type to configure hyperparameters for
     * 
     * @param modelType Name of the model type
     */
    void setModelType(const std::string& modelType);

    /**
     * @brief Set the callback for when hyperparameters are selected
     * 
     * @param callback Function to call when hyperparameters are confirmed
     */
    void setHyperparametersSelectedCallback(std::function<void(const std::unordered_map<std::string, std::string>&)> callback);
    
    /**
     * @brief Set the callback for back button
     * 
     * @param callback Function to call when back button is clicked
     */
    void setBackButtonCallback(std::function<void()> callback);

private:
    void clearUI();
    void buildUIForModelType();
    void createElasticNetUI();
    void createXGBoostUI();
    void createRandomForestUI();
    void createNeuralNetworkUI();
    void createGradientBoostingUI();
    void createLinearRegressionUI();
    
    void addSliderParam(const std::string& name, const std::string& label, 
                       double min, double max, double value, double step = 0.01);
    void addIntSliderParam(const std::string& name, const std::string& label, 
                          int min, int max, int value);
    void addChoiceParam(const std::string& name, const std::string& label, 
                       const std::vector<std::string>& options, int defaultIndex = 0);
    void addCheckParam(const std::string& name, const std::string& label, bool defaultValue = false);
    void addTextParam(const std::string& name, const std::string& label, const std::string& defaultValue = "");
    
    // Create automatic parameter toggle
    void addAutoToggle(const std::string& paramName);
    void updateParamVisibility(const std::string& paramName, bool autoEnabled);
    
    void handleNextButtonClick();
    void handleBackButtonClick();
    void handleAutoToggle(const std::string& paramName);
    
    // Helper struct to store parameter information
    struct ParamWidgets {
        std::string name;
        Fl_Widget* widget = nullptr;
        Fl_Check_Button* autoToggle = nullptr;
    };
    
    // Static callbacks
    static void nextButtonCallback(Fl_Widget* widget, void* userData);
    static void backButtonCallback(Fl_Widget* widget, void* userData);
    static void autoToggleCallback(Fl_Widget* widget, void* userData);
    
    // Helper method to find parameter by auto toggle button pointer
    std::string findParamNameByAutoToggle(Fl_Check_Button* autoToggle);
    
    std::string currentModelType;
    
    // UI components
    Fl_Box* titleLabel;
    Fl_Box* descriptionLabel;
    Fl_Group* parametersGroup;
    Fl_Button* nextButton;
    Fl_Button* backButton;
    
    // Store parameter widgets by name
    std::vector<ParamWidgets> paramWidgets;
    
    // Callback functions
    std::function<void(const std::unordered_map<std::string, std::string>&)> hyperparametersSelectedCallback;
    std::function<void()> backButtonCallbackFn;
    
    // Helper methods
    std::unordered_map<std::string, std::string> collectParameters();
}; 