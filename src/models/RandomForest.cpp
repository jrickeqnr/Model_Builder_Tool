#include "models/RandomForest.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <numeric>

RandomForest::RandomForest()
    : nEstimators(100), maxDepth(10), minSamplesSplit(2), minSamplesLeaf(1),
      maxFeatures("auto"), bootstrap(true), isFitted(false), nSamples(0), nFeatures(0), rmse(0.0) {
    // Initialize random number generator with a random seed
    std::random_device rd;
    rng = std::mt19937(rd());
}

RandomForest::RandomForest(int n_estimators, int max_depth, int min_samples_split, 
                         int min_samples_leaf, const std::string& max_features, bool bootstrap)
    : nEstimators(n_estimators), maxDepth(max_depth), minSamplesSplit(min_samples_split),
      minSamplesLeaf(min_samples_leaf), maxFeatures(max_features), bootstrap(bootstrap),
      isFitted(false), nSamples(0), nFeatures(0), rmse(0.0) {
    // Initialize random number generator with a random seed
    std::random_device rd;
    rng = std::mt19937(rd());
}

bool RandomForest::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y,
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
        
        // Clear existing trees
        trees.clear();
        
        // Create bootstrap indices for each tree
        std::uniform_int_distribution<int> dist(0, nSamples - 1);
        
        // Train each tree in the forest
        for (int i = 0; i < nEstimators; ++i) {
            // Create sample indices for this tree
            std::vector<int> sampleIndices;
            
            if (bootstrap) {
                // Bootstrap sampling (sampling with replacement)
                sampleIndices.resize(nSamples);
                for (int j = 0; j < nSamples; ++j) {
                    sampleIndices[j] = dist(rng);
                }
            } else {
                // Use all samples without bootstrap
                sampleIndices.resize(nSamples);
                std::iota(sampleIndices.begin(), sampleIndices.end(), 0);
            }
            
            // Create a new tree
            DecisionTree tree;
            tree.featureImportance.resize(nFeatures, 0);
            
            // Build the tree
            tree.root = buildTree(X, y, sampleIndices, 0, tree);
            
            // Add the tree to the forest
            trees.push_back(tree);
        }
        
        // Calculate feature importance
        calculateFeatureImportance();
        
        // Calculate RMSE directly instead of using predict()
        Eigen::VectorXd predictions = Eigen::VectorXd::Zero(nSamples);
        for (const auto& tree : trees) {
            for (int i = 0; i < nSamples; ++i) {
                predictions(i) += predictTree(X.row(i), tree.root);
            }
        }
        predictions /= nEstimators;
        rmse = std::sqrt((predictions - y).array().square().mean());
        
        isFitted = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error fitting Random Forest model: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<RandomForest::TreeNode> RandomForest::buildTree(
    const Eigen::MatrixXd& X, 
    const Eigen::VectorXd& y,
    const std::vector<int>& sampleIndices,
    int depth,
    DecisionTree& tree) {
    
    // Create a new node
    auto node = std::make_shared<TreeNode>();
    
    // Check stopping criteria
    if (depth >= maxDepth || 
        static_cast<int>(sampleIndices.size()) < minSamplesSplit || 
        static_cast<int>(sampleIndices.size()) <= minSamplesLeaf) {
        
        node->isLeaf = true;
        node->outputValue = calculateMean(y, sampleIndices);
        return node;
    }
    
    // Calculate the variance before splitting
    double nodeVariance = calculateVariance(y, sampleIndices);
    
    // If the variance is 0, all targets are the same, so make it a leaf
    if (nodeVariance < 1e-6) {
        node->isLeaf = true;
        node->outputValue = calculateMean(y, sampleIndices);
        return node;
    }
    
    // Get the number of features to consider
    int numFeaturesToConsider = getNumFeaturesToConsider();
    
    // Randomly select features to consider for this split
    std::vector<int> allFeatureIndices(nFeatures);
    std::iota(allFeatureIndices.begin(), allFeatureIndices.end(), 0);
    std::shuffle(allFeatureIndices.begin(), allFeatureIndices.end(), rng);
    
    std::vector<int> featureIndices(allFeatureIndices.begin(), 
                                  allFeatureIndices.begin() + numFeaturesToConsider);
    
    // Find the best split
    int bestFeatureIndex;
    double bestSplitValue;
    double bestScore;
    double impurityDecrease;
    std::vector<int> leftIndices, rightIndices;
    
    findBestSplit(X, y, sampleIndices, featureIndices, bestFeatureIndex, bestSplitValue, 
                bestScore, impurityDecrease, leftIndices, rightIndices);
    
    // If we couldn't find a good split or one side is empty, make it a leaf
    if (bestFeatureIndex == -1 || leftIndices.empty() || rightIndices.empty() || 
        static_cast<int>(leftIndices.size()) < minSamplesLeaf || 
        static_cast<int>(rightIndices.size()) < minSamplesLeaf) {
        
        node->isLeaf = true;
        node->outputValue = calculateMean(y, sampleIndices);
        return node;
    }
    
    // Otherwise, create a split node
    node->isLeaf = false;
    node->featureIndex = bestFeatureIndex;
    node->splitValue = bestSplitValue;
    node->impurityDecrease = impurityDecrease;
    
    // Update feature importance in this tree
    tree.featureImportance[bestFeatureIndex] += static_cast<int>(sampleIndices.size()) * impurityDecrease;
    
    // Recursively build left and right subtrees
    node->leftChild = buildTree(X, y, leftIndices, depth + 1, tree);
    node->rightChild = buildTree(X, y, rightIndices, depth + 1, tree);
    
    return node;
}

void RandomForest::findBestSplit(
    const Eigen::MatrixXd& X,
    const Eigen::VectorXd& y,
    const std::vector<int>& sampleIndices,
    const std::vector<int>& featureIndices,
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
    
    // Get the variance before splitting
    double nodeVariance = calculateVariance(y, sampleIndices);
    double nodeSize = static_cast<double>(sampleIndices.size());
    
    // Try splitting on each feature
    for (int featIdx : featureIndices) {
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
            
            // Calculate variances after splitting
            double leftVariance = calculateVariance(y, tempLeftIndices);
            double rightVariance = calculateVariance(y, tempRightIndices);
            
            // Calculate weighted variance reduction (score)
            double leftSize = static_cast<double>(tempLeftIndices.size());
            double rightSize = static_cast<double>(tempRightIndices.size());
            
            double weightedVariance = (leftSize * leftVariance + rightSize * rightVariance) / nodeSize;
            double score = nodeVariance - weightedVariance;
            
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
                impurityDecrease = nodeVariance - weightedVariance;
                
                // Update the sample indices for left and right children
                leftIndices = tempLeftIndices;
                rightIndices = tempRightIndices;
            }
        }
    }
}

double RandomForest::calculateVariance(const Eigen::VectorXd& y, const std::vector<int>& indices) const {
    if (indices.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (int idx : indices) {
        sum += y(idx);
    }
    
    double mean = sum / indices.size();
    
    double sumSquaredDiff = 0.0;
    for (int idx : indices) {
        double diff = y(idx) - mean;
        sumSquaredDiff += diff * diff;
    }
    
    return sumSquaredDiff / indices.size();
}

double RandomForest::calculateMean(const Eigen::VectorXd& y, const std::vector<int>& indices) const {
    if (indices.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (int idx : indices) {
        sum += y(idx);
    }
    
    return sum / indices.size();
}

double RandomForest::predictTree(const Eigen::VectorXd& x, const std::shared_ptr<TreeNode>& node) const {
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

int RandomForest::getNumFeaturesToConsider() const {
    if (maxFeatures == "auto" || maxFeatures == "sqrt") {
        // sqrt(n_features) is the default for regression in scikit-learn
        return std::max(1, static_cast<int>(std::sqrt(nFeatures)));
    } else if (maxFeatures == "log2") {
        return std::max(1, static_cast<int>(std::log2(nFeatures)));
    } else if (maxFeatures == "all") {
        return nFeatures;
    } else {
        // Default to auto/sqrt if invalid option
        return std::max(1, static_cast<int>(std::sqrt(nFeatures)));
    }
}

Eigen::VectorXd RandomForest::predict(const Eigen::MatrixXd& X) const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    if (X.cols() != nFeatures) {
        throw std::invalid_argument("Number of features in X (" + std::to_string(X.cols()) + 
                                   ") does not match the number of features the model was trained on (" + 
                                   std::to_string(nFeatures) + ")");
    }
    
    // Initialize predictions vector
    Eigen::VectorXd predictions = Eigen::VectorXd::Zero(X.rows());
    
    // Average predictions from all trees
    for (const auto& tree : trees) {
        for (int i = 0; i < X.rows(); ++i) {
            predictions(i) += predictTree(X.row(i), tree.root);
        }
    }
    
    return predictions / nEstimators;
}

std::string RandomForest::getName() const {
    return "Random Forest";
}

std::unordered_map<std::string, double> RandomForest::getParameters() const {
    std::unordered_map<std::string, double> params;
    params["n_estimators"] = static_cast<double>(nEstimators);
    params["max_depth"] = static_cast<double>(maxDepth);
    params["min_samples_split"] = static_cast<double>(minSamplesSplit);
    params["min_samples_leaf"] = static_cast<double>(minSamplesLeaf);
    params["bootstrap"] = bootstrap ? 1.0 : 0.0;
    
    return params;
}

std::unordered_map<std::string, double> RandomForest::getStatistics() const {
    std::unordered_map<std::string, double> stats;
    stats["rmse"] = rmse;
    stats["n_samples"] = static_cast<double>(nSamples);
    stats["n_features"] = static_cast<double>(nFeatures);
    stats["n_trees"] = static_cast<double>(trees.size());
    
    return stats;
}

std::string RandomForest::getDescription() const {
    return "Random Forest Regression with " + std::to_string(nEstimators) + " trees.";
}

std::vector<std::string> RandomForest::getVariableNames() const {
    return inputVariableNames;
}

std::string RandomForest::getTargetName() const {
    return targetVariableName;
}

void RandomForest::calculateFeatureImportance() {
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

std::unordered_map<std::string, double> RandomForest::getFeatureImportance() const {
    if (!isFitted) {
        throw std::runtime_error("Model has not been fitted yet");
    }
    
    return featureImportanceScores;
} 