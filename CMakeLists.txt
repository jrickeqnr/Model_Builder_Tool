cmake_minimum_required(VERSION 3.15)
project(LinearRegressionTool VERSION 1.0.0 LANGUAGES CXX)

# Force C++17 and Release build
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CONFIGURATION_TYPES Release)  # Only allow Release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")  # Optimize for Release

# Find required packages first
find_package(Python3 3.13 REQUIRED COMPONENTS Interpreter Development NumPy)

# Set Python paths explicitly for Windows
set(PYTHON_ROOT_DIR "C:/Users/JRICK/Appdata/Local/Programs/Python/Python313")
set(PYTHON_DLL_DIR "${PYTHON_ROOT_DIR}/DLLs")
set(PYTHON_LIBS_DIR "${PYTHON_ROOT_DIR}/Lib")
set(PYTHON_SITE_PACKAGES "${PYTHON_LIBS_DIR}/site-packages")

# Debug prints for Python paths
message(STATUS "Python paths:")
message(STATUS "PYTHON_ROOT_DIR: ${PYTHON_ROOT_DIR}")
message(STATUS "PYTHON_DLL_DIR: ${PYTHON_DLL_DIR}")
message(STATUS "PYTHON_SITE_PACKAGES: ${PYTHON_SITE_PACKAGES}")

# Verify critical files exist
execute_process(
    COMMAND ${CMAKE_COMMAND} -E echo "Checking Python DLL files..."
)

# Check core Python DLLs
foreach(dll_file 
    "${PYTHON_ROOT_DIR}/python313.dll"
    "${PYTHON_ROOT_DIR}/python3.dll"
    "${PYTHON_ROOT_DIR}/vcruntime140.dll"
    "${PYTHON_ROOT_DIR}/vcruntime140_1.dll"
)
    if(EXISTS "${dll_file}")
        message(STATUS "Found DLL: ${dll_file}")
    else()
        message(WARNING "Missing DLL: ${dll_file}")
    endif()
endforeach()

# Check Python extension DLLs
foreach(pyd_file 
    "${PYTHON_DLL_DIR}/_ssl.pyd"
    "${PYTHON_DLL_DIR}/_socket.pyd"
    "${PYTHON_DLL_DIR}/_hashlib.pyd"
    "${PYTHON_DLL_DIR}/select.pyd"
    "${PYTHON_DLL_DIR}/unicodedata.pyd"
    "${PYTHON_DLL_DIR}/_bz2.pyd"
    "${PYTHON_DLL_DIR}/_lzma.pyd"
    "${PYTHON_DLL_DIR}/pyexpat.pyd"
    "${PYTHON_DLL_DIR}/libcrypto-3.dll"
    "${PYTHON_DLL_DIR}/libssl-3.dll"
    "${PYTHON_DLL_DIR}/sqlite3.dll"
    "${PYTHON_DLL_DIR}/libffi-8.dll"
    "${PYTHON_DLL_DIR}/zlib1.dll"
)
    if(EXISTS "${pyd_file}")
        message(STATUS "Found PYD/DLL: ${pyd_file}")
    else()
        message(WARNING "Missing PYD/DLL: ${pyd_file}")
    endif()
endforeach()

# Check Python packages
if(EXISTS "${PYTHON_SITE_PACKAGES}/numpy")
    message(STATUS "Found numpy at: ${PYTHON_SITE_PACKAGES}/numpy")
else()
    message(WARNING "Missing numpy at: ${PYTHON_SITE_PACKAGES}/numpy")
endif()

if(EXISTS "${PYTHON_SITE_PACKAGES}/matplotlib")
    message(STATUS "Found matplotlib at: ${PYTHON_SITE_PACKAGES}/matplotlib")
else()
    message(WARNING "Missing matplotlib at: ${PYTHON_SITE_PACKAGES}/matplotlib")
endif()

# Set paths for external libraries
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../eigen-3.4.0")
set(FLTK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../fltk-1.4.2-source/fltk-1.4.2/build")

# Find other required packages
find_package(FLTK REQUIRED NO_DEFAULT_PATH PATHS ${FLTK_DIR})
find_package(Eigen3 REQUIRED)

# Get NumPy include directory
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
    OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get matplotlib include directory
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import matplotlib; import os; print(os.path.join(os.path.dirname(matplotlib.__file__), 'include'))"
    OUTPUT_VARIABLE MATPLOTLIB_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set matplotlib cpp directory
set(MATPLOTLIB_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/plotting")

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MATPLOTLIB_CPP_DIR}
    ${Python3_INCLUDE_DIRS}
    ${NUMPY_INCLUDE_DIR}
    ${MATPLOTLIB_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${FLTK_INCLUDE_DIRS}
)

# Add compile definitions for matplotlibcpp
add_compile_definitions(
    WITHOUT_NUMPY  # Disable numpy integration since we're using it directly
    _USE_MATH_DEFINES  # Required for Windows
    NOMINMAX  # Prevent Windows.h from defining min/max macros
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

# Copy Python DLLs to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying Python DLLs from ${PYTHON_ROOT_DIR} to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PYTHON_ROOT_DIR}/python313.dll"
        "${PYTHON_ROOT_DIR}/python3.dll"
        "${PYTHON_ROOT_DIR}/vcruntime140.dll"
        "${PYTHON_ROOT_DIR}/vcruntime140_1.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying Python extensions from ${PYTHON_DLL_DIR} to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PYTHON_DLL_DIR}/_ssl.pyd"
        "${PYTHON_DLL_DIR}/_socket.pyd"
        "${PYTHON_DLL_DIR}/_hashlib.pyd"
        "${PYTHON_DLL_DIR}/select.pyd"
        "${PYTHON_DLL_DIR}/unicodedata.pyd"
        "${PYTHON_DLL_DIR}/_bz2.pyd"
        "${PYTHON_DLL_DIR}/_lzma.pyd"
        "${PYTHON_DLL_DIR}/pyexpat.pyd"
        "${PYTHON_DLL_DIR}/libcrypto-3.dll"
        "${PYTHON_DLL_DIR}/libssl-3.dll"
        "${PYTHON_DLL_DIR}/sqlite3.dll"
        "${PYTHON_DLL_DIR}/libffi-8.dll"
        "${PYTHON_DLL_DIR}/zlib1.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Copy Python packages to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/numpy"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/matplotlib"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PYTHON_SITE_PACKAGES}/numpy"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/numpy"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PYTHON_SITE_PACKAGES}/matplotlib"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/matplotlib"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Python3_LIBRARIES}
    ${FLTK_LIBRARIES}
    Eigen3::Eigen
)

# Windows-specific filesystem library
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shlwapi)
endif()