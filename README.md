# Linear Regression Tool with FLTK

This repository contains a C++ application for performing linear regression analysis on CSV data with a graphical user interface built using FLTK (Fast Light Toolkit).

## About FLTK

FLTK is a cross-platform, lightweight C++ GUI toolkit that's free and open-source. It's designed to be small, fast, and easy to use. FLTK provides a simple API for creating windows, buttons, menus, and other common UI elements.

## Building the Application

### Prerequisites

Before building the project, make sure you have the following installed:

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libfltk1.3-dev libeigen3-dev python3-dev python3-matplotlib
```

#### macOS (with Homebrew)
```bash
brew install cmake fltk eigen python
pip3 install matplotlib
```

#### Windows
1. Install CMake from https://cmake.org/download/
2. Install FLTK using one of these methods:
   - Download from https://www.fltk.org/software.php and build it
   - Use vcpkg: `vcpkg install fltk:x64-windows`
3. Install Eigen: `vcpkg install eigen3:x64-windows`
4. Install Python and matplotlib: `pip install matplotlib`

### Building

1. Clone the repository
2. Create a build directory and navigate to it:
```bash
mkdir build
cd build
```

3. Run CMake:
```bash
cmake .. 
```

If CMake can't find FLTK, you may need to specify its location:
```bash
cmake .. -DFLTK_DIR=/path/to/fltk
```

4. Build the project:
```bash
cmake --build . --config Release
```

5. Run the application:
```bash
./LinearRegressionTool
```

## Project Structure

This project follows a modular design with the following components:

- **GUI Components**: Implemented using FLTK widgets
  - `MainWindow`: Main application window
  - `FileSelector`: Screen for selecting CSV files
  - `ModelSelector`: Screen for selecting regression models
  - `VariableSelector`: Screen for selecting input and target variables
  - `ResultsView`: Screen for displaying model results and visualizations

- **Data Handling**: Pure C++ classes for data management
  - `DataFrame`: Data structure for storing and manipulating tabular data
  - `CSVReader`: Utility for reading CSV files

- **Statistical Models**: Implementations of regression models
  - `Model`: Abstract base class for all regression models
  - `LinearRegression`: Implementation of ordinary least squares regression

## Differences from Qt Implementation

If you're familiar with Qt, here are some key differences when working with FLTK:

1. **Signal-Slot Mechanism**: FLTK uses traditional callbacks instead of Qt's signal-slot system. We use std::function to create a similar mechanism.

2. **Layout Management**: FLTK requires manual positioning of widgets with explicit coordinates, unlike Qt's layout system.

3. **Widget Ownership**: In FLTK, parent widgets own and delete their children automatically.

4. **Drawing**: FLTK uses a simple drawing API with the `fl_*` functions.

5. **Main Loop**: FLTK uses `Fl::run()` instead of `QApplication::exec()`.

## Customizing the Application

### Adding New Models

To add a new regression model:

1. Create a new class that inherits from the `Model` base class
2. Implement all required virtual methods
3. Add the model to the `createModel` method in `MainWindow.cpp`
4. Add the model to the list in `ModelSelector.cpp`

### Styling the UI

FLTK has a simpler theming system than Qt. To customize the appearance:

1. Use `Fl::scheme("gtk+")` to set a predefined theme
2. Use `widget->color(fl_rgb_color(r, g, b))` to set widget colors
3. Use `widget->labelsize(size)` and `widget->labelfont(font)` to customize text

## Troubleshooting

### CMake Can't Find FLTK

If CMake can't find FLTK, specify its location manually:
```bash
cmake .. -DFLTK_DIR=/path/to/fltk/lib/cmake/fltk
```

### Linker Errors

On Windows, you might need to link additional libraries:
```cmake
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 comctl32 gdi32 oleaut32 uuid)
endif()
```

### X11 Errors on Linux

On Linux, you might need to install X11 development libraries:
```bash
sudo apt-get install libx11-dev libxft-dev libxext-dev
```