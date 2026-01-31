# uniCloud Web SDK

## 下载地址
从以下地址下载 uniCloud Web SDK：
https://doc.dcloud.net.cn/uniCloud/unicloud-web.html

## 使用步骤

### 1. 创建 uniCloud 服务空间
1. 登录 [DCloud 开发者中心](https://dev.dcloud.net.cn/)
2. 创建一个 uniCloud 服务空间（阿里云或腾讯云）
3. 获取 `spaceId` 和 `clientSecret`

### 2. 配置 SDK
在 `html/index.html` 中配置你的服务空间信息：
```javascript
uniCloud.init({
    spaceId: '你的服务空间ID',
    clientSecret: '你的客户端密钥',
    provider: 'aliyun' // 或 'tencent'
});
```

### 3. 上传云函数
1. 使用 HBuilderX 打开项目
2. 右键点击 `unicloud-functions/onenet-verify` 文件夹
3. 选择"上传部署"

### 4. 配置云函数 URL 化（可选）
如果需要通过 HTTP 直接调用云函数：
1. 在 uniCloud 控制台配置云函数 URL 化
2. 设置访问路径和权限

## 本地开发模式
如果不配置 uniCloud，系统会自动降级到本地验证模式：
- 验证输入格式（至少3个字符）
- 保存设备信息到本地
- 实际验证将在连接 ESP32 设备时进行
