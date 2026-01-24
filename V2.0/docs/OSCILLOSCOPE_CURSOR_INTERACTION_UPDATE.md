# 示波器游标和偏移交互更新

## 修改日期
2026-01-24

## 问题描述
用户反馈点击X-Pos、Y-Pos按钮后无法在波形区域滑动调整偏移。

## 根本原因分析

### 问题1：功能互斥逻辑不完善
三个功能（Survey游标测量、X-Pos时间偏移、Y-Pos电压偏移）没有正确互斥，导致：
1. 当游标模式激活时，波形区域的触摸事件被游标逻辑拦截并break
2. X/Y偏移的滑动逻辑永远无法执行

### 问题2：事件处理优先级错误
波形区域事件处理器中，游标处理在X/Y偏移之前，导致即使关闭游标模式，如果游标对象还未删除，仍会拦截事件。

### 问题3：图表对象拦截触摸事件（关键问题）⭐
**这是导致无法滑动的根本原因！**

波形区域结构：
```
contWaveform (容器，有事件处理器)
  └── chartWaveform (图表，覆盖几乎整个容器)
```

- 图表对象默认是可点击的（`LV_OBJ_FLAG_CLICKABLE`）
- 图表覆盖了容器的大部分区域（716x396像素）
- 当用户触摸波形区域时，触摸事件被图表对象拦截
- 图表对象没有事件处理器，所以事件被"吞掉"
- 容器的事件处理器永远收不到 `LV_EVENT_PRESSED` 和 `LV_EVENT_PRESSING` 事件

## 解决方案

### 0. 让图表对象不拦截触摸事件（关键修复）⭐
**位置**：`setup_scr_scrOscilloscope.c`

```c
// 关键修复：让图表不拦截触摸事件，触摸事件会传递到父容器（contWaveform）
lv_obj_clear_flag(ui->scrOscilloscope_chartWaveform, LV_OBJ_FLAG_CLICKABLE);
lv_obj_add_flag(ui->scrOscilloscope_chartWaveform, LV_OBJ_FLAG_EVENT_BUBBLE);
```

**说明**：
- `LV_OBJ_FLAG_CLICKABLE`：清除此标志，图表不再响应点击事件
- `LV_OBJ_FLAG_EVENT_BUBBLE`：添加此标志，事件会冒泡到父对象
- 这样触摸事件就会传递到 `contWaveform`，由其事件处理器处理

### 1. 删除紫色触发电压横线
- 位置：`events_init.c` 的 `scrOscilloscope_event_handler`
- 删除了创建 `osc_trigger_line` 和 `osc_trigger_marker` 的代码
- 这是旧的触发电压选择功能，已不再需要

### 2. 从按钮移除滑动逻辑
- **X-Pos按钮**：删除 `LV_EVENT_PRESSED`、`LV_EVENT_PRESSING`、`LV_EVENT_RELEASED` 事件处理
- **Y-Pos按钮**：删除 `LV_EVENT_PRESSED`、`LV_EVENT_PRESSING`、`LV_EVENT_RELEASED` 事件处理
- 按钮现在只处理 `LV_EVENT_CLICKED` 来切换激活状态

### 3. 调整波形区域事件处理优先级
**位置**：`events_init.c` 的 `scrOscilloscope_contWaveform_event_handler`

**修改前的逻辑**（错误）：
```
LV_EVENT_PRESSING:
  1. 先检查游标模式 → 如果激活就break
  2. 后检查X/Y偏移 → 永远执行不到
```

**修改后的逻辑**（正确）：
```
LV_EVENT_PRESSING:
  1. 先检查X偏移激活 → 如果激活就处理并break
  2. 再检查Y偏移激活 → 如果激活就处理并break  
  3. 最后检查游标模式 → 如果激活就处理并break
```

这样确保了X/Y偏移的优先级高于游标，不会被拦截。

### 4. 实现三个功能的互斥逻辑

#### Survey按钮（游标测量）
```c
if (osc_cursor_mode != OSC_CURSOR_OFF) {
    // 关闭X/Y偏移模式
    osc_x_offset_active = false;
    osc_y_offset_active = false;
    // 恢复X-Pos和Y-Pos按钮样式
    // 隐藏Y基线
}
```

#### X-Pos按钮（时间偏移）
```c
if (osc_x_offset_active) {
    // 关闭游标模式和Y偏移
    osc_cursor_mode = OSC_CURSOR_OFF;
    osc_y_offset_active = false;
    // 删除游标对象
    // 恢复Survey和Y-Pos按钮样式
}
```

#### Y-Pos按钮（电压偏移）
```c
if (osc_y_offset_active) {
    // 关闭游标模式和X偏移
    osc_cursor_mode = OSC_CURSOR_OFF;
    osc_x_offset_active = false;
    // 删除游标对象
    // 恢复Survey和X-Pos按钮样式
}
```

### 4. 波形区域触摸逻辑保持不变
波形区域的 `scrOscilloscope_contWaveform_event_handler` 已经有完整的处理逻辑：
- 游标模式：拖动游标线
- X偏移模式：左右滑动调整时间偏移
- Y偏移模式：上下滑动调整电压偏移

## 使用流程

### 游标测量（Survey按钮）
1. 点击Survey按钮 → 循环切换：OFF → H-CUR → V-CUR
2. 在波形区域拖动 → 移动游标线并显示测量值
3. 自动关闭X-Pos和Y-Pos功能

### 时间偏移（X-Pos按钮）
1. 点击X-Pos按钮 → 激活（白底黑字）
2. 在波形区域左右滑动 → 调整时间偏移
3. 自动关闭Survey和Y-Pos功能

### 电压偏移（Y-Pos按钮）
1. 点击Y-Pos按钮 → 激活（白底黑字），显示绿色基线
2. 在波形区域上下滑动 → 调整电压偏移，基线跟随移动
3. 自动关闭Survey和X-Pos功能

## 技术细节

### 关键修复说明 ⭐
**为什么图表对象会拦截触摸事件？**

LVGL的事件传递机制：
1. 触摸事件首先发送给最上层的可点击对象
2. 如果对象有 `LV_OBJ_FLAG_CLICKABLE` 标志，它会处理事件
3. 如果对象没有事件处理器，事件会被"吞掉"（不传递）
4. 只有设置了 `LV_OBJ_FLAG_EVENT_BUBBLE`，事件才会冒泡到父对象

**我们的情况**：
- 图表对象默认可点击，覆盖了整个波形区域（716x396像素）
- 图表对象没有事件处理器
- 触摸事件被图表"吞掉"，容器的事件处理器收不到事件
- 结果：无法滑动！

**解决方法**：
```c
// 清除图表的可点击标志
lv_obj_clear_flag(ui->scrOscilloscope_chartWaveform, LV_OBJ_FLAG_CLICKABLE);
// 添加事件冒泡标志
lv_obj_add_flag(ui->scrOscilloscope_chartWaveform, LV_OBJ_FLAG_EVENT_BUBBLE);
```
- 触摸事件现在会传递到容器，由容器的事件处理器处理
- X-Pos、Y-Pos、Survey功能恢复正常！

### 互斥变量
- `osc_cursor_mode`：游标模式（OFF/HORIZONTAL/VERTICAL）
- `osc_x_offset_active`：X偏移激活标志
- `osc_y_offset_active`：Y偏移激活标志

### 按钮样式
- **激活**：白色背景、黑色文字、3px彩色边框
- **未激活**：黑色背景、白色文字、2px彩色边框

### 颜色方案
- Survey：紫色边框（0xFF00FF），游标线青色（0x00FFFF）或黄色（0xFFFF00）
- X-Pos：紫色边框（0xFF00FF）
- Y-Pos：绿色边框（0x00FF00），基线绿色

## 测试要点
1. ✅ 点击Survey后能在波形区域拖动游标
2. ✅ 点击X-Pos后能在波形区域左右滑动调整偏移
3. ✅ 点击Y-Pos后能在波形区域上下滑动调整偏移
4. ✅ 激活一个功能时，其他两个自动关闭
5. ✅ 按钮样式正确切换（白底黑字 ↔ 黑底白字）
6. ✅ 紫色触发电压横线已删除
7. ✅ 图表对象不再拦截触摸事件（关键修复）

## 相关文件
- `BSP/GUIDER/generated/events_init.c` - 事件处理逻辑
- `BSP/GUIDER/generated/setup_scr_scrOscilloscope.c` - UI初始化（关键修复在这里）

## 总结
问题的根本原因是**图表对象拦截了触摸事件**，而不是事件处理逻辑的问题。通过清除图表的 `LV_OBJ_FLAG_CLICKABLE` 标志并添加 `LV_OBJ_FLAG_EVENT_BUBBLE` 标志，触摸事件现在可以正确传递到容器的事件处理器，X-Pos、Y-Pos、Survey功能恢复正常。
