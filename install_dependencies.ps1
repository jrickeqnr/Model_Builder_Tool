# PowerShell script to install numpy and matplotlib for CPython 3.13.2

# Set Python paths
$PYTHON_ROOT = "C:/Users/JRICK/dev/Model_Builder_Tool/python/cpython-3.13.2"
$PYTHON_EXECUTABLE = "$PYTHON_ROOT/PCbuild/amd64/python.exe"

# First, ensure pip is installed
Write-Host "Installing pip..."
& $PYTHON_EXECUTABLE -m ensurepip --default-pip

# Upgrade pip to latest version
Write-Host "Upgrading pip..."
& $PYTHON_EXECUTABLE -m pip install --upgrade pip

# Install build dependencies
Write-Host "Installing build dependencies..."
& $PYTHON_EXECUTABLE -m pip install wheel setuptools

# Install numpy and matplotlib with specific versions for Python 3.13.2
Write-Host "Installing numpy..."
& $PYTHON_EXECUTABLE -m pip install "numpy>=1.26.0" --no-cache-dir

Write-Host "Installing matplotlib..."
& $PYTHON_EXECUTABLE -m pip install "matplotlib>=3.8.0" --no-cache-dir

# Verify installations
Write-Host "Verifying installations..."
& $PYTHON_EXECUTABLE -c "import numpy; print('NumPy version:', numpy.__version__)"
& $PYTHON_EXECUTABLE -c "import matplotlib; print('Matplotlib version:', matplotlib.__version__)"

Write-Host "Installation complete!"
Write-Host "You can now run CMake to build the project." 