# 示波器测量值异常修复

## 问题描述

示波器统计信息显示异常值：
- 电压最大值显示 1000+ V
- 电压最小值显示异常值（如 32V 或 -1000V）
- 这些值远超实际测量范围（-50V 到 +50V）

## 根本原因

### 1. 测量计算函数初始化问题

在 `oscilloscope_core.c` 的 `calculate_measurements()` 函数中：

```c
// 旧代码 - 有问题
float vmax = -1000.0f;  // 极端初始值
float vmin = 1000.0f;   // 极端初始值
```

**问题**：
- 如果没有有效数据或数据处理失败，这些极端初始值会被直接返回
- 导致 UI 显示 "Vmax: 1000.00V" 和 "Vmin: -1000.00V"

### 2. Fallback 图表渲染代码的相同问题

在 `events_init.c` 的旧图表渲染代码中（第 3377 行）：

```c
// 旧代码 - 同样的问题
float min_voltage = 999.0f;
float max_voltage = -999.0f;
```

**问题**：
- 这是 fallback 代码路径，当新的 canvas 渲染不可用时使用
- 使用了相同的极端初始值问题
- 没有经过 `calculate_measurements()` 的修复

### 3. 缺少数据有效性检查

原代码只检查 `num_points == 0`，但没有检查：
- 数据是否在合理范围内
- 数据是否有变化（全是相同值）
- 数据是否包含异常值

### 4. 触发配置不一致

两个地方设置触发器，配置冲突：
- `oscilloscope_core.c`: `trigger.enabled = true` (正常触发)
- `oscilloscope_adc.c`: `trigger.enabled = false` (自动触发)

### 5. 自动触发间隔过长

```c
const uint32_t AUTO_TRIGGER_SAMPLES = 5000;  // 固定 5000 样本
```

**问题**：
- 在低采样率（如 1kHz）下，5000 样本 = 5 秒才触发一次
- 导致波形更新非常慢，用户体验差

## 修复方案

### 2. 改进测量计算逻辑（两处）

**oscilloscope_core.c - calculate_measurements():**

```c
// 新代码 - 使用第一个样本值初始化
float vmax = waveform->voltage_data[0];
float vmin = waveform->voltage_data[0];
```

**events_init.c - fallback 图表渲染代码:**

```c
// 新代码 - 使用第一个样本值初始化
float min_voltage = display_buffer[0];
float max_voltage = display_buffer[0];
```

**添加数据有效性检查（示波器测量范围：-50V 到 +50V）:**

```c
const float MAX_VOLTAGE = 55.0f;   // +50V + 5V margin
const float MIN_VOLTAGE = -55.0f;  // -50V - 5V margin

if (vmax > MAX_VOLTAGE || vmin < MIN_VOLTAGE) {
    // 数据超出示波器测量范围，标记为无效
    measurements_valid = false;
    // 显示 "---" 而不是异常值
}
```

### 2. 改进错误处理

```c
// 在 osc_core_get_measurements() 中
// 返回 ESP_ERR_NOT_FOUND 如果测量无效
return ctx->measurements_valid ? ESP_OK : ESP_ERR_NOT_FOUND;
```

### 3. UI 层显示 "---" 当测量无效

```c
if (osc_core_get_measurements(...) == ESP_OK) {
    // 显示有效测量值
    snprintf(buf, sizeof(buf), "Vmax: %.2fV", vmax);
} else {
    // 显示占位符
    lv_label_set_text(label, "Vmax: ---");
}
```

### 4. 统一触发配置

```c
// oscilloscope_core.c - 默认使用自动触发
ctx->trigger.enabled = false;  // false = AUTO 模式
```

### 5. 动态调整自动触发间隔

```c
// 根据采样率动态计算，目标 100ms 刷新率
uint32_t auto_trigger_samples = ctx->sample_rate_hz / 10;  // 100ms
if (auto_trigger_samples < 100) auto_trigger_samples = 100;
if (auto_trigger_samples > 10000) auto_trigger_samples = 10000;
```

**效果**：
- 1 kHz 采样率：100 样本 = 100ms 触发间隔
- 100 kHz 采样率：10000 样本 = 100ms 触发间隔
- 1 MHz 采样率：10000 样本 = 10ms 触发间隔（限制最大值）

## 修改文件

1. **BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_core.c**
   - 改进 `calculate_measurements()` 函数
   - 使用第一个样本值初始化 vmax/vmin
   - 添加数据有效性检查（-55V 到 +55V）
   - 添加合理范围检查
   - 修改 `osc_core_get_measurements()` 返回值

2. **BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_adc.c**
   - 动态计算自动触发间隔
   - 添加触发间隔日志

3. **BSP/GUIDER/generated/events_init.c**
   - 修复 fallback 图表渲染代码的初始化问题（第 3377 行）
   - 使用第一个样本值初始化 min_voltage/max_voltage
   - 添加范围检查和有效性标志
   - 添加测量无效时的 UI 处理
   - 显示 "---" 占位符而不是异常值

## 测试验证

### 正常情况
- 有效信号输入：显示正确的 Vmax、Vmin、Vpp、Vrms
- 值在示波器测量范围内（**-50V 到 +50V**）
- 允许 ±5V 的测量容差（-55V 到 +55V）

### 异常情况
- 无信号输入：显示 "Vmax: ---", "Vmin: ---" 等
- 数据超出范围（> 55V 或 < -55V）：标记为无效，显示 "---"
- 数据异常：不显示错误的 1000V 值
- 自动触发：每 100ms 左右更新一次波形

## 触发模式说明

### AUTO 模式（默认）
- `trigger.enabled = false`
- 自动周期性触发，无需等待特定电压条件
- 适合查看任意信号
- 刷新率：约 10 Hz (100ms 间隔)

### NORMAL 模式
- `trigger.enabled = true`
- 等待信号满足触发条件（电平 + 边沿）
- 适合稳定显示周期信号
- 需要用户设置触发电平

## 后续改进建议

1. **添加触发模式切换按钮**
   - 让用户可以在 AUTO 和 NORMAL 模式间切换

2. **添加触发电平调节**
   - UI 控件调节触发电平
   - 显示触发电平线

3. **改进测量算法**
   - 使用更精确的频率测量（FFT 或周期计数）
   - 添加上升时间、下降时间等参数

4. **添加测量有效性指示**
   - 显示图标或颜色表示测量是否可靠
   - 提示用户调整设置

## 相关文档

- [示波器时间刻度修复](OSCILLOSCOPE_TIME_SCALE_FIX.md)
- [示波器性能优化](OSCILLOSCOPE_PERFORMANCE_OPTIMIZATION.md)
- [示波器 FFT 修复](OSCILLOSCOPE_FFT_FIX.md)
