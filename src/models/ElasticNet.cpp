#include "models/ElasticNet.h"
#include <cmath>
#include <iostream>
#include <algorithm>

ElasticNet::ElasticNet() 
    : intercept(0.0), rSquared(0.0), adjustedRSquared(0.0), rmse(0.0),
      nSamples(0), nFeatures(0), isFitted(false),
      alpha(0.5), lambda(1.0), maxIter(1000), tol(0.0001) {
}

ElasticNet::ElasticNet(double alpha, double lambda, int maxIter, double tol)
    : intercept(0.0), rSquared(0.0), adjustedRSquared(0.0), rmse(0.0),
      nSamples(0), nFeatures(0), isFitted(false),
      alpha(alpha), lambda(lambda), maxIter(maxIter), tol(tol) {
}

bool ElasticNet::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
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

        // Initialize coefficients to zeros
        coefficients = Eigen::VectorXd::Zero(nFeatures);
        intercept = 0.0;

        // Run coordinate descent algorithm to find optimal coefficients
        coordinateDescent(X, y);

        // Set isFitted to true
        isFitted = true;
        
        // Calculate statistics
        calculateStatistics(X, y);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting ElasticNet model: " << e.what() << std::endl;
        return false;
    }
}

void ElasticNet::coordinateDescent(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    // Center the data (subtract mean)
    Eigen::VectorXd y_centered = y.array() - y.mean();
    Eigen::MatrixXd X_centered = X;
    
    // Center each column of X
    for (int j = 0; j < X.cols(); ++j) {
        Eigen::VectorXd col = X.col(j);
        double mean = col.mean();
        X_centered.col(j) = col.array() - mean;
    }
    
    // Initialize coefficients
    coefficients = Eigen::VectorXd::Zero(nFeatures);
    
    // Calculate initial residuals
    Eigen::VectorXd residuals = y_centered - X_centered * coefficients;
    
    // Iterate until convergence or max iterations
    for (int iter = 0; iter < maxIter; ++iter) {
        double max_change = 0.0;
        
        // Update each coefficient using coordinate descent
        for (int j = 0; j < nFeatures; ++j) {
            // Get the column of X
            Eigen::VectorXd X_j = X_centered.col(j);
            
            // Calculate dot product with current residuals
            double rho = X_j.dot(residuals) + coefficients(j) * X_j.squaredNorm();
            
            // Calculate update with soft thresholding
            double old_coef = coefficients(j);
            
            if (rho > lambda * alpha) {
                coefficients(j) = (rho - lambda * alpha) / (1.0 + lambda * (1.0 - alpha));
            } else if (rho < -lambda * alpha) {
                coefficients(j) = (rho + lambda * alpha) / (1.0 + lambda * (1.0 - alpha));
            } else {
                coefficients(j) = 0.0;
            }
            
            // Update residuals
            double delta_coef = coefficients(j) - old_coef;
            if (delta_coef != 0.0) {
                residuals -= X_j * delta_coef;
            }
            
            // Track maximum coefficient change
            max_change = std::max(max_change, std::abs(delta_coef));
        }
        
        // Check convergence
        if (max_change < tol) {
            break;
        }
    }
    
    // Calculate intercept
    Eigen::VectorXd X_mean = X.colwise().mean();
    intercept = y.mean() - X_mean.dot(coefficients);
}

Eigen::VectorXd ElasticNet::predict(const Eigen::MatrixXd& X) const {
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

std::string ElasticNet::getName() const {
    return "ElasticNet";
}

std::unordered_map<std::string, double> ElasticNet::getParameters() const {
    std::unordered_map<std::string, double> params;
    params["intercept"] = intercept;
    params["alpha"] = alpha;
    params["lambda"] = lambda;
    params["max_iter"] = static_cast<double>(maxIter);
    params["tol"] = tol;
    
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

std::unordered_map<std::string, double> ElasticNet::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    stats["r_squared"] = rSquared;
    stats["adjusted_r_squared"] = adjustedRSquared;
    stats["rmse"] = rmse;
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    stats["non_zero_coefficients"] = static_cast<double>(
        (coefficients.array() != 0.0).count());
    
    return stats;
}

std::string ElasticNet::getDescription() const {
    return "ElasticNet Regression with L1 and L2 regularization (alpha=" 
          + std::to_string(alpha) + ", lambda=" + std::to_string(lambda) + ").";
}

Eigen::VectorXd ElasticNet::getCoefficients() const {
    return coefficients;
}

double ElasticNet::getIntercept() const {
    return intercept;
}

double ElasticNet::getRSquared() const {
    return rSquared;
}

double ElasticNet::getAdjustedRSquared() const {
    return adjustedRSquared;
}

double ElasticNet::getRMSE() const {
    return rmse;
}

std::vector<std::string> ElasticNet::getVariableNames() const {
    return inputVariableNames;
}

std::string ElasticNet::getTargetName() const {
    return targetVariableName;
}

void ElasticNet::calculateStatistics(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
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

void ElasticNet::calculateFeatureStdDevs(const Eigen::MatrixXd& X) {
    // Initialize feature standard deviations vector
    featureStdDevs = Eigen::VectorXd(X.cols());
    
    // Calculate standard deviation for each feature
    for (int i = 0; i < X.cols(); ++i) {
        Eigen::VectorXd feature = X.col(i);
        double mean = feature.mean();
        featureStdDevs(i) = std::sqrt((feature.array() - mean).square().sum() / (feature.size() - 1));
    }
}

std::unordered_map<std::string, double> ElasticNet::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }

    std::unordered_map<std::string, double> importance;
    
    // Calculate standardized coefficients
    Eigen::VectorXd standardizedCoefs = coefficients.array() * featureStdDevs.array();
    
    // Take absolute values and normalize to sum to 1
    Eigen::VectorXd absStandardizedCoefs = standardizedCoefs.array().abs();
    double sum = absStandardizedCoefs.sum();
    
    // Handle case where all coefficients are zero
    if (sum == 0.0) {
        for (int i = 0; i < nFeatures; ++i) {
            if (i < inputVariableNames.size()) {
                importance[inputVariableNames[i]] = 1.0 / nFeatures;
            } else {
                importance["Variable_" + std::to_string(i+1)] = 1.0 / nFeatures;
            }
        }
        return importance;
    }
    
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