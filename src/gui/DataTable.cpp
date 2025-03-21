#include "gui/DataTable.h"
#include <FL/fl_draw.H>
#include <cstdio>

DataTable::DataTable(int x, int y, int w, int h, const char* label)
    : Fl_Table(x, y, w, h, label) 
{
    // Setup the table
    rows(0);
    row_header(0);
    row_height_all(25);
    row_resize(0);
    
    cols(2);
    col_header(1);
    col_width(0, 150);    // Parameter name column
    col_width(1, 100);    // Value column
    col_resize(1);
    end();
}

DataTable::~DataTable() {
    // No explicit cleanup needed
}

void DataTable::setData(const std::unordered_map<std::string, double>& data) {
    // Clear existing data
    names.clear();
    values.clear();
    
    // Copy data into vectors
    for (const auto& pair : data) {
        names.push_back(pair.first);
        values.push_back(pair.second);
    }
    
    // Update number of rows and redraw
    rows((int)names.size());
    redraw();
}

void DataTable::draw_cell(TableContext context, int row, int col, int x, int y, int w, int h) {
    switch (context) {
        case CONTEXT_STARTPAGE:
            fl_font(FL_HELVETICA, 12);
            return;
            
        case CONTEXT_COL_HEADER:
            fl_push_clip(x, y, w, h);
            fl_draw_box(FL_THIN_UP_BOX, x, y, w, h, col_header_color());
            fl_color(FL_BLACK);
            fl_draw(col == 0 ? "Parameter" : "Value", x + 2, y, w, h, FL_ALIGN_LEFT);
            fl_pop_clip();
            return;
            
        case CONTEXT_CELL:
            fl_push_clip(x, y, w, h);
            // Draw cell bg
            fl_color(FL_WHITE);
            fl_rectf(x, y, w, h);
            // Draw cell data
            fl_color(FL_BLACK);
            if (col == 0) {
                fl_draw(names[row].c_str(), x + 2, y, w, h, FL_ALIGN_LEFT);
            } else {
                char s[30];
                snprintf(s, 30, "%.6f", values[row]);
                fl_draw(s, x + 2, y, w, h, FL_ALIGN_LEFT);
            }
            // Draw cell borders
            fl_color(FL_LIGHT2);
            fl_rect(x, y, w, h);
            fl_pop_clip();
            return;
            
        default:
            return;
    }
} 