#!/bin/bash

# Run OpenVINS on custom EuRoC dataset
# Usage: bash run_euroc_custom.sh

set -e

DATASET_PATH="/media/vector/Software/ubuntu_datasets/vicon_room1/V1_01_easy/V1_01_easy/mav0"
CONFIG_PATH="/home/vector/code/open_vins/config/euroc_mav/estimator_config.yaml"
BUILD_DIR="/home/vector/code/open_vins/ov_msckf/build"

echo "Running OpenVINS on EuRoC dataset..."
echo "Dataset: $DATASET_PATH"
echo "Config: $CONFIG_PATH"
echo ""

cd $BUILD_DIR
./run_euroc $CONFIG_PATH $DATASET_PATH
