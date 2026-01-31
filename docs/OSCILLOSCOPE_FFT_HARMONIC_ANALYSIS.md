# 示波器FFT谐波分析功能

## 修改日期
2026-01-24

## 功能描述
在FFT模式下，底部测量显示改为显示谐波分析信息，包括基波幅值、3次谐波幅值和总谐波失真（THD）。

## 修改内容

### 1. FFT测量显示修改

#### 修改前：
- **Peak**: 峰值频率
- **Span**: 频率范围
- **Range**: 幅度范围
- **Res**: 频率分辨率
- **FFT**: FFT点数

#### 修改后：
- **Freq**: 信号频率（基波频率）
- **H1**: 1次谐波幅值（基波幅值）
- **H3**: 3次谐波幅值
- **THD**: 总谐波失真（百分比）
- **FFT**: FFT点数（保持不变）

### 2. 谐波分析算法

#### 基波检测
```c
// 在FFT频谱中找到最大幅值对应的频率bin
float fundamental_magnitude = fft_magnitude[peak_bin];
float signal_frequency = peak_bin * freq_resolution;
```

#### 3次谐波检测
```c
// 计算3次谐波的频率bin
int harmonic_3_bin = peak_bin * 3;

// 在3次谐波频率附近搜索峰值（±2个bin）
for (int i = harmonic_3_bin - 2; i <= harmonic_3_bin + 2; i++) {
    if (fft_magnitude[i] > max_h3) {
        max_h3 = fft_magnitude[i];
    }
}
harmonic_3_magnitude = max_h3;
```

#### THD计算
```c
// THD = sqrt(sum(harmonics^2)) / fundamental * 100%
// 考虑2-10次谐波

float harmonics_sum_sq = 0.0f;
for (int h = 2; h <= 10; h++) {
    int h_bin = peak_bin * h;
    // 在谐波频率附近搜索峰值（±2个bin）
    // 累加谐波幅值的平方
    harmonics_sum_sq += max_h * max_h;
}

thd = sqrt(harmonics_sum_sq) / fundamental_magnitude * 100.0f;
```

### 3. 显示格式

#### Freq（信号频率）
- 单位自动切换：Hz / kHz / MHz
- 格式：`Freq: 1.00kHz`

#### H1（基波幅值）
- 单位：V（伏特）
- 格式：`H1: 1.234V`
- 精度：3位小数

#### H3（3次谐波幅值）
- 单位：V（伏特）
- 格式：`H3: 0.123V`
- 精度：3位小数

#### THD（总谐波失真）
- 单位：%（百分比）
- 格式：`THD: 5.67%`
- 精度：2位小数

## 技术细节

### 谐波搜索窗口
在计算谐波幅值时，不是直接取理论频率bin的值，而是在附近±2个bin范围内搜索峰值。这样可以：
1. 补偿频率估计误差
2. 处理频率泄漏效应
3. 提高谐波检测的鲁棒性

### THD计算范围
- 考虑2-10次谐波（不包括基波）
- 如果某次谐波超出FFT范围（> Nyquist频率），则忽略
- 使用RMS方法：`sqrt(H2² + H3² + ... + H10²) / H1`

### 变量作用域
谐波分析变量定义在FFT模式代码块的外层，确保在显示更新时可以访问：
```c
float fundamental_magnitude = 0.0f;
float harmonic_3_magnitude = 0.0f;
float thd = 0.0f;
```

## 使用场景

### 1. 信号质量评估
- 查看THD值评估信号失真程度
- THD < 1%：高质量信号
- THD 1-5%：中等质量
- THD > 5%：明显失真

### 2. 谐波分析
- H1显示基波幅值（主要信号成分）
- H3显示3次谐波幅值（常见的失真成分）
- 比较H3/H1比值评估3次谐波失真

### 3. 音频分析
- 分析音频信号的谐波成分
- 评估放大器或音频设备的失真特性
- 检测非线性失真

## 测试要点
1. ✅ FFT模式下显示Freq、H1、H3、THD、FFT
2. ✅ 基波频率正确检测
3. ✅ 基波幅值准确显示
4. ✅ 3次谐波正确检测和显示
5. ✅ THD计算准确（考虑2-10次谐波）
6. ✅ 单位自动切换（Hz/kHz/MHz）
7. ✅ 数值格式正确（小数位数）

## 相关文件
- `BSP/GUIDER/generated/events_init.c` - FFT谐波分析和显示逻辑

## 示例输出
```
Freq: 1.00kHz    (信号频率)
H1: 1.234V       (基波幅值)
H3: 0.056V       (3次谐波幅值)
THD: 4.54%       (总谐波失真)
FFT: 512pt       (FFT点数)
```

## 注意事项
1. 谐波检测需要信号频率稳定
2. 采样率为50kHz，最高可检测25kHz（Nyquist频率）
3. 如果谐波频率超出Nyquist频率，将无法检测
4. FFT使用Hanning窗减少频谱泄漏
5. 使用ESP-DSP硬件加速FFT计算
