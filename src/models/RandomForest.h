#pragma once

#include "models/Model.h"
#include <vector>
#include <memory>
#include <random>

/**
 * @brief Random Forest regression model
 * 
 * This class implements a Random Forest regression model, which is an ensemble
 * of decision trees. Each tree is trained on a bootstrap sample of the training
 * data, and split decisions at each node consider a random subset of features.
 */
class RandomForest : public Model {
public:
    RandomForest();
    
    /**
     * @brief Constructor with hyperparameters
     * 
     * @param n_estimators Number of trees in the forest
     * @param max_depth Maximum depth of each tree
     * @param min_samples_split Minimum number of samples required to split a node
     * @param min_samples_leaf Minimum number of samples required in a leaf node
     * @param max_features Method for selecting the maximum number of features 
     * @param bootstrap Whether to use bootstrap samples
     */
    RandomForest(int n_estimators, int max_depth, int min_samples_split, 
                int min_samples_leaf, const std::string& max_features, bool bootstrap);
    
    ~RandomForest() override = default;

    /**
     * @brief Fit the Random Forest model to the given data
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
     * @brief Make predictions using the fitted Random Forest model
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
     * @brief Get a description of the Random Forest model
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
     * For Random Forest, feature importance is calculated based on the decrease in impurity
     * (using variance for regression tasks) when splitting on each feature, averaged across all trees.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    std::unordered_map<std::string, double> getFeatureImportance() const override;

private:
    // Model hyperparameters
    int nEstimators;
    int maxDepth;
    int minSamplesSplit;
    int minSamplesLeaf;
    std::string maxFeatures;
    bool bootstrap;
    
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
    
    // Random number generator
    std::mt19937 rng;
    
    // Model implementation details
    class TreeNode {
    public:
        bool isLeaf;
        int featureIndex;
        double splitValue;
        double outputValue;
        std::shared_ptr<TreeNode> leftChild;
        std::shared_ptr<TreeNode> rightChild;
        double impurityDecrease;
        
        TreeNode() : isLeaf(true), featureIndex(-1), splitValue(0.0), outputValue(0.0),
                   leftChild(nullptr), rightChild(nullptr), impurityDecrease(0.0) {}
    };
    
    class DecisionTree {
    public:
        std::shared_ptr<TreeNode> root;
        std::vector<int> featureImportance;
        
        DecisionTree() : root(std::make_shared<TreeNode>()) {
        }
    };
    
    std::vector<DecisionTree> trees;
    
    /**
     * @brief Build a decision tree
     * 
     * @param X Input features
     * @param y Target values
     * @param sampleIndices Indices of samples to use for this tree
     * @param depth Current depth
     * @param tree Reference to the tree being built
     * @return std::shared_ptr<TreeNode> Root node of the tree
     */
    std::shared_ptr<TreeNode> buildTree(const Eigen::MatrixXd& X, 
                                      const Eigen::VectorXd& y,
                                      const std::vector<int>& sampleIndices,
                                      int depth,
                                      DecisionTree& tree);
    
    /**
     * @brief Find the best split for a node
     * 
     * @param X Input features
     * @param y Target values
     * @param sampleIndices Indices of samples to use
     * @param featureIndices Indices of features to consider
     * @param bestFeatureIndex Best feature index (output)
     * @param bestSplitValue Best split value (output)
     * @param bestScore Best score (output)
     * @param impurityDecrease Decrease in impurity (output)
     * @param leftIndices Left child sample indices (output)
     * @param rightIndices Right child sample indices (output)
     */
    void findBestSplit(const Eigen::MatrixXd& X,
                     const Eigen::VectorXd& y,
                     const std::vector<int>& sampleIndices,
                     const std::vector<int>& featureIndices,
                     int& bestFeatureIndex,
                     double& bestSplitValue,
                     double& bestScore,
                     double& impurityDecrease,
                     std::vector<int>& leftIndices,
                     std::vector<int>& rightIndices);
    
    /**
     * @brief Calculate the variance (impurity measure for regression)
     * 
     * @param y Target values
     * @param indices Sample indices
     * @return double Variance
     */
    double calculateVariance(const Eigen::VectorXd& y, const std::vector<int>& indices) const;
    
    /**
     * @brief Calculate the mean of target values for a set of samples
     * 
     * @param y Target values
     * @param indices Sample indices
     * @return double Mean value
     */
    double calculateMean(const Eigen::VectorXd& y, const std::vector<int>& indices) const;
    
    /**
     * @brief Predict using a single tree
     * 
     * @param x Single instance features
     * @param node Current tree node
     * @return double Prediction value
     */
    double predictTree(const Eigen::VectorXd& x, const std::shared_ptr<TreeNode>& node) const;
    
    /**
     * @brief Get the number of features to consider at each split
     * 
     * @return int Number of features
     */
    int getNumFeaturesToConsider() const;
    
    /**
     * @brief Calculate feature importance based on impurity decrease
     */
    void calculateFeatureImportance();
}; 