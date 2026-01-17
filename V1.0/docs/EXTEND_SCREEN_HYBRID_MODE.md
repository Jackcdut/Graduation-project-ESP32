# 扩展屏混合模式实现文档

## 概述

本文档描述了ESP32-P4扩展屏功能的混合模式实现，支持两种显示模式：
1. **LVGL Canvas模式**：低性能（~10-20 FPS），兼容性好
2. **直接Framebuffer模式**：高性能（~60 FPS），推荐使用

## 架构设计

### 模式切换流程

```
正常UI模式 (LVGL)
    ↓
[用户点击投屏按钮]
    ↓
禁用触摸输入
    ↓
[直接FB模式] 隐藏LVGL UI → 使用DPI framebuffer直接绘制
[Canvas模式] 创建LVGL Canvas → 通过LVGL渲染
    ↓
USB接收 → JPEG解码 → 显示
    ↓
[用户关闭投屏]
    ↓
恢复触摸输入
    ↓
[直接FB模式] 恢复LVGL UI
    ↓
返回正常UI模式
```

### 性能对比

| 模式 | 帧率 | CPU占用 | 内存占用 | 延迟 |
|------|------|---------|----------|------|
| **直接Framebuffer** | **50-60 FPS** | **低** | **低** | **<20ms** |
| LVGL Canvas | 10-20 FPS | 高 | 高 | 50-100ms |

## 关键实现

### 1. 直接Framebuffer模式

**优势**：
- ✅ 零拷贝：JPEG直接解码到DPI framebuffer
- ✅ 无LVGL开销：完全绕过LVGL渲染流程
- ✅ 硬件加速：JPEG解码器 + DMA + 三缓冲
- ✅ 低延迟：USB接收 → JPEG解码 → DMA输出

**实现**：
```c
// 获取DPI panel的framebuffer
esp_lcd_dpi_panel_get_frame_buffer(panel_handle, 3, &fb[0], &fb[1], &fb[2]);

// JPEG直接解码到framebuffer
jpeg_decoder_process(jpgd_handle, &decode_cfg, jpeg_data, jpeg_len, 
                     fb[current_index], fb_size, &out_size);

// 切换framebuffer显示
esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 800, 480, fb[current_index]);

// 轮换缓冲区
current_index = (current_index + 1) % 3;
```

### 2. 触摸管理

**进入扩展屏模式**：
```c
// 获取触摸设备句柄
lv_indev_t *touch_indev = bsp_display_get_input_dev();

// 禁用触摸
lv_indev_enable(touch_indev, false);
```

**退出扩展屏模式**：
```c
// 恢复触摸
lv_indev_enable(touch_indev, true);
```

### 3. UI隐藏/恢复（直接FB模式）

**隐藏UI**：
```c
// 保存当前屏幕
lv_obj_t *original_screen = lv_scr_act();

// 创建黑屏
lv_obj_t *blank_screen = lv_obj_create(NULL);
lv_obj_set_style_bg_color(blank_screen, lv_color_black(), 0);
lv_scr_load(blank_screen);
```

**恢复UI**：
```c
// 恢复原始屏幕
lv_scr_load(original_screen);
```

## API接口

### 设置显示模式
```c
// 设置为直接framebuffer模式（推荐）
extend_screen_set_mode(EXTEND_DISPLAY_MODE_DIRECT_FB);

// 设置为LVGL canvas模式
extend_screen_set_mode(EXTEND_DISPLAY_MODE_LVGL_CANVAS);
```

### 启动/停止扩展屏
```c
// 启动扩展屏模式
extend_screen_start();

// 停止扩展屏模式
extend_screen_stop();

// 检查运行状态
bool is_running = extend_screen_is_running();
```

## 配置说明

### 三缓冲配置
```c
#define USB_EXTEND_LCD_BUF_NUM  (3)  // 三缓冲以获得最佳性能
```

**三缓冲工作原理**：
- 缓冲区A：JPEG正在解码
- 缓冲区B：DMA正在传输到屏幕
- 缓冲区C：屏幕正在显示
- 三者并行工作，减少等待和撕裂

## 性能优化

### 1. JPEG解码优化
- 使用硬件JPEG解码器
- 直接解码到framebuffer，避免额外拷贝

### 2. 内存优化
- 直接FB模式：使用DPI panel的framebuffer，无需额外内存
- Canvas模式：使用PSRAM存储canvas buffer

### 3. 日志优化
- USB回调日志：每1000个包记录一次
- 帧日志：使用ESP_LOGD（默认关闭）
- FPS统计：每50帧输出一次

## 使用示例

```c
// 在设置页面初始化时
extend_screen_init(&guider_ui);

// 设置为高性能模式
extend_screen_set_mode(EXTEND_DISPLAY_MODE_DIRECT_FB);

// 用户点击投屏按钮
extend_screen_start();

// 用户关闭投屏
extend_screen_stop();
```

## 预期效果

✅ **直接Framebuffer模式**：
- 帧率：50-60 FPS
- 延迟：<20ms
- 触摸：自动禁用，退出后恢复
- UI：自动隐藏，退出后恢复

✅ **LVGL Canvas模式**：
- 帧率：10-20 FPS
- 延迟：50-100ms
- 触摸：自动禁用，退出后恢复
- UI：显示canvas覆盖层

## 故障排除

### 问题1：帧率低于预期
- 检查是否使用直接FB模式
- 检查日志级别（减少日志输出）
- 检查USB传输速度

### 问题2：触摸不工作
- 确认已退出扩展屏模式
- 检查触摸恢复日志

### 问题3：UI不显示
- 确认已退出扩展屏模式
- 检查UI恢复日志

