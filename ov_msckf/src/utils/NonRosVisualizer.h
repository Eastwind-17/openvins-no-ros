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

#include <memory>
#include <mutex>
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
   */
  NonRosVisualizer(std::shared_ptr<VioManager> app) : app_(app), follow_camera_(true), show_features_(true) {
    // Initialize Pangolin window
    pangolin::CreateWindowAndBind("OpenVINS Trajectory", 1280, 720);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Define projection and initial ModelView matrix
    s_cam_ = pangolin::OpenGlRenderState(
        pangolin::ProjectionMatrix(1280, 720, 500, 500, 640, 360, 0.1, 1000),
        pangolin::ModelViewLookAt(0, -5, -10, 0, 0, 0, 0, -1, 0));

    // Create control panel
    pangolin::CreatePanel("ui")
        .SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(175));

    // Create Interactive View in the remaining space
    d_cam_ = pangolin::CreateDisplay()
                 .SetBounds(0.0, 1.0, pangolin::Attach::Pix(175), 1.0, -1280.0f / 720.0f)
                 .SetHandler(new pangolin::Handler3D(s_cam_));

    // Create UI controls
    menu_follow_camera_ = new pangolin::Var<bool>("ui.Follow Camera", true, true);
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

    // Store trajectory
    {
      std::lock_guard<std::mutex> lock(traj_mutex_);
      trajectory_.push_back(pos);
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

private:
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
    show_features_ = menu_show_features_->Get();
    bool show_grid = menu_show_grid_->Get();

    // Handle view mode buttons
    if (pangolin::Pushed(*menu_reset_view_)) {
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(0, -5, -10, 0, 0, 0, 0, -1, 0));
      follow_camera_ = false;
      menu_follow_camera_->operator=(false);
    }
    if (pangolin::Pushed(*menu_top_view_)) {
      // Get current trajectory center
      Eigen::Vector3d center(0, 0, 0);
      if (!trajectory_.empty()) {
        center = trajectory_.back();
      }
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(
          center(0), center(1), center(2) + 20,  // Camera 20m above current position
          center(0), center(1), center(2),        // Look at current position
          0, 1, 0));                              // Y-axis up
      follow_camera_ = false;
      menu_follow_camera_->operator=(false);
    }

    // Follow camera mode: update view to follow current position
    if (follow_camera_ && app_->initialized() && !trajectory_.empty()) {
      auto state = app_->get_state();
      Eigen::Vector3d pos = state->_imu->pos();
      Eigen::Vector4d quat_JPL = state->_imu->quat();
      
      // Convert JPL to Hamilton quaternion
      Eigen::Quaterniond q_Hamilton(quat_JPL(3), quat_JPL(0), quat_JPL(1), quat_JPL(2));
      Eigen::Matrix3d R_GtoI = q_Hamilton.toRotationMatrix();
      Eigen::Matrix3d R_ItoG = R_GtoI.transpose();
      
      // Camera looks from behind and above the robot
      Eigen::Vector3d cam_offset_I(-5, 0, -3);  // 5m behind, 3m above in IMU frame
      Eigen::Vector3d cam_pos_G = pos + R_ItoG * cam_offset_I;
      
      s_cam_.SetModelViewMatrix(pangolin::ModelViewLookAt(
          cam_pos_G(0), cam_pos_G(1), cam_pos_G(2),  // Camera position
          pos(0), pos(1), pos(2),                     // Look at current position
          0, 0, -1));                                 // Z-axis up
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

      // Draw current camera pose(s)
      if (!trajectory_.empty() && app_->initialized()) {
        auto state = app_->get_state();
        Eigen::Vector3d pos_IinG = state->_imu->pos();
        
        // Get rotation matrix from state
        // According to State.h:146, _imu stores (q_GtoI, p_IinG)
        // So Rot() returns R_GtoI (rotation from Global to IMU frame)
        // For visualization, we need T_ItoG (IMU pose in Global frame)
        Eigen::Matrix3d R_GtoI = state->_imu->Rot();
        Eigen::Matrix3d R_ItoG = R_GtoI.transpose();
        
        // Create transformation matrix T_ItoG (IMU to Global)
        Eigen::Matrix4d T_ItoG = Eigen::Matrix4d::Identity();
        T_ItoG.block<3, 3>(0, 0) = R_ItoG;
        T_ItoG.block<3, 1>(0, 3) = pos_IinG;
        
        // Draw IMU coordinate frame
        draw_axes(T_ItoG, 0.5);
        
        // Store camera positions in global frame for baseline visualization
        std::vector<Eigen::Vector3d> cam_positions;
        std::vector<size_t> cam_ids;
        
        // Draw camera frustum for each camera
        for (const auto& calib_pair : state->_calib_IMUtoCAM) {
          size_t cam_id = calib_pair.first;
          auto calib_cam = calib_pair.second;
          
          // Get extrinsics from state
          // According to State.h:158, _calib_IMUtoCAM stores (R_ItoC, p_IinC)
          Eigen::Matrix3d R_ItoC = calib_cam->Rot();
          Eigen::Vector3d p_IinC = calib_cam->pos();
          
          // Compute R_GtoC (Global to Camera)
          // R_GtoC = R_ItoC * R_GtoI
          Eigen::Matrix3d R_GtoC = R_ItoC * R_GtoI;
          
          // For OpenGL, we need Camera pose in Global frame (T_CtoG)
          // R_CtoG = R_GtoC^T
          Eigen::Matrix3d R_CtoG = R_GtoC.transpose();
          
          // Camera position in Global frame:
          // p_CinG = p_IinG - R_GtoC^T * p_IinC
          Eigen::Vector3d p_CinG = pos_IinG - R_CtoG * p_IinC;
          
          // Create T_CtoG transformation
          Eigen::Matrix4d T_CtoG = Eigen::Matrix4d::Identity();
          T_CtoG.block<3, 3>(0, 0) = R_CtoG;
          T_CtoG.block<3, 1>(0, 3) = p_CinG;
          
          // Extract camera position in global frame
          Eigen::Vector3d cam_pos_G = T_CtoG.block<3, 1>(0, 3);
          cam_positions.push_back(cam_pos_G);
          cam_ids.push_back(cam_id);
          
          // Draw camera frustum with different colors for different cameras
          if (cam_id == 0) {
            draw_camera(T_CtoG, 0.3, 0.2, 0.5, 1.0, 0.0, 0.0); // Red for cam0
            // Draw small sphere at camera center
            glPushMatrix();
            glTranslated(cam_pos_G(0), cam_pos_G(1), cam_pos_G(2));
            glColor3f(1.0, 0.0, 0.0);
            pangolin::glDrawCircle(0, 0, 0.05); // 5cm radius sphere
            glPopMatrix();
          } else if (cam_id == 1) {
            draw_camera(T_CtoG, 0.3, 0.2, 0.5, 0.0, 1.0, 0.0); // Green for cam1
            // Draw small sphere at camera center
            glPushMatrix();
            glTranslated(cam_pos_G(0), cam_pos_G(1), cam_pos_G(2));
            glColor3f(0.0, 1.0, 0.0);
            pangolin::glDrawCircle(0, 0, 0.05); // 5cm radius sphere
            glPopMatrix();
          } else {
            draw_camera(T_CtoG, 0.3, 0.2, 0.5, 0.0, 0.0, 1.0); // Blue for others
          }
        }
        
        // Draw baseline between stereo cameras (if we have 2+ cameras)
        if (cam_positions.size() >= 2) {
          glLineWidth(3);
          glColor3f(1.0, 1.0, 0.0); // Yellow baseline
          glBegin(GL_LINES);
          for (size_t i = 0; i < cam_positions.size() - 1; i++) {
            glVertex3d(cam_positions[i](0), cam_positions[i](1), cam_positions[i](2));
            glVertex3d(cam_positions[i+1](0), cam_positions[i+1](1), cam_positions[i+1](2));
          }
          glEnd();
        }

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
  pangolin::Var<bool> *menu_show_features_;
  pangolin::Var<bool> *menu_show_grid_;
  pangolin::Var<bool> *menu_reset_view_;
  pangolin::Var<bool> *menu_top_view_;

  // Visualization state
  bool follow_camera_;
  bool show_features_;

  // Trajectory storage
  std::vector<Eigen::Vector3d> trajectory_;
  std::mutex traj_mutex_;
};

} // namespace ov_msckf

#endif // OV_MSCKF_NONROS_VISUALIZER_H
