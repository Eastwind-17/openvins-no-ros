#!/bin/bash

# Run OpenVINS on EuRoC dataset without ROS

set -e

CONFIG="/home/vector/code/open_vins/config/euroc_mav/estimator_config.yaml"
DATASET="/home/vector/code/datasets/V1_01_easy_extracted/mav0"
EXECUTABLE="/home/vector/code/open_vins/ov_msckf/build/run_euroc"

echo "Running OpenVINS on EuRoC V1_01_easy dataset..."
echo "Config: $CONFIG"
echo "Dataset: $DATASET"
echo ""

$EXECUTABLE $CONFIG $DATASET
