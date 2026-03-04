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

#ifndef OV_MSCKF_NONROS_VISUALIZER_H
#define OV_MSCKF_NONROS_VISUALIZER_H

#include <eigen3/Eigen/src/Core/Matrix.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>
#include <pangolin/pangolin.h>

#include "state/State.h"

namespace ov_msckf {

class VioManager;

/**
 * @brief Non-ROS visualizer using OpenCV and Pangolin
 *
 * - Shows feature tracking image using OpenCV imshow
 * - Shows trajectory using Pangolin 3D viewer
 */
class NonRosVisualizer {
public:
  /**
   * @brief Constructor
   * @param app VioManager instance
   * @param filepath_traj Path to save TUM format trajectory (empty string to disable)
   */
  NonRosVisualizer(std::shared_ptr<VioManager> app, const std::string &filepath_traj = "") 
      : app_(app), follow_camera_(true), camera_view_(false), show_features_(true), save_trajectory_(!filepath_traj.empty()) {
    
    // Open trajectory file if path is provided
    if (save_trajectory_) {
      std::cout << "[NonRosVisualizer] Attempting to open trajectory file: " << filepath_traj << std::endl;
      traj_file_.open(filepath_traj);
      if (!traj_file_.is_open()) {
        std::cerr << "[NonRosVisualizer] ERROR: Failed to open trajectory file: " << filepath_traj << std::endl;
        save_trajectory_ = false;
      } else {
        // Write TUM format header
        traj_file_ << "# timestamp(s) tx ty tz qx qy qz qw" << std::endl;
        traj_file_.flush();  // Force write to disk
        std::cout << "[NonRosVisualizer] SUCCESS: Trajectory will be saved to: " << filepath_traj << std::endl;
      }
    } else {
      std::cout << "[NonRosVisualizer] Trajectory saving disabled (filepath_traj is empty)" << std::endl;
    }
    
    // Initialize Pangolin window
    pangolin::CreateWindowAndBind("OpenVINS Trajectory", 1280, 720);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Define projection and initial ModelView matrix
    // Using Z-up coordinate system (more intuitive)
    s_cam_ = pangolin::OpenGlRenderState(
        pangolin::ProjectionMatrix(1280, 720, 500, 500, 640, 360, 0.1, 1000),
        pangolin::ModelViewLookAt(0, 5, 10, 0, 0, 0, 0, 0, 1));

    // Create control panel
    pangolin::CreatePanel("ui")
        .SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(175));

    // Create Interactive View in the remaining space
    d_cam_ = pangolin::CreateDisplay()
                 .SetBounds(0.0, 1.0, pangolin::Attach::Pix(175), 1.0, -1280.0f / 720.0f)
                 .SetHandler(new pangolin::Handler3D(s_cam_));

    // Create UI controls
    menu_follow_camera_ = new pangolin::Var<bool>("ui.Follow Camera", true, true);
    menu_camera_view_ = new pangolin::Var<bool>("ui.Camera View", false, true);
    menu_show_features_ = new pangolin::Var<bool>("ui.Show Features", true, true);
    menu_show_grid_ = new pangolin::Var<bool>("ui.Show Grid", true, true);
    menu_reset_view_ = new pangolin::Var<bool>("ui.Reset View", false, false);
    menu_top_view_ = new pangolin::Var<bool>("ui.Top View", false, false);

    // Create OpenCV window for feature tracking
    cv::namedWindow("Feature Tracking", cv::WINDOW_AUTOSIZE);
  }

  /**
   * @brief Visualize current state
   */
  void visualize() {
    if (!app_->initialized())
      return;
    auto state = app_->get_state();

    // Update trajectory
    Eigen::Vector3d pos = state->_imu->pos();
    Eigen::Vector4d quat = state->_imu->quat();  // qx, qy, qz, qw
    double timestamp = state->_timestamp;

    // Store trajectory
    {
      std::lock_guard<std::mutex> lock(traj_mutex_);
      trajectory_.push_back(pos);
    }

    // Save to TUM format file if enabled
    if (save_trajectory_ && traj_file_.is_open()) {
      save_pose_to_file(timestamp, pos, quat);
    }

    // Visualize trajectory with Pangolin
    visualize_trajectory();

    // Visualize feature tracking with OpenCV
    visualize_tracking();
  }

  /**
   * @brief Check if visualizer is still running
   */
  bool should_quit() { return pangolin::ShouldQuit(); }

  /**
   * @brief Destructor - close trajectory file
   */
  ~NonRosVisualizer() {
    if (traj_file_.is_open()) {
      traj_file_.close();
      std::cout << "Trajectory file closed." << std::endl;
    }
  }

private:
  /**
   * @brief Save pose to TUM format file
   * @param timestamp Timestamp in seconds
   * @param pos Position (tx, ty, tz)
   * @param quat Quaternion (qx, qy, qz, qw)
   */
  void save_pose_to_file(double timestamp, const Eigen::Vector3d &pos, const Eigen::Vector4d &quat) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    // timestamp with 5 decimal places
    traj_file_ << std::fixed << std::setprecision(5) << timestamp << " ";
    
    // position and quaternion with 6 decimal places
    traj_file_ << std::setprecision(6);
    traj_file_ << pos.x() << " " << pos.y() << " " << pos.z() << " ";
    traj_file_ << quat(0) << " " << quat(1) << " " << quat(2) << " " << quat(3);
    traj_file_ << std::endl;
  }

  /**
   * @brief Draw a ground grid
   */
  void draw_grid() {
    glLineWidth(1);
    glColor4f(0.3f, 0.3f, 0.3f, 0.5f); // Gray with transparency
    
    float grid_size = 50.0f;
    float grid_step = 1.0f;
    
    glBegin(GL_LINES);
    for (float i = -grid_size; i <= grid_size; i += grid_step) {
      // Lines parallel to X axis
      glVertex3f(i, -grid_size, 0);
      glVertex3f(i, grid_size, 0);
      // Lines parallel to Y axis
      glVertex3f(-grid_size, i, 0);
      glVertex3f(grid_size, i, 0);
    }
    glEnd();
  }

  /**
   * @brief Draw coordinate axes
   * @param T Transformation matrix
   * @param scale Scale of the axes
   */
  void draw_axes(const Eigen::Matrix4d &T, float scale) {
    glPushMatrix();
    
    // Eigen uses column-major storage, same as OpenGL
    glMultMatrixd(T.data());
    
    // Draw axes
    glLineWidth(3);
    glBegin(GL_LINES);
    // X axis - red
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0, 0, 0);
    glVertex3f(scale, 0, 0);
    // Y axis - green
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, scale, 0);
    // Z axis - blue
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, scale);
    glEnd();
    
    glPopMatrix();
  }

  /**
   * @brief Draw SLAM features
   * @param state Current state with features
   */
  void draw_features(std::shared_ptr<State> state) {
    // Draw SLAM features
    glPointSize(3);
    glColor3f(1.0, 1.0, 1.0); // White points
    glBegin(GL_POINTS);
    
    for (auto const& feat : state->_features_SLAM) {
      Eigen::Vector3d pos = feat.second->get_xyz(false);
      glVertex3d(pos(0), pos(1), pos(2));
    }
    
    glEnd();
  }

  /**
   * @brief Draw a camera frustum
   * @param T_CtoG Transformation from camera to global frame
   * @param w Width scale
   * @param h Height scale
   * @param z Depth scale
   * @param r Red color component
   * @param g Green color component
   * @param b Blue color component
   */
  void draw_camera(const Eigen::Matrix4d &T_CtoG, float w, float h, float z, float r, float g, float b) {
    glPushMatrix();
    
    // Eigen uses column-major storage, same as OpenGL
    // So we can directly pass the data pointer
    glMultMatrixd(T_CtoG.data());
    
    // Draw camera frustum
    glLineWidth(2);
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    
    // Draw frustum edges from camera center to corners
    glVertex3f(0, 0, 0);
    glVertex3f(w, h, z);
    glVertex3f(0, 0, 0);
    glVertex3f(w, -h, z);
    glVertex3f(0, 0, 0);
    glVertex3f(-w, -h, z);
    glVertex3f(0, 0, 0);
    glVertex3f(-w, h, z);
    
    // Draw frustum rectangle at depth z
    glVertex3f(w, h, z);
    glVertex3f(w, -h, z);
    glVertex3f(w, -h, z);
    glVertex3f(-w, -h, z);
    glVertex3f(-w, -h, z);
    glVertex3f(-w, h, z);
    glVertex3f(-w, h, z);
    glVertex3f(w, h, z);
    
    glEnd();
    glPopMatrix();
  }

  /**
   * @brief Visualize trajectory using Pangolin
   */
  void visualize_trajectory() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update UI state
    follow_camera_ = menu_follow_camera_->Get();
    camera_view_ = menu_camera_view_->Get();
    show_features_ = menu_show_features_->Get();
    bool show_grid = menu_show_grid_->Get();

    // Camera View and Follow Camera are mutually exclusive
    if (camera_view_ && follow_camera_) {
      follow_camera_ = false;
      menu_follow_camera_->operator=(false);
    }
    if (follow_camera_ && camera_view_) {
      camera_view_ = false;
      menu_camera_view_->operator=(false);
    }

    // Handle view mode buttons
    if (pangolin::Pushed(*menu_reset_view_)) {
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(0, 5, 10, 0, 0, 0, 0, 0, 1));
      follow_camera_ = false;
      camera_view_ = false;
      menu_follow_camera_->operator=(false);
      menu_camera_view_->operator=(false);
    }
    if (pangolin::Pushed(*menu_top_view_)) {
      // Get current trajectory center
      Eigen::Vector3d center(0, 0, 0);
      if (!trajectory_.empty()) {
        center = trajectory_.back();
      }
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(
          center(0), center(1), center(2) + 20,  // Camera 20m above current position (Z-up)
          center(0), center(1), center(2),        // Look at current position
          0, 1, 0));                              // Y-axis forward (horizontal)
      follow_camera_ = false;
      camera_view_ = false;
      menu_follow_camera_->operator=(false);
      menu_camera_view_->operator=(false);
    }

    // Camera View mode: first-person view from IMU/camera position
    if (camera_view_ && app_->initialized() && !trajectory_.empty()) {
      auto state = app_->get_state();
      Eigen::Vector3d pos = state->_imu->pos();
      Eigen::Matrix3d R_GtoI = state->_imu->Rot();
      Eigen::Matrix3d R_ItoG = R_GtoI.transpose();
      
      // Look direction: forward in IMU frame is +Z axis
      Eigen::Vector3d look_dir_I(0, 0, 1);  // Forward in IMU frame
      Eigen::Vector3d look_dir_G = R_ItoG * look_dir_I;
      Eigen::Vector3d look_at = pos + look_dir_G;
      
      // Up direction: -Y axis in IMU frame
      Eigen::Vector3d up_dir_I(0, -1, 0);
      Eigen::Vector3d up_dir_G = R_ItoG * up_dir_I;
      
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(
          pos(0), pos(1), pos(2),                 // Camera at IMU position
          look_at(0), look_at(1), look_at(2),     // Look forward
          up_dir_G(0), up_dir_G(1), up_dir_G(2)));// Up direction
    }
    // Follow camera mode: update view to follow current position from behind
    else if (follow_camera_ && app_->initialized() && !trajectory_.empty()) {
      auto state = app_->get_state();
      Eigen::Vector3d pos = state->_imu->pos();
      Eigen::Matrix3d R_GtoI = state->_imu->Rot();
      Eigen::Matrix3d R_ItoG = R_GtoI.transpose();
      
      // Camera looks from behind and above the robot
      Eigen::Vector3d cam_offset_I(-5, 0, -3);  // 5m behind, 3m above in IMU frame
      Eigen::Vector3d cam_pos_G = pos + R_ItoG * cam_offset_I;
      
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(
          cam_pos_G(0), cam_pos_G(1), cam_pos_G(2),  // Camera position
          pos(0), pos(1), pos(2),                     // Look at current position
          0, 0, 1));                                  // Z-axis up
    }

    d_cam_.Activate(s_cam_);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background

    // Draw grid
    if (show_grid) {
      draw_grid();
    }

    // Draw coordinate axes at origin
    draw_axes(Eigen::Matrix4d::Identity(), 1.0);

    // Draw trajectory
    std::lock_guard<std::mutex> lock(traj_mutex_);
    if (trajectory_.size() > 1) {
      glLineWidth(3);
      glColor3f(0.0, 1.0, 1.0); // Cyan trajectory (visible on black background)
      glBegin(GL_LINE_STRIP);
      for (const auto &pos : trajectory_) {
        glVertex3d(pos(0), pos(1), pos(2));
      }
      glEnd();

      // Draw current IMU pose
      if (!trajectory_.empty() && app_->initialized()) {
        auto state = app_->get_state();
        Eigen::Vector3d pos_IinG = state->_imu->pos();
        Eigen::Matrix3d R_GtoI = state->_imu->Rot();
        
        // Convert to IMU pose in Global frame: R_ItoG = R_GtoI^T
        // Apply coordinate transformation from Y-down to Z-up:
        // X stays X, Y_old (down) becomes -Z_new (up), Z_old (forward) becomes Y_new
      

        Eigen::Matrix3d R_ItoG = R_GtoI.transpose();
        
        // Create IMU pose transformation
        Eigen::Matrix4d Twi = Eigen::Matrix4d::Identity();
        Twi.block<3, 3>(0, 0) = R_ItoG;
        Twi.block<3, 1>(0, 3) = pos_IinG;
        
        // Draw coordinate axes at IMU position
        draw_axes(Twi, 0.5);

        // Draw SLAM features if enabled
        if (show_features_) {
          draw_features(state);
        }
      }
    }

    pangolin::FinishFrame();
  }

  /**
   * @brief Visualize feature tracking using OpenCV
   */
  void visualize_tracking() {
    // Get tracking image from VioManager
    cv::Mat img_history = app_->get_historical_viz_image();
    if (img_history.empty())
      return;

    // Display the image
    cv::imshow("Feature Tracking", img_history);
    cv::waitKey(1); // Process events
  }

  std::shared_ptr<VioManager> app_;

  // Pangolin visualization
  pangolin::OpenGlRenderState s_cam_;
  pangolin::View d_cam_;

  // UI controls
  pangolin::Var<bool> *menu_follow_camera_;
  pangolin::Var<bool> *menu_camera_view_;
  pangolin::Var<bool> *menu_show_features_;
  pangolin::Var<bool> *menu_show_grid_;
  pangolin::Var<bool> *menu_reset_view_;
  pangolin::Var<bool> *menu_top_view_;

  // Visualization state
  bool follow_camera_;
  bool camera_view_;
  bool show_features_;

  // Trajectory storage
  std::vector<Eigen::Vector3d> trajectory_;
  std::mutex traj_mutex_;

  // TUM format trajectory file
  bool save_trajectory_;
  std::ofstream traj_file_;
  std::mutex file_mutex_;
};

} // namespace ov_msckf

#endif // OV_MSCKF_NONROS_VISUALIZER_H
