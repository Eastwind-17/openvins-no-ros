/*
 * OpenVINS: An Open Platform for Visual-Inertial Research
 * Copyright (C) 2018-2023 Patrick Geneva
 * Copyright (C) 2018-2023 Guoquan Huang
 * Copyright (C) 2018-2023 OpenVINS Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "core/VioManager.h"
#include "utils/OpenLorisLoader.h"
#include "utils/NonRosVisualizer.h"

#include "utils/colors.h"
#include "utils/print.h"
#include "utils/sensor_data.h"

using namespace ov_msckf;

std::shared_ptr<VioManager> sys;
std::shared_ptr<NonRosVisualizer> viz;

// Signal handler for ctrl-c
void signal_callback_handler(int signum) {
  std::cout << "\nCaught signal " << signum << ", exiting..." << std::endl;
  std::exit(signum);
}

int main(int argc, char **argv) {

  // Check arguments
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <config_path> <dataset_path>" << std::endl;
    std::cerr << "Example: " << argv[0] << " /path/to/estimator_config.yaml /path/to/market1-1" << std::endl;
    return EXIT_FAILURE;
  }

  std::string config_path = argv[1];
  std::string dataset_path = argv[2];

  // Load configuration
  auto parser = std::make_shared<ov_core::YamlParser>(config_path);

  // Set verbosity
  std::string verbosity = "INFO";
  parser->parse_config("verbosity", verbosity);
  ov_core::Printer::setPrintLevel(verbosity);

  // Create VIO system
  VioManagerOptions params;
  params.print_and_load(parser);
  params.num_opencv_threads = 4; // Use multiple threads for better performance
  params.use_multi_threading_pubs = false;
  params.use_multi_threading_subs = false;

  // Ensure all parameters were parsed successfully
  if (!parser->successful()) {
    PRINT_ERROR(RED "Unable to parse all parameters, please fix\n" RESET);
    return EXIT_FAILURE;
  }

  // Create VioManager
  sys = std::make_shared<VioManager>(params);

  // Create visualizer
  viz = std::make_shared<NonRosVisualizer>(sys);

  // Set signal handler
  signal(SIGINT, signal_callback_handler);

  // Load OpenLoris dataset
  OpenLorisLoader loader(dataset_path);

  // Load IMU data from T265 sensor
  PRINT_INFO("Loading IMU data from T265 sensor...\n");
  if (!loader.loadImuData("t265")) {
    PRINT_ERROR(RED "Failed to load IMU data\n" RESET);
    return EXIT_FAILURE;
  }

  // Load camera data for fisheye1 and fisheye2
  PRINT_INFO("Loading camera data...\n");
  if (!loader.loadCameraData("fisheye1", 0)) {
    PRINT_ERROR(RED "Failed to load fisheye1 data\n" RESET);
    return EXIT_FAILURE;
  }

  if (params.state_options.num_cameras > 1) {
    if (!loader.loadCameraData("fisheye2", 1)) {
      PRINT_ERROR(RED "Failed to load fisheye2 data\n" RESET);
      return EXIT_FAILURE;
    }
  }

  const auto& imu_data = loader.getImuData();
  const auto& cam0_data = loader.getCam0Data();
  const auto& cam1_data = loader.getCam1Data();

  PRINT_INFO("Loaded %zu IMU measurements\n", imu_data.size());
  PRINT_INFO("Loaded %zu fisheye1 measurements\n", cam0_data.size());
  if (params.state_options.num_cameras > 1) {
    PRINT_INFO("Loaded %zu fisheye2 measurements\n", cam1_data.size());
  }

  // Load masks if configured
  cv::Mat mask0, mask1;
  bool use_mask = false;
  std::string mask0_path, mask1_path;
  
  parser->parse_config("use_mask", use_mask, false);
  
  if (use_mask) {
    parser->parse_config("mask0", mask0_path);
    parser->parse_config("mask1", mask1_path);
    
    // Try relative path first (relative to config file)
    std::string config_dir = config_path.substr(0, config_path.find_last_of("/\\") + 1);
    
    // Load mask0
    std::string full_mask0_path = config_dir + mask0_path;
    mask0 = cv::imread(full_mask0_path, cv::IMREAD_GRAYSCALE);
    if (mask0.empty()) {
      // Try absolute path
      mask0 = cv::imread(mask0_path, cv::IMREAD_GRAYSCALE);
    }
    
    if (mask0.empty()) {
      PRINT_ERROR(RED "Failed to load mask0 from: %s\n" RESET, full_mask0_path.c_str());
      use_mask = false;
    } else {
      // Binarize mask: threshold at 127 (pixels > 127 become 255, others become 0)
      cv::threshold(mask0, mask0, 127, 255, cv::THRESH_BINARY);
      PRINT_INFO("Loaded and binarized mask0: %dx%d from %s\n", mask0.cols, mask0.rows, full_mask0_path.c_str());
    }
    
    // Load mask1 if stereo
    if (params.state_options.num_cameras > 1) {
      std::string full_mask1_path = config_dir + mask1_path;
      mask1 = cv::imread(full_mask1_path, cv::IMREAD_GRAYSCALE);
      if (mask1.empty()) {
        mask1 = cv::imread(mask1_path, cv::IMREAD_GRAYSCALE);
      }
      
      if (mask1.empty()) {
        PRINT_ERROR(RED "Failed to load mask1 from: %s\n" RESET, full_mask1_path.c_str());
        use_mask = false;
      } else {
        // Binarize mask: threshold at 127 (pixels > 127 become 255, others become 0)
        cv::threshold(mask1, mask1, 127, 255, cv::THRESH_BINARY);
        PRINT_INFO("Loaded and binarized mask1: %dx%d from %s\n", mask1.cols, mask1.rows, full_mask1_path.c_str());
      }
    }
  }
  
  if (!use_mask) {
    PRINT_INFO(YELLOW "Not using masks (all pixels valid)\n" RESET);
  }

  // Process data
  size_t imu_idx = 0;
  size_t cam0_idx = 0;
  size_t cam1_idx = 0;

  PRINT_INFO(GREEN "Starting VIO processing...\n" RESET);

  while (cam0_idx < cam0_data.size() && !viz->should_quit()) {

    // Get current camera timestamp
    double cam0_time = cam0_data[cam0_idx].timestamp;

    // Feed all IMU messages up to this camera time
    while (imu_idx < imu_data.size() && imu_data[imu_idx].timestamp <= cam0_time) {
      ov_core::ImuData imu;
      imu.timestamp = imu_data[imu_idx].timestamp;
      imu.wm = imu_data[imu_idx].gyro;
      imu.am = imu_data[imu_idx].accel;
      sys->feed_measurement_imu(imu);
      imu_idx++;
    }

    // Prepare camera data
    ov_core::CameraData cam_msg;
    cam_msg.timestamp = cam0_time;

    // Load cam0 (fisheye1) image
    cv::Mat img0 = loader.loadImage(cam0_data[cam0_idx].filename);
    if (img0.empty()) {
      PRINT_ERROR(RED "Failed to load fisheye1 image: %s\n" RESET, cam0_data[cam0_idx].filename.c_str());
      cam0_idx++;
      continue;
    }
    cam_msg.sensor_ids.push_back(0);
    cam_msg.images.push_back(img0);
    
    // Use mask if available, otherwise all pixels valid
    if (use_mask && !mask0.empty()) {
      cam_msg.masks.push_back(mask0);
    } else {
      cam_msg.masks.push_back(cv::Mat::zeros(img0.rows, img0.cols, CV_8UC1));
    }

    // Load cam1 (fisheye2) image if stereo
    if (params.state_options.num_cameras > 1) {
      // Find matching cam1 timestamp (within 5ms tolerance for OpenLoris)
      while (cam1_idx < cam1_data.size() && cam1_data[cam1_idx].timestamp < cam0_time - 0.005) {
        cam1_idx++;
      }

      if (cam1_idx < cam1_data.size() && std::abs(cam1_data[cam1_idx].timestamp - cam0_time) < 0.005) {
        cv::Mat img1 = loader.loadImage(cam1_data[cam1_idx].filename);
        if (!img1.empty()) {
          cam_msg.sensor_ids.push_back(1);
          cam_msg.images.push_back(img1);
          
          // Use mask if available, otherwise all pixels valid
          if (use_mask && !mask1.empty()) {
            cam_msg.masks.push_back(mask1);
          } else {
            cam_msg.masks.push_back(cv::Mat::zeros(img1.rows, img1.cols, CV_8UC1));
          }
        }
        cam1_idx++;
      }
    }

    // Feed camera measurement
    sys->feed_measurement_camera(cam_msg);

    // Visualize if initialized
    if (sys->initialized()) {
      // Print first initialization message
      static bool first_init = true;
      if (first_init) {
        PRINT_INFO(GREEN "=== SYSTEM INITIALIZED ===" RESET "\n");
        first_init = false;
      }
      
      viz->visualize();

      // Print status every 100 frames
      if (cam0_idx % 100 == 0) {
        auto state = sys->get_state();
        Eigen::Vector3d pos = state->_imu->pos();
        PRINT_INFO("Frame %zu/%zu | Time: %.3f | Position: [%.3f, %.3f, %.3f]\n", cam0_idx, cam0_data.size(), cam0_time, pos(0), pos(1),
                   pos(2));
      }
    }

    cam0_idx++;
  }

  PRINT_INFO(GREEN "Processing complete!\n" RESET);
  PRINT_INFO("Processed %zu frames\n", cam0_idx);

  // Keep windows open
  std::cout << "Press Ctrl+C to exit..." << std::endl;
  while (!viz->should_quit()) {
    viz->visualize();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }

  return EXIT_SUCCESS;
}
