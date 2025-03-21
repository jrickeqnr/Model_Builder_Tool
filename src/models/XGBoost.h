#pragma once

#include "models/Model.h"
#include <vector>
#include <memory>

/**
 * @brief XGBoost gradient boosting tree model
 * 
 * This class implements XGBoost, a powerful gradient boosting library
 * for tree-based models. It provides a wrapper around the core XGBoost
 * algorithm for regression tasks.
 */
class XGBoost : public Model {
public:
    XGBoost();
    
    /**
     * @brief Constructor with hyperparameters
     * 
     * @param learning_rate Learning rate for gradient descent
     * @param max_depth Maximum tree depth
     * @param n_estimators Number of trees (boosting rounds)
     * @param subsample Subsample ratio of training instances
     * @param colsample_bytree Subsample ratio of columns when constructing each tree
     * @param min_child_weight Minimum sum of instance weight needed in a child
     * @param gamma Minimum loss reduction required to make a further partition
     */
    XGBoost(double learning_rate, int max_depth, int n_estimators,
           double subsample, double colsample_bytree, 
           int min_child_weight, double gamma);
    
    ~XGBoost() override = default;

    /**
     * @brief Fit the XGBoost model to the given data
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
     * @brief Make predictions using the fitted XGBoost model
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
     * @brief Get a description of the XGBoost model
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
     * For XGBoost, feature importance is calculated using the 'gain' metric,
     * which represents the improvement in accuracy brought by a feature to the branches it is on.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    std::unordered_map<std::string, double> getFeatureImportance() const override;

private:
    // Model hyperparameters
    double learningRate;
    int maxDepth;
    int nEstimators;
    double subsample;
    double colsampleBytree;
    int minChildWeight;
    double gamma;
    
    // Model state
    bool isFitted;
    int nSamples;
    int nFeatures;
    double rmse;
    
    // Variable names storage
    std::vector<std::string> inputVariableNames;
    std::string targetVariableName;
    
    // Feature importance scores
    std::unordered_map<std::string, double> featureImportanceScores;
    
    // Model implementation details
    class TreeNode {
    public:
        bool isLeaf;
        int featureIndex;
        double splitValue;
        double outputValue;
        std::shared_ptr<TreeNode> leftChild;
        std::shared_ptr<TreeNode> rightChild;
        
        TreeNode() : isLeaf(true), featureIndex(-1), splitValue(0.0), outputValue(0.0),
                   leftChild(nullptr), rightChild(nullptr) {}
    };
    
    class Tree {
    public:
        std::shared_ptr<TreeNode> root;
        Tree() : root(std::make_shared<TreeNode>()) {}
    };
    
    std::vector<Tree> trees;
    double initialPrediction;
    
    /**
     * @brief Build a regression tree
     * 
     * @param X Input features
     * @param gradients Gradient values
     * @param hessians Second-order gradient values
     * @param sampleIndices Indices of samples to use
     * @param depth Current depth
     * @return std::shared_ptr<TreeNode> Root node of the tree
     */
    std::shared_ptr<TreeNode> buildTree(const Eigen::MatrixXd& X, 
                                       const Eigen::VectorXd& gradients,
                                       const Eigen::VectorXd& hessians,
                                       const std::vector<int>& sampleIndices,
                                       int depth);
    
    /**
     * @brief Find the best split for a node
     * 
     * @param X Input features
     * @param gradients Gradient values
     * @param hessians Second-order gradient values
     * @param sampleIndices Indices of samples to use
     * @param featureIndices Indices of features to consider
     * @param bestFeatureIndex Best feature index (output)
     * @param bestSplitValue Best split value (output)
     * @param bestGain Best gain value (output)
     * @param leftIndices Left child sample indices (output)
     * @param rightIndices Right child sample indices (output)
     */
    void findBestSplit(const Eigen::MatrixXd& X,
                     const Eigen::VectorXd& gradients,
                     const Eigen::VectorXd& hessians,
                     const std::vector<int>& sampleIndices,
                     const std::vector<int>& featureIndices,
                     int& bestFeatureIndex,
                     double& bestSplitValue,
                     double& bestGain,
                     std::vector<int>& leftIndices,
                     std::vector<int>& rightIndices);
    
    /**
     * @brief Calculate the output value for a leaf node
     * 
     * @param gradients Gradient values
     * @param hessians Second-order gradient values
     * @param sampleIndices Indices of samples in this leaf
     * @return double Output value
     */
    double calculateLeafValue(const Eigen::VectorXd& gradients,
                            const Eigen::VectorXd& hessians,
                            const std::vector<int>& sampleIndices);
    
    /**
     * @brief Predict using a single tree
     * 
     * @param x Single instance features
     * @param node Current tree node
     * @return double Prediction value
     */
    double predictTree(const Eigen::VectorXd& x, const std::shared_ptr<TreeNode>& node) const;
    
    /**
     * @brief Calculate tree prediction for all instances
     * 
     * @param X Input features
     * @return Eigen::VectorXd Predictions
     */
    Eigen::VectorXd predictAllTrees(const Eigen::MatrixXd& X) const;
    
    /**
     * @brief Calculate feature importance based on the trained trees
     */
    void calculateFeatureImportance();
}; 