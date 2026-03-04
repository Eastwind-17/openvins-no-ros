#!/bin/bash

# OpenLoris runner script for rectified pinhole stereo camera dataset
# Usage: ./run_openloris_pinhole.sh [dataset_path]
# Default dataset: cafe1-1 with rectified fisheye cameras

# Set paths
CONFIG_DIR="$(pwd)/config/openloris"
DATASET_PATH="${1:-/media/vector/Software/ubuntu_datasets/cafe1-1_2-package/cafe1-1}"
EXECUTABLE="$(pwd)/ov_msckf/build/run_openloris"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please build the project first (e.g., ./build_no_ros.sh)"
    exit 1
fi

# Check if dataset path provided
if [ "$#" -eq 0 ]; then
    echo "Using default dataset: $DATASET_PATH"
    echo "(You can specify a different path: $0 <dataset_path>)"
    echo ""
fi

# Check if dataset exists
if [ ! -d "$DATASET_PATH" ]; then
    echo "Error: Dataset not found at $DATASET_PATH"
    echo "Please provide a valid dataset path"
    exit 1
fi

# Check if config exists
if [ ! -f "$CONFIG_DIR/estimator_config_pinhole.yaml" ]; then
    echo "Error: Config not found at $CONFIG_DIR/estimator_config_pinhole.yaml"
    exit 1
fi

echo "========================================"
echo "Running OpenVINS on OpenLoris Pinhole Dataset"
echo "========================================"
echo "Dataset: $DATASET_PATH"
echo "Config:  $CONFIG_DIR/estimator_config_pinhole.yaml"
echo ""
echo "Expected dataset structure:"
echo "  $DATASET_PATH/"
echo "    ├── t265_gyroscope.txt"
echo "    ├── t265_accelerometer.txt"
echo "    ├── fisheye1_rect.txt"
echo "    ├── fisheye1_rect/"
echo "    │   └── <timestamp>.png"
echo "    ├── fisheye2_rect.txt"
echo "    └── fisheye2_rect/"
echo "        └── <timestamp>.png"
echo ""

# Run the executable
$EXECUTABLE "$CONFIG_DIR/estimator_config_pinhole.yaml" "$DATASET_PATH"
