#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <functional>
#include <memory>
#include <vector>
#include <string>

/**
 * @brief Widget for variable selection
 * 
 * This widget provides UI for selecting the input and target variables for regression.
 */
class VariableSelector : public Fl_Group {
public:
    /**
     * @brief Construct a new VariableSelector widget
     * 
     * @param x X position
     * @param y Y position
     * @param w Width
     * @param h Height
     */
    VariableSelector(int x, int y, int w, int h);
    
    /**
     * @brief Destroy the VariableSelector widget
     */
    ~VariableSelector() override;

    /**
     * @brief Set the available variables from the dataset
     * 
     * @param variables List of variable names
     */
    void setAvailableVariables(const std::vector<std::string>& variables);
    
    /**
     * @brief Set the callback function for variables selected
     * 
     * @param callback Function to call when variables are selected
     */
    void setVariablesSelectedCallback(
        std::function<void(const std::vector<std::string>&, const std::string&)> callback);
    
    /**
     * @brief Set the callback function for back button
     * 
     * @param callback Function to call when back button is clicked
     */
    void setBackButtonCallback(std::function<void()> callback);

private:
    struct Layout;
    std::unique_ptr<Layout> layout;

    Fl_Browser* availableVariablesBrowser{};
    Fl_Browser* selectedVariablesBrowser{};
    Fl_Browser* targetVariableBrowser{};
    Fl_Button* addButton{};
    Fl_Button* removeButton{};
    Fl_Button* runButton{};
    Fl_Button* backButton{};
    Fl_Box* variableInfoBox{};

    std::function<void(const std::vector<std::string>&, const std::string&)> variablesSelectedCallback;
    std::function<void()> backButtonCallback;

    void initUI();
    void createHeader();
    void createBrowsers();
    void createNavigationButtons();
    
    void createLabeledBrowser(int x, const char* label, Fl_Browser*& browser, Fl_Callback* cb, int y, int h);
    void createTargetBrowser();
    Fl_Button* createButton(int x, int y, const char* label, Fl_Callback* cb);

    // Static callbacks for FLTK
    static void addButtonCallback(Fl_Widget* widget, void* userData);
    static void removeButtonCallback(Fl_Widget* widget, void* userData);
    static void runButtonCallback(Fl_Widget* widget, void* userData);
    static void backButtonCallback_static(Fl_Widget* widget, void* userData);
    static void availableBrowserCallback(Fl_Widget* widget, void* userData);
    static void selectedBrowserCallback(Fl_Widget* widget, void* userData);
    static void targetBrowserCallback(Fl_Widget* widget, void* userData);

    /**
     * @brief Handle add variable button click
     */
    void handleAddVariableClick();
    
    /**
     * @brief Handle remove variable button click
     */
    void handleRemoveVariableClick();
    
    /**
     * @brief Handle target variable selection change
     */
    void handleTargetVariableChange();
    
    /**
     * @brief Handle available variable selection change
     */
    void handleAvailableVariableSelectionChange();
    
    /**
     * @brief Handle selected variable selection change
     */
    void handleSelectedVariableSelectionChange();
    
    /**
     * @brief Handle run button click
     */
    void handleRunButtonClick();
    
    /**
     * @brief Handle back button click
     */
    void handleBackButtonClick();
    
    /**
     * @brief Update the run button enabled state
     */
    void updateRunButtonState();
    
    /**
     * @brief Show variable information
     * 
     * @param variableName Name of the variable
     */
    void showVariableInfo(const std::string& variableName);
};