#include "data/CSVReader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <regex>
#include <map>

DataFrame CSVReader::readCSV(const std::string& filePath, char separator, bool hasHeader) {
    // Open the file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    DataFrame df;
    std::string line;
    std::vector<std::vector<std::string>> rawData;
    
    // Read the header if present
    if (hasHeader && std::getline(file, line)) {
        columnNames = split(line, separator);
        for (auto& name : columnNames) {
            name = trim(name);
        }
    } else {
        // If no header, rewind to the beginning
        file.clear();
        file.seekg(0, std::ios::beg);
    }
    
    // Read all data rows
    while (std::getline(file, line)) {
        auto values = split(line, separator);
        
        // Trim each value
        for (auto& value : values) {
            value = trim(value);
        }
        
        // Skip empty lines
        if (values.empty() || (values.size() == 1 && values[0].empty())) {
            continue;
        }
        
        // Add to raw data
        rawData.push_back(values);
    }
    
    // If no data was read
    if (rawData.empty()) {
        throw std::runtime_error("No data found in the file");
    }
    
    // If no header was provided, generate column names
    if (columnNames.empty()) {
        for (size_t i = 0; i < rawData[0].size(); ++i) {
            columnNames.push_back("Column" + std::to_string(i + 1));
        }
    }
    
    // Ensure all rows have the same number of columns
    size_t numColumns = columnNames.size();
    for (const auto& row : rawData) {
        if (row.size() != numColumns) {
            throw std::runtime_error("Inconsistent number of columns in the CSV file");
        }
    }
    
    // Process each column
    for (size_t col = 0; col < numColumns; ++col) {
        std::string columnName = columnNames[col];
        bool isDateColumn = false;
        
        // Check if this might be a date column (check first few rows)
        size_t checkRows = std::min(rawData.size(), size_t(5));
        for (size_t i = 0; i < checkRows; ++i) {
            if (isDateFormat(rawData[i][col])) {
                isDateColumn = true;
                break;
            }
        }
        
        if (isDateColumn) {
            // Handle date column by extracting year, month, day as separate features
            processDateColumn(df, rawData, col, columnName);
        } else {
            // Check if column is numeric
            bool allNumeric = true;
            for (const auto& row : rawData) {
                if (!isNumeric(row[col])) {
                    allNumeric = false;
                    break;
                }
            }
            
            // Process numeric column
            if (allNumeric) {
                std::vector<double> columnData;
                columnData.reserve(rawData.size());
                
                for (const auto& row : rawData) {
                    columnData.push_back(std::stod(row[col]));
                }
                df.addColumn(columnName, columnData);
            } else {
                // Skip non-numeric, non-date columns with a warning
                std::cerr << "Warning: Column '" << columnName 
                          << "' contains non-numeric, non-date values and will be skipped." << std::endl;
            }
        }
    }
    
    return df;
}

void CSVReader::processDateColumn(DataFrame& df, const std::vector<std::vector<std::string>>& rawData, 
                                  size_t colIndex, const std::string& columnName) {
    size_t rowCount = rawData.size();
    
    // Create vectors for year, month, day
    std::vector<double> yearValues(rowCount);
    std::vector<double> monthValues(rowCount);
    std::vector<double> dayValues(rowCount);
    
    // Extract date components
    for (size_t row = 0; row < rowCount; ++row) {
        std::string dateStr = rawData[row][colIndex];
        
        // Extract year, month, day from the date string
        std::tuple<int, int, int> dateComponents = extractDateComponents(dateStr);
        
        yearValues[row] = static_cast<double>(std::get<0>(dateComponents));
        monthValues[row] = static_cast<double>(std::get<1>(dateComponents));
        dayValues[row] = static_cast<double>(std::get<2>(dateComponents));
    }
    
    // Add extracted features to the DataFrame
    df.addColumn(columnName + "_year", yearValues);
    df.addColumn(columnName + "_month", monthValues);
    df.addColumn(columnName + "_day", dayValues);
    
    std::cout << "Info: Date column '" << columnName << "' processed and split into "
              << columnName + "_year, " << columnName + "_month, " << columnName + "_day" << std::endl;
}

bool CSVReader::isDateFormat(const std::string& str) {
    // Check common date formats
    static const std::regex dateRegex1(R"(\d{4}-\d{1,2}-\d{1,2})");  // YYYY-MM-DD
    static const std::regex dateRegex2(R"(\d{1,2}/\d{1,2}/\d{4})");  // MM/DD/YYYY or DD/MM/YYYY
    static const std::regex dateRegex3(R"(\d{1,2}-\d{1,2}-\d{4})");  // MM-DD-YYYY or DD-MM-YYYY
    
    return std::regex_match(str, dateRegex1) || 
           std::regex_match(str, dateRegex2) || 
           std::regex_match(str, dateRegex3);
}

std::tuple<int, int, int> CSVReader::extractDateComponents(const std::string& dateStr) {
    int year = 0, month = 0, day = 0;
    
    // Try YYYY-MM-DD format
    static const std::regex isoDateRegex(R"((\d{4})-(\d{1,2})-(\d{1,2}))");
    std::smatch matches;
    
    if (std::regex_match(dateStr, matches, isoDateRegex)) {
        year = std::stoi(matches[1]);
        month = std::stoi(matches[2]);
        day = std::stoi(matches[3]);
    } else {
        // Try other formats if needed
        // For MM/DD/YYYY or DD/MM/YYYY (assuming MM/DD/YYYY)
        static const std::regex slashDateRegex(R"((\d{1,2})/(\d{1,2})/(\d{4}))");
        
        if (std::regex_match(dateStr, matches, slashDateRegex)) {
            month = std::stoi(matches[1]);
            day = std::stoi(matches[2]);
            year = std::stoi(matches[3]);
        } else {
            // MM-DD-YYYY or DD-MM-YYYY (assuming MM-DD-YYYY if not YYYY-MM-DD)
            static const std::regex dashDateRegex(R"((\d{1,2})-(\d{1,2})-(\d{4}))");
            
            if (std::regex_match(dateStr, matches, dashDateRegex)) {
                month = std::stoi(matches[1]);
                day = std::stoi(matches[2]);
                year = std::stoi(matches[3]);
            }
        }
    }
    
    return std::make_tuple(year, month, day);
}

std::vector<std::string> CSVReader::getColumnNames() const {
    return columnNames;
}

bool CSVReader::isNumeric(const std::string& str) {
    // Empty string is not numeric
    if (str.empty()) {
        return false;
    }
    
    bool hasDecimal = false;
    bool hasDigit = false;
    
    size_t i = 0;
    // Check for sign
    if (str[0] == '+' || str[0] == '-') {
        i = 1;
    }
    
    // Check rest of the string
    for (; i < str.length(); ++i) {
        if (std::isdigit(str[i])) {
            hasDigit = true;
        } else if (str[i] == '.' && !hasDecimal) {
            hasDecimal = true;
        } else {
            return false;
        }
    }
    
    return hasDigit;
}

std::vector<std::string> CSVReader::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string CSVReader::trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    
    return (start < end) ? std::string(start, end) : std::string();
}