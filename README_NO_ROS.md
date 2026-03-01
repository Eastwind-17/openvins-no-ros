# OpenVINS 无 ROS 版本

OpenVINS Visual-Inertial Odometry 系统的无 ROS 依赖版本。

## 特性

✅ **完全去除 ROS 依赖** - 使用纯 CMake 构建  
✅ **黑色背景可视化** - 减少眼睛疲劳  
✅ **相机模型显示** - 红色 frustum 显示当前相机位姿  
✅ **实时特征跟踪** - OpenVINS 原生可视化  
✅ **3D 轨迹显示** - Pangolin 交互式 3D 视图  

## 快速开始

### 编译（仅需一次）
```bash
docker exec d13d6171c0ed bash /home/vector/code/open_vins/build_no_ros.sh
```

### 运行
```bash
docker exec d13d6171c0ed bash /home/vector/code/open_vins/run_euroc_custom.sh
```

## 可视化

<可视化效果示意>

**Pangolin 3D 窗口**
- 黑色背景
- 青色轨迹线
- 红色相机 frustum 模型
- RGB 坐标轴

**OpenCV 特征跟踪窗口**
- 实时特征点跟踪
- 特征历史轨迹

## 交互控制

**Pangolin 3D 窗口:**
- 左键拖动: 旋转视角
- 右键拖动: 平移场景
- 滚轮: 缩放
- 双击: 重置视角

**退出:**
- Ctrl+C

## 数据集

当前配置使用:
```
/media/vector/Software/ubuntu_datasets/vicon_room1/V1_01_easy/V1_01_easy/mav0
```

支持标准 EuRoC MAV 数据集格式。

## 文档

- **[USAGE_NO_ROS.md](USAGE_NO_ROS.md)** - 详细使用说明
- **[VISUALIZATION_FEATURES.md](VISUALIZATION_FEATURES.md)** - 可视化功能详解
- **[FINAL_STATUS.md](FINAL_STATUS.md)** - 完整技术文档

## 系统要求

- CMake >= 3.3
- Eigen3
- OpenCV >= 3.0
- Boost
- Ceres Solver
- Pangolin

## 典型输出

```
Loaded 29120 IMU measurements
Loaded 2912 camera measurements for cam0
Loaded 2912 camera measurements for cam1
Starting VIO processing...
[init]: successful initialization in 0.0004 seconds
=== SYSTEM INITIALIZED ===
Frame 200/2912 | Time: 1403715283.262 | Position: [0.878, 0.077, 0.146]
Frame 400/2912 | Time: 1403715293.262 | Position: [-0.374, -1.696, 0.377]
...
```

## 许可证

遵循 OpenVINS 原始许可证 (GPLv3)

## 参考

基于 [OpenVINS](https://github.com/rpng/open_vins) 2.7
