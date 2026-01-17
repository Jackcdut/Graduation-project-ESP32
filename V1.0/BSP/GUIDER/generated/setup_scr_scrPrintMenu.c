/*******************************************************************************
 * 专业数字万用表界面 - 浅色调现代设计
 * Professional Digital Multimeter Interface - Light Theme Modern Design
 *
 * 设计规范:
 * - 屏幕尺寸: 800x480
 * - 电压测量: ±36V (无档位, <1V显示mV)
 * - 电流测量: ±2A (无档位, <1A显示mA)
 * - 电阻测量: 0-10MΩ, 5档 (100Ω, 1kΩ, 10kΩ, 100kΩ, 1MΩ)
 * - 采样率: 7.2M S/s
 ******************************************************************************/

#include "lvgl.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"

// 字体声明
LV_FONT_DECLARE(lv_font_Collins_66)
LV_FONT_DECLARE(lv_font_Collins2_82)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_14)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_16)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_18)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_20)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_22)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_24)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_28)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_30)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_45)

// 图标声明 - 统计图标
LV_IMG_DECLARE(_max_alpha_20x20);
LV_IMG_DECLARE(_min_alpha_20x20);
LV_IMG_DECLARE(_avg_alpha_20x20);
LV_IMG_DECLARE(_std_alpha_30x30);
// 图标声明 - 状态图标
LV_IMG_DECLARE(_running_alpha_20x20);
LV_IMG_DECLARE(_stop_alpha_20x20);
// 图标声明 - 趋势箭头
LV_IMG_DECLARE(_up_alpha_20x20);
LV_IMG_DECLARE(_down_alpha_25x25);
// 图标声明 - 图表模式切换
LV_IMG_DECLARE(_1_alpha_25x25);  // 波形图标
LV_IMG_DECLARE(_2_alpha_25x25);  // 直方图标
LV_IMG_DECLARE(_3_alpha_25x25);  // 柱状图标

/*********************
 *      DEFINES
 *********************/
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =====================================
// 浅色专业仪器界面配色方案 - Light Theme
// =====================================
// 背景色系
#define COLOR_BG_MAIN         0xF5F7FA   // 主背景 - 浅灰白
#define COLOR_BG_CARD         0xFFFFFF   // 卡片背景 - 纯白
#define COLOR_BG_CARD_HOVER   0xF0F4F8   // 卡片悬停背景

// 主色调 - 专业蓝
#define COLOR_PRIMARY         0x2563EB   // 主蓝色
#define COLOR_PRIMARY_LIGHT   0x3B82F6   // 浅蓝
#define COLOR_PRIMARY_DARK    0x1D4ED8   // 深蓝

// 功能色 - 测量模式 (数值显示专用，固定颜色)
#define COLOR_VOLTAGE         0xF59E0B   // 电压 - 琥珀色/黄色
#define COLOR_CURRENT         0xEF4444   // 电流 - 红色
#define COLOR_RESISTANCE      0x3B82F6   // 电阻 - 蓝色

// 状态色
#define COLOR_SUCCESS         0x10B981   // 成功/正极性 - 翠绿
#define COLOR_WARNING         0xF59E0B   // 警告 - 琥珀
#define COLOR_DANGER          0xEF4444   // 危险/负极性/过载 - 红
#define COLOR_INFO            0x06B6D4   // 信息 - 青

// 文字色
#define COLOR_TEXT_PRIMARY    0x1F2937   // 主文字 - 深灰
#define COLOR_TEXT_SECONDARY  0x6B7280   // 次要文字 - 中灰
#define COLOR_TEXT_TERTIARY   0x9CA3AF   // 辅助文字 - 浅灰
#define COLOR_TEXT_INVERSE    0xFFFFFF   // 反色文字 - 白

// 边框与分割
#define COLOR_BORDER          0xE5E7EB   // 边框 - 浅灰
#define COLOR_BORDER_DARK     0xD1D5DB   // 深边框
#define COLOR_DIVIDER         0xF3F4F6   // 分割线

// 阴影色
#define COLOR_SHADOW          0x000000   // 阴影基色 (配合透明度使用)

// 兼容性别名 - 映射到新配色
#define COLOR_ACCENT_BLUE     COLOR_VOLTAGE
#define COLOR_ACCENT_GREEN    COLOR_SUCCESS
#define COLOR_ACCENT_RED      COLOR_DANGER
#define COLOR_ACCENT_YELLOW   COLOR_WARNING
#define COLOR_ACCENT_ORANGE   COLOR_CURRENT
#define COLOR_ACCENT_PURPLE   COLOR_RESISTANCE
#define COLOR_OVERLOAD        COLOR_DANGER
#define COLOR_BG_DARK         COLOR_BG_MAIN
#define COLOR_BG_CARD_LIGHT   COLOR_BG_CARD
#define COLOR_TEXT_DIM        COLOR_TEXT_TERTIARY

/*********************
 *      TYPEDEFS
 *********************/
// DMM (Digital Multimeter) mode enumeration
typedef enum {
    DMM_MODE_VOLTAGE = 0,     // 电压测量模式
    DMM_MODE_CURRENT = 1,     // 电流测量模式
    DMM_MODE_RESISTANCE = 2   // 电阻测量模式
} dmm_mode_t;

// 电阻量程 - 5档 (100Ω, 1kΩ, 10kΩ, 100kΩ, 1MΩ) - 按用户要求
typedef enum {
    RES_RANGE_100R = 0,       // 0-100Ω
    RES_RANGE_1K = 1,         // 0-1kΩ
    RES_RANGE_10K = 2,        // 0-10kΩ
    RES_RANGE_100K = 3,       // 0-100kΩ
    RES_RANGE_1M = 4          // 0-1MΩ
} res_range_t;

// 趋势状态
typedef enum {
    TREND_STABLE = 0,         // 稳定
    TREND_RISING = 1,         // 上升
    TREND_FALLING = 2         // 下降
} trend_state_t;



// DMM statistics structure - 增强版统计
typedef struct {
    float max_value;           // 最大值
    float min_value;           // 最小值
    float avg_value;           // 平均值
    float rms_value;           // 有效值
    float std_dev;             // 标准差
    uint32_t sample_count;     // 采样计数
    float sum_value;           // 累加和
    float sum_square;          // 平方和
    // 短期统计（1秒窗口）
    float avg_1s;
    float max_1s;
    float min_1s;
    uint32_t count_1s;
    float sum_1s;
} dmm_stats_t;

// 图表模式
typedef enum {
    CHART_MODE_WAVEFORM = 0,  // 波形图
    CHART_MODE_HISTOGRAM = 1, // 直方图
    CHART_MODE_BAR = 2        // 柱状图
} chart_mode_t;

// DMM state structure - 按新规范
typedef struct {
    dmm_mode_t mode;
    res_range_t res_range;     // 仅电阻有档位
    bool is_hold;              // 保持模式
    bool is_recording;         // 记录中
    bool is_overload;          // 过载状态
    bool is_connected;         // 连接状态
    float current_value;       // 当前测量值
    float reference_value;     // 参考值（相对测量）
    bool relative_mode;        // 相对测量模式
    trend_state_t trend;       // 趋势状态
    float trend_percent;       // 趋势百分比
    chart_mode_t chart_mode;   // 图表模式
    uint32_t record_duration;  // 记录时长（秒）
    uint32_t record_points;    // 记录点数
    float record_max;          // 记录期间最大值
    float record_min;          // 记录期间最小值
    dmm_stats_t stats;         // 统计数据
    lv_obj_t* stat_labels[4];  // 统计值显示标签 [MAX, MIN, AVG, STD]
    lv_obj_t* y_axis_labels[3]; // Y轴坐标标签 [max, mid, min]
    lv_obj_t* y_axis_unit;     // Y轴单位标签
    lv_obj_t* polarity_label;  // 极性指示标签
    lv_obj_t* trend_label;     // 趋势指示标签 (数值旁 百分比)
    lv_obj_t* trend_arrow_img; // 趋势箭头图标
    lv_obj_t* overload_indicator; // 过载指示器
    lv_obj_t* connection_indicator; // 连接状态指示
    lv_obj_t* recording_indicator;  // 记录状态指示
    lv_obj_t* mode_label;      // 顶部模式标签
    lv_obj_t* range_label;     // 顶部量程标签
    lv_obj_t* status_label;    // 顶部连接状态
    lv_obj_t* quality_label;   // 数据质量标签
    lv_obj_t* baud_label;      // 波特率标签
    lv_obj_t* hold_status_label; // HOLD状态标签
    lv_obj_t* hold_status_icon;  // HOLD状态图标
    lv_obj_t* chart_obj;       // 图表对象
    lv_obj_t* stat_icons[4];   // 统计图标 [MAX, MIN, AVG, STD]
} dmm_state_t;

/*********************
 *  STATIC VARIABLES
 *********************/
static dmm_state_t dmm_state = {
    .mode = DMM_MODE_VOLTAGE,
    .res_range = RES_RANGE_1M,
    .is_hold = false,
    .is_recording = false,
    .is_overload = false,
    .is_connected = true,
    .current_value = 0.0f,
    .reference_value = 0.0f,
    .relative_mode = false,
    .trend = TREND_STABLE,
    .trend_percent = 0.0f,
    .chart_mode = CHART_MODE_WAVEFORM,
    .record_duration = 0,
    .record_points = 0,
    .record_max = 0.0f,
    .record_min = 0.0f,
    .stats = {
        .max_value = 0.0f,
        .min_value = 0.0f,
        .avg_value = 0.0f,
        .rms_value = 0.0f,
        .std_dev = 0.0f,
        .sample_count = 0,
        .sum_value = 0.0f,
        .sum_square = 0.0f,
        .avg_1s = 0.0f,
        .max_1s = 0.0f,
        .min_1s = 0.0f,
        .count_1s = 0,
        .sum_1s = 0.0f
    },
    .stat_labels = {NULL, NULL, NULL, NULL},
    .y_axis_labels = {NULL, NULL, NULL},
    .y_axis_unit = NULL,
    .polarity_label = NULL,
    .trend_label = NULL,
    .trend_arrow_img = NULL,
    .overload_indicator = NULL,
    .connection_indicator = NULL,
    .recording_indicator = NULL,
    .mode_label = NULL,
    .range_label = NULL,
    .status_label = NULL,
    .quality_label = NULL,
    .baud_label = NULL,
    .hold_status_label = NULL,
    .hold_status_icon = NULL,
    .chart_obj = NULL,
    .stat_icons = {NULL, NULL, NULL, NULL}
};

static lv_chart_series_t * trend_series = NULL;
static lv_timer_t * dmm_animation_timer = NULL;
static lv_ui * dmm_ui_ptr = NULL;
static lv_timer_t * dmm_time_timer = NULL;
static lv_timer_t * dmm_sim_data_timer = NULL;  // 模拟数据生成定时器
static int sim_data_index = 0;  // 模拟数据索引
static int32_t histogram_bins[10] = {0};  // 直方图分布统计(10个区间)
static int32_t chart_data_history[32] = {0};  // 保存图表历史数据用于模式切换

// 保存测量模式按钮指针，用于切换选中状态
static lv_obj_t * btn_voltage_ptr = NULL;
static lv_obj_t * btn_current_ptr = NULL;
static lv_obj_t * btn_resistance_ptr = NULL;

// 保存按钮子控件指针，用于更新选中状态下的颜色
static lv_obj_t * volt_icon_ptr = NULL;
static lv_obj_t * volt_label_ptr = NULL;
static lv_obj_t * amp_icon_ptr = NULL;
static lv_obj_t * amp_label_ptr = NULL;
static lv_obj_t * ohm_icon_ptr = NULL;
static lv_obj_t * ohm_label_ptr = NULL;

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void dmm_voltage_btn_event_cb(lv_event_t * e);
static void dmm_current_btn_event_cb(lv_event_t * e);
static void dmm_resistance_btn_event_cb(lv_event_t * e);
static void dmm_range_btn_event_cb(lv_event_t * e);
static void dmm_hold_btn_event_cb(lv_event_t * e);
static void dmm_reset_stats_btn_event_cb(lv_event_t * e);
static void dmm_chart_mode_wave_cb(lv_event_t * e);
static void dmm_chart_mode_bar_cb(lv_event_t * e);
static void dmm_chart_mode_hist_cb(lv_event_t * e);
static void dmm_time_update_cb(lv_timer_t * timer);
static void dmm_animation_timer_cb(lv_timer_t * timer);
static void dmm_sim_data_timer_cb(lv_timer_t * timer);  // 模拟数据定时器
static void update_main_display(lv_ui *ui);
static void update_trend_chart(lv_ui *ui, float value);
static void update_statistics(float new_value);
static void update_statistics_display(void);
static void apply_mode_transition_animation(lv_ui *ui, dmm_mode_t new_mode);
static void apply_range_transition_animation(lv_ui *ui);
static void update_y_axis_labels(void);
static const char* get_range_string(dmm_mode_t mode, int range);
static float get_range_max_value(dmm_mode_t mode, int range);
static const char* get_unit_string(dmm_mode_t mode);
static void reset_statistics(void);
static void update_histogram_display(void);  // 直方图显示
void dmm_update_measurement(float value);  // 公共API前向声明

// 保存图表模式按钮指针
static lv_obj_t * btn_chart_wave_ptr = NULL;
static lv_obj_t * btn_chart_bar_ptr = NULL;
static lv_obj_t * btn_chart_hist_ptr = NULL;

/*********************
 *  STATIC FUNCTIONS
 *********************/

/**
 * 重置统计数据 - 清零所有统计值和直方图
 */
static void reset_statistics(void)
{
    dmm_state.stats.max_value = 0.0f;
    dmm_state.stats.min_value = 0.0f;
    dmm_state.stats.avg_value = 0.0f;
    dmm_state.stats.rms_value = 0.0f;
    dmm_state.stats.sample_count = 0;
    dmm_state.stats.sum_value = 0.0f;
    dmm_state.stats.sum_square = 0.0f;

    // 重置直方图统计
    for (int i = 0; i < 10; i++) {
        histogram_bins[i] = 0;
    }
    sim_data_index = 0;

    // 重置图表历史数据
    for (int i = 0; i < 32; i++) {
        chart_data_history[i] = 0;
    }
}

/**
 * 更新统计数据 - 基于真实数据点进行统计
 * MAX: 所有采样点的最大值
 * MIN: 所有采样点的最小值
 * AVG: 所有采样点的算术平均值
 * RMS: 所有采样点的均方根值 (Root Mean Square)
 */
static void update_statistics(float new_value)
{
    if (dmm_state.is_hold) return;
    
    // 累加样本数和统计值
    dmm_state.stats.sample_count++;
    dmm_state.stats.sum_value += new_value;
    dmm_state.stats.sum_square += new_value * new_value;
    
    // 更新最大值 MAX - 跟踪所有数据点中的最大值
    if (dmm_state.stats.sample_count == 1) {
        // 第一个数据点，初始化MAX和MIN
        dmm_state.stats.max_value = new_value;
        dmm_state.stats.min_value = new_value;
    } else {
        // 后续数据点，比较并更新
        if (new_value > dmm_state.stats.max_value) {
            dmm_state.stats.max_value = new_value;
        }
        if (new_value < dmm_state.stats.min_value) {
            dmm_state.stats.min_value = new_value;
        }
    }
    
    // 计算平均值 AVG - 所有数据点的算术平均
    dmm_state.stats.avg_value = dmm_state.stats.sum_value / dmm_state.stats.sample_count;
    
    // 计算有效值 RMS - 均方根值
    // RMS = sqrt(平方和的平均值) = sqrt((x1² + x2² + ... + xn²) / n)
    float mean_square = dmm_state.stats.sum_square / dmm_state.stats.sample_count;
    dmm_state.stats.rms_value = sqrtf(mean_square);
}

/**
 * 更新统计值显示 - 基于真实采样数据
 * 统计值含义：
 * MAX: 最大值 - 所有采样点中的峰值
 * MIN: 最小值 - 所有采样点中的谷值
 * AVG: 平均值 - 算术平均，适用于直流测量
 * RMS: 有效值 - 均方根值，适用于交流测量
 */
static void update_statistics_display(void)
{
    if (dmm_ui_ptr == NULL) return;
    
    char buf[32];
    
    // 根据测量模式选择合适的显示格式
    const char* format = "%.3f";  // 默认3位小数
    
    // 如果是电阻模式且值较大，使用kΩ显示
    bool use_k_unit = (dmm_state.mode == DMM_MODE_RESISTANCE);
    float scale = use_k_unit ? 1000.0f : 1.0f;
    
    // 更新MAX - 最大值
    if (dmm_state.stat_labels[0] != NULL && lv_obj_is_valid(dmm_state.stat_labels[0])) {
        float display_val = dmm_state.stats.max_value;
        if (use_k_unit && display_val >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.2fk", display_val / scale);
        } else {
            snprintf(buf, sizeof(buf), format, display_val);
        }
        lv_label_set_text(dmm_state.stat_labels[0], buf);
    }
    
    // 更新MIN - 最小值
    if (dmm_state.stat_labels[1] != NULL && lv_obj_is_valid(dmm_state.stat_labels[1])) {
        float display_val = dmm_state.stats.min_value;
        if (use_k_unit && display_val >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.2fk", display_val / scale);
        } else {
            snprintf(buf, sizeof(buf), format, display_val);
        }
        lv_label_set_text(dmm_state.stat_labels[1], buf);
    }
    
    // 更新AVG - 平均值
    if (dmm_state.stat_labels[2] != NULL && lv_obj_is_valid(dmm_state.stat_labels[2])) {
        float display_val = dmm_state.stats.avg_value;
        if (use_k_unit && display_val >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.2fk", display_val / scale);
        } else {
            snprintf(buf, sizeof(buf), format, display_val);
        }
        lv_label_set_text(dmm_state.stat_labels[2], buf);
    }
    
    // 更新RMS - 有效值（均方根值）
    if (dmm_state.stat_labels[3] != NULL && lv_obj_is_valid(dmm_state.stat_labels[3])) {
        float display_val = dmm_state.stats.rms_value;
        if (use_k_unit && display_val >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.2fk", display_val / scale);
        } else {
            snprintf(buf, sizeof(buf), format, display_val);
        }
        lv_label_set_text(dmm_state.stat_labels[3], buf);
    }
}

/**
 * 获取单位字符串 - 电阻根据档位显示不同单位
 */
static const char* get_unit_string(dmm_mode_t mode)
{
    switch (mode) {
        case DMM_MODE_VOLTAGE: return "V";
        case DMM_MODE_CURRENT: return "A";
        case DMM_MODE_RESISTANCE:
            // 根据电阻档位返回对应单位
            switch (dmm_state.res_range) {
                case RES_RANGE_100R: return "R";
                case RES_RANGE_1K:   return "kR";
                case RES_RANGE_10K:  return "kR";
                case RES_RANGE_100K: return "kR";
                case RES_RANGE_1M:   return "MR";
                default: return "R";
            }
        default: return "";
    }
}

/**
 * 获取挡位显示字符串 - 按新规范
 * 电压: ±36V (无档位)
 * 电流: ±2A (无档位)
 * 电阻: 5档 (100Ω, 1kΩ, 10kΩ, 100kΩ, 1MΩ)
 */
static const char* get_range_string(dmm_mode_t mode, int range)
{
    if (mode == DMM_MODE_VOLTAGE) {
        return "36V";  // 固定量程
    } else if (mode == DMM_MODE_CURRENT) {
        return "2A";   // 固定量程
    } else if (mode == DMM_MODE_RESISTANCE) {
        switch (range) {
            case RES_RANGE_100R: return "100R";
            case RES_RANGE_1K: return "1kR";
            case RES_RANGE_10K: return "10kR";
            case RES_RANGE_100K: return "100kR";
            case RES_RANGE_1M: return "1MR";
            default: return "1MR";
        }
    }
    return "";
}

/**
 * 获取量程最大值 - 按新规范
 */
static float get_range_max_value(dmm_mode_t mode, int range)
{
    if (mode == DMM_MODE_VOLTAGE) {
        return 36.0f;   // ±36V
    } else if (mode == DMM_MODE_CURRENT) {
        return 2.0f;    // ±2A
    } else if (mode == DMM_MODE_RESISTANCE) {
        switch (range) {
            case RES_RANGE_100R: return 100.0f;
            case RES_RANGE_1K: return 1000.0f;
            case RES_RANGE_10K: return 10000.0f;
            case RES_RANGE_100K: return 100000.0f;
            case RES_RANGE_1M: return 1000000.0f;
            default: return 1000000.0f;
        }
    }
    return 100.0f;
}

/**
 * 格式化测量值显示 - 带智能单位切换
 * 电压: <1V显示mV, 否则显示V
 * 电流: <1A显示mA, 否则显示A
 * 电阻: 根据档位自动切换Ω/kΩ/MΩ
 */
static void format_measurement_value(char* buf, size_t buf_size, float value, dmm_mode_t mode, const char** unit_out)
{
    if (mode == DMM_MODE_VOLTAGE) {
        float abs_val = fabsf(value);
        if (abs_val < 1.0f && abs_val > 0.0001f) {
            snprintf(buf, buf_size, "%.2f", value * 1000.0f);
            *unit_out = "mV";
        } else {
            snprintf(buf, buf_size, "%.4f", value);
            *unit_out = "V";
        }
    } else if (mode == DMM_MODE_CURRENT) {
        float abs_val = fabsf(value);
        if (abs_val < 1.0f && abs_val > 0.0001f) {
            snprintf(buf, buf_size, "%.2f", value * 1000.0f);
            *unit_out = "mA";
        } else {
            snprintf(buf, buf_size, "%.4f", value);
            *unit_out = "A";
        }
    } else if (mode == DMM_MODE_RESISTANCE) {
        // 根据档位决定显示单位和数值缩放
        switch (dmm_state.res_range) {
            case RES_RANGE_100R:
                // 100R档位: 直接显示欧姆值
                snprintf(buf, buf_size, "%.2f", value);
                *unit_out = "R";
                break;
            case RES_RANGE_1K:
                // 1kR档位: 显示千欧，保留3位小数
                snprintf(buf, buf_size, "%.3f", value / 1000.0f);
                *unit_out = "kR";
                break;
            case RES_RANGE_10K:
                // 10kR档位: 显示千欧，保留2位小数
                snprintf(buf, buf_size, "%.2f", value / 1000.0f);
                *unit_out = "kR";
                break;
            case RES_RANGE_100K:
                // 100kR档位: 显示千欧，保留1位小数
                snprintf(buf, buf_size, "%.1f", value / 1000.0f);
                *unit_out = "kR";
                break;
            case RES_RANGE_1M:
                // 1MR档位: 显示兆欧
                snprintf(buf, buf_size, "%.4f", value / 1000000.0f);
                *unit_out = "MR";
                break;
            default:
                snprintf(buf, buf_size, "%.4f", value);
                *unit_out = "R";
                break;
        }
    }
}

/**
 * 获取极性符号 - 0或正值显示+，负值显示-
 */
static const char* get_polarity_symbol(float value)
{
    if (value < -0.0001f) return "-";
    return "+";  // 0或正值都显示+
}

/**
 * 获取数值显示颜色 - 根据测量模式返回固定颜色
 * 电压：琥珀色，电流：红色，电阻：蓝色
 */
static uint32_t get_value_display_color(void)
{
    switch (dmm_state.mode) {
        case DMM_MODE_VOLTAGE:
            return COLOR_VOLTAGE;     // 琥珀色
        case DMM_MODE_CURRENT:
            return COLOR_CURRENT;     // 红色
        case DMM_MODE_RESISTANCE:
            return COLOR_RESISTANCE;  // 蓝色
        default:
            return COLOR_VOLTAGE;
    }
}

/**
 * 获取趋势符号
 */
static const char* get_trend_symbol(trend_state_t trend)
{
    switch (trend) {
        case TREND_RISING: return "↑";
        case TREND_FALLING: return "↓";
        case TREND_STABLE:
        default: return "→";
    }
}

/**
 * 更新Y轴坐标标签
 */
static void update_y_axis_labels(void)
{
    if (dmm_ui_ptr == NULL) return;

    char buf[16];
    // 电压和电流无档位，电阻有5档
    float max_val = get_range_max_value(dmm_state.mode, dmm_state.res_range);
    
    const char* unit = get_unit_string(dmm_state.mode);
    
    // 更新Y轴单位标签
    if (dmm_state.y_axis_unit != NULL && lv_obj_is_valid(dmm_state.y_axis_unit)) {
        lv_label_set_text(dmm_state.y_axis_unit, unit);
    }
    
    // 更新最大值标签
    if (dmm_state.y_axis_labels[0] != NULL && lv_obj_is_valid(dmm_state.y_axis_labels[0])) {
        if (dmm_state.mode == DMM_MODE_RESISTANCE && max_val >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.0fk", max_val / 1000.0f);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", max_val);
        }
        lv_label_set_text(dmm_state.y_axis_labels[0], buf);
    }
    
    // 更新中间值标签
    if (dmm_state.y_axis_labels[1] != NULL && lv_obj_is_valid(dmm_state.y_axis_labels[1])) {
        float mid_val = max_val / 2.0f;
        if (dmm_state.mode == DMM_MODE_RESISTANCE && mid_val >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.0fk", mid_val / 1000.0f);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", mid_val);
        }
        lv_label_set_text(dmm_state.y_axis_labels[1], buf);
    }
    
    // 更新最小值标签 (0)
    if (dmm_state.y_axis_labels[2] != NULL && lv_obj_is_valid(dmm_state.y_axis_labels[2])) {
        lv_label_set_text(dmm_state.y_axis_labels[2], "0");
    }
}

/**
 * Update main display value based on current mode
 * 使用智能单位切换和极性/趋势指示
 * 颜色固定：电压-琥珀色，电流-红色，电阻-蓝色
 */
static void update_main_display(lv_ui *ui)
{
    if (ui == NULL || !lv_obj_is_valid(ui->scrPrintMenu_labelUSB)) return;

    char buf[64];
    const char* unit = "";

    // 使用智能格式化函数
    format_measurement_value(buf, sizeof(buf), dmm_state.current_value, dmm_state.mode, &unit);

    lv_label_set_text(ui->scrPrintMenu_labelUSB, buf);

    // 获取基于模式的固定显示颜色
    uint32_t display_color = get_value_display_color();

    // 更新极性指示 - 使用黑色
    if (dmm_state.polarity_label != NULL && lv_obj_is_valid(dmm_state.polarity_label)) {
        const char* polarity = get_polarity_symbol(dmm_state.current_value);
        lv_label_set_text(dmm_state.polarity_label, polarity);
        lv_obj_set_style_text_color(dmm_state.polarity_label, lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新主数值颜色
    lv_obj_set_style_text_color(ui->scrPrintMenu_labelUSB, lv_color_hex(display_color), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 更新单位显示 - 根据智能格式化的结果更新单位文字和颜色
    if (dmm_state.y_axis_unit != NULL && lv_obj_is_valid(dmm_state.y_axis_unit)) {
        lv_label_set_text(dmm_state.y_axis_unit, unit);  // 使用格式化后的单位
        lv_obj_set_style_text_color(dmm_state.y_axis_unit, lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 趋势指示由 dmm_update_value 中的百分比显示处理，这里不再覆盖

    // 更新测量模式显示
    const char* mode_str = "";
    uint32_t mode_color = COLOR_ACCENT_BLUE;
    switch (dmm_state.mode) {
        case DMM_MODE_VOLTAGE:
            mode_str = "VOLTAGE";
            mode_color = COLOR_ACCENT_BLUE;
            break;
        case DMM_MODE_CURRENT:
            mode_str = "CURRENT";
            mode_color = COLOR_ACCENT_ORANGE;
            break;
        case DMM_MODE_RESISTANCE:
            mode_str = "RESISTANCE";
            mode_color = COLOR_ACCENT_PURPLE;
            break;
    }

    if (lv_obj_is_valid(ui->scrPrintMenu_contInternet)) {
        lv_label_set_text(ui->scrPrintMenu_contInternet, mode_str);
        lv_obj_set_style_text_color(ui->scrPrintMenu_contInternet, lv_color_hex(mode_color), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新挡位显示
    if (lv_obj_is_valid(ui->scrPrintMenu_contMobile)) {
        const char* range_str = get_range_string(dmm_state.mode, dmm_state.res_range);
        lv_label_set_text(ui->scrPrintMenu_contMobile, range_str);
    }

    // 检查过载状态
    float max_val = get_range_max_value(dmm_state.mode, dmm_state.res_range);
    bool is_overload = fabsf(dmm_state.current_value) > max_val * 0.95f;

    if (dmm_state.overload_indicator != NULL && lv_obj_is_valid(dmm_state.overload_indicator)) {
        if (is_overload) {
            lv_obj_clear_flag(dmm_state.overload_indicator, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(dmm_state.overload_indicator, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * Update trend chart with new value - 带平滑过渡
 */
static void update_trend_chart(lv_ui *ui, float value)
{
    if (ui == NULL || !lv_obj_is_valid(ui->scrPrintMenu_imgUSB) || trend_series == NULL) return;
    
    // 根据当前挡位获取最大值 - 电压电流无档位，电阻有档位
    float max_val = get_range_max_value(dmm_state.mode, dmm_state.res_range);

    // 缩放数值到0-100范围，带限制
    int32_t chart_value = (int32_t)((value / max_val) * 100.0f);

    if (chart_value < 0) chart_value = 0;
    if (chart_value > 100) chart_value = 100;

    // 保存到历史数据数组(循环覆盖)
    static int history_write_idx = 0;
    chart_data_history[history_write_idx] = chart_value;
    history_write_idx = (history_write_idx + 1) % 32;

    // 添加到图表
    lv_chart_set_next_value(ui->scrPrintMenu_imgUSB, trend_series, chart_value);
    lv_chart_refresh(ui->scrPrintMenu_imgUSB);
}

/**
 * Apply smooth transition animation when mode changes
 */
static void apply_mode_transition_animation(lv_ui *ui, dmm_mode_t new_mode)
{
    if (ui == NULL || !lv_obj_is_valid(ui->scrPrintMenu_labelUSB)) return;
    
    // 淡出动画 - 主数值显示
    lv_anim_t anim_out;
    lv_anim_init(&anim_out);
    lv_anim_set_var(&anim_out, ui->scrPrintMenu_labelUSB);
    lv_anim_set_values(&anim_out, LV_OPA_COVER, LV_OPA_20);
    lv_anim_set_time(&anim_out, 250);
    lv_anim_set_exec_cb(&anim_out, (lv_anim_exec_xcb_t)lv_obj_set_style_text_opa);
    lv_anim_start(&anim_out);
    
    // 淡入动画
    lv_anim_t anim_in;
    lv_anim_init(&anim_in);
    lv_anim_set_var(&anim_in, ui->scrPrintMenu_labelUSB);
    lv_anim_set_values(&anim_in, LV_OPA_20, LV_OPA_COVER);
    lv_anim_set_time(&anim_in, 250);
    lv_anim_set_delay(&anim_in, 250);
    lv_anim_set_exec_cb(&anim_in, (lv_anim_exec_xcb_t)lv_obj_set_style_text_opa);
    lv_anim_start(&anim_in);
    
    // 单位标签缩放动画
    if (lv_obj_is_valid(ui->scrPrintMenu_contInternet)) {
        lv_anim_t anim_zoom;
        lv_anim_init(&anim_zoom);
        lv_anim_set_var(&anim_zoom, ui->scrPrintMenu_contInternet);
        lv_anim_set_values(&anim_zoom, 256, 300);  // 放大
        lv_anim_set_time(&anim_zoom, 150);
        lv_anim_set_playback_time(&anim_zoom, 150);
        lv_anim_set_exec_cb(&anim_zoom, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_start(&anim_zoom);
    }
}

/**
 * Apply range transition animation
 */
static void apply_range_transition_animation(lv_ui *ui)
{
    if (ui == NULL || !lv_obj_is_valid(ui->scrPrintMenu_contMobile)) return;
    
    // 挡位指示器脉冲动画
    lv_anim_t anim_pulse;
    lv_anim_init(&anim_pulse);
    lv_anim_set_var(&anim_pulse, ui->scrPrintMenu_contMobile);
    lv_anim_set_values(&anim_pulse, LV_OPA_COVER, LV_OPA_80);
    lv_anim_set_time(&anim_pulse, 200);
    lv_anim_set_playback_time(&anim_pulse, 200);
    lv_anim_set_exec_cb(&anim_pulse, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
    lv_anim_start(&anim_pulse);
}

/**
 * Voltage mode button event handler
 */
static void dmm_voltage_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_ui * ui = (lv_ui*)lv_event_get_user_data(e);
    if (ui == NULL) return;

    dmm_state.mode = DMM_MODE_VOLTAGE;

    // 更新按钮选中状态
    if (btn_voltage_ptr != NULL && lv_obj_is_valid(btn_voltage_ptr)) {
        lv_obj_add_state(btn_voltage_ptr, LV_STATE_CHECKED);
    }
    if (btn_current_ptr != NULL && lv_obj_is_valid(btn_current_ptr)) {
        lv_obj_clear_state(btn_current_ptr, LV_STATE_CHECKED);
    }
    if (btn_resistance_ptr != NULL && lv_obj_is_valid(btn_resistance_ptr)) {
        lv_obj_clear_state(btn_resistance_ptr, LV_STATE_CHECKED);
    }

    // 更新按钮子控件颜色 - VOLTAGE选中(白色), 其他未选中(原色)
    if (volt_icon_ptr && lv_obj_is_valid(volt_icon_ptr)) {
        lv_obj_set_style_img_recolor(volt_icon_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (volt_label_ptr && lv_obj_is_valid(volt_label_ptr)) {
        lv_obj_set_style_text_color(volt_label_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (amp_icon_ptr && lv_obj_is_valid(amp_icon_ptr)) {
        lv_obj_set_style_img_recolor(amp_icon_ptr, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (amp_label_ptr && lv_obj_is_valid(amp_label_ptr)) {
        lv_obj_set_style_text_color(amp_label_ptr, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (ohm_icon_ptr && lv_obj_is_valid(ohm_icon_ptr)) {
        lv_obj_set_style_img_recolor(ohm_icon_ptr, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (ohm_label_ptr && lv_obj_is_valid(ohm_label_ptr)) {
        lv_obj_set_style_text_color(ohm_label_ptr, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新顶部状态栏 - 模式和量程
    if (dmm_state.mode_label != NULL && lv_obj_is_valid(dmm_state.mode_label)) {
        lv_label_set_text(dmm_state.mode_label, "VOLTAGE");
        lv_obj_set_style_text_color(dmm_state.mode_label, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (dmm_state.range_label != NULL && lv_obj_is_valid(dmm_state.range_label)) {
        lv_label_set_text(dmm_state.range_label, "36V");
        lv_obj_set_style_text_color(dmm_state.range_label, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新Y轴坐标
    update_y_axis_labels();
    update_main_display(ui);

    // 重置统计数据
    reset_statistics();
    update_statistics_display();
    
    // 应用过渡动画
    apply_mode_transition_animation(ui, DMM_MODE_VOLTAGE);
}

/**
 * Current mode button event handler
 */
static void dmm_current_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_ui * ui = (lv_ui*)lv_event_get_user_data(e);
    if (ui == NULL) return;

    dmm_state.mode = DMM_MODE_CURRENT;

    // 更新按钮选中状态
    if (btn_voltage_ptr != NULL && lv_obj_is_valid(btn_voltage_ptr)) {
        lv_obj_clear_state(btn_voltage_ptr, LV_STATE_CHECKED);
    }
    if (btn_current_ptr != NULL && lv_obj_is_valid(btn_current_ptr)) {
        lv_obj_add_state(btn_current_ptr, LV_STATE_CHECKED);
    }
    if (btn_resistance_ptr != NULL && lv_obj_is_valid(btn_resistance_ptr)) {
        lv_obj_clear_state(btn_resistance_ptr, LV_STATE_CHECKED);
    }

    // 更新按钮子控件颜色 - CURRENT选中(白色), 其他未选中(原色)
    if (volt_icon_ptr && lv_obj_is_valid(volt_icon_ptr)) {
        lv_obj_set_style_img_recolor(volt_icon_ptr, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (volt_label_ptr && lv_obj_is_valid(volt_label_ptr)) {
        lv_obj_set_style_text_color(volt_label_ptr, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (amp_icon_ptr && lv_obj_is_valid(amp_icon_ptr)) {
        lv_obj_set_style_img_recolor(amp_icon_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (amp_label_ptr && lv_obj_is_valid(amp_label_ptr)) {
        lv_obj_set_style_text_color(amp_label_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (ohm_icon_ptr && lv_obj_is_valid(ohm_icon_ptr)) {
        lv_obj_set_style_img_recolor(ohm_icon_ptr, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (ohm_label_ptr && lv_obj_is_valid(ohm_label_ptr)) {
        lv_obj_set_style_text_color(ohm_label_ptr, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新顶部状态栏 - 模式和量程
    if (dmm_state.mode_label != NULL && lv_obj_is_valid(dmm_state.mode_label)) {
        lv_label_set_text(dmm_state.mode_label, "CURRENT");
        lv_obj_set_style_text_color(dmm_state.mode_label, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (dmm_state.range_label != NULL && lv_obj_is_valid(dmm_state.range_label)) {
        lv_label_set_text(dmm_state.range_label, "2A");
        lv_obj_set_style_text_color(dmm_state.range_label, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新Y轴坐标
    update_y_axis_labels();
    update_main_display(ui);

    // 重置统计数据
    reset_statistics();
    update_statistics_display();
    
    // 应用过渡动画
    apply_mode_transition_animation(ui, DMM_MODE_CURRENT);
}

/**
 * Resistance mode button event handler
 */
static void dmm_resistance_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_ui * ui = (lv_ui*)lv_event_get_user_data(e);
    if (ui == NULL) return;

    dmm_state.mode = DMM_MODE_RESISTANCE;

    // 更新按钮选中状态
    if (btn_voltage_ptr != NULL && lv_obj_is_valid(btn_voltage_ptr)) {
        lv_obj_clear_state(btn_voltage_ptr, LV_STATE_CHECKED);
    }
    if (btn_current_ptr != NULL && lv_obj_is_valid(btn_current_ptr)) {
        lv_obj_clear_state(btn_current_ptr, LV_STATE_CHECKED);
    }
    if (btn_resistance_ptr != NULL && lv_obj_is_valid(btn_resistance_ptr)) {
        lv_obj_add_state(btn_resistance_ptr, LV_STATE_CHECKED);
    }

    // 更新按钮子控件颜色 - RESISTANCE选中(白色), 其他未选中(原色)
    if (volt_icon_ptr && lv_obj_is_valid(volt_icon_ptr)) {
        lv_obj_set_style_img_recolor(volt_icon_ptr, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (volt_label_ptr && lv_obj_is_valid(volt_label_ptr)) {
        lv_obj_set_style_text_color(volt_label_ptr, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (amp_icon_ptr && lv_obj_is_valid(amp_icon_ptr)) {
        lv_obj_set_style_img_recolor(amp_icon_ptr, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (amp_label_ptr && lv_obj_is_valid(amp_label_ptr)) {
        lv_obj_set_style_text_color(amp_label_ptr, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (ohm_icon_ptr && lv_obj_is_valid(ohm_icon_ptr)) {
        lv_obj_set_style_img_recolor(ohm_icon_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if (ohm_label_ptr && lv_obj_is_valid(ohm_label_ptr)) {
        lv_obj_set_style_text_color(ohm_label_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新顶部状态栏 - 模式和量程
    if (dmm_state.mode_label != NULL && lv_obj_is_valid(dmm_state.mode_label)) {
        lv_label_set_text(dmm_state.mode_label, "RESISTANCE");
        lv_obj_set_style_text_color(dmm_state.mode_label, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    // 根据当前量程更新量程显示
    if (dmm_state.range_label != NULL && lv_obj_is_valid(dmm_state.range_label)) {
        const char* range_str = get_range_string(DMM_MODE_RESISTANCE, dmm_state.res_range);
        lv_label_set_text(dmm_state.range_label, range_str);
        lv_obj_set_style_text_color(dmm_state.range_label, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 更新Y轴坐标
    update_y_axis_labels();
    update_main_display(ui);

    // 重置统计数据
    reset_statistics();
    update_statistics_display();
    
    // 应用过渡动画
    apply_mode_transition_animation(ui, DMM_MODE_RESISTANCE);
}

/**
 * Range button event handler (cycle through 5 ranges)
 */
static void dmm_range_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_ui * ui = (lv_ui*)lv_event_get_user_data(e);
    if (ui == NULL) return;

    // 仅电阻模式有档位切换 (5档: 100Ω, 1kΩ, 10kΩ, 100kΩ, 1MΩ)
    // 电压(±36V)和电流(±2A)为固定量程
    if (dmm_state.mode == DMM_MODE_RESISTANCE) {
        dmm_state.res_range = (res_range_t)((dmm_state.res_range + 1) % 5);

        // 更新Y轴坐标标签
        update_y_axis_labels();
        update_main_display(ui);

        // 应用挡位切换动画
        apply_range_transition_animation(ui);
    }
    // 电压和电流模式点击RANGE按钮无效果（固定量程）
}

// 保存HOLD按钮内的图标和文字对象指针
static lv_obj_t * hold_icon_ptr = NULL;
static lv_obj_t * hold_label_ptr = NULL;

/**
 * Hold button event handler
 * HOLD功能：
 * - 未暂停(is_hold=false): 显示_a_alpha_24x24图标（运行中），测量和波形正常更新
 * - 已暂停(is_hold=true): 显示_b_alpha_24x24图标（暂停），测量和波形停止更新
 */
static void dmm_hold_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_ui * ui = (lv_ui*)lv_event_get_user_data(e);
    if (ui == NULL) return;

    // 切换保持状态
    dmm_state.is_hold = !dmm_state.is_hold;

    // 更新按钮外观
    lv_obj_t * btn = lv_event_get_target(e);
    if (dmm_state.is_hold) {
        // 暂停状态
        lv_obj_add_state(btn, LV_STATE_CHECKED);
        // 切换图标和颜色
        if (hold_icon_ptr != NULL && lv_obj_is_valid(hold_icon_ptr)) {
            lv_img_set_src(hold_icon_ptr, &_b_alpha_24x24);
            lv_obj_set_style_img_recolor(hold_icon_ptr, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        // 更新按钮文字 - HOLD状态时文字变红色
        if (hold_label_ptr != NULL && lv_obj_is_valid(hold_label_ptr)) {
            lv_label_set_text(hold_label_ptr, "HOLD");
            lv_obj_set_style_text_color(hold_label_ptr, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        // 更新顶部HOLD状态图标
        if (dmm_state.hold_status_icon != NULL && lv_obj_is_valid(dmm_state.hold_status_icon)) {
            lv_img_set_src(dmm_state.hold_status_icon, &_stop_alpha_20x20);
            lv_obj_set_style_img_recolor(dmm_state.hold_status_icon, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        // 更新顶部HOLD状态文字
        if (dmm_state.hold_status_label != NULL && lv_obj_is_valid(dmm_state.hold_status_label)) {
            lv_label_set_text(dmm_state.hold_status_label, "HOLD");
            lv_obj_set_style_text_color(dmm_state.hold_status_label, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
    } else {
        // 运行状态
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
        // 切换图标和颜色
        if (hold_icon_ptr != NULL && lv_obj_is_valid(hold_icon_ptr)) {
            lv_img_set_src(hold_icon_ptr, &_a_alpha_24x24);
            lv_obj_set_style_img_recolor(hold_icon_ptr, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        // 更新按钮文字 - RUN状态时文字变绿色
        if (hold_label_ptr != NULL && lv_obj_is_valid(hold_label_ptr)) {
            lv_label_set_text(hold_label_ptr, "RUN");
            lv_obj_set_style_text_color(hold_label_ptr, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        // 更新顶部HOLD状态图标
        if (dmm_state.hold_status_icon != NULL && lv_obj_is_valid(dmm_state.hold_status_icon)) {
            lv_img_set_src(dmm_state.hold_status_icon, &_running_alpha_20x20);
            lv_obj_set_style_img_recolor(dmm_state.hold_status_icon, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        // 更新顶部HOLD状态文字
        if (dmm_state.hold_status_label != NULL && lv_obj_is_valid(dmm_state.hold_status_label)) {
            lv_label_set_text(dmm_state.hold_status_label, "RUNNING");
            lv_obj_set_style_text_color(dmm_state.hold_status_label, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
    }
}

/**
 * Reset statistics button event handler
 */
static void dmm_reset_stats_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    lv_ui * ui = (lv_ui*)lv_event_get_user_data(e);
    if (ui == NULL) return;
    
    // 重置统计数据
    reset_statistics();
    update_statistics_display();
    
    // 播放重置动画
    lv_obj_t * btn = lv_event_get_target(e);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, btn);
    lv_anim_set_values(&anim, 256, 280);
    lv_anim_set_time(&anim, 100);
    lv_anim_set_playback_time(&anim, 100);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
    lv_anim_start(&anim);
}

/**
 * Time update timer callback - 更新运行时间显示
 */
static void dmm_time_update_cb(lv_timer_t * timer)
{
    lv_ui * ui = dmm_ui_ptr;
    if (ui == NULL || !lv_obj_is_valid(ui->scrPrintMenu) || 
        !lv_obj_is_valid(ui->scrPrintMenu_labelnternet)) return;
    
    static uint32_t seconds = 0;
    seconds++;
    
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    char time_buf[16];
    snprintf(time_buf, sizeof(time_buf), "%02u:%02u:%02u", hours, minutes, secs);
    lv_label_set_text(ui->scrPrintMenu_labelnternet, time_buf);
}

/**
 * Animation timer callback
 */
static void dmm_animation_timer_cb(lv_timer_t * timer)
{
    (void)timer;  // 消除未使用警告
}

/**
 * 更新直方图显示 - 显示数据分布
 */
static void update_histogram_display(void)
{
    if (dmm_state.chart_obj == NULL || !lv_obj_is_valid(dmm_state.chart_obj)) return;
    if (trend_series == NULL) return;

    // 清空图表并设置为10个柱子(对应10个区间)
    lv_chart_set_point_count(dmm_state.chart_obj, 10);

    // 找出最大频次用于归一化
    int32_t max_freq = 1;
    for (int i = 0; i < 10; i++) {
        if (histogram_bins[i] > max_freq) max_freq = histogram_bins[i];
    }

    // 填充直方图数据(归一化到0-100)
    for (int i = 0; i < 10; i++) {
        int32_t bar_height = (histogram_bins[i] * 100) / max_freq;
        lv_chart_set_value_by_id(dmm_state.chart_obj, trend_series, i, bar_height);
    }
    lv_chart_refresh(dmm_state.chart_obj);
}

/**
 * 更新图表模式按钮样式
 */
static void update_chart_mode_buttons(chart_mode_t mode)
{
    // 统一使用深黄色(0xD97706)作为选中背景色
    if (btn_chart_wave_ptr && lv_obj_is_valid(btn_chart_wave_ptr)) {
        if (mode == CHART_MODE_WAVEFORM) {
            lv_obj_set_style_bg_color(btn_chart_wave_ptr, lv_color_hex(0xD97706), LV_PART_MAIN|LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(btn_chart_wave_ptr, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
    }
    if (btn_chart_bar_ptr && lv_obj_is_valid(btn_chart_bar_ptr)) {
        if (mode == CHART_MODE_BAR) {
            lv_obj_set_style_bg_color(btn_chart_bar_ptr, lv_color_hex(0xD97706), LV_PART_MAIN|LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(btn_chart_bar_ptr, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
    }
    if (btn_chart_hist_ptr && lv_obj_is_valid(btn_chart_hist_ptr)) {
        if (mode == CHART_MODE_HISTOGRAM) {
            lv_obj_set_style_bg_color(btn_chart_hist_ptr, lv_color_hex(0xD97706), LV_PART_MAIN|LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(btn_chart_hist_ptr, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
    }

    // 切换图表类型和显示
    if (dmm_state.chart_obj && lv_obj_is_valid(dmm_state.chart_obj) && trend_series != NULL) {
        if (mode == CHART_MODE_WAVEFORM) {
            lv_chart_set_type(dmm_state.chart_obj, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(dmm_state.chart_obj, 32);
            // 恢复历史数据
            for (int i = 0; i < 32; i++) {
                lv_chart_set_value_by_id(dmm_state.chart_obj, trend_series, i, chart_data_history[i]);
            }
        } else if (mode == CHART_MODE_BAR) {
            lv_chart_set_type(dmm_state.chart_obj, LV_CHART_TYPE_BAR);
            lv_chart_set_point_count(dmm_state.chart_obj, 32);
            // 恢复历史数据
            for (int i = 0; i < 32; i++) {
                lv_chart_set_value_by_id(dmm_state.chart_obj, trend_series, i, chart_data_history[i]);
            }
        } else if (mode == CHART_MODE_HISTOGRAM) {
            lv_chart_set_type(dmm_state.chart_obj, LV_CHART_TYPE_BAR);
            update_histogram_display();  // 显示直方图分布
        }
        lv_chart_refresh(dmm_state.chart_obj);
    }
}

/**
 * 波形图模式按钮回调
 */
static void dmm_chart_mode_wave_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    dmm_state.chart_mode = CHART_MODE_WAVEFORM;
    update_chart_mode_buttons(CHART_MODE_WAVEFORM);
}

/**
 * 柱状图模式按钮回调
 */
static void dmm_chart_mode_bar_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    dmm_state.chart_mode = CHART_MODE_BAR;
    update_chart_mode_buttons(CHART_MODE_BAR);
}

/**
 * 直方图模式按钮回调
 */
static void dmm_chart_mode_hist_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    dmm_state.chart_mode = CHART_MODE_HISTOGRAM;
    update_chart_mode_buttons(CHART_MODE_HISTOGRAM);
}

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * PUBLIC API: 从ESP32更新万用表测量值
 * 
 * @param value - 测量值，根据当前模式自动选择：
 *                电压模式: 0-36V
 *                电流模式: 0-2A
 *                电阻模式: 0-1MΩ
 * 
 * 功能说明：
 * 1. 更新主显示数值
 * 2. 自动计算并更新统计值（MAX/MIN/AVG/RMS）
 * 3. 更新实时波形图
 * 4. 根据当前挡位自动缩放显示
 * 
 * 统计值说明：
 * - MAX: 所有采样点的最大值（峰值）
 * - MIN: 所有采样点的最小值（谷值）
 * - AVG: 所有采样点的算术平均值（适用于直流）
 * - RMS: 所有采样点的均方根值（适用于交流，有效值）
 * 
 * ESP32使用示例:
 *   // 周期性调用此函数更新测量值
 *   float measurement = read_adc();
 *   dmm_update_measurement(measurement);
 */
void dmm_update_measurement(float value)
{
    if (dmm_ui_ptr == NULL || !lv_obj_is_valid(dmm_ui_ptr->scrPrintMenu)) return;
    if (dmm_state.is_hold) return;  // HOLD模式下不更新

    lv_ui *ui = dmm_ui_ptr;

    // 保存上一次的值用于趋势计算
    static float last_value = 0.0f;
    static bool first_sample = true;

    // 更新当前值
    dmm_state.current_value = value;

    // 计算趋势（与上次值比较）
    if (!first_sample && fabsf(last_value) > 0.0001f) {
        float change = value - last_value;
        float percent = (change / fabsf(last_value)) * 100.0f;
        dmm_state.trend_percent = percent;

        // 根据变化幅度确定趋势状态（阈值0.5%）
        if (percent > 0.5f) {
            dmm_state.trend = TREND_RISING;
        } else if (percent < -0.5f) {
            dmm_state.trend = TREND_FALLING;
        } else {
            dmm_state.trend = TREND_STABLE;
        }

        // 更新趋势箭头图标显示 - 始终显示
        if (dmm_state.trend_arrow_img != NULL && lv_obj_is_valid(dmm_state.trend_arrow_img)) {
            if (dmm_state.trend == TREND_RISING) {
                lv_img_set_src(dmm_state.trend_arrow_img, &_up_alpha_20x20);
                lv_obj_set_style_img_recolor(dmm_state.trend_arrow_img, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
            } else if (dmm_state.trend == TREND_FALLING) {
                lv_img_set_src(dmm_state.trend_arrow_img, &_down_alpha_25x25);
                lv_obj_set_style_img_recolor(dmm_state.trend_arrow_img, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
            } else {
                // 稳定时显示up图标，灰色
                lv_img_set_src(dmm_state.trend_arrow_img, &_up_alpha_20x20);
                lv_obj_set_style_img_recolor(dmm_state.trend_arrow_img, lv_color_hex(0x94A3B8), LV_PART_MAIN|LV_STATE_DEFAULT);
            }
        }

        // 更新趋势百分比显示 - 始终显示
        if (dmm_state.trend_label != NULL && lv_obj_is_valid(dmm_state.trend_label)) {
            char trend_buf[16];
            snprintf(trend_buf, sizeof(trend_buf), "%.2f%%", fabsf(percent));
            lv_label_set_text(dmm_state.trend_label, trend_buf);
            if (dmm_state.trend == TREND_RISING) {
                lv_obj_set_style_text_color(dmm_state.trend_label, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
            } else if (dmm_state.trend == TREND_FALLING) {
                lv_obj_set_style_text_color(dmm_state.trend_label, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
            } else {
                // 稳定时显示灰色
                lv_obj_set_style_text_color(dmm_state.trend_label, lv_color_hex(0x94A3B8), LV_PART_MAIN|LV_STATE_DEFAULT);
            }
        }
    }
    first_sample = false;
    last_value = value;

    // 更新统计数据（基于真实采样点）
    update_statistics(value);

    // 更新主显示
    update_main_display(ui);

    // 更新统计值显示
    update_statistics_display();

    // 更新波形图
    update_trend_chart(ui, value);
}

/**
 * 模拟数据生成定时器回调 - 每1秒生成一个数据点
 */
static void dmm_sim_data_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    if (dmm_ui_ptr == NULL) return;
    if (dmm_state.is_hold) return;  // 保持模式不更新

    // 根据当前模式生成不同范围的随机数据
    float sim_value = 0.0f;
    uint32_t seed = (uint32_t)(sim_data_index * 1103515245 + 12345);  // 伪随机种子

    switch (dmm_state.mode) {
        case DMM_MODE_VOLTAGE:
            // 电压: 0~36V，正弦波+噪声
            sim_value = 18.0f + 15.0f * sinf(sim_data_index * 0.15f) + ((seed % 100) - 50) * 0.05f;
            if (sim_value < 0) sim_value = 0;
            if (sim_value > 36.0f) sim_value = 36.0f;
            break;
        case DMM_MODE_CURRENT:
            // 电流: 0~2A，正弦波+噪声
            sim_value = 1.0f + 0.8f * sinf(sim_data_index * 0.12f) + ((seed % 100) - 50) * 0.005f;
            if (sim_value < 0) sim_value = 0;
            if (sim_value > 2.0f) sim_value = 2.0f;
            break;
        case DMM_MODE_RESISTANCE:
            // 电阻: 根据档位生成
            {
                float max_r = get_range_max_value(DMM_MODE_RESISTANCE, dmm_state.res_range);
                sim_value = max_r * 0.5f + max_r * 0.4f * sinf(sim_data_index * 0.1f) + ((seed % 100) - 50) * max_r * 0.002f;
                if (sim_value < 0) sim_value = 0;
                if (sim_value > max_r) sim_value = max_r;
            }
            break;
    }

    // 更新直方图统计(将值映射到0-9的区间)
    float max_val = get_range_max_value(dmm_state.mode, dmm_state.res_range);
    int bin_index = (int)((sim_value / max_val) * 9.99f);
    if (bin_index < 0) bin_index = 0;
    if (bin_index > 9) bin_index = 9;
    histogram_bins[bin_index]++;

    // 调用公共API更新显示和统计
    dmm_update_measurement(sim_value);

    // 如果当前是直方图模式，实时更新直方图显示
    if (dmm_state.chart_mode == CHART_MODE_HISTOGRAM) {
        update_histogram_display();
    }

    sim_data_index++;
    if (sim_data_index >= 32) {
        sim_data_index = 0;  // 循环(32个点)
        // 不重置直方图统计，让它持续累积
    }
}

/**
 * PUBLIC API: 手动重置统计数据
 * 
 * 功能说明：
 * 清零所有统计值（MAX/MIN/AVG/RMS），重新开始统计
 * 适用场景：切换测量对象、重新开始测量等
 * 
 * ESP32使用示例:
 *   dmm_reset_statistics_data();
 */
void dmm_reset_statistics_data(void)
{
    reset_statistics();
    update_statistics_display();
}

/*******************************************************************************
 * setup_scr_scrPrintMenu - 专业数字万用表界面 (全新浅色主题设计)
 * Professional Digital Multimeter Interface - Complete Light Theme Redesign
 *
 * 全新布局结构 (800x480) - 完全重新设计:
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │ [←]                                                           [7.2M S/s]  │  <- 极简顶栏 (H:50)
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                             │
 * │   ┌─────────────────────────────────────────────────────────────────────┐   │
 * │   │                         VOLTAGE                                     │   │
 * │   │                                                                     │   │
 * │   │              +  12.3456  V                                          │   │  <- 主显示区 (H:180)
 * │   │                                                                     │   │
 * │   │   [CONNECTED]              [±36V]              [→ STABLE]           │   │
 * │   └─────────────────────────────────────────────────────────────────────┘   │
 * │                                                                             │
 * ├───────────────────┬─────────────────────────────────┬───────────────────────┤
 * │   STATISTICS      │        WAVEFORM / HISTOGRAM     │     MODE SELECT       │
 * │  ┌─────┬─────┐    │   ┌─────────────────────────┐   │   ┌─────────────────┐ │
 * │  │ MAX │ MIN │    │   │ ~~~~~~~~~~~~~~~~~~~~~~~~│   │   │   [V] VOLT      │ │  <- 下半区 (H:200)
 * │  ├─────┼─────┤    │   │ ~~~~~~~~~~~~~~~~~~~~~~~~│   │   │   [A] CURRENT   │ │
 * │  │ AVG │ σ   │    │   └─────────────────────────┘   │   │   [Ω] RESIST    │ │
 * │  └─────┴─────┘    │                                 │   │   [Δ] RANGE     │ │
 * │  [→ TREND]        │   Δ: +0.0012V  (0.01%)          │   │   [H] HOLD      │ │
 * │  [REC: 00:00]     │                                 │   └─────────────────┘ │
 * └───────────────────┴─────────────────────────────────┴───────────────────────┘
 ******************************************************************************/
void setup_scr_scrPrintMenu(lv_ui *ui)
{
	//==========================================================================
	// ██████╗  █████╗ ██████╗ ████████╗     ██╗
	// ██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝    ███║
	// ██████╔╝███████║██████╔╝   ██║       ╚██║   屏幕初始化与背景
	// ██╔═══╝ ██╔══██║██╔══██╗   ██║        ██║
	// ██║     ██║  ██║██║  ██║   ██║        ██║
	// ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝        ╚═╝
	//==========================================================================
	ui->scrPrintMenu = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrPrintMenu, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrPrintMenu, LV_SCROLLBAR_MODE_OFF);

	// 纯净浅色背景 - 极简专业风格
	lv_obj_set_style_bg_opa(ui->scrPrintMenu, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPrintMenu, lv_color_hex(0xF0F4F8), LV_PART_MAIN|LV_STATE_DEFAULT);

	// 隐藏元素 - 保持UI结构兼容性
	ui->scrPrintMenu_contBG = lv_obj_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_contBG, LV_OBJ_FLAG_HIDDEN);
	ui->scrPrintMenu_labelTitle = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_labelTitle, LV_OBJ_FLAG_HIDDEN);
	ui->scrPrintMenu_labelPrompt = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_labelPrompt, LV_OBJ_FLAG_HIDDEN);
	ui->scrPrintMenu_labelnternet = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_labelnternet, LV_OBJ_FLAG_HIDDEN);
	ui->scrPrintMenu_labelMobile = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_labelMobile, LV_OBJ_FLAG_HIDDEN);

	//==========================================================================
	// 顶部状态栏 - 白色卡片包裹每个信息 (整体下移3像素)
	//==========================================================================

	// ========== 顶部状态栏卡片样式宏定义 ==========
	#define TOP_BADGE_H 38
	#define TOP_BADGE_Y 11
	#define TOP_BADGE_RADIUS 10
	#define TOP_BADGE_GAP 5

	// 返回按钮 - 左侧
	ui->scrPrintMenu_btnBack = lv_btn_create(ui->scrPrintMenu);
	ui->scrPrintMenu_btnBack_label = lv_label_create(ui->scrPrintMenu_btnBack);
	lv_label_set_text(ui->scrPrintMenu_btnBack_label, LV_SYMBOL_LEFT);
	lv_obj_center(ui->scrPrintMenu_btnBack_label);
	lv_obj_set_pos(ui->scrPrintMenu_btnBack, 15, TOP_BADGE_Y);
	lv_obj_set_size(ui->scrPrintMenu_btnBack, 38, TOP_BADGE_H);
	lv_obj_set_style_bg_color(ui->scrPrintMenu_btnBack, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPrintMenu_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPrintMenu_btnBack, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPrintMenu_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPrintMenu_btnBack, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPrintMenu_btnBack, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPrintMenu_btnBack, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPrintMenu_btnBack_label, lv_color_hex(0x475569), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPrintMenu_btnBack_label, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPrintMenu_btnBack, lv_color_hex(COLOR_PRIMARY), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->scrPrintMenu_btnBack_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_PRESSED);

	// 模式卡片 - VOLTAGE/CURRENT/RESISTANCE
	lv_obj_t * mode_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(mode_card, 58, TOP_BADGE_Y);
	lv_obj_set_size(mode_card, 115, TOP_BADGE_H);
	lv_obj_set_style_bg_color(mode_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(mode_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(mode_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(mode_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(mode_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(mode_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(mode_card, LV_SCROLLBAR_MODE_OFF);
	dmm_state.mode_label = lv_label_create(mode_card);
	lv_label_set_text(dmm_state.mode_label, "VOLTAGE");
	lv_obj_center(dmm_state.mode_label);
	lv_obj_set_style_text_color(dmm_state.mode_label, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.mode_label, &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 连接状态卡片
	lv_obj_t * conn_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(conn_card, 178, TOP_BADGE_Y);
	lv_obj_set_size(conn_card, 75, TOP_BADGE_H);
	lv_obj_set_style_bg_color(conn_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(conn_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(conn_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(conn_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(conn_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(conn_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(conn_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(conn_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(conn_card, LV_SCROLLBAR_MODE_OFF);
	dmm_state.status_label = lv_label_create(conn_card);
	lv_label_set_text(dmm_state.status_label, LV_SYMBOL_OK " OK");
	lv_obj_center(dmm_state.status_label);
	lv_obj_set_style_text_color(dmm_state.status_label, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.status_label, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 量程卡片
	lv_obj_t * range_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(range_card, 258, TOP_BADGE_Y);
	lv_obj_set_size(range_card, 72, TOP_BADGE_H);
	lv_obj_set_style_bg_color(range_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(range_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(range_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(range_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(range_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(range_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(range_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(range_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(range_card, LV_SCROLLBAR_MODE_OFF);
	dmm_state.range_label = lv_label_create(range_card);
	lv_label_set_text(dmm_state.range_label, "36V");
	lv_obj_center(dmm_state.range_label);
	lv_obj_set_style_text_color(dmm_state.range_label, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.range_label, &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 采样率卡片
	lv_obj_t * sample_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(sample_card, 335, TOP_BADGE_Y);
	lv_obj_set_size(sample_card, 95, TOP_BADGE_H);
	lv_obj_set_style_bg_color(sample_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(sample_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(sample_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(sample_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(sample_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(sample_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(sample_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(sample_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(sample_card, LV_SCROLLBAR_MODE_OFF);
	lv_obj_t * sample_lbl = lv_label_create(sample_card);
	lv_label_set_text(sample_lbl, "7.2M S/s");
	lv_obj_center(sample_lbl);
	lv_obj_set_style_text_color(sample_lbl, lv_color_hex(0x475569), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(sample_lbl, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 数据质量卡片 (由STM32串口发送)
	lv_obj_t * quality_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(quality_card, 435, TOP_BADGE_Y);
	lv_obj_set_size(quality_card, 100, TOP_BADGE_H);
	lv_obj_set_style_bg_color(quality_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(quality_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(quality_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(quality_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(quality_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(quality_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(quality_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(quality_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(quality_card, LV_SCROLLBAR_MODE_OFF);
	dmm_state.quality_label = lv_label_create(quality_card);
	lv_label_set_text(dmm_state.quality_label, "SNR:42dB");  // 信噪比量化指标
	lv_obj_center(dmm_state.quality_label);
	lv_obj_set_style_text_color(dmm_state.quality_label, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.quality_label, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 波特率/通信速率卡片
	lv_obj_t * baud_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(baud_card, 540, TOP_BADGE_Y);
	lv_obj_set_size(baud_card, 85, TOP_BADGE_H);
	lv_obj_set_style_bg_color(baud_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(baud_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(baud_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(baud_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(baud_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(baud_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(baud_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(baud_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(baud_card, LV_SCROLLBAR_MODE_OFF);
	dmm_state.baud_label = lv_label_create(baud_card);
	lv_label_set_text(dmm_state.baud_label, "115200");
	lv_obj_center(dmm_state.baud_label);
	lv_obj_set_style_text_color(dmm_state.baud_label, lv_color_hex(0x64748B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.baud_label, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// HOLD状态卡片 - 与右侧控制面板右边对齐(785)
	lv_obj_t * hold_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(hold_card, 630, TOP_BADGE_Y);
	lv_obj_set_size(hold_card, 155, TOP_BADGE_H);
	lv_obj_set_style_bg_color(hold_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(hold_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(hold_card, TOP_BADGE_RADIUS, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(hold_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(hold_card, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(hold_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(hold_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(hold_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(hold_card, LV_SCROLLBAR_MODE_OFF);
	// HOLD状态图标
	dmm_state.hold_status_icon = lv_img_create(hold_card);
	lv_img_set_src(dmm_state.hold_status_icon, &_running_alpha_20x20);
	lv_obj_set_pos(dmm_state.hold_status_icon, 15, 9);
	lv_obj_set_style_img_recolor(dmm_state.hold_status_icon, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(dmm_state.hold_status_icon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	// HOLD状态文字 - 使用居中对齐，下移3像素
	dmm_state.hold_status_label = lv_label_create(hold_card);
	lv_label_set_text(dmm_state.hold_status_label, "RUNNING");
	lv_obj_set_width(dmm_state.hold_status_label, 115);  // 155(卡片宽) - 15(左图标) - 20(边距) - 5(间距)
	lv_obj_set_pos(dmm_state.hold_status_label, 38, 13);  // 下移3像素: 10 -> 13
	lv_obj_set_style_text_align(dmm_state.hold_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(dmm_state.hold_status_label, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.hold_status_label, &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	//==========================================================================
	// 核心测量显示区 - 主卡片
	//==========================================================================
	ui->scrPrintMenu_contUSB = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(ui->scrPrintMenu_contUSB, 15, 60);
	lv_obj_set_size(ui->scrPrintMenu_contUSB, 680, 180);
	lv_obj_set_scrollbar_mode(ui->scrPrintMenu_contUSB, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrPrintMenu_contUSB, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui->scrPrintMenu_contUSB, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPrintMenu_contUSB, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPrintMenu_contUSB, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPrintMenu_contUSB, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPrintMenu_contUSB, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPrintMenu_contUSB, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPrintMenu_contUSB, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrPrintMenu_contUSB, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrPrintMenu_contUSB, 15, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 隐藏旧的兼容性元素
	ui->scrPrintMenu_contInternet = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_contInternet, LV_OBJ_FLAG_HIDDEN);
	ui->scrPrintMenu_contMobile = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(ui->scrPrintMenu_contMobile, LV_OBJ_FLAG_HIDDEN);

	// ===== 极性指示 - 显示+/-符号 =====
	dmm_state.polarity_label = lv_label_create(ui->scrPrintMenu_contUSB);
	lv_label_set_text(dmm_state.polarity_label, "+");  // 默认正极性
	lv_obj_set_pos(dmm_state.polarity_label, 35, 55);  // 数值左侧
	lv_obj_set_style_text_color(dmm_state.polarity_label, lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.polarity_label, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);

	// ===== 主数值显示 - 使用Collins2_82字体，右移15 =====
	ui->scrPrintMenu_labelUSB = lv_label_create(ui->scrPrintMenu_contUSB);
	lv_label_set_text(ui->scrPrintMenu_labelUSB, "0.0000");
	lv_label_set_long_mode(ui->scrPrintMenu_labelUSB, LV_LABEL_LONG_CLIP);
	lv_obj_set_size(ui->scrPrintMenu_labelUSB, 450, 90);  // 高度适应82号字体
	lv_obj_set_pos(ui->scrPrintMenu_labelUSB, 85, 45);  // 右移15(70->85)
	lv_obj_set_style_text_color(ui->scrPrintMenu_labelUSB, lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPrintMenu_labelUSB, &lv_font_Collins2_82, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrPrintMenu_labelUSB, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrPrintMenu_labelUSB, 3, LV_PART_MAIN|LV_STATE_DEFAULT);

	// ===== 单位显示 - 右侧，使用黑色，右移15 =====
	dmm_state.y_axis_unit = lv_label_create(ui->scrPrintMenu_contUSB);
	lv_label_set_text(dmm_state.y_axis_unit, "V");
	lv_obj_set_pos(dmm_state.y_axis_unit, 560, 55);  // 右移15(545->560)
	lv_obj_set_style_text_color(dmm_state.y_axis_unit, lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);  // 黑色
	lv_obj_set_style_text_font(dmm_state.y_axis_unit, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);

	// ===== 趋势箭头(图标)和百分比 - 数字右上方显示(位于0.0000右上角) =====
	dmm_state.trend_arrow_img = lv_img_create(ui->scrPrintMenu_contUSB);
	lv_img_set_src(dmm_state.trend_arrow_img, &_up_alpha_20x20);  // 默认使用up图标
	lv_obj_set_pos(dmm_state.trend_arrow_img, 415, 20);  // 上移15(35->20)，右移15(400->415)
	lv_obj_set_style_img_recolor(dmm_state.trend_arrow_img, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(dmm_state.trend_arrow_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	// 初始显示，不隐藏

	dmm_state.trend_label = lv_label_create(ui->scrPrintMenu_contUSB);
	lv_label_set_text(dmm_state.trend_label, "00.00%");  // 初始显示00.00%
	lv_obj_set_pos(dmm_state.trend_label, 437, 20);  // 上移15(35->20)，右移15(422->437)
	lv_obj_set_style_text_color(dmm_state.trend_label, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.trend_label, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);  // 增大字体

	// 录制指示器（隐藏）
	dmm_state.recording_indicator = lv_label_create(ui->scrPrintMenu_contUSB);
	lv_label_set_text(dmm_state.recording_indicator, "REC");
	lv_obj_set_pos(dmm_state.recording_indicator, 620, 15);
	lv_obj_set_style_text_color(dmm_state.recording_indicator, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.recording_indicator, &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(dmm_state.recording_indicator, LV_OBJ_FLAG_HIDDEN);

	// ===== 过载警告(隐藏) - 使用带背景的按钮样式容器 =====
	dmm_state.overload_indicator = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(dmm_state.overload_indicator, 25, 70);
	lv_obj_set_size(dmm_state.overload_indicator, 60, 28);
	lv_obj_set_style_bg_color(dmm_state.overload_indicator, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(dmm_state.overload_indicator, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(dmm_state.overload_indicator, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(dmm_state.overload_indicator, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(dmm_state.overload_indicator, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_clear_flag(dmm_state.overload_indicator, LV_OBJ_FLAG_SCROLLABLE);
	// OL文字
	lv_obj_t * ol_label = lv_label_create(dmm_state.overload_indicator);
	lv_label_set_text(ol_label, "OL");
	lv_obj_center(ol_label);
	lv_obj_set_style_text_color(ol_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ol_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(dmm_state.overload_indicator, LV_OBJ_FLAG_HIDDEN);

	// 隐藏的兼容性对象
	dmm_state.connection_indicator = lv_label_create(ui->scrPrintMenu);
	lv_obj_add_flag(dmm_state.connection_indicator, LV_OBJ_FLAG_HIDDEN);

	//==========================================================================
	// 右侧模式选择卡片 - 上半部分(VOLT/AMP/OHM)
	//==========================================================================
	lv_obj_t * mode_panel = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(mode_panel, 700, 60);
	lv_obj_set_size(mode_panel, 90, 260);
	lv_obj_set_style_bg_color(mode_panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(mode_panel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(mode_panel, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(mode_panel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(mode_panel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(mode_panel, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(mode_panel, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(mode_panel, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(mode_panel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(mode_panel, LV_OBJ_FLAG_SCROLLABLE);

	// 按钮尺寸
	#define MODE_BTN_W 74
	#define MODE_BTN_H 75
	#define MODE_BTN_GAP 4

	// ===== VOLT 电压按钮 - 简洁样式 =====
	lv_obj_t * btn_voltage = lv_btn_create(mode_panel);
	lv_obj_set_pos(btn_voltage, 0, 0);
	lv_obj_set_size(btn_voltage, MODE_BTN_W, MODE_BTN_H);
	lv_obj_add_flag(btn_voltage, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_style_radius(btn_voltage, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_voltage, lv_color_hex(0xFFFBEB), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅琥珀色背景
	lv_obj_set_style_bg_opa(btn_voltage, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(btn_voltage, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_voltage, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_voltage, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_CHECKED);
	// 使用图片图标
	ui->scrPrintMenu_imgInternet = lv_img_create(btn_voltage);
	lv_img_set_src(ui->scrPrintMenu_imgInternet, &_f_alpha_24x24);
	lv_obj_align(ui->scrPrintMenu_imgInternet, LV_ALIGN_CENTER, 0, -10);
	lv_obj_set_style_img_recolor(ui->scrPrintMenu_imgInternet, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrPrintMenu_imgInternet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	volt_icon_ptr = ui->scrPrintMenu_imgInternet;  // 保存图标指针
	lv_obj_t * lbl_volt = lv_label_create(btn_voltage);
	lv_label_set_text(lbl_volt, "VOLT");
	lv_obj_align(lbl_volt, LV_ALIGN_CENTER, 0, 20);
	lv_obj_set_style_text_font(lbl_volt, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(lbl_volt, lv_color_hex(COLOR_VOLTAGE), LV_PART_MAIN|LV_STATE_DEFAULT);  // 琥珀色文字
	volt_label_ptr = lbl_volt;  // 保存标签指针

	// ===== AMP 电流按钮 =====
	lv_obj_t * btn_current = lv_btn_create(mode_panel);
	lv_obj_set_pos(btn_current, 0, MODE_BTN_H + MODE_BTN_GAP);
	lv_obj_set_size(btn_current, MODE_BTN_W, MODE_BTN_H);
	lv_obj_add_flag(btn_current, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_style_radius(btn_current, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_current, lv_color_hex(0xFEE2E2), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅红色背景
	lv_obj_set_style_bg_opa(btn_current, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(btn_current, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_current, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_current, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_CHECKED);
	ui->scrPrintMenu_imgMobile = lv_img_create(btn_current);
	lv_img_set_src(ui->scrPrintMenu_imgMobile, &_e_alpha_24x24);
	lv_obj_align(ui->scrPrintMenu_imgMobile, LV_ALIGN_CENTER, 0, -10);
	lv_obj_set_style_img_recolor(ui->scrPrintMenu_imgMobile, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrPrintMenu_imgMobile, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	amp_icon_ptr = ui->scrPrintMenu_imgMobile;  // 保存图标指针
	lv_obj_t * lbl_amp = lv_label_create(btn_current);
	lv_label_set_text(lbl_amp, "AMP");
	lv_obj_align(lbl_amp, LV_ALIGN_CENTER, 0, 20);
	lv_obj_set_style_text_font(lbl_amp, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(lbl_amp, lv_color_hex(COLOR_CURRENT), LV_PART_MAIN|LV_STATE_DEFAULT);  // 红色文字
	amp_label_ptr = lbl_amp;  // 保存标签指针

	// ===== OHM 电阻按钮 =====
	lv_obj_t * btn_resistance = lv_btn_create(mode_panel);
	lv_obj_set_pos(btn_resistance, 0, 2*(MODE_BTN_H + MODE_BTN_GAP));
	lv_obj_set_size(btn_resistance, MODE_BTN_W, MODE_BTN_H);
	lv_obj_add_flag(btn_resistance, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_style_radius(btn_resistance, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_resistance, lv_color_hex(0xDBEAFE), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅蓝色背景
	lv_obj_set_style_bg_opa(btn_resistance, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(btn_resistance, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_resistance, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_resistance, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_t * ohm_img = lv_img_create(btn_resistance);
	lv_img_set_src(ohm_img, &_g_alpha_24x24);
	lv_obj_align(ohm_img, LV_ALIGN_CENTER, 0, -10);
	lv_obj_set_style_img_recolor(ohm_img, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ohm_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	ohm_icon_ptr = ohm_img;  // 保存图标指针
	lv_obj_t * lbl_ohm = lv_label_create(btn_resistance);
	lv_label_set_text(lbl_ohm, "OHM");
	lv_obj_align(lbl_ohm, LV_ALIGN_CENTER, 0, 20);
	lv_obj_set_style_text_font(lbl_ohm, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(lbl_ohm, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);  // 蓝色文字
	ohm_label_ptr = lbl_ohm;  // 保存标签指针

	//==========================================================================
	// 右侧控制卡片 - 下半部分(HOLD) - 底部与波形卡片对齐(460)
	//==========================================================================
	lv_obj_t * ctrl_panel = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(ctrl_panel, 700, 335);
	lv_obj_set_size(ctrl_panel, 90, 125);  // 335 + 125 = 460
	lv_obj_set_style_bg_color(ctrl_panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ctrl_panel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ctrl_panel, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ctrl_panel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ctrl_panel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ctrl_panel, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ctrl_panel, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ctrl_panel, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(ctrl_panel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ctrl_panel, LV_OBJ_FLAG_SCROLLABLE);

	// ===== HOLD 保持按钮 - 清爽样式 =====
	lv_obj_t * btn_hold = lv_btn_create(ctrl_panel);
	lv_obj_set_pos(btn_hold, 0, 0);
	lv_obj_set_size(btn_hold, MODE_BTN_W, 105);  // 调整高度适应卡片
	lv_obj_add_flag(btn_hold, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_style_radius(btn_hold, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_hold, lv_color_hex(0xD1FAE5), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅绿色背景
	lv_obj_set_style_bg_opa(btn_hold, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(btn_hold, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_hold, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_hold, lv_color_hex(0xFEE2E2), LV_PART_MAIN|LV_STATE_CHECKED);  // 浅红色背景(暂停)
	lv_obj_t * hold_img = lv_img_create(btn_hold);
	lv_img_set_src(hold_img, &_a_alpha_24x24);
	lv_obj_align(hold_img, LV_ALIGN_CENTER, 0, -20);
	lv_obj_set_style_img_recolor(hold_img, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(hold_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	hold_icon_ptr = hold_img;
	lv_obj_t * lbl_hold = lv_label_create(btn_hold);
	lv_label_set_text(lbl_hold, "RUN");  // 默认显示RUN
	lv_obj_align(lbl_hold, LV_ALIGN_CENTER, 0, 25);
	lv_obj_set_style_text_font(lbl_hold, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(lbl_hold, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);  // 绿色文字
	lv_obj_set_style_text_color(lbl_hold, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_CHECKED);  // 红色文字(暂停)
	hold_label_ptr = lbl_hold;  // 保存标签指针

	// 为兼容性创建隐藏的btn_range变量
	lv_obj_t * btn_range = btn_resistance;

	//==========================================================================
	// 下半区 - 统计(居中) + 波形图
	//==========================================================================

	// 左侧统计卡片 - 与波形图高度一致
	ui->scrPrintMenu_contMain = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(ui->scrPrintMenu_contMain, 15, 250);
	lv_obj_set_size(ui->scrPrintMenu_contMain, 200, 210);
	lv_obj_set_scrollbar_mode(ui->scrPrintMenu_contMain, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrPrintMenu_contMain, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui->scrPrintMenu_contMain, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPrintMenu_contMain, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPrintMenu_contMain, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPrintMenu_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPrintMenu_contMain, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPrintMenu_contMain, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPrintMenu_contMain, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrPrintMenu_contMain, 8, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 统计标题 - 居中
	lv_obj_t * stat_title = lv_label_create(ui->scrPrintMenu_contMain);
	lv_label_set_text(stat_title, "STATISTICS");
	lv_obj_align(stat_title, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_text_color(stat_title, lv_color_hex(0x64748B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(stat_title, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// ===== 统计面板2x2布局 - 无单位显示 =====
	#define STAT_CARD_W 88
	#define STAT_CARD_H 85
	#define STAT_GAP 6
	#define STAT_LEFT_OFFSET 1

	// MAX 统计框 - 左上
	lv_obj_t * max_card = lv_obj_create(ui->scrPrintMenu_contMain);
	lv_obj_set_pos(max_card, STAT_LEFT_OFFSET, 20);
	lv_obj_set_size(max_card, STAT_CARD_W, STAT_CARD_H);
	lv_obj_set_style_bg_color(max_card, lv_color_hex(0xFEE2E2), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(max_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(max_card, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(max_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(max_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(max_card, LV_SCROLLBAR_MODE_OFF);
	// MAX图标 - 使用min图标(交换)
	dmm_state.stat_icons[0] = lv_img_create(max_card);
	lv_img_set_src(dmm_state.stat_icons[0], &_min_alpha_20x20);
	lv_obj_set_pos(dmm_state.stat_icons[0], 15, 8);
	lv_obj_set_style_img_recolor(dmm_state.stat_icons[0], lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(dmm_state.stat_icons[0], 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * max_lbl = lv_label_create(max_card);
	lv_label_set_text(max_lbl, "MAX");
	lv_obj_set_pos(max_lbl, 38, 10);
	lv_obj_set_style_text_color(max_lbl, lv_color_hex(COLOR_DANGER), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(max_lbl, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	dmm_state.stat_labels[0] = lv_label_create(max_card);
	lv_label_set_text(dmm_state.stat_labels[0], "0.0000");
	lv_obj_set_pos(dmm_state.stat_labels[0], 0, 45);
	lv_obj_set_width(dmm_state.stat_labels[0], STAT_CARD_W);
	lv_obj_set_style_text_align(dmm_state.stat_labels[0], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(dmm_state.stat_labels[0], lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.stat_labels[0], &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	// MIN 统计框 - 右上
	lv_obj_t * min_card = lv_obj_create(ui->scrPrintMenu_contMain);
	lv_obj_set_pos(min_card, STAT_LEFT_OFFSET + STAT_CARD_W + STAT_GAP, 20);
	lv_obj_set_size(min_card, STAT_CARD_W, STAT_CARD_H);
	lv_obj_set_style_bg_color(min_card, lv_color_hex(0xDBEAFE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(min_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(min_card, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(min_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(min_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(min_card, LV_SCROLLBAR_MODE_OFF);
	// MIN图标 - 使用max图标(交换)
	dmm_state.stat_icons[1] = lv_img_create(min_card);
	lv_img_set_src(dmm_state.stat_icons[1], &_max_alpha_20x20);
	lv_obj_set_pos(dmm_state.stat_icons[1], 15, 8);
	lv_obj_set_style_img_recolor(dmm_state.stat_icons[1], lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(dmm_state.stat_icons[1], 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * min_lbl = lv_label_create(min_card);
	lv_label_set_text(min_lbl, "MIN");
	lv_obj_set_pos(min_lbl, 38, 10);
	lv_obj_set_style_text_color(min_lbl, lv_color_hex(COLOR_RESISTANCE), LV_PART_MAIN|LV_STATE_DEFAULT);  // 蓝色
	lv_obj_set_style_text_font(min_lbl, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	dmm_state.stat_labels[1] = lv_label_create(min_card);
	lv_label_set_text(dmm_state.stat_labels[1], "0.0000");
	lv_obj_set_pos(dmm_state.stat_labels[1], 0, 45);
	lv_obj_set_width(dmm_state.stat_labels[1], STAT_CARD_W);
	lv_obj_set_style_text_align(dmm_state.stat_labels[1], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(dmm_state.stat_labels[1], lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.stat_labels[1], &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	// AVG 统计框 - 左下
	lv_obj_t * avg_card = lv_obj_create(ui->scrPrintMenu_contMain);
	lv_obj_set_pos(avg_card, STAT_LEFT_OFFSET, 20 + STAT_CARD_H + STAT_GAP);
	lv_obj_set_size(avg_card, STAT_CARD_W, STAT_CARD_H);
	lv_obj_set_style_bg_color(avg_card, lv_color_hex(0xFEF3C7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(avg_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(avg_card, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(avg_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(avg_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(avg_card, LV_SCROLLBAR_MODE_OFF);
	// AVG图标
	dmm_state.stat_icons[2] = lv_img_create(avg_card);
	lv_img_set_src(dmm_state.stat_icons[2], &_avg_alpha_20x20);
	lv_obj_set_pos(dmm_state.stat_icons[2], 15, 8);
	lv_obj_set_style_img_recolor(dmm_state.stat_icons[2], lv_color_hex(0xD97706), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(dmm_state.stat_icons[2], 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * avg_lbl = lv_label_create(avg_card);
	lv_label_set_text(avg_lbl, "AVG");
	lv_obj_set_pos(avg_lbl, 38, 10);
	lv_obj_set_style_text_color(avg_lbl, lv_color_hex(0xD97706), LV_PART_MAIN|LV_STATE_DEFAULT);  // 深琥珀色
	lv_obj_set_style_text_font(avg_lbl, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	dmm_state.stat_labels[2] = lv_label_create(avg_card);
	lv_label_set_text(dmm_state.stat_labels[2], "0.0000");
	lv_obj_set_pos(dmm_state.stat_labels[2], 0, 45);
	lv_obj_set_width(dmm_state.stat_labels[2], STAT_CARD_W);
	lv_obj_set_style_text_align(dmm_state.stat_labels[2], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(dmm_state.stat_labels[2], lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.stat_labels[2], &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	// STD 统计框 - 右下
	lv_obj_t * std_card = lv_obj_create(ui->scrPrintMenu_contMain);
	lv_obj_set_pos(std_card, STAT_LEFT_OFFSET + STAT_CARD_W + STAT_GAP, 20 + STAT_CARD_H + STAT_GAP);
	lv_obj_set_size(std_card, STAT_CARD_W, STAT_CARD_H);
	lv_obj_set_style_bg_color(std_card, lv_color_hex(0xD1FAE5), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(std_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(std_card, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(std_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(std_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(std_card, LV_SCROLLBAR_MODE_OFF);
	// STD图标 (30x30，上移2像素)
	dmm_state.stat_icons[3] = lv_img_create(std_card);
	lv_img_set_src(dmm_state.stat_icons[3], &_std_alpha_30x30);
	lv_obj_set_pos(dmm_state.stat_icons[3], 10, 1);
	lv_obj_set_style_img_recolor(dmm_state.stat_icons[3], lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(dmm_state.stat_icons[3], 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * std_lbl = lv_label_create(std_card);
	lv_label_set_text(std_lbl, "STD");
	lv_obj_set_pos(std_lbl, 43, 10);
	lv_obj_set_style_text_color(std_lbl, lv_color_hex(COLOR_SUCCESS), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(std_lbl, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	dmm_state.stat_labels[3] = lv_label_create(std_card);
	lv_label_set_text(dmm_state.stat_labels[3], "0.0000");
	lv_obj_set_pos(dmm_state.stat_labels[3], 0, 45);
	lv_obj_set_width(dmm_state.stat_labels[3], STAT_CARD_W);
	lv_obj_set_style_text_align(dmm_state.stat_labels[3], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(dmm_state.stat_labels[3], lv_color_hex(0x1E293B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.stat_labels[3], &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 中间波形图卡片 - 浅色高对比度设计
	lv_obj_t * wave_card = lv_obj_create(ui->scrPrintMenu);
	lv_obj_set_pos(wave_card, 220, 250);
	lv_obj_set_size(wave_card, 475, 210);
	lv_obj_set_style_bg_color(wave_card, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(wave_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(wave_card, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(wave_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(wave_card, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(wave_card, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(wave_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(wave_card, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(wave_card, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(wave_card, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(wave_card, LV_OBJ_FLAG_SCROLLABLE);

	// 波形标题和图表模式切换按钮
	lv_obj_t * wave_title = lv_label_create(wave_card);
	lv_label_set_text(wave_title, "WAVEFORM");
	lv_obj_set_pos(wave_title, 5, 0);
	lv_obj_set_style_text_color(wave_title, lv_color_hex(0x64748B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(wave_title, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(wave_title, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 图表模式切换按钮组
	lv_obj_t * chart_mode_box = lv_obj_create(wave_card);
	lv_obj_set_pos(chart_mode_box, 320, -2);
	lv_obj_set_size(chart_mode_box, 135, 28);
	lv_obj_set_style_bg_color(chart_mode_box, lv_color_hex(0xE2E8F0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(chart_mode_box, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(chart_mode_box, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(chart_mode_box, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_scrollbar_mode(chart_mode_box, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_pad_all(chart_mode_box, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 波形按钮 - 使用图片图标，深黄色背景
	lv_obj_t * btn_wave = lv_btn_create(chart_mode_box);
	btn_chart_wave_ptr = btn_wave;
	lv_obj_set_pos(btn_wave, 0, 0);
	lv_obj_set_size(btn_wave, 42, 22);
	lv_obj_set_style_radius(btn_wave, 11, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_wave, lv_color_hex(0xD97706), LV_PART_MAIN|LV_STATE_DEFAULT);  // 深黄色
	lv_obj_set_style_bg_opa(btn_wave, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_wave, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * img_wave = lv_img_create(btn_wave);
	lv_img_set_src(img_wave, &_1_alpha_25x25);
	lv_obj_center(img_wave);
	lv_obj_set_style_img_recolor(img_wave, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(img_wave, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_event_cb(btn_wave, dmm_chart_mode_wave_cb, LV_EVENT_CLICKED, NULL);

	// 柱状图按钮 - 使用图片图标，深黄色背景(选中时)
	lv_obj_t * btn_bar_chart = lv_btn_create(chart_mode_box);
	btn_chart_bar_ptr = btn_bar_chart;
	lv_obj_set_pos(btn_bar_chart, 44, 0);
	lv_obj_set_size(btn_bar_chart, 42, 22);
	lv_obj_set_style_radius(btn_bar_chart, 11, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_bar_chart, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(btn_bar_chart, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_bar_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * img_bar_chart = lv_img_create(btn_bar_chart);
	lv_img_set_src(img_bar_chart, &_3_alpha_25x25);
	lv_obj_center(img_bar_chart);
	lv_obj_set_style_img_recolor(img_bar_chart, lv_color_hex(0x64748B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(img_bar_chart, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_event_cb(btn_bar_chart, dmm_chart_mode_bar_cb, LV_EVENT_CLICKED, NULL);

	// 直方图按钮 - 使用图片图标，深黄色背景(选中时)
	lv_obj_t * btn_hist = lv_btn_create(chart_mode_box);
	btn_chart_hist_ptr = btn_hist;
	lv_obj_set_pos(btn_hist, 88, 0);
	lv_obj_set_size(btn_hist, 42, 22);
	lv_obj_set_style_radius(btn_hist, 11, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_hist, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(btn_hist, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(btn_hist, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_t * img_hist = lv_img_create(btn_hist);
	lv_img_set_src(img_hist, &_2_alpha_25x25);
	lv_obj_center(img_hist);
	lv_obj_set_style_img_recolor(img_hist, lv_color_hex(0x64748B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(img_hist, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_event_cb(btn_hist, dmm_chart_mode_hist_cb, LV_EVENT_CLICKED, NULL);

	// Y轴刻度 - 与图表区域对齐（图表高度150，从y=30开始）
	// 最大值标签 - 图表顶部
	dmm_state.y_axis_labels[0] = lv_label_create(wave_card);
	lv_label_set_text(dmm_state.y_axis_labels[0], "36.0");
	lv_obj_set_pos(dmm_state.y_axis_labels[0], 0, 25);
	lv_obj_set_style_text_color(dmm_state.y_axis_labels[0], lv_color_hex(0x475569), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.y_axis_labels[0], &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 中间值标签 - 图表中部
	dmm_state.y_axis_labels[1] = lv_label_create(wave_card);
	lv_label_set_text(dmm_state.y_axis_labels[1], "18.0");
	lv_obj_set_pos(dmm_state.y_axis_labels[1], 0, 95);
	lv_obj_set_style_text_color(dmm_state.y_axis_labels[1], lv_color_hex(0x475569), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.y_axis_labels[1], &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 最小值标签 - 图表底部
	dmm_state.y_axis_labels[2] = lv_label_create(wave_card);
	lv_label_set_text(dmm_state.y_axis_labels[2], "0.0");
	lv_obj_set_pos(dmm_state.y_axis_labels[2], 0, 165);
	lv_obj_set_style_text_color(dmm_state.y_axis_labels[2], lv_color_hex(0x475569), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dmm_state.y_axis_labels[2], &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 波形图表 - 浅色高对比度背景
	ui->scrPrintMenu_imgUSB = lv_chart_create(wave_card);
	dmm_state.chart_obj = ui->scrPrintMenu_imgUSB;
	lv_obj_set_pos(ui->scrPrintMenu_imgUSB, 40, 30);
	lv_obj_set_size(ui->scrPrintMenu_imgUSB, 415, 155);
	lv_chart_set_type(ui->scrPrintMenu_imgUSB, LV_CHART_TYPE_LINE);
	lv_chart_set_range(ui->scrPrintMenu_imgUSB, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
	lv_chart_set_div_line_count(ui->scrPrintMenu_imgUSB, 4, 8);
	lv_chart_set_point_count(ui->scrPrintMenu_imgUSB, 32);  // 32个数据点
	lv_obj_set_style_width(ui->scrPrintMenu_imgUSB, 0, LV_PART_INDICATOR);
	lv_obj_set_style_height(ui->scrPrintMenu_imgUSB, 0, LV_PART_INDICATOR);
	// 浅色背景 - 高对比度
	lv_obj_set_style_bg_color(ui->scrPrintMenu_imgUSB, lv_color_hex(0xF1F5F9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPrintMenu_imgUSB, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPrintMenu_imgUSB, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrPrintMenu_imgUSB, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPrintMenu_imgUSB, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrPrintMenu_imgUSB, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	// 网格线颜色 - 浅灰色
	lv_obj_set_style_line_color(ui->scrPrintMenu_imgUSB, lv_color_hex(0xCBD5E1), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui->scrPrintMenu_imgUSB, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 添加趋势线 - 深蓝色
	trend_series = lv_chart_add_series(ui->scrPrintMenu_imgUSB, lv_color_hex(COLOR_VOLTAGE), LV_CHART_AXIS_PRIMARY_Y);
	lv_obj_set_style_line_width(ui->scrPrintMenu_imgUSB, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);

	// 初始化图表数据为0
	for (int i = 0; i < 32; i++) {
		lv_chart_set_next_value(ui->scrPrintMenu_imgUSB, trend_series, 0);
	}

	// 初始化统计数据(包括直方图)
	reset_statistics();

	// 启动模拟数据生成定时器 - 每1000ms(1秒)生成一个数据点
	dmm_sim_data_timer = lv_timer_create(dmm_sim_data_timer_cb, 1000, NULL);

	//==========================================================================
	// 保存按钮指针并设置初始选中状态
	//==========================================================================
	btn_voltage_ptr = btn_voltage;
	btn_current_ptr = btn_current;
	btn_resistance_ptr = btn_resistance;

	// 默认选中电压测量按钮
	lv_obj_add_state(btn_voltage, LV_STATE_CHECKED);
	// 设置选中按钮子控件为白色
	lv_obj_set_style_img_recolor(volt_icon_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(volt_label_ptr, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//==========================================================================
	// 事件回调绑定
	//==========================================================================
	lv_obj_add_event_cb(btn_voltage, dmm_voltage_btn_event_cb, LV_EVENT_CLICKED, ui);
	lv_obj_add_event_cb(btn_current, dmm_current_btn_event_cb, LV_EVENT_CLICKED, ui);
	lv_obj_add_event_cb(btn_resistance, dmm_resistance_btn_event_cb, LV_EVENT_CLICKED, ui);
	lv_obj_add_event_cb(btn_range, dmm_range_btn_event_cb, LV_EVENT_CLICKED, ui);
	lv_obj_add_event_cb(btn_hold, dmm_hold_btn_event_cb, LV_EVENT_CLICKED, ui);

	//==========================================================================
	// 初始化和定时器
	//==========================================================================
	dmm_ui_ptr = ui;

	if (dmm_time_timer == NULL) {
		dmm_time_timer = lv_timer_create(dmm_time_update_cb, 1000, NULL);
	}

	if (dmm_animation_timer == NULL) {
		dmm_animation_timer = lv_timer_create(dmm_animation_timer_cb, 50, NULL);
	}

	update_y_axis_labels();

	// 将overload指示器移到最上层，避免被其他元素遮挡
	if (dmm_state.overload_indicator && lv_obj_is_valid(dmm_state.overload_indicator)) {
		lv_obj_move_foreground(dmm_state.overload_indicator);
	}

	lv_obj_update_layout(ui->scrPrintMenu);
	events_init_scrPrintMenu(ui);
}
