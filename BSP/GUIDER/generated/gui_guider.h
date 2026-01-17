/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
  
	lv_obj_t *scrHome;
	bool scrHome_del;
	lv_obj_t *scrHome_contBG;
	lv_obj_t *scrHome_imgWifiStatus;
	lv_obj_t *scrHome_labelWifiStatus;
	lv_obj_t *scrHome_contMain;
	lv_obj_t *scrHome_contPrint;
	lv_obj_t *scrHome_imgIconPrint;
	lv_obj_t *scrHome_labelPrint;
	lv_obj_t *scrHome_contCopy;
	lv_obj_t *scrHome_imgIconCopy;
	lv_obj_t *scrHome_labelCopy;
	lv_obj_t *scrHome_contScan;
	lv_obj_t *scrHome_imgIconScan;
	lv_obj_t *scrHome_labelScan;
	lv_obj_t *scrHome_cont_1;
	lv_obj_t *scrHome_img_1;
	lv_obj_t *scrHome_label_1;
	lv_obj_t *scrHome_cont_2;
	lv_obj_t *scrHome_img_2;
	lv_obj_t *scrHome_label_2;
	lv_obj_t *scrHome_cont_5;
	lv_obj_t *scrHome_img_4;
	lv_obj_t *scrHome_label_5;
	lv_obj_t *scrHome_contTop;
	lv_obj_t *scrHome_imgIconCall;
	lv_obj_t *scrHome_spangroup_1;
	lv_obj_t *scrHome_contColorInk;
	lv_obj_t *scrHome_barBlueInk;
	lv_obj_t *scrHome_barRedInk;
	lv_obj_t *scrHome_barYellowInk;
	lv_obj_t *scrHome_barBlackInk;
	lv_obj_t *scrHome_btn_1;
	lv_obj_t *scrHome_btn_1_label;
	lv_obj_t *scrCopy;
	bool scrCopy_del;
	lv_obj_t *scrCopy_contBG;
	lv_obj_t *scrCopy_labelTitle;
	lv_obj_t *scrCopy_imgScanned;
	lv_obj_t *scrCopy_contPanel;
	lv_obj_t *scrCopy_imgBright;
	lv_obj_t *scrCopy_imgColor;
	lv_obj_t *scrCopy_sliderBright;
	lv_obj_t *scrCopy_sliderHue;
	lv_obj_t *scrCopy_btnNext;
	lv_obj_t *scrCopy_btnNext_label;
	lv_obj_t *scrCopy_btnBack;
	lv_obj_t *scrCopy_btnBack_label;
	lv_obj_t *scrScan;
	bool scrScan_del;
	lv_obj_t *scrScan_contBG;
	lv_obj_t *scrScan_labelTitle;
	lv_obj_t *scrScan_imgScanned;
	lv_obj_t *scrScan_contPanel;
	lv_obj_t *scrScan_imgIconBright;
	lv_obj_t *scrScan_imgIconHue;
	lv_obj_t *scrScan_sliderBright;
	lv_obj_t *scrScan_sliderHue;
	lv_obj_t *scrScan_btnNext;
	lv_obj_t *scrScan_btnNext_label;
	lv_obj_t *scrScan_btnBack;
	lv_obj_t *scrScan_btnBack_label;
	lv_obj_t *scrPrintMenu;
	bool scrPrintMenu_del;
	lv_obj_t *scrPrintMenu_contBG;
	lv_obj_t *scrPrintMenu_labelTitle;
	lv_obj_t *scrPrintMenu_btnBack;
	lv_obj_t *scrPrintMenu_btnBack_label;
	lv_obj_t *scrPrintMenu_labelPrompt;
	lv_obj_t *scrPrintMenu_contMain;
	lv_obj_t *scrPrintMenu_contInternet;
	lv_obj_t *scrPrintMenu_imgInternet;
	lv_obj_t *scrPrintMenu_labelnternet;
	lv_obj_t *scrPrintMenu_contMobile;
	lv_obj_t *scrPrintMenu_imgMobile;
	lv_obj_t *scrPrintMenu_labelMobile;
	lv_obj_t *scrPrintMenu_contUSB;
	lv_obj_t *scrPrintMenu_imgUSB;
	lv_obj_t *scrPrintMenu_labelUSB;
	lv_obj_t *scrPowerSupply;
	bool scrPowerSupply_del;
	lv_obj_t *scrPowerSupply_contBG;
	lv_obj_t *scrPowerSupply_labelTitle;
	lv_obj_t *scrPowerSupply_btnBack;
	lv_obj_t *scrPowerSupply_btnBack_label;
	lv_obj_t *scrPowerSupply_contLeft;
	lv_obj_t *scrPowerSupply_contVoltage;
	lv_obj_t *scrPowerSupply_labelVoltageTitle;
	lv_obj_t *scrPowerSupply_labelVoltageValue;
	lv_obj_t *scrPowerSupply_labelVoltageUnit;
	lv_obj_t *scrPowerSupply_contCurrent;
	lv_obj_t *scrPowerSupply_labelCurrentTitle;
	lv_obj_t *scrPowerSupply_labelCurrentValue;
	lv_obj_t *scrPowerSupply_labelCurrentUnit;
	lv_obj_t *scrPowerSupply_contPower;
	lv_obj_t *scrPowerSupply_labelPowerTitle;
	lv_obj_t *scrPowerSupply_labelPowerValue;
	lv_obj_t *scrPowerSupply_labelPowerUnit;
	lv_obj_t *scrPowerSupply_contRight;
	lv_obj_t *scrPowerSupply_switchPower;
	lv_obj_t *scrPowerSupply_labelModeStatus;
	lv_obj_t *scrPowerSupply_labelVoltageSet;
	lv_obj_t *scrPowerSupply_sliderVoltage;
	lv_obj_t *scrPowerSupply_labelCurrentSet;
	lv_obj_t *scrPowerSupply_sliderCurrent;
	lv_obj_t *scrPowerSupply_chartPower;
	lv_chart_series_t *scrPowerSupply_chartPowerSeries;
	lv_obj_t *scrAIChat;
	bool scrAIChat_del;
	lv_obj_t *scrAIChat_contBG;
	lv_obj_t *scrAIChat_labelTitle;
	lv_obj_t *scrAIChat_btnBack;
	lv_obj_t *scrAIChat_btnBack_label;
	lv_obj_t *scrAIChat_contChatArea;  // 左侧聊天对话卡片
	lv_obj_t *scrAIChat_contInputCard;  // 右侧输入控制卡片
	lv_obj_t *scrAIChat_textAreaInput;  // 文本输入框（在右侧卡片内）
	lv_obj_t *scrAIChat_btnVoiceInput;  // 语音输入按钮（在右侧卡片内）
	lv_obj_t *scrAIChat_btnVoiceInput_label;
	lv_obj_t *scrAIChat_btnDelete;  // 删除按钮（在右侧卡片内）
	lv_obj_t *scrAIChat_btnDelete_label;
	lv_obj_t *scrAIChat_btnSend;  // 发送按钮（在右侧卡片内）
	lv_obj_t *scrAIChat_btnSend_label;
	lv_obj_t *scrAIChat_labelStatus;
	lv_obj_t *scrSettings;
	bool scrSettings_del;
	lv_obj_t *scrSettings_contBG;
	lv_obj_t *scrSettings_labelTitle;
	lv_obj_t *scrSettings_btnBack;
	lv_obj_t *scrSettings_btnBack_label;
	lv_obj_t *scrSettings_contLeft;  // 左侧菜单栏
	lv_obj_t *scrSettings_contRight; // 右侧内容区域
	// 左侧菜单项
	lv_obj_t *scrSettings_btnMenuBrightness;
	lv_obj_t *scrSettings_btnMenuBrightness_label;
	lv_obj_t *scrSettings_imgMenuBrightness;
	lv_obj_t *scrSettings_btnMenuWifi;
	lv_obj_t *scrSettings_btnMenuWifi_label;
	lv_obj_t *scrSettings_imgMenuWifi;
	lv_obj_t *scrSettings_btnMenuAbout;
	lv_obj_t *scrSettings_btnMenuAbout_label;
	lv_obj_t *scrSettings_imgMenuAbout;
	lv_obj_t *scrSettings_btnMenuGallery;
	lv_obj_t *scrSettings_btnMenuGallery_label;
	lv_obj_t *scrSettings_imgMenuGallery;
	// 右侧亮度调节内容
	lv_obj_t *scrSettings_contBrightnessPanel;
	lv_obj_t *scrSettings_sliderBrightness;
	lv_obj_t *scrSettings_labelBrightnessValue;
	// 右侧WiFi内容
	lv_obj_t *scrSettings_contWifiPanel;
	lv_obj_t *scrSettings_labelWifiTitle;
	lv_obj_t *scrSettings_listWifi;  // WiFi列表
	lv_obj_t *scrSettings_btnWifiScan;
	lv_obj_t *scrSettings_btnWifiScan_label;
	lv_obj_t *scrSettings_btnWifiCustom;
	lv_obj_t *scrSettings_btnWifiCustom_label;
	lv_obj_t *scrSettings_btnWifiDisconnect;
	lv_obj_t *scrSettings_btnWifiDisconnect_label;
	lv_obj_t *scrSettings_labelWifiStatus;
	lv_obj_t *scrSettings_spinnerWifiScan; // 扫描动画
	// WiFi自定义连接对话框
	lv_obj_t *scrSettings_contWifiDialog;
	lv_obj_t *scrSettings_textareaSSID;
	lv_obj_t *scrSettings_textareaPassword;
	lv_obj_t *scrSettings_btnWifiConnect;
	lv_obj_t *scrSettings_btnWifiConnect_label;
	lv_obj_t *scrSettings_btnWifiCancel;
	lv_obj_t *scrSettings_btnWifiCancel_label;
	// 右侧关于内容
	lv_obj_t *scrSettings_contAboutPanel;
	lv_obj_t *scrSettings_labelAboutTitle;
	lv_obj_t *scrSettings_labelAboutContent;
	// 右侧SD卡管理内容 (SD Card Manager Panel)
	lv_obj_t *scrSettings_contGalleryPanel;    // SD Card管理面板容器
	lv_obj_t *scrSettings_labelGalleryTitle;   // SD Card标题
	lv_obj_t *scrSettings_switchUSB;           // Legacy - 已弃用
	lv_obj_t *scrSettings_labelUSB;            // Legacy - 已弃用
	lv_obj_t *scrSettings_listGallery;         // Legacy - 已弃用
	lv_obj_t *scrSettings_labelGalleryEmpty;   // Legacy - 已弃用
	// Legacy分类筛选按钮 - 已弃用，保留用于兼容
	lv_obj_t *scrSettings_btnGalleryAll;       // Legacy - 已弃用
	lv_obj_t *scrSettings_btnGalleryFlash;     // Legacy - 已弃用
	lv_obj_t *scrSettings_btnGalleryPsram;     // Legacy - 已弃用
	lv_obj_t *scrSettings_labelGalleryCount;   // SD卡容量/文件统计
	// 存储选择弹窗
	lv_obj_t *scrSettings_contStorageDialog;   // 存储选择弹窗容器
	lv_obj_t *scrSettings_labelStorageTitle;   // 弹窗标题
	lv_obj_t *scrSettings_btnStorageFlash;     // FLASH选项按钮
	lv_obj_t *scrSettings_btnStoragePsram;     // PSRAM选项按钮
	lv_obj_t *scrSettings_btnStorageClose;     // 关闭按钮
	lv_obj_t *scrSettings_labelStorageInfo;    // 存储信息标签
	// 图片全屏查看对话框
	lv_obj_t *scrSettings_contImageViewer;
	lv_obj_t *scrSettings_imgViewer;
	lv_obj_t *scrSettings_btnImageDelete;
	lv_obj_t *scrSettings_btnImageDelete_label;
	lv_obj_t *scrSettings_btnImageClose;
	lv_obj_t *scrSettings_btnImageClose_label;
	lv_obj_t *scrSettings_labelImageInfo;      // 图片信息标签（存储位置等）
	lv_obj_t *scrSettings_btnImagePrev;        // 上一张按钮
	lv_obj_t *scrSettings_btnImageNext;        // 下一张按钮
	lv_obj_t *scrSettings_btnImageMove;        // 迁移存储位置按钮
	lv_obj_t *scrSettings_btnImageMove_label;  // 迁移按钮标签
	lv_obj_t *scrSettings_contLoadingOverlay;  // 加载动画遮罩层
	lv_obj_t *scrSettings_spinnerLoading;      // 加载动画旋转器
	lv_obj_t *scrSettings_labelLoadingText;    // 加载提示文字
	// USB模式切换弹窗
	lv_obj_t *scrSettings_contUSBModeDialog;   // USB模式切换弹窗
	lv_obj_t *scrSettings_labelUSBModeTitle;   // 弹窗标题
	lv_obj_t *scrSettings_labelUSBModeDesc;    // 描述文字
	lv_obj_t *scrSettings_btnUSBModeExtend;    // 拓展屏模式按钮
	lv_obj_t *scrSettings_btnUSBModeMSC;       // U盘模式按钮
	lv_obj_t *scrSettings_labelUSBModeStatus;  // 当前模式状态
	lv_obj_t *scrSettings_btnUSBModeCancel;    // 取消按钮
	// 云平台管理菜单按钮
	lv_obj_t *scrSettings_btnMenuCloud;        // 云平台管理按钮
	lv_obj_t *scrSettings_btnMenuCloud_label;  // 按钮标签
	lv_obj_t *scrSettings_imgMenuCloud;        // 按钮图标
	// 云平台管理面板
	lv_obj_t *scrSettings_contCloudPanel;      // 云平台管理面板容器
	// Success/Finish screen (reused for WiFi connection success)
	lv_obj_t *scrScanFini;
	bool scrScanFini_del;
	lv_obj_t *scrScanFini_contBG;
	lv_obj_t *scrScanFini_imgIconOk;
	lv_obj_t *scrScanFini_labelPrompt;
	lv_obj_t *scrScanFini_btnNxet;
	lv_obj_t *scrScanFini_btnNxet_label;
	// Print Internet error screen
	lv_obj_t *scrPrintInternet;
	bool scrPrintInternet_del;
	lv_obj_t *scrPrintInternet_contBG;
	lv_obj_t *scrPrintInternet_imgWave;
	lv_obj_t *scrPrintInternet_imgCloud;
	lv_obj_t *scrPrintInternet_labelPrompt;
	lv_obj_t *scrPrintInternet_btnBack;
	lv_obj_t *scrPrintInternet_btnBack_label;
	// Wireless Serial screen
	lv_obj_t *scrWirelessSerial;
	bool scrWirelessSerial_del;
	lv_obj_t *scrWirelessSerial_labelTitle;
	lv_obj_t *scrWirelessSerial_btnBack;
	lv_obj_t *scrWirelessSerial_btnBack_label;
	lv_obj_t *scrWirelessSerial_textareaReceive;  // WiFi list display
	lv_obj_t *scrWirelessSerial_textareaSend;  // Command input
	lv_obj_t *scrWirelessSerial_btnSend;  // Right panel (Configuration)
	lv_obj_t *scrWirelessSerial_btnClear;  // Send button
	lv_obj_t *scrWirelessSerial_btnClear_label;
	// Configuration dropdowns
	lv_obj_t *scrWirelessSerial_labelBaudRate;
	lv_obj_t *scrWirelessSerial_dropdownBaudRate;
	lv_obj_t *scrWirelessSerial_labelStopBits;
	lv_obj_t *scrWirelessSerial_dropdownStopBits;
	lv_obj_t *scrWirelessSerial_labelLength;
	lv_obj_t *scrWirelessSerial_dropdownLength;
	lv_obj_t *scrWirelessSerial_labelParity;
	lv_obj_t *scrWirelessSerial_dropdownParity;
	// Checkboxes
	lv_obj_t *scrWirelessSerial_checkboxHexSend;
	lv_obj_t *scrWirelessSerial_checkboxHexReceive;
	lv_obj_t *scrWirelessSerial_checkboxSendNewLine;
	// Clear button container and button
	lv_obj_t *scrWirelessSerial_contClearCard;  // White card container for Send-NewLine and Clear
	lv_obj_t *scrWirelessSerial_btnClearReceive;
	lv_obj_t *scrWirelessSerial_btnClearReceive_label;
	// AT command type selector container
	lv_obj_t *scrWirelessSerial_contATSelector;  // White card container for AT type selector
	lv_obj_t *scrWirelessSerial_labelATType;
	lv_obj_t *scrWirelessSerial_dropdownATType;
	// Send area container
	lv_obj_t *scrWirelessSerial_contSendCard;  // White card container for Send textarea and Send button

	// Oscilloscope screen
	lv_obj_t *scrOscilloscope;
	bool scrOscilloscope_del;
	lv_obj_t *scrOscilloscope_btnBack;
	lv_obj_t *scrOscilloscope_btnBack_label;
	lv_obj_t *scrOscilloscope_btnAuto;
	lv_obj_t *scrOscilloscope_btnAuto_label;

	// Waveform display area
	lv_obj_t *scrOscilloscope_contWaveform;
	lv_obj_t *scrOscilloscope_chartWaveform;

	// Top control buttons
	lv_obj_t *scrOscilloscope_btnStartStop;
	lv_obj_t *scrOscilloscope_btnStartStop_label;
	lv_obj_t *scrOscilloscope_btnPanZoom;
	lv_obj_t *scrOscilloscope_btnPanZoom_label;
	lv_obj_t *scrOscilloscope_sliderWavePos;  // Waveform position indicator
	lv_obj_t *scrOscilloscope_sliderWaveMask;  // Waveform position mask (blue overlay)
	lv_obj_t *scrOscilloscope_btnFFT;
	lv_obj_t *scrOscilloscope_btnFFT_label;
	lv_obj_t *scrOscilloscope_btnExport;
	lv_obj_t *scrOscilloscope_btnExport_label;

	// Right side control panel - Channel
	lv_obj_t *scrOscilloscope_contChannel;
	lv_obj_t *scrOscilloscope_labelChannelTitle;
	lv_obj_t *scrOscilloscope_labelChannelValue;

	// Right side control panel - Time scale
	lv_obj_t *scrOscilloscope_contTimeScale;
	lv_obj_t *scrOscilloscope_labelTimeScaleTitle;
	lv_obj_t *scrOscilloscope_labelTimeScaleValue;

	// Right side control panel - Voltage scale
	lv_obj_t *scrOscilloscope_contVoltScale;
	lv_obj_t *scrOscilloscope_labelVoltScaleTitle;
	lv_obj_t *scrOscilloscope_labelVoltScaleValue;

	// Right side control panel - X offset
	lv_obj_t *scrOscilloscope_contXOffset;
	lv_obj_t *scrOscilloscope_labelXOffsetTitle;
	lv_obj_t *scrOscilloscope_labelXOffsetValue;

	// Right side control panel - Y offset
	lv_obj_t *scrOscilloscope_contYOffset;
	lv_obj_t *scrOscilloscope_labelYOffsetTitle;
	lv_obj_t *scrOscilloscope_labelYOffsetValue;

	// Right side control panel - Trigger voltage
	lv_obj_t *scrOscilloscope_contTrigger;
	lv_obj_t *scrOscilloscope_labelTriggerTitle;
	lv_obj_t *scrOscilloscope_labelTriggerValue;

	// Right side control panel - Coupling
	lv_obj_t *scrOscilloscope_contCoupling;
	lv_obj_t *scrOscilloscope_labelCouplingTitle;
	lv_obj_t *scrOscilloscope_labelCouplingValue;

	// Right side control panel - Trigger mode
	lv_obj_t *scrOscilloscope_contTriggerMode;
	lv_obj_t *scrOscilloscope_labelTriggerModeTitle;
	lv_obj_t *scrOscilloscope_labelTriggerModeValue;

	// Bottom measurement displays
	lv_obj_t *scrOscilloscope_contFreq;
	lv_obj_t *scrOscilloscope_labelFreqTitle;
	lv_obj_t *scrOscilloscope_labelFreqValue;

	lv_obj_t *scrOscilloscope_contVmax;
	lv_obj_t *scrOscilloscope_labelVmaxTitle;
	lv_obj_t *scrOscilloscope_labelVmaxValue;

	lv_obj_t *scrOscilloscope_contVmin;
	lv_obj_t *scrOscilloscope_labelVminTitle;
	lv_obj_t *scrOscilloscope_labelVminValue;

	lv_obj_t *scrOscilloscope_contVpp;
	lv_obj_t *scrOscilloscope_labelVppTitle;
	lv_obj_t *scrOscilloscope_labelVppValue;

	lv_obj_t *scrOscilloscope_contVrms;
	lv_obj_t *scrOscilloscope_labelVrmsTitle;
	lv_obj_t *scrOscilloscope_labelVrmsValue;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_move_animation(void * var, int32_t duration, int32_t delay, int32_t x_end, int32_t y_end, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void ui_scale_animation(void * var, int32_t duration, int32_t delay, int32_t width, int32_t height, lv_anim_path_cb_t path_cb,
                        uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                        lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void ui_img_zoom_animation(void * var, int32_t duration, int32_t delay, int32_t zoom, lv_anim_path_cb_t path_cb,
                           uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                           lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void ui_img_rotate_animation(void * var, int32_t duration, int32_t delay, lv_coord_t x, lv_coord_t y, int32_t rotate,
                   lv_anim_path_cb_t path_cb, uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time,
                   uint32_t playback_delay, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);


extern lv_ui guider_ui;


void setup_scr_scrHome(lv_ui *ui);
void setup_scr_scrCopy(lv_ui *ui);
void setup_scr_scrScan(lv_ui *ui);
void setup_scr_scrPrintMenu(lv_ui *ui);
void setup_scr_scrPowerSupply(lv_ui *ui);
void setup_scr_scrAIChat(lv_ui *ui);
void setup_scr_scrSettings(lv_ui *ui);
void setup_scr_scrScanFini(lv_ui *ui);
void setup_scr_scrPrintInternet(lv_ui *ui);
void setup_scr_scrWirelessSerial(lv_ui *ui);
void setup_scr_scrOscilloscope(lv_ui *ui);
LV_IMG_DECLARE(_22_alpha_76x75);
LV_IMG_DECLARE(_1_alpha_74x79);
LV_IMG_DECLARE(_5_alpha_99x109);
LV_IMG_DECLARE(_COM001_alpha_89x70);
LV_IMG_DECLARE(_set_alpha_68x70);
LV_IMG_DECLARE(_33_alpha_76x75);
LV_IMG_DECLARE(_2_alpha_258x171);
LV_IMG_DECLARE(_example_alpha_516x342);
LV_IMG_DECLARE(_bright_alpha_33x33);
LV_IMG_DECLARE(_hue_alpha_30x30);
LV_IMG_DECLARE(_example_alpha_401x266);
LV_IMG_DECLARE(_example_alpha_516x342);
LV_IMG_DECLARE(_bright_alpha_33x33);
LV_IMG_DECLARE(_hue_alpha_30x30);

LV_IMG_DECLARE(_btn_bg_4_166x211);
LV_IMG_DECLARE(_internet_alpha_66x66);

LV_IMG_DECLARE(_btn_bg_3_166x211);
LV_IMG_DECLARE(_mobile_alpha_58x70);

LV_IMG_DECLARE(_btn_bg_2_166x211);
LV_IMG_DECLARE(_usb_alpha_68x70);
LV_IMG_DECLARE(_printer2_alpha_145x150);
LV_IMG_DECLARE(_wave_alpha_35x58);
LV_IMG_DECLARE(_phone_alpha_91x123);
LV_IMG_DECLARE(_printer2_alpha_140x137);
LV_IMG_DECLARE(_no_internet_alpha_66x65);
LV_IMG_DECLARE(_cloud_alpha_123x90);
LV_IMG_DECLARE(_cloud_alpha_30x30);
LV_IMG_DECLARE(_printer2_alpha_140x137);
LV_IMG_DECLARE(_no_internet_alpha_66x65);
LV_IMG_DECLARE(_cloud_alpha_123x90);
LV_IMG_DECLARE(_ready_alpha_183x183);
LV_IMG_DECLARE(_ready_alpha_183x183);

LV_IMG_DECLARE(_btn_bg_4_166x211);
LV_IMG_DECLARE(_33_alpha_66x66);

LV_IMG_DECLARE(_btn_bg_3_166x211);
LV_IMG_DECLARE(_22_alpha_65x62);

LV_IMG_DECLARE(_btn_bg_1_166x211);
LV_IMG_DECLARE(_1_alpha_55x57);

LV_IMG_DECLARE(_btn_bg_2_166x211);
LV_IMG_DECLARE(_5_alpha_68x78);

LV_IMG_DECLARE(_btn_bg_3_166x211);
LV_IMG_DECLARE(_22_alpha_65x62);

LV_IMG_DECLARE(_btn_bg_1_166x211);
LV_IMG_DECLARE(_1_alpha_55x57);

LV_IMG_DECLARE(_btn_bg_1_166x211);
LV_IMG_DECLARE(_1_alpha_55x57);

LV_IMG_DECLARE(_btn_bg_1_166x211);
LV_IMG_DECLARE(_1_alpha_55x57);

LV_IMG_DECLARE(_btn_bg_1_166x211);
LV_IMG_DECLARE(_1_alpha_55x57);
LV_IMG_DECLARE(_wifi_alpha_55x44);
LV_IMG_DECLARE(_wifi_alpha_39x34);
LV_IMG_DECLARE(_tel_alpha_46x42);
LV_IMG_DECLARE(_eco_alpha_48x38);
LV_IMG_DECLARE(_pc_alpha_50x42);
LV_IMG_DECLARE(_1111_alpha_40x40);
LV_IMG_DECLARE(_load_alpha_40x40);
LV_IMG_DECLARE(_IF_alpha_45x45);
LV_IMG_DECLARE(_about_alpha_36x36);
LV_IMG_DECLARE(_a_alpha_24x24);
LV_IMG_DECLARE(_b_alpha_24x24);
LV_IMG_DECLARE(_c_alpha_24x24);
LV_IMG_DECLARE(_e_alpha_24x24);
LV_IMG_DECLARE(_f_alpha_24x24);
LV_IMG_DECLARE(_g_alpha_24x24);
LV_IMG_DECLARE(_CC_alpha_47x39);
LV_IMG_DECLARE(_3_alpha_533x378);  /* School logo for boot animation */
LV_IMG_DECLARE(_sina_alpha_75x75);  /* Signal-Gen icon */
LV_IMG_DECLARE(_666_alpha_70x70);   /* Oscilloscope icon */
LV_IMG_DECLARE(_vao_alpha_70x70);   /* Voltmeter icon */

LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_montserratMedium_26)
LV_FONT_DECLARE(lv_font_montserratMedium_19)
LV_FONT_DECLARE(lv_font_montserratMedium_24)
LV_FONT_DECLARE(lv_font_montserratMedium_22)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_montserratMedium_33)
LV_FONT_DECLARE(lv_font_montserratMedium_30)
LV_FONT_DECLARE(lv_font_montserratMedium_41)
LV_FONT_DECLARE(lv_font_montserratMedium_23)
LV_FONT_DECLARE(lv_font_Collins_66)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_14)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_16)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_18)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_20)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_22)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_24)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_28)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_30)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_45)


#ifdef __cplusplus
}
#endif
#endif
