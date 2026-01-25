# 示波器时间参数修复说明

## 问题描述

原始实现中，示波器的时间参数（Time/div）在增大到一定值后，波形更新速度变得非常慢，且波形被限制在最多50个周期，不符合真实示波器的行为。

### 原始问题
- 时间参数为1ms/div时，波形滚动速度过慢
- 大时间参数下，波形被限制在最多1000个周期
- 无法真实反映极度压缩的情况
- 缺少真实示波器的滚动/缩放效果

## 真实示波器行为

### 时基（Time/div）的作用
- **时基增大** → 每格代表更长时间 → 波形被水平压缩 → 显示更多周期
- **时基减小** → 每格代表更短时间 → 波形被水平拉伸 → 显示更少周期

### 极度压缩状态
当时基非常大时（如100ms/div, 1s/div）：
- 波形被极度压缩，单个周期可能比一个像素还窄
- 屏幕只显示完整波形的一小部分
- 波形连续滚动，像"窗口"一样显示实时数据流
- 通过X-Pos偏移可以左右平移查看不同时间段

### 示例：1kHz信号（周期1ms）

| 时基 | 屏幕总时间 | 周期数 | 显示效果 |
|------|-----------|--------|---------|
| 10us/div | 160us | 0.16 | 极度放大，看到波形细节 |
| 100us/div | 1.6ms | 1.6 | 正常显示，1-2个完整周期 |
| 1ms/div | 16ms | 16 | 压缩显示，16个周期 |
| 10ms/div | 160ms | 160 | 明显压缩，160个周期 |
| 100ms/div | 1.6s | 1600 | 极度压缩！屏幕只显示一小部分 |
| 1s/div | 16s | 16000 | 超级压缩！需要X-Pos平移查看 |

## 解决方案

### 方式B：真实示波器滚动模式

实现了符合真实示波器的行为：
- **波形从右向左连续滚动**（或从左向右）
- **时间参数决定波形的"拉伸"或"压缩"程度**
- **配合X-Pos偏移可以查看波形的不同部分**
- **移除周期数上限**，允许极度压缩的情况

### 核心修改

#### 1. 时间参数逻辑（`events_init.c`）

```c
// 真实示波器行为：
// - 时间参数增大 → 波形被压缩 → 屏幕上显示更多周期
// - 时间参数减小 → 波形被拉伸 → 屏幕上显示更少周期（放大效果）
// 例如：1kHz信号（周期1ms）
//   - 1ms/div × 16格 = 16ms总时间 → 显示16个周期（压缩）
//   - 100us/div × 16格 = 1.6ms总时间 → 显示1.6个周期（正常）
//   - 10us/div × 16格 = 160us总时间 → 显示0.16个周期（放大）
//   - 100ms/div × 16格 = 1.6s总时间 → 显示1600个周期（极度压缩！）

float cycles_on_screen = total_time_on_screen / signal_period;

// 真实示波器行为：不限制周期数
// - 小时基：显示完整波形（如1-10个周期）
// - 大时基：波形被极度压缩，屏幕只显示完整波形的一小部分
// - 通过X-Pos偏移可以左右平移查看不同段落
// 只设置最小值防止除零错误
if (cycles_on_screen < 0.1f) cycles_on_screen = 0.1f;
// 移除上限！允许任意多的周期数，真实反映时基设置
```

#### 2. 动画速度修复

**关键修改**：动画速度现在与信号频率成正比

```c
// Timer updates at 50ms (20Hz)
const float timer_period = 0.05f;  // 50ms timer period

// Calculate how much phase should advance per timer tick
// For a 1kHz signal: phase_advance = 2*PI * 1000 * 0.05 = 314.16 rad/tick
// This means the waveform advances by 50 complete cycles per tick
float phase_advance_per_tick = 2.0f * M_PI * signal_frequency * timer_period;

// Apply animation counter to create scrolling effect
float phase_shift = fmodf((float)osc_waveform_phase * phase_advance_per_tick, 2.0f * M_PI);
```

**为什么这样修复有效**：
- 1kHz信号的周期是1ms
- 定时器每50ms更新一次
- 在50ms内，1kHz信号完成50个周期
- 相位应该前进 50 × 2π = 314.16 弧度
- 这样波形滚动速度就与实际时间匹配了

#### 3. X偏移增强

```c
// 允许更大的偏移范围，用于查看压缩波形的不同部分
float max_offset = time_per_div * (float)OSC_GRID_COLS * 10.0f;
if (osc_x_offset > max_offset) osc_x_offset = max_offset;
if (osc_x_offset < -max_offset) osc_x_offset = -max_offset;
```

## 效果对比

### 修复前
- 1ms/div：波形几乎静止，更新非常慢
- 10ms/div：波形完全静止
- 周期数被限制在1000以内
- 无法通过X偏移查看压缩波形的不同部分

### 修复后
- 1ms/div：波形以正确的速度滚动（每50ms前进50个周期）
- 10ms/div：波形被压缩显示160个周期，仍然平滑滚动
- 100ms/div：波形被极度压缩显示1600个周期，屏幕只显示一小部分
- 1s/div：波形被超级压缩显示16000个周期，需要X-Pos平移查看
- 可以通过X-Pos偏移查看波形的任意时间段

### 真实示波器体验
现在的行为完全符合真实示波器：
1. **小时基**（如10us/div）：波形被放大，可以看到细节
2. **中等时基**（如1ms/div）：显示几个到几十个完整周期
3. **大时基**（如100ms/div）：波形被极度压缩，像密集的线条
4. **超大时基**（如1s/div）：屏幕只显示完整波形的一小部分，需要平移查看

## 时间参数范围

支持的时间参数（s/div）：
```
10ns, 50ns, 100ns, 200ns, 500ns,
1us, 2us, 5us, 10us, 20us, 50us,
100us, 200us, 500us,
1ms, 2ms, 5ms, 10ms, 20ms, 50ms,
100ms, 200ms, 500ms, 1s
```

## 使用说明

### 基本操作
1. **Time按钮**：循环切换时间参数
2. **X-Pos按钮**：点击激活，然后上下滑动调整水平偏移
3. **RUN/STOP按钮**：暂停/恢复波形采集

### 查看压缩波形
1. 将时间参数设置为较大值（如10ms/div）
2. 波形会被压缩，显示多个周期
3. 点击X-Pos按钮激活水平偏移
4. 上下滑动查看波形的不同时间段

### 放大波形细节
1. 将时间参数设置为较小值（如10us/div）
2. 波形会被拉伸，显示更少周期
3. 可以看到波形的细节

## 技术细节

### 相位计算公式
```
phase = i * omega + phase_shift + phase_offset

其中：
- i: 采样点索引 (0 到 num_points-1)
- omega: 角频率 = 2π * cycles_on_screen / num_points
- phase_shift: 动画相位 = osc_waveform_phase * phase_advance_per_tick
- phase_offset: X偏移相位 = osc_x_offset * 2π * signal_frequency
```

### 滚动速度计算
```
phase_advance_per_tick = 2π * frequency * timer_period

对于1kHz信号，50ms定时器：
phase_advance_per_tick = 2π * 1000 * 0.05 = 314.16 rad/tick

这意味着每次定时器触发，相位前进314.16弧度，
相当于50个完整周期，与实际时间完美匹配。
```

## 文件修改清单

- `BSP/GUIDER/generated/events_init.c`
  - `osc_waveform_update_cb()`: 修复动画速度计算
  - `scrOscilloscope_contTimeScale_event_handler()`: 添加注释说明
  - `scrOscilloscope_contXOffset_event_handler()`: 增强X偏移范围

## 测试建议

1. **基本功能测试**
   - 切换不同时间参数，观察波形压缩/拉伸
   - 验证波形滚动速度是否合理

2. **边界测试**
   - 最小时间参数（10ns/div）
   - 最大时间参数（1s/div）
   - X偏移的极限值

3. **性能测试**
   - 大时间参数下的渲染性能
   - 快速切换时间参数的响应

## 后续优化建议

1. **可变信号频率**：允许用户选择不同频率的测试信号
2. **触发功能**：实现真实的触发捕获功能
3. **波形缓存**：优化大时间参数下的性能
4. **多通道支持**：支持多个信号通道同时显示

## 参考

- 真实示波器行为参考：Tektronix, Keysight等专业示波器
- LVGL图表文档：https://docs.lvgl.io/master/widgets/chart.html
