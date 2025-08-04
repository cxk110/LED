#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config.h"
#include "bluetooth_protocol.h"
#include "DisplayDriver.h"

// 颜色定义（从DisplayDriver.h迁移）
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0

// ==================== 结构体定义 ====================
// 文本显示状态结构
struct TextDisplayState
{
    String upperText;             // 上半屏文本
    String lowerText;             // 下半屏文本
    int upperIndex;               // 上半屏当前显示起始索引
    int lowerIndex;               // 下半屏当前显示起始索引
    unsigned long lastSwitchTime; // 上次切换时间
    bool needUpdate;              // 是否需要更新显示
    uint8_t displayDirection;     // 文本显示方向（0x00正向，0x01竖向）
};

// 颜色状态结构（支持上下半屏独立颜色）
struct ColorState
{
    // 上半屏颜色
    uint16_t upperTextColor;                    // 上半屏文本颜色
    uint16_t upperBackgroundColor;              // 上半屏背景颜色
    uint8_t upperTextMode;                      // 上半屏文本颜色模式
    uint8_t upperBgMode;                        // 上半屏背景颜色模式
    uint8_t upperTextR, upperTextG, upperTextB; // 上半屏文本RGB值
    uint8_t upperBgR, upperBgG, upperBgB;       // 上半屏背景RGB值
    uint8_t upperGradientMode;                  // 上半屏渐变模式

    // 下半屏颜色
    uint16_t lowerTextColor;                    // 下半屏文本颜色
    uint16_t lowerBackgroundColor;              // 下半屏背景颜色
    uint8_t lowerTextMode;                      // 下半屏文本颜色模式
    uint8_t lowerBgMode;                        // 下半屏背景颜色模式
    uint8_t lowerTextR, lowerTextG, lowerTextB; // 下半屏文本RGB值
    uint8_t lowerBgR, lowerBgG, lowerBgB;       // 下半屏背景RGB值
    uint8_t lowerGradientMode;                  // 下半屏渐变模式

    // 全屏颜色（兼容性）
    uint16_t textColor;          // 全屏文本颜色
    uint16_t backgroundColor;    // 全屏背景颜色
    uint8_t textMode;            // 全屏文本颜色模式
    uint8_t bgMode;              // 全屏背景颜色模式
    uint8_t textR, textG, textB; // 全屏文本RGB值
    uint8_t bgR, bgG, bgB;       // 全屏背景RGB值
    uint8_t gradientMode;        // 全屏渐变模式

    unsigned long gradientTime; // 渐变时间计数
    bool needColorUpdate;       // 是否需要更新颜色
};

// 亮度状态结构
struct BrightnessState
{
    uint8_t brightness;        // 当前亮度值 (0-255)
    bool needBrightnessUpdate; // 是否需要更新亮度
};

// 特效状态结构（包含滚动、闪烁、呼吸等）
struct EffectState
{
    // 滚动特效
    bool upperScrollActive;   // 上半屏滚动激活
    bool lowerScrollActive;   // 下半屏滚动激活
    uint8_t upperScrollType;  // 上半屏滚动类型（左/右）
    uint8_t lowerScrollType;  // 下半屏滚动类型（左/右）
    uint8_t upperScrollSpeed; // 上半屏滚动速度
    uint8_t lowerScrollSpeed; // 下半屏滚动速度
    int upperScrollOffset;    // 上半屏滚动偏移量
    int lowerScrollOffset;    // 下半屏滚动偏移量

    // 闪烁特效
    bool upperBlinkActive;   // 上半屏闪烁激活
    bool lowerBlinkActive;   // 下半屏闪烁激活
    uint8_t upperBlinkSpeed; // 上半屏闪烁速度
    uint8_t lowerBlinkSpeed; // 下半屏闪烁速度
    bool upperBlinkVisible;  // 上半屏当前是否可见
    bool lowerBlinkVisible;  // 下半屏当前是否可见

    // 呼吸特效
    bool upperBreatheActive;   // 上半屏呼吸激活
    bool lowerBreatheActive;   // 下半屏呼吸激活
    uint8_t upperBreatheSpeed; // 上半屏呼吸速度
    uint8_t lowerBreatheSpeed; // 下半屏呼吸速度
    float upperBreathePhase;   // 上半屏呼吸相位
    float lowerBreathePhase;   // 下半屏呼吸相位

    unsigned long lastEffectTime; // 上次特效更新时间
    bool needEffectUpdate;        // 是否需要更新特效
};

// ==================== 全局变量声明 ====================
extern TextDisplayState textState;
extern ColorState colorState;
extern BrightnessState brightnessState;
extern EffectState effectState;
extern uint8_t currentFontSize;
extern MatrixPanel_I2S_DMA *dma_display;

// 动态点阵数据存储
extern uint16_t *dynamic_upper_text; // 动态上半屏点阵数据
extern uint16_t *dynamic_lower_text; // 动态下半屏点阵数据
extern uint16_t *dynamic_full_text;  // 动态全屏点阵数据
extern int dynamic_upper_char_count; // 动态上半屏字符数
extern int dynamic_lower_char_count; // 动态下半屏字符数
extern int dynamic_full_char_count;  // 动态全屏字符数

// ==================== 函数声明 ====================
// 硬件初始化
bool initializeDisplay();

// 内存管理
void freeDynamicTextData(); // 释放动态点阵数据内存

// 示例演示函数
void demoBluetoothDataUsage(); // 演示如何使用蓝牙点阵数据

// 文本显示相关函数
void handleTextCommand(const uint16_t *upperData, int upperCharCount, const uint16_t *lowerData, int lowerCharCount); // 处理点阵数据命令
void handleDirectionCommand(uint8_t direction);                                                                       // 处理显示方向命令
void displayTextOnHalf(int y, bool isUpper);                                                                          // 在半屏显示文本
void displayFullScreenText32x32();                                                                                    // 32x32全屏文本显示
void handleFullScreenTextCommand(const uint16_t *fontData, int charCount);                                            // 处理32x32全屏点阵数据命令
void updateTextDisplay();                                                                                             // 更新文本显示

// 颜色相关函数
void handleColorCommand(const BluetoothFrame &frame);                        // 处理颜色命令
void updateColors();                                                         // 更新颜色状态
uint16_t getGradientColor(int x, int y, bool isUpper, uint8_t gradientMode); // 获取渐变色

// 亮度相关函数
void handleBrightnessCommand(const BluetoothFrame &frame); // 处理亮度命令
void updateBrightness();                                   // 更新亮度设置

// 特效相关函数
void handleEffectCommand(const BluetoothFrame &frame);                                                            // 处理特效命令
void clearAllEffects(bool isUpper);                                                                               // 清除指定半屏的所有特效
void setScrollEffect(bool isUpper, uint8_t scrollType, uint8_t speed);                                            // 设置滚动特效
void setBlinkEffect(bool isUpper, uint8_t speed);                                                                 // 设置闪烁特效
void setBreatheEffect(bool isUpper, uint8_t speed);                                                               // 设置呼吸特效
void displayScrollingText(const uint16_t *font_data, int char_count, int offset, int y, uint8_t scrollType);      // 显示滚动文本
void displayScrollingText32x32(const uint16_t *font_data, int char_count, int offset, int y, uint8_t scrollType); // 显示32x32滚动文本
void updateScrollEffect();                                                                                        // 更新滚动特效
void updateBlinkEffect();                                                                                         // 更新闪烁特效
void updateBreatheEffect();                                                                                       // 更新呼吸特效
void updateAllEffects();                                                                                          // 更新所有特效

#endif