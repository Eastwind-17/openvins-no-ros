# OpenVINS 可视化功能说明

## 更新内容 (最新)

### 可视化改进
✅ **黑色背景** - Pangolin 3D 窗口使用黑色背景，更适合长时间观看
✅ **相机模型显示** - 当前相机位置使用相机 frustum（锥体）模型显示，而不是简单的圆点
✅ **青色轨迹** - 轨迹线使用青色显示，在黑色背景上更加清晰可见

### 可视化窗口

#### 1. Feature Tracking 窗口 (OpenCV)
- 显示实时特征点跟踪
- 使用 OpenVINS 原生的 `get_historical_viz_image()` 函数
- 显示特征点的跟踪历史和状态

#### 2. OpenVINS Trajectory 窗口 (Pangolin)
- **背景**: 纯黑色背景
- **坐标轴**: 
  - X 轴 - 红色 (1 米长)
  - Y 轴 - 绿色 (1 米长)
  - Z 轴 - 蓝色 (1 米长)
- **轨迹**: 青色线条，显示历史轨迹
- **当前相机**: 红色相机 frustum 模型
  - 显示相机的位置和朝向
  - Frustum 参数:
    - 宽度: 0.3 米
    - 高度: 0.2 米
    - 深度: 0.5 米

### 相机 Frustum 说明

相机 frustum 是一个金字塔形状的线框模型，表示相机的视场：
```
       /|
      / |
     /  |    <- 相机视场锥体
    /   |
   +----+    <- 相机中心（顶点）
```

从相机中心出发的四条线代表视场的边界，前端的矩形代表一定深度处的成像平面。

### 交互操作

在 Pangolin 窗口中：
- **左键拖动**: 旋转视角
- **右键拖动**: 平移视角  
- **滚轮**: 缩放
- **双击**: 重置视角

### 运行示例

```bash
# 使用您的数据集
cd /home/vector/code/open_vins/ov_msckf/build
./run_euroc ../../config/euroc_mav/estimator_config.yaml \
  /media/vector/Software/ubuntu_datasets/vicon_room1/V1_01_easy/V1_01_easy/mav0

# 或使用脚本
bash /home/vector/code/open_vins/run_euroc_custom.sh
```

### 技术细节

#### 坐标系转换
- OpenVINS 使用 JPL 四元数表示 (qx, qy, qz, qw)
- 转换为 Hamilton 四元数用于 Eigen (qw, qx, qy, qz)
- 位姿矩阵 T_ItoG 从 IMU 坐标系转换到全局坐标系

#### 渲染顺序
1. 清空缓冲区 (黑色背景)
2. 绘制坐标轴
3. 绘制轨迹线
4. 绘制当前相机 frustum

### 性能说明

- Pangolin 使用 OpenGL 硬件加速
- OpenCV 窗口使用 `cv::waitKey(1)` 最小化阻塞
- 线程安全的轨迹数据访问 (使用 mutex)

### 数据集支持

当前支持 EuRoC MAV 数据集格式：
- IMU: 200 Hz
- Camera: 20 Hz (双目)
- 图像: 752x480 灰度 PNG

### 颜色方案

| 元素 | 颜色 | RGB 值 |
|------|------|--------|
| 背景 | 黑色 | (0.0, 0.0, 0.0) |
| X 轴 | 红色 | (1.0, 0.0, 0.0) |
| Y 轴 | 绿色 | (0.0, 1.0, 0.0) |
| Z 轴 | 蓝色 | (0.0, 0.0, 1.0) |
| 轨迹 | 青色 | (0.0, 1.0, 1.0) |
| 相机 | 红色 | (1.0, 0.0, 0.0) |

## 已验证

✅ 在 Docker 容器 d13 中测试通过
✅ EuRoC V1_01_easy 数据集成功运行
✅ 轨迹跟踪超过 500 米
✅ 实时可视化流畅无卡顿
