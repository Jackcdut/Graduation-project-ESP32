# 示波器游标测量功能说明

## 功能概述

示波器的游标测量功能（Cursor Measurement）是专业示波器的标准功能，用于精确测量波形上的时间、频率、电压和幅值。本实现将原有的Trig按钮替换为Cursor按钮，提供专业的刻度线查询功能。

## 功能特性

### 1. 三种工作模式

- **OFF（关闭）**: 无游标显示，正常查看波形
- **H-CUR（横轴游标）**: 显示垂直游标线，用于测量时间/频率
- **V-CUR（纵轴游标）**: 显示水平游标线，用于测量电压/幅值

### 2. 专业视觉设计

#### 横轴游标（H-CUR）
- **颜色**: 青色（#00FFFF）
- **样式**: 虚线（8px实线 + 4px间隔）
- **效果**: 发光阴影效果，模拟专业示波器显示
- **标签**: 深色半透明背景，带边框和圆角

#### 纵轴游标（V-CUR）
- **颜色**: 黄色（#FFFF00）
- **样式**: 虚线（8px实线 + 4px间隔）
- **效果**: 发光阴影效果
- **标签**: 深色半透明背景，带边框和圆角

### 3. 智能测量显示

#### 时域模式（Time Domain）
- **横轴**: 显示时间值
  - 自动单位转换：s（秒）、ms（毫秒）、μs（微秒）、ns（纳秒）
  - 精度：根据时间范围自动调整
  
- **纵轴**: 显示电压值
  - 单位：V（伏特）
  - 精度：3位小数
  - 范围：根据当前电压刻度自动计算

#### FFT模式（频域分析）
- **横轴**: 显示频率值
  - 自动单位转换：Hz（赫兹）、kHz（千赫兹）、MHz（兆赫兹）
  - 精度：根据频率范围自动调整
  
- **纵轴**: 显示幅值
  - 单位：dB（分贝）
  - 精度：1位小数
  - 范围：根据当前幅值范围自动计算

## 使用方法

### 基本操作

1. **激活游标**
   - 点击右侧控制面板的"Cursor"按钮
   - 按钮边框会变色并发光，指示当前模式

2. **切换模式**
   - 连续点击"Cursor"按钮循环切换：OFF → H-CUR → V-CUR → OFF
   - 按钮标签会显示当前模式

3. **移动游标**
   - 在波形显示区域触摸并拖动
   - 游标线会跟随触摸位置移动
   - 实时显示当前位置的测量值

4. **读取测量值**
   - 游标线旁边的标签实时显示测量值
   - 标签位置智能调整，避免超出边界

### 高级技巧

1. **精确定位**
   - 慢速拖动可以精确定位到感兴趣的点
   - 标签会实时更新显示值

2. **边界保护**
   - 游标线自动限制在波形显示区域内
   - 标签自动调整位置避免遮挡

3. **模式记忆**
   - 游标位置在切换模式时会重置到中心
   - 每次激活游标都从中心位置开始

## 技术实现

### 坐标系统

#### 波形显示区域
- 宽度：720像素
- 高度：400像素
- 网格：16列 × 9行

#### 时域坐标转换
```c
// 时间计算
float time_per_div = time_scale_values[osc_time_scale_index];
float total_time = time_per_div * OSC_GRID_COLS;
float time = (float)cursor_x * total_time / 720.0f;

// 电压计算
float volts_per_div = volt_scale_values[osc_volt_scale_index];
float chart_y = (400.0f - cursor_y) * 1000.0f / 400.0f;
float voltage = ((chart_y - 500.0f) / 100.0f) * volts_per_div;
```

#### FFT坐标转换
```c
// 频率计算
float max_freq = osc_fft_freq_ranges[osc_fft_freq_range_index];
float freq = (float)cursor_x * max_freq / 720.0f;

// 幅值计算（dB）
float amp_range = osc_fft_amp_ranges[osc_fft_amp_range_index];
float chart_y = (400.0f - cursor_y) * 1000.0f / 400.0f;
float db = (chart_y / 1000.0f) * amp_range - amp_range;
```

### 视觉效果实现

#### 虚线样式
```c
lv_obj_set_style_line_dash_width(line, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
lv_obj_set_style_line_dash_gap(line, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
```

#### 发光效果
```c
lv_obj_set_style_shadow_width(line, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
lv_obj_set_style_shadow_color(line, color, LV_PART_MAIN|LV_STATE_DEFAULT);
lv_obj_set_style_shadow_opa(line, LV_OPA_50, LV_PART_MAIN|LV_STATE_DEFAULT);
lv_obj_set_style_shadow_spread(line, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
```

## 与专业示波器的对比

### 符合标准
- ✅ 虚线游标样式
- ✅ 发光视觉效果
- ✅ 实时数值显示
- ✅ 自动单位转换
- ✅ 时域/频域支持

### 改进空间
- 双游标测量（Δ测量）
- 游标跟踪波形
- 游标位置数字输入
- 测量结果保存

## 相关文件

- `BSP/GUIDER/generated/events_init.c` - 游标事件处理
- `BSP/GUIDER/generated/setup_scr_scrOscilloscope.c` - UI初始化
- `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_core.h` - 核心定义
- `BSP/GUIDER/custom/modules/oscilloscope/oscilloscope_draw.c` - 绘图实现

## 更新日志

### 2026-01-24
- ✅ 将Trig按钮功能替换为Cursor游标测量
- ✅ 实现横轴游标（时间/频率测量）
- ✅ 实现纵轴游标（电压/幅值测量）
- ✅ 添加专业示波器风格的虚线和发光效果
- ✅ 优化标签显示和边界保护
- ✅ 支持时域和FFT模式
- ✅ 添加触摸拖动功能
- ✅ 实时数值更新和单位自动转换
