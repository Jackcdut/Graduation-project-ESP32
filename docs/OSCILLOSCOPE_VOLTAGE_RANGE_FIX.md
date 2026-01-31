# 示波器电压范围修复文档

## 问题描述

原始实现存在以下问题：
1. ADC电压范围仅为0-3.3V，但显示范围应该是-50V到+50V
2. 坐标轴显示不正确，没有正确处理负电压
3. 波形绘制时，横坐标轴（0V参考线）应该在屏幕中心
4. 电压统计和测量逻辑没有考虑负电压

## 解决方案

### 1. 电压映射 (oscilloscope_adc.c/h)

**新增宏定义：**
```c
#define OSC_ADC_VOLTAGE_MIN     -50.0f  // 最小电压范围 (-50V)
#define OSC_ADC_VOLTAGE_MAX     50.0f   // 最大电压范围 (+50V)
#define OSC_ADC_VOLTAGE_RANGE   100.0f  // 总电压范围 (100V)
#define OSC_ADC_MIDPOINT        1.65f   // ADC中点电压 (3.3V / 2)
```

**电压转换函数修改：**
```c
float osc_adc_raw_to_voltage(uint16_t raw_value)
{
    // 12位ADC: 0-4095 映射到 0-3.3V
    float adc_voltage = (float)raw_value * 3.3f / 4095.0f;
    
    // 线性映射: V_actual = (V_adc - 1.65) * (100 / 3.3)
    // V_adc = 0V    -> V_actual = -50V
    // V_adc = 1.65V -> V_actual = 0V
    // V_adc = 3.3V  -> V_actual = +50V
    float actual_voltage = (adc_voltage - OSC_ADC_MIDPOINT) * (OSC_ADC_VOLTAGE_RANGE / 3.3f);
    
    return actual_voltage;
}
```

### 2. 坐标系统 (oscilloscope_draw.c)

**坐标系定义：**
- Y=0 (顶部) 对应最大正电压
- Y=center (OSC_CANVAS_HEIGHT/2) 对应 0V（横坐标轴/地线）
- Y=OSC_CANVAS_HEIGHT (底部) 对应最大负电压

**绘制逻辑：**
```c
// 电压转Y坐标
// 正电压 -> 中心线上方（Y值较小）
// 负电压 -> 中心线下方（Y值较大）
float y_float = chart_center - (voltage * units_per_volt);
```

### 3. 测量统计修复 (oscilloscope_core.c)

**修改内容：**
- 正确初始化vmax和vmin（vmax从-1000开始，vmin从+1000开始）
- 添加调试日志输出测量结果
- 确保RMS计算正确处理负电压

### 4. 自动调整修复 (oscilloscope_core.c)

**Y轴偏移计算：**
```c
// 信号中心应该在0V（屏幕中心）
// Y_offset = -vcenter 将信号中心移到屏幕中心
ctx->y_offset = -vcenter;
```

### 5. 触发电平默认值

**修改：**
- 从 1.65V（ADC中点）改为 0V（实际电压中点）
- 这样触发电平默认在-50V到+50V范围的中心

## 硬件要求

**注意：** 此实现假设存在外部电压调理电路：

```
输入信号 (-50V ~ +50V)
    ↓
[电压分压器 + 偏置电路]
    ↓
ADC输入 (0V ~ 3.3V)
```

**电路设计要点：**
1. 电压分压：将±50V范围压缩到3.3V范围
2. 电平偏移：将-50V~+50V偏移到0~3.3V
3. 保护电路：防止超压损坏ADC

**转换公式：**
```
V_adc = (V_input + 50) * 3.3 / 100
```

**示例：**
- V_input = -50V → V_adc = 0V
- V_input = 0V   → V_adc = 1.65V
- V_input = +50V → V_adc = 3.3V

## 测试验证

### 测试用例

1. **零电压测试**
   - 输入：0V
   - 预期：波形在屏幕中心（横坐标轴上）
   - 测量：Vmax≈0V, Vmin≈0V, Vpp≈0V

2. **正电压测试**
   - 输入：+5V
   - 预期：波形在中心线上方
   - 测量：Vmax≈+5V, Vmin≈+5V

3. **负电压测试**
   - 输入：-5V
   - 预期：波形在中心线下方
   - 测量：Vmax≈-5V, Vmin≈-5V

4. **交流信号测试**
   - 输入：±10V正弦波
   - 预期：波形在中心线上下对称振荡
   - 测量：Vmax≈+10V, Vmin≈-10V, Vpp≈20V

## 调试日志

修改后的代码会输出详细的调试信息：

```
I (12345) OscADC: Voltage range: -50.0V to 50.0V
I (12346) OscDraw: Drawing REAL ADC data: 1024 points, Vmin=-5.23V, Vmax=4.87V, Vpp=10.10V
I (12347) OscCore: Measurements: Vmax=4.87V, Vmin=-5.23V, Vpp=10.10V, Vrms=3.45V, Freq=50.0Hz
```

## 已知限制

1. **ADC精度**：ESP32 ADC为12位，实际有效位数约10-11位
2. **采样率**：最大1MSPS，限制了可测量的最高频率
3. **电压范围**：需要外部电路支持±50V输入
4. **噪声**：ADC本身有噪声，建议添加硬件滤波

## 后续改进建议

1. 添加电压范围配置选项（如±5V, ±10V, ±50V）
2. 实现软件校准功能
3. 添加AC/DC耦合选项
4. 实现带宽限制功能
5. 添加探头衰减系数设置（1x, 10x, 100x）

## 修改文件列表

- `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_adc.h` - 添加电压范围宏定义
- `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_adc.c` - 修改电压转换函数
- `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_draw.c` - 修复坐标系统和绘制逻辑
- `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_core.c` - 修复测量和自动调整逻辑

## 编译和测试

```bash
# 清理并重新编译
./rebuild.sh

# 烧录到设备
idf.py flash monitor
```

## 日期

2026-01-24
