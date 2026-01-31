# 网络通信模块说明

本文档详细说明 `main/` 目录下的网络通信相关文件。

---

## 模块架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        应用层 (Application)                      │
├───────────────────┬───────────────────┬─────────────────────────┤
│   wifi_onenet     │    weather_api    │    (UI Modules)         │
│   OneNET云平台    │    心知天气API    │    wireless_serial等    │
├───────────────────┴───────────────────┴─────────────────────────┤
│                        基础层 (Foundation)                       │
├─────────────────────────────────────────────────────────────────┤
│                         wifi_manager                             │
│                    WiFi连接管理 (ESP-Hosted)                     │
└─────────────────────────────────────────────────────────────────┘
```

> **注意**: `wireless_serial` 模块已移至 `BSP/GUIDER/custom/modules/wireless_serial/`，
> 因为它是与UI紧密耦合的功能模块，不属于纯网络通信层。

---

## 1. wifi_manager - WiFi底层管理

**文件**: `wifi_manager.c` / `wifi_manager.h`

**职责**: 提供WiFi连接的底层管理，是其他网络模块的基础。

### 主要功能

| 功能 | API | 说明 |
|------|-----|------|
| 初始化 | `wifi_manager_init()` | 初始化WiFi驱动和事件处理 |
| 扫描 | `wifi_manager_scan_start()` | 异步扫描周围WiFi热点 |
| 获取扫描结果 | `wifi_manager_get_scan_results()` | 获取扫描到的AP列表 |
| 连接 | `wifi_manager_connect()` | 连接指定WiFi |
| 断开 | `wifi_manager_disconnect()` | 断开当前连接 |
| 状态查询 | `wifi_manager_is_connected()` | 检查是否已连接 |
| 凭证存储 | `wifi_manager_save_credentials()` | 保存WiFi密码到NVS |
| 自动连接 | `wifi_manager_auto_connect()` | 使用保存的凭证自动连接 |

### 使用示例

```c
#include "wifi_manager.h"

// 初始化
wifi_manager_init();

// 连接WiFi
wifi_manager_connect("MyWiFi", "password123", status_callback);

// 检查连接状态
if (wifi_manager_is_connected()) {
    char ssid[33];
    wifi_manager_get_ssid(ssid, sizeof(ssid));
    printf("Connected to: %s\n", ssid);
}
```

---

## 2. wifi_onenet - OneNET云平台通信

**文件**: `wifi_onenet.c` / `wifi_onenet.h`

**职责**: 与中国移动OneNET物联网平台进行HTTP通信。

### 主要功能

| 功能 | API | 说明 |
|------|-----|------|
| 初始化 | `onenet_http_init()` | 初始化HTTP客户端 |
| 设备上线 | `onenet_device_online()` | 设置设备在线状态 |
| 设备下线 | `onenet_device_offline()` | 设置设备离线状态 |
| 属性上报 | `onenet_http_report_property()` | 上报单个属性 |
| 批量上报 | `onenet_http_report_properties()` | 上报多个属性(JSON) |
| WiFi定位 | `onenet_http_report_wifi_location()` | 上报WiFi定位数据 |
| 获取定位 | `onenet_get_wifi_location()` | 获取定位结果 |
| 时间同步 | `onenet_sync_time()` | SNTP网络时间同步 |

### 配置参数

```c
#define ONENET_HTTP_PRODUCT_ID  "FCwDzD6VU0"           // 产品ID
#define ONENET_ACCESS_KEY       "74ttu7Ofi9n3Z7dfT6dfkctJR8E3xyVxFXdX71/Hs4k="  // Access Key
#define ONENET_HTTP_API_HOST    "open.iot.10086.cn"   // API服务器
```

### 使用示例

```c
#include "wifi_onenet.h"

// 初始化
onenet_http_init();

// 设备上线
onenet_device_online();

// 上报温度属性
onenet_http_report_property("temperature", "25.5");

// 上报多个属性
onenet_http_report_properties("\"temp\":{\"value\":25.5},\"humidity\":{\"value\":60}");
```

---

## 3. weather_api - 心知天气API

**文件**: `weather_api.c` / `weather_api.h`

**职责**: 获取实时天气数据，支持定时自动更新。

### 主要功能

| 功能 | API | 说明 |
|------|-----|------|
| 初始化 | `weather_api_init()` | 初始化天气模块 |
| 启动自动更新 | `weather_api_start_auto_update()` | 每30分钟自动获取天气 |
| 停止自动更新 | `weather_api_stop_auto_update()` | 停止定时更新 |
| 立即获取 | `weather_api_fetch_now()` | 立即获取一次天气 |
| 获取缓存数据 | `weather_api_get_last_data()` | 获取最后一次的天气数据 |

### 配置参数

```c
#define SENIVERSE_API_KEY       "SJ-R69u0cq-9v19ew"   // 心知天气私钥
#define SENIVERSE_LOCATION      "chengdu"             // 城市（拼音）
#define SENIVERSE_LANGUAGE      "en"                  // 语言
#define WEATHER_UPDATE_INTERVAL_MS  (30 * 60 * 1000)  // 更新间隔30分钟
```

### 天气数据结构

```c
typedef struct {
    bool valid;                 // 数据是否有效
    char location_name[64];     // 位置名称
    char text[32];              // 天气现象（晴、多云等）
    int temperature;            // 温度（摄氏度）
    int feels_like;             // 体感温度
    int humidity;               // 相对湿度（0-100%）
    char wind_direction[16];    // 风向
    int wind_scale;             // 风力等级
} weather_info_t;
```

### 使用示例

```c
#include "weather_api.h"

// 天气更新回调
void on_weather_update(const weather_info_t *weather) {
    printf("天气: %s, 温度: %d°C\n", weather->text, weather->temperature);
}

// 初始化并启动自动更新
weather_api_init(on_weather_update);
weather_api_start_auto_update();
```

---

## 4. wireless_serial - TCP无线串口

**文件**: `wireless_serial.c` / `wireless_serial.h`

**职责**: 通过TCP Socket实现设备间的无线串口通信。

### 主要功能

| 功能 | API | 说明 |
|------|-----|------|
| 初始化 | `wireless_serial_init()` | 初始化无线串口 |
| 启动服务端 | `wireless_serial_start_server()` | ESP32-P4作为服务端 |
| 停止服务端 | `wireless_serial_stop_server()` | 停止服务端 |
| 连接服务端 | `wireless_serial_connect()` | 作为客户端连接 |
| 断开连接 | `wireless_serial_disconnect()` | 断开连接 |
| 发送数据 | `wireless_serial_send()` | 发送数据 |
| 状态查询 | `wireless_serial_is_connected()` | 检查连接状态 |

### 配置参数

```c
#define WIRELESS_SERIAL_PORT        8888    // TCP端口
#define WIRELESS_SERIAL_MAX_CLIENTS 4       // 最大客户端数
#define WIRELESS_SERIAL_BUFFER_SIZE 1024    // 缓冲区大小
```

### 工作模式

1. **服务端模式** (ESP32-P4)
   - 监听TCP端口8888
   - 接受客户端连接
   - 接收/发送数据

2. **客户端模式**
   - 连接到指定IP的服务端
   - 双向数据传输

### 使用示例

```c
#include "wireless_serial.h"

// 数据接收回调
void on_data_received(const uint8_t *data, size_t len) {
    printf("Received: %.*s\n", (int)len, data);
}

// 初始化
wireless_serial_init(on_data_received, NULL);

// 启动服务端
wireless_serial_start_server();

// 发送数据
wireless_serial_send((uint8_t*)"Hello", 5);
```

---

## 模块依赖关系

```
main.c
  │
  ├── wifi_manager_init()          // 初始化WiFi
  │
  └── custom_init() (in custom.c)
        │
        ├── onenet_http_init()     // 初始化OneNET
        ├── weather_api_init()     // 初始化天气API
        └── wireless_serial_init() // 初始化无线串口
```

---

## 相关UI界面

| 模块 | 对应UI界面 |
|------|-----------|
| wifi_manager | `scrSettings` - WiFi设置页面 |
| wifi_onenet | `scrHome` - 主页云端状态显示 |
| weather_api | `scrHome` - 主页天气显示 |
| wireless_serial | `scrWirelessSerial` - 无线串口界面 |

---

## 已知问题修复记录

### SD卡管理页面斜向切换问题 (2025-01-08)

**问题**: 在设置页面点击SD卡管理按钮时，SD卡管理UI出现斜向切换效果。

**原因**: SD卡管理UI创建在`lv_scr_act()`上时，使用`lv_obj_set_pos(g_screen, 0, 0)`设置位置，但由于设置页面的右侧面板有滑动动画，导致位置计算不正确。

**修复**: 
- 使用`lv_obj_align(g_screen, LV_ALIGN_TOP_LEFT, 0, 0)`替代`lv_obj_set_pos`
- 添加`lv_obj_move_foreground(g_screen)`确保UI在最前面

**文件**: `BSP/GUIDER/custom/modules/sdcard_manager/sdcard_manager_ui.c`

---

*最后更新: 2025-01-08*
