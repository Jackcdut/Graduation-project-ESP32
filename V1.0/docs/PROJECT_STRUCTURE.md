# ESP32-P4 多功能开发板项目结构说明

## 项目概述

本项目是基于 ESP32-P4 + ESP32-C6 的多功能开发板，集成了示波器、电源管理、WiFi通信、云平台对接等功能。

---

## 目录结构

```
ESP32-P4-Project/
├── main/                          # 🔧 主程序源码
│   ├── main.c                     # 程序入口
│   └── network/                   # 网络通信模块
│       ├── wifi_manager.[c/h]     # WiFi管理模块
│       ├── wifi_onenet.[c/h]      # OneNET云平台通信
│       └── weather_api.[c/h]      # 心知天气API
│
├── BSP/                           # 🖥️ 板级支持包
│   ├── common_components/         # 通用组件
│   │   ├── bsp_extra/             # BSP扩展
│   │   ├── espressif__esp_lcd_st7701/  # LCD驱动
│   │   └── espressif__esp32_p4_function_ev_board/  # 开发板BSP
│   │
│   └── GUIDER/                    # GUI设计器生成代码
│       ├── generated/             # 自动生成的UI代码
│       │   ├── gui_guider.[c/h]   # GUI主框架
│       │   ├── events_init.[c/h]  # 事件处理
│       │   ├── setup_scr_*.c      # 各屏幕界面
│       │   ├── guider_fonts/      # 字体资源
│       │   └── images/            # 图片资源
│       │
│       └── custom/                # 自定义UI代码
│           ├── custom.[c/h]       # 自定义逻辑
│           └── modules/           # 功能模块
│               ├── boot_animation/    # 开机动画
│               ├── cloud_manager/     # 云端管理
│               ├── oscilloscope/      # 示波器模块
│               ├── screenshot/        # 截图功能
│               ├── sdcard_manager/    # SD卡管理
│               ├── gallery/           # 图库
│               ├── fonts/             # 字体
│               ├── widgets/           # 自定义控件
│               └── wireless_serial/   # 无线串口模块
│
├── html/                          # 🌐 Web前端
│   ├── index.html                 # 主页
│   ├── cloud-dashboard.html       # 云端仪表盘
│   ├── serial-debug.html          # 串口调试工具
│   ├── exbug-tool.html            # 调试工具
│   ├── data.html                  # 数据页面
│   ├── feedback.html              # 反馈页面
│   ├── css/                       # 样式文件
│   ├── js/                        # JavaScript脚本
│   │   ├── auth*.js               # 认证相关
│   │   ├── cloud-dashboard.js     # 云端仪表盘
│   │   ├── onenet-auth.js         # OneNET认证
│   │   └── ...
│   ├── libs/                      # 第三方库
│   ├── img/                       # 图片资源
│   ├── aliyun-fc/                 # 阿里云函数计算
│   └── unicloud-functions/        # UniCloud云函数
│       ├── onenet-verify/         # OneNET验证
│       ├── send-verification-code/ # 发送验证码
│       └── user-register/         # 用户注册
│
├── docs/                          # 📚 项目文档
│   ├── PROJECT_STRUCTURE.md       # 本文件 - 项目结构说明
│   ├── Technical_Roadmap.html     # 技术路线图
│   ├── PERFORMANCE_OPTIMIZATION.md # 性能优化指南
│   ├── SCREEN_TEARING_SOLUTION.md # 屏幕撕裂解决方案
│   ├── EXTEND_SCREEN_*.md         # 扩展屏幕文档
│   └── OneNet_*.html              # OneNET相关流程图
│
├── matlab/                        # 📊 MATLAB仿真
│   ├── CC_CV_Power_Supply_Simulation.m  # CC/CV电源仿真
│   ├── CCCV_Power_Supply*.slx     # Simulink模型
│   ├── DDS_Waveform_Visualization.m # DDS波形可视化
│   └── slprj/                     # Simulink项目文件
│
├── managed_components/            # 📦 ESP-IDF托管组件
│   ├── espressif__esp_lvgl_port/  # LVGL移植
│   ├── espressif__esp_hosted/     # ESP-Hosted (WiFi)
│   ├── espressif__esp_lcd_*/      # LCD驱动
│   ├── lvgl__lvgl/                # LVGL图形库
│   └── ...
│
├── build/                         # 🔨 主构建输出 (ESP32-P4)
├── build_slave/                   # 🔨 从机构建输出 (ESP32-C6)
│
├── .kiro/                         # ⚙️ Kiro IDE配置
├── .vscode/                       # ⚙️ VSCode配置
│
├── CMakeLists.txt                 # CMake主配置
├── sdkconfig                      # ESP-IDF SDK配置
├── partitions.csv                 # 分区表
├── dependencies.lock              # 依赖锁定文件
└── JC4880P443_C6.bin              # ESP32-C6固件
```

---

## 功能模块说明

### 1. 核心功能 (main/)

| 文件 | 功能 |
|------|------|
| `main.c` | 程序入口，初始化显示、WiFi、UI |

#### 网络通信模块 (main/network/)

网络相关代码已整理到 `main/network/` 子文件夹，采用分层架构：

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                    │
├─────────────────┬─────────────────┬─────────────────────────┤
│  wifi_onenet    │   weather_api   │   (UI Modules)          │
│  OneNET云平台   │   心知天气API   │   wireless_serial等     │
│  - 设备上下线   │   - 实时天气    │   - TCP无线串口         │
│  - 属性上报     │   - 定时更新    │   - 示波器/截图等       │
│  - WiFi定位     │                 │                         │
│  - SNTP时间同步 │                 │                         │
├─────────────────┴─────────────────┴─────────────────────────┤
│                      基础层 (Foundation)                     │
├─────────────────────────────────────────────────────────────┤
│                      wifi_manager                            │
│                   WiFi连接管理 (ESP-Hosted)                  │
│  - WiFi扫描/连接/断开  - 凭证保存(NVS)  - 自动重连          │
└─────────────────────────────────────────────────────────────┘
```

| 文件 | 层级 | 功能说明 |
|------|------|----------|
| `wifi_manager.[c/h]` | 基础层 | WiFi底层管理：扫描、连接、断开、凭证存储(NVS)、自动重连 |
| `wifi_onenet.[c/h]` | 应用层 | OneNET云平台：设备上下线、属性上报、WiFi定位、SNTP时间同步 |
| `weather_api.[c/h]` | 应用层 | 心知天气API：获取实时天气、30分钟自动更新 |

### 2. GUI界面 (BSP/GUIDER/)

- **generated/**: GUI Guider工具生成的代码，包含各屏幕界面
  - `scrHome` - 主页
  - `scrOscilloscope` - 示波器
  - `scrPowerSupply` - 电源管理
  - `scrSettings` - 设置
  - `scrWirelessSerial` - 无线串口
  - `scrAIChat` - AI聊天
  - `scrPrintMenu/Internet` - 打印功能
  - `scrScan/ScanFini` - 扫描功能

- **custom/modules/**: 自定义功能模块
  - `oscilloscope/` - 示波器数据采集与显示
  - `cloud_manager/` - 云端数据同步
  - `screenshot/` - 三指截图功能
  - `boot_animation/` - 开机动画

### 3. Web前端 (html/)

| 页面 | 功能 |
|------|------|
| `index.html` | 主页/登录页 |
| `cloud-dashboard.html` | 云端数据仪表盘 |
| `serial-debug.html` | Web串口调试工具 |
| `exbug-tool.html` | 设备调试工具 |

### 4. 云函数 (html/unicloud-functions/)

- `onenet-verify/` - OneNET设备验证
- `send-verification-code/` - 发送验证码
- `user-register/` - 用户注册

---

## 硬件架构

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32-P4 (主控)                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │   LVGL UI   │  │  示波器ADC  │  │   SD卡存储  │     │
│  │  800x480    │  │   数据采集  │  │   截图/数据 │     │
│  └─────────────┘  └─────────────┘  └─────────────┘     │
│                         │                               │
│                    SPI/SDIO                             │
│                         │                               │
│  ┌─────────────────────────────────────────────────┐   │
│  │              ESP32-C6 (WiFi从机)                 │   │
│  │  ┌─────────┐  ┌─────────────┐  ┌─────────────┐ │   │
│  │  │  WiFi   │  │   OneNET    │  │   天气API   │ │   │
│  │  │  管理   │  │   云平台    │  │   心知天气  │ │   │
│  │  └─────────┘  └─────────────┘  └─────────────┘ │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 编译主程序 (ESP32-P4)
```bash
idf.py build
idf.py -p COM3 flash monitor
```

### 编译从机程序 (ESP32-C6)
```bash
idf.py -B build_slave build
idf.py -B build_slave -p COM4 flash
```

### Web前端开发
```bash
cd html
# 使用任意HTTP服务器
python -m http.server 8080
```

---

## 相关文档

- [性能优化指南](PERFORMANCE_OPTIMIZATION.md)
- [屏幕撕裂解决方案](SCREEN_TEARING_SOLUTION.md)
- [扩展屏幕使用说明](EXTEND_SCREEN_USAGE.md)
- [技术路线图](Technical_Roadmap.html)
- [OneNET集成流程](OneNet_Complete_Flowchart_Paper.html)

---

*最后更新: 2025-12-27*
