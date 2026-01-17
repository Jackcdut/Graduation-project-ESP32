@echo off
chcp 65001 >nul
echo ========================================
echo ESP32-P4 USB 显示驱动安装助手
echo ========================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 请以管理员身份运行此脚本！
    echo.
    echo 右键点击此文件 -^> 以管理员身份运行
    pause
    exit /b 1
)

echo [1/4] 检查驱动文件...
set DRIVER_DIR=%~dp0win10_idd_xfz1986_usb_graphic_driver_display-main\bin\xfz1986_usb_graphic_windos10_driver-v2.0
set INF_FILE=%DRIVER_DIR%\xfz1986_usb_graphic_800x480.inf

if not exist "%INF_FILE%" (
    echo [错误] 找不到驱动文件：%INF_FILE%
    echo.
    echo 请确保以下文件存在：
    echo - xfz1986_usb_graphic_800x480.inf
    echo - xfz1986_usb_graphic.dll
    echo - xfz1986_usb_graphic.cat
    pause
    exit /b 1
)

echo [✓] 驱动文件检查完成
echo.

echo [2/4] 检查测试模式状态...
bcdedit /enum {current} | findstr /C:"testsigning" | findstr /C:"Yes" >nul
if %errorLevel% equ 0 (
    echo [✓] 测试模式已开启
) else (
    echo [!] 测试模式未开启
    echo.
    echo 选项 1: 使用签名版驱动（推荐）
    echo   - 运行: win10_idd_xfz1986_usb_graphic_driver_display-main\windows_driver_sign\xfz1986_usb_graphic_250224_rc_sign.exe
    echo.
    echo 选项 2: 开启测试模式
    echo   - 命令: bcdedit /set testsigning on
    echo   - 需要重启电脑
    echo.
    set /p choice="是否现在开启测试模式？(Y/N): "
    if /i "%choice%"=="Y" (
        bcdedit /set testsigning on
        if %errorLevel% equ 0 (
            echo [✓] 测试模式已开启，请重启电脑后再次运行此脚本
            pause
            exit /b 0
        ) else (
            echo [错误] 无法开启测试模式
            pause
            exit /b 1
        )
    ) else (
        echo [!] 跳过测试模式设置
    )
)
echo.

echo [3/4] 准备安装驱动...
echo.
echo 驱动信息：
echo - 设备名称: ESP32-P4 USB Display (800x480)
echo - VID: 0x303A (Espressif)
echo - PID: 0x2987
echo - 分辨率: 800 x 480
echo.

echo 请按照以下步骤操作：
echo.
echo 1. 连接 ESP32-P4 USB 线到电脑
echo 2. 打开设备管理器 (Win + X -^> 设备管理器)
echo 3. 找到未识别的 USB 设备（可能在"其他设备"下）
echo 4. 右键 -^> 更新驱动程序
echo 5. 浏览我的电脑以查找驱动程序
echo 6. 选择文件夹: %DRIVER_DIR%
echo 7. 选择 INF 文件: xfz1986_usb_graphic_800x480.inf
echo 8. 点击"下一步"完成安装
echo.

set /p continue="按任意键打开设备管理器..."
devmgmt.msc

echo.
echo [4/4] 安装完成检查
echo.
echo 请在设备管理器中确认：
echo - "显示适配器" 下应该有 "ESP32-P4 USB Display (800x480)"
echo.
echo 然后在显示设置中：
echo - 右键桌面 -^> 显示设置
echo - 应该看到第二个显示器 (800x480)
echo.

pause

