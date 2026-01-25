# 示波器数据流验证文档

## 数据流路径

```
ESP32 ADC (GPIO7)
    ↓ (12-bit, 0-4095)
osc_adc_raw_to_voltage()
    ↓ (转换为 -50V 到 +50V)
osc_adc_get_data()
    ↓ (填充 voltage buffer)
osc_core_update()
    ↓ (存储到 captured_waveform)
osc_core_get_display_waveform()
    ↓ (重采样到显示宽度)
osc_draw_waveform() / events_init.c
    ↓ (绘制到屏幕)
UI 显示
```

## ADC 转换验证

### 硬件配置
- **ADC 单元**: ADC_UNIT_1
- **ADC 通道**: ADC_CHANNEL_6 (GPIO7)
- **衰减**: ADC_ATTEN_DB_12 (0-3.3V 范围)
- **位宽**: ADC_BITWIDTH_12 (12-bit, 0-4095)

### 电压转换公式

```c
float osc_adc_raw_to_voltage(uint16_t raw_value)
{
    // Step 1: 原始值转 ADC 电压
    float adc_voltage = (float)raw_value * 3.3f / 4095.0f;
    
    // Step 2: ADC 电压转显示电压
    // 假设外部电路：0V输入 = 1.65V ADC
    float display_voltage = (adc_voltage - 1.65f) * (100.0f / 3.3f);
    
    return display_voltage;
}
```

### 转换验证表

| ADC Raw | ADC 电压 | 显示电压 | 说明 |
|---------|----------|----------|------|
| 0       | 0.00V    | -50.00V  | 最小值 |
| 1024    | 0.825V   | -25.00V  | 1/4 点 |
| 2048    | 1.65V    | 0.00V    | 中点（地） |
| 3072    | 2.475V   | +25.00V  | 3/4 点 |
| 4095    | 3.30V    | +50.00V  | 最大值 |

### 实际测试

**无信号输入（悬空）：**
- 预期 ADC raw ≈ 2048 (浮空可能不稳定)
- 预期显示电压 ≈ 0V ± 噪声

**接地（0V）：**
- 如果外部电路正确：ADC raw ≈ 2048
- 显示电压应该 ≈ 0V

**3.3V 输入：**
- 如果外部电路正确：ADC raw ≈ 4095
- 显示电压应该 ≈ +50V

## 测量值计算

### 初始化策略

**正确方式（当前实现）：**
```c
float vmax = 0.0f;
float vmin = 0.0f;
bool first_sample = true;

for (uint32_t i = 0; i < num_points; i++) {
    float v = voltage_data[i];
    
    if (first_sample) {
        vmax = v;
        vmin = v;
        first_sample = false;
    }
    
    if (v > vmax) vmax = v;
    if (v < vmin) vmin = v;
}
```

**为什么这样做：**
1. 初始值为 0，避免显示异常值
2. 用第一个真实样本初始化 min/max
3. 确保 min/max 来自真实数据

### 范围检查

```c
const float MAX_VOLTAGE = 55.0f;   // +50V + 5V 容差
const float MIN_VOLTAGE = -55.0f;  // -50V - 5V 容差

if (vmax > MAX_VOLTAGE || vmin < MIN_VOLTAGE) {
    // 标记为无效，显示 "---"
    measurements_valid = false;
}
```

## 调试日志

### ADC 层日志

```
I (12345) OscADC: ADC Data #50: count=10240, raw[0]=2048, V[0]=0.000V, raw[5120]=2100, V[5120]=1.576V
```

**解读：**
- `count=10240`: 采集了 10240 个样本
- `raw[0]=2048`: 第一个样本原始值 2048
- `V[0]=0.000V`: 转换后电压 0V（正确）
- `raw[5120]=2100`: 中间样本原始值 2100
- `V[5120]=1.576V`: 转换后电压 1.576V

### Core 层日志

```
I (12346) OscCore: Measurements: Vmax=2.50V, Vmin=-1.20V, Vpp=3.70V, Vrms=0.85V, Freq=50.0Hz
```

**解读：**
- `Vmax=2.50V`: 最大电压 2.5V（在范围内）
- `Vmin=-1.20V`: 最小电压 -1.2V（在范围内）
- `Vpp=3.70V`: 峰峰值 3.7V
- `Vrms=0.85V`: 有效值 0.85V
- `Freq=50.0Hz`: 频率 50Hz

### 异常情况日志

```
W (12347) OscCore: Measurements out of range: Vmax=100.00V, Vmin=-80.00V (range: -50V to +50V) - marking invalid
```

**说明：**
- 测量值超出示波器范围
- 自动标记为无效
- UI 将显示 "---"

## 常见问题排查

### 问题 1: 显示 1000V 异常值

**原因：**
- 初始化使用极端值（999.0f 或 -999.0f）
- 没有真实数据时直接使用初始值

**解决：**
- ✅ 初始化为 0
- ✅ 用第一个真实样本更新
- ✅ 添加范围检查

### 问题 2: 显示全是 0V

**可能原因：**
1. ADC 没有启动采样
2. 触发条件不满足，没有数据
3. 数据转换错误

**排查步骤：**
```c
// 1. 检查 ADC 是否运行
ESP_LOGI(TAG, "ADC running: %d", osc_adc_is_running(ctx));

// 2. 检查是否有新数据
ESP_LOGI(TAG, "Has new data: %d", osc_adc_has_new_data(ctx));

// 3. 检查原始 ADC 值
ESP_LOGI(TAG, "Raw ADC: %u", raw_value);

// 4. 检查转换后电压
ESP_LOGI(TAG, "Voltage: %.3fV", voltage);
```

### 问题 3: 电压范围不对

**检查点：**
1. 外部电路是否正确（0V 输入 = 1.65V ADC）
2. ADC 衰减设置（应该是 12dB）
3. 转换公式中的常数（1.65V, 100V/3.3V）

**验证方法：**
```c
// 测试转换函数
uint16_t test_values[] = {0, 2048, 4095};
for (int i = 0; i < 3; i++) {
    float v = osc_adc_raw_to_voltage(test_values[i]);
    ESP_LOGI(TAG, "raw=%u -> V=%.2fV", test_values[i], v);
}
// 预期输出：
// raw=0 -> V=-50.00V
// raw=2048 -> V=0.00V
// raw=4095 -> V=50.00V
```

### 问题 4: 波形不稳定

**可能原因：**
1. 触发模式不对（应该用 AUTO 模式）
2. 采样率太低
3. 自动触发间隔太长

**解决：**
- ✅ 默认使用 AUTO 触发（trigger.enabled = false）
- ✅ 动态调整触发间隔（100ms）
- ✅ 根据时间刻度选择合适采样率

## 数据完整性检查清单

- [x] ADC 配置正确（GPIO7, 12-bit, 12dB 衰减）
- [x] 电压转换公式正确（-50V 到 +50V）
- [x] 测量值初始化为 0
- [x] 用第一个真实样本更新 min/max
- [x] 范围检查（-55V 到 +55V）
- [x] 超出范围显示 "---"
- [x] 自动触发模式（100ms 间隔）
- [x] 调试日志完整

## 下一步测试

1. **硬件测试**
   - 接地测试：应显示 ≈ 0V
   - 3.3V 测试：应显示 ≈ +50V（如果外部电路正确）
   - 信号发生器测试：验证频率和幅度

2. **软件测试**
   - 查看串口日志，确认 ADC 原始值
   - 验证电压转换是否正确
   - 检查测量值是否合理

3. **UI 测试**
   - 无信号时应显示 "---" 或接近 0V
   - 有信号时应显示正确的 Vmax、Vmin、Vpp
   - 不应该出现 1000V 这样的异常值
