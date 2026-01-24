# 示波器性能优化快速测试指南

## 快速开始

### 1. 编译项目
```bash
idf.py build
```

### 2. 烧录到设备
```bash
idf.py flash monitor
```

### 3. 进入示波器界面
在设备上导航到示波器功能

### 4. 观察FPS日志
在串口监视器中查看：
```
I (xxxxx) OscDraw: FPS: 75.3
```

---

## 预期结果

### 成功标志
✅ 看到 "PPA hardware acceleration enabled" 日志
✅ FPS显示在 60-80 之间
✅ 波形滚动流畅，无卡顿
✅ 触摸响应灵敏

### 失败标志
❌ 看到 "Failed to initialize hardware acceleration" 警告
❌ FPS低于40
❌ 波形滚动卡顿
❌ 内存分配失败

---

## 性能测试

### 测试1: 基本波形显示
1. 进入示波器界面
2. 观察1kHz正弦波
3. 检查FPS是否 > 60

**预期**: FPS 70-80

### 测试2: 时间参数切换
1. 点击 "Time" 按钮切换时基
2. 观察波形是否流畅变化
3. 检查FPS是否稳定

**预期**: FPS保持 > 60

### 测试3: FFT模式
1. 点击 "FFT" 按钮
2. 观察频谱显示
3. 检查FPS

**预期**: FPS 60-70（略低于时域）

### 测试4: 参数调整
1. 调整 X-Pos（水平偏移）
2. 调整 Y-Pos（垂直偏移）
3. 调整 Volt（电压档位）
4. 观察响应速度

**预期**: 实时响应，无延迟

---

## 日志分析

### 正常日志示例
```
I (12345) OscDraw: Sin lookup table initialized (256 entries)
I (12346) OscDraw: Canvas buffer allocated: 524288 bytes in PSRAM
I (12347) OscDraw: PPA hardware acceleration enabled
I (12348) OscDraw: Oscilloscope drawing context initialized successfully
I (12349) OSC: Hardware-accelerated drawing initialized (FPS monitoring enabled)
I (13000) OscDraw: FPS: 75.3
I (14000) OscDraw: FPS: 78.1
I (15000) OscDraw: FPS: 76.8
```

### 降级日志示例
```
W (12345) OscDraw: PPA hardware acceleration not available
W (12346) OSC: Failed to initialize hardware acceleration, falling back to chart
```

### 错误日志示例
```
E (12345) OscDraw: Failed to allocate canvas buffer (524288 bytes)
E (12346) OSC: Failed to initialize hardware acceleration, falling back to chart
```

---

## 性能基准

| 场景 | 目标FPS | 可接受FPS | 不可接受FPS |
|------|---------|-----------|-------------|
| 时域波形 | 70-80 | 60-70 | < 50 |
| FFT频谱 | 60-70 | 50-60 | < 40 |
| 参数调整 | 70-80 | 60-70 | < 50 |
| STOP模式 | N/A | N/A | N/A |

---

## 故障排除

### FPS < 40
1. 检查CPU频率: `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ`
2. 检查LVGL配置: `CONFIG_LV_DISP_DEF_REFR_PERIOD`
3. 检查IRAM优化: `CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM`
4. 尝试降低定时器周期到16ms

### 内存分配失败
1. 检查PSRAM配置: `CONFIG_SPIRAM=y`
2. 检查可用内存: `esp_get_free_heap_size()`
3. 减少其他模块的内存使用

### 硬件加速不可用
1. 检查芯片型号是否为ESP32-P4
2. 检查PPA配置: `CONFIG_SOC_PPA_SUPPORTED`
3. 降级到软件绘制（自动）

---

## 性能对比测试

### 测试方法
1. 记录优化前的FPS（应该是20左右）
2. 应用优化
3. 记录优化后的FPS
4. 计算提升倍数

### 记录表格
| 测试项 | 优化前FPS | 优化后FPS | 提升倍数 |
|--------|-----------|-----------|----------|
| 时域波形 | 20 | ___ | ___ |
| FFT频谱 | 18 | ___ | ___ |
| 参数调整 | 20 | ___ | ___ |

---

## 下一步

如果测试通过：
✅ 性能优化成功！
✅ 可以继续使用和开发

如果测试失败：
1. 查看日志中的错误信息
2. 参考故障排除章节
3. 检查硬件配置
4. 联系技术支持

---

## 相关文档

- [优化总结](OSCILLOSCOPE_OPTIMIZATION_SUMMARY.md)
- [详细优化说明](OSCILLOSCOPE_PERFORMANCE_OPTIMIZATION.md)
- [模块README](../BSP/GUIDER/custom/modules/oscilloscope/README.md)
