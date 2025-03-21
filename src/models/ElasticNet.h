#pragma once

#include "models/Model.h"

/**
 * @brief ElasticNet regression model
 * 
 * This class implements ElasticNet regression, which combines L1 and L2 regularization
 * to balance between Ridge and Lasso regression methods.
 */
class ElasticNet : public Model {
public:
    ElasticNet();
    
    /**
     * @brief Constructor with regularization parameters
     * 
     * @param alpha L1 ratio (0 for Ridge, 1 for Lasso, between 0-1 for ElasticNet)
     * @param lambda Regularization strength
     * @param maxIter Maximum number of iterations
     * @param tol Tolerance for convergence
     */
    ElasticNet(double alpha, double lambda, int maxIter = 1000, double tol = 0.0001);
    
    ~ElasticNet() override = default;

    /**
     * @brief Fit the ElasticNet model to the given data
     * 
     * @param X Input features (predictor variables)
     * @param y Target variable (response variable)
     * @param variableNames Names of the input variables (features)
     * @param targetName Name of the target variable
     * @return bool True if fitting was successful, false otherwise
     */
    bool fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
            const std::vector<std::string>& variableNames = {},
            const std::string& targetName = "") override;

    /**
     * @brief Make predictions using the fitted ElasticNet model
     * 
     * @param X Input features for prediction
     * @return Eigen::VectorXd Predicted values
     */
    Eigen::VectorXd predict(const Eigen::MatrixXd& X) const override;

    /**
     * @brief Get the name of the model
     * 
     * @return std::string Model name
     */
    std::string getName() const override;

    /**
     * @brief Get the model parameters (coefficients, intercept, and regularization parameters)
     * 
     * @return std::unordered_map<std::string, double> Map of parameter names to values
     */
    std::unordered_map<std::string, double> getParameters() const override;

    /**
     * @brief Get the model statistics (R², adjusted R², etc.)
     * 
     * @return std::unordered_map<std::string, double> Map of statistic names to values
     */
    std::unordered_map<std::string, double> getStatistics() const override;

    /**
     * @brief Get a description of the ElasticNet model
     * 
     * @return std::string Model description
     */
    std::string getDescription() const override;

    /**
     * @brief Get the model coefficients
     * 
     * @return Eigen::VectorXd Vector of coefficients
     */
    Eigen::VectorXd getCoefficients() const;

    /**
     * @brief Get the model intercept
     * 
     * @return double Intercept value
     */
    double getIntercept() const;

    /**
     * @brief Get the coefficient of determination (R²)
     * 
     * @return double R² value
     */
    double getRSquared() const;

    /**
     * @brief Get the adjusted coefficient of determination
     * 
     * @return double Adjusted R² value
     */
    double getAdjustedRSquared() const;

    /**
     * @brief Get the root mean squared error
     * 
     * @return double RMSE value
     */
    double getRMSE() const;

    /**
     * @brief Get the names of input variables
     * 
     * @return std::vector<std::string> Names of input variables
     */
    std::vector<std::string> getVariableNames() const override;

    /**
     * @brief Get the name of the target variable
     * 
     * @return std::string Name of target variable
     */
    std::string getTargetName() const override;

    /**
     * @brief Get the feature importance scores
     * 
     * For ElasticNet, feature importance is calculated using
     * standardized coefficients (absolute values). The coefficients
     * are standardized by scaling them with the standard deviation
     * of their respective features.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    std::unordered_map<std::string, double> getFeatureImportance() const override;

private:
    Eigen::VectorXd coefficients;
    double intercept;
    double rSquared;
    double adjustedRSquared;
    double rmse;
    int nSamples;
    int nFeatures;
    bool isFitted;
    
    // ElasticNet specific parameters
    double alpha;     // L1 ratio (0 for Ridge, 1 for Lasso)
    double lambda;    // Regularization strength
    int maxIter;      // Maximum iterations
    double tol;       // Tolerance for convergence
    
    // Added variable names storage
    std::vector<std::string> inputVariableNames;
    std::string targetVariableName;

    // Store feature standard deviations for importance calculation
    Eigen::VectorXd featureStdDevs;

    /**
     * @brief Calculate model statistics after fitting
     * 
     * @param X Input features used for fitting
     * @param y Target values used for fitting
     */
    void calculateStatistics(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);

    /**
     * @brief Calculate feature standard deviations
     * 
     * @param X Input features matrix
     */
    void calculateFeatureStdDevs(const Eigen::MatrixXd& X);
    
    /**
     * @brief Coordinate descent algorithm for ElasticNet optimization
     * 
     * @param X Input features
     * @param y Target values
     */
    void coordinateDescent(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
}; 