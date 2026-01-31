# 数字万用表界面设计文档

## 概述
这是一个专业级的数字万用表(Digital Multimeter)界面，使用LVGL v8.3设计，参考OWON等专业万用表的样式和布局。

## 界面布局

### 1. 顶部状态栏 (0-60px)
- **返回按钮**: 左上角，返回主界面
- **AUTO+标识**: 当前测量模式 (AUTO+/V=/V~/A=/A~/Ω)
- **RUN状态**: 运行状态指示
- **时间显示**: 00:00:00 格式
- **量程显示**: 当前挡位 (AUTO/400mV/4V/40V等)
- **蓝牙图标**: 蓝牙连接状态
- **电池图标**: 电池电量显示

### 2. 主显示区域 (75-275px)
- **LCD风格面板**: 黑底橙色字，专业万用表风格
- **副显示值**: 顶部显示两个辅助测量值 (如 3.3030V~ 和 5.0164V==)
- **主数值显示**: 超大号数字 (6.0061)，居中显示
- **单位指示**: 右下角显示单位 (V~/V=/A~/A=/Ω)
- **详细参数**: 底部显示多个测量参数 (电压/电流/电阻/温度)

### 3. 波形图区域 (290-400px)
- **趋势图表**: 显示测量数据的变化趋势
- **网格线**: 专业示波器风格的网格
- **实时更新**: 100个数据点，橙色波形线

### 4. 底部按钮区 (415-470px)
8个功能按钮，排列整齐：
- **V~**: AC电压测量 (蓝色)
- **V=**: DC电压测量 (深蓝色)
- **A~**: AC电流测量 (橙色)
- **A=**: DC电流测量 (深橙色)
- **Ω**: 电阻测量 (紫色)
- **AUTO**: 自动测量模式 (绿色)
- **RANGE**: 挡位切换按钮 (灰色)
- **HOLD**: 数值保持功能 (灰色/红色)

## 测量范围

### 电压测量 (0-36V)
- **AUTO**: 自动选择最佳挡位
- **400mV**: 0-0.4V (精度0.0001V)
- **4V**: 0-4V (精度0.001V)
- **40V**: 0-40V (精度0.01V)

### 电流测量 (0-2A)
- **AUTO**: 自动选择最佳挡位
- **40mA**: 0-40mA (精度0.01mA)
- **400mA**: 0-400mA (精度0.1mA)
- **2A**: 0-2A (精度0.001A)

### 电阻测量 (0-1MΩ)
- **AUTO**: 自动选择最佳挡位
- **400Ω**: 0-400Ω (精度0.1Ω)
- **4kΩ**: 0-4kΩ (精度1Ω)
- **40kΩ**: 0-40kΩ (精度10Ω)
- **400kΩ**: 0-400kΩ (精度100Ω)
- **1MΩ**: 0-1MΩ (精度1kΩ)

## ESP32集成接口

### 主要函数

#### 1. 更新测量数据
```c
void dmm_update_measurement(float voltage, float current, float resistance);
```

**参数说明:**
- `voltage`: 电压值，单位伏特 (0.000 - 36.000V)
- `current`: 电流值，单位安培 (0.000 - 2.000A)
- `resistance`: 电阻值，单位欧姆 (0 - 1000000Ω)

**使用示例:**
```c
// ESP32代码中调用
float v = read_adc_voltage();     // 读取ADC电压
float i = read_adc_current();     // 读取ADC电流
float r = read_adc_resistance();  // 读取电阻

dmm_update_measurement(v, i, r);  // 更新显示
```

#### 2. 获取当前状态
界面内部维护测量状态，包括：
- 当前测量模式 (AUTO/电压/电流/电阻)
- 当前量程设置
- HOLD状态
- 自动/手动量程模式

### ESP32实现建议

#### ADC采集代码示例
```c
#include "esp_adc/adc_oneshot.h"

// ADC配置
static adc_oneshot_unit_handle_t adc1_handle;

void adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
}

float read_adc_voltage(void) {
    int adc_raw;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);
    
    // 转换为电压 (0-3.3V参考电压 * 电压分压比)
    float voltage = (adc_raw / 4095.0f) * 3.3f * VOLTAGE_DIVIDER_RATIO;
    return voltage;
}

float read_adc_current(void) {
    // 使用INA219或类似芯片读取电流
    // 这里是示例代码
    return 0.0f;
}

float read_adc_resistance(void) {
    // 使用电压分压法测量电阻
    float v_ref = 3.3f;
    float v_measured = read_adc_voltage();
    float r_known = 10000.0f;  // 10kΩ已知电阻
    
    float resistance = (v_measured * r_known) / (v_ref - v_measured);
    return resistance;
}
```

#### 定时更新任务
```c
void dmm_update_task(void *pvParameters) {
    while(1) {
        float voltage = read_adc_voltage();
        float current = read_adc_current();
        float resistance = read_adc_resistance();
        
        // 更新显示
        dmm_update_measurement(voltage, current, resistance);
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms更新一次
    }
}
```

## 动画效果

### 1. 模式切换动画
- 淡出淡入效果 (200ms)
- 主显示数值平滑过渡

### 2. AUTO模式呼吸灯
- 主数值颜色呼吸变化
- 吸引用户注意

### 3. HOLD按钮状态
- 正常: 灰色背景
- 激活: 红色背景 + 阴影增强

### 4. 波形图实时更新
- 100个数据点滚动显示
- 橙色波形线，3px线宽

## 颜色方案

### 主色调
- **LCD背景**: #050505 ~ #0A0A0A (深黑色渐变)
- **LCD文字**: #FF9500 (橙黄色，经典万用表显示色)
- **背景色**: #F3F8FE (浅蓝灰色)

### 按钮配色
- **电压AC**: #2196F3 (蓝色)
- **电压DC**: #3F51B5 (深蓝色)
- **电流AC**: #FF9800 (橙色)
- **电流DC**: #FF5722 (深橙色)
- **电阻**: #9C27B0 (紫色)
- **AUTO**: #4CAF50 (绿色)
- **功能**: #607D8B (蓝灰色)

## 所需图标

虽然当前使用LVGL内置符号，如需更专业的效果，建议准备以下图标：

1. **测量模式图标** (32x32px):
   - 闪电符号 (电压)
   - 波形符号 (交流)
   - Ω符号 (电阻)
   
2. **状态图标** (24x24px):
   - 蓝牙图标
   - WiFi图标
   - 电池图标

## 特色功能

### 1. AUTO自动测量模式
- 自动识别连接的信号类型
- 自动选择最佳挡位
- 绿色标识，易于识别

### 2. 智能量程切换
- 点击RANGE按钮循环切换
- 显示当前挡位
- 获得更高精度

### 3. HOLD数值保持
- 冻结当前显示值
- 便于读取和记录
- 视觉反馈明显

### 4. 多参数同时显示
- 主参数: 超大号显示
- 副参数: 顶部小字显示
- 详细参数: 底部显示电压/电流/电阻/温度

### 5. 趋势图表
- 实时显示测量值变化
- 网格背景，专业示波器风格
- 帮助观察信号稳定性

## 使用说明

### 基本操作流程

1. **选择测量模式**
   - 点击底部对应按钮 (V~/V=/A~/A=/Ω)
   - 或点击AUTO自动识别

2. **调整量程**
   - 点击RANGE按钮切换挡位
   - AUTO模式自动选择最佳挡位

3. **读取数值**
   - 主显示区域: 当前主要测量值
   - 副显示区域: 其他测量值
   - 趋势图: 观察数值变化

4. **保持数值**
   - 点击HOLD按钮冻结显示
   - 再次点击解除保持

## 技术实现要点

### 内存优化
- 静态变量存储状态
- 避免频繁内存分配

### 性能优化
- 100ms更新间隔，平衡响应和性能
- 波形图采用高效的chart控件
- 动画使用硬件加速

### 代码结构
- 清晰的函数分离
- 完整的注释文档
- 易于维护和扩展

## 后续扩展建议

1. **数据记录功能**
   - 保存测量历史
   - 导出CSV数据

2. **无线传输**
   - 蓝牙/WiFi连接
   - 手机APP查看

3. **更多测量模式**
   - 电容测量
   - 频率测量
   - 温度测量

4. **语音播报**
   - 测量结果语音提示
   - 适合特殊场景

---

## 作者说明

本界面设计完全重构了原有的scrPrintMenu界面，创建了一个专业级的数字万用表显示界面。界面设计参考了OWON、Fluke等专业万用表的经典设计，同时融入现代化的UI元素和动画效果。

所有代码已经过优化，确保在ESP32等嵌入式平台上流畅运行。界面布局美观、功能完善、交互流畅，完全符合专业数字万用表的使用体验。
















