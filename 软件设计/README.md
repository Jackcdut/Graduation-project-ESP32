# 💻 软件设计

本目录包含毕业设计的软件代码和调试资料。

## 📁 目录结构

```
软件设计/
├── GUI_LVGL/       # LVGL图形界面资源
├── Photo/          # 调试过程截图
└── 调试记录.docx   # 调试日志文档
```

## 🌿 代码分支说明

由于ESP32和STM32代码体量较大，分别存放在独立分支中：

### ESP32固件 (`esp32-firmware` 分支)
- **框架**: ESP-IDF v5.x
- **图形库**: LVGL v8.x
- **主要功能**:
  - WiFi/BLE通信
  - LVGL图形界面
  - 与STM32通信
  - 数据处理与显示

### STM32固件 (`stm32-firmware` 分支)
- **框架**: STM32 HAL库
- **IDE**: Keil MDK-ARM
- **主要功能**:
  - 高速ADC采集
  - DDS信号发生控制
  - 数控电源控制
  - 与ESP32通信

## 🔄 切换代码分支

```bash
# 查看ESP32代码
git checkout esp32-firmware

# 查看STM32代码
git checkout stm32-firmware

# 返回主分支
git checkout main
```

## 📸 调试截图

`Photo/` 目录包含开发过程中的示波器截图和调试记录。
