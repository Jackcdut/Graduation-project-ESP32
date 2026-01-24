# 示波器测试模式配置

## 概述

当前示波器处于**测试模式**，使用 ESP32-P4 内置 ADC 直接采样 0-3.3V 信号。这是一个临时配置，用于验证示波器软件逻辑。

**重要说明：**
- 测试模式：ADC 范围 0-3.3V
- 生产模式：将使用外部 ADC 硬件，支持 -50V 到 +50V 范围
- ESP32 ADC 仅用于软件开发和测试

## 测试模式配置

### ADC 硬件配置

```c
// oscilloscope_adc.h
#define OSC_ADC_UNIT            ADC_UNIT_1
#define OSC_ADC_CHANNEL         ADC_CHANNEL_6  // GPIO7
#define OSC_ADC_ATTEN           ADC_ATTEN_DB_12  // 0-3.3V range
#define OSC_ADC_BITWIDTH        ADC_BITWIDTH_12  // 12-bit (0-4095)
```

### 电压范围

```c
// oscilloscope_adc.h
#define OSC_DISPLAY_VOLTAGE_MIN     0.0f    // 最小电压 0V
#define OSC_DISPLAY_VOLTAGE_MAX     3.3f    // 最大电压 3.3V
#define OSC_DISPLAY_VOLTAGE_CENTER  1.65f   // 中心电压 1.65V
```

### ADC 转换函数（简化版）

```c
// oscilloscope_adc.c
float osc_adc_raw_to_voltage(uint16_t raw_value)
{
    // 直接线性转换：0-4095 → 0-3.3V
    float voltage = (float)raw_value * 3.3f / 4095.0f;
    return voltage;
}
```

**转换表：**

| ADC Raw | 电压 (V) | 说明 |
|---------|----------|------|
| 0       | 0.00     | 最小值 |
| 1024    | 0.825    | 1/4 点 |
| 2048    | 1.65     | 中点 |
| 3072    | 2.475    | 3/4 点 |
| 4095    | 3.30     | 最大值 |

### 测量值范围检查

```c
// oscilloscope_core.c
const float MAX_VOLTAGE = 3.5f;   // 3.3V + 0.2V 容差
const float MIN_VOLTAGE = -0.2f;  // 0V - 0.2V 容差（允许小负噪声）

if (vmax > MAX_VOLTAGE || vmin < MIN_VOLTAGE) {
    // 标记为无效，显示 "---"
    measurements_valid = false;
}
```

## 测试方法

### 1. 无信号测试（悬空）

**预期结果：**
- ADC 读数不稳定（浮空状态）
- 电压可能在 0-3.3V 范围内跳动
- 这是正常现象

**验证：**
```
Vmax: 1.8V (示例)
Vmin: 1.5V (示例)
Vpp: 0.3V (噪声)
```

### 2. 接地测试（GND）

**连接：** GPIO7 接 GND

**预期结果：**
```
Vmax: ~0.00V
Vmin: ~0.00V
Vpp: ~0.00V
Vrms: ~0.00V
```

### 3. 3.3V 测试

**连接：** GPIO7 接 3.3V

**预期结果：**
```
Vmax: ~3.30V
Vmin: ~3.30V
Vpp: ~0.00V
Vrms: ~3.30V
```

### 4. 信号发生器测试

**连接：** GPIO7 接信号发生器输出（0-3.3V 范围内）

**示例：1kHz 正弦波，1.65V 偏置，±1V 幅度**

**预期结果：**
```
Vmax: ~2.65V (1.65 + 1.0)
Vmin: ~0.65V (1.65 - 1.0)
Vpp: ~2.00V
Vrms: ~1.65V (近似)
Freq: ~1000Hz
```

## 常见问题

### Q1: 为什么显示 "---"？

**可能原因：**
1. 没有连接信号（ADC 悬空）
2. 电压超出 0-3.3V 范围
3. ADC 采样未启动

**解决：**
- 检查 GPIO7 连接
- 确保信号在 0-3.3V 范围内
- 查看串口日志确认 ADC 状态

### Q2: 电压读数不稳定

**原因：**
- GPIO7 悬空（没有连接）
- 信号源阻抗太高
- 噪声干扰

**解决：**
- 连接稳定的信号源
- 使用接地测试验证
- 添加去耦电容（如果需要）

### Q3: 显示的电压不准确

**检查：**
1. ADC 校准（ESP32 ADC 精度有限）
2. 参考电压是否准确（应该是 3.3V）
3. 转换公式是否正确

**验证方法：**
```c
// 打印原始 ADC 值和转换后电压
ESP_LOGI(TAG, "Raw: %u, Voltage: %.3fV", raw_value, voltage);
```

### Q4: 测量值显示 1000V 异常值

**已修复！** 这是初始化问题，现在已经修复：
- ✅ 所有初始值改为 0
- ✅ 用第一个真实样本更新
- ✅ 添加范围检查（0-3.3V）
- ✅ 超出范围显示 "---"

## 生产模式迁移计划

### 未来配置（外部 ADC）

```c
// 生产模式配置（示例）
#define OSC_DISPLAY_VOLTAGE_MIN     -50.0f   // -50V
#define OSC_DISPLAY_VOLTAGE_MAX     50.0f    // +50V
#define OSC_DISPLAY_VOLTAGE_CENTER  0.0f     // 0V

// 外部 ADC 转换函数（需要实现）
float external_adc_to_voltage(uint16_t raw_value) {
    // 根据外部 ADC 规格实现
    // 例如：16-bit ADC，±50V 范围
    float voltage = ((float)raw_value - 32768.0f) * 100.0f / 65535.0f;
    return voltage;
}
```

### 迁移步骤

1. **硬件准备**
   - 选择合适的外部 ADC（如 ADS1256, AD7606 等）
   - 设计电压调理电路（±50V → ADC 输入范围）
   - 添加保护电路

2. **软件修改**
   - 更新 `oscilloscope_adc.h` 中的电压范围常量
   - 实现新的 ADC 驱动（SPI/I2C 通信）
   - 修改 `osc_adc_raw_to_voltage()` 转换函数
   - 更新范围检查逻辑

3. **测试验证**
   - 校准外部 ADC
   - 验证电压测量精度
   - 测试全范围（-50V 到 +50V）
   - 验证保护电路

## 调试日志

### 启用详细日志

```c
// oscilloscope_adc.c
ESP_LOGI(TAG, "ADC Data: raw[0]=%u, V[0]=%.3fV", raw_value, voltage);

// oscilloscope_core.c
ESP_LOGI(TAG, "Measurements: Vmax=%.2fV, Vmin=%.2fV, Vpp=%.2fV", vmax, vmin, vpp);
```

### 日志示例（正常）

```
I (12345) OscADC: ADC sampling started: rate=100000 Hz
I (12346) OscADC: Auto-trigger: 10000 samples (100.0ms @ 100000 Hz)
I (12450) OscADC: Trigger #1: voltage=1.650V, auto=1
I (12451) OscADC: ADC Data #1: count=10240, raw[0]=2048, V[0]=1.650V
I (12452) OscCore: Measurements: Vmax=2.50V, Vmin=0.80V, Vpp=1.70V, Vrms=1.65V, Freq=1000.0Hz
```

### 日志示例（异常）

```
W (12453) OscCore: Measurements out of range: Vmax=5.00V, Vmin=-1.00V (ADC range: 0-3.3V) - marking invalid
```

## 总结

- ✅ 测试模式：0-3.3V，使用 ESP32 ADC
- ✅ 简化转换：直接线性映射
- ✅ 范围检查：0-3.3V ± 0.2V 容差
- ✅ 初始值修复：所有初始值为 0
- ⏳ 生产模式：将使用外部 ADC，-50V 到 +50V

当前配置适合软件开发和逻辑验证，不适合实际测量应用。
