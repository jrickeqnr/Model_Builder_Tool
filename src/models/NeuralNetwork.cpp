#include "models/NeuralNetwork.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric>  // For std::iota and std::accumulate
#include <sstream>
#include <random>
#include <iomanip>

NeuralNetwork::NeuralNetwork() 
    : hiddenActivation(Activation::RELU), outputActivation(Activation::LINEAR),
      learningRate(0.01), epochs(1000), batchSize(32), tol(0.0001),
      rSquared(0.0), adjustedRSquared(0.0), rmse(0.0),
      nSamples(0), nFeatures(0), isFitted(false) {
    // Initialize with a simple architecture (one hidden layer with 10 neurons)
    layerSizes = {10};
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& hiddenLayers,
                         Activation activation,
                         Activation outputActivation,
                         double learningRate,
                         int epochs,
                         int batchSize,
                         double tol)
    : layerSizes(hiddenLayers), hiddenActivation(activation), outputActivation(outputActivation),
      learningRate(learningRate), epochs(epochs), batchSize(batchSize), tol(tol),
      rSquared(0.0), adjustedRSquared(0.0), rmse(0.0),
      nSamples(0), nFeatures(0), isFitted(false) {
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& hiddenLayers,
                         const std::string& activation,
                         double learningRate,
                         int epochs,
                         int batchSize,
                         const std::string& solver,
                         double alpha)
    : layerSizes(hiddenLayers),
      learningRate(learningRate), epochs(epochs), batchSize(batchSize), tol(0.0001),
      rSquared(0.0), adjustedRSquared(0.0), rmse(0.0),
      nSamples(0), nFeatures(0), isFitted(false) {
    
    // Convert activation string to enum
    if (activation == "relu") {
        hiddenActivation = Activation::RELU;
    } else if (activation == "sigmoid") {
        hiddenActivation = Activation::SIGMOID;
    } else if (activation == "tanh") {
        hiddenActivation = Activation::TANH;
    } else if (activation == "identity") {
        hiddenActivation = Activation::LINEAR;
    } else {
        std::cerr << "Warning: Unknown activation function '" << activation 
                 << "'. Using ReLU as default." << std::endl;
        hiddenActivation = Activation::RELU;
    }
    
    // For now, always use LINEAR for output activation
    outputActivation = Activation::LINEAR;
    
    // Note: solver parameter currently not used in this implementation
    // but could be extended to support different optimization algorithms
    if (solver != "adam" && solver != "sgd" && solver != "lbfgs") {
        std::cerr << "Warning: Solver '" << solver << "' not supported. Using default implementation." << std::endl;
    }
}

bool NeuralNetwork::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
                     const std::vector<std::string>& variableNames,
                     const std::string& targetName) {
    if (X.rows() != y.rows()) {
        std::cerr << "Error: Number of samples in X (" << X.rows() 
                 << ") does not match number of samples in y (" << y.rows() << ")." << std::endl;
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

        // Calculate normalization parameters
        calculateNormalizationParams(X, y);

        // Initialize network architecture
        // Input layer (nFeatures) -> Hidden layers -> Output layer (1)
        std::vector<int> architecture;
        architecture.push_back(nFeatures);
        for (int hiddenSize : layerSizes) {
            architecture.push_back(hiddenSize);
        }
        architecture.push_back(1);  // One output neuron for regression

        // Initialize weights and biases
        weights.clear();
        biases.clear();
        
        for (size_t i = 0; i < architecture.size() - 1; ++i) {
            int inputSize = architecture[i];
            int outputSize = architecture[i + 1];
            
            // Initialize weights with Xavier initialization
            double weightScale = std::sqrt(2.0 / (inputSize + outputSize));
            
            // Create random weight matrix with values ~ N(0, weightScale)
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> dist(0.0, weightScale);
            
            Eigen::MatrixXd W(outputSize, inputSize);
            for (int r = 0; r < outputSize; ++r) {
                for (int c = 0; c < inputSize; ++c) {
                    W(r, c) = dist(gen);
                }
            }
            weights.push_back(W);
            
            // Initialize biases to zero
            Eigen::VectorXd b = Eigen::VectorXd::Zero(outputSize);
            biases.push_back(b);
        }

        // Normalize features
        Eigen::MatrixXd X_norm = normalizeFeatures(X);
        
        // Normalize target
        Eigen::VectorXd y_norm = (y.array() - targetMean) / targetStdDev;
        
        // Initialize variables for training
        double prevLoss = std::numeric_limits<double>::max();
        
        // Training loop
        for (int epoch = 0; epoch < epochs; ++epoch) {
            // Shuffle indices for stochastic gradient descent
            std::vector<int> indices(nSamples);
            std::iota(indices.begin(), indices.end(), 0);
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indices.begin(), indices.end(), g);
            
            double epochLoss = 0.0;
            
            // Mini-batch gradient descent
            for (int i = 0; i < nSamples; i += batchSize) {
                int actualBatchSize = std::min(batchSize, nSamples - i);
                
                // Create batch
                Eigen::MatrixXd X_batch(actualBatchSize, nFeatures);
                Eigen::VectorXd y_batch(actualBatchSize);
                
                for (int j = 0; j < actualBatchSize; ++j) {
                    X_batch.row(j) = X_norm.row(indices[i + j]);
                    y_batch(j) = y_norm(indices[i + j]);
                }
                
                // Forward propagation
                std::vector<Eigen::MatrixXd> activations = forwardPropagate(X_batch);
                
                // Backward propagation
                auto [weightGrads, biasGrads] = backwardPropagate(X_batch, y_batch, activations);
                
                // Update parameters
                updateParameters(weightGrads, biasGrads);
                
                // Calculate batch loss (MSE)
                Eigen::VectorXd predictions = activations.back();
                double batchLoss = (predictions - y_batch).array().square().mean();
                epochLoss += batchLoss * actualBatchSize;
            }
            
            // Average loss for the epoch
            epochLoss /= nSamples;
            
            // Check for convergence
            double improvement = std::abs(prevLoss - epochLoss);
            prevLoss = epochLoss;
            
            if (improvement < tol) {
                break;
            }
        }

        // Set isFitted to true
        isFitted = true;
        
        // Calculate statistics
        calculateStatistics(X, y);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting Neural Network model: " << e.what() << std::endl;
        return false;
    }
}

void NeuralNetwork::calculateNormalizationParams(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    // Calculate feature means
    featureMeans = X.colwise().mean();
    
    // Calculate feature standard deviations
    featureStdDevs = Eigen::VectorXd(X.cols());
    for (int i = 0; i < X.cols(); ++i) {
        Eigen::VectorXd col = X.col(i);
        featureStdDevs(i) = std::sqrt((col.array() - featureMeans(i)).square().sum() / (col.size() - 1));
        
        // Handle constant features (std dev = 0)
        if (featureStdDevs(i) < 1e-10) {
            featureStdDevs(i) = 1.0;
        }
    }
    
    // Calculate target mean and standard deviation
    targetMean = y.mean();
    targetStdDev = std::sqrt((y.array() - targetMean).square().sum() / (y.size() - 1));
    
    // Handle constant target (std dev = 0)
    if (targetStdDev < 1e-10) {
        targetStdDev = 1.0;
    }
}

Eigen::MatrixXd NeuralNetwork::normalizeFeatures(const Eigen::MatrixXd& X) const {
    Eigen::MatrixXd X_norm = X;
    
    for (int i = 0; i < X.cols(); ++i) {
        X_norm.col(i) = (X.col(i).array() - featureMeans(i)) / featureStdDevs(i);
    }
    
    return X_norm;
}

std::vector<Eigen::MatrixXd> NeuralNetwork::forwardPropagate(const Eigen::MatrixXd& X) const {
    std::vector<Eigen::MatrixXd> activations;
    
    // Input layer activation is just the input
    activations.push_back(X);
    
    // Forward pass through hidden layers
    Eigen::MatrixXd current = X;
    
    for (size_t i = 0; i < weights.size() - 1; ++i) {
        // Linear transformation: Z = XW^T + b
        Eigen::MatrixXd Z = current * weights[i].transpose();
        Z.rowwise() += biases[i].transpose();
        
        // Apply activation function
        current = applyActivation(Z, hiddenActivation);
        
        // Store activation
        activations.push_back(current);
    }
    
    // Output layer
    size_t lastLayerIdx = weights.size() - 1;
    Eigen::MatrixXd Z = current * weights[lastLayerIdx].transpose();
    Z.rowwise() += biases[lastLayerIdx].transpose();
    
    // Apply output activation function
    current = applyActivation(Z, outputActivation);
    
    // Store output activation
    activations.push_back(current);
    
    return activations;
}

std::pair<std::vector<Eigen::MatrixXd>, std::vector<Eigen::VectorXd>> 
NeuralNetwork::backwardPropagate(const Eigen::MatrixXd& X, const Eigen::VectorXd& y, 
                               const std::vector<Eigen::MatrixXd>& activations) {
    int numLayers = weights.size();
    
    // Initialize gradient containers
    std::vector<Eigen::MatrixXd> weightGrads(numLayers);
    std::vector<Eigen::VectorXd> biasGrads(numLayers);
    
    // Calculate error at output layer
    Eigen::MatrixXd outputError(y.size(), 1);
    outputError.col(0) = activations.back().col(0) - y;
    
    // Backward pass (output to input)
    Eigen::MatrixXd delta = outputError;
    
    if (outputActivation != Activation::LINEAR) {
        // If non-linear output, apply activation derivative
        Eigen::MatrixXd derivOutput = applyActivationDerivative(activations.back(), outputActivation);
        delta = delta.array() * derivOutput.array();
    }
    
    // Output layer gradients
    weightGrads[numLayers - 1] = delta.transpose() * activations[numLayers - 1] / X.rows();
    biasGrads[numLayers - 1] = delta.colwise().sum() / X.rows();
    
    // Hidden layers (backwards)
    for (int l = numLayers - 2; l >= 0; --l) {
        // Propagate error backward
        delta = delta * weights[l + 1];
        
        // Apply activation derivative
        Eigen::MatrixXd derivActivation = applyActivationDerivative(activations[l + 1], hiddenActivation);
        delta = delta.array() * derivActivation.array();
        
        // Calculate gradients
        weightGrads[l] = delta.transpose() * activations[l] / X.rows();
        biasGrads[l] = delta.colwise().sum() / X.rows();
    }
    
    return {weightGrads, biasGrads};
}

void NeuralNetwork::updateParameters(const std::vector<Eigen::MatrixXd>& weightGrads,
                                  const std::vector<Eigen::VectorXd>& biasGrads) {
    for (size_t i = 0; i < weights.size(); ++i) {
        weights[i] -= learningRate * weightGrads[i];
        biases[i] -= learningRate * biasGrads[i];
    }
}

Eigen::MatrixXd NeuralNetwork::applyActivation(const Eigen::MatrixXd& X, Activation activation) const {
    Eigen::MatrixXd result = X;
    
    switch (activation) {
        case Activation::RELU:
            result = result.array().max(0.0);
            break;
        case Activation::SIGMOID:
            result = 1.0 / (1.0 + (-result.array()).exp());
            break;
        case Activation::TANH:
            result = result.array().tanh();
            break;
        case Activation::LINEAR:
            // No transformation for linear activation
            break;
    }
    
    return result;
}

Eigen::MatrixXd NeuralNetwork::applyActivationDerivative(const Eigen::MatrixXd& X, Activation activation) const {
    Eigen::MatrixXd result(X.rows(), X.cols());
    
    switch (activation) {
        case Activation::RELU:
            result = (X.array() > 0.0).cast<double>();
            break;
        case Activation::SIGMOID:
            // sigmoid derivative: f(x) * (1 - f(x))
            result = X.array() * (1.0 - X.array());
            break;
        case Activation::TANH:
            // tanh derivative: 1 - f(x)^2
            result = 1.0 - X.array().square();
            break;
        case Activation::LINEAR:
            // Derivative of linear is 1
            result.setOnes();
            break;
    }
    
    return result;
}

Eigen::VectorXd NeuralNetwork::predict(const Eigen::MatrixXd& X) const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }

    if (X.cols() != nFeatures) {
        throw std::invalid_argument("Number of features in X (" + std::to_string(X.cols()) + 
                                   ") does not match the number of features the model was trained on (" + 
                                   std::to_string(nFeatures) + ")");
    }

    // Normalize input
    Eigen::MatrixXd X_norm = normalizeFeatures(X);
    
    // Forward pass
    std::vector<Eigen::MatrixXd> activations = forwardPropagate(X_norm);
    
    // Get output (last activation)
    Eigen::VectorXd y_norm = activations.back().col(0);
    
    // Denormalize output
    return y_norm.array() * targetStdDev + targetMean;
}

std::string NeuralNetwork::getName() const {
    return "NeuralNetwork";
}

std::unordered_map<std::string, double> NeuralNetwork::getParameters() const {
    std::unordered_map<std::string, double> params;
    
    // Add hyperparameters
    params["learning_rate"] = learningRate;
    params["epochs"] = static_cast<double>(epochs);
    params["batch_size"] = static_cast<double>(batchSize);
    params["tolerance"] = tol;
    
    // Add layer sizes
    params["input_layer_size"] = static_cast<double>(nFeatures);
    for (size_t i = 0; i < layerSizes.size(); ++i) {
        params["hidden_layer_" + std::to_string(i+1) + "_size"] = static_cast<double>(layerSizes[i]);
    }
    params["output_layer_size"] = 1.0;
    
    // Add activation functions (as numeric codes)
    params["hidden_activation"] = static_cast<double>(hiddenActivation);
    params["output_activation"] = static_cast<double>(outputActivation);
    
    // Add total number of parameters (weights and biases)
    int totalParams = 0;
    for (size_t i = 0; i < weights.size(); ++i) {
        totalParams += weights[i].size() + biases[i].size();
    }
    params["total_parameters"] = static_cast<double>(totalParams);
    
    return params;
}

std::unordered_map<std::string, double> NeuralNetwork::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    
    stats["r_squared"] = rSquared;
    stats["adjusted_r_squared"] = adjustedRSquared;
    stats["rmse"] = rmse;
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    
    return stats;
}

std::string NeuralNetwork::getDescription() const {
    std::stringstream ss;
    ss << "Neural Network with ";
    
    // Format layer structure
    ss << nFeatures << " input features, ";
    for (size_t i = 0; i < layerSizes.size(); ++i) {
        ss << layerSizes[i] << " neurons in hidden layer " << (i+1);
        if (i < layerSizes.size() - 1) {
            ss << ", ";
        }
    }
    ss << " and 1 output neuron";
    
    // Add activation function info
    std::string hiddenActStr;
    switch (hiddenActivation) {
        case Activation::RELU: hiddenActStr = "ReLU"; break;
        case Activation::SIGMOID: hiddenActStr = "Sigmoid"; break;
        case Activation::TANH: hiddenActStr = "Tanh"; break;
        case Activation::LINEAR: hiddenActStr = "Linear"; break;
    }
    
    std::string outputActStr;
    switch (outputActivation) {
        case Activation::RELU: outputActStr = "ReLU"; break;
        case Activation::SIGMOID: outputActStr = "Sigmoid"; break;
        case Activation::TANH: outputActStr = "Tanh"; break;
        case Activation::LINEAR: outputActStr = "Linear"; break;
    }
    
    ss << " (hidden: " << hiddenActStr << ", output: " << outputActStr << ")";
    
    return ss.str();
}

std::vector<std::string> NeuralNetwork::getVariableNames() const {
    return inputVariableNames;
}

std::string NeuralNetwork::getTargetName() const {
    return targetVariableName;
}

void NeuralNetwork::calculateStatistics(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
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

std::unordered_map<std::string, double> NeuralNetwork::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    std::unordered_map<std::string, double> importance;
    
    // Create a random number generator for permutation
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Create a test matrix with random values in the same range as the normalized data
    int testSamples = 1000;
    Eigen::MatrixXd X_test = Eigen::MatrixXd::Random(testSamples, nFeatures);
    
    // Normalize by standard deviation range (Random produces values in [-1,1])
    for (int i = 0; i < nFeatures; ++i) {
        X_test.col(i) = X_test.col(i).array() * 3.0; // Scale to cover typical normalized range
    }
    
    // Get baseline predictions
    Eigen::VectorXd baseline_pred = predict(X_test);
    
    // Compute baseline MSE (irrelevant for importance calculation but needed for scale)
    double baseline_mse = baseline_pred.array().square().mean();
    
    // Store feature importance scores
    std::vector<double> scores(nFeatures);
    
    // For each feature, calculate permutation importance
    for (int i = 0; i < nFeatures; ++i) {
        // Create a copy of the test data
        Eigen::MatrixXd X_permuted = X_test;
        
        // Permute the current feature
        Eigen::VectorXd feature = X_test.col(i);
        std::shuffle(feature.data(), feature.data() + feature.size(), gen);
        X_permuted.col(i) = feature;
        
        // Get predictions with permuted feature
        Eigen::VectorXd permuted_pred = predict(X_permuted);
        
        // Calculate MSE with permuted feature
        double permuted_mse = (permuted_pred - baseline_pred).array().square().mean();
        
        // Importance is the increase in error
        scores[i] = permuted_mse;
    }
    
    // Normalize scores
    double sum = std::accumulate(scores.begin(), scores.end(), 0.0);
    
    // Handle case where all scores are zero
    if (sum < 1e-10) {
        for (int i = 0; i < nFeatures; ++i) {
            std::string name = (i < inputVariableNames.size()) ? inputVariableNames[i] : "Variable_" + std::to_string(i+1);
            importance[name] = 1.0 / nFeatures;
        }
    } else {
        // Normalize to sum to 1
        for (int i = 0; i < nFeatures; ++i) {
            std::string name = (i < inputVariableNames.size()) ? inputVariableNames[i] : "Variable_" + std::to_string(i+1);
            importance[name] = scores[i] / sum;
        }
    }
    
    return importance;
} 