# ESP32-P4 毕业设计项目

ESP32-P4 + ESP32-C6 多功能开发板项目，包含LVGL GUI、OneNET云平台和Web仪表盘。

## 📁 版本管理

本仓库采用文件夹方式管理不同版本，每个版本都是一个独立的完整项目。

### 当前版本

- **[V2.0](./V2.0/)** - 最新版本（2026-01-18）
  - ✅ **无线串口功能完善** - 现在能够正常使用
  - ✅ **视频播放功能** - 支持AVI视频播放（复杂画面会有轻微卡顿）
  - 基于V1.0的所有功能
  - [查看V2.0详细说明](./V2.0/README.md)

- **[V1.0](./V1.0/)** - 首个正式版本
  - ESP32-P4 + ESP32-C6 双芯片架构
  - LVGL v8.3 全屏双缓冲显示
  - 网络模块（WiFi、OneNET、天气API）
  - 11个GUI界面屏幕
  - 示波器、无线串口、截图功能
  - Web数据可视化仪表盘
  - 性能优化（100 FPS理论帧率）

## 🚀 快速开始

进入对应版本文件夹查看详细文档：

```bash
cd V1.0
```

查看版本说明文档：
- [V1.0 版本说明](./V1.0/docs/README_V1.0.md)
- [GitHub使用指南](./V1.0/docs/GITHUB_GUIDE.md)
- [项目结构说明](./V1.0/docs/PROJECT_STRUCTURE.md)

## 📖 文档

每个版本文件夹内都包含完整的文档：
- 项目结构说明
- 网络模块详解
- 性能优化方案
- 使用指南

## 🔧 编译和烧录

### 主程序 (ESP32-P4)
```bash
cd V1.0
idf.py build
idf.py -p COM3 flash monitor
```

### 从机程序 (ESP32-C6)
```bash
cd V1.0
idf.py -B build_slave build
idf.py -B build_slave -p COM4 flash
```

## 📝 版本历史

| 版本 | 发布日期 | 主要特性 |
|------|---------|---------|
| V2.0 | 2026-01-18 | 无线串口完善、视频播放功能 |
| V1.0 | 2026-01-17 | 首个正式版本，功能完整 |

## 📄 许可证

本项目仅用于学习和研究目的。

---

**项目作者**: Jack  
**GitHub**: https://github.com/Jackcdut/Graduation-project-ESP32
