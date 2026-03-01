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
  NonRosVisualizer(std::shared_ptr<VioManager> app) : app_(app) {
    // Initialize Pangolin window
    pangolin::CreateWindowAndBind("OpenVINS Trajectory", 1024, 768);
    glEnable(GL_DEPTH_TEST);

    // Define projection and initial ModelView matrix
    s_cam_ = pangolin::OpenGlRenderState(pangolin::ProjectionMatrix(1024, 768, 500, 500, 512, 384, 0.1, 1000),
                                         pangolin::ModelViewLookAt(0, -5, -10, 0, 0, 0, 0, -1, 0));

    // Create Interactive View
    d_cam_ = pangolin::CreateDisplay()
                 .SetBounds(0.0, 1.0, 0.0, 1.0, -1024.0f / 768.0f)
                 .SetHandler(new pangolin::Handler3D(s_cam_));

    // Create OpenCV window for feature tracking
    cv::namedWindow("Feature Tracking", cv::WINDOW_AUTOSIZE);
  }

  /**
   * @brief Visualize current state
   */
  void visualize() {
    if (!app_->initialized())
      return;

    // Get current state
    auto state = app_->get_state();
    double timestamp = state->_timestamp;

    // Get pose (q_GtoI, p_IinG)
    Eigen::Vector3d pos = state->_imu->pos();
    Eigen::Vector4d quat = state->_imu->quat(); // JPL quaternion

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
    
    // Convert Eigen matrix to OpenGL format (column-major)
    Eigen::Matrix4d T = T_CtoG;
    GLdouble glT[16];
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        glT[j * 4 + i] = T(i, j);
      }
    }
    glMultMatrixd(glT);
    
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

    d_cam_.Activate(s_cam_);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background

    // Draw coordinate axes at origin
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0); // X axis - red
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    glColor3f(0.0, 1.0, 0.0); // Y axis - green
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);
    glColor3f(0.0, 0.0, 1.0); // Z axis - blue
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);
    glEnd();

    // Draw trajectory
    std::lock_guard<std::mutex> lock(traj_mutex_);
    if (trajectory_.size() > 1) {
      glLineWidth(2);
      glColor3f(0.0, 1.0, 1.0); // Cyan trajectory (visible on black background)
      glBegin(GL_LINE_STRIP);
      for (const auto &pos : trajectory_) {
        glVertex3d(pos(0), pos(1), pos(2));
      }
      glEnd();

      // Draw current camera pose
      if (!trajectory_.empty() && app_->initialized()) {
        auto state = app_->get_state();
        Eigen::Vector3d pos = state->_imu->pos();
        Eigen::Vector4d quat_JPL = state->_imu->quat(); // JPL quaternion (x,y,z,w)
        
        // Convert JPL quaternion to rotation matrix
        // JPL: q = [qx, qy, qz, qw], Hamilton: q = [qw, qx, qy, qz]
        Eigen::Quaterniond q_Hamilton(quat_JPL(3), quat_JPL(0), quat_JPL(1), quat_JPL(2));
        Eigen::Matrix3d R_GtoI = q_Hamilton.toRotationMatrix();
        
        // Create transformation matrix T_ItoG (IMU to Global)
        Eigen::Matrix4d T_ItoG = Eigen::Matrix4d::Identity();
        T_ItoG.block<3, 3>(0, 0) = R_GtoI.transpose(); // R_ItoG = R_GtoI^T
        T_ItoG.block<3, 1>(0, 3) = pos;
        
        // Draw camera frustum at current position
        draw_camera(T_ItoG, 0.3, 0.2, 0.5, 1.0, 0.0, 0.0); // Red camera
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

  // Trajectory storage
  std::vector<Eigen::Vector3d> trajectory_;
  std::mutex traj_mutex_;
};

} // namespace ov_msckf

#endif // OV_MSCKF_NONROS_VISUALIZER_H
