# 项目文件索引

## 按功能分类

### 🔧 嵌入式源码

#### 主程序 (main/)
| 文件 | 说明 | 依赖 |
|------|------|------|
| `main.c` | 程序入口 | LVGL, WiFi, GUI |

#### 网络通信模块 (main/network/) - 分层架构

**基础层 - WiFi连接管理**
| 文件 | 说明 | 功能 |
|------|------|------|
| `network/wifi_manager.c/h` | WiFi底层管理 | 扫描、连接、断开、凭证NVS存储、自动重连、RSSI获取 |

**应用层 - 网络服务**
| 文件 | 说明 | 功能 | 依赖 |
|------|------|------|------|
| `network/wifi_onenet.c/h` | OneNET云平台 | 设备上下线、属性上报、WiFi定位、SNTP时间同步 | wifi_manager |
| `network/weather_api.c/h` | 心知天气API | 获取实时天气、30分钟自动更新 | wifi_manager |

**UI功能模块 (BSP/GUIDER/custom/modules/)**
| 目录 | 说明 | 功能 |
|------|------|------|
| `wireless_serial/` | 无线串口模块 | TCP Socket通信、服务端/客户端模式、UI数据显示 |

#### GUI界面 (BSP/GUIDER/)
| 文件 | 说明 |
|------|------|
| `generated/gui_guider.c/h` | GUI主框架 |
| `generated/events_init.c/h` | 事件处理 |
| `generated/setup_scr_scrHome.c` | 主页界面 |
| `generated/setup_scr_scrOscilloscope.c` | 示波器界面 |
| `generated/setup_scr_scrPowerSupply.c` | 电源界面 |
| `generated/setup_scr_scrSettings.c` | 设置界面 |
| `generated/setup_scr_scrWirelessSerial.c` | 无线串口界面 |
| `generated/setup_scr_scrAIChat.c` | AI聊天界面 |
| `custom/custom.c/h` | 自定义逻辑 |

#### 功能模块 (BSP/GUIDER/custom/modules/)
| 目录 | 说明 |
|------|------|
| `oscilloscope/` | 示波器数据采集显示 |
| `cloud_manager/` | 云端数据管理 |
| `screenshot/` | 截图功能 |
| `boot_animation/` | 开机动画 |
| `sdcard_manager/` | SD卡管理 |
| `wireless_serial/` | 无线串口通信（TCP Socket） |
| `gallery/` | 图库浏览 |
| `fonts/` | 自定义字体 |
| `widgets/` | 自定义控件 |
| `media_player/` | 媒体播放器（PNG/BMP图片查看、AVI视频播放） |

---

### 🌐 Web前端 (html/)

#### 页面文件
| 文件 | 说明 |
|------|------|
| `index.html` | 主页/登录 |
| `cloud-dashboard.html` | 云端仪表盘 |
| `serial-debug.html` | 串口调试 |
| `exbug-tool.html` | 调试工具 |
| `data.html` | 数据展示 |
| `feedback.html` | 用户反馈 |

#### JavaScript (html/js/)
| 文件 | 说明 |
|------|------|
| `auth.js` / `auth-enhanced.js` | 用户认证 |
| `cloud-dashboard.js` | 仪表盘逻辑 |
| `onenet-auth.js` | OneNET认证 |
| `fft-analysis.js` | FFT分析 |
| `navigation.js` | 页面导航 |
| `sidebar.js` | 侧边栏 |
| `performance-optimizer.js` | 性能优化 |

#### 云函数 (html/unicloud-functions/)
| 目录 | 说明 |
|------|------|
| `onenet-verify/` | OneNET设备验证 |
| `send-verification-code/` | 发送验证码 |
| `user-register/` | 用户注册 |

---

### 📚 文档 (docs/)

| 文件 | 说明 |
|------|------|
| `PROJECT_STRUCTURE.md` | 项目结构说明 |
| `FILE_INDEX.md` | 本文件 - 文件索引 |
| `NETWORK_MODULES.md` | 网络通信模块详细说明 |
| `Technical_Roadmap.html` | 技术路线图 |
| `PERFORMANCE_OPTIMIZATION.md` | 性能优化 |
| `SCREEN_TEARING_SOLUTION.md` | 屏幕撕裂解决 |
| `EXTEND_SCREEN_USAGE.md` | 扩展屏幕使用 |
| `EXTEND_SCREEN_HYBRID_MODE.md` | 混合模式 |
| `CC_CV_Power_Supply_Diagram.html` | CC/CV电源图 |
| `DMM_Interface_Guide.md` | 数字万用表界面设计文档 |
| `OneNet_*.html` | OneNET流程图 |

---

### 📊 MATLAB仿真 (matlab/)

| 文件 | 说明 |
|------|------|
| `CC_CV_Power_Supply_Simulation.m` | CC/CV电源仿真脚本 |
| `CCCV_Power_Supply.slx` | Simulink模型 |
| `CCCV_Power_Supply_Model.slx` | 电源模型 |
| `Create_CCCV_Simulink_Model.m` | 创建模型脚本 |
| `DDS_Waveform_Visualization.m` | DDS波形可视化 |

---

### ⚙️ 配置文件

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | CMake主配置 |
| `sdkconfig` | ESP-IDF配置 |
| `sdkconfig.defaults` | 默认配置 |
| `partitions.csv` | 分区表 |
| `dependencies.lock` | 依赖锁定 |
| `.clangd` | Clangd配置 |
| `.gitignore` | Git忽略规则 |

---

### 🔨 构建输出

| 目录 | 说明 |
|------|------|
| `build/` | ESP32-P4主程序构建 |
| `build_slave/` | ESP32-C6从机构建 |

---

## 按开发任务分类

### WiFi/网络相关（分层）

**基础层**
- `main/network/wifi_manager.c/h` - WiFi底层管理

**应用层**
- `main/network/wifi_onenet.c/h` - OneNET云平台通信
- `main/network/weather_api.c/h` - 天气API

**UI功能模块**
- `BSP/GUIDER/custom/modules/wireless_serial/` - TCP无线串口

**Web前端**
- `html/js/onenet-auth.js` - OneNET认证
- `html/unicloud-functions/` - 云函数

### UI界面相关
- `BSP/GUIDER/generated/` (所有文件)
- `BSP/GUIDER/custom/custom.c/h`
- `BSP/GUIDER/custom/modules/`

### 示波器功能
- `BSP/GUIDER/generated/setup_scr_scrOscilloscope.c`
- `BSP/GUIDER/custom/modules/oscilloscope/`
- `html/js/fft-analysis.js`

### 云平台集成
- `main/network/wifi_onenet.c/h`
- `html/cloud-dashboard.html`
- `html/js/cloud-dashboard.js`
- `html/unicloud-functions/`
- `docs/OneNet_*.html`

### 电源管理
- `BSP/GUIDER/generated/setup_scr_scrPowerSupply.c`
- `matlab/CC_CV_Power_Supply_Simulation.m`
- `matlab/CCCV_Power_Supply*.slx`
- `docs/CC_CV_Power_Supply_Diagram.html`

---

*最后更新: 2025-12-27*
