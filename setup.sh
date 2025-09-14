
#!/bin/bash

# Setup script for hsm-ide project
# This script installs all necessary dependencies to build and run the application

set -e

echo "Setting up development environment for hsm-ide..."

# Update package lists
echo "Updating package lists..."
sudo apt-get update

# Install required packages
echo "Installing required packages..."
sudo apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6svg6-dev \
    libboost-all-dev \
    doxygen \
    graphviz \
    plantuml \
    clang-format

# Create build directory if it doesn't exist
echo "Setting up build directory..."
mkdir -p build

# Generate build files
echo "Generating build files..."
cd build
cmake ..

echo "Development environment setup complete!"
echo "You can now build the project with: cd build && make"
echo "Or run the application with: ./build/hsm-ide"

cd ..

echo "Setup finished successfully!"
