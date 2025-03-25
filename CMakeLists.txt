cmake_minimum_required(VERSION 3.15)
project(LinearRegressionTool VERSION 1.0.0 LANGUAGES CXX)

# Force 64-bit build
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_VS_PLATFORM_TOOLSET "v143")
set(CMAKE_VS_PLATFORM "x64")

# Force C++17 and Release build
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")

# Prevent CMake from falling back to debug libraries
set_property(DIRECTORY PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Set custom Python paths
set(PYTHON_ROOT "C:/Users/JRICK/dev/Model_Builder_Tool/python/cpython-3.13.2")
set(PYTHON_INCLUDE_DIRS
    "${PYTHON_ROOT}/Include"
    "${PYTHON_ROOT}/PC"  # Add PC directory for pyconfig.h
    "${PYTHON_ROOT}/PCbuild/amd64"  # Add PCbuild/amd64 directory for pyconfig.h
)
set(PYTHON_LIBRARY_RELEASE "${PYTHON_ROOT}/PCbuild/amd64/python313.lib")
set(PYTHON_LIBRARY_DEBUG ${PYTHON_LIBRARY_RELEASE})  # Force using Release library even in Debug
set(PYTHON_LIBRARY ${PYTHON_LIBRARY_RELEASE})
set(PYTHON_EXECUTABLE "${PYTHON_ROOT}/PCbuild/amd64/python.exe")

# Disable Python debug libraries
set(Python3_USE_DEBUG_LIBRARY FALSE)
set(Python3_FIND_UNVERSIONED_NAMES NEVER)

# Set hardcoded paths for numpy and matplotlib
set(NUMPY_INCLUDE_DIR "C:/Users/JRICK/dev/Model_Builder_Tool/python/cpython-3.13.2/Lib/site-packages/numpy/core/include")
set(MATPLOTLIB_INCLUDE_DIR "C:/Users/JRICK/dev/Model_Builder_Tool/python/cpython-3.13.2/Lib/site-packages/matplotlib/include")
set(MATPLOTLIB_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/plotting")

# Get Python environment information
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print('Python executable:', sys.executable); import site; print('Site packages:', site.getsitepackages()); print('User site packages:', site.getusersitepackages())"
    OUTPUT_VARIABLE PYTHON_ENV_INFO
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Python Environment Info:\n${PYTHON_ENV_INFO}")

# Get NumPy include directory directly
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
    OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "NumPy include directory: ${NUMPY_INCLUDE_DIR}")

# Set Python3 variables manually since find_package is having trouble
set(Python3_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
set(Python3_LIBRARIES ${PYTHON_LIBRARY})
set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
set(Python3_NumPy_INCLUDE_DIRS ${NUMPY_INCLUDE_DIR})

# Add include directories first
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MATPLOTLIB_CPP_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${FLTK_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${NUMPY_INCLUDE_DIR}
    ${MATPLOTLIB_INCLUDE_DIR}
)

# Add each site-packages directory and its numpy/matplotlib includes
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import site; print(';'.join(site.getsitepackages() + [site.getusersitepackages()]))"
    OUTPUT_VARIABLE ALL_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Split the site-packages directories
string(REPLACE ";" " " SITE_PACKAGES_LIST "${ALL_SITE_PACKAGES}")

foreach(site_pkg ${SITE_PACKAGES_LIST})
    message(STATUS "Checking site-packages directory: ${site_pkg}")
    
    # Check if numpy is in this directory
    if(EXISTS "${site_pkg}/numpy/core/include")
        message(STATUS "Found numpy in: ${site_pkg}")
        include_directories(${site_pkg}/numpy/core/include)
    endif()
    
    # Check if matplotlib is in this directory
    if(EXISTS "${site_pkg}/matplotlib/include")
        message(STATUS "Found matplotlib in: ${site_pkg}")
        include_directories(${site_pkg}/matplotlib/include)
    endif()
endforeach()

# Check if matplotlib is installed
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import matplotlib; print(matplotlib.__file__)"
    OUTPUT_VARIABLE MATPLOTLIB_PATH
    ERROR_VARIABLE MATPLOTLIB_ERROR
    RESULT_VARIABLE MATPLOTLIB_RESULT
)

message(STATUS "Matplotlib installation path: ${MATPLOTLIB_PATH}")

# Check if numpy is installed
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import numpy; print(numpy.__file__)"
    OUTPUT_VARIABLE NUMPY_PATH
    ERROR_VARIABLE NUMPY_ERROR
    RESULT_VARIABLE NUMPY_RESULT
)

message(STATUS "NumPy installation path: ${NUMPY_PATH}")

set(FLTK_DIR "C:/Users/JRICK/Dev/fltk-1.4.2-source/fltk-1.4.2/build")
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../eigen-3.4.0")

# Find packages
find_package(FLTK REQUIRED NO_DEFAULT_PATH PATHS ${FLTK_DIR})
find_package(Eigen3 REQUIRED)

# Add compile definitions
add_compile_definitions(
    CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    _USE_MATH_DEFINES
    NOMINMAX
    Py_ENABLE_SHARED=1
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

# Copy Python DLLs and dependencies to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PYTHON_ROOT}/PCbuild/amd64/python313.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/python3.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/vcruntime140.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/vcruntime140_1.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/libcrypto-3.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/libssl-3.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/sqlite3.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/libffi-8.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/zlib1.dll"
        "${PYTHON_ROOT}/PCbuild/amd64/_ssl.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/_socket.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/_hashlib.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/select.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/unicodedata.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/_bz2.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/_lzma.pyd"
        "${PYTHON_ROOT}/PCbuild/amd64/pyexpat.pyd"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Copy Python packages to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PYTHON_ROOT}/Lib/site-packages/numpy"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/numpy"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PYTHON_ROOT}/Lib/site-packages/matplotlib"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/matplotlib"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PYTHON_ROOT}/Lib"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Lib"
)

# Link libraries (remove NumPy include dir from here)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FLTK_LIBRARIES}
    Eigen3::Eigen
    ${PYTHON_LIBRARY}
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
message(STATUS "Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Platform: ${CMAKE_GENERATOR_PLATFORM}")
message(STATUS "Toolset: ${CMAKE_VS_PLATFORM_TOOLSET}")