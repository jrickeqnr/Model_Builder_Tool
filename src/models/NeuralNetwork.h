#pragma once

#include "models/Model.h"
#include <vector>
#include <random>
#include <functional>

/**
 * @brief Activation function types for neural network layers
 */
enum class Activation {
    RELU,
    SIGMOID,
    TANH,
    LINEAR
};

/**
 * @brief Simple feedforward neural network model
 * 
 * This class implements a fully connected feedforward neural network 
 * with configurable layers and activation functions.
 */
class NeuralNetwork : public Model {
public:
    /**
     * @brief Default constructor
     */
    NeuralNetwork();
    
    /**
     * @brief Constructor with network architecture parameters
     * 
     * @param hiddenLayers Vector containing the number of neurons in each hidden layer
     * @param activation Activation function for hidden layers
     * @param outputActivation Activation function for output layer
     * @param learningRate Learning rate for gradient descent
     * @param epochs Maximum number of training epochs
     * @param batchSize Batch size for mini-batch gradient descent
     * @param tol Tolerance for convergence
     */
    NeuralNetwork(const std::vector<int>& hiddenLayers,
                 Activation activation = Activation::RELU,
                 Activation outputActivation = Activation::LINEAR,
                 double learningRate = 0.01,
                 int epochs = 1000,
                 int batchSize = 32,
                 double tol = 0.0001);
    
    /**
     * @brief Constructor with network architecture parameters (string version)
     * 
     * @param hiddenLayers Vector containing the number of neurons in each hidden layer
     * @param activation Activation function name (relu, sigmoid, tanh, identity)
     * @param learningRate Learning rate for gradient descent
     * @param epochs Maximum number of training epochs
     * @param batchSize Batch size for mini-batch gradient descent
     * @param solver Solver type (adam, sgd, lbfgs)
     * @param alpha L2 penalty (regularization)
     */
    NeuralNetwork(const std::vector<int>& hiddenLayers,
                 const std::string& activation,
                 double learningRate = 0.01,
                 int epochs = 1000,
                 int batchSize = 32,
                 const std::string& solver = "adam",
                 double alpha = 0.0001);
    
    ~NeuralNetwork() override = default;

    /**
     * @brief Fit the neural network to the given data
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
     * @brief Make predictions using the fitted neural network
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
     * @brief Get a description of the neural network model
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
     * For neural networks, feature importance is approximated using
     * a permutation-based method. Each feature is permuted and the
     * resulting change in prediction error is used as a measure of importance.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    std::unordered_map<std::string, double> getFeatureImportance() const override;

private:
    // Network architecture
    std::vector<int> layerSizes;
    Activation hiddenActivation;
    Activation outputActivation;
    
    // Training parameters
    double learningRate;
    int epochs;
    int batchSize;
    double tol;
    
    // Model state
    std::vector<Eigen::MatrixXd> weights;
    std::vector<Eigen::VectorXd> biases;
    double rSquared;
    double adjustedRSquared;
    double rmse;
    int nSamples;
    int nFeatures;
    bool isFitted;
    
    // Input/output names
    std::vector<std::string> inputVariableNames;
    std::string targetVariableName;
    
    // Feature normalization parameters
    Eigen::VectorXd featureMeans;
    Eigen::VectorXd featureStdDevs;
    double targetMean;
    double targetStdDev;

    /**
     * @brief Initialize network weights and biases
     */
    void initializeParameters();
    
    /**
     * @brief Forward propagation through the network
     * 
     * @param X Input features matrix
     * @return std::vector<Eigen::MatrixXd> Output and activations from each layer
     */
    std::vector<Eigen::MatrixXd> forwardPropagate(const Eigen::MatrixXd& X) const;
    
    /**
     * @brief Backward propagation to compute gradients
     * 
     * @param X Input features matrix
     * @param y Target values
     * @param activations Activations from forward propagation
     * @return std::pair<std::vector<Eigen::MatrixXd>, std::vector<Eigen::VectorXd>> Gradients for weights and biases
     */
    std::pair<std::vector<Eigen::MatrixXd>, std::vector<Eigen::VectorXd>> 
    backwardPropagate(const Eigen::MatrixXd& X, const Eigen::VectorXd& y, 
                      const std::vector<Eigen::MatrixXd>& activations);
    
    /**
     * @brief Update weights and biases with computed gradients
     * 
     * @param weightGrads Gradients for weights
     * @param biasGrads Gradients for biases
     */
    void updateParameters(const std::vector<Eigen::MatrixXd>& weightGrads,
                         const std::vector<Eigen::VectorXd>& biasGrads);
    
    /**
     * @brief Apply activation function to a matrix
     * 
     * @param X Input matrix
     * @param activation Activation function to apply
     * @return Eigen::MatrixXd Result after applying activation
     */
    Eigen::MatrixXd applyActivation(const Eigen::MatrixXd& X, Activation activation) const;
    
    /**
     * @brief Apply derivative of activation function to a matrix
     * 
     * @param X Input matrix
     * @param activation Activation function whose derivative to apply
     * @return Eigen::MatrixXd Result after applying activation derivative
     */
    Eigen::MatrixXd applyActivationDerivative(const Eigen::MatrixXd& X, Activation activation) const;
    
    /**
     * @brief Normalize input features
     * 
     * @param X Input features matrix
     * @return Eigen::MatrixXd Normalized features
     */
    Eigen::MatrixXd normalizeFeatures(const Eigen::MatrixXd& X) const;
    
    /**
     * @brief Calculate model statistics after fitting
     * 
     * @param X Input features used for fitting
     * @param y Target values used for fitting
     */
    void calculateStatistics(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    
    /**
     * @brief Calculate feature means and standard deviations
     * 
     * @param X Input features matrix
     * @param y Target values
     */
    void calculateNormalizationParams(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
}; 