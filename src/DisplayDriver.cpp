#include "DisplayDriver.h"

// 前向声明外部变量和结构
extern struct ColorState
{
    uint16_t upperTextColor, upperBackgroundColor;
    uint8_t upperTextMode, upperBgMode;
    uint8_t upperTextR, upperTextG, upperTextB;
    uint8_t upperBgR, upperBgG, upperBgB;
    uint8_t upperGradientMode;
    uint16_t lowerTextColor, lowerBackgroundColor;
    uint8_t lowerTextMode, lowerBgMode;
    uint8_t lowerTextR, lowerTextG, lowerTextB;
    uint8_t lowerBgR, lowerBgG, lowerBgB;
    uint8_t lowerGradientMode;
    uint16_t textColor, backgroundColor;
    uint8_t textMode, bgMode;
    uint8_t textR, textG, textB;
    uint8_t bgR, bgG, bgB;
    uint8_t gradientMode;
    unsigned long gradientTime;
    bool needColorUpdate;
} colorState;

// 渐变色组合结构
struct GradientColors
{
    uint8_t colors[7][3];
};

// 预定义的渐变色组合
const GradientColors gradientCombinations[6] = {
    // 上下渐变组合1：红橙黄绿青蓝紫
    {{{255, 0, 0}, {255, 127, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255}, {0, 0, 255}, {127, 0, 255}}},
    // 上下渐变组合2：明亮彩虹色（红粉紫蓝青绿黄）
    {{{255, 0, 0}, {255, 0, 255}, {128, 0, 255}, {0, 128, 255}, {0, 255, 255}, {0, 255, 0}, {255, 255, 0}}},
    // 上下渐变组合3：紫蓝青绿黄橙红
    {{{127, 0, 255}, {0, 0, 255}, {0, 255, 255}, {0, 255, 0}, {255, 255, 0}, {255, 127, 0}, {255, 0, 0}}},
    // 左右渐变组合1：火焰渐变（红→橙→黄→白→黄→橙→红）
    {{{255, 0, 0}, {255, 127, 0}, {255, 255, 0}, {255, 255, 255}, {255, 255, 0}, {255, 127, 0}, {255, 0, 0}}},
    // 左右渐变组合2：明亮霓虹色（粉红→橙→黄→绿→青→蓝→紫）
    {{{255, 20, 147}, {255, 165, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255}, {0, 0, 255}, {138, 43, 226}}},
    // 左右渐变组合3：森林渐变（深绿→浅绿→黄绿→白→黄绿→浅绿→深绿）
    {{{0, 100, 0}, {0, 200, 0}, {127, 255, 0}, {255, 255, 255}, {127, 255, 0}, {0, 200, 0}, {0, 100, 0}}}};


void drawChar16x16(int x, int y, const uint16_t *font_data, uint16_t color)
{
    // 使用列取模方式：每个uint16_t代表一列的16个像素
    for (int col = 0; col < 16; col++)
    {
        uint16_t col_data = font_data[col];
        for (int row = 0; row < 16; row++)
        {
            // 检查对应位是否为1（从高位开始，高位对应上方像素）
            if (col_data & (0x8000 >> row))
            {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

// 竖向显示字符（向左旋转90度）
void drawChar16x16Vertical(int x, int y, const uint16_t *font_data, uint16_t color)
{
    // 向左旋转90度：原来的(col, row)变成(row, 15-col)
    for (int col = 0; col < 16; col++)
    {
        uint16_t col_data = font_data[col];
        for (int row = 0; row < 16; row++)
        {
            // 检查对应位是否为1
            if (col_data & (0x8000 >> row))
            {
                int px = x + row;        // 向左旋转后的X坐标
                int py = y + (15 - col); // 向左旋转后的Y坐标
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

void drawChar16x16Gradient(int x, int y, const uint16_t *font_data, bool isUpper, uint8_t gradientMode)
{
    // 使用列取模方式：每个uint16_t代表一列的16个像素
    for (int col = 0; col < 16; col++)
    {
        uint16_t col_data = font_data[col];
        for (int row = 0; row < 16; row++)
        {
            // 检查对应位是否为1（从高位开始，高位对应上方像素）
            if (col_data & (0x8000 >> row))
            {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    // 根据像素位置获取渐变色
                    uint16_t color = getGradientColor(px, py, isUpper, gradientMode);
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

// 竖向渐变字符（向左旋转90度）
void drawChar16x16VerticalGradient(int x, int y, const uint16_t *font_data, bool isUpper, uint8_t gradientMode)
{
    // 向左旋转90度：原来的(col, row)变成(row, 15-col)
    for (int col = 0; col < 16; col++)
    {
        uint16_t col_data = font_data[col];
        for (int row = 0; row < 16; row++)
        {
            // 检查对应位是否为1
            if (col_data & (0x8000 >> row))
            {
                int px = x + row;        // 向左旋转后的X坐标
                int py = y + (15 - col); // 向左旋转后的Y坐标
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    // 根据旋转后的像素位置获取渐变色
                    uint16_t color = getGradientColor(px, py, isUpper, gradientMode);
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

void drawString16x16(int x, int y, const uint16_t *font_array, int char_count, uint16_t color)
{
    int pos_x = x;

    // 遍历每个字符的字库数据
    for (int i = 0; i < char_count; i++)
    {
        // 每个字符占用16个uint16_t
        const uint16_t *char_data = &font_array[i * 16];
        drawChar16x16(pos_x, y, char_data, color);
        pos_x += CHAR_SPACING_16; // 移动到下一个字符位置
    }
}

void drawString16x16Gradient(int x, int y, const uint16_t *font_array, int char_count, bool isUpper, uint8_t gradientMode)
{
    int pos_x = x;

    // 遍历每个字符的字库数据
    for (int i = 0; i < char_count; i++)
    {
        // 每个字符占用16个uint16_t
        const uint16_t *char_data = &font_array[i * 16];
        drawChar16x16Gradient(pos_x, y, char_data, isUpper, gradientMode);
        pos_x += CHAR_SPACING_16; // 移动到下一个字符位置
    }
}

// 竖向显示字符串
void drawString16x16Vertical(int x, int y, const uint16_t *font_array, int char_count, uint16_t color)
{
    int pos_x = x;

    // 遍历每个字符的字库数据，向左旋转后从左到右排列
    for (int i = 0; i < char_count; i++)
    {
        // 每个字符占用16个uint16_t
        const uint16_t *char_data = &font_array[i * 16];
        drawChar16x16Vertical(pos_x, y, char_data, color);
        pos_x += CHAR_SPACING_16; // 水平移动到下一个字符位置
    }
}

// 竖向渐变字符串
void drawString16x16VerticalGradient(int x, int y, const uint16_t *font_array, int char_count, bool isUpper, uint8_t gradientMode)
{
    int pos_x = x;

    // 遍历每个字符的字库数据，向左旋转后从左到右排列
    for (int i = 0; i < char_count; i++)
    {
        // 每个字符占用16个uint16_t
        const uint16_t *char_data = &font_array[i * 16];
        drawChar16x16VerticalGradient(pos_x, y, char_data, isUpper, gradientMode);
        pos_x += CHAR_SPACING_16; // 水平移动到下一个字符位置
    }
}

// RGB888转RGB565颜色格式
uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// 获取渐变色
uint16_t getGradientColor(int x, int y, bool isUpper, uint8_t gradientMode)
{
    if (gradientMode == BT_GRADIENT_FIXED)
    {
        // 固定色，返回基础颜色
        return isUpper ? colorState.upperTextColor : colorState.lowerTextColor;
    }

    // 获取对应的渐变组合
    int combinationIndex = -1;
    bool isVertical = false;

    if (gradientMode >= BT_GRADIENT_VERTICAL_1 && gradientMode <= BT_GRADIENT_VERTICAL_3)
    {
        combinationIndex = gradientMode - BT_GRADIENT_VERTICAL_1; // 0,1,2
        isVertical = true;
    }
    else if (gradientMode >= BT_GRADIENT_HORIZONTAL_1 && gradientMode <= BT_GRADIENT_HORIZONTAL_3)
    {
        combinationIndex = gradientMode - BT_GRADIENT_HORIZONTAL_1 + 3; // 3,4,5
        isVertical = false;
    }

    if (combinationIndex < 0 || combinationIndex >= 6)
    {
        // 无效的渐变模式，返回基础颜色
        return isUpper ? colorState.upperTextColor : colorState.lowerTextColor;
    }

    const GradientColors &gradient = gradientCombinations[combinationIndex];
    int colorIndex = 0;

    if (isVertical)
    {
        // 上下渐变：根据Y坐标计算颜色索引
        int relativeY = isUpper ? y : (y - FONT_HEIGHT_16); // 转换为相对于半屏的Y坐标
        colorIndex = (relativeY * 7) / FONT_HEIGHT_16;      // 16像素高度分成7块
    }
    else
    {
        // 左右渐变：根据X坐标计算颜色索引
        colorIndex = (x * 7) / SCREEN_WIDTH; // 屏幕宽度分成7块
    }

    // 确保索引在有效范围内
    if (colorIndex < 0)
        colorIndex = 0;
    if (colorIndex >= 7)
        colorIndex = 6;

    // 获取对应颜色并转换为RGB565
    uint8_t r = gradient.colors[colorIndex][0];
    uint8_t g = gradient.colors[colorIndex][1];
    uint8_t b = gradient.colors[colorIndex][2];

    return rgb888to565(r, g, b);
}

// 32x32字符显示函数
void drawChar32x32(int x, int y, const uint16_t *font_data, uint16_t color)
{
    // 使用列取模方式：每个uint16_t代表一列的32个像素
    for (int col = 0; col < 32; col++)
    {
        uint16_t column_data_low = font_data[col * 2];      // 低16位
        uint16_t column_data_high = font_data[col * 2 + 1]; // 高16位

        for (int row = 0; row < 32; row++)
        {
            bool pixel_on;
            if (row < 16)
            {
                pixel_on = (column_data_low & (0x8000 >> row)) != 0; // 统一为从高位开始
            }
            else
            {
                pixel_on = (column_data_high & (0x8000 >> (row - 16))) != 0; // 统一为从高位开始
            }

            if (pixel_on)
            {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

// 32x32竖向字符显示函数
void drawChar32x32Vertical(int x, int y, const uint16_t *font_data, uint16_t color)
{
    // 竖向显示：将字符向左旋转90度
    for (int col = 0; col < 32; col++)
    {
        uint16_t column_data_low = font_data[col * 2];      // 低16位
        uint16_t column_data_high = font_data[col * 2 + 1]; // 高16位

        for (int row = 0; row < 32; row++)
        {
            bool pixel_on;
            if (row < 16)
            {
                pixel_on = (column_data_low & (0x8000 >> row)) != 0; // 统一为从高位开始
            }
            else
            {
                pixel_on = (column_data_high & (0x8000 >> (row - 16))) != 0; // 统一为从高位开始
            }

            if (pixel_on)
            {
                // 旋转坐标：原(col, row) -> 新(row, 31-col)
                int px = x + row;
                int py = y + (31 - col);
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

// 32x32字符串显示函数
void drawString32x32(int x, int y, const uint16_t *font_array, int char_count, uint16_t color)
{
    for (int i = 0; i < char_count; i++)
    {
        const uint16_t *char_data = font_array + (i * 64); // 每个32x32字符64个uint16_t
        drawChar32x32(x + i * CHAR_SPACING_32, y, char_data, color);
    }
}

// 32x32竖向字符串显示函数
void drawString32x32Vertical(int x, int y, const uint16_t *font_array, int char_count, uint16_t color)
{
    for (int i = 0; i < char_count; i++)
    {
        const uint16_t *char_data = font_array + (i * 64); // 每个32x32字符64个uint16_t
        drawChar32x32Vertical(x + i * CHAR_SPACING_32, y, char_data, color);
    }
}

// 32x32字符渐变色显示函数
void drawChar32x32Gradient(int x, int y, const uint16_t *font_data, uint8_t gradientMode)
{
    // 使用列取模方式：每个uint16_t代表一列的32个像素
    for (int col = 0; col < 32; col++)
    {
        uint16_t column_data_low = font_data[col * 2];      // 低16位
        uint16_t column_data_high = font_data[col * 2 + 1]; // 高16位

        for (int row = 0; row < 32; row++)
        {
            bool pixel_on;
            if (row < 16)
            {
                pixel_on = (column_data_low & (0x8000 >> row)) != 0;
            }
            else
            {
                pixel_on = (column_data_high & (0x8000 >> (row - 16))) != 0;
            }

            if (pixel_on)
            {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    // 根据像素位置获取32x32渐变色
                    uint16_t color = getGradientColor32x32(px, py, gradientMode);
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

// 32x32竖向字符渐变色显示函数（向左旋转90度）
void drawChar32x32VerticalGradient(int x, int y, const uint16_t *font_data, uint8_t gradientMode)
{
    // 向左旋转90度：原(col, row) -> 新(row, 31-col)
    for (int col = 0; col < 32; col++)
    {
        uint16_t column_data_low = font_data[col * 2];      // 低16位
        uint16_t column_data_high = font_data[col * 2 + 1]; // 高16位

        for (int row = 0; row < 32; row++)
        {
            bool pixel_on;
            if (row < 16)
            {
                pixel_on = (column_data_low & (0x8000 >> row)) != 0;
            }
            else
            {
                pixel_on = (column_data_high & (0x8000 >> (row - 16))) != 0;
            }

            if (pixel_on)
            {
                // 旋转坐标：原(col, row) -> 新(row, 31-col)
                int px = x + row;
                int py = y + (31 - col);
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y)
                {
                    // 根据旋转后的像素位置获取32x32渐变色
                    uint16_t color = getGradientColor32x32(px, py, gradientMode);
                    dma_display->drawPixel(px, py, color);
                }
            }
        }
    }
}

// 32x32字符串渐变色显示函数
void drawString32x32Gradient(int x, int y, const uint16_t *font_array, int char_count, uint8_t gradientMode)
{
    for (int i = 0; i < char_count; i++)
    {
        const uint16_t *char_data = font_array + (i * 64); // 每个32x32字符64个uint16_t
        drawChar32x32Gradient(x + i * CHAR_SPACING_32, y, char_data, gradientMode);
    }
}

// 32x32竖向字符串渐变色显示函数
void drawString32x32VerticalGradient(int x, int y, const uint16_t *font_array, int char_count, uint8_t gradientMode)
{
    for (int i = 0; i < char_count; i++)
    {
        const uint16_t *char_data = font_array + (i * 64); // 每个32x32字符64个uint16_t
        drawChar32x32VerticalGradient(x + i * CHAR_SPACING_32, y, char_data, gradientMode);
    }
}

// 获取32x32渐变色（仿照16x16逻辑）
uint16_t getGradientColor32x32(int x, int y, uint8_t gradientMode)
{
    if (gradientMode == BT_GRADIENT_FIXED)
    {
        // 固定色，返回全屏文本颜色
        return colorState.textColor;
    }

    // 获取对应的渐变组合
    int combinationIndex = -1;
    bool isVertical = false;

    if (gradientMode >= BT_GRADIENT_VERTICAL_1 && gradientMode <= BT_GRADIENT_VERTICAL_3)
    {
        combinationIndex = gradientMode - BT_GRADIENT_VERTICAL_1; // 0,1,2
        isVertical = true;
    }
    else if (gradientMode >= BT_GRADIENT_HORIZONTAL_1 && gradientMode <= BT_GRADIENT_HORIZONTAL_3)
    {
        combinationIndex = gradientMode - BT_GRADIENT_HORIZONTAL_1 + 3; // 3,4,5
        isVertical = false;
    }

    if (combinationIndex < 0 || combinationIndex >= 6)
    {
        // 无效的渐变模式，返回全屏文本颜色
        return colorState.textColor;
    }

    const GradientColors &gradient = gradientCombinations[combinationIndex];
    int colorIndex = 0;

    if (isVertical)
    {
        // 上下渐变：32像素高度分成7块（关键差异）
        colorIndex = (y * 7) / FONT_HEIGHT_32;
    }
    else
    {
        // 左右渐变：SCREEN_WIDTH像素宽度分成7块（使用宏定义）
        colorIndex = (x * 7) / SCREEN_WIDTH;
    }

    // 确保索引在有效范围内
    if (colorIndex < 0)
        colorIndex = 0;
    if (colorIndex >= 7)
        colorIndex = 6;

    // 获取对应颜色并转换为RGB565
    uint8_t r = gradient.colors[colorIndex][0];
    uint8_t g = gradient.colors[colorIndex][1];
    uint8_t b = gradient.colors[colorIndex][2];

    return rgb888to565(r, g, b);
}
