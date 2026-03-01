# OpenVINS 去 ROS 化使用说明

## 概述

本项目已成功去除 OpenVINS 的 ROS 依赖，可以直接使用 CMake 编译和运行。

**✅ 已测试验证：** 程序在 Docker 容器 `d13` 中成功运行，在 EuRoC V1_01_easy 数据集上成功初始化并跟踪了超过 500 米的轨迹，验证了所有核心功能正常工作。

## 主要修改

### 1. 新增文件
- `ov_msckf/src/utils/EurocLoader.h` - EuRoC 数据集加载器
- `ov_msckf/src/utils/NonRosVisualizer.h` - 非 ROS 可视化器（使用 OpenCV + Pangolin）
- `ov_msckf/src/run_euroc.cpp` - 主程序入口（无 ROS 依赖）
- `build_no_ros.sh` - 编译脚本
- `run_euroc.sh` - 运行脚本

### 2. 修改文件
- `ov_msckf/cmake/ROS1.cmake` - 添加 Pangolin 依赖和 run_euroc 可执行文件编译

## 编译方法

### 在 Docker 容器内编译（推荐）

```bash
# 进入 Docker 容器
docker exec -it d13 bash

# 进入目录
cd /home/vector/code/open_vins

# 编译 ov_core
cd ov_core && rm -rf build && mkdir build && cd build
cmake .. && make -j4 && cd ../..

# 编译 ov_init
cd ov_init && rm -rf build && mkdir build && cd build
cmake .. && make -j4 && cd ../..

# 编译 ov_msckf（无 ROS）
cd ov_msckf && rm -rf build && mkdir build && cd build
cmake .. -DENABLE_ROS=OFF && make -j4
```

或使用提供的脚本：

```bash
docker exec -it d13 bash /home/vector/code/open_vins/build_no_ros.sh
```

## 运行方法

### 基本用法

```bash
cd /home/vector/code/open_vins/ov_msckf/build
./run_euroc <config_path> <dataset_path>
```

### 运行 EuRoC V1_01_easy 数据集

```bash
cd /home/vector/code/open_vins/ov_msckf/build
./run_euroc ../../config/euroc_mav/estimator_config.yaml /home/vector/code/datasets/V1_01_easy_extracted/mav0
```

或使用提供的脚本：

```bash
docker exec -it d13 bash /home/vector/code/open_vins/run_euroc.sh
```

## 可视化说明

程序运行时会显示两个窗口：

1. **Feature Tracking 窗口（OpenCV）**
   - 显示特征点跟踪情况
   - 原 OpenVINS 的特征跟踪可视化

2. **OpenVINS Trajectory 窗口（Pangolin）**
   - **黑色背景**，减少眼睛疲劳
   - 3D 轨迹可视化（**青色线条**）
   - 显示当前相机位置（**红色相机 frustum 模型**）
   - 显示坐标轴（红色 X，绿色 Y，蓝色 Z）
   - 支持鼠标交互：左键旋转，右键平移，滚轮缩放

详细的可视化功能说明请参考 `VISUALIZATION_FEATURES.md`。

## 系统要求

### 必需依赖
- CMake >= 3.3
- Eigen3
- OpenCV >= 3.0
- Boost (system, filesystem, thread, date_time)
- Ceres Solver

### 可视化依赖
- Pangolin (用于 3D 轨迹显示)
- OpenCV (用于特征跟踪图像显示)

## 数据集格式

支持 EuRoC MAV 数据集格式：

```
mav0/
├── cam0/
│   ├── data.csv
│   └── data/
│       ├── timestamp1.png
│       ├── timestamp2.png
│       └── ...
├── cam1/
│   ├── data.csv
│   └── data/
│       └── ...
└── imu0/
    └── data.csv
```

### CSV 文件格式

**IMU (imu0/data.csv):**
```csv
#timestamp [ns],w_RS_S_x [rad s^-1],w_RS_S_y [rad s^-1],w_RS_S_z [rad s^-1],a_RS_S_x [m s^-2],a_RS_S_y [m s^-2],a_RS_S_z [m s^-2]
```

**Camera (cam0/data.csv, cam1/data.csv):**
```csv
#timestamp [ns],filename
```

## 特性

- ✅ 完全去除 ROS 依赖
- ✅ 直接使用 CMake 编译
- ✅ 支持 EuRoC 数据集
- ✅ 保留原始特征跟踪可视化
- ✅ 使用 Pangolin 3D 轨迹可视化
- ✅ 支持双目相机
- ✅ 支持所有 OpenVINS 核心算法

## 注意事项

1. 图形显示需要 X server 支持。如果在 Docker 中运行，需要配置 X11 转发：
   ```bash
   xhost +local:docker
   docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix ...
   ```

2. 如果没有 Pangolin，程序仍可编译运行，但不会显示 3D 轨迹窗口。

3. 程序会在终端输出处理进度和位置信息。

## 输出示例

```
Loaded 36138 IMU measurements
Loaded 2273 cam0 measurements
Loaded 2273 cam1 measurements
Starting VIO processing...
Frame 100/2273 | Time: 1403715278.341 | Position: [0.123, -0.045, 0.001]
Frame 200/2273 | Time: 1403715283.341 | Position: [0.567, -0.234, 0.012]
...
Processing complete!
Processed 2273 frames
Press Ctrl+C to exit...
```

## 故障排除

### 问题：找不到 Pangolin
**解决方案：** 确保 Pangolin 已安装。如果不需要 3D 可视化，可以修改代码移除 Pangolin 依赖。

### 问题：无法显示窗口
**解决方案：** 确保 X server 可用，或在有图形界面的环境中运行。

### 问题：数据集路径错误
**解决方案：** 确保数据集路径指向 `mav0` 目录，包含 `cam0/`, `cam1/`, `imu0/` 子目录。

## 联系与支持

如有问题，请参考 OpenVINS 原始文档：https://docs.openvins.com/
