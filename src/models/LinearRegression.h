#pragma once

#include "models/Model.h"

/**
 * @brief Linear Regression model
 * 
 * This class implements a simple linear regression model using ordinary
 * least squares (OLS) for parameter estimation.
 */
class LinearRegression : public Model {
public:
    LinearRegression();
    ~LinearRegression() override = default;

    /**
     * @brief Fit the linear regression model to the given data
     * 
     * @param X Input features (predictor variables)
     * @param y Target variable (response variable)
     * @return bool True if fitting was successful, false otherwise
     */
    bool fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) override;

    /**
     * @brief Make predictions using the fitted linear regression model
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
     * @brief Get the model parameters (coefficients and intercept)
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
     * @brief Get a description of the linear regression model
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

private:
    Eigen::VectorXd coefficients;
    double intercept;
    double rSquared;
    double adjustedRSquared;
    double rmse;
    int nSamples;
    int nFeatures;
    bool isFitted;

    /**
     * @brief Calculate model statistics after fitting
     * 
     * @param X Input features used for fitting
     * @param y Target values used for fitting
     */
    void calculateStatistics(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
};