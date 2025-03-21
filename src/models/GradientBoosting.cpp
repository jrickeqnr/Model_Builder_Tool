#include "models/GradientBoosting.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include <numeric>

GradientBoosting::GradientBoosting()
    : learningRate(0.1), nEstimators(100), maxDepth(3), minSamplesSplit(2), 
      minSamplesLeaf(1), subsample(1.0), loss("squared_error"),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0), initialPrediction(0.0) {
}

GradientBoosting::GradientBoosting(double learning_rate, int n_estimators, int max_depth, 
                                 int min_samples_split, int min_samples_leaf, 
                                 double subsample, const std::string& loss)
    : learningRate(learning_rate), nEstimators(n_estimators), maxDepth(max_depth),
      minSamplesSplit(min_samples_split), minSamplesLeaf(min_samples_leaf),
      subsample(subsample), loss(loss),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0), initialPrediction(0.0) {
}

bool GradientBoosting::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
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
        std::mt19937 rng(rd());
        
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
        
        // Step 1: Initialize model with a constant value
        // For regression, this is the mean of the target values
        initialPrediction = y.mean();
        
        // Current predictions
        Eigen::VectorXd F = Eigen::VectorXd::Constant(nSamples, initialPrediction);
        
        // Step 2: For m = 1 to M (number of estimators):
        for (int m = 0; m < nEstimators; ++m) {
            // a) Calculate pseudo-residuals
            Eigen::VectorXd residuals = calculatePseudoResiduals(y, F);
            
            // b) Subsample for this tree
            std::vector<int> sampleIndices;
            
            if (subsample < 1.0) {
                int subsampleSize = static_cast<int>(nSamples * subsample);
                std::vector<int> allIndices(nSamples);
                std::iota(allIndices.begin(), allIndices.end(), 0);
                std::shuffle(allIndices.begin(), allIndices.end(), rng);
                sampleIndices.assign(allIndices.begin(), allIndices.begin() + subsampleSize);
            } else {
                // Use all samples
                sampleIndices.resize(nSamples);
                std::iota(sampleIndices.begin(), sampleIndices.end(), 0);
            }
            
            // c) Fit a regression tree to the pseudo-residuals
            RegressionTree tree(nFeatures);
            tree.root = buildTree(X, residuals, sampleIndices, 0, tree);
            
            // d) Update the model
            for (int i = 0; i < nSamples; ++i) {
                F(i) += learningRate * predictTree(X.row(i), tree.root);
            }
            
            // Store the tree
            trees.push_back(tree);
        }
        
        // Calculate RMSE
        Eigen::VectorXd predictions = predict(X);
        rmse = std::sqrt((predictions - y).array().square().mean());
        
        // Calculate feature importance
        calculateFeatureImportance();
        
        isFitted = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting Gradient Boosting model: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<GradientBoosting::TreeNode> GradientBoosting::buildTree(
    const Eigen::MatrixXd& X, 
    const Eigen::VectorXd& residuals,
    const std::vector<int>& sampleIndices,
    int depth,
    RegressionTree& tree) {
    
    // Create a new node
    auto node = std::make_shared<TreeNode>();
    
    // Check stopping criteria
    if (depth >= maxDepth || 
        static_cast<int>(sampleIndices.size()) < minSamplesSplit || 
        static_cast<int>(sampleIndices.size()) <= minSamplesLeaf) {
        
        node->isLeaf = true;
        node->outputValue = calculateMean(residuals, sampleIndices);
        return node;
    }
    
    // Find the best split
    int bestFeatureIndex;
    double bestSplitValue;
    double bestScore;
    double impurityDecrease;
    std::vector<int> leftIndices, rightIndices;
    
    findBestSplit(X, residuals, sampleIndices, bestFeatureIndex, bestSplitValue, 
                bestScore, impurityDecrease, leftIndices, rightIndices);
    
    // If we couldn't find a good split or one side is empty, make it a leaf
    if (bestFeatureIndex == -1 || leftIndices.empty() || rightIndices.empty() || 
        static_cast<int>(leftIndices.size()) < minSamplesLeaf || 
        static_cast<int>(rightIndices.size()) < minSamplesLeaf) {
        
        node->isLeaf = true;
        node->outputValue = calculateMean(residuals, sampleIndices);
        return node;
    }
    
    // Otherwise, create a split node
    node->isLeaf = false;
    node->featureIndex = bestFeatureIndex;
    node->splitValue = bestSplitValue;
    node->impurityDecrease = impurityDecrease;
    
    // Update feature importance in this tree
    tree.featureImportance[bestFeatureIndex] += impurityDecrease * sampleIndices.size();
    
    // Recursively build left and right subtrees
    node->leftChild = buildTree(X, residuals, leftIndices, depth + 1, tree);
    node->rightChild = buildTree(X, residuals, rightIndices, depth + 1, tree);
    
    return node;
}

void GradientBoosting::findBestSplit(
    const Eigen::MatrixXd& X,
    const Eigen::VectorXd& residuals,
    const std::vector<int>& sampleIndices,
    int& bestFeatureIndex,
    double& bestSplitValue,
    double& bestScore,
    double& impurityDecrease,
    std::vector<int>& leftIndices,
    std::vector<int>& rightIndices) {
    
    // Initialize best values
    bestScore = std::numeric_limits<double>::lowest();
    bestFeatureIndex = -1;
    bestSplitValue = 0.0;
    impurityDecrease = 0.0;
    
    // Calculate the MSE before splitting
    double nodeMSE = calculateMSE(residuals, sampleIndices);
    double nodeSize = static_cast<double>(sampleIndices.size());
    
    // Try splitting on each feature
    for (int featIdx = 0; featIdx < nFeatures; ++featIdx) {
        // Sort sample indices by the feature value
        std::vector<std::pair<double, int>> featureValueIndices;
        for (int idx : sampleIndices) {
            featureValueIndices.push_back({X(idx, featIdx), idx});
        }
        
        std::sort(featureValueIndices.begin(), featureValueIndices.end());
        
        // Try each possible split point
        for (size_t i = 0; i < featureValueIndices.size() - 1; ++i) {
            // Skip if the next value is the same as the current one
            if (i < featureValueIndices.size() - 1 && 
                featureValueIndices[i].first == featureValueIndices[i + 1].first) {
                continue;
            }
            
            // Generate left and right sample indices
            std::vector<int> tempLeftIndices, tempRightIndices;
            for (const auto& pair : featureValueIndices) {
                if (pair.first <= featureValueIndices[i].first) {
                    tempLeftIndices.push_back(pair.second);
                } else {
                    tempRightIndices.push_back(pair.second);
                }
            }
            
            // Skip if either side has too few samples
            if (static_cast<int>(tempLeftIndices.size()) < minSamplesLeaf || 
                static_cast<int>(tempRightIndices.size()) < minSamplesLeaf) {
                continue;
            }
            
            // Calculate MSE after splitting
            double leftMSE = calculateMSE(residuals, tempLeftIndices);
            double rightMSE = calculateMSE(residuals, tempRightIndices);
            
            // Calculate weighted MSE reduction (score)
            double leftSize = static_cast<double>(tempLeftIndices.size());
            double rightSize = static_cast<double>(tempRightIndices.size());
            
            double weightedMSE = (leftSize * leftMSE + rightSize * rightMSE) / nodeSize;
            double score = nodeMSE - weightedMSE;
            
            // Check if this is the best split so far
            if (score > bestScore) {
                bestScore = score;
                bestFeatureIndex = featIdx;
                
                // Use average of current and next value as split point if possible
                if (i < featureValueIndices.size() - 1) {
                    bestSplitValue = (featureValueIndices[i].first + featureValueIndices[i + 1].first) / 2.0;
                } else {
                    bestSplitValue = featureValueIndices[i].first;
                }
                
                // Calculate impurity decrease
                impurityDecrease = nodeMSE - weightedMSE;
                
                // Update the sample indices for left and right children
                leftIndices = tempLeftIndices;
                rightIndices = tempRightIndices;
            }
        }
    }
}

double GradientBoosting::calculateMSE(const Eigen::VectorXd& residuals, const std::vector<int>& indices) const {
    if (indices.empty()) {
        return 0.0;
    }
    
    double mean = calculateMean(residuals, indices);
    
    double sumSquaredDiff = 0.0;
    for (int idx : indices) {
        double diff = residuals(idx) - mean;
        sumSquaredDiff += diff * diff;
    }
    
    return sumSquaredDiff / indices.size();
}

double GradientBoosting::calculateMean(const Eigen::VectorXd& residuals, const std::vector<int>& indices) const {
    if (indices.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (int idx : indices) {
        sum += residuals(idx);
    }
    
    return sum / indices.size();
}

Eigen::VectorXd GradientBoosting::calculatePseudoResiduals(
    const Eigen::VectorXd& y, const Eigen::VectorXd& predictions) const {
    
    if (loss == "squared_error") {
        // For squared error, the negative gradient is simply (y - prediction)
        return y - predictions;
    } else if (loss == "absolute_error") {
        // For absolute error, the negative gradient is sign(y - prediction)
        Eigen::VectorXd residuals(y.size());
        for (int i = 0; i < y.size(); ++i) {
            double diff = y(i) - predictions(i);
            residuals(i) = (diff > 0) ? 1.0 : ((diff < 0) ? -1.0 : 0.0);
        }
        return residuals;
    } else if (loss == "huber") {
        // For Huber loss, combine MSE and MAE
        const double delta = 1.0;  // Threshold parameter
        Eigen::VectorXd residuals(y.size());
        for (int i = 0; i < y.size(); ++i) {
            double diff = y(i) - predictions(i);
            if (std::abs(diff) <= delta) {
                residuals(i) = diff;  // MSE gradient for small errors
            } else {
                residuals(i) = delta * ((diff > 0) ? 1.0 : -1.0);  // MAE gradient for large errors
            }
        }
        return residuals;
    } else {
        // Default to squared error
        return y - predictions;
    }
}

double GradientBoosting::predictTree(const Eigen::VectorXd& x, const std::shared_ptr<TreeNode>& node) const {
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

Eigen::VectorXd GradientBoosting::predict(const Eigen::MatrixXd& X) const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    if (X.cols() != nFeatures) {
        throw std::invalid_argument("Number of features in X (" + std::to_string(X.cols()) + 
                                   ") does not match the number of features the model was trained on (" + 
                                   std::to_string(nFeatures) + ")");
    }
    
    // Initialize predictions with the initial prediction (global mean)
    Eigen::VectorXd predictions = Eigen::VectorXd::Constant(X.rows(), initialPrediction);
    
    // Add contributions from each tree
    for (const auto& tree : trees) {
        for (int i = 0; i < X.rows(); ++i) {
            predictions(i) += learningRate * predictTree(X.row(i), tree.root);
        }
    }
    
    return predictions;
}

std::string GradientBoosting::getName() const {
    return "Gradient Boosting";
}

std::unordered_map<std::string, double> GradientBoosting::getParameters() const {
    std::unordered_map<std::string, double> params;
    params["learning_rate"] = learningRate;
    params["n_estimators"] = static_cast<double>(nEstimators);
    params["max_depth"] = static_cast<double>(maxDepth);
    params["min_samples_split"] = static_cast<double>(minSamplesSplit);
    params["min_samples_leaf"] = static_cast<double>(minSamplesLeaf);
    params["subsample"] = subsample;
    
    return params;
}

std::unordered_map<std::string, double> GradientBoosting::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    stats["rmse"] = rmse;
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    stats["n_trees"] = static_cast<double>(trees.size());
    
    return stats;
}

std::string GradientBoosting::getDescription() const {
    std::string lossStr;
    if (loss == "squared_error") {
        lossStr = "squared error";
    } else if (loss == "absolute_error") {
        lossStr = "absolute error";
    } else if (loss == "huber") {
        lossStr = "Huber";
    } else if (loss == "quantile") {
        lossStr = "quantile";
    } else {
        lossStr = loss;
    }
    
    return "Gradient Boosting Regression with " + std::to_string(nEstimators) + 
           " trees and " + lossStr + " loss function.";
}

std::vector<std::string> GradientBoosting::getVariableNames() const {
    return inputVariableNames;
}

std::string GradientBoosting::getTargetName() const {
    return targetVariableName;
}

void GradientBoosting::calculateFeatureImportance() {
    // Initialize importance scores to zero
    featureImportanceScores.clear();
    for (const auto& name : inputVariableNames) {
        featureImportanceScores[name] = 0.0;
    }
    
    // Sum feature importance across all trees
    std::vector<double> importanceValues(nFeatures, 0.0);
    
    for (const auto& tree : trees) {
        for (int i = 0; i < nFeatures; ++i) {
            importanceValues[i] += tree.featureImportance[i];
        }
    }
    
    // Normalize importance scores
    double totalImportance = 0.0;
    for (int i = 0; i < nFeatures; ++i) {
        totalImportance += importanceValues[i];
    }
    
    if (totalImportance > 0.0) {
        for (int i = 0; i < nFeatures; ++i) {
            if (i < inputVariableNames.size()) {
                featureImportanceScores[inputVariableNames[i]] = importanceValues[i] / totalImportance;
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

std::unordered_map<std::string, double> GradientBoosting::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    return featureImportanceScores;
} 