# 无线串口模块使用说明

## 功能概述

无线串口模块实现了ESP32-P4与ESP32-C6之间的WiFi串口透传功能，支持：
- TCP Server/Client双模式
- UART透传（GPIO51 TX, GPIO52 RX）
- 可配置串口参数（波特率、数据位、停止位、校验位）
- UI集成（发送/接收数据显示）

## 硬件连接

### UART透传引脚
- **TX**: GPIO51
- **RX**: GPIO52
- **波特率**: 115200（默认）
- **配置**: 8N1

### 外部设备连接示例（如STM32）
```
ESP32-P4 GPIO51 (TX) --> 外部设备 RX
ESP32-P4 GPIO52 (RX) <-- 外部设备 TX
ESP32-P4 GND         --> 外部设备 GND
```

## 使用方法

### 1. 启动TCP Server模式
```c
wireless_serial_start_server();  // 监听端口8888
```

### 2. 启动TCP Client模式
```c
wireless_serial_connect("192.168.1.100", 8888);
```

### 3. 启用UART透传
```c
wireless_serial_enable_uart_passthrough();
```

### 4. 发送数据
```c
const char *data = "Hello World";
wireless_serial_send(data, strlen(data));
```

### 5. 禁用UART透传
```c
wireless_serial_disable_uart_passthrough();
```

## 验证方法

### 方法1：WiFi透传测试（无需外部设备）

1. **启动设备**
   - 上电后进入主界面
   - 点击"无线串口"卡片进入功能界面

2. **连接WiFi**
   - 进入设置界面
   - 扫描并连接WiFi网络

3. **启动TCP Server**
   - 无线串口界面会自动启动Server模式
   - 监听端口：8888

4. **使用网络调试工具连接**
   - 工具推荐：NetAssist、SocketTest
   - 连接类型：TCP Client
   - 目标IP：ESP32的IP地址
   - 目标端口：8888

5. **测试数据收发**
   - 在UI发送框输入文本
   - 点击"发送"按钮
   - 网络工具应收到数据
   - 从网络工具发送数据
   - UI接收框应显示收到的数据

### 方法2：UART透传测试（需要外部设备）

1. **硬件连接**
   ```
   ESP32-P4 GPIO51 --> USB转串口模块 RX
   ESP32-P4 GPIO52 <-- USB转串口模块 TX
   ESP32-P4 GND    --> USB转串口模块 GND
   ```

2. **启用UART透传**
   - 代码已自动启用（在wireless_serial_init中）
   - 或手动调用：`wireless_serial_enable_uart_passthrough()`

3. **打开串口工具**
   - 工具推荐：PuTTY、SecureCRT、串口助手
   - 波特率：115200
   - 数据位：8
   - 停止位：1
   - 校验位：None

4. **测试双向透传**
   - **WiFi → UART**: 从网络工具发送数据，串口工具应收到
   - **UART → WiFi**: 从串口工具发送数据，网络工具应收到

### 方法3：完整透传测试（STM32示例）

1. **STM32端代码**
   ```c
   // STM32 UART配置：115200-8N1
   void uart_init(void) {
       // 配置UART1: 115200-8N1
       // TX: PA9, RX: PA10
   }
   
   void test_wireless_serial(void) {
       char tx_buf[] = "Hello from STM32\r\n";
       HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), 100);
       
       char rx_buf[128];
       HAL_UART_Receive(&huart1, (uint8_t*)rx_buf, 128, 1000);
   }
   ```

2. **连接示意**
   ```
   STM32 PA9 (TX) --> ESP32-P4 GPIO52 (RX)
   STM32 PA10 (RX) <-- ESP32-P4 GPIO51 (TX)
   STM32 GND --> ESP32-P4 GND
   ```

3. **测试流程**
   - STM32发送数据 → ESP32 UART接收 → WiFi转发 → 网络工具显示
   - 网络工具发送 → ESP32 WiFi接收 → UART转发 → STM32接收

## 调试信息

启用ESP-IDF日志查看运行状态：
```bash
idf.py monitor
```

关键日志标签：
- `WIRELESS_SERIAL`: 模块初始化和状态
- `WS_SERVER`: TCP Server事件
- `WS_CLIENT`: TCP Client事件
- `WS_UART`: UART透传事件

## API参考

### 初始化
```c
esp_err_t wireless_serial_init(void *rx_callback, void *status_callback);
```

### Server模式
```c
esp_err_t wireless_serial_start_server(void);
esp_err_t wireless_serial_stop_server(void);
```

### Client模式
```c
esp_err_t wireless_serial_connect(const char *ip, uint16_t port);
esp_err_t wireless_serial_disconnect(void);
```

### 数据收发
```c
esp_err_t wireless_serial_send(const uint8_t *data, size_t len);
```

### UART透传
```c
esp_err_t wireless_serial_enable_uart_passthrough(void);
esp_err_t wireless_serial_disable_uart_passthrough(void);
```

### 清理
```c
esp_err_t wireless_serial_deinit(void);
```

## 常见问题

### Q: 无法连接到TCP Server
**A**: 检查WiFi连接状态，确认ESP32已获取IP地址

### Q: UART无数据
**A**: 
1. 检查GPIO51/52引脚连接
2. 确认波特率匹配（115200）
3. 检查是否调用了`wireless_serial_enable_uart_passthrough()`

### Q: 数据丢失
**A**: 
1. 降低发送速率
2. 检查缓冲区大小（默认1024字节）
3. 确认WiFi信号强度

## 技术规格

- **WiFi协议**: TCP/IP
- **默认端口**: 8888
- **最大客户端**: 4
- **缓冲区大小**: 1024字节
- **UART配置**: 115200-8N1
- **UART引脚**: TX=GPIO51, RX=GPIO52

## 更新日志

### v1.0.0 (2026-01-17)
- ✅ 实现TCP Server/Client双模式
- ✅ 添加UART透传功能
- ✅ UI集成（发送/接收界面）
- ✅ 支持外部设备连接
