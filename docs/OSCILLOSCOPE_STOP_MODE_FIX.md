# 示波器停止模式逻辑完善

## 问题描述

原来的示波器运行/停止按钮逻辑不符合真实示波器的行为：
- 停止时应该冻结整个波形窗口
- 横轴偏移时，只应显示已采集的波形数据
- 超出原始窗口的部分应该看不见
- 波形预览区域的遮罩应该相应变化

## 参考文档

参考了DS100 Mini数字示波器用户手册V4.3中的采样情况图标说明：

```
采样情况图标：
(1) 波浪线表示采样数据长度
(2) 蓝色覆盖的波浪线表示不在波形区域显示的数据（隐藏数据）
(3) T形图标表示触发位置
```

![采样情况示意图](https://www.alientek.com/docs/ds100_sampling_indicator.png)

## 实现方案

### 1. 数据结构改进

添加了停止时触发位置的记录：

```c
// Frozen waveform data for STOP mode - 完整采集的波形数据
static float osc_frozen_voltage_data[OSC_GRID_WIDTH];  // Store actual voltage values when frozen
static bool osc_frozen_data_valid = false;
static int osc_frozen_time_scale_index = 1;  // Time scale when frozen
static int osc_frozen_volt_scale_index = 7;  // Voltage scale when frozen
static float osc_frozen_x_offset_at_stop = 0.0f;  // X offset when stopped (trigger position)
```

### 2. 运行/停止按钮逻辑

修改了按钮事件处理，记录停止时的触发位置：

```c
static void scrOscilloscope_btnStartStop_event_handler (lv_event_t *e)
{
    osc_running = !osc_running;
    if (osc_running) {
        // 恢复运行
        lv_label_set_text(guider_ui.scrOscilloscope_btnStartStop_label, "RUN");
        lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnStartStop, lv_color_hex(0x00FF00), ...);
        // 清除冻结数据标记，重新开始采集
        osc_frozen_data_valid = false;
    } else {
        // 停止运行 - 冻结整个波形窗口
        lv_label_set_text(guider_ui.scrOscilloscope_btnStartStop_label, "STOP");
        lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnStartStop, lv_color_hex(0xFF0000), ...);
        // 记录停止时的触发位置（当前X偏移）
        osc_frozen_x_offset_at_stop = osc_x_offset;
        // 冻结数据已在波形更新回调中保存
    }
}
```

### 3. 波形显示逻辑

修改了停止模式下的波形显示逻辑，只显示已采集的数据：

```c
// 真实示波器行为：
// 1. 停止时冻结整个波形窗口（记录触发点位置）
// 2. 横轴偏移时，只显示已采集的波形数据
// 3. 超出原始窗口的部分不显示（显示为空白或零）
if (!osc_running && osc_frozen_data_valid) {
    // 计算相对偏移（相对于停止时的触发位置）
    float relative_x_offset = osc_x_offset - osc_frozen_x_offset_at_stop;
    int x_offset_pixels = (int)(relative_x_offset * pixels_per_second);
    
    for(int i = 0; i < num_points; i++) {
        // 计算源数据位置
        float source_pos = ((float)i - (float)num_points / 2.0f) * time_scale_ratio 
                         + (float)num_points / 2.0f - x_offset_pixels;
        int source_index = (int)source_pos;
        
        float voltage;
        // 真实示波器行为：超出原始采集窗口的数据不显示
        if (source_index < 0 || source_index >= num_points) {
            // 超出范围，显示为零
            voltage = 0.0f;
        } else {
            // 线性插值获取电压值
            voltage = osc_frozen_voltage_data[source_index];
        }
        
        // 应用Y偏移并转换为图表坐标
        voltage += osc_y_offset;
        float y_float = chart_center - (voltage * units_per_volt);
        ser->y_points[i] = (int)(y_float + 0.5f);
    }
    
    // 更新波形预览区域的遮罩
    update_preview_mask();
}
```

### 4. 波形预览遮罩更新

添加了`update_preview_mask()`函数来更新波形预览区域的蓝色遮罩：

```c
static void update_preview_mask(void)
{
    if (!osc_frozen_data_valid || osc_running) {
        // 运行模式：隐藏遮罩
        if (osc_preview_mask_left != NULL) {
            lv_obj_add_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
        }
        if (osc_preview_mask_right != NULL) {
            lv_obj_add_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    
    // 停止模式：根据X偏移更新遮罩位置
    float time_per_div = time_scale_values[osc_time_scale_index];
    float max_offset = time_per_div * (float)OSC_GRID_COLS;
    
    // 相对偏移（相对于停止时的触发位置）
    float relative_x_offset = osc_x_offset - osc_frozen_x_offset_at_stop;
    
    // 归一化偏移：-1.0（完全向左）到 +1.0（完全向右）
    float normalized_offset = relative_x_offset / max_offset;
    
    // 计算可见窗口在预览区域中的位置
    float visible_center = (float)preview_w / 2.0f - normalized_offset * (float)preview_w / 2.0f;
    float visible_width = (float)preview_w * 0.4f;
    float visible_left = visible_center - visible_width / 2.0f;
    float visible_right = visible_center + visible_width / 2.0f;
    
    // 更新左侧遮罩
    if (visible_left > 0) {
        lv_obj_clear_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(osc_preview_mask_left, 0, 0);
        lv_obj_set_size(osc_preview_mask_left, (lv_coord_t)visible_left, preview_h);
    } else {
        lv_obj_add_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 更新右侧遮罩
    if (visible_right < preview_w) {
        lv_obj_clear_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(osc_preview_mask_right, (lv_coord_t)visible_right, 0);
        lv_obj_set_size(osc_preview_mask_right, preview_w - (lv_coord_t)visible_right, preview_h);
    } else {
        lv_obj_add_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
    }
}
```

### 5. UI结构改进

在`gui_guider.h`中添加了遮罩对象的引用：

```c
lv_obj_t *scrOscilloscope_sliderWaveMask;  // Waveform position mask (blue overlay) - LEFT mask
lv_obj_t *scrOscilloscope_sliderWaveMaskRight;  // Waveform position mask (blue overlay) - RIGHT mask
lv_obj_t *scrOscilloscope_sliderWaveTrigger;  // Trigger position indicator (T-shaped marker)
```

在`setup_scr_scrOscilloscope.c`中保存遮罩对象的引用：

```c
ui->scrOscilloscope_sliderWaveMask = maskLeft;  // 保存左侧遮罩引用
ui->scrOscilloscope_sliderWaveMaskRight = maskRight;  // 保存右侧遮罩引用
ui->scrOscilloscope_sliderWaveTrigger = triggerLine;  // 保存触发位置标记引用
```

## 使用说明

### 停止模式操作

1. **停止采集**：点击"RUN"按钮，按钮变为红色"STOP"，波形冻结
2. **查看波形**：使用X-Pos控制调整横轴偏移，查看冻结波形的不同部分
3. **观察预览**：顶部波形预览区域的蓝色遮罩会随着偏移变化，显示当前可见窗口的位置
4. **恢复运行**：再次点击"STOP"按钮，按钮变为绿色"RUN"，恢复实时采集

### 波形预览区域说明

- **波浪线**：表示完整的采样数据长度
- **蓝色遮罩**：表示不在当前显示窗口的数据（隐藏数据）
- **T形图标**：表示触发位置（停止时的位置）
- **可见窗口**：未被遮罩覆盖的部分，对应主波形显示区域

## 技术细节

### 坐标系统

- **主波形区域**：688个数据点，对应16个水平格子
- **预览区域**：动态宽度（约200像素），显示完整采样数据的缩略图
- **遮罩位置**：根据X偏移和时间档位动态计算

### 偏移计算

```
相对偏移 = 当前X偏移 - 停止时X偏移
归一化偏移 = 相对偏移 / 最大偏移
可见窗口中心 = 预览区域中心 - 归一化偏移 * 预览区域半宽
```

### 数据范围限制

- 超出原始采集窗口的数据显示为零（或空白）
- X偏移范围限制为±10个屏幕宽度
- 遮罩位置限制在预览区域内

## 测试建议

1. **基本功能测试**
   - 运行模式下，波形应该正常滚动
   - 停止后，波形应该冻结
   - 调整X偏移，波形应该平移
   - 超出范围的部分应该显示为零

2. **预览遮罩测试**
   - 运行模式下，遮罩应该隐藏
   - 停止后，遮罩应该显示
   - 调整X偏移，遮罩应该相应移动
   - 遮罩位置应该正确反映可见窗口

3. **边界条件测试**
   - X偏移到最大值，右侧遮罩应该消失
   - X偏移到最小值，左侧遮罩应该消失
   - 切换时间档位，遮罩应该正确更新

## 相关文件

- `BSP/GUIDER/generated/events_init.c` - 事件处理和逻辑实现
- `BSP/GUIDER/generated/setup_scr_scrOscilloscope.c` - UI创建和初始化
- `BSP/GUIDER/generated/gui_guider.h` - UI结构定义
- `docs/OSCILLOSCOPE_STOP_MODE_FIX.md` - 本文档

## 参考资料

- DS100 Mini数字示波器用户手册V4.3
- www.alientek.com - 正点原子官网
