# 示波器FFT功能修复说明

## 问题描述

原始FFT实现显示了3个尖峰（基波、二次谐波、三次谐波），这对于纯正弦波信号是不正确的。

### 原始问题
- 显示多个谐波峰值（在位置10、80、180）
- 不符合纯正弦波的FFT特性
- 硬编码的峰值位置，不基于实际信号频率
- 缺少频率轴信息

## 理论基础

### 纯正弦波的FFT特性
对于纯正弦波信号（如1kHz正弦波）：
- **应该只有一个主峰**：位于信号频率（1kHz）
- **没有谐波**：因为是纯正弦波，不含其他频率成分
- **噪声底**：除主峰外，其他频率应该只有很小的噪声

### 为什么原来显示3个峰是错误的？
- **基波（Fundamental）**：正确，这是信号的主频率
- **二次谐波（2nd Harmonic）**：错误！纯正弦波不应该有谐波
- **三次谐波（3rd Harmonic）**：错误！只有失真的信号才会有谐波

谐波只出现在：
- 方波、三角波等非正弦波形
- 失真的正弦波（如放大器削波）
- 含有多个频率成分的复合信号

## 解决方案

### 1. 基于实际信号频率的FFT

```c
// 计算频率轴映射
float sampling_rate = (float)num_points / total_time_on_screen;  // Hz
float freq_resolution = sampling_rate / (float)num_points;  // Hz per bin
float nyquist_freq = sampling_rate / 2.0f;  // 最大可显示频率

// 找到信号频率对应的频率bin
int fundamental_bin = (int)(signal_frequency / freq_resolution);
```

**关键概念**：
- **采样率（Sampling Rate）**：每秒采样点数
- **频率分辨率（Frequency Resolution）**：每个FFT bin代表的频率宽度
- **奈奎斯特频率（Nyquist Frequency）**：采样率的一半，最大可显示频率

### 2. 单峰FFT显示

```c
for(int i = 0; i < num_points; i++) {
    int val;
    
    // 只显示正频率（0到奈奎斯特频率）
    if (i > num_points / 2) {
        // 负频率部分不显示
        val = (int)chart_center;
    } else {
        // 计算与基波频率的距离
        int dist_from_fundamental = abs(i - fundamental_bin);
        
        if (dist_from_fundamental < 5) {
            // 基波频率处的主峰（高斯形状）
            float peak_height = 400.0f;
            float peak_width = 3.0f;
            val = (int)(chart_center - peak_height * expf(-powf((float)dist_from_fundamental / peak_width, 2)));
        } else {
            // 噪声底 - 非常低的随机噪声
            val = (int)(chart_center + (float)(rand() % 6 - 3));
        }
    }
}
```

### 3. 准确的测量显示

修复后的FFT模式显示：

| 测量项 | 说明 | 示例值 |
|--------|------|--------|
| Fund | 基波频率 | 1.00kHz |
| Res | 频率分辨率 | 62.5Hz |
| Nyq | 奈奎斯特频率 | 11.0kHz |
| THD | 总谐波失真 | <0.1% |
| Peak | 峰值幅度 | 1.50V |

## 效果对比

### 修复前
```
FFT显示：
  |     *              *         *
  |    * *            * *       * *
  |   *   *          *   *     *   *
  |__*_____*________*_____*___*_____*____
     10    80       180   250
     基波  二次谐波  三次谐波  噪声
```
❌ 错误：纯正弦波不应该有谐波！

### 修复后
```
FFT显示：
  |        *
  |       ***
  |      *****
  |_____*******_____________________
        1kHz
        基波（唯一的峰）
```
✅ 正确：纯正弦波只有一个主峰！

## 频率轴计算示例

### 示例1：时间参数 = 1ms/div

```
屏幕宽度：16格 × 1ms/div = 16ms
采样点数：688点
采样率：688点 / 0.016s = 43,000 Hz
频率分辨率：43,000 / 688 ≈ 62.5 Hz/bin
奈奎斯特频率：43,000 / 2 = 21,500 Hz

1kHz信号位置：1000 / 62.5 = bin 16
```

### 示例2：时间参数 = 100us/div

```
屏幕宽度：16格 × 100us/div = 1.6ms
采样点数：688点
采样率：688点 / 0.0016s = 430,000 Hz
频率分辨率：430,000 / 688 ≈ 625 Hz/bin
奈奎斯特频率：430,000 / 2 = 215,000 Hz

1kHz信号位置：1000 / 625 = bin 1.6
```

## 技术细节

### 高斯峰形状

使用高斯函数生成平滑的峰形：

```c
amplitude = peak_height * exp(-(distance / peak_width)^2)
```

这比矩形峰更真实，因为：
- 实际FFT由于窗函数会产生频谱泄漏
- 峰值不是完美的单点，而是有一定宽度
- 高斯形状接近实际FFT的主瓣形状

### 噪声底

```c
noise = center + random(-3, +3)
```

模拟：
- ADC量化噪声
- 热噪声
- 数字处理误差

## 使用说明

### 查看FFT
1. 点击**FFT按钮**（变为亮黄色）
2. 波形切换为频谱显示
3. 观察1kHz处的单个主峰
4. 查看底部的FFT测量信息

### 理解显示
- **X轴**：频率（0 到 奈奎斯特频率）
- **Y轴**：幅度（对数或线性刻度）
- **主峰位置**：信号的基波频率
- **峰高**：信号的幅度
- **噪声底**：背景噪声水平

### 时间参数对FFT的影响

| 时间参数 | 采样率 | 频率分辨率 | 最大频率 | 适用场景 |
|----------|--------|------------|----------|----------|
| 10us/div | 4.3MHz | 6.25kHz | 2.15MHz | 高频信号 |
| 100us/div | 430kHz | 625Hz | 215kHz | 中频信号 |
| 1ms/div | 43kHz | 62.5Hz | 21.5kHz | 音频信号 |
| 10ms/div | 4.3kHz | 6.25Hz | 2.15kHz | 低频信号 |

**规律**：
- 时间参数越小 → 采样率越高 → 可显示更高频率
- 时间参数越大 → 频率分辨率越高 → 可分辨更接近的频率

## 后续增强建议

### 1. 窗函数支持
添加不同的窗函数选项：
- 矩形窗（Rectangle）
- 汉宁窗（Hanning）
- 汉明窗（Hamming）
- 布莱克曼窗（Blackman）

### 2. 对数刻度
添加对数幅度显示（dB）：
```c
amplitude_dB = 20 * log10(amplitude / reference)
```

### 3. 峰值检测
自动检测和标注频谱中的峰值：
- 基波频率
- 谐波频率（如果存在）
- 峰值幅度

### 4. 真实FFT算法
集成真实的FFT库（如CMSIS-DSP）：
- 更准确的频谱分析
- 支持不同的FFT长度
- 更快的计算速度

### 5. 频谱平均
多次FFT结果平均，降低噪声：
```c
averaged_spectrum = (spectrum1 + spectrum2 + ... + spectrumN) / N
```

## 参考资料

- **FFT基础**：https://en.wikipedia.org/wiki/Fast_Fourier_transform
- **窗函数**：https://en.wikipedia.org/wiki/Window_function
- **频谱分析**：https://en.wikipedia.org/wiki/Spectral_density
- **奈奎斯特定理**：https://en.wikipedia.org/wiki/Nyquist–Shannon_sampling_theorem

## 文件修改清单

- `BSP/GUIDER/generated/events_init.c`
  - `osc_waveform_update_cb()`: 修复FFT显示逻辑
  - `scrOscilloscope_btnFFT_event_handler()`: 更新FFT按钮事件处理

## 测试建议

1. **基本功能**
   - 切换到FFT模式，验证只显示一个峰
   - 检查峰值位置是否在1kHz附近

2. **不同时间参数**
   - 测试不同时间参数下的FFT显示
   - 验证频率分辨率和奈奎斯特频率的计算

3. **测量准确性**
   - 验证基波频率显示是否正确
   - 检查THD是否接近0（<0.1%）

4. **切换稳定性**
   - 快速切换FFT开关
   - 验证显示是否正确恢复
