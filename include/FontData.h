#ifndef FONTDATA_H
#define FONTDATA_H

#include <Arduino.h>

// ==================== 字库数据声明 ====================
// "人人人人人人" - 6个字符测试长文本分组显示
extern uint16_t upper_text[];

// "人人人人人" - 5个字符测试长文本分组显示
extern uint16_t lower_text[];

// 32x32向右箭头(→)图案 - 三个相同箭头测试
extern uint16_t full_text[];

// ==================== 辅助函数声明 ====================
// 获取字符数量的辅助函数
int getUpperTextCharCount(); // 获取上半屏文本字符数
int getLowerTextCharCount(); // 获取下半屏文本字符数
int getFullTextCharCount();  // 获取全屏文本字符数

#endif