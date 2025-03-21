#include "models/LinearRegression.h"
#include <cmath>
#include <iostream>

LinearRegression::LinearRegression() 
    : intercept(0.0), rSquared(0.0), adjustedRSquared(0.0), rmse(0.0),
      nSamples(0), nFeatures(0), isFitted(false) {
}

bool LinearRegression::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
                          const std::vector<std::string>& variableNames,
                          const std::string& targetName) {
    if (X.rows() != y.rows()) {
        std::cerr << "Error: Number of samples in X (" << X.rows() 
                 << ") does not match number of samples in y (" << y.rows() << ")." << std::endl;
        return false;
    }

    if (X.rows() <= X.cols()) {
        std::cerr << "Error: Number of samples (" << X.rows() 
                 << ") must be greater than number of features (" << X.cols() << ")." << std::endl;
        return false;
    }

    try {
        nSamples = X.rows();
        nFeatures = X.cols();

        // Store variable names
        if (variableNames.size() == 0) {
            // If no variable names provided, generate default ones
            inputVariableNames.clear();
            for (int i = 0; i < nFeatures; ++i) {
                inputVariableNames.push_back("Variable_" + std::to_string(i+1));
            }
        } else if (variableNames.size() != nFeatures) {
            std::cerr << "Warning: Number of variable names (" << variableNames.size()
                     << ") does not match number of features (" << nFeatures 
                     << "). Using default names." << std::endl;
            
            // Generate default names
            inputVariableNames.clear();
            for (int i = 0; i < nFeatures; ++i) {
                inputVariableNames.push_back("Variable_" + std::to_string(i+1));
            }
        } else {
            // Store the provided variable names
            inputVariableNames = variableNames;
        }
        
        // Store target variable name
        targetVariableName = targetName.empty() ? "Target" : targetName;

        // Calculate feature standard deviations for importance calculation
        calculateFeatureStdDevs(X);

        // Add a column of ones to X for the intercept
        Eigen::MatrixXd X_aug(nSamples, nFeatures + 1);
        X_aug.col(0).setOnes();
        X_aug.rightCols(nFeatures) = X;

        // Compute coefficients using normal equation: (X'X)^(-1)X'y
        Eigen::VectorXd theta = (X_aug.transpose() * X_aug).inverse() * X_aug.transpose() * y;

        // Extract intercept and coefficients
        intercept = theta(0);
        coefficients = theta.tail(nFeatures);

        // Set isFitted to true before calling functions that depend on it
        isFitted = true;
        
        // Calculate statistics after setting isFitted to true
        calculateStatistics(X, y);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting linear regression model: " << e.what() << std::endl;
        return false;
    }
}

Eigen::VectorXd LinearRegression::predict(const Eigen::MatrixXd& X) const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }

    if (X.cols() != nFeatures) {
        throw std::invalid_argument("Number of features in X (" + std::to_string(X.cols()) + 
                                   ") does not match the number of features the model was trained on (" + 
                                   std::to_string(nFeatures) + ")");
    }

    return X * coefficients + Eigen::VectorXd::Constant(X.rows(), intercept);
}

std::string LinearRegression::getName() const {
    return "Linear Regression";
}

std::unordered_map<std::string, double> LinearRegression::getParameters() const {
    std::unordered_map<std::string, double> params;
    params["intercept"] = intercept;
    
    for (int i = 0; i < coefficients.size(); ++i) {
        // Use variable names instead of generic coefficient_N names
        if (i < inputVariableNames.size()) {
            params[inputVariableNames[i]] = coefficients(i);
        } else {
            params["coefficient_" + std::to_string(i)] = coefficients(i);
        }
    }
    
    return params;
}

std::unordered_map<std::string, double> LinearRegression::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    stats["r_squared"] = rSquared;
    stats["adjusted_r_squared"] = adjustedRSquared;
    stats["rmse"] = rmse;
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    
    return stats;
}

std::string LinearRegression::getDescription() const {
    return "Ordinary Least Squares (OLS) Linear Regression model.";
}

Eigen::VectorXd LinearRegression::getCoefficients() const {
    return coefficients;
}

double LinearRegression::getIntercept() const {
    return intercept;
}

double LinearRegression::getRSquared() const {
    return rSquared;
}

double LinearRegression::getAdjustedRSquared() const {
    return adjustedRSquared;
}

double LinearRegression::getRMSE() const {
    return rmse;
}

std::vector<std::string> LinearRegression::getVariableNames() const {
    return inputVariableNames;
}

std::string LinearRegression::getTargetName() const {
    return targetVariableName;
}

void LinearRegression::calculateStatistics(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    // Get predictions
    Eigen::VectorXd y_pred = predict(X);
    
    // Calculate SST (total sum of squares)
    double y_mean = y.mean();
    double sst = (y.array() - y_mean).square().sum();
    
    // Calculate SSR (regression sum of squares)
    double ssr = (y_pred.array() - y_mean).square().sum();
    
    // Calculate SSE (error sum of squares)
    double sse = (y.array() - y_pred.array()).square().sum();
    
    // Calculate R²
    rSquared = ssr / sst;
    
    // Calculate adjusted R²
    adjustedRSquared = 1.0 - (1.0 - rSquared) * (nSamples - 1) / (nSamples - nFeatures - 1);
    
    // Calculate RMSE
    rmse = std::sqrt(sse / nSamples);
}

void LinearRegression::calculateFeatureStdDevs(const Eigen::MatrixXd& X) {
    // Initialize feature standard deviations vector
    featureStdDevs = Eigen::VectorXd(X.cols());
    
    // Calculate standard deviation for each feature
    for (int i = 0; i < X.cols(); ++i) {
        Eigen::VectorXd feature = X.col(i);
        double mean = feature.mean();
        featureStdDevs(i) = std::sqrt((feature.array() - mean).square().sum() / (feature.size() - 1));
    }
}

std::unordered_map<std::string, double> LinearRegression::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }

    std::unordered_map<std::string, double> importance;
    
    // Calculate standardized coefficients
    Eigen::VectorXd standardizedCoefs = coefficients.array() * featureStdDevs.array();
    
    // Take absolute values and normalize to sum to 1
    Eigen::VectorXd absStandardizedCoefs = standardizedCoefs.array().abs();
    double sum = absStandardizedCoefs.sum();
    
    // Store normalized importance scores
    for (int i = 0; i < nFeatures; ++i) {
        if (i < inputVariableNames.size()) {
            importance[inputVariableNames[i]] = absStandardizedCoefs(i) / sum;
        } else {
            importance["Variable_" + std::to_string(i+1)] = absStandardizedCoefs(i) / sum;
        }
    }
    
    return importance;
}