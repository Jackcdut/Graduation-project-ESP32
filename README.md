# ESP32-P4 多功能开发板

基于 ESP32-P4 + ESP32-C6 的嵌入式开发板项目，集成示波器、电源管理、WiFi通信、OneNET云平台等功能。

## 快速导航

| 目录 | 说明 |
|------|------|
| [main/](main/) | 主程序源码 |
| [BSP/GUIDER/](BSP/GUIDER/) | GUI界面代码 |
| [html/](html/) | Web前端 |
| [docs/](docs/) | 项目文档 |
| [matlab/](matlab/) | MATLAB仿真 |

## 文档

- [项目结构说明](docs/PROJECT_STRUCTURE.md) - 完整的目录结构和模块说明
- [文件索引](docs/FILE_INDEX.md) - 按功能分类的文件索引
- [性能优化](docs/PERFORMANCE_OPTIMIZATION.md)
- [技术路线图](docs/Technical_Roadmap.html)

## 编译

```bash
# ESP32-P4 主程序
idf.py build
idf.py -p COM3 flash monitor

# ESP32-C6 从机
idf.py -B build_slave build
```

## 硬件

- 主控: ESP32-P4 (LVGL UI, 示波器ADC)
- WiFi: ESP32-C6 (ESP-Hosted)
- 显示: 800x480 LCD
- 存储: SD卡

---

*ESP-IDF v5.x | LVGL v8.3*
