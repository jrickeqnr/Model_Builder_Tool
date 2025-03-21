#include "models/NeuralNetwork.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>

// Layer implementation
NeuralNetwork::Layer::Layer(int inputSize, int outputSize, std::mt19937& rng) {
    // Initialize weights with Xavier/Glorot initialization
    double stddev = std::sqrt(2.0 / (inputSize + outputSize));
    std::normal_distribution<double> dist(0.0, stddev);
    
    weights = Eigen::MatrixXd(outputSize, inputSize);
    biases = Eigen::VectorXd::Zero(outputSize);
    
    // Randomly initialize weights
    for (int i = 0; i < outputSize; ++i) {
        for (int j = 0; j < inputSize; ++j) {
            weights(i, j) = dist(rng);
        }
        biases(i) = dist(rng);
    }
}

void NeuralNetwork::Layer::forwardPass(const Eigen::MatrixXd& input, const std::string& activationFunc) {
    // Z = W * X + b
    Eigen::MatrixXd z = (weights * input.transpose()).colwise() + biases;
    
    // A = activation(Z)
    activations = applyActivation(z, activationFunc);
}

Eigen::MatrixXd NeuralNetwork::Layer::backwardPass(const Eigen::MatrixXd& nextLayerDeltas, 
                                                 const Eigen::MatrixXd& nextLayerWeights,
                                                 const std::string& activationFunc) {
    // For hidden layers: delta = (W^(l+1)' * delta^(l+1)) .* activation'(z^(l))
    deltas = (nextLayerWeights.transpose() * nextLayerDeltas).array() * 
             applyActivationDerivative(activations, activationFunc).array();
    
    return deltas;
}

Eigen::MatrixXd NeuralNetwork::Layer::applyActivation(const Eigen::MatrixXd& input, 
                                                    const std::string& activationFunc) const {
    Eigen::MatrixXd result = input;
    
    if (activationFunc == "sigmoid") {
        // sigmoid(x) = 1 / (1 + exp(-x))
        result = 1.0 / (1.0 + (-input.array()).exp());
    } else if (activationFunc == "tanh") {
        // tanh(x) = (exp(x) - exp(-x)) / (exp(x) + exp(-x))
        result = input.array().tanh();
    } else if (activationFunc == "relu") {
        // relu(x) = max(0, x)
        result = input.array().max(0.0);
    } else {
        // identity: do nothing
    }
    
    return result;
}

Eigen::MatrixXd NeuralNetwork::Layer::applyActivationDerivative(const Eigen::MatrixXd& output, 
                                                              const std::string& activationFunc) const {
    Eigen::MatrixXd result = Eigen::MatrixXd::Ones(output.rows(), output.cols());
    
    if (activationFunc == "sigmoid") {
        // sigmoid'(x) = sigmoid(x) * (1 - sigmoid(x))
        result = output.array() * (1.0 - output.array());
    } else if (activationFunc == "tanh") {
        // tanh'(x) = 1 - tanh^2(x)
        result = 1.0 - output.array().square();
    } else if (activationFunc == "relu") {
        // relu'(x) = 1 if x > 0, 0 otherwise
        for (int i = 0; i < output.rows(); ++i) {
            for (int j = 0; j < output.cols(); ++j) {
                result(i, j) = output(i, j) > 0.0 ? 1.0 : 0.0;
            }
        }
    } else {
        // identity: derivative is 1
    }
    
    return result;
}

// NeuralNetwork implementation
NeuralNetwork::NeuralNetwork()
    : hiddenLayerSizes({10, 10}), activation("relu"), solver("adam"),
      learningRate(0.01), maxIter(1000), batchSize(32), alpha(0.0001),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0), 
      finalLoss(0.0), nIterations(0), targetMean(0.0), targetStdDev(1.0) {
    // Initialize random number generator with a random seed
    std::random_device rd;
    rng = std::mt19937(rd());
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& hiddenLayerSizes, 
                           const std::string& activation,
                           double learningRate,
                           int maxIter,
                           int batchSize,
                           const std::string& solver,
                           double alpha)
    : hiddenLayerSizes(hiddenLayerSizes), activation(activation), solver(solver),
      learningRate(learningRate), maxIter(maxIter), batchSize(batchSize), alpha(alpha),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0), 
      finalLoss(0.0), nIterations(0), targetMean(0.0), targetStdDev(1.0) {
    // Initialize random number generator with a random seed
    std::random_device rd;
    rng = std::mt19937(rd());
}

bool NeuralNetwork::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
                       const std::vector<std::string>& variableNames,
                       const std::string& targetName) {
    try {
        // Check dimensions
        if (X.rows() != y.rows()) {
            std::cerr << "Error: Number of samples in X (" << X.rows() 
                    << ") does not match number of samples in y (" << y.rows() << ")." << std::endl;
            return false;
        }
        
        nSamples = X.rows();
        nFeatures = X.cols();
        
        // Store variable names
        if (variableNames.size() == 0) {
            // Generate default variable names
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
        
        // Standardize features and target
        featureMeans = X.colwise().mean();
        featureStdDevs = ((X.rowwise() - featureMeans.transpose()).array().square().colwise().sum() / (nSamples - 1)).sqrt();
        
        // Check for constant features (zero standard deviation) and set to 1 to avoid division by zero
        for (int i = 0; i < nFeatures; ++i) {
            if (featureStdDevs(i) < 1e-10) {
                featureStdDevs(i) = 1.0;
            }
        }
        
        Eigen::MatrixXd X_scaled = standardizeFeatures(X);
        
        // Standardize the target
        targetMean = y.mean();
        targetStdDev = std::sqrt((y.array() - targetMean).square().sum() / (nSamples - 1));
        Eigen::VectorXd y_scaled = (y.array() - targetMean) / targetStdDev;
        
        // Initialize layers
        layers.clear();
        
        // Input to first hidden layer
        layers.push_back(Layer(nFeatures, hiddenLayerSizes[0], rng));
        
        // Hidden layers
        for (size_t i = 1; i < hiddenLayerSizes.size(); ++i) {
            layers.push_back(Layer(hiddenLayerSizes[i-1], hiddenLayerSizes[i], rng));
        }
        
        // Output layer (1 output for regression)
        layers.push_back(Layer(hiddenLayerSizes.back(), 1, rng));
        
        // Training using stochastic gradient descent
        double currentLearningRate = learningRate;
        double previousLoss = std::numeric_limits<double>::max();
        
        // For early stopping
        int patience = 10;
        int noImprovement = 0;
        
        // For Adam optimizer
        std::vector<Eigen::MatrixXd> m_weights(layers.size());
        std::vector<Eigen::VectorXd> m_biases(layers.size());
        std::vector<Eigen::MatrixXd> v_weights(layers.size());
        std::vector<Eigen::VectorXd> v_biases(layers.size());
        
        for (size_t i = 0; i < layers.size(); ++i) {
            m_weights[i] = Eigen::MatrixXd::Zero(layers[i].weights.rows(), layers[i].weights.cols());
            m_biases[i] = Eigen::VectorXd::Zero(layers[i].biases.size());
            v_weights[i] = Eigen::MatrixXd::Zero(layers[i].weights.rows(), layers[i].weights.cols());
            v_biases[i] = Eigen::VectorXd::Zero(layers[i].biases.size());
        }
        
        // Adam parameters
        double beta1 = 0.9;
        double beta2 = 0.999;
        double epsilon = 1e-8;
        
        // Training loop
        for (int iter = 0; iter < maxIter; ++iter) {
            // Shuffle data for stochastic updates
            std::vector<int> indices(nSamples);
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), rng);
            
            // Minibatch training
            for (int batch_start = 0; batch_start < nSamples; batch_start += batchSize) {
                int batchSize = std::min(this->batchSize, nSamples - batch_start);
                
                Eigen::MatrixXd X_batch(batchSize, nFeatures);
                Eigen::VectorXd y_batch(batchSize);
                
                for (int i = 0; i < batchSize; ++i) {
                    int idx = indices[batch_start + i];
                    X_batch.row(i) = X_scaled.row(idx);
                    y_batch(i) = y_scaled(idx);
                }
                
                // Forward pass
                forwardPass(X_batch);
                
                // Backward pass
                backwardPass(X_batch, y_batch);
                
                // Update parameters
                if (solver == "adam") {
                    // Adam update
                    for (size_t i = 0; i < layers.size(); ++i) {
                        // Update biases
                        m_biases[i] = beta1 * m_biases[i] + (1 - beta1) * layers[i].deltas.rowwise().sum();
                        v_biases[i] = beta2 * v_biases[i] + (1 - beta2) * (layers[i].deltas.rowwise().sum().array().square().matrix());
                        
                        Eigen::VectorXd m_bias_corrected = m_biases[i] / (1 - std::pow(beta1, iter + 1));
                        Eigen::VectorXd v_bias_corrected = v_biases[i] / (1 - std::pow(beta2, iter + 1));
                        
                        layers[i].biases -= currentLearningRate * m_bias_corrected.array() / 
                                          (v_bias_corrected.array().sqrt() + epsilon);
                        
                        // Update weights
                        Eigen::MatrixXd prev_activations = (i == 0) ? X_batch : layers[i-1].activations;
                        Eigen::MatrixXd weight_gradient = layers[i].deltas * prev_activations;
                        
                        m_weights[i] = beta1 * m_weights[i] + (1 - beta1) * weight_gradient;
                        v_weights[i] = beta2 * v_weights[i] + (1 - beta2) * (weight_gradient.array().square().matrix());
                        
                        Eigen::MatrixXd m_weight_corrected = m_weights[i] / (1 - std::pow(beta1, iter + 1));
                        Eigen::MatrixXd v_weight_corrected = v_weights[i] / (1 - std::pow(beta2, iter + 1));
                        
                        layers[i].weights -= currentLearningRate * m_weight_corrected.array() / 
                                           (v_weight_corrected.array().sqrt() + epsilon).matrix();
                        
                        // Add L2 regularization
                        if (alpha > 0) {
                            layers[i].weights -= currentLearningRate * alpha * layers[i].weights;
                        }
                    }
                } else {
                    // Standard SGD update
                    updateParameters(currentLearningRate);
                }
            }
            
            // Calculate loss and check for convergence
            Eigen::VectorXd predictions = predict(X);
            double current_loss = calculateLoss(predictions, y);
            
            // Learning rate decay
            if (solver == "sgd" && iter > 0 && iter % 20 == 0) {
                currentLearningRate *= 0.9;
            }
            
            // Early stopping check
            if (current_loss < previousLoss - 1e-4) {
                noImprovement = 0;
            } else {
                noImprovement++;
                if (noImprovement >= patience) {
                    std::cout << "Early stopping at iteration " << iter << std::endl;
                    break;
                }
            }
            
            previousLoss = current_loss;
            finalLoss = current_loss;
            nIterations = iter + 1;
            
            // Print progress
            if (iter % 100 == 0) {
                std::cout << "Iteration " << iter << ", Loss: " << current_loss << std::endl;
            }
        }
        
        // Calculate RMSE
        Eigen::VectorXd predictions = predict(X);
        rmse = std::sqrt((predictions - y).array().square().mean());
        
        // Calculate feature importance
        calculateFeatureImportance(X, y);
        
        isFitted = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting Neural Network model: " << e.what() << std::endl;
        return false;
    }
}

void NeuralNetwork::forwardPass(const Eigen::MatrixXd& X) {
    // For the first layer, input is X
    layers[0].forwardPass(X, activation);
    
    // For hidden layers
    for (size_t i = 1; i < layers.size() - 1; ++i) {
        layers[i].forwardPass(layers[i-1].activations, activation);
    }
    
    // Output layer uses identity activation for regression
    layers.back().forwardPass(layers[layers.size()-2].activations, "identity");
}

void NeuralNetwork::backwardPass(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    int outputLayerIdx = layers.size() - 1;
    
    // Calculate error at output layer (MSE derivative = prediction - target)
    Eigen::MatrixXd outputError = layers[outputLayerIdx].activations - y;
    layers[outputLayerIdx].deltas = outputError;
    
    // Backpropagate error to hidden layers
    for (int i = outputLayerIdx - 1; i >= 0; --i) {
        layers[i].backwardPass(layers[i+1].deltas, layers[i+1].weights, activation);
    }
}

void NeuralNetwork::updateParameters(double learningRate) {
    for (size_t i = 0; i < layers.size(); ++i) {
        // Get input to this layer
        Eigen::MatrixXd prev_activations;
        if (i == 0) {
            // For first hidden layer, input is X
            prev_activations = X;
        } else {
            prev_activations = layers[i-1].activations;
        }
        
        // Calculate gradients
        Eigen::MatrixXd weight_gradient = layers[i].deltas * prev_activations;
        Eigen::VectorXd bias_gradient = layers[i].deltas.rowwise().sum();
        
        // Update weights and biases
        layers[i].weights -= learningRate * weight_gradient;
        layers[i].biases -= learningRate * bias_gradient;
        
        // Add L2 regularization
        if (alpha > 0) {
            layers[i].weights -= learningRate * alpha * layers[i].weights;
        }
    }
}

double NeuralNetwork::calculateLoss(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const {
    // Mean Squared Error
    double mse = (predictions - targets).array().square().mean();
    
    // Add L2 regularization term
    double l2_term = 0.0;
    if (alpha > 0) {
        for (const auto& layer : layers) {
            l2_term += (layer.weights.array().square()).sum();
        }
        l2_term *= 0.5 * alpha / nSamples;
    }
    
    return mse + l2_term;
}

Eigen::MatrixXd NeuralNetwork::standardizeFeatures(const Eigen::MatrixXd& X) const {
    return (X.rowwise() - featureMeans.transpose()).array().rowwise() / featureStdDevs.transpose().array();
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
    
    // Standardize features
    Eigen::MatrixXd X_scaled = standardizeFeatures(X);
    
    // Forward pass through the network
    Eigen::MatrixXd current_input = X_scaled;
    
    for (size_t i = 0; i < layers.size() - 1; ++i) {
        Eigen::MatrixXd z = (layers[i].weights * current_input.transpose()).colwise() + layers[i].biases;
        current_input = layers[i].applyActivation(z, activation).transpose();
    }
    
    // Output layer (identity activation)
    Eigen::MatrixXd z_output = (layers.back().weights * current_input.transpose()).colwise() + layers.back().biases;
    Eigen::VectorXd output = z_output.transpose();
    
    // Unstandardize predictions
    return output.array() * targetStdDev + targetMean;
}

std::string NeuralNetwork::getName() const {
    return "Neural Network";
}

std::unordered_map<std::string, double> NeuralNetwork::getParameters() const {
    std::unordered_map<std::string, double> params;
    
    // Network architecture
    std::stringstream ss;
    for (size_t i = 0; i < hiddenLayerSizes.size(); ++i) {
        ss << hiddenLayerSizes[i];
        if (i < hiddenLayerSizes.size() - 1) {
            ss << ",";
        }
    }
    
    params["hidden_layer_sizes"] = 0;  // Cannot directly return a string, client should use description
    params["learning_rate"] = learningRate;
    params["max_iter"] = static_cast<double>(maxIter);
    params["batch_size"] = static_cast<double>(batchSize);
    params["alpha"] = alpha;
    
    return params;
}

std::unordered_map<std::string, double> NeuralNetwork::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    stats["rmse"] = rmse;
    stats["final_loss"] = finalLoss;
    stats["n_iterations"] = static_cast<double>(nIterations);
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    
    return stats;
}

std::string NeuralNetwork::getDescription() const {
    std::stringstream ss;
    ss << "Neural Network (MLP) with architecture: ";
    
    ss << nFeatures << " -> ";
    for (size_t i = 0; i < hiddenLayerSizes.size(); ++i) {
        ss << hiddenLayerSizes[i];
        if (i < hiddenLayerSizes.size() - 1) {
            ss << " -> ";
        }
    }
    ss << " -> 1, activation=" << activation << ", solver=" << solver;
    
    return ss.str();
}

std::vector<std::string> NeuralNetwork::getVariableNames() const {
    return inputVariableNames;
}

std::string NeuralNetwork::getTargetName() const {
    return targetVariableName;
}

void NeuralNetwork::calculateFeatureImportance(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    // Initialize importance scores
    featureImportanceScores.clear();
    for (const auto& name : inputVariableNames) {
        featureImportanceScores[name] = 0.0;
    }
    
    // Get baseline score
    Eigen::VectorXd baseline_pred = predict(X);
    double baseline_mse = (baseline_pred - y).array().square().mean();
    
    // For each feature, calculate permutation importance
    for (int i = 0; i < nFeatures; ++i) {
        // Create a copy of the dataset with the current feature permuted
        Eigen::MatrixXd X_permuted = X;
        
        // Extract the column to permute
        Eigen::VectorXd col = X.col(i);
        
        // Create a permutation of indices
        std::vector<int> indices(nSamples);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        
        // Apply permutation
        for (int j = 0; j < nSamples; ++j) {
            X_permuted(j, i) = X(indices[j], i);
        }
        
        // Get predictions on permuted data
        Eigen::VectorXd permuted_pred = predict(X_permuted);
        double permuted_mse = (permuted_pred - y).array().square().mean();
        
        // Importance is the increase in error when the feature is permuted
        double importance = permuted_mse - baseline_mse;
        
        // Store importance (if negative, set to 0)
        if (i < inputVariableNames.size()) {
            featureImportanceScores[inputVariableNames[i]] = std::max(0.0, importance);
        }
    }
    
    // Normalize importance scores to sum to 1
    double sum_importance = 0.0;
    for (const auto& pair : featureImportanceScores) {
        sum_importance += pair.second;
    }
    
    if (sum_importance > 0.0) {
        for (auto& pair : featureImportanceScores) {
            pair.second /= sum_importance;
        }
    } else {
        // If all importance scores are 0, assign equal importance
        double equal_importance = 1.0 / nFeatures;
        for (auto& pair : featureImportanceScores) {
            pair.second = equal_importance;
        }
    }
}

std::unordered_map<std::string, double> NeuralNetwork::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    return featureImportanceScores;
} 