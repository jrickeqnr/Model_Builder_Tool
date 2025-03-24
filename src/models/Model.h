#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <Eigen/Dense>
#include "data/DataFrame.h"

// Forward declaration of ModelType if not already defined
enum class ModelType {
    Regression,
    Classification,
    Neural_Network,
    Random_Forest,
    Gradient_Boosting,
    Ensemble,
    Other
};

/**
 * @brief Base class for statistical models
 * 
 * This abstract class defines the interface for all statistical models
 * in the application.
 */
class Model {
public:
    virtual ~Model() = default;

    /**
     * @brief Fit the model to the given data
     * 
     * @param X Input features (predictor variables)
     * @param y Target variable (response variable)
     * @param variableNames Names of the input variables (features)
     * @param targetName Name of the target variable
     * @return bool True if fitting was successful, false otherwise
     */
    virtual bool fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y, 
                    const std::vector<std::string>& variableNames = {},
                    const std::string& targetName = "") = 0;

    /**
     * @brief Make predictions using the fitted model
     * 
     * @param X Input features for prediction
     * @return Eigen::VectorXd Predicted values
     */
    virtual Eigen::VectorXd predict(const Eigen::MatrixXd& X) const = 0;

    /**
     * @brief Get the name of the model
     * 
     * @return std::string Model name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the model parameters
     * 
     * @return std::unordered_map<std::string, double> Map of parameter names to values
     */
    virtual std::unordered_map<std::string, double> getParameters() const = 0;

    /**
     * @brief Get the model statistics
     * 
     * @return std::unordered_map<std::string, double> Map of statistic names to values
     */
    virtual std::unordered_map<std::string, double> getStatistics() const = 0;

    /**
     * @brief Get a description of the model
     * 
     * @return std::string Model description
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Get the names of input variables
     * 
     * @return std::vector<std::string> Names of input variables
     */
    virtual std::vector<std::string> getVariableNames() const = 0;

    /**
     * @brief Get the name of the target variable
     * 
     * @return std::string Name of target variable
     */
    virtual std::string getTargetName() const = 0;

    /**
     * @brief Get the feature importance scores
     * 
     * This method returns a map of feature names to their importance scores.
     * The interpretation of importance scores depends on the specific model implementation.
     * For linear regression, this is based on the absolute values of standardized coefficients.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    virtual std::unordered_map<std::string, double> getFeatureImportance() const = 0;
    
    /**
     * @brief Get the learning curve data
     * 
     * This method populates vectors with training set sizes, training scores, and validation scores
     * for plotting a learning curve. Default implementation returns empty vectors.
     * 
     * @param trainingSizes Vector to store training set sizes
     * @param trainingScores Vector to store training scores
     * @param validationScores Vector to store validation scores
     */
    virtual void getLearningCurve(std::vector<double>& trainingSizes,
                                 std::vector<double>& trainingScores,
                                 std::vector<double>& validationScores) const {
        // Default implementation returns empty vectors
        trainingSizes.clear();
        trainingScores.clear();
        validationScores.clear();
    }
    
    /**
     * @brief Check if the model supports learning curve visualization
     * 
     * @return bool True if learning curve visualization is supported, false otherwise
     */
    virtual bool supportsLearningCurve() const {
        return false;  // Default implementation does not support learning curves
    }
    
    /**
     * @brief Check if the model supports neural network architecture visualization
     * 
     * @return bool True if architecture visualization is supported, false otherwise
     */
    virtual bool supportsArchitectureVisualization() const {
        return false;  // Default implementation does not support architecture visualization
    }
    
    /**
     * @brief Check if the model supports tree visualization
     * 
     * @return bool True if tree visualization is supported, false otherwise
     */
    virtual bool supportsTreeVisualization() const {
        return false;  // Default implementation does not support tree visualization
    }

    /**
     * @brief Get the type of the model
     * 
     * @return ModelType The type of the model
     */
    virtual ModelType getType() const {
        return ModelType::Other;  // Default implementation
    }

    /**
     * @brief Get the DataFrame associated with this model
     * 
     * @return std::shared_ptr<DataFrame> Pointer to the associated DataFrame
     */
    virtual std::shared_ptr<DataFrame> getDataFrame() const {
        return dataFrame;  // Return the stored DataFrame
    }

    /**
     * @brief Set the DataFrame associated with this model
     * 
     * @param df Pointer to the DataFrame
     */
    virtual void setDataFrame(std::shared_ptr<DataFrame> df) {
        dataFrame = df;  // Store the DataFrame
    }

    /**
     * @brief Get a specific metric value
     * 
     * @param metricName Name of the metric to retrieve
     * @return double Value of the metric, or 0.0 if not found
     */
    virtual double getMetric(const std::string& metricName) const {
        auto stats = getStatistics();
        auto it = stats.find(metricName);
        return (it != stats.end()) ? it->second : 0.0;
    }

    /**
     * @brief Get a specific parameter value
     * 
     * @param paramName Name of the parameter to retrieve
     * @return double Value of the parameter, or 0.0 if not found
     */
    virtual double getParameter(const std::string& paramName) const {
        auto params = getParameters();
        auto it = params.find(paramName);
        return (it != params.end()) ? it->second : 0.0;
    }

    /**
     * @brief Check if the model supports feature importance visualization
     * 
     * @return bool True if feature importance visualization is supported, false otherwise
     */
    virtual bool supportsFeatureImportance() const {
        // By default, return true if getFeatureImportance() would return non-empty map
        return !getFeatureImportance().empty();
    }

protected:
    std::shared_ptr<DataFrame> dataFrame;  // Store the DataFrame associated with this model
};