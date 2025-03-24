#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <Eigen/Dense>

/**
 * @brief DataFrame class for storing and manipulating tabular data
 * 
 * This class provides a simple data frame implementation for storing
 * and accessing columns of data with named headers.
 */
class DataFrame {
public:
    DataFrame() = default;
    ~DataFrame() = default;

    /**
     * @brief Add a column to the data frame
     * 
     * @param name Column name
     * @param data Column data
     */
    void addColumn(const std::string& name, const std::vector<double>& data);

    /**
     * @brief Get column data by name
     * 
     * @param name Column name
     * @return std::vector<double> Column data
     */
    std::vector<double> getColumn(const std::string& name) const;

    /**
     * @brief Convert multiple columns to an Eigen matrix
     * 
     * @param columnNames List of column names to include
     * @return Eigen::MatrixXd Matrix representation of selected columns
     */
    Eigen::MatrixXd toMatrix(const std::vector<std::string>& columnNames) const;

    /**
     * @brief Get all column names
     * 
     * @return std::vector<std::string> List of column names
     */
    std::vector<std::string> getColumnNames() const;

    /**
     * @brief Get number of rows in the data frame
     * 
     * @return size_t Number of rows
     */
    size_t getNumRows() const { return rows; }

    /**
     * @brief Get number of columns in the data frame
     * 
     * @return size_t Number of columns
     */
    size_t columnCount() const;

    /**
     * @brief Check if a column exists
     * 
     * @param name Column name
     * @return true If column exists
     * @return false If column does not exist
     */
    bool hasColumn(const std::string& name) const;

    /**
     * @brief Get a subset of rows
     * 
     * @param start Starting row index
     * @param end Ending row index (exclusive)
     * @return DataFrame Subset of the data frame
     */
    DataFrame subset(size_t start, size_t end) const;

    /**
     * @brief Get value at specific row and column index
     * 
     * @param row Row index
     * @param col Column index
     * @return double Value at the specified position
     */
    double getValue(size_t row, size_t col) const {
        if (col >= columnOrder.size()) {
            throw std::out_of_range("Column index out of range");
        }
        const std::string& colName = columnOrder[col];
        if (row >= data.at(colName).size()) {
            throw std::out_of_range("Row index out of range");
        }
        return data.at(colName)[row];
    }

    /**
     * @brief Get column index by name
     * 
     * @param name Column name
     * @return int Column index, or -1 if not found
     */
    int getColumnIndex(const std::string& name) const {
        for (size_t i = 0; i < columnOrder.size(); ++i) {
            if (columnOrder[i] == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

private:
    std::unordered_map<std::string, std::vector<double>> data;
    std::vector<std::string> columnOrder;
    size_t rows = 0;
};