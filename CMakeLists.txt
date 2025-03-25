cmake_minimum_required(VERSION 3.15)
project(LinearRegressionTool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLTK_DIR "C:/Users/JRICK/Dev/fltk-1.4.2-source/fltk-1.4.2/build")
set(EIGEN3_INCLUDE_DIR "C:/Users/JRICK/Dev/eigen-3.4.0")

# Find packages
find_package(FLTK REQUIRED NO_DEFAULT_PATH PATHS ${FLTK_DIR})
find_package(Eigen3 REQUIRED)
find_package(Python3 COMPONENTS Development NumPy REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIR}/Eigen
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
    ${FLTK_INCLUDE_DIRS}
)

# Add compile definitions
add_compile_definitions(
    CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

# Add matplotlib-cpp as a header-only library
add_library(matplotlib_cpp INTERFACE)
target_include_directories(matplotlib_cpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/FileSelector.cpp
    src/gui/ModelSelector.cpp
    src/gui/VariableSelector.cpp
    src/gui/ResultsView.cpp
    src/gui/ExportDialog.cpp
    src/gui/DataTable.cpp
    src/gui/HyperparameterSelector.cpp
    src/models/LinearRegression.cpp
    src/models/ElasticNet.cpp
    src/models/XGBoost.cpp
    src/models/RandomForest.cpp
    src/models/GradientBoosting.cpp
    src/models/NeuralNetwork.cpp
    src/data/CSVReader.cpp
    src/data/DataFrame.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FLTK_LIBRARIES}
    Eigen3::Eigen
    ${Python3_LIBRARIES}
    matplotlib_cpp
)

# FLTK might require additional system libraries on some platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 m dl)
endif()

# Windows-specific filesystem library (needed for std::filesystem)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shlwapi)
endif()

# Create the plotting_scripts directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/plotting_scripts)

# Copy the Python script to the build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotting_scripts/plot_regression.py
    ${CMAKE_CURRENT_BINARY_DIR}/plotting_scripts/plot_regression.py
    COPYONLY
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/plotting_scripts/ 
        DESTINATION bin/plotting_scripts
        FILES_MATCHING PATTERN "*.py")