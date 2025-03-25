#include "models/XGBoost.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include <unordered_set>

XGBoost::XGBoost()
    : learningRate(0.1), maxDepth(6), nEstimators(100), 
      subsample(1.0), colsampleBytree(1.0), minChildWeight(1), gamma(0.0),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0), initialPrediction(0.0) {
}

XGBoost::XGBoost(double learning_rate, int max_depth, int n_estimators,
                double subsample, double colsample_bytree, 
                int min_child_weight, double gamma)
    : learningRate(learning_rate), maxDepth(max_depth), nEstimators(n_estimators),
      subsample(subsample), colsampleBytree(colsample_bytree), 
      minChildWeight(min_child_weight), gamma(gamma),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0), initialPrediction(0.0) {
}

bool XGBoost::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
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
        
        // Initialize random number generator
        std::random_device rd;
        std::mt19937 generator(rd());
        
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
        
        // Clear existing trees
        trees.clear();
        
        // Initialize with the mean of the target variable
        initialPrediction = y.mean();
        
        // Current predictions
        Eigen::VectorXd F = Eigen::VectorXd::Constant(nSamples, initialPrediction);
        
        // Boosting iterations
        for (int iter = 0; iter < nEstimators; ++iter) {
            // Calculate gradients (for squared error loss, gradient = prediction - target)
            Eigen::VectorXd gradients = F - y;
            
            // For squared error loss, hessian is constant = 1 for all samples
            Eigen::VectorXd hessians = Eigen::VectorXd::Ones(nSamples);
            
            // Subsample training instances
            std::vector<int> sampleIndices;
            if (subsample < 1.0) {
                int subsampleSize = static_cast<int>(nSamples * subsample);
                sampleIndices.resize(nSamples);
                for (int i = 0; i < nSamples; ++i) {
                    sampleIndices[i] = i;
                }
                
                std::shuffle(sampleIndices.begin(), sampleIndices.end(), generator);
                sampleIndices.resize(subsampleSize);
            } else {
                // Use all samples
                sampleIndices.resize(nSamples);
                for (int i = 0; i < nSamples; ++i) {
                    sampleIndices[i] = i;
                }
            }
            
            // Build a new tree
            Tree tree;
            tree.root = buildTree(X, gradients, hessians, sampleIndices, 0);
            trees.push_back(tree);
            
            // Update predictions
            Eigen::VectorXd treeOutput = Eigen::VectorXd::Zero(nSamples);
            for (int i = 0; i < nSamples; ++i) {
                treeOutput(i) = predictTree(X.row(i), tree.root);
            }
            
            F += learningRate * treeOutput;
        }
        
        // Calculate feature importance
        calculateFeatureImportance();
        
        // Calculate RMSE directly instead of using predict()
        Eigen::VectorXd predictions = predictAllTrees(X);
        rmse = std::sqrt((predictions - y).array().square().mean());
        
        isFitted = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting XGBoost model: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<XGBoost::TreeNode> XGBoost::buildTree(
    const Eigen::MatrixXd& X, 
    const Eigen::VectorXd& gradients,
    const Eigen::VectorXd& hessians,
    const std::vector<int>& sampleIndices,
    int depth) {
    
    // Create a new node
    auto node = std::make_shared<TreeNode>();
    
    // Check if we've reached maximum depth or have too few samples
    if (depth >= maxDepth || sampleIndices.size() <= minChildWeight) {
        node->isLeaf = true;
        node->outputValue = calculateLeafValue(gradients, hessians, sampleIndices);
        return node;
    }
    
    // Select features for this tree (column subsampling)
    std::vector<int> featureIndices;
    if (colsampleBytree < 1.0) {
        std::random_device rd;
        std::mt19937 generator(rd());
        
        int colsampleSize = static_cast<int>(nFeatures * colsampleBytree);
        featureIndices.resize(nFeatures);
        for (int i = 0; i < nFeatures; ++i) {
            featureIndices[i] = i;
        }
        
        std::shuffle(featureIndices.begin(), featureIndices.end(), generator);
        featureIndices.resize(colsampleSize);
    } else {
        // Use all features
        featureIndices.resize(nFeatures);
        for (int i = 0; i < nFeatures; ++i) {
            featureIndices[i] = i;
        }
    }
    
    // Find the best split
    int bestFeatureIndex;
    double bestSplitValue;
    double bestGain;
    std::vector<int> leftIndices, rightIndices;
    
    findBestSplit(X, gradients, hessians, sampleIndices, featureIndices,
                 bestFeatureIndex, bestSplitValue, bestGain, leftIndices, rightIndices);
    
    // If no good split is found or the gain is below gamma threshold, make it a leaf
    if (bestGain <= gamma || leftIndices.empty() || rightIndices.empty()) {
        node->isLeaf = true;
        node->outputValue = calculateLeafValue(gradients, hessians, sampleIndices);
        return node;
    }
    
    // Otherwise, create a split node
    node->isLeaf = false;
    node->featureIndex = bestFeatureIndex;
    node->splitValue = bestSplitValue;
    
    // Recursively build left and right subtrees
    node->leftChild = buildTree(X, gradients, hessians, leftIndices, depth + 1);
    node->rightChild = buildTree(X, gradients, hessians, rightIndices, depth + 1);
    
    return node;
}

void XGBoost::findBestSplit(
    const Eigen::MatrixXd& X,
    const Eigen::VectorXd& gradients,
    const Eigen::VectorXd& hessians,
    const std::vector<int>& sampleIndices,
    const std::vector<int>& featureIndices,
    int& bestFeatureIndex,
    double& bestSplitValue,
    double& bestGain,
    std::vector<int>& leftIndices,
    std::vector<int>& rightIndices) {
    
    // Initialize best values
    bestGain = -1.0;
    bestFeatureIndex = -1;
    bestSplitValue = 0.0;
    
    // Calculate sum of gradients and hessians for the current node
    double sumGradients = 0.0;
    double sumHessians = 0.0;
    for (int idx : sampleIndices) {
        sumGradients += gradients(idx);
        sumHessians += hessians(idx);
    }
    
    // Current node score (without splitting)
    double currentScore = sumGradients * sumGradients / (sumHessians + 1e-6);
    
    // Try splitting on each feature
    for (int featIdx : featureIndices) {
        // Sort sample indices by the feature value
        std::vector<std::pair<double, int>> featureValueIndices;
        for (int idx : sampleIndices) {
            featureValueIndices.push_back({X(idx, featIdx), idx});
        }
        
        std::sort(featureValueIndices.begin(), featureValueIndices.end());
        
        // Try each possible split point
        double leftGradSum = 0.0;
        double leftHessSum = 0.0;
        
        // Skip the last element since we need at least one sample on each side
        for (size_t i = 0; i < featureValueIndices.size() - 1; ++i) {
            int idx = featureValueIndices[i].second;
            leftGradSum += gradients(idx);
            leftHessSum += hessians(idx);
            
            double rightGradSum = sumGradients - leftGradSum;
            double rightHessSum = sumHessians - leftHessSum;
            
            // Skip if either side has too few hessians (weights)
            if (leftHessSum < minChildWeight || rightHessSum < minChildWeight) {
                continue;
            }
            
            // Calculate the gain for this split
            double leftScore = leftGradSum * leftGradSum / (leftHessSum + 1e-6);
            double rightScore = rightGradSum * rightGradSum / (rightHessSum + 1e-6);
            double gain = leftScore + rightScore - currentScore;
            
            // Check if this is the best split so far
            if (gain > bestGain) {
                bestGain = gain;
                bestFeatureIndex = featIdx;
                
                // Use average of current and next value as split point if not the last element
                if (i < featureValueIndices.size() - 1) {
                    bestSplitValue = (featureValueIndices[i].first + featureValueIndices[i + 1].first) / 2.0;
                } else {
                    bestSplitValue = featureValueIndices[i].first;
                }
                
                // Update the sample indices for left and right children
                leftIndices.clear();
                rightIndices.clear();
                
                for (int idx : sampleIndices) {
                    if (X(idx, featIdx) <= bestSplitValue) {
                        leftIndices.push_back(idx);
                    } else {
                        rightIndices.push_back(idx);
                    }
                }
            }
        }
    }
}

double XGBoost::calculateLeafValue(
    const Eigen::VectorXd& gradients,
    const Eigen::VectorXd& hessians,
    const std::vector<int>& sampleIndices) {
    
    double sumGradients = 0.0;
    double sumHessians = 0.0;
    
    for (int idx : sampleIndices) {
        sumGradients += gradients(idx);
        sumHessians += hessians(idx);
    }
    
    // Newton step: -sum(grad) / sum(hess)
    return -sumGradients / (sumHessians + 1e-6);
}

double XGBoost::predictTree(const Eigen::VectorXd& x, const std::shared_ptr<TreeNode>& node) const {
    if (!node) {
        return 0.0;
    }
    
    if (node->isLeaf) {
        return node->outputValue;
    }
    
    // Navigate to the appropriate child based on the feature value
    if (x(node->featureIndex) <= node->splitValue) {
        return predictTree(x, node->leftChild);
    } else {
        return predictTree(x, node->rightChild);
    }
}

Eigen::VectorXd XGBoost::predictAllTrees(const Eigen::MatrixXd& X) const {
    Eigen::VectorXd predictions = Eigen::VectorXd::Constant(X.rows(), initialPrediction);
    
    for (const auto& tree : trees) {
        for (int i = 0; i < X.rows(); ++i) {
            predictions(i) += learningRate * predictTree(X.row(i), tree.root);
        }
    }
    
    return predictions;
}

Eigen::VectorXd XGBoost::predict(const Eigen::MatrixXd& X) const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    if (X.cols() != nFeatures) {
        throw std::invalid_argument("Number of features in X (" + std::to_string(X.cols()) + 
                                   ") does not match the number of features the model was trained on (" + 
                                   std::to_string(nFeatures) + ")");
    }
    
    return predictAllTrees(X);
}

std::string XGBoost::getName() const {
    return "XGBoost";
}

std::unordered_map<std::string, double> XGBoost::getParameters() const {
    std::unordered_map<std::string, double> params;
    params["learning_rate"] = learningRate;
    params["max_depth"] = static_cast<double>(maxDepth);
    params["n_estimators"] = static_cast<double>(nEstimators);
    params["subsample"] = subsample;
    params["colsample_bytree"] = colsampleBytree;
    params["min_child_weight"] = static_cast<double>(minChildWeight);
    params["gamma"] = gamma;
    
    return params;
}

std::unordered_map<std::string, double> XGBoost::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    stats["rmse"] = rmse;
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    stats["n_trees"] = static_cast<double>(trees.size());
    
    return stats;
}

std::string XGBoost::getDescription() const {
    return "XGBoost Gradient Boosting Trees for regression.";
}

std::vector<std::string> XGBoost::getVariableNames() const {
    return inputVariableNames;
}

std::string XGBoost::getTargetName() const {
    return targetVariableName;
}

void XGBoost::calculateFeatureImportance() {
    // Initialize importance scores to zero
    featureImportanceScores.clear();
    for (const auto& name : inputVariableNames) {
        featureImportanceScores[name] = 0.0;
    }
    
    // Count the number of times each feature is used for splitting across all trees
    std::unordered_map<int, int> featureCounts;
    for (int i = 0; i < nFeatures; ++i) {
        featureCounts[i] = 0;
    }
    
    // Function to recursively traverse tree and count feature usage
    std::function<void(const std::shared_ptr<TreeNode>&)> countFeatures = 
        [&](const std::shared_ptr<TreeNode>& node) {
            if (!node || node->isLeaf) {
                return;
            }
            
            // Increment count for this feature
            featureCounts[node->featureIndex]++;
            
            // Recursively process children
            countFeatures(node->leftChild);
            countFeatures(node->rightChild);
        };
    
    // Count features in all trees
    for (const auto& tree : trees) {
        countFeatures(tree.root);
    }
    
    // Calculate total splits
    int totalSplits = 0;
    for (const auto& pair : featureCounts) {
        totalSplits += pair.second;
    }
    
    // Normalize importance scores
    if (totalSplits > 0) {
        for (int i = 0; i < nFeatures; ++i) {
            if (i < inputVariableNames.size()) {
                featureImportanceScores[inputVariableNames[i]] = 
                    static_cast<double>(featureCounts[i]) / totalSplits;
            }
        }
    } else {
        // If no splits were made, assign equal importance
        double equalImportance = 1.0 / nFeatures;
        for (int i = 0; i < nFeatures; ++i) {
            if (i < inputVariableNames.size()) {
                featureImportanceScores[inputVariableNames[i]] = equalImportance;
            }
        }
    }
}

std::unordered_map<std::string, double> XGBoost::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    return featureImportanceScores;
} 