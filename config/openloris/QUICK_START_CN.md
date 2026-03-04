# 快速使用指南 - OpenLORIS 针孔相机配置

## 配置说明

已配置好用于去畸变后的针孔相机数据集（fisheye1_rect 和 fisheye2_rect）。

### 数据集结构

你的数据集应该包含以下文件：

```
cafe1-1/
├── t265_gyroscope.txt          # IMU陀螺仪数据
├── t265_accelerometer.txt      # IMU加速度计数据
├── fisheye1_rect.txt           # 左相机时间戳文件
├── fisheye1_rect/              # 左相机图像文件夹
│   └── *.png
├── fisheye2_rect.txt           # 右相机时间戳文件
└── fisheye2_rect/              # 右相机图像文件夹
    └── *.png
```

### 使用方法

#### 1. 使用默认数据集路径运行（推荐）

```bash
./run_openloris_pinhole.sh
```

默认数据集路径：`/media/vector/Software/ubuntu_datasets/cafe1-1_2-package/cafe1-1`

#### 2. 使用自定义数据集路径

```bash
./run_openloris_pinhole.sh /path/to/your/dataset
```

#### 3. 直接运行可执行文件

```bash
./ov_msckf/build/run_openloris \
  config/openloris/estimator_config_pinhole.yaml \
  /media/vector/Software/ubuntu_datasets/cafe1-1_2-package/cafe1-1
```

## 配置文件说明

### 相机内参配置 (kalibr_imucam_chain_pinhole.yaml)

基于你提供的立体校准参数配置：
- **分辨率**: 640x400
- **焦距**: fx=fy=254.39
- **主点**: cx=354.93, cy=84.80
- **畸变**: 无（已去畸变）
- **基线**: ~64mm

### 相机名称配置 (estimator_config_pinhole.yaml)

```yaml
openloris_imu_prefix: "t265"
openloris_cam0_name: "fisheye1_rect"
openloris_cam1_name: "fisheye2_rect"
```

**如果你的数据集使用不同的名称，只需修改这三个参数即可，无需重新编译！**

## 输出结果

结果将保存到 `/tmp/` 目录：
- `/tmp/openloris_pinhole_estimate.txt` - 估计轨迹
- `/tmp/openloris_pinhole_estimate_std.txt` - 估计标准差
- `/tmp/openloris_pinhole_timing.txt` - 性能统计

## 重要提醒

✅ **配置已就绪！** 

针孔相机配置已经使用了 T265 的以下参数：

1. **相机-IMU外参** (`kalibr_imucam_chain_pinhole.yaml`)
   - ✅ 已配置为 T265 的相机-IMU外参
   - cam0 和 cam1 的外参与 T265 保持一致

2. **IMU噪声参数** (`kalibr_imu_chain_pinhole.yaml`)
   - ✅ 已配置为 T265 的 IMU 噪声参数
   - 包括陀螺仪和加速度计的内参矫正

**只需确保你的去畸变图像内参正确即可直接运行！**

## 修改记录

### 2024-03-04 更新
- ✅ 支持从配置文件读取相机和IMU名称
- ✅ 配置为使用 `fisheye1_rect` 和 `fisheye2_rect`
- ✅ 默认数据集路径设置为 `cafe1-1`
- ✅ 更新了 `run_openloris.cpp` 以支持配置文件中的相机名称参数

## 更多信息

详细文档请参考：`config/openloris/README_PINHOLE.md`
