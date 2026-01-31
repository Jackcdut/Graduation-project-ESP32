# 扩展屏功能使用指南

## 快速开始

### 1. 编译和烧录

```bash
# 编译项目
idf.py build

# 烧录到ESP32-P4
idf.py flash monitor
```

### 2. 启动扩展屏模式

1. **在ESP32设备上**：
   - 进入"设置"页面
   - 找到"扩展屏"开关
   - 打开开关启动扩展屏模式
   - 等待状态显示"USB Extend Screen Ready!"

2. **在Windows PC上**：
   - 安装Windows IDD驱动（位于 `win10_idd_xfz1986_usb_graphic_driver_display-main/`）
   - 通过USB连接ESP32-P4到PC
   - Windows会自动识别为新显示器
   - 在显示设置中配置为扩展屏或复制屏

### 3. 使用扩展屏

- **分辨率**：800x480
- **刷新率**：最高60 FPS
- **颜色深度**：RGB565 (16位)
- **传输方式**：USB 2.0 High Speed

### 4. 停止扩展屏模式

- 在ESP32设备上关闭"扩展屏"开关
- 或在Windows上断开USB连接

## 性能模式选择

### 方案1：直接Framebuffer模式（推荐）⭐⭐⭐⭐⭐

**特点**：
- ✅ 最高性能：50-60 FPS
- ✅ 最低延迟：<20ms
- ✅ 最低CPU占用
- ✅ 硬件加速：JPEG解码 + DMA + 三缓冲

**适用场景**：
- 视频播放
- 游戏投屏
- 实时演示
- 需要高帧率的应用

**代码示例**：
```c
// 默认已设置为直接FB模式
extend_screen_set_mode(EXTEND_DISPLAY_MODE_DIRECT_FB);
extend_screen_start();
```

### 方案2：LVGL Canvas模式

**特点**：
- ⚠️ 较低性能：10-20 FPS
- ⚠️ 较高延迟：50-100ms
- ✅ 兼容性好
- ✅ 可与LVGL UI共存

**适用场景**：
- 静态内容显示
- 文档查看
- 不需要高帧率的应用

**代码示例**：
```c
extend_screen_set_mode(EXTEND_DISPLAY_MODE_LVGL_CANVAS);
extend_screen_start();
```

## 功能特性

### 1. 自动触摸管理

**进入扩展屏模式时**：
- ✅ 自动禁用ESP32触摸输入
- ✅ 避免误触影响PC操作

**退出扩展屏模式时**：
- ✅ 自动恢复ESP32触摸功能
- ✅ 可以正常操作本地UI

### 2. UI自动隐藏（直接FB模式）

**进入扩展屏模式时**：
- ✅ 自动隐藏LVGL UI
- ✅ 全屏显示PC内容
- ✅ 无UI元素干扰

**退出扩展屏模式时**：
- ✅ 自动恢复LVGL UI
- ✅ 返回设置页面

### 3. 硬件加速

- ✅ **JPEG解码**：ESP32-P4硬件JPEG解码器
- ✅ **DMA传输**：零CPU占用的数据传输
- ✅ **三缓冲**：解码、传输、显示并行工作

### 4. 性能监控

**FPS显示**：
- 每50帧输出一次FPS统计
- 日志示例：`📺 Display FPS: 58.3 (Direct FB mode)`

**查看日志**：
```bash
idf.py monitor
```

## 故障排除

### 问题1：Windows无法识别设备

**解决方案**：
1. 检查USB连接是否正常
2. 确认已安装Windows IDD驱动
3. 在设备管理器中查看是否有未知设备
4. 重新安装驱动程序

### 问题2：帧率低于预期

**解决方案**：
1. 确认使用直接FB模式：
   ```c
   extend_screen_set_mode(EXTEND_DISPLAY_MODE_DIRECT_FB);
   ```
2. 减少日志输出（修改sdkconfig）：
   ```
   CONFIG_LOG_DEFAULT_LEVEL_INFO=y
   CONFIG_LOG_DEFAULT_LEVEL_DEBUG=n
   ```
3. 检查USB线缆质量（使用USB 2.0高速线缆）

### 问题3：触摸不工作

**症状**：退出扩展屏模式后，触摸无响应

**解决方案**：
1. 检查日志是否有"Touch re-enabled"消息
2. 重启设备
3. 检查代码中是否正确调用了`restore_touch_after_extend_mode()`

### 问题4：屏幕花屏或黑屏

**解决方案**：
1. 检查USB连接稳定性
2. 降低分辨率或刷新率
3. 检查Windows驱动配置
4. 查看ESP32日志是否有错误

### 问题5：UI不显示

**症状**：退出扩展屏模式后，LVGL UI不显示

**解决方案**：
1. 检查日志是否有"LVGL UI restored"消息
2. 确认使用的是直接FB模式
3. 重启设备

## 性能优化建议

### 1. 网络优化
- 使用USB 2.0高速线缆
- 避免使用USB Hub
- 直接连接到PC的USB 3.0端口（向下兼容）

### 2. 系统优化
- 关闭不必要的日志输出
- 使用直接FB模式
- 启用三缓冲（默认已启用）

### 3. 内容优化
- 使用JPEG压缩（Windows驱动自动处理）
- 避免频繁的全屏更新
- 利用脏矩形优化（Windows驱动自动处理）

## 技术规格

| 参数 | 值 |
|------|-----|
| 分辨率 | 800x480 |
| 颜色深度 | RGB565 (16位) |
| 最大帧率 | 60 FPS |
| USB接口 | USB 2.0 High Speed |
| 传输带宽 | 480 Mbps |
| JPEG压缩 | 硬件加速 |
| 缓冲模式 | 三缓冲 |
| 延迟 | <20ms (直接FB模式) |

## 开发者信息

### 修改显示模式

在 `main/main.c` 或 `BSP/GUIDER/custom/extend_screen.c` 中：

```c
// 设置默认模式为直接FB
g_display_mode = EXTEND_DISPLAY_MODE_DIRECT_FB;

// 或在运行时切换
extend_screen_set_mode(EXTEND_DISPLAY_MODE_DIRECT_FB);
```

### 调整缓冲区数量

在 `BSP/GUIDER/custom/usb_extend/app_lcd_extend.h` 中：

```c
// 双缓冲（较低内存占用）
#define USB_EXTEND_LCD_BUF_NUM  (2)

// 三缓冲（最佳性能，推荐）
#define USB_EXTEND_LCD_BUF_NUM  (3)
```

### 自定义FPS统计间隔

在 `BSP/GUIDER/custom/usb_extend/app_lcd_extend.c` 中：

```c
// 每50帧统计一次（默认）
if (fps_count == 50) {
    // 输出FPS
}

// 改为每100帧统计一次
if (fps_count == 100) {
    // 输出FPS
}
```

## 更多信息

- 架构设计：参见 `docs/EXTEND_SCREEN_HYBRID_MODE.md`
- Windows驱动：参见 `win10_idd_xfz1986_usb_graphic_driver_display-main/`
- 问题反馈：提交Issue到项目仓库

