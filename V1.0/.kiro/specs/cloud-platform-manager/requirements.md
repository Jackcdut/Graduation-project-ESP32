# Requirements Document

## Introduction

本文档定义了设置页面中云平台管理功能的 UI 重新设计和功能完善需求。在前期 UI 布局优化的基础上，本次迭代主要解决以下问题：
1. 设备激活后显示的设备ID不正确（显示数字而非真实的字符串ID如 FCwDzD6VU0）
2. 激活后页面部分布局空白，信息展示不充分
3. Connect 按钮点击后无响应，设备未连接到 OneNET
4. 手动上传功能需要实现完整的 CSV 数据上传逻辑和美观的交互界面

## Glossary

- **Cloud Manager UI**: 云平台管理用户界面，用于设备激活和云端数据同步管理
- **Device ID**: 设备在 OneNET 平台上的唯一标识符（字符串格式，如 FCwDzD6VU0）
- **OneNET**: 中国移动物联网开放平台，提供 MQTT 和 HTTP API 服务
- **MQTT**: 消息队列遥测传输协议，用于设备与云平台的实时通信
- **CSV Upload**: 将示波器导出的 CSV 格式数据上传到云平台
- **Activated Panel**: 设备激活成功后显示的管理面板
- **QR Panel**: 二维码面板，显示 WiFi 连接、激活 URL 和设备码的二维码
- **LVGL**: Light and Versatile Graphics Library，嵌入式图形库
- **NVS**: Non-Volatile Storage，ESP32 的非易失性存储

## Requirements

### Requirement 1

**User Story:** As a user, I want to see the correct device ID after activation, so that I can identify my device on the OneNET platform.

#### Acceptance Criteria

1. WHEN the device is activated successfully THEN the system SHALL display the real device ID string (e.g., FCwDzD6VU0) returned from OneNET API
2. WHEN parsing the OneNET API response THEN the system SHALL correctly handle the device ID regardless of whether it is returned as a string or number format
3. WHEN the device ID is displayed THEN the system SHALL store the device ID in NVS for persistence across reboots
4. WHEN the activated panel is shown THEN the system SHALL retrieve and display the stored device ID from NVS if available

### Requirement 2

**User Story:** As a user, I want the activated panel to display comprehensive device information, so that I can monitor my device status at a glance.

#### Acceptance Criteria

1. WHEN the activated panel is displayed THEN the system SHALL show device information including: Device ID, Device Name, Product ID, and Activation Time
2. WHEN the activated panel is displayed THEN the system SHALL show connection status with visual indicators (green for connected, red for disconnected)
3. WHEN the activated panel is displayed THEN the system SHALL show sync statistics including: total uploads, last sync time, and data points count
4. WHEN the panel layout is rendered THEN the system SHALL utilize the full available space (520x370 pixels) with a card-based design
5. WHEN device information changes THEN the system SHALL update the display in real-time without requiring manual refresh

### Requirement 3

**User Story:** As a user, I want the Connect button to establish a real MQTT connection to OneNET, so that my device can communicate with the cloud platform.

#### Acceptance Criteria

1. WHEN the Connect button is clicked THEN the system SHALL initiate an MQTT connection to OneNET using the device credentials
2. WHEN the connection is in progress THEN the system SHALL display a loading indicator and disable the button
3. WHEN the connection succeeds THEN the system SHALL update the button text to "Disconnect" and show a success indicator
4. WHEN the connection fails THEN the system SHALL display an error message with the failure reason
5. WHEN the Disconnect button is clicked THEN the system SHALL gracefully close the MQTT connection
6. WHEN the connection state changes THEN the system SHALL update the OneNET status label accordingly

### Requirement 4

**User Story:** As a user, I want to manually upload CSV data files from SD card to OneNET, so that I can sync my oscilloscope measurements to the cloud.

#### Acceptance Criteria

1. WHEN the Upload button is clicked THEN the system SHALL display a file browser popup showing CSV files from the SD card
2. WHEN a CSV file is selected THEN the system SHALL parse the file and display file information (size, data points count, date range)
3. WHEN the upload is confirmed THEN the system SHALL show a progress popup with animated progress bar and status text
4. WHEN uploading data THEN the system SHALL send data points to OneNET using the MQTT protocol in batches
5. WHEN the upload completes THEN the system SHALL display a success message with upload statistics
6. WHEN the upload fails THEN the system SHALL display an error message and offer retry option

### Requirement 5

**User Story:** As a user, I want the upload interface to have beautiful animations and visual feedback, so that the interaction feels smooth and professional.

#### Acceptance Criteria

1. WHEN the upload progress popup is displayed THEN the system SHALL show an animated circular progress indicator
2. WHEN data is being uploaded THEN the system SHALL display real-time statistics (bytes sent, points uploaded, estimated time)
3. WHEN the upload status changes THEN the system SHALL use smooth transitions and color changes to indicate progress
4. WHEN the upload completes successfully THEN the system SHALL display a success animation with checkmark icon
5. WHEN the upload fails THEN the system SHALL display an error animation with appropriate visual feedback

### Requirement 6

**User Story:** As a user, I want the CSV parser to correctly handle oscilloscope data format, so that my measurements are accurately uploaded.

#### Acceptance Criteria

1. WHEN parsing a CSV file THEN the system SHALL support standard oscilloscope export format with timestamp and value columns
2. WHEN parsing CSV data THEN the system SHALL handle various delimiter formats (comma, semicolon, tab)
3. WHEN parsing CSV data THEN the system SHALL skip header rows and handle numeric values with decimal points
4. WHEN invalid data is encountered THEN the system SHALL log the error and continue processing valid rows
5. WHEN the CSV file is large THEN the system SHALL process data in chunks to avoid memory issues

### Requirement 7

**User Story:** As a user, I want the device credentials to be properly managed, so that the device can reconnect automatically after reboot.

#### Acceptance Criteria

1. WHEN the device is activated THEN the system SHALL store device ID, device name, and sec_key in NVS
2. WHEN the application starts THEN the system SHALL check NVS for existing activation data
3. WHEN valid activation data exists THEN the system SHALL display the activated panel with stored information
4. WHEN connecting to OneNET THEN the system SHALL use the stored credentials to authenticate

### Requirement 8 (Preserved from Previous)

**User Story:** As a user, I want the QR codes to be properly spaced and aligned, so that I can easily scan each QR code without confusion.

#### Acceptance Criteria

1. WHEN the cloud manager UI displays multiple QR codes THEN the system SHALL maintain a minimum spacing of 15 pixels between adjacent QR code containers
2. WHEN the QR codes are rendered THEN the system SHALL align all three QR codes horizontally with equal spacing distribution
3. WHEN the QR code panel is displayed THEN the system SHALL ensure each QR code container has consistent dimensions of 110x110 pixels

### Requirement 9 (Preserved from Previous)

**User Story:** As a user, I want the help/instructions popup to be fully visible within the screen, so that I can read all the activation instructions without any content being cut off.

#### Acceptance Criteria

1. WHEN the help popup is opened THEN the system SHALL display the popup with a fixed size that fits within the 800x480 screen resolution
2. WHEN the help popup content exceeds the visible area THEN the system SHALL provide a scrollable container with vertical scrollbar
3. WHEN the help popup is displayed THEN the system SHALL center the popup on the screen with a semi-transparent overlay background
4. WHEN the help popup is displayed THEN the system SHALL apply the same visual style as the SD card manager select folder popup including border radius of 16 pixels and white background

