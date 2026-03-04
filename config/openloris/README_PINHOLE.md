# OpenLORIS Pinhole Stereo Configuration

This directory contains configuration files for running OpenVINS on OpenLORIS dataset with **pinhole stereo cameras**.

## Configuration Files

### 1. `estimator_config_pinhole.yaml`
Main VIO estimator configuration for pinhole stereo cameras.

**Key settings:**
- **Stereo mode:** Enabled (`use_stereo: true`)
- **Feature tracking:** Optimized for pinhole cameras (640x400 resolution)
- **Grid size:** 8x5 (adjusted for 640x400 image size)
- **FAST threshold:** 20
- **Number of features:** 200
- **Tracking frequency:** 20 Hz (adjust based on your camera FPS)

### 2. `kalibr_imucam_chain_pinhole.yaml`
Camera intrinsics and extrinsics configuration.

**Camera intrinsics (from your provided config):**
- **Camera model:** pinhole
- **Resolution:** 640x400
- **Intrinsics:** [fx=254.39, fy=254.39, cx=354.93, cy=84.80]
- **Distortion:** radtan (all coefficients are 0.0 - no distortion)
- **Baseline:** ~64mm (0.064m in x-direction)

**Important:** 
- The `T_cam_imu` transformation is set to identity. **You must update this with your actual camera-to-IMU extrinsics.**
- The stereo transformation `T_c1_c2` is configured based on your provided stereo calibration.

### 3. `kalibr_imu_chain_pinhole.yaml`
IMU noise parameters and intrinsics.

**Important:** The IMU noise parameters are **example values**. You should:
1. Check your IMU sensor datasheet for actual noise specifications
2. Or perform IMU calibration using tools like Kalibr or Allan variance analysis

**Default values:**
- `accelerometer_noise_density: 0.01`
- `accelerometer_random_walk: 0.001`
- `gyroscope_noise_density: 0.005`
- `gyroscope_random_walk: 0.0001`

## Camera Parameters Explanation

Your provided configuration had these parameters:

```yaml
# From stereo_fisheye_rectify.py output
Camera.type: PinHole
Camera1.fx: 254.3928153045
Camera1.fy: 254.3928153045
Camera1.cx: 354.9341066832
Camera1.cy: 84.7966105271
Camera.width: 640
Camera.height: 400

Stereo.ThDepth: 35.0
Stereo.T_c1_c2: (4x4 transformation matrix)
Camera.bf: -16.2755238752
```

### Stereo Baseline Calculation
From your config:
- `Camera.bf = -16.2755238752`
- `fx = 254.3928153045`
- **Baseline = |bf| / fx = 16.2755238752 / 254.3928153045 ≈ 0.064m = 64mm**

This matches the translation in `T_c1_c2[0,3] = 0.06397651m`.

## Dataset Structure

The OpenLORIS loader expects the following dataset structure for **rectified pinhole cameras**:

```
dataset_path/  (e.g., cafe1-1/)
├── t265_gyroscope.txt              # IMU gyroscope data
├── t265_accelerometer.txt          # IMU accelerometer data
├── fisheye1_rect.txt               # Camera 0 (left) timestamps
├── fisheye1_rect/                  # Camera 0 rectified images
│   ├── timestamp1.png
│   ├── timestamp2.png
│   └── ...
├── fisheye2_rect.txt               # Camera 1 (right) timestamps
└── fisheye2_rect/                  # Camera 1 rectified images
    ├── timestamp1.png
    ├── timestamp2.png
    └── ...
```

**Note:** The configuration is set to use `fisheye1_rect` and `fisheye2_rect` for rectified (undistorted) pinhole images. These camera names are configured in `estimator_config_pinhole.yaml`:
- `openloris_imu_prefix: "t265"`
- `openloris_cam0_name: "fisheye1_rect"`
- `openloris_cam1_name: "fisheye2_rect"`

### File Formats

**IMU files format (Unix seconds):**
```
timestamp gx gy gz         # gyroscope.txt
timestamp ax ay az         # accelerometer.txt
```

**Camera files format:**
```
timestamp relative_image_path
```
Example:
```
1234567890.123456 cam0/1234567890123456.png
1234567890.156789 cam0/1234567890156789.png
```

## Usage

### Option 1: Using the shell script (recommended)

Run with default dataset path (cafe1-1):
```bash
./run_openloris_pinhole.sh
```

Or specify a custom dataset path:
```bash
./run_openloris_pinhole.sh /path/to/your/dataset
```

### Option 2: Direct command

```bash
./ov_msckf/build/run_openloris \
  config/openloris/estimator_config_pinhole.yaml \
  /media/vector/Software/ubuntu_datasets/cafe1-1_2-package/cafe1-1
```

### Option 3: Customize camera/IMU names in config

If your dataset uses different camera or IMU names, simply edit `estimator_config_pinhole.yaml`:

```yaml
# OpenLORIS dataset camera and IMU names
openloris_imu_prefix: "t265"           # Change to your IMU prefix
openloris_cam0_name: "fisheye1_rect"   # Change to your cam0 name
openloris_cam1_name: "fisheye2_rect"   # Change to your cam1 name
```

**No need to recompile!** The `run_openloris` executable now reads these parameters from the config file.

## Important Calibration Notes

### 1. Camera-IMU Extrinsics

The `T_cam_imu` in `kalibr_imucam_chain_pinhole.yaml` is currently set to **identity (no rotation, no translation)**.

**You must update this with your actual calibration:**
- Use Kalibr tool for camera-IMU calibration
- Or use your existing calibration data
- Format: 4x4 transformation matrix from camera frame to IMU frame

### 2. IMU Noise Parameters

The IMU noise parameters in `kalibr_imu_chain_pinhole.yaml` are **generic examples**.

**Recommended:**
- Check your IMU datasheet for noise specifications
- Or perform Allan variance analysis
- Update the YAML file with your actual values

### 3. Stereo Calibration Verification

The stereo transformation `T_c1_c2` has been configured from your provided data:
```yaml
T_c1_c2: [
  [0.99997103, -0.00308591,  0.00695768,  0.06397651],
  [0.00311212,  0.99998809, -0.00375913,  0.00014827],
  [-0.00694600,  0.00378068,  0.99996873, -0.00039847],
  [0.0,         0.0,         0.0,         1.0]
]
```

This represents:
- **Rotation:** Very small (~0.4 degrees total)
- **Translation:** [63.98mm, 0.15mm, -0.40mm] - primarily in x-direction (baseline)

## Output Files

Results will be saved to `/tmp/`:
- `/tmp/openloris_pinhole_timing.txt` - Timing statistics
- `/tmp/openloris_pinhole_estimate.txt` - Estimated trajectory
- `/tmp/openloris_pinhole_estimate_std.txt` - Estimation standard deviations
- `/tmp/openloris_pinhole_groundtruth.txt` - Ground truth (if available)

## Troubleshooting

### Issue: "Failed to load camera data"
- Check that your dataset has the correct file structure
- Verify camera names match what's in `run_openloris.cpp` (default: "fisheye1", "fisheye2")
- Modify the loader if your dataset uses different names

### Issue: "Failed to load IMU data"
- Check that IMU files exist: `<prefix>_gyroscope.txt` and `<prefix>_accelerometer.txt`
- Default prefix is "t265", modify `run_openloris.cpp` if yours is different

### Issue: Poor tracking performance
- Adjust `fast_threshold` in `estimator_config_pinhole.yaml` (increase if too many features, decrease if too few)
- Adjust `num_pts` to change number of tracked features
- Check that your images are grayscale (loader converts to grayscale automatically)

### Issue: Incorrect scale or drift
- Verify your camera-IMU extrinsics calibration
- Check IMU noise parameters
- Ensure IMU and camera timestamps are synchronized and in Unix seconds format

## Comparison with T265 Configuration

| Parameter | T265 (Fisheye) | Pinhole |
|-----------|----------------|---------|
| Camera model | equidistant | pinhole |
| Resolution | 848x800 | 640x400 |
| Distortion | equidistant | radtan (none) |
| Grid size | 10x10 | 8x5 |
| FAST threshold | 15 | 20 |
| Feature count | 200 | 200 |
| Tracking frequency | 30 Hz | 20 Hz |
| Masks | Yes (fisheye edges) | No |

## Additional Resources

- [OpenVINS Documentation](https://docs.openvins.com/)
- [Kalibr Calibration Tool](https://github.com/ethz-asl/kalibr)
- [OpenLORIS-Scene Dataset](https://lifelong-robotic-vision.github.io/dataset/scene)

## Notes

- The existing `OpenLorisLoader.h` is already generic and works with both fisheye and pinhole cameras
- The camera model (fisheye vs pinhole) is determined by the YAML configuration, not the loader
- No code changes are needed to the loader itself
- Only configuration files need to be adjusted for your specific camera setup
