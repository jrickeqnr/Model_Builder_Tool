#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "utils/PlottingUtility.h"

/**
 * @brief Simple widget for creating plots
 * 
 * This class provides functionality for creating plots using Python scripts.
 */
class PlotWidget : public Fl_Group {
public:
    PlotWidget(int x, int y, int w, int h);
    ~PlotWidget();

    /**
     * @brief Create a scatter plot
     */
    void createScatterPlot(const std::vector<double>& actualValues,
                          const std::vector<double>& predictedValues,
                          const std::string& xLabel,
                          const std::string& yLabel,
                          const std::string& title,
                          const std::string& tempDataPath = "temp_plot_data.csv",
                          const std::string& tempImagePath = "temp_plot_image.png",
                          const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a time series plot
     */
    void createTimeseriesPlot(const std::vector<double>& actualValues,
                             const std::vector<double>& predictedValues,
                             const std::string& title,
                             const std::string& tempDataPath = "temp_plot_data.csv",
                             const std::string& tempImagePath = "temp_plot_image.png",
                             const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create an importance plot
     */
    void createImportancePlot(const std::unordered_map<std::string, double>& importance,
                             const std::string& title,
                             const std::string& tempDataPath = "temp_plot_data.csv",
                             const std::string& tempImagePath = "temp_plot_image.png",
                             const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a residual plot
     */
    void createResidualPlot(const std::vector<double>& actualValues,
                           const std::vector<double>& predictedValues,
                           const std::string& title,
                           const std::string& tempDataPath = "temp_plot_data.csv",
                           const std::string& tempImagePath = "temp_plot_image.png",
                           const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a learning curve plot
     */
    void createLearningCurvePlot(const std::vector<double>& trainingScores,
                                const std::vector<double>& validationScores,
                                const std::vector<int>& trainingSizes,
                                const std::string& title,
                                const std::string& tempDataPath = "temp_plot_data.csv",
                                const std::string& tempImagePath = "temp_plot_image.png",
                                const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a neural network architecture plot
     */
    void createNeuralNetworkArchitecturePlot(const std::vector<int>& layerSizes,
                                            const std::string& title,
                                            const std::string& tempDataPath = "temp_plot_data.csv",
                                            const std::string& tempImagePath = "temp_plot_image.png",
                                            const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Create a tree visualization plot
     */
    void createTreeVisualizationPlot(const std::string& treeStructure,
                                    const std::string& title,
                                    const std::string& tempDataPath = "temp_plot_data.csv",
                                    const std::string& tempImagePath = "temp_plot_image.png",
                                    const std::string& tempScriptPath = "temp_plot_script.py");

    /**
     * @brief Save the current plot to a file
     */
    bool savePlot(const std::string& filename);

    /**
     * @brief Create temporary file paths
     */
    void createTempFilePaths(const std::string& baseName,
                            std::string& dataPath,
                            std::string& imagePath, 
                            std::string& scriptPath);

    /**
     * @brief Override resize to regenerate plot
     */
    void resize(int x, int y, int w, int h) override;

    /**
     * @brief Override draw to display the plot
     */
    void draw() override;

private:
    enum class PlotType {
        None,
        Scatter,
        Timeseries,
        Importance,
        Residual,
        LearningCurve,
        NeuralNetworkArchitecture,
        TreeVisualization
    };

    // Helper methods
    bool createTempDataFile(const std::string& data, const std::string& filename);
    static void resizeTimeoutCallback(void* v);
    void regeneratePlot();
    bool executePythonScript(const std::string& scriptPath, const std::string& tempDataPath, const std::string& tempImagePath);
    std::string formatPathForPython(const std::string& path);

    // Member variables
    Fl_Box* plotBox;
    char* plotImageData;
    int plotImageWidth;
    int plotImageHeight;
    PlotType currentPlotType;
    std::unordered_map<std::string, std::string> tempFilePaths;
    
    // Variables for resize handling
    bool resizeTimerActive;
    int pendingWidth;
    int pendingHeight;
    static const double RESIZE_DELAY;

    // Stored data for regeneration
    std::vector<double> storedActualValues;
    std::vector<double> storedPredictedValues;
    std::string storedXLabel;
    std::string storedYLabel;
    std::string storedTitle;
    std::unordered_map<std::string, double> storedImportance;
    std::vector<double> storedTrainingScores;
    std::vector<double> storedValidationScores;
    std::vector<int> storedTrainingSizes;
    std::vector<int> storedLayerSizes;
    std::string storedTreeStructure;
}; 