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

#ifndef OV_MSCKF_EUROC_LOADER_H
#define OV_MSCKF_EUROC_LOADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

namespace ov_msckf {

/**
 * @brief Struct for IMU measurement
 */
struct ImuMeasurement {
  double timestamp;      // seconds
  Eigen::Vector3d gyro;  // rad/s
  Eigen::Vector3d accel; // m/s^2
};

/**
 * @brief Struct for camera measurement
 */
struct CameraMeasurement {
  double timestamp; // seconds
  std::string filename;
};

/**
 * @brief EuRoC dataset loader
 *
 * Loads IMU and camera data from EuRoC MAV dataset format
 */
class EurocLoader {
public:
  /**
   * @brief Constructor
   * @param dataset_path Path to the mav0 directory
   */
  EurocLoader(const std::string &dataset_path) : dataset_path_(dataset_path) {}

  /**
   * @brief Load IMU data from CSV file
   * @param imu_data Output vector of IMU measurements
   * @return True if successful
   */
  bool load_imu_data(std::vector<ImuMeasurement> &imu_data) {
    std::string imu_file = dataset_path_ + "/imu0/data.csv";
    std::ifstream file(imu_file);
    if (!file.is_open()) {
      std::cerr << "Failed to open IMU file: " << imu_file << std::endl;
      return false;
    }

    std::string line;
    // Skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string token;
      ImuMeasurement imu;

      // Parse timestamp (nanoseconds)
      std::getline(ss, token, ',');
      imu.timestamp = std::stod(token) * 1e-9; // Convert to seconds

      // Parse gyroscope (rad/s)
      std::getline(ss, token, ',');
      imu.gyro(0) = std::stod(token);
      std::getline(ss, token, ',');
      imu.gyro(1) = std::stod(token);
      std::getline(ss, token, ',');
      imu.gyro(2) = std::stod(token);

      // Parse accelerometer (m/s^2)
      std::getline(ss, token, ',');
      imu.accel(0) = std::stod(token);
      std::getline(ss, token, ',');
      imu.accel(1) = std::stod(token);
      std::getline(ss, token, ',');
      imu.accel(2) = std::stod(token);

      imu_data.push_back(imu);
    }

    std::cout << "Loaded " << imu_data.size() << " IMU measurements" << std::endl;
    return true;
  }

  /**
   * @brief Load camera data from CSV file
   * @param cam_id Camera ID (0 or 1)
   * @param cam_data Output vector of camera measurements
   * @return True if successful
   */
  bool load_camera_data(int cam_id, std::vector<CameraMeasurement> &cam_data) {
    std::string cam_file = dataset_path_ + "/cam" + std::to_string(cam_id) + "/data.csv";
    std::ifstream file(cam_file);
    if (!file.is_open()) {
      std::cerr << "Failed to open camera file: " << cam_file << std::endl;
      return false;
    }

    std::string line;
    // Skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string token;
      CameraMeasurement cam;

      // Parse timestamp (nanoseconds)
      std::getline(ss, token, ',');
      cam.timestamp = std::stod(token) * 1e-9; // Convert to seconds

      // Parse filename
      std::getline(ss, token, ',');
      // Trim whitespace and newlines
      token.erase(token.find_last_not_of(" \n\r\t") + 1);
      cam.filename = token;

      cam_data.push_back(cam);
    }

    std::cout << "Loaded " << cam_data.size() << " camera measurements for cam" << cam_id << std::endl;
    return true;
  }

  /**
   * @brief Load camera image
   * @param cam_id Camera ID (0 or 1)
   * @param filename Filename from the CSV
   * @param image Output image
   * @return True if successful
   */
  bool load_camera_image(int cam_id, const std::string &filename, cv::Mat &image) {
    std::string img_path = dataset_path_ + "/cam" + std::to_string(cam_id) + "/data/" + filename;
    image = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
    if (image.empty()) {
      std::cerr << "Failed to load image: " << img_path << std::endl;
      return false;
    }
    return true;
  }

private:
  std::string dataset_path_;
};

} // namespace ov_msckf

#endif // OV_MSCKF_EUROC_LOADER_H
