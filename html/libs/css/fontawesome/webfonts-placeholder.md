# Font Awesome 字体文件

**⚠️ 重要提示：需要下载字体文件到此目录**

## 字体文件列表

请下载以下字体文件到 `libs/css/fontawesome/webfonts/` 目录：

### 必需的字体文件
1. **fa-solid-900.woff2** - 实心图标（推荐，最小文件）
2. **fa-solid-900.woff** - 实心图标（备用）
3. **fa-solid-900.ttf** - 实心图标（最终备用）
4. **fa-regular-400.woff2** - 常规图标
5. **fa-regular-400.woff** - 常规图标（备用）
6. **fa-regular-400.ttf** - 常规图标（最终备用）
7. **fa-brands-400.woff2** - 品牌图标
8. **fa-brands-400.woff** - 品牌图标（备用）
9. **fa-brands-400.ttf** - 品牌图标（最终备用）

## 下载链接

### 官方CDN下载
```bash
# 创建webfonts目录
mkdir -p libs/css/fontawesome/webfonts

# 下载字体文件
curl -o libs/css/fontawesome/webfonts/fa-solid-900.woff2 https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.woff2
curl -o libs/css/fontawesome/webfonts/fa-solid-900.woff https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.woff
curl -o libs/css/fontawesome/webfonts/fa-solid-900.ttf https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.ttf

curl -o libs/css/fontawesome/webfonts/fa-regular-400.woff2 https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-regular-400.woff2
curl -o libs/css/fontawesome/webfonts/fa-regular-400.woff https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-regular-400.woff
curl -o libs/css/fontawesome/webfonts/fa-regular-400.ttf https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-regular-400.ttf

curl -o libs/css/fontawesome/webfonts/fa-brands-400.woff2 https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-brands-400.woff2
curl -o libs/css/fontawesome/webfonts/fa-brands-400.woff https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-brands-400.woff
curl -o libs/css/fontawesome/webfonts/fa-brands-400.ttf https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-brands-400.ttf
```

### Windows PowerShell 下载
```powershell
# 创建目录
New-Item -ItemType Directory -Force -Path "libs\css\fontawesome\webfonts"

# 下载字体文件
Invoke-WebRequest -Uri "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.woff2" -OutFile "libs\css\fontawesome\webfonts\fa-solid-900.woff2"
Invoke-WebRequest -Uri "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.woff" -OutFile "libs\css\fontawesome\webfonts\fa-solid-900.woff"
Invoke-WebRequest -Uri "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.ttf" -OutFile "libs\css\fontawesome\webfonts\fa-solid-900.ttf"
```

## 文件大小参考
- fa-solid-900.woff2: ~79KB
- fa-solid-900.woff: ~205KB
- fa-solid-900.ttf: ~405KB
- fa-regular-400.woff2: ~13KB
- fa-brands-400.woff2: ~130KB

## 验证安装
字体文件下载完成后，重新访问页面，图标应该正常显示。

如果字体文件仍然加载失败，页面会自动使用Unicode备用字符。 