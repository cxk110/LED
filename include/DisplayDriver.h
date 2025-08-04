#ifndef DISPLAYDRIVER_H
#define DISPLAYDRIVER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config.h"

// 字体参数在config.h中定义，根据currentFontSize动态选择
// 移除固定宏定义，改为在代码中根据字体大小标志位选择对应参数

// 使用config.h中的屏幕尺寸定义
#define PANEL_RES_X SCREEN_WIDTH
#define PANEL_RES_Y SCREEN_HEIGHT

// 外部显示对象声明
extern MatrixPanel_I2S_DMA *dma_display;

// 外部字体大小标志位声明
extern uint8_t currentFontSize;

// 颜色定义
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0

// 函数声明
void drawChar16x16(int x, int y, const uint16_t *font_data, uint16_t color);
void drawChar16x16Gradient(int x, int y, const uint16_t *font_data, bool isUpper, uint8_t gradientMode);
void drawChar16x16Vertical(int x, int y, const uint16_t *font_data, uint16_t color);                             // 竖向显示字符
void drawChar16x16VerticalGradient(int x, int y, const uint16_t *font_data, bool isUpper, uint8_t gradientMode); // 竖向渐变字符
void drawString16x16(int x, int y, const uint16_t *font_array, int char_count, uint16_t color);
void drawString16x16Gradient(int x, int y, const uint16_t *font_array, int char_count, bool isUpper, uint8_t gradientMode);
void drawString16x16Vertical(int x, int y, const uint16_t *font_array, int char_count, uint16_t color);                             // 竖向显示字符串
void drawString16x16VerticalGradient(int x, int y, const uint16_t *font_array, int char_count, bool isUpper, uint8_t gradientMode); // 竖向渐变字符串
void drawChar32x32(int x, int y, const uint16_t *font_data, uint16_t color);                                                        // 32x32字符显示
void drawChar32x32Vertical(int x, int y, const uint16_t *font_data, uint16_t color);                                                // 32x32竖向显示字符
void drawChar32x32Gradient(int x, int y, const uint16_t *font_data, uint8_t gradientMode);                                          // 32x32字符渐变色显示
void drawChar32x32VerticalGradient(int x, int y, const uint16_t *font_data, uint8_t gradientMode);                                  // 32x32竖向字符渐变色显示
void drawString32x32(int x, int y, const uint16_t *font_array, int char_count, uint16_t color);                                     // 32x32字符串显示
void drawString32x32Vertical(int x, int y, const uint16_t *font_array, int char_count, uint16_t color);                             // 32x32竖向显示字符串
void drawString32x32Gradient(int x, int y, const uint16_t *font_array, int char_count, uint8_t gradientMode);                       // 32x32字符串渐变色显示
void drawString32x32VerticalGradient(int x, int y, const uint16_t *font_array, int char_count, uint8_t gradientMode);               // 32x32竖向字符串渐变色显示
uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b);                                                                              // RGB888转RGB565
uint16_t getGradientColor(int x, int y, bool isUpper, uint8_t gradientMode);                                                        // 获取16x16渐变色
uint16_t getGradientColor32x32(int x, int y, uint8_t gradientMode);                                                                 // 获取32x32渐变色

#endif