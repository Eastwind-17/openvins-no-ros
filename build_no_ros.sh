#!/bin/bash

# Build script for OpenVINS without ROS

set -e

echo "Building OpenVINS without ROS..."

# Create build directory in ov_msckf
cd /home/vector/code/open_vins
mkdir -p ov_msckf/build
cd ov_msckf/build

# Configure with CMake (disable ROS)
cmake .. -DENABLE_ROS=OFF

# Build
make -j$(nproc)

echo "Build complete!"
echo "Executable location: /home/vector/code/open_vins/ov_msckf/build/run_euroc"
echo "Run with: ./run_euroc ../../config/euroc_mav/estimator_config.yaml /home/vector/code/datasets/V1_01_easy_extracted/mav0"
