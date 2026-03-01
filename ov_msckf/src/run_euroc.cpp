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
#include "utils/EurocLoader.h"
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
    std::cerr << "Example: " << argv[0] << " /path/to/estimator_config.yaml /path/to/mav0" << std::endl;
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

  // Load EuRoC dataset
  EurocLoader loader(dataset_path);

  // Load IMU data
  std::vector<ImuMeasurement> imu_data;
  if (!loader.load_imu_data(imu_data)) {
    PRINT_ERROR(RED "Failed to load IMU data\n" RESET);
    return EXIT_FAILURE;
  }

  // Load camera data for cam0 and cam1
  std::vector<CameraMeasurement> cam0_data, cam1_data;
  if (!loader.load_camera_data(0, cam0_data)) {
    PRINT_ERROR(RED "Failed to load cam0 data\n" RESET);
    return EXIT_FAILURE;
  }

  if (params.state_options.num_cameras > 1) {
    if (!loader.load_camera_data(1, cam1_data)) {
      PRINT_ERROR(RED "Failed to load cam1 data\n" RESET);
      return EXIT_FAILURE;
    }
  }

  PRINT_INFO("Loaded %zu IMU measurements\n", imu_data.size());
  PRINT_INFO("Loaded %zu cam0 measurements\n", cam0_data.size());
  if (params.state_options.num_cameras > 1) {
    PRINT_INFO("Loaded %zu cam1 measurements\n", cam1_data.size());
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

    // Load cam0 image
    cv::Mat img0;
    if (!loader.load_camera_image(0, cam0_data[cam0_idx].filename, img0)) {
      PRINT_ERROR(RED "Failed to load cam0 image: %s\n" RESET, cam0_data[cam0_idx].filename.c_str());
      cam0_idx++;
      continue;
    }
    cam_msg.sensor_ids.push_back(0);
    cam_msg.images.push_back(img0);
    cam_msg.masks.push_back(cv::Mat::zeros(img0.rows, img0.cols, CV_8UC1)); // Empty mask (all pixels valid)

    // Load cam1 image if stereo
    if (params.state_options.num_cameras > 1) {
      // Find matching cam1 timestamp (within 1ms tolerance)
      while (cam1_idx < cam1_data.size() && cam1_data[cam1_idx].timestamp < cam0_time - 0.001) {
        cam1_idx++;
      }

      if (cam1_idx < cam1_data.size() && std::abs(cam1_data[cam1_idx].timestamp - cam0_time) < 0.001) {
        cv::Mat img1;
        if (loader.load_camera_image(1, cam1_data[cam1_idx].filename, img1)) {
          cam_msg.sensor_ids.push_back(1);
          cam_msg.images.push_back(img1);
          cam_msg.masks.push_back(cv::Mat::zeros(img1.rows, img1.cols, CV_8UC1)); // Empty mask (all pixels valid)
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
