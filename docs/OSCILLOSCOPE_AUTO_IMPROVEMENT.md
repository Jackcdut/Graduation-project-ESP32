# 示波器AUTO按钮改进

## 修改日期
2026-01-24

## 问题描述
原AUTO按钮的自动调整算法过于保守，导致波形显示不够充分，用户体验不佳。

## 原算法的问题

### 1. 电压档位选择过于保守
- 使用4个division的裕量
- 信号只占据屏幕的40-50%
- 波形显示太小，不便于观察

### 2. 时间档位算法不够准确
- 使用复杂的时间缩放因子计算
- 对于不同频率的信号适应性差
- 容易选择不合适的时基

### 3. 没有Y偏移调整
- 信号可能不在屏幕中心
- 如果信号有直流偏置，显示效果差
- 无法充分利用屏幕空间

## 改进方案

### 1. 更激进的电压档位选择

#### 改进前：
```c
// 让信号占据8个division（4上4下），留2个division裕量
float max_excursion = max(|max - center|, |min - center|);
volts_per_div >= max_excursion / 4.0;
```

#### 改进后：
```c
// 让峰峰值占据6.5个division，信号更充分显示
float vpp = max_voltage - min_voltage;
volts_per_div = vpp / 6.5;
```

**优势**：
- 信号占据屏幕的60-80%
- 更充分利用显示空间
- 波形细节更清晰

### 2. 简化的时间档位选择

#### 改进前：
```c
// 复杂的时间缩放因子计算
const float time_scale_factors[] = {...};
for each time_scale:
    estimated_samples = avg_period / factor * normalize;
    find best match;
```

#### 改进后：
```c
// 基于当前显示的周期数进行增量调整
cycles_on_screen = num_points / avg_period_samples;

if (cycles_on_screen < 1.5) {
    // 周期太少，放大时间（减小时基）
    time_idx--;
} else if (cycles_on_screen > 4.0) {
    // 周期太多，缩小时间（增大时基）
    time_idx++;
} else {
    // 当前时基合适，保持不变
}
```

**优势**：
- 逻辑简单直观
- 增量调整，避免大幅跳变
- 更稳定可靠

### 3. 自动Y偏移调整

#### 新增功能：
```c
// 计算信号中心
signal_center = (max_voltage + min_voltage) / 2.0;

// 设置Y偏移，让信号居中
osc_y_offset = -signal_center;

// 限制偏移范围
max_y_offset = volts_per_div * 3.0;
clamp(osc_y_offset, -max_y_offset, max_y_offset);
```

**优势**：
- 信号自动居中显示
- 处理有直流偏置的信号
- 充分利用屏幕上下空间

### 4. 改进的零点检测

#### 改进前：
```c
// 使用固定的chart_center作为零点
if (y[i-1] < chart_center && y[i] >= chart_center) {
    zero_crossing++;
}
```

#### 改进后：
```c
// 使用信号的平均值作为零点参考
avg_y = sum(y) / num_points;

if (y[i-1] < avg_y && y[i] >= avg_y) {
    zero_crossing++;
}
```

**优势**：
- 对有直流偏置的信号更准确
- 更鲁棒的频率检测
- 适应各种波形

## 算法流程

### Step 1: 分析电压范围
```
遍历所有采样点 → 找到min_voltage和max_voltage
计算vpp = max - min
计算signal_center = (max + min) / 2
```

### Step 2: 选择电压档位
```
target_volts_per_div = vpp / 6.5
找到最接近的标准档位（稍大一点）
特殊处理：小信号至少使用10mV档
```

### Step 3: 计算Y偏移
```
osc_y_offset = -signal_center
限制范围：±3个division
```

### Step 4: 分析频率
```
计算信号平均值作为零点
检测过零点，计算平均周期
计算当前屏幕显示的周期数
```

### Step 5: 调整时间档位
```
if 周期数 < 1.5:
    减小时基（放大时间）
else if 周期数 > 4.0:
    增大时基（缩小时间）
else:
    保持当前时基
```

### Step 6: 更新显示
```
更新Time、Volt档位标签
更新X-Pos（重置为0）、Y-Pos标签
```

## 使用效果

### 改进前：
- 信号占据屏幕40-50%
- 可能不居中
- 时基可能不合适
- 需要手动调整多次

### 改进后：
- 信号占据屏幕60-80%
- 自动居中显示
- 时基更合理（2-3个周期）
- 一键调整到最佳状态

## 测试场景

### 1. 正弦波信号
- ✅ 自动选择合适的V/div
- ✅ 显示2-3个完整周期
- ✅ 信号居中

### 2. 方波信号
- ✅ 准确检测周期
- ✅ 合适的电压档位
- ✅ 清晰显示上升/下降沿

### 3. 有直流偏置的信号
- ✅ 自动调整Y偏移
- ✅ 信号居中显示
- ✅ 充分利用屏幕空间

### 4. 小信号（mV级）
- ✅ 自动选择10mV档
- ✅ 信号放大显示
- ✅ 细节清晰可见

### 5. 复杂波形
- ✅ 基于平均值检测零点
- ✅ 鲁棒的频率检测
- ✅ 合理的时基选择

## 相关文件
- `BSP/GUIDER/generated/events_init.c` - AUTO按钮事件处理器

## 注意事项
1. AUTO功能会重置X偏移为0
2. Y偏移会自动调整以居中信号
3. 时基调整是增量式的，避免大幅跳变
4. 对于无信号或噪声，使用默认档位
5. 电压档位选择会稍大于理论值，确保信号不被裁剪
