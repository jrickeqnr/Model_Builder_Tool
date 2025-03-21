#pragma once

#include "models/Model.h"
#include <vector>
#include <memory>

/**
 * @brief Gradient Boosting regression model
 * 
 * This class implements Gradient Boosting for regression, which 
 * sequentially builds an ensemble of weak regression trees to
 * model the target variable. Each tree tries to correct the 
 * errors made by the previous trees.
 */
class GradientBoosting : public Model {
public:
    GradientBoosting();
    
    /**
     * @brief Constructor with hyperparameters
     * 
     * @param learning_rate Learning rate shrinks the contribution of each tree
     * @param n_estimators Number of boosting stages (trees)
     * @param max_depth Maximum depth of each tree
     * @param min_samples_split Minimum number of samples required to split a node
     * @param min_samples_leaf Minimum number of samples required in a leaf node
     * @param subsample Fraction of samples used for fitting each tree
     * @param loss Loss function to be optimized
     */
    GradientBoosting(double learning_rate, int n_estimators, int max_depth, 
                    int min_samples_split, int min_samples_leaf, 
                    double subsample, const std::string& loss);
    
    ~GradientBoosting() override = default;

    /**
     * @brief Fit the Gradient Boosting model to the given data
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
     * @brief Make predictions using the fitted Gradient Boosting model
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
     * @brief Get a description of the Gradient Boosting model
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
     * For Gradient Boosting, feature importance is calculated based on the amount
     * each feature contributes to reducing the loss function across all trees.
     * 
     * @return std::unordered_map<std::string, double> Map of feature names to importance scores
     */
    std::unordered_map<std::string, double> getFeatureImportance() const override;

private:
    // Model hyperparameters
    double learningRate;
    int nEstimators;
    int maxDepth;
    int minSamplesSplit;
    int minSamplesLeaf;
    double subsample;
    std::string loss;
    
    // Model state
    bool isFitted;
    int nSamples;
    int nFeatures;
    double rmse;
    double initialPrediction;
    
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
        double impurityDecrease;
        
        TreeNode() : isLeaf(true), featureIndex(-1), splitValue(0.0), outputValue(0.0),
                   leftChild(nullptr), rightChild(nullptr), impurityDecrease(0.0) {}
    };
    
    class RegressionTree {
    public:
        std::shared_ptr<TreeNode> root;
        std::vector<double> featureImportance;
        
        RegressionTree(int nFeatures) : root(std::make_shared<TreeNode>()) {
            featureImportance.resize(nFeatures, 0.0);
        }
    };
    
    std::vector<RegressionTree> trees;
    
    /**
     * @brief Build a regression tree for gradient boosting
     * 
     * @param X Input features
     * @param residuals Current residuals
     * @param sampleIndices Indices of samples to use
     * @param depth Current depth
     * @param tree Reference to the tree being built
     * @return std::shared_ptr<TreeNode> Root node of the tree
     */
    std::shared_ptr<TreeNode> buildTree(const Eigen::MatrixXd& X, 
                                      const Eigen::VectorXd& residuals,
                                      const std::vector<int>& sampleIndices,
                                      int depth,
                                      RegressionTree& tree);
    
    /**
     * @brief Find the best split for a node
     * 
     * @param X Input features
     * @param residuals Current residuals
     * @param sampleIndices Indices of samples to use
     * @param bestFeatureIndex Best feature index (output)
     * @param bestSplitValue Best split value (output)
     * @param bestScore Best score (output)
     * @param impurityDecrease Decrease in impurity (output)
     * @param leftIndices Left child sample indices (output)
     * @param rightIndices Right child sample indices (output)
     */
    void findBestSplit(const Eigen::MatrixXd& X,
                     const Eigen::VectorXd& residuals,
                     const std::vector<int>& sampleIndices,
                     int& bestFeatureIndex,
                     double& bestSplitValue,
                     double& bestScore,
                     double& impurityDecrease,
                     std::vector<int>& leftIndices,
                     std::vector<int>& rightIndices);
    
    /**
     * @brief Calculate the mean squared error
     * 
     * @param residuals Current residuals
     * @param indices Sample indices
     * @return double MSE value
     */
    double calculateMSE(const Eigen::VectorXd& residuals, const std::vector<int>& indices) const;
    
    /**
     * @brief Calculate the mean of residuals
     * 
     * @param residuals Current residuals
     * @param indices Sample indices
     * @return double Mean value
     */
    double calculateMean(const Eigen::VectorXd& residuals, const std::vector<int>& indices) const;
    
    /**
     * @brief Calculate the pseudo-residuals based on the loss function
     * 
     * @param y Target values
     * @param predictions Current predictions
     * @return Eigen::VectorXd Pseudo-residuals
     */
    Eigen::VectorXd calculatePseudoResiduals(const Eigen::VectorXd& y, const Eigen::VectorXd& predictions) const;
    
    /**
     * @brief Predict using a single tree
     * 
     * @param x Single instance features
     * @param node Current tree node
     * @return double Prediction value
     */
    double predictTree(const Eigen::VectorXd& x, const std::shared_ptr<TreeNode>& node) const;
    
    /**
     * @brief Calculate feature importance based on impurity reduction
     */
    void calculateFeatureImportance();
}; 