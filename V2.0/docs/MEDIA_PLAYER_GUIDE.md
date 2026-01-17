# 媒体播放器模块使用指南

## 概述

媒体播放器模块支持在 ESP32-P4 上全屏查看图片和播放视频，使用 ESP-IDF 官方 `avi_player` 组件。

## 支持的格式

### 图片格式
- **BMP** - 完全支持，无需额外配置
- **PNG** - 需要启用 LVGL PNG 解码器（见配置说明）
- **JPEG** - 计划支持（利用硬件 JPEG 解码器）

### 视频格式
- **AVI (MJPEG + PCM)** - 完全支持，硬件加速
  - 视频编码：MJPEG（Motion JPEG）- ESP32-P4 硬件解码
  - 音频编码：PCM（16-bit，单声道/立体声）
  - 推荐分辨率：800x480（匹配屏幕）或 1280x720
  - 推荐帧率：15-30 fps

- **AVI (H.264 + PCM)** - 仅解析，无硬件解码
  - ESP32-P4 没有 H.264 硬件解码器
  - 建议转换为 MJPEG 格式

## 重要：视频格式要求

ESP32-P4 只有 **JPEG 硬件解码器**，不支持 H.264 硬件解码。如果你的视频是 H.264 编码，需要转换为 MJPEG 格式。

## 视频转换

### 方法1：使用转换脚本（推荐）

```batch
tools\convert_video.bat input.mp4 output.avi
```

### 方法2：手动使用 FFmpeg

```bash
ffmpeg -i input.mp4 -vcodec mjpeg -q:v 5 -acodec pcm_s16le -ar 22050 -ac 1 -s 800x480 output.avi
```

### 参数说明
| 参数 | 说明 |
|------|------|
| `-vcodec mjpeg` | 使用 MJPEG 视频编码（ESP32-P4 硬件解码） |
| `-q:v 5` | 视频质量（1-31，越小质量越高，文件越大） |
| `-acodec pcm_s16le` | 使用 16-bit PCM 音频 |
| `-ar 22050` | 音频采样率 22050 Hz |
| `-ac 1` | 单声道（使用 `-ac 2` 为立体声） |
| `-s 800x480` | 输出分辨率（匹配屏幕） |

### 高质量转换（1280x720）

```bash
ffmpeg -i input.mp4 -vcodec mjpeg -q:v 3 -acodec pcm_s16le -ar 44100 -ac 2 -s 1280x720 output.avi
```

## 使用方法

### 通过 SD 卡管理器

1. 打开 SD 卡管理界面
2. 点击 "Open Files" 按钮
3. 在弹出的 "Select Folder" 窗口中选择 "Media"
4. 浏览媒体文件列表
5. 点击文件即可全屏查看/播放

### 编程接口

```c
#include "media_player.h"

// 初始化
media_player_init();

// 播放媒体文件
media_player_play("/sdcard/Media/video.avi");

// 停止
media_player_stop();

// 设置音量 (0-100)
media_player_set_volume(80);

// 获取播放状态
media_player_state_t state = media_player_get_state();
bool is_playing = media_player_is_video_playing();

// 获取媒体信息
media_info_t info;
media_player_get_info(&info);
printf("Resolution: %dx%d\n", info.width, info.height);
```

## 文件存放位置

媒体文件应放在 SD 卡的以下目录：
- `/sdcard/Media/` - 推荐目录

## 架构说明

```
┌─────────────────────────────────────────────────────────────┐
│                    media_player.c                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │ AVI Parser  │  │ JPEG HW Dec │  │ Audio Output        │ │
│  │ (avi_player)│  │ (esp_jpeg)  │  │ (bsp_extra_codec)   │ │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │                │                     │            │
│         ▼                ▼                     ▼            │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              LVGL Display (800x480)                     ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

## 配置说明

### 启用 PNG 支持（可选）

在 `sdkconfig.defaults` 中添加：

```
CONFIG_LV_USE_PNG=y
CONFIG_LV_USE_FS_POSIX=y
CONFIG_LV_FS_POSIX_LETTER='S'
```

### 硬件加速

项目已配置以下硬件加速：
- **JPEG 硬件解码** - `CONFIG_SOC_JPEG_DECODE_SUPPORTED=y`
- **PPA 图像处理** - `CONFIG_LVGL_PORT_ENABLE_PPA=y`

## 模块文件

```
BSP/GUIDER/custom/modules/media_player/
├── media_player.h      # 主模块头文件
└── media_player.c      # 主模块实现（使用 ESP-IDF avi_player 组件）

tools/
└── convert_video.bat   # 视频转换脚本
```

## 依赖组件

在 `main/idf_component.yml` 中：
```yaml
dependencies:
  espressif/avi_player:
    version: "*"
  esp_driver_jpeg:
    version: "*"
```

## 注意事项

1. **视频格式** - 必须使用 MJPEG 编码，H.264 无法硬件解码
2. **内存使用** - 视频播放需要约 2MB PSRAM 用于帧缓冲
3. **文件大小** - 建议单个视频文件不超过 100MB
4. **帧率** - 高分辨率视频建议使用较低帧率（15-20 fps）
5. **音频** - 仅支持 PCM 格式，不支持 MP3/AAC 等压缩格式

## 故障排除

### 视频无法播放
- 检查视频编码是否为 MJPEG（使用 `ffprobe` 查看）
- 确保音频为 PCM 格式
- 检查文件路径是否正确

### 播放卡顿
- 降低视频分辨率（推荐 800x480）
- 降低帧率（推荐 15-20 fps）
- 增加 JPEG 质量参数（-q:v 值增大）

### 无声音
- 检查音量设置
- 确保音频为 PCM 格式
- 检查采样率是否支持（推荐 22050 或 44100 Hz）

---

*最后更新: 2025-12-28*
