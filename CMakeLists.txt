cmake_minimum_required(VERSION 3.15)
project(LinearRegressionTool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLTK_DIR "C:/Users/JRICK/Dev/fltk-1.4.2-source/fltk-1.4.2/build")
set(EIGEN3_INCLUDE_DIR "C:/Users/JRICK/Dev/eigen-3.4.0")
set(IMGUI_DIR "C:/Users/JRICK/dev/imgui")
set(IMPLOT_DIR "C:/Users/JRICK/dev/implot")

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
    ${IMGUI_DIR}
    ${IMPLOT_DIR}
)

# Add compile definitions
add_compile_definitions(
    CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    NOMINMAX
    GLEW_STATIC
)

# Add ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/imgui_impl_fltk.cpp
)

# ImPlot source files
set(IMPLOT_SOURCES
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
)

# Source files
set(SOURCES
    # ImGui and ImPlot sources must come first
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    
    # Application source files
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/FileSelector.cpp
    src/gui/ModelSelector.cpp
    src/gui/VariableSelector.cpp
    src/gui/ResultsView.cpp
    src/gui/ExportDialog.cpp
    src/gui/DataTable.cpp
    src/gui/HyperparameterSelector.cpp
    src/gui/PlotGLWindow.cpp
    src/models/LinearRegression.cpp
    src/models/ElasticNet.cpp
    src/models/XGBoost.cpp
    src/models/RandomForest.cpp
    src/models/GradientBoosting.cpp
    src/models/NeuralNetwork.cpp
    src/data/CSVReader.cpp
    src/data/DataFrame.cpp
    src/utils/PlottingUtility.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Set ImGui compile definitions for the ImGui source files
target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:IMGUI_IMPL_API=__declspec\(dllexport\)>
  $<$<COMPILE_LANGUAGE:CXX>:IMPLOT_IMPL_API=__declspec\(dllexport\)>
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FLTK_LIBRARIES}
    Eigen3::Eigen
    ${Python3_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# FLTK might require additional system libraries on some platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 m dl)
endif()

# Windows-specific filesystem library (needed for std::filesystem)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shlwapi)
endif()

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)