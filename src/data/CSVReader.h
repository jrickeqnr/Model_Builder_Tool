#pragma once

#include <string>
#include <vector>
#include <tuple>
#include "data/DataFrame.h"

/**
 * @brief CSV file reader class
 * 
 * This class provides functionality to read CSV files and convert them
 * to a DataFrame object for further analysis.
 */
class CSVReader {
public:
    CSVReader() = default;
    ~CSVReader() = default;

    /**
     * @brief Read a CSV file and convert to DataFrame
     * 
     * @param filePath Path to the CSV file
     * @param separator Column separator character (default: ',')
     * @param hasHeader Whether the file has a header row (default: true)
     * @return DataFrame DataFrame containing the CSV data
     */
    DataFrame readCSV(const std::string& filePath, 
                     char separator = ',', 
                     bool hasHeader = true);

    /**
     * @brief Get column names from the last read CSV file
     * 
     * @return std::vector<std::string> List of column names
     */
    std::vector<std::string> getColumnNames() const;

    /**
     * @brief Check if a string can be converted to a number
     * 
     * @param str String to check
     * @return true If string can be converted to a number
     * @return false If string cannot be converted to a number
     */
    static bool isNumeric(const std::string& str);

private:
    std::vector<std::string> columnNames;
    
    /**
     * @brief Split a string by a delimiter
     * 
     * @param str String to split
     * @param delimiter Delimiter character
     * @return std::vector<std::string> Vector of split strings
     */
    std::vector<std::string> split(const std::string& str, char delimiter);
    
    /**
     * @brief Trim whitespace from start and end of a string
     * 
     * @param str String to trim
     * @return std::string Trimmed string
     */
    std::string trim(const std::string& str);
    
    /**
     * @brief Check if a string appears to be in a date format
     * 
     * @param str String to check
     * @return true If string matches a common date format
     * @return false If string does not match a date format
     */
    bool isDateFormat(const std::string& str);
    
    /**
     * @brief Extract year, month, and day components from a date string
     * 
     * @param dateStr Date string
     * @return std::tuple<int, int, int> Tuple of (year, month, day)
     */
    std::tuple<int, int, int> extractDateComponents(const std::string& dateStr);
    
    /**
     * @brief Process a date column and extract date components
     * 
     * @param df DataFrame to add columns to
     * @param rawData Raw data from CSV
     * @param colIndex Column index
     * @param columnName Column name
     */
    void processDateColumn(DataFrame& df, const std::vector<std::vector<std::string>>& rawData, 
                          size_t colIndex, const std::string& columnName);
};