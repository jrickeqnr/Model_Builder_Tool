#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <Eigen/Dense>
#include "data/DataFrame.h"

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
     * @return bool True if fitting was successful, false otherwise
     */
    virtual bool fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) = 0;

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
};