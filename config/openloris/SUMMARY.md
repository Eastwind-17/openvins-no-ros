# OpenLORIS 针孔相机配置 - 配置完成总结

## ✅ 配置完成状态

所有配置文件已准备就绪，可以直接使用！

---

## 📁 创建/修改的文件

### 1. 配置文件
- ✅ `config/openloris/estimator_config_pinhole.yaml` - VIO估计器配置
- ✅ `config/openloris/kalibr_imucam_chain_pinhole.yaml` - 相机内参和外参（已使用T265外参）
- ✅ `config/openloris/kalibr_imu_chain_pinhole.yaml` - IMU参数（已使用T265参数）

### 2. 代码修改
- ✅ `ov_msckf/src/run_openloris.cpp` - 支持从配置文件读取相机名称

### 3. 脚本和文档
- ✅ `run_openloris_pinhole.sh` - 运行脚本
- ✅ `config/openloris/README_PINHOLE.md` - 详细英文文档
- ✅ `config/openloris/QUICK_START_CN.md` - 中文快速指南
- ✅ `config/openloris/SUMMARY.md` - 本总结文档

---

## 🎯 关键配置信息

### 数据集设置
```yaml
# 在 estimator_config_pinhole.yaml 中
openloris_imu_prefix: "t265"
openloris_cam0_name: "fisheye1_rect"
openloris_cam1_name: "fisheye2_rect"
```

### 相机内参（针孔去畸变）
- **分辨率**: 640x400
- **焦距**: fx=fy=254.39
- **主点**: cx=354.93, cy=84.80
- **畸变**: 无（radtan模型，系数全为0）

### 相机-IMU外参
✅ **已配置** - 使用 T265 的相机-IMU外参
- cam0 T_cam_imu: 从 T265 fisheye1 复制
- cam1 T_cam_imu: 从 T265 fisheye2 复制

### IMU参数
✅ **已配置** - 使用 T265 的 IMU 噪声和内参
- 陀螺仪噪声密度: 0.00227
- 加速度计噪声密度: 0.00818
- 包含 T265 的 IMU 内参矫正矩阵

---

## 🚀 如何使用

### 方法1：使用脚本（推荐）
```bash
# 使用默认路径
./run_openloris_pinhole.sh

# 或指定路径
./run_openloris_pinhole.sh /path/to/cafe1-1
```

### 方法2：直接命令
```bash
./ov_msckf/build/run_openloris \
  config/openloris/estimator_config_pinhole.yaml \
  /media/vector/Software/ubuntu_datasets/cafe1-1_2-package/cafe1-1
```

---

## 📂 数据集结构要求

```
cafe1-1/
├── t265_gyroscope.txt          # IMU陀螺仪数据
├── t265_accelerometer.txt      # IMU加速度计数据
├── fisheye1_rect.txt           # 左相机时间戳列表
├── fisheye1_rect/              # 左相机去畸变图像
│   ├── timestamp1.png
│   └── ...
├── fisheye2_rect.txt           # 右相机时间戳列表
└── fisheye2_rect/              # 右相机去畸变图像
    ├── timestamp1.png
    └── ...
```

---

## 🔧 灵活性增强

### 无需重新编译即可更换数据集
只需修改 `estimator_config_pinhole.yaml` 中的相机名称：

```yaml
# 例如，如果你的数据集使用 cam0/cam1 命名
openloris_cam0_name: "cam0"
openloris_cam1_name: "cam1"
```

---

## 📊 输出文件

运行后结果保存在 `/tmp/` 目录：
- `/tmp/openloris_pinhole_estimate.txt` - 估计的轨迹
- `/tmp/openloris_pinhole_estimate_std.txt` - 估计的标准差
- `/tmp/openloris_pinhole_timing.txt` - 性能统计信息
- `/tmp/openloris_pinhole_groundtruth.txt` - 真值（如果有）
- `/home/vector/code/open_vins/traj_pinhole.txt` - 轨迹备份

---

## ✨ 与 T265 配置的区别

| 参数 | T265 (原始鱼眼) | Pinhole (去畸变) |
|------|----------------|------------------|
| 相机模型 | equidistant | pinhole |
| 畸变 | 有鱼眼畸变 | 无畸变（已去畸变） |
| 分辨率 | 848x800 | 640x400 |
| 相机名称 | fisheye1, fisheye2 | fisheye1_rect, fisheye2_rect |
| IMU外参 | T265原始外参 | **相同（T265外参）** |
| IMU参数 | T265 IMU参数 | **相同（T265参数）** |
| 网格大小 | 10x10 | 8x5 |
| FAST阈值 | 15 | 20 |
| 掩码 | 需要（鱼眼边缘） | 不需要 |

---

## 💡 关键改进说明

### 1. 代码层面
- 修改了 `run_openloris.cpp` 以支持从配置文件读取相机和IMU名称
- 向后兼容：未配置时使用默认值（t265, fisheye1, fisheye2）

### 2. 配置层面
- 使用 T265 的相机-IMU外参（因为是同一套传感器）
- 使用 T265 的 IMU 噪声参数和内参
- 只更新了相机内参以匹配去畸变后的针孔模型

### 3. 用户体验
- 一键运行脚本
- 支持命令行参数
- 详细的中英文文档

---

## 🎉 就绪状态

**配置已完成，可以直接运行！**

你的去畸变针孔相机使用的是与 T265 相同的传感器设置，所以：
- ✅ 相机-IMU外参相同
- ✅ IMU噪声参数相同  
- ✅ 只有相机内参不同（因为去畸变后的分辨率和焦距改变）

直接运行即可：
```bash
./run_openloris_pinhole.sh
```

---

## 📞 问题排查

如果遇到问题，请检查：
1. 数据集路径是否正确
2. 数据集文件结构是否符合要求（fisheye1_rect, fisheye2_rect）
3. 图像文件是否存在
4. 可执行文件是否已编译（`./ov_msckf/build/run_openloris`）

详细信息请查看：`config/openloris/README_PINHOLE.md`

---

**配置完成时间**: 2024-03-05  
**版本**: v1.0
