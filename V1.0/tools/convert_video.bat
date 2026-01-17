@echo off
REM ============================================================
REM AVI Video Converter for ESP32-P4 Media Player
REM Converts video to MJPEG + PCM format
REM 
REM Screen: 800x480 landscape (LVGL handles rotation)
REM ============================================================

if "%~1"=="" (
    echo Usage: convert_video.bat input_file [output_file] [quality]
    echo.
    echo Converts video to MJPEG + PCM AVI format for ESP32-P4
    echo.
    echo Options:
    echo   input_file   - Source video file (mp4, avi, mkv, etc.)
    echo   output_file  - Output AVI file (optional, default: input_mjpeg.avi)
    echo   quality      - Video quality: low, medium, high (default: medium)
    echo.
    echo Quality presets:
    echo   low    - 854x480,  q=6  (480p, small file)
    echo   medium - 1280x720, q=4  (720p HD, balanced)
    echo   high   - 1920x1080,q=3  (1080p Full HD)
    echo.
    echo Example:
    echo   convert_video.bat video.mp4
    echo   convert_video.bat video.mp4 output.avi medium
    exit /b 1
)

set INPUT=%~1
set OUTPUT=%~2
set QUALITY=%~3

if "%OUTPUT%"=="" (
    set OUTPUT=%~n1_mjpeg.avi
)

if "%QUALITY%"=="" (
    set QUALITY=medium
)

REM Set resolution and quality based on preset
if /i "%QUALITY%"=="low" (
    set RES=854x480
    set QVAL=6
    set FPS=24
) else if /i "%QUALITY%"=="high" (
    set RES=1920x1080
    set QVAL=3
    set FPS=24
    REM Note: 1080p at 24fps is recommended for smooth playback on ESP32-P4
) else (
    set RES=1280x720
    set QVAL=4
    set FPS=25
)

echo.
echo Converting: %INPUT%
echo Output:     %OUTPUT%
echo Quality:    %QUALITY% (%RES%, q=%QVAL%, %FPS%fps)
echo.

REM Check if ffmpeg is available
where ffmpeg >nul 2>&1
if errorlevel 1 (
    echo ERROR: ffmpeg not found!
    echo Please install ffmpeg and add it to PATH
    echo Download: https://ffmpeg.org/download.html
    exit /b 1
)

REM Convert to MJPEG + PCM
REM -vcodec mjpeg     : Use MJPEG video codec (hardware decoded on ESP32-P4)
REM -q:v              : Video quality (1-31, lower = better quality, larger file)
REM -pix_fmt yuvj420p : JPEG pixel format for better compatibility
REM -r                : Frame rate
REM -acodec pcm_s16le : 16-bit PCM audio (little endian)
REM -ar 16000         : Audio sample rate 16kHz (optimized for ESP32-P4)
REM -ac 1             : Mono audio

echo Running ffmpeg...
echo.

ffmpeg -i "%INPUT%" ^
    -vcodec mjpeg -q:v %QVAL% -pix_fmt yuvj420p ^
    -s %RES% -r %FPS% ^
    -acodec pcm_s16le -ar 16000 -ac 1 ^
    -y "%OUTPUT%"

if errorlevel 1 (
    echo.
    echo ERROR: Conversion failed!
    exit /b 1
)

echo.
echo ============================================================
echo Conversion complete!
echo Output file: %OUTPUT%
echo Resolution:  %RES% @ %FPS%fps
echo Audio:       16kHz Mono PCM (optimized for ESP32-P4)
echo.
echo Copy this file to your SD card's Media folder:
echo   /sdcard/Media/
echo ============================================================
