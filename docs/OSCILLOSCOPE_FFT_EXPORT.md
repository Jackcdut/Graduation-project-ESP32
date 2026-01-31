# 示波器 FFT 数据导出功能

## 概述
示波器现在支持完整的 FFT 频谱数据导出功能，可以将频域分析结果保存到 SD 卡。

## 功能特性

### 1. 自动 FFT 检测
- 当示波器处于 FFT 模式时，点击 EXPORT 按钮会自动导出两个文件：
  - `Oscilloscope_XXX.csv` - 时域波形数据
  - `Oscilloscope_XXX_FFT.csv` - 频域频谱数据

### 2. FFT 文件内容

#### 文件头信息
```csv
# Oscilloscope FFT Spectrum Data
# Timestamp: 2026-01-24 12:34:56
# Fundamental Frequency: 1000.00 Hz
# H1 (Fundamental): 1.500 V
# H3 (3rd Harmonic): 0.050 V
# THD (Total Harmonic Distortion): 3.33 %
# FFT Size: 512 points
# Max Frequency: 10000 Hz
# Amplitude Range: 60 dB
# Spectrum Points: 256
```

#### 数据列
- **Index**: 频谱点索引 (0-255)
- **Frequency(Hz)**: 频率值
- **Magnitude(dB)**: 幅度（分贝）

### 3. 导出的测量值

FFT 模式下导出的特殊测量值：
- **Fundamental Frequency**: 基波频率（信号主频率）
- **H1**: 1次谐波幅度（基波幅度）
- **H3**: 3次谐波幅度
- **THD**: 总谐波失真百分比
- **FFT Size**: FFT 点数（512）
- **Max Frequency**: 显示的最大频率范围
- **Amplitude Range**: 幅度显示范围（dB）

### 4. 数据格式

频谱数据按照以下格式保存：
```csv
Index,Frequency(Hz),Magnitude(dB)
0,0.00,-60.000
1,39.06,-58.234
2,78.13,-55.678
...
255,9960.94,-45.123
```

## 使用方法

1. **进入 FFT 模式**
   - 点击示波器界面的 FFT 按钮
   - 波形显示切换为紫色频谱

2. **调整 FFT 参数**
   - 使用时间刻度按钮调整频率范围（1kHz - 50kHz）
   - 使用电压刻度按钮调整幅度范围（20dB - 100dB）

3. **导出数据**
   - 点击 EXPORT 按钮
   - 系统自动保存时域和频域两个文件
   - 按钮显示 "SAVED #XXX" 表示成功

4. **查看文件**
   - 文件保存在 SD 卡的 `/sdcard/Oscilloscope/` 目录
   - 使用 CSV 查看器或 Excel 打开
   - 可以用 Python/MATLAB 进行进一步分析

## 技术细节

### 数据转换
- 图表 Y 坐标 (0-100) → dB 幅度
- 公式: `dB = (y/100) * amp_range - amp_range`
- 频率计算: `freq = index * (max_freq / num_points)`

### 频谱分辨率
- FFT 点数: 512
- 显示点数: 256（柱状图）
- 频率分辨率: `max_freq / 256`

### 文件命名
- 时域: `Oscilloscope_001.csv`
- 频域: `Oscilloscope_001_FFT.csv`
- 编号自动递增

## 应用场景

1. **信号质量分析**
   - 导出频谱数据
   - 分析谐波成分
   - 计算失真度

2. **频率响应测试**
   - 扫频测试
   - 记录不同频率点的响应
   - 绘制频率响应曲线

3. **噪声分析**
   - 识别干扰频率
   - 分析噪声分布
   - 评估信噪比

4. **教学演示**
   - 展示傅里叶变换
   - 对比时域和频域
   - 理解频谱概念

## 注意事项

1. **SD 卡要求**
   - 必须插入 SD 卡才能导出
   - 确保有足够存储空间
   - 建议使用 Class 10 或更高速度

2. **数据有效性**
   - 确保信号稳定后再导出
   - FFT 需要足够的采样点
   - 避免在信号变化时导出

3. **文件管理**
   - 定期清理旧文件
   - 编号达到 999 后会循环
   - 可以通过 USB 读取 SD 卡

## 更新日志

### 2026-01-24
- ✅ 添加完整的 FFT 数据导出功能
- ✅ 导出基波、谐波、THD 等测量值
- ✅ 正确的频率-幅度映射
- ✅ 独立的 FFT 文件格式
- ✅ 详细的文件头信息
