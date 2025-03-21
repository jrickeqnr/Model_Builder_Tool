#pragma once

#include "models/Model.h"
#include <vector>
#include <random>
#include <memory>

/**
 * @brief Neural Network regression model
 * 
 * This class implements a multi-layer perceptron neural network
 * for regression tasks. It supports various activation functions,
 * solvers, and network architectures.
 */
class NeuralNetwork : public Model {
public:
    NeuralNetwork();
    
    /**
     * @brief Constructor with hyperparameters
     * 
     * @param hiddenLayerSizes Vector defining the network architecture
     * @param activation Activation function ('relu', 'tanh', 'sigmoid', 'identity')
     * @param learningRate Initial learning rate
     * @param maxIter Maximum number of iterations
     * @param batchSize Size of minibatches for stochastic optimizers
     * @param solver Solver ('adam', 'sgd', 'lbfgs')
     * @param alpha L2 regularization parameter
     */
    NeuralNetwork(const std::vector<int>& hiddenLayerSizes, 
                const std::string& activation,
                double learningRate,
                int maxIter,
                int batchSize,
                const std::string& solver,
                double alpha);
    
    ~NeuralNetwork() override = default;

    /**
     * @brief Fit the Neural Network model to the given data
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
     * @brief Make predictions using the fitted Neural Network model
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
     * @brief Get the model parameters
     * 
     * @return std::unordered_map<std::string, double> Map of parameter names to values
     */
    std::unordered_map<std::string, double> getParameters() const override;

    /**
     * @brief Get the model statistics
     * 
     * @return std::unordered_map<std::string, double> Map of statistic names to values
     */
    std::unordered_map<std::string, double> getStatistics() const override;

    /**
     * @brief Get a description of the Neural Network model
     * 
     * @return std::string Model description
     */
    std::string getDescription() const override;

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
     * For Neural Networks, feature importance is approximated through
     * a permutation importance technique that measures the decrease in
     * model performance when a feature's values are randomly shuffled.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    std::unordered_map<std::string, double> getFeatureImportance() const override;

private:
    // Network architecture
    std::vector<int> hiddenLayerSizes;
    std::string activation;
    std::string solver;
    double learningRate;
    int maxIter;
    int batchSize;
    double alpha;
    
    // Model state
    bool isFitted;
    int nSamples;
    int nFeatures;
    double rmse;
    double finalLoss;
    int nIterations;
    
    // Variable names storage
    std::vector<std::string> inputVariableNames;
    std::string targetVariableName;
    
    // Feature importance scores
    std::unordered_map<std::string, double> featureImportanceScores;
    
    // Random number generator
    std::mt19937 rng;
    
    // Neural network implementation
    class Layer {
    public:
        Eigen::MatrixXd weights;
        Eigen::VectorXd biases;
        Eigen::MatrixXd activations;
        Eigen::MatrixXd deltas;
        
        Layer(int inputSize, int outputSize, std::mt19937& rng);
        void forwardPass(const Eigen::MatrixXd& input, const std::string& activationFunc);
        Eigen::MatrixXd backwardPass(const Eigen::MatrixXd& nextLayerDeltas, 
                                   const Eigen::MatrixXd& nextLayerWeights,
                                   const std::string& activationFunc);
        Eigen::MatrixXd applyActivation(const Eigen::MatrixXd& input, 
                                      const std::string& activationFunc) const;
        Eigen::MatrixXd applyActivationDerivative(const Eigen::MatrixXd& input, 
                                               const std::string& activationFunc) const;
    };
    
    std::vector<Layer> layers;
    
    /**
     * @brief Initialize the neural network layers
     */
    void initializeLayers();
    
    /**
     * @brief Forward pass through the network
     * 
     * @param X Input features
     */
    void forwardPass(const Eigen::MatrixXd& X);
    
    /**
     * @brief Backward pass (backpropagation)
     * 
     * @param X Input features
     * @param y Target values
     */
    void backwardPass(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    
    /**
     * @brief Update weights and biases using gradient descent
     * 
     * @param learningRate Learning rate for the update
     */
    void updateParameters(double learningRate);
    
    /**
     * @brief Calculate the loss (mean squared error)
     * 
     * @param predictions Predicted values
     * @param targets Actual target values
     * @return double Loss value
     */
    double calculateLoss(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const;
    
    /**
     * @brief Standardize the input features
     * 
     * @param X Input features
     * @return Eigen::MatrixXd Standardized features
     */
    Eigen::MatrixXd standardizeFeatures(const Eigen::MatrixXd& X) const;
    
    // Feature scaling parameters
    Eigen::VectorXd featureMeans;
    Eigen::VectorXd featureStdDevs;
    double targetMean;
    double targetStdDev;
    
    /**
     * @brief Calculate feature importance using permutation importance
     * 
     * @param X Input features
     * @param y Target values
     */
    void calculateFeatureImportance(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
}; 