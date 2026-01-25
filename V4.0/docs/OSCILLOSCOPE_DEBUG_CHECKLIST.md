# 示波器调试检查清单

## 问题：显示 35V 和 2.7V（应该是 0-3.3V）

### 数据流追踪

```
ESP32 ADC (GPIO7, 12-bit)
    ↓ raw: 0-4095
osc_adc_raw_to_voltage()
    ↓ voltage = raw * 3.3 / 4095
    ↓ 应该输出: 0-3.3V
osc_adc_get_data()
    ↓ 填充 voltage buffer
calculate_measurements()
    ↓ 计算 vmax, vmin, vpp, vrms
UI 显示
```

### 检查点 1: ADC 转换函数

**位置**: `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_adc.c`

```c
float osc_adc_raw_to_voltage(uint16_t raw_value)
{
    // 应该是简单的线性转换
    float voltage = (float)raw_value * 3.3f / 4095.0f;
    return voltage;
}
```

**验证**:
- ✅ 无映射
- ✅ 无偏移
- ✅ 公式正确：raw * 3.3 / 4095

**测试**:
```c
// raw=0    → 0.00V
// raw=2048 → 1.65V
// raw=4095 → 3.30V
```

### 检查点 2: 测量计算初始化

**位置**: `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_core.c`

```c
// 初始化
float vmax = 0.0f;
float vmin = 0.0f;
bool first_sample = true;

// 循环中
if (first_sample) {
    vmax = v;
    vmin = v;
    first_sample = false;
}
```

**验证**:
- ✅ 初始值为 0
- ✅ 用第一个样本更新
- ✅ 无极端值（1000.0f）

### 检查点 3: 范围检查

**位置**: `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_core.c`

```c
const float MAX_VOLTAGE = 55.0f;   // +50V + 5V margin
const float MIN_VOLTAGE = -55.0f;  // -50V - 5V margin

if (vmax > MAX_VOLTAGE || vmin < MIN_VOLTAGE) {
    // 标记为无效
}
```

**验证**:
- ✅ 范围：-55V 到 +55V
- ✅ 0-3.3V 在范围内，不会被拒绝

### 检查点 4: 调试日志

**启用日志**:

1. **ADC 层** (`oscilloscope_adc.c`):
```
I (xxx) OscADC: ADC Data #50: count=10240, raw[0]=2048→1.650V, raw[mid]=2100→1.692V, Vmin=1.600V, Vmax=1.700V
```

2. **Core 层** (`oscilloscope_core.c`):
```
I (xxx) OscCore: Measurements: Vmax=1.70V, Vmin=1.60V, Vpp=0.10V, Vrms=1.65V, Freq=0.0Hz
```

3. **Draw 层** (`oscilloscope_draw.c`):
```
I (xxx) OscDraw: Drawing REAL ADC data: 10240 points, Vmin=1.600V, Vmax=1.700V, Vpp=0.100V
```

### 可能的问题来源

#### 1. 旧代码残留

**搜索极端初始值**:
```bash
grep -r "1000\.0f\|999\.0f\|-1000\.0f\|-999\.0f" BSP/GUIDER/
```

**已修复位置**:
- ✅ `oscilloscope_core.c` - calculate_measurements
- ✅ `oscilloscope_draw.c` - 调试日志
- ✅ `events_init.c` - 3 处

#### 2. 意外的映射或缩放

**搜索电压运算**:
```bash
grep -r "voltage\s*\*\|voltage\s*/\|voltage\s*+" BSP/GUIDER/custom/modules/oscilloscope/
```

**检查**:
- ✅ 无意外的乘法/除法
- ✅ 无偏移量添加

#### 3. 数据类型问题

**检查**:
- ✅ `uint16_t raw_value` (0-4095)
- ✅ `float voltage` (0-3.3)
- ✅ 无整数溢出

#### 4. 缓冲区问题

**检查**:
- ✅ `triggered_buffer` 存储原始 ADC 值
- ✅ 转换在 `osc_adc_get_data()` 中进行
- ✅ 无缓冲区覆盖

### 调试步骤

#### 步骤 1: 验证 ADC 原始值

在 `osc_adc_raw_to_voltage()` 中添加日志:

```c
float osc_adc_raw_to_voltage(uint16_t raw_value)
{
    float voltage = (float)raw_value * 3.3f / 4095.0f;
    
    // 临时调试
    static uint32_t call_count = 0;
    if (++call_count % 1000 == 0) {
        ESP_LOGI(TAG, "Convert: raw=%u → voltage=%.3fV", raw_value, voltage);
    }
    
    return voltage;
}
```

**预期输出**:
```
I (xxx) OscADC: Convert: raw=2048 → voltage=1.650V
```

#### 步骤 2: 验证测量计算

在 `calculate_measurements()` 中添加详细日志:

```c
// 在循环前
ESP_LOGI(TAG, "Calculating measurements: num_points=%lu", waveform->num_points);

// 在循环中（前几个样本）
if (i < 5) {
    ESP_LOGI(TAG, "  Sample[%lu]: v=%.3fV", i, v);
}

// 在循环后
ESP_LOGI(TAG, "Result: vmax=%.3fV, vmin=%.3fV (from %lu samples)", vmax, vmin, waveform->num_points);
```

#### 步骤 3: 检查 UI 显示代码

在 `events_init.c` 中查找显示测量值的代码:

```c
// 搜索
snprintf(buf, sizeof(buf), "Vmax: %.2fV", vmax);
```

**验证**:
- 使用的是 `vmax` 变量（来自 `osc_core_get_measurements()`）
- 无额外的缩放或转换

### 预期结果

**无信号（GPIO7 悬空）**:
```
Vmax: ~1.8V (不稳定)
Vmin: ~1.5V (不稳定)
Vpp: ~0.3V (噪声)
```

**接地（GPIO7 → GND）**:
```
Vmax: ~0.00V
Vmin: ~0.00V
Vpp: ~0.00V
```

**3.3V（GPIO7 → 3.3V）**:
```
Vmax: ~3.30V
Vmin: ~3.30V
Vpp: ~0.00V
```

### 如果仍然显示 35V

#### 可能性 1: 编译缓存问题

```bash
# 清理并重新编译
idf.py fullclean
idf.py build
idf.py flash
```

#### 可能性 2: 多个代码路径

检查是否有多个渲染路径:
- Canvas 渲染（新）
- Chart 渲染（旧）

确保两个路径都已修复。

#### 可能性 3: 宏定义问题

检查 `oscilloscope_adc.h`:

```c
#define OSC_DISPLAY_VOLTAGE_MIN     -50.0f
#define OSC_DISPLAY_VOLTAGE_MAX     50.0f
```

这些只是**规格定义**，不应该影响实际测量值。

### 最终验证

运行以下测试:

1. **串口日志检查**:
   - 查看 ADC 原始值（应该 0-4095）
   - 查看转换后电压（应该 0-3.3V）
   - 查看测量值（应该 0-3.3V）

2. **UI 显示检查**:
   - Vmax 应该 ≤ 3.3V
   - Vmin 应该 ≥ 0V
   - 不应该出现 35V 这样的值

3. **代码审查**:
   - 确认所有极端初始值已修复
   - 确认无意外的映射或缩放
   - 确认 ADC 转换函数正确

## 总结

如果显示 35V 和 2.7V，可能的原因：

1. ❌ 旧代码未完全修复（极端初始值残留）
2. ❌ 编译缓存问题（需要 fullclean）
3. ❌ 多个代码路径未全部修复
4. ❌ 意外的映射或缩放逻辑

**下一步**:
1. 查看串口日志，确认 ADC 原始值和转换后电压
2. 搜索所有 `1000.0f` 和 `-1000.0f`，确保全部修复
3. 清理编译缓存，重新编译
4. 如果问题仍然存在，提供完整的串口日志
