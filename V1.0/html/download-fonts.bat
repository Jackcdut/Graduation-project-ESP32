@echo off
chcp 65001 >nul
echo ==============================================
echo Font Awesome 字体文件下载工具
echo ==============================================
echo.

:: 创建webfonts目录
echo 📁 创建字体目录...
if not exist "libs\css\fontawesome\webfonts" mkdir "libs\css\fontawesome\webfonts"

:: 检查网络连接
echo 📡 检查网络连接...
ping -n 1 www.baidu.com >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 网络连接异常，无法下载字体文件
    echo 请检查网络连接后重试
    pause
    exit /b 1
)

echo ✅ 网络连接正常，开始下载字体文件...
echo.

:: 下载必需的字体文件
echo 📥 下载 fa-solid-900.woff2...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.woff2' -OutFile 'libs\css\fontawesome\webfonts\fa-solid-900.woff2' -TimeoutSec 30; Write-Host '✅ fa-solid-900.woff2 下载成功' } catch { Write-Host '❌ fa-solid-900.woff2 下载失败' }"

echo 📥 下载 fa-solid-900.woff...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.woff' -OutFile 'libs\css\fontawesome\webfonts\fa-solid-900.woff' -TimeoutSec 30; Write-Host '✅ fa-solid-900.woff 下载成功' } catch { Write-Host '❌ fa-solid-900.woff 下载失败' }"

echo 📥 下载 fa-solid-900.ttf...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-solid-900.ttf' -OutFile 'libs\css\fontawesome\webfonts\fa-solid-900.ttf' -TimeoutSec 30; Write-Host '✅ fa-solid-900.ttf 下载成功' } catch { Write-Host '❌ fa-solid-900.ttf 下载失败' }"

echo 📥 下载 fa-regular-400.woff2...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-regular-400.woff2' -OutFile 'libs\css\fontawesome\webfonts\fa-regular-400.woff2' -TimeoutSec 30; Write-Host '✅ fa-regular-400.woff2 下载成功' } catch { Write-Host '❌ fa-regular-400.woff2 下载失败' }"

echo 📥 下载 fa-regular-400.woff...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-regular-400.woff' -OutFile 'libs\css\fontawesome\webfonts\fa-regular-400.woff' -TimeoutSec 30; Write-Host '✅ fa-regular-400.woff 下载成功' } catch { Write-Host '❌ fa-regular-400.woff 下载失败' }"

echo 📥 下载 fa-regular-400.ttf...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-regular-400.ttf' -OutFile 'libs\css\fontawesome\webfonts\fa-regular-400.ttf' -TimeoutSec 30; Write-Host '✅ fa-regular-400.ttf 下载成功' } catch { Write-Host '❌ fa-regular-400.ttf 下载失败' }"

echo 📥 下载 fa-brands-400.woff2...
powershell -Command "try { Invoke-WebRequest -Uri 'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/webfonts/fa-brands-400.woff2' -OutFile 'libs\css\fontawesome\webfonts\fa-brands-400.woff2' -TimeoutSec 30; Write-Host '✅ fa-brands-400.woff2 下载成功' } catch { Write-Host '❌ fa-brands-400.woff2 下载失败' }"

echo.
echo ==============================================
echo ✅ 字体文件下载完成！
echo ==============================================
echo.
echo 现在刷新页面，所有图标应该正常显示
echo 如果仍有问题，页面会自动使用Unicode备用字符
echo.
pause 