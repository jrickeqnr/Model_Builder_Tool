cmake_minimum_required(VERSION 3.15)
project(LinearRegressionTool VERSION 1.0.0 LANGUAGES CXX)

# Force 64-bit build
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_VS_PLATFORM_TOOLSET "v143")
set(CMAKE_VS_PLATFORM "x64")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug symbols and debug mode
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Set custom Python paths
set(PYTHON_ROOT "C:/Users/JRICK/dev/cpython-3.13.2")
set(PYTHON_INCLUDE_DIR "C:/Users/JRICK/dev/cpython-3.13.2/Include")
set(PYTHON_LIBRARY "C:/Users/JRICK/dev/cpython-3.13.2/PCbuild/amd64/python313_d.lib")
set(PYTHON_EXECUTABLE "C:/Users/JRICK/dev/cpython-3.13.2/PCbuild/amd64/python_d.exe")

# Find NumPy with specific Python version
find_package(Python3 3.13 COMPONENTS NumPy REQUIRED)

set(FLTK_DIR "C:/Users/JRICK/Dev/fltk-1.4.2-source/fltk-1.4.2/build")
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../eigen-3.4.0")
set(MATPLOTLIB_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/plotting")

# Find packages
find_package(FLTK REQUIRED NO_DEFAULT_PATH PATHS ${FLTK_DIR})
find_package(Eigen3 REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MATPLOTLIB_CPP_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${FLTK_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIR}
    ${Python3_NumPy_INCLUDE_DIRS}
)

# Add compile definitions
add_compile_definitions(
    CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    _USE_MATH_DEFINES
    NOMINMAX
    Py_ENABLE_SHARED=1
    _DEBUG  # Enable debug mode
    WIN64   # Explicitly define 64-bit Windows
)

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

# Copy Python DLL to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PYTHON_ROOT}/PCbuild/amd64/python313_d.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FLTK_LIBRARIES}
    Eigen3::Eigen
    ${PYTHON_LIBRARY}
    Python3::NumPy
)

# FLTK might require additional system libraries on some platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 m dl)
endif()

# Windows-specific filesystem library (needed for std::filesystem)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shlwapi)
endif()

# Print debug information
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Platform: ${CMAKE_GENERATOR_PLATFORM}")
message(STATUS "Toolset: ${CMAKE_VS_PLATFORM_TOOLSET}")