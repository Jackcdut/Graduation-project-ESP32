# STM32F407 固件 - 信号采集与处理

本分支包含 STM32F407 微控制器的固件代码，负责高精度模拟信号采集与处理。

## 📁 目录结构

```
stm32/
├── BSP/                       # 板级支持包
│   ├── Communication/         # 与ESP32通信协议
│   │   ├── comm_protocol.h    # 协议定义
│   │   ├── uart_comm.c/h      # UART通信驱动
│   │   ├── cmd_handler.c/h    # 命令处理
│   │   └── data_reporter.c/h  # 数据上报
│   ├── Oscilloscope/          # 示波器模块
│   ├── Multimeter/            # 万用表模块
│   ├── SignalGenerator/       # 信号发生器模块
│   └── PowerSupply/           # 数控电源模块
├── Core/                      # STM32 HAL 核心代码
│   ├── Inc/                   # 头文件
│   └── Src/                   # 源文件
├── Drivers/                   # HAL驱动库
│   ├── CMSIS/                 # ARM CMSIS
│   └── STM32F4xx_HAL_Driver/  # STM32 HAL
├── MDK-ARM/                   # Keil MDK 工程
│   ├── project.uvprojx        # 工程文件
│   └── startup_stm32f407xx.s  # 启动文件
└── project.ioc                # STM32CubeMX 配置
```

## 🔧 功能模块

### 示波器 (Oscilloscope)
- ADC 高速采样 (最高 1MSPS)
- PGA 可编程增益放大 (MCP6S21)
- AC/DC 耦合切换
- 自动量程调节

### 万用表 (Multimeter)
- 电压测量 (DC/AC)
- 电流测量
- 电阻测量
- 自动量程

### 信号发生器 (SignalGenerator)
- DDS 波形生成
- 正弦/三角/方波输出
- 频率/幅值可调

### 数控电源 (PowerSupply)
- CC/CV 恒流恒压控制
- PD 快充协议支持
- 过压/过流/过温保护

## 📡 通信协议

与 ESP32-P4 通过 UART 通信，波特率 921600：

```
帧格式: [帧头][功能码][数据长度][数据][校验和]
        0xAA   1字节    2字节    N字节   1字节
```

功能码定义：
- `0x01-0x0F`: STM32 -> ESP32 数据上报
- `0x10-0x1F`: ESP32 -> STM32 控制命令
- `0xFE`: 心跳包
- `0xFF`: 应答包

## 🛠️ 编译环境

- Keil MDK v5.x
- STM32CubeMX (可选，用于重新配置外设)
- ST-Link 调试器

### 编译步骤

1. 使用 Keil MDK 打开 `MDK-ARM/project.uvprojx`
2. 点击 Build (F7) 编译
3. 连接 ST-Link，点击 Download 烧录

## 📌 硬件资源

| 外设 | 用途 |
|------|------|
| ADC1 | 示波器信号采集 |
| ADC2 | 万用表/电源测量 |
| DAC1 | 信号发生器输出 |
| TIM2 | ADC 采样触发 |
| TIM3 | DDS 波形生成 |
| USART1 | ESP32 通信 |
| SPI1 | PGA 控制 |

## 🔗 相关分支

- [main](../../tree/main) - 项目总览
- [esp32-firmware](../../tree/esp32-firmware) - ESP32 主控固件
- [hardware](../../tree/hardware) - 硬件设计资料
- [docs](../../tree/docs) - 项目文档
