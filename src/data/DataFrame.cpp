#include "data/DataFrame.h"
#include <stdexcept>

void DataFrame::addColumn(const std::string& name, const std::vector<double>& data) {
    // Check if column already exists
    if (this->data.find(name) != this->data.end()) {
        throw std::invalid_argument("Column '" + name + "' already exists in the DataFrame");
    }

    // Check if data size is consistent with existing columns
    if (!this->data.empty() && data.size() != rows) {
        throw std::invalid_argument("Column '" + name + "' has " + std::to_string(data.size()) + 
                                   " rows, but DataFrame has " + std::to_string(rows) + " rows");
    }

    // If this is the first column, set the row count
    if (this->data.empty()) {
        rows = data.size();
    }

    // Add the column
    this->data[name] = data;
    columnOrder.push_back(name);
}

std::vector<double> DataFrame::getColumn(const std::string& name) const {
    auto it = data.find(name);
    if (it == data.end()) {
        throw std::out_of_range("Column '" + name + "' not found in DataFrame");
    }
    return it->second;
}

Eigen::MatrixXd DataFrame::toMatrix(const std::vector<std::string>& columnNames) const {
    if (columnNames.empty()) {
        throw std::invalid_argument("No columns specified for matrix conversion");
    }

    // Create a matrix with rows x columns
    Eigen::MatrixXd matrix(rows, columnNames.size());
    
    // Fill the matrix column by column
    for (size_t col = 0; col < columnNames.size(); ++col) {
        const auto& name = columnNames[col];
        auto it = data.find(name);
        if (it == data.end()) {
            throw std::out_of_range("Column '" + name + "' not found in DataFrame");
        }

        const auto& columnData = it->second;
        for (size_t row = 0; row < rows; ++row) {
            matrix(row, col) = columnData[row];
        }
    }
    
    return matrix;
}

std::vector<std::string> DataFrame::getColumnNames() const {
    return columnOrder;
}

size_t DataFrame::rowCount() const {
    return rows;
}

size_t DataFrame::columnCount() const {
    return data.size();
}

bool DataFrame::hasColumn(const std::string& name) const {
    return data.find(name) != data.end();
}

DataFrame DataFrame::subset(size_t start, size_t end) const {
    if (start >= rows || end > rows || start >= end) {
        throw std::out_of_range("Invalid subset range");
    }

    DataFrame result;
    for (const auto& name : columnOrder) {
        const auto& fullColumn = data.at(name);
        std::vector<double> subColumn(fullColumn.begin() + start, fullColumn.begin() + end);
        result.addColumn(name, subColumn);
    }
    
    return result;
}