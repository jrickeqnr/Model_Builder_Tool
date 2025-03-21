#pragma once

#include <FL/Fl_Table.H>
#include <vector>
#include <string>
#include <unordered_map>

/**
 * @brief Custom table to display parameters and statistics
 */
class DataTable : public Fl_Table {
public:
    DataTable(int x, int y, int w, int h, const char* label = 0);
    ~DataTable();
    
    /**
     * @brief Set the data for the table
     * 
     * @param data Map of names to values
     */
    void setData(const std::unordered_map<std::string, double>& data);
    
protected:
    /**
     * @brief Draw a cell
     */
    void draw_cell(TableContext context, int row, int col, int x, int y, int w, int h) override;
    
private:
    std::vector<std::string> names;
    std::vector<double> values;
}; 