# 🎓 毕业设计 - 基于ESP32的便携式多功能测量仪

[![GitHub](https://img.shields.io/badge/GitHub-Jackcdut-blue)](https://github.com/Jackcdut)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

## 📋 项目简介

本项目为本科毕业设计，设计并实现一款基于ESP32的便携式多功能测量仪器，集成以下功能：
- 📊 **数字示波器** - 实时波形采集与显示
- 📈 **信号发生器** - 基于AD9833的多波形输出
- 🔌 **数字万用表** - 电压/电流/电阻测量
- ⚡ **数控电源** - 可调稳压输出

## 🏗️ 仓库结构

```
Graduation-project-ESP32/
├── README.md                 # 项目说明文档
├── LICENSE                   # 开源许可证
├── .gitignore               # Git忽略配置
│
├── 📁 任务书/                # 毕业设计任务书
├── 📁 选题思路/              # 选题分析与设计框架
├── 📁 信息文档/              # 功能模块说明文档
│
├── 📁 毕业论文/              # 论文相关材料
│   ├── 开题/                # 开题报告
│   ├── 文献翻译/            # 外文翻译
│   ├── 参考文献/            # 参考论文
│   └── 图/                  # 论文插图
│
├── 📁 硬件设计/              # 硬件设计资料
│   ├── 原理图及PCB/         # 主板原理图与PCB
│   ├── 产品资料/            # 芯片数据手册
│   └── 模板工程/            # 各模块参考设计
│
└── 📁 软件设计/              # 软件代码
    ├── GUI_LVGL/            # LVGL图形界面
    └── Photo/               # 调试截图
```

## 🌿 分支说明

| 分支名称 | 说明 |
|---------|------|
| `main` | 主分支，包含完整项目文档和硬件设计 |
| `esp32-firmware` | ESP32主控固件代码（ESP-IDF框架） |
| `stm32-firmware` | STM32协处理器固件代码（HAL库） |

## 🛠️ 技术栈

### 硬件平台
- **主控芯片**: ESP32-S3 (双核240MHz, WiFi/BLE)
- **协处理器**: STM32F407 (高速ADC采集)
- **信号发生**: AD9833 DDS芯片
- **电源管理**: LM5176, SCT2432, SY7208

### 软件框架
- **ESP32**: ESP-IDF + LVGL图形库
- **STM32**: HAL库 + FreeRTOS
- **通信协议**: SPI/UART

## 📖 使用说明

### 克隆仓库
```bash
# 克隆主分支（文档和硬件）
git clone https://github.com/Jackcdut/Graduation-project-ESP32.git

# 切换到ESP32固件分支
git checkout esp32-firmware

# 切换到STM32固件分支
git checkout stm32-firmware
```

### 编译ESP32固件
```bash
cd 软件设计/ESP32
idf.py set-target esp32s3
idf.py build
idf.py flash
```

### 编译STM32固件
使用Keil MDK打开 `软件设计/STM32/MDK-ARM/*.uvprojx` 工程文件进行编译

## 📝 开发日志

- **2025.11** - 完成选题与开题报告
- **2025.12** - 完成硬件原理图设计与PCB打样
- **2026.01** - 软件开发与系统调试

## 👤 作者信息

- **姓名**: 纪帅
- **学号**: 202206010320
- **指导教师**: [指导教师姓名]
- **学校**: [学校名称]

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

> 💡 如有问题或建议，欢迎提交 Issue 或 Pull Request
