# ESP32-P4 毕业设计项目 - V4.0

基于 ESP32-P4 + ESP32-C6 的多功能开发板项目，V4.0 版本主要优化了UI界面和云平台管理功能。

## 📋 V4.0 版本更新内容

### 1. 界面美化
- **电源页面美化**: 重新设计数控电源界面，优化视觉效果和用户体验
- **设置页面美化**: 改进设置页面布局和样式，提升整体一致性

### 2. 数控电源功能优化
- **逻辑优化**: 优化数控电源页面的控制逻辑，提高响应速度和稳定性
- **About界面更新**: 更换About界面显示内容，展示更详细的项目信息

### 3. 云平台管理改进
- **激活状态重置**: 将云平台管理页面改为未激活状态，方便演示和测试
- **二维码显示修复**: 启用LVGL QR Code组件，WiFi和URL现在以二维码形式显示（需重新编译）

## 🔧 编译说明

### 主程序 (ESP32-P4)
```bash
cd V4.0
idf.py build
idf.py -p COM3 flash monitor
```

### 从机程序 (ESP32-C6)
```bash
cd V4.0
idf.py -B build_slave build
idf.py -B build_slave -p COM4 flash
```

## 📁 项目结构

```
V4.0/
├── BSP/                    # 板级支持包
│   ├── GUIDER/            # GUI Guider生成的界面代码
│   │   ├── custom/        # 自定义模块
│   │   │   └── modules/   # 功能模块
│   │   │       ├── cloud_manager/  # 云平台管理
│   │   │       ├── oscilloscope/   # 示波器
│   │   │       └── ...
│   │   └── generated/     # 自动生成的界面代码
│   └── common_components/ # 通用组件
├── main/                   # 主程序
│   └── network/           # 网络模块
├── html/                   # Web仪表盘
├── docs/                   # 文档
└── sdkconfig              # ESP-IDF配置
```

## ✨ 主要功能

- **示波器**: 支持FFT频谱分析、数据导出
- **数控电源**: CC/CV模式控制
- **无线串口**: ESP32-C6 WiFi透传
- **云平台**: OneNET物联网平台集成
- **Web仪表盘**: 实时数据可视化

## 📝 配置说明

### 启用二维码显示
在 `sdkconfig` 中已启用 `CONFIG_LV_USE_QRCODE=y`，重新编译后云平台管理页面的WiFi和URL将以二维码形式显示。

## 📄 许可证

本项目仅用于学习和研究目的。

---

**项目作者**: Jack  
**GitHub**: https://github.com/Jackcdut/Graduation-project-ESP32
