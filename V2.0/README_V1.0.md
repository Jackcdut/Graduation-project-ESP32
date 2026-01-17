# ESP32-P4 多功能开发板 V1.0

基于 ESP32-P4 + ESP32-C6 的嵌入式开发板项目，集成示波器、电源管理、WiFi通信、OneNET云平台等功能。

## 版本信息

**版本**: V1.0  
**发布日期**: 2026-01-17  
**状态**: ✅ 功能完整，可用于生产

---

## 硬件平台

- **主控芯片**: ESP32-P4 (双核，400MHz)
- **WiFi模块**: ESP32-C6 (ESP-Hosted协议)
- **显示屏**: 800x480 LCD (全屏双缓冲)
- **存储**: SD卡 (截图、数据导出)
- **内存**: 32MB PSRAM

---

## 技术栈

| 组件 | 版本 | 说明 |
|------|------|------|
| ESP-IDF | v5.x | 开发框架 |
| LVGL | v8.3 | GUI图形库 |
| ESP-Hosted | v1.0.3 | WiFi通信协议 |
| OneNET | HTTP API | 云平台集成 |
| Chart.js | Latest | Web数据可视化 |

---

## V1.0 已完成功能

### ✅ 核心系统
- [x] ESP32-P4双核架构优化 (Core 0: 业务逻辑, Core 1: LVGL渲染)
- [x] 全屏双缓冲显示系统 (1.5MB PSRAM, 消除撕裂)
- [x] 性能优化 (100 FPS理论帧率, IRAM优化)
- [x] 开机动画与启动流程
- [x] SD卡文件系统支持

### ✅ 网络通信
- [x] WiFi管理模块 (扫描、连接、NVS凭证存储、自动重连)
- [x] OneNET云平台HTTP集成
  - 设备上下线管理
  - 属性数据上报
  - WiFi定位服务
  - SNTP时间同步
- [x] 心知天气API (30分钟自动更新)
- [x] ESP-Hosted WiFi协处理器通信

### ✅ GUI界面 (11个屏幕)
- [x] scrHome - 主页 (天气、时间、系统状态)
- [x] scrOscilloscope - 示波器界面
- [x] scrPowerSupply - 电源管理界面
- [x] scrSettings - 系统设置
- [x] scrWirelessSerial - 无线串口调试
- [x] scrAIChat - AI聊天界面
- [x] scrPrintMenu/Internet - 打印功能
- [x] scrScan/ScanFini - 扫描功能
- [x] 自定义事件处理与UI逻辑

### ✅ 功能模块
- [x] **示波器模块**
  - 数据采集与实时显示
  - CSV格式导出到SD卡
  - 波形和FFT数据支持
- [x] **无线串口**
  - TCP Socket通信 (端口8888)
  - 服务端/客户端模式
  - 多客户端连接支持
- [x] **截图功能**
  - 三指手势触发
  - BMP格式保存到SD卡
  - 自动文件命名
- [x] **云端数据管理**
  - 实时数据同步
  - 历史数据查询
- [x] **SD卡管理**
  - 文件浏览
  - 数据导出

### ✅ Web前端
- [x] 云端仪表盘 (cloud-dashboard.html)
- [x] OneNET数据可视化
- [x] 实时设备状态监控
- [x] 历史数据分析与图表
- [x] Chart.js多图表展示
- [x] 响应式设计

### ✅ 文档系统
- [x] 项目结构说明 (PROJECT_STRUCTURE.md)
- [x] 文件索引 (FILE_INDEX.md)
- [x] 网络模块详解 (NETWORK_MODULES.md)
- [x] 性能优化方案 (PERFORMANCE_OPTIMIZATION.md)
- [x] 技术路线图与流程图

---

## 性能指标

| 指标 | 数值 | 说明 |
|------|------|------|
| 理论帧率 | 100 FPS | 实际受限于屏幕60Hz |
| 显示延迟 | <10ms | LVGL刷新周期 |
| 触摸响应 | 20ms | 输入设备读取周期 |
| WiFi连接 | <3s | 自动重连支持 |
| 内存使用 | 1.5MB PSRAM | 双缓冲 |
| CPU占用 | <30% | 双核并行 |

---

## 快速开始

### 环境要求
- ESP-IDF v5.x
- Python 3.8+
- Git

### 编译主程序 (ESP32-P4)
```bash
cd /path/to/lvgl_demo_v8
idf.py build
idf.py -p COM3 flash monitor
```

### 编译从机程序 (ESP32-C6)
```bash
idf.py -B build_slave build
idf.py -B build_slave -p COM4 flash
```

### Web前端
```bash
cd html
python -m http.server 8080
# 访问 http://localhost:8080
```

---

## 项目结构

```
lvgl_demo_v8/
├── main/                    # 主程序源码
│   ├── main.c              # 程序入口
│   └── network/            # 网络通信模块
├── BSP/                    # 板级支持包
│   └── GUIDER/             # GUI界面代码
│       ├── generated/      # GUI Guider生成代码
│       └── custom/         # 自定义功能模块
├── html/                   # Web前端
│   ├── cloud-dashboard.html
│   └── js/                 # JavaScript脚本
├── docs/                   # 项目文档
├── matlab/                 # MATLAB仿真
└── managed_components/     # ESP-IDF组件
```

---

## 依赖组件

```yaml
lvgl/lvgl: 8.3.*
espressif/esp_hosted: ^1.0.3
espressif/esp_wifi_remote: ^0.4.1
espressif/esp_tinyusb: ^1.7.2
espressif/avi_player: *
esp_driver_jpeg: *
```

---

## 配置说明

### WiFi配置
在设置界面配置WiFi SSID和密码，凭证将保存到NVS。

### OneNET配置
修改 `main/network/wifi_onenet.h`:
```c
#define ONENET_HTTP_PRODUCT_ID  "your_product_id"
#define ONENET_ACCESS_KEY       "your_access_key"
```

### 天气API配置
修改 `main/network/weather_api.h`:
```c
#define SENIVERSE_API_KEY       "your_api_key"
#define SENIVERSE_LOCATION      "your_city"
```

---

## 已知问题

无重大已知问题。

---

## 后续计划 (V1.1+)

- [ ] 添加更多传感器支持
- [ ] 优化功耗管理
- [ ] 增强AI聊天功能
- [ ] 支持OTA固件升级
- [ ] 添加更多云平台支持

---

## 贡献者

- 主要开发者: [Your Name]
- 项目类型: 毕业设计

---

## 许可证

本项目仅用于学习和研究目的。

---

## 联系方式

如有问题或建议，请通过以下方式联系：
- Email: [your-email@example.com]
- GitHub Issues: [repository-url]/issues

---

**最后更新**: 2026-01-17  
**版本**: V1.0
