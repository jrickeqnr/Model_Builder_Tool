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
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIR}/Eigen
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
    ${FLTK_INCLUDE_DIRS}
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
    src/models/LinearRegression.cpp
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
    ${OPENGL_LIBRARIES}
)

# FLTK might require additional system libraries on some platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 m dl)
endif()

# Copy Python scripts to build directory if needed
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/plotting_scripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)