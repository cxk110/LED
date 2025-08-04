#ifndef CONFIG_H
#define CONFIG_H

/* ------------------------------------------------------------------------
 * 基础屏幕参数配置
 * ------------------------------------------------------------------------ */
#define SCREEN_WIDTH 64                                    // 屏幕宽度（列数）
#define SCREEN_HEIGHT 32                                   // 屏幕高度（行数）
#define TOTAL_LED_COUNT (SCREEN_WIDTH * SCREEN_HEIGHT * 3) // 总LED数量

/* ------------------------------------------------------------------------
 * 蓝牙协议配置
 * ------------------------------------------------------------------------ */
#define BT_FRAME_HEADER_1 0xAA     // 帧头第1字节
#define BT_FRAME_HEADER_2 0x55     // 帧头第2字节
#define BT_FRAME_TAIL_1 0x0D       // 帧尾第1字节
#define BT_FRAME_TAIL_2 0x0A       // 帧尾第2字节
#define BT_CMD_SET_DIRECTION 0x00  // 设置文本显示方向命令（默认：正向显示）
#define BT_CMD_SET_VERTICAL 0x01   // 设置文本竖向显示命令
#define BT_CMD_SET_FONT_16x16 0x02 // 设置16x16字体命令
#define BT_CMD_SET_FONT_32x32 0x03 // 设置32x32字体命令
#define BT_CMD_SET_TEXT 0x04       // 设置文本命令
#define BT_CMD_SET_ANIMATION 0x05  // 设置动画命令
#define BT_CMD_SET_COLOR 0x06      // 设置颜色命令
#define BT_CMD_SET_BRIGHTNESS 0x07 // 设置亮度命令
#define BT_CMD_SET_EFFECT 0x08     // 设置特效命令

/* ------------------------------------------------------------------------
 * 参数定义
 * ------------------------------------------------------------------------ */
#define BT_COLOR_TARGET_TEXT 0x01       // 选择文本
#define BT_COLOR_TARGET_BACKGROUND 0x02 // 选择背景
#define BT_COLOR_MODE_FIXED 0x01        // 固定色
#define BT_COLOR_MODE_GRADIENT 0x02     // 渐变色
#define BT_COLOR_DATA_LEN 7             // 颜色命令数据长度（屏幕区域+目标+模式+RGB+渐变模式，1+1+1+3+1字节）
#define BT_BRIGHTNESS_DATA_LEN 1        // 亮度命令数据长度（1字节）
#define BT_EFFECT_DATA_LEN 3            // 特效命令数据长度（3字节：屏幕区域+特效类型+速度）

/* ------------------------------------------------------------------------
 * 特效类型定义
 * ------------------------------------------------------------------------ */
#define BT_EFFECT_FIXED 0x00        // 固定显示
#define BT_EFFECT_SCROLL_LEFT 0x01  // 左移滚动
#define BT_EFFECT_SCROLL_RIGHT 0x02 // 右移滚动
#define BT_EFFECT_BLINK 0x03        // 闪烁特效
#define BT_EFFECT_BREATHE 0x04      // 呼吸特效
#define BT_EFFECT_SCROLL_UP 0x07    // 向上滚动（竖向显示时的左滚动）
#define BT_EFFECT_SCROLL_DOWN 0x08  // 向下滚动（竖向显示时的右滚动）

/* ------------------------------------------------------------------------
 * 渐变色模式定义
 * ------------------------------------------------------------------------ */
#define BT_GRADIENT_FIXED 0x00        // 固定色（无渐变）
#define BT_GRADIENT_VERTICAL_1 0x01   // 上下渐变组合1
#define BT_GRADIENT_VERTICAL_2 0x02   // 上下渐变组合2
#define BT_GRADIENT_VERTICAL_3 0x03   // 上下渐变组合3
#define BT_GRADIENT_HORIZONTAL_1 0x04 // 左右渐变组合1
#define BT_GRADIENT_HORIZONTAL_2 0x05 // 左右渐变组合2
#define BT_GRADIENT_HORIZONTAL_3 0x06 // 左右渐变组合3

/* ------------------------------------------------------------------------
 * 屏幕区域定义
 * ------------------------------------------------------------------------ */
#define BT_SCREEN_UPPER 0x01 // 上半屏
#define BT_SCREEN_LOWER 0x02 // 下半屏
#define BT_SCREEN_BOTH 0x03  // 全屏

/* ------------------------------------------------------------------------
 * 文本显示方向定义
 * ------------------------------------------------------------------------ */
#define BT_DIRECTION_HORIZONTAL 0x00 // 正向显示（默认）
#define BT_DIRECTION_VERTICAL 0x01   // 竖向显示（旋转90度）

/* ------------------------------------------------------------------------
 * 字体大小定义
 * ------------------------------------------------------------------------ */
#define BT_FONT_16x16 0x00 // 16x16字体（默认）
#define BT_FONT_32x32 0x01 // 32x32字体

/* ------------------------------------------------------------------------
 * 16x16字体参数
 * ------------------------------------------------------------------------ */
#define FONT_WIDTH_16 16   // 16x16字体宽度
#define FONT_HEIGHT_16 16  // 16x16字体高度
#define FONT_BYTES_16 32   // 16x16字体字节数
#define CHAR_SPACING_16 16 // 16x16字符间距

/* ------------------------------------------------------------------------
 * 32x32字体参数
 * ------------------------------------------------------------------------ */
#define FONT_WIDTH_32 32   // 32x32字体宽度
#define FONT_HEIGHT_32 32  // 32x32字体高度
#define FONT_BYTES_32 128  // 32x32字体字节数
#define CHAR_SPACING_32 32 // 32x32字符间距

/* ------------------------------------------------------------------------
 * 性能配置
 * ------------------------------------------------------------------------ */
#define BT_MAX_FRAME_SIZE 8192   // 最大帧大小
#define BT_FRAME_TIMEOUT_MS 5000 // 帧超时时间（毫秒）
#define BT_MAX_ERROR_COUNT 100   // 最大错误计数

#endif // CONFIG_H
