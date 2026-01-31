@echo off
chcp 65001 >nul
echo ==============================================
echo 数据分析平台 - 依赖库本地化设置
echo ==============================================
echo.

:: 检查Python是否安装
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 未检测到Python，请先安装Python 3.6+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

:: 检查网络连接
echo 📡 检查网络连接...
ping -n 1 www.baidu.com >nul 2>&1
if %errorlevel% neq 0 (
    echo ⚠️  网络连接异常，将跳过资源下载
    goto :skip_download
)

:: 安装requests库（如果未安装）
echo 📦 检查依赖库...
python -c "import requests" >nul 2>&1
if %errorlevel% neq 0 (
    echo 正在安装requests库...
    pip install requests
)

:: 运行下载脚本
echo 🚀 开始下载依赖库...
python download-libs.py

:skip_download
:: 创建基本目录结构
echo 📁 创建目录结构...
if not exist "libs" mkdir libs
if not exist "libs\css" mkdir libs\css
if not exist "libs\css\fontawesome" mkdir libs\css\fontawesome
if not exist "libs\css\animate" mkdir libs\css\animate
if not exist "libs\js" mkdir libs\js
if not exist "libs\js\chart" mkdir libs\js\chart
if not exist "libs\js\xlsx" mkdir libs\js\xlsx
if not exist "libs\js\particles" mkdir libs\js\particles
if not exist "libs\fonts" mkdir libs\fonts

:: 创建备用CSS文件（简化版）
echo 🎨 创建备用样式文件...
echo /* Font Awesome 图标备用样式 */ > libs\css\fontawesome\fallback.css
echo .fas, .far, .fab, .fal { font-family: "Font Awesome 6 Free", "Font Awesome 6 Pro", FontAwesome, sans-serif; } >> libs\css\fontawesome\fallback.css

echo /* Animate.css 基础动画 */ > libs\css\animate\fallback.css
echo @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } } >> libs\css\animate\fallback.css
echo .animate__fadeIn { animation: fadeIn 1s; } >> libs\css\animate\fallback.css

:: 创建离线检测脚本
echo 🔧 创建离线检测脚本...
(
echo // 检测资源是否可用
echo function checkResourceAvailability^(^) {
echo     const testUrls = [
echo         'https://cdn.bootcdn.net/ajax/libs/font-awesome/6.0.0/css/all.min.css',
echo         'https://cdn.jsdelivr.net/npm/chart.js'
echo     ];
echo     
echo     return Promise.all^(testUrls.map^(url =^> 
echo         fetch^(url, { method: 'HEAD', mode: 'no-cors' }^)
echo             .then^(^(^) =^> true^)
echo             .catch^(^(^) =^> false^)
echo     ^^)^);
echo }
echo.
echo // 设置离线模式
echo if ^(navigator.onLine === false^) {
echo     document.documentElement.classList.add^('offline-mode'^);
echo     console.log^('离线模式已激活'^);
echo }
) > libs\js\offline-detector.js

echo.
echo ✅ 设置完成！
echo.
echo 📋 后续步骤:
echo 1. 检查 libs 目录是否包含所需文件
echo 2. 如果网络下载失败，请手动下载缺失的库文件
echo 3. 运行项目并检查功能是否正常
echo.
echo 🌐 手动下载地址:
echo - Font Awesome: https://fontawesome.com/download
echo - Chart.js: https://www.chartjs.org/
echo - SheetJS: https://sheetjs.com/
echo.
pause 