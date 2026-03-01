/**
 * @file OpenLorisLoader.h
 * @brief Loader for OpenLoris-Scene dataset format
 * 
 * This loader handles the OpenLoris-Scene dataset which includes:
 * - T265 IMU data (gyroscope and accelerometer in separate files)
 * - Stereo fisheye cameras
 * - Unix timestamp format (seconds with decimal fraction)
 */

#ifndef OV_MSCKF_OPENLORIS_LOADER_H
#define OV_MSCKF_OPENLORIS_LOADER_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

namespace ov_msckf {

struct ImuData {
    double timestamp;
    Eigen::Vector3d gyro;
    Eigen::Vector3d accel;
    
    bool operator<(const ImuData& other) const {
        return timestamp < other.timestamp;
    }
};

struct CameraData {
    double timestamp;
    std::string filename;
    
    bool operator<(const CameraData& other) const {
        return timestamp < other.timestamp;
    }
};

class OpenLorisLoader {
public:
    OpenLorisLoader(const std::string& dataset_path) : dataset_path_(dataset_path) {}
    
    /**
     * @brief Load IMU data from gyroscope and accelerometer files
     * @param imu_file_prefix Prefix for IMU files (e.g., "t265" or "d400")
     * @return True if loading successful
     */
    bool loadImuData(const std::string& imu_file_prefix = "t265") {
        std::string gyro_file = dataset_path_ + "/" + imu_file_prefix + "_gyroscope.txt";
        std::string accel_file = dataset_path_ + "/" + imu_file_prefix + "_accelerometer.txt";
        
        // Load gyroscope data
        std::map<double, Eigen::Vector3d> gyro_data;
        std::ifstream gyro_fs(gyro_file);
        if (!gyro_fs.is_open()) {
            std::cerr << "Failed to open gyroscope file: " << gyro_file << std::endl;
            return false;
        }
        
        std::string line;
        std::getline(gyro_fs, line); // Skip header
        
        while (std::getline(gyro_fs, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            double timestamp;
            double gx, gy, gz;
            
            if (iss >> timestamp >> gx >> gy >> gz) {
                gyro_data[timestamp] = Eigen::Vector3d(gx, gy, gz);
            }
        }
        gyro_fs.close();
        
        // Load accelerometer data
        std::map<double, Eigen::Vector3d> accel_data;
        std::ifstream accel_fs(accel_file);
        if (!accel_fs.is_open()) {
            std::cerr << "Failed to open accelerometer file: " << accel_file << std::endl;
            return false;
        }
        
        std::getline(accel_fs, line); // Skip header
        
        while (std::getline(accel_fs, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            double timestamp;
            double ax, ay, az;
            
            if (iss >> timestamp >> ax >> ay >> az) {
                accel_data[timestamp] = Eigen::Vector3d(ax, ay, az);
            }
        }
        accel_fs.close();
        
        // Merge gyro and accel data by interpolating to common timestamps
        // We'll use gyro timestamps as the base and interpolate accel
        for (const auto& gyro_entry : gyro_data) {
            double t = gyro_entry.first;
            
            // Find closest accel measurements
            auto it = accel_data.lower_bound(t);
            
            Eigen::Vector3d accel;
            if (it == accel_data.begin()) {
                accel = it->second;
            } else if (it == accel_data.end()) {
                accel = accel_data.rbegin()->second;
            } else {
                // Linear interpolation
                auto it_prev = std::prev(it);
                double t1 = it_prev->first;
                double t2 = it->first;
                double alpha = (t - t1) / (t2 - t1);
                accel = (1.0 - alpha) * it_prev->second + alpha * it->second;
            }
            
            ImuData imu;
            imu.timestamp = t;
            imu.gyro = gyro_entry.second;
            imu.accel = accel;
            imu_data_.push_back(imu);
        }
        
        std::sort(imu_data_.begin(), imu_data_.end());
        
        std::cout << "Loaded " << imu_data_.size() << " IMU measurements" << std::endl;
        if (!imu_data_.empty()) {
            std::cout << "IMU time range: " << imu_data_.front().timestamp 
                      << " to " << imu_data_.back().timestamp << std::endl;
        }
        
        return !imu_data_.empty();
    }
    
    /**
     * @brief Load camera timestamps and image filenames
     * @param camera_name Camera folder name (e.g., "fisheye1", "fisheye2")
     * @param cam_id Camera ID (0 or 1)
     * @return True if loading successful
     */
    bool loadCameraData(const std::string& camera_name, int cam_id) {
        std::string cam_file = dataset_path_ + "/" + camera_name + ".txt";
        std::string cam_folder = dataset_path_ + "/" + camera_name + "/";
        
        std::ifstream cam_fs(cam_file);
        if (!cam_fs.is_open()) {
            std::cerr << "Failed to open camera file: " << cam_file << std::endl;
            return false;
        }
        
        std::vector<CameraData>& cam_data = (cam_id == 0) ? cam0_data_ : cam1_data_;
        cam_data.clear();
        
        std::string line;
        while (std::getline(cam_fs, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            double timestamp;
            std::string relative_path;
            
            if (iss >> timestamp >> relative_path) {
                CameraData cam;
                cam.timestamp = timestamp;
                // Extract filename from relative path (e.g., "fisheye1/123.png" -> "123.png")
                size_t last_slash = relative_path.find_last_of('/');
                std::string filename = (last_slash != std::string::npos) 
                    ? relative_path.substr(last_slash + 1) 
                    : relative_path;
                cam.filename = cam_folder + filename;
                cam_data.push_back(cam);
            }
        }
        cam_fs.close();
        
        std::sort(cam_data.begin(), cam_data.end());
        
        std::cout << "Loaded " << cam_data.size() << " images for camera " << cam_id 
                  << " (" << camera_name << ")" << std::endl;
        if (!cam_data.empty()) {
            std::cout << "Camera " << cam_id << " time range: " 
                      << cam_data.front().timestamp << " to " 
                      << cam_data.back().timestamp << std::endl;
        }
        
        return !cam_data.empty();
    }
    
    /**
     * @brief Load image from file
     * @param filename Full path to image file
     * @return Loaded image
     */
    cv::Mat loadImage(const std::string& filename) {
        cv::Mat img = cv::imread(filename, cv::IMREAD_GRAYSCALE);
        if (img.empty()) {
            std::cerr << "Warning: Failed to load image: " << filename << std::endl;
        }
        return img;
    }
    
    // Getters
    const std::vector<ImuData>& getImuData() const { return imu_data_; }
    const std::vector<CameraData>& getCam0Data() const { return cam0_data_; }
    const std::vector<CameraData>& getCam1Data() const { return cam1_data_; }
    
    size_t getImuCount() const { return imu_data_.size(); }
    size_t getCam0Count() const { return cam0_data_.size(); }
    size_t getCam1Count() const { return cam1_data_.size(); }
    
private:
    std::string dataset_path_;
    std::vector<ImuData> imu_data_;
    std::vector<CameraData> cam0_data_;
    std::vector<CameraData> cam1_data_;
};

} // namespace ov_msckf

#endif // OV_MSCKF_OPENLORIS_LOADER_H
