#!/bin/bash

# OpenLoris runner script for market1-1 dataset
# Usage: ./run_openloris.sh

# Set paths
CONFIG_DIR="$(pwd)/config/openloris"
DATASET_PATH="/media/vector/Software/ubuntu_datasets/market1-1_3-package/market1-1"
EXECUTABLE="$(pwd)/ov_msckf/build/run_openloris"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please run ./build_no_ros.sh first"
    exit 1
fi

# Check if dataset exists
if [ ! -d "$DATASET_PATH" ]; then
    echo "Error: Dataset not found at $DATASET_PATH"
    echo "Please update DATASET_PATH in this script"
    exit 1
fi

# Check if config exists
if [ ! -f "$CONFIG_DIR/estimator_config_t265.yaml" ]; then
    echo "Error: Config not found at $CONFIG_DIR/estimator_config_t265.yaml"
    exit 1
fi

echo "========================================"
echo "Running OpenVINS on OpenLoris dataset"
echo "========================================"
echo "Dataset: $DATASET_PATH"
echo "Config:  $CONFIG_DIR/estimator_config_t265.yaml"
echo ""

# Run the executable
$EXECUTABLE "$CONFIG_DIR/estimator_config_t265.yaml" "$DATASET_PATH"
