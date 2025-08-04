#include "LEDController.h"
#include "DisplayDriver.h"
#include "FontData.h"

// ==================== 全局变量定义 ====================
TextDisplayState textState = {"", "", 0, 0, 0, false, BT_DIRECTION_HORIZONTAL}; // 全局文本状态
uint8_t currentFontSize = BT_FONT_16x16;                                        // 全局字体大小标志位
ColorState colorState = {
    // 上半屏颜色初始化
    COLOR_WHITE, 0x0000, BT_COLOR_MODE_FIXED, BT_COLOR_MODE_FIXED,
    255, 255, 255, 0, 0, 0, BT_GRADIENT_FIXED,
    // 下半屏颜色初始化
    COLOR_WHITE, 0x0000, BT_COLOR_MODE_FIXED, BT_COLOR_MODE_FIXED,
    255, 255, 255, 0, 0, 0, BT_GRADIENT_FIXED,
    // 全屏颜色初始化（兼容性）
    COLOR_WHITE, 0x0000, BT_COLOR_MODE_FIXED, BT_COLOR_MODE_FIXED,
    255, 255, 255, 0, 0, 0, BT_GRADIENT_FIXED,
    0, false}; // 全局颜色状态
EffectState effectState = {
    // 滚动特效初始化
    false, false, BT_EFFECT_FIXED, BT_EFFECT_FIXED, 5, 5, 0, 0,
    // 闪烁特效初始化
    false, false, 5, 5, true, true,
    // 呼吸特效初始化
    false, false, 5, 5, 0.0, 0.0,
    0, false};                                  // 全局特效状态
BrightnessState brightnessState = {128, false}; // 全局亮度状态，默认50%亮度

MatrixPanel_I2S_DMA *dma_display = nullptr;

// ==================== 动态点阵数据存储 ====================
uint16_t *dynamic_upper_text = nullptr; // 动态上半屏点阵数据
uint16_t *dynamic_lower_text = nullptr; // 动态下半屏点阵数据
uint16_t *dynamic_full_text = nullptr;  // 动态全屏点阵数据
int dynamic_upper_char_count = 0;       // 动态上半屏字符数
int dynamic_lower_char_count = 0;       // 动态下半屏字符数
int dynamic_full_char_count = 0;        // 动态全屏字符数

// 内存管理辅助函数
void freeDynamicTextData()
{
    if (dynamic_upper_text)
    {
        free(dynamic_upper_text);
        dynamic_upper_text = nullptr;
    }
    if (dynamic_lower_text)
    {
        free(dynamic_lower_text);
        dynamic_lower_text = nullptr;
    }
    if (dynamic_full_text)
    {
        free(dynamic_full_text);
        dynamic_full_text = nullptr;
    }
    dynamic_upper_char_count = 0;
    dynamic_lower_char_count = 0;
    dynamic_full_char_count = 0;
}

// ==================== 硬件初始化 ====================
bool initializeDisplay()
{
    // HUB75自定义引脚配置
    HUB75_I2S_CFG::i2s_pins _pins = {
        25, 26, 27, 14, 12, 13, // R1, G1, B1, R2, G2, B2
        23, 22, 21, 19, -1,     // A, B, C, D, E
        4, 15, 16               // LAT, OE, CLK
    };

    HUB75_I2S_CFG mxconfig(
        SCREEN_WIDTH,  // 模块宽度
        SCREEN_HEIGHT, // 模块高度
        1,             // 面板链长度
        _pins          // 自定义引脚配置
    );

    // 创建显示对象
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    if (dma_display == nullptr)
    {
        return false;
    }

    dma_display->begin();
    dma_display->setBrightness8(50); // 亮度0-255
    dma_display->clearScreen();

    return true;
}

// ==================== 文本显示相关函数 ====================
// 在半屏显示文本（支持分组显示和所有特效）
void displayTextOnHalf(int y, bool isUpper)
{
    // 检查闪烁特效是否应该隐藏文本
    bool blinkActive = isUpper ? effectState.upperBlinkActive : effectState.lowerBlinkActive;
    bool blinkVisible = isUpper ? effectState.upperBlinkVisible : effectState.lowerBlinkVisible;

    if (blinkActive && !blinkVisible)
    {
        return; // 闪烁特效激活且当前应该隐藏，直接返回
    }

    // 根据上下半屏选择对应的点阵数据（优先使用动态数据）
    const uint16_t *font_data;
    int total_char_count;

    if (isUpper)
    {
        // 上半屏：优先使用动态数据，否则使用静态数据
        if (dynamic_upper_text && dynamic_upper_char_count > 0)
        {
            font_data = dynamic_upper_text;
            total_char_count = dynamic_upper_char_count;
        }
        else
        {
            font_data = upper_text;
            total_char_count = getUpperTextCharCount();
        }
    }
    else
    {
        // 下半屏：优先使用动态数据，否则使用静态数据
        if (dynamic_lower_text && dynamic_lower_char_count > 0)
        {
            font_data = dynamic_lower_text;
            total_char_count = dynamic_lower_char_count;
        }
        else
        {
            font_data = lower_text;
            total_char_count = getLowerTextCharCount();
        }
    }

    // 检查是否启用滚动特效
    bool scrollActive = isUpper ? effectState.upperScrollActive : effectState.lowerScrollActive;

    // 检查是否使用渐变色
    uint8_t textMode = isUpper ? colorState.upperTextMode : colorState.lowerTextMode;
    uint8_t gradientMode = isUpper ? colorState.upperGradientMode : colorState.lowerGradientMode;
    bool useGradient = (textMode == BT_COLOR_MODE_GRADIENT && gradientMode != BT_GRADIENT_FIXED);

    // 获取基础颜色（用于固定色和呼吸特效）
    uint16_t baseColor = isUpper ? colorState.upperTextColor : colorState.lowerTextColor;
    uint16_t textColor = baseColor;

    // 应用呼吸特效到颜色（仅对固定色有效，渐变色不支持呼吸特效）
    bool breatheActive = isUpper ? effectState.upperBreatheActive : effectState.lowerBreatheActive;
    if (breatheActive && !useGradient)
    {
        float phase = isUpper ? effectState.upperBreathePhase : effectState.lowerBreathePhase;
        float brightness = (sin(phase) + 1.0) / 2.0; // 0.0 到 1.0 的正弦波
        brightness = 0.2 + brightness * 0.8;         // 范围从0.2到1.0，避免完全黑暗

        // 提取RGB分量（RGB565格式）
        uint8_t r = (baseColor >> 11) & 0x1F; // 5位红色
        uint8_t g = (baseColor >> 5) & 0x3F;  // 6位绿色
        uint8_t b = baseColor & 0x1F;         // 5位蓝色

        // 应用呼吸亮度
        r = (uint8_t)(r * brightness);
        g = (uint8_t)(g * brightness);
        b = (uint8_t)(b * brightness);

        // 重新组合颜色
        textColor = (r << 11) | (g << 5) | b;
    }

    if (scrollActive)
    {
        // 滚动模式：显示所有字符
        uint8_t scrollType = isUpper ? effectState.upperScrollType : effectState.lowerScrollType;
        int scrollOffset = isUpper ? effectState.upperScrollOffset : effectState.lowerScrollOffset;
        displayScrollingText(font_data, total_char_count, scrollOffset, y, scrollType);
        return;
    }

    // 检查显示方向
    bool isVertical = (textState.displayDirection == BT_DIRECTION_VERTICAL);

    // 非滚动模式：分组显示
    if (isVertical)
    {
        // 竖向显示：每行最多显示4个字符（向左旋转后水平排列）
        const int maxCharsPerLine = 4;

        // 如果字符数不超过4个，直接居中显示
        if (total_char_count <= maxCharsPerLine)
        {
            int display_x = (SCREEN_WIDTH - total_char_count * CHAR_SPACING_16) / 2;
            if (display_x < 0)
                display_x = 0;

            if (useGradient)
            {
                drawString16x16VerticalGradient(display_x, y, font_data, total_char_count, isUpper, gradientMode);
            }
            else
            {
                drawString16x16Vertical(display_x, y, font_data, total_char_count, textColor);
            }
            return;
        }

        // 超过4个字符，按组显示
        int currentIndex = isUpper ? textState.upperIndex : textState.lowerIndex;
        int startCharIndex = currentIndex * maxCharsPerLine;
        int displayCharCount = min(maxCharsPerLine, total_char_count - startCharIndex);

        if (displayCharCount <= 0)
            return;

        // 计算居中位置
        int display_x = (SCREEN_WIDTH - displayCharCount * CHAR_SPACING_16) / 2;
        if (display_x < 0)
            display_x = 0;

        const uint16_t *current_font_data = font_data + (startCharIndex * 16);
        if (useGradient)
        {
            drawString16x16VerticalGradient(display_x, y, current_font_data, displayCharCount, isUpper, gradientMode);
        }
        else
        {
            drawString16x16Vertical(display_x, y, current_font_data, displayCharCount, textColor);
        }
    }
    else
    {
        // 水平显示：每行最多显示4个字符
        const int maxCharsPerLine = 4;

        // 如果字符数不超过4个，直接居中显示
        if (total_char_count <= maxCharsPerLine)
        {
            int x = (SCREEN_WIDTH - total_char_count * CHAR_SPACING_16) / 2;
            if (x < 0)
                x = 0;

            if (useGradient)
            {
                drawString16x16Gradient(x, y, font_data, total_char_count, isUpper, gradientMode);
            }
            else
            {
                drawString16x16(x, y, font_data, total_char_count, textColor);
            }
            return;
        }

        // 超过4个字符，按组显示
        int currentIndex = isUpper ? textState.upperIndex : textState.lowerIndex;
        int startCharIndex = currentIndex * maxCharsPerLine;
        int displayCharCount = min(maxCharsPerLine, total_char_count - startCharIndex);

        if (displayCharCount <= 0)
            return;

        // 计算居中位置
        int x = (SCREEN_WIDTH - displayCharCount * CHAR_SPACING_16) / 2;
        if (x < 0)
            x = 0;

        // 显示当前组的字符
        const uint16_t *current_font_data = font_data + (startCharIndex * 16);
        if (useGradient)
        {
            drawString16x16Gradient(x, y, current_font_data, displayCharCount, isUpper, gradientMode);
        }
        else
        {
            drawString16x16(x, y, current_font_data, displayCharCount, textColor);
        }
    }
}

// 32x32全屏文本显示函数（仿照16x16逻辑）
void displayFullScreenText32x32()
{
    // 检查闪烁特效是否应该隐藏文本（仿照16x16逻辑）
    bool blinkActive = effectState.upperBlinkActive; // 32x32全屏使用上半屏闪烁状态
    bool blinkVisible = effectState.upperBlinkVisible;

    if (blinkActive && !blinkVisible)
    {
        return; // 闪烁特效激活且当前应该隐藏，直接返回
    }

    // 使用全屏点阵数据（优先使用动态数据）
    const uint16_t *font_data;
    int total_char_count;

    if (dynamic_full_text && dynamic_full_char_count > 0)
    {
        // 优先使用动态数据
        font_data = dynamic_full_text;
        total_char_count = dynamic_full_char_count;
    }
    else
    {
        // 否则使用静态数据
        font_data = full_text;
        total_char_count = getFullTextCharCount();
    }

    // 检查是否启用滚动特效（仿照16x16逻辑）
    bool scrollActive = effectState.upperScrollActive; // 32x32全屏使用上半屏滚动状态

    // 检查是否使用渐变色（仿照16x16逻辑）
    uint8_t textMode = colorState.textMode;
    uint8_t gradientMode = colorState.gradientMode;
    bool useGradient = (textMode == BT_COLOR_MODE_GRADIENT && gradientMode != BT_GRADIENT_FIXED);

    // 获取基础颜色（用于固定色和呼吸特效）
    uint16_t baseColor = colorState.textColor;
    uint16_t textColor = baseColor;

    // 应用呼吸特效到颜色（仅对固定色有效，渐变色不支持呼吸特效）
    bool breatheActive = effectState.upperBreatheActive; // 32x32全屏使用上半屏呼吸状态
    if (breatheActive && !useGradient)
    {
        float phase = effectState.upperBreathePhase;
        float brightness = (sin(phase) + 1.0) / 2.0; // 0.0 到 1.0 的正弦波
        brightness = 0.2 + brightness * 0.8;         // 范围从0.2到1.0，避免完全黑暗

        // 提取RGB分量（RGB565格式）
        uint8_t r = (baseColor >> 11) & 0x1F; // 5位红色
        uint8_t g = (baseColor >> 5) & 0x3F;  // 6位绿色
        uint8_t b = baseColor & 0x1F;         // 5位蓝色

        // 应用呼吸亮度
        r = (uint8_t)(r * brightness);
        g = (uint8_t)(g * brightness);
        b = (uint8_t)(b * brightness);

        // 重新组合颜色
        textColor = (r << 11) | (g << 5) | b;
    }

    // 检查显示方向
    bool isVertical = (textState.displayDirection == BT_DIRECTION_VERTICAL);

    if (scrollActive)
    {
        // 滚动模式：显示所有字符
        uint8_t scrollType = effectState.upperScrollType;
        int scrollOffset = effectState.upperScrollOffset;
        displayScrollingText32x32(font_data, total_char_count, scrollOffset, 0, scrollType);
        return;
    }

    // 32x32字体居中显示和分组切换功能
    // 32x32字体全屏最多显示2个字符（64像素宽度/32像素每字符）
    const int maxCharsPerScreen = SCREEN_WIDTH / CHAR_SPACING_32;

    if (isVertical)
    {
        // 竖向显示：每屏最多显示2个字符
        if (total_char_count <= maxCharsPerScreen)
        {
            // 字符数不超过最大显示数，直接居中显示
            int display_x = (SCREEN_WIDTH - total_char_count * CHAR_SPACING_32) / 2;
            if (display_x < 0)
                display_x = 0;

            if (useGradient)
            {
                drawString32x32VerticalGradient(display_x, 0, font_data, total_char_count, gradientMode);
            }
            else
            {
                drawString32x32Vertical(display_x, 0, font_data, total_char_count, textColor);
            }
        }
        else
        {
            // 超过最大显示数，按组显示
            int currentIndex = textState.upperIndex; // 使用上半屏的索引
            int startCharIndex = currentIndex * maxCharsPerScreen;
            int displayCharCount = min(maxCharsPerScreen, total_char_count - startCharIndex);

            if (displayCharCount > 0)
            {
                // 计算居中位置
                int display_x = (SCREEN_WIDTH - displayCharCount * CHAR_SPACING_32) / 2;
                if (display_x < 0)
                    display_x = 0;

                const uint16_t *current_font_data = font_data + (startCharIndex * 64);
                if (useGradient)
                {
                    drawString32x32VerticalGradient(display_x, 0, current_font_data, displayCharCount, gradientMode);
                }
                else
                {
                    drawString32x32Vertical(display_x, 0, current_font_data, displayCharCount, textColor);
                }
            }
        }
    }
    else
    {
        // 水平显示：每屏最多显示2个字符
        if (total_char_count <= maxCharsPerScreen)
        {
            // 字符数不超过最大显示数，直接居中显示
            int x = (SCREEN_WIDTH - total_char_count * CHAR_SPACING_32) / 2;
            if (x < 0)
                x = 0;

            if (useGradient)
            {
                drawString32x32Gradient(x, 0, font_data, total_char_count, gradientMode);
            }
            else
            {
                drawString32x32(x, 0, font_data, total_char_count, textColor);
            }
        }
        else
        {
            // 超过最大显示数，按组显示
            int currentIndex = textState.upperIndex; // 使用上半屏的索引
            int startCharIndex = currentIndex * maxCharsPerScreen;
            int displayCharCount = min(maxCharsPerScreen, total_char_count - startCharIndex);

            if (displayCharCount > 0)
            {
                // 计算居中位置
                int x = (SCREEN_WIDTH - displayCharCount * CHAR_SPACING_32) / 2;
                if (x < 0)
                    x = 0;

                const uint16_t *current_font_data = font_data + (startCharIndex * 64);
                if (useGradient)
                {
                    drawString32x32Gradient(x, 0, current_font_data, displayCharCount, gradientMode);
                }
                else
                {
                    drawString32x32(x, 0, current_font_data, displayCharCount, textColor);
                }
            }
        }
    }
}

// 处理点阵数据命令（新的函数签名）
void handleTextCommand(const uint16_t *upperData, int upperCharCount, const uint16_t *lowerData, int lowerCharCount)
{
    Serial.printf("设置点阵数据 - 上半屏: %d字符, 下半屏: %d字符\n", upperCharCount, lowerCharCount);

    // 释放旧的动态数据
    if (dynamic_upper_text)
    {
        free(dynamic_upper_text);
        dynamic_upper_text = nullptr;
    }
    if (dynamic_lower_text)
    {
        free(dynamic_lower_text);
        dynamic_lower_text = nullptr;
    }

    // 分配并复制上半屏数据
    if (upperData && upperCharCount > 0)
    {
        int upperDataSize = upperCharCount * 16 * sizeof(uint16_t); // 16x16字体每字符16个uint16_t
        dynamic_upper_text = (uint16_t *)malloc(upperDataSize);
        if (dynamic_upper_text)
        {
            memcpy(dynamic_upper_text, upperData, upperDataSize);
            dynamic_upper_char_count = upperCharCount;
            Serial.printf("上半屏数据已存储: %d字符, %d字节\n", upperCharCount, upperDataSize);
        }
        else
        {
            Serial.println("错误: 上半屏数据内存分配失败");
            dynamic_upper_char_count = 0;
        }
    }
    else
    {
        dynamic_upper_char_count = 0;
    }

    // 分配并复制下半屏数据
    if (lowerData && lowerCharCount > 0)
    {
        int lowerDataSize = lowerCharCount * 16 * sizeof(uint16_t); // 16x16字体每字符16个uint16_t
        dynamic_lower_text = (uint16_t *)malloc(lowerDataSize);
        if (dynamic_lower_text)
        {
            memcpy(dynamic_lower_text, lowerData, lowerDataSize);
            dynamic_lower_char_count = lowerCharCount;
            Serial.printf("下半屏数据已存储: %d字符, %d字节\n", lowerCharCount, lowerDataSize);
        }
        else
        {
            Serial.println("错误: 下半屏数据内存分配失败");
            dynamic_lower_char_count = 0;
        }
    }
    else
    {
        dynamic_lower_char_count = 0;
    }

    // 更新显示状态
    textState.upperIndex = 0;
    textState.lowerIndex = 0;
    textState.lastSwitchTime = millis();
    textState.needUpdate = true;
}

// 处理32x32全屏点阵数据命令
void handleFullScreenTextCommand(const uint16_t *fontData, int charCount)
{
    Serial.printf("设置32x32全屏点阵数据: %d字符\n", charCount);

    // 释放旧的动态数据
    if (dynamic_full_text)
    {
        free(dynamic_full_text);
        dynamic_full_text = nullptr;
    }

    // 分配并复制全屏数据
    if (fontData && charCount > 0)
    {
        int dataSize = charCount * 64 * sizeof(uint16_t); // 32x32字体每字符64个uint16_t
        dynamic_full_text = (uint16_t *)malloc(dataSize);
        if (dynamic_full_text)
        {
            memcpy(dynamic_full_text, fontData, dataSize);
            dynamic_full_char_count = charCount;
            Serial.printf("全屏数据已存储: %d字符, %d字节\n", charCount, dataSize);
        }
        else
        {
            Serial.println("错误: 全屏数据内存分配失败");
            dynamic_full_char_count = 0;
        }
    }
    else
    {
        dynamic_full_char_count = 0;
    }

    // 更新显示状态
    textState.upperIndex = 0;
    textState.lastSwitchTime = millis();
    textState.needUpdate = true;
}

// 处理显示方向命令
void handleDirectionCommand(uint8_t direction)
{
    Serial.printf("设置显示方向: %s\n",
                  (direction == BT_DIRECTION_HORIZONTAL) ? "正向显示" : "竖向显示");

    // 更新显示方向状态
    textState.displayDirection = direction;
    textState.needUpdate = true; // 触发重绘
}

// 更新文本显示
void updateTextDisplay()
{
    unsigned long currentTime = millis();
    const unsigned long switchInterval = 2000; // 2秒切换间隔

    // 32x32字体使用简化逻辑
    if (currentFontSize == BT_FONT_32x32)
    {
        // 32x32字体分组切换逻辑 (仿照16x16逻辑)
        const int maxCharsPerScreen = SCREEN_WIDTH / CHAR_SPACING_32; // 每屏最多2个字符

        if (currentTime - textState.lastSwitchTime >= switchInterval)
        {
            // 获取字符总数（优先使用动态数据）
            int totalChars = (dynamic_full_text && dynamic_full_char_count > 0)
                                 ? dynamic_full_char_count
                                 : getFullTextCharCount();
            if (totalChars > maxCharsPerScreen)
            {
                int totalGroups = (totalChars + maxCharsPerScreen - 1) / maxCharsPerScreen;
                textState.upperIndex++;
                if (textState.upperIndex >= totalGroups)
                {
                    textState.upperIndex = 0;
                }
                textState.lastSwitchTime = currentTime;
                textState.needUpdate = true; // 关键：设置needUpdate标志！
            }
        }

        if (!textState.needUpdate && !colorState.needColorUpdate)
        {
            return;
        }

        // 清除屏幕并填充背景色
        dma_display->clearScreen();
        if (colorState.upperBackgroundColor != 0x0000)
        {
            for (int y = 0; y < SCREEN_HEIGHT; y++)
            {
                for (int x = 0; x < SCREEN_WIDTH; x++)
                {
                    dma_display->drawPixel(x, y, colorState.upperBackgroundColor);
                }
            }
        }

        // 显示32x32全屏文本
        displayFullScreenText32x32();
        textState.needUpdate = false;
        colorState.needColorUpdate = false; // 重置颜色更新标志
        return;
    }

    // 16x16字体的原有逻辑
    // 根据显示方向确定每组最大字符数
    bool isVertical = (textState.displayDirection == BT_DIRECTION_VERTICAL);
    const int maxCharsPerGroup = 4; // 无论水平还是竖向都是每组4个字符

    // 检查是否需要切换显示内容
    if (currentTime - textState.lastSwitchTime >= switchInterval)
    {
        bool needUpdate = false;

        // 处理上半屏分组切换（优先使用动态数据）
        int upperTotalChars = (dynamic_upper_text && dynamic_upper_char_count > 0)
                                  ? dynamic_upper_char_count
                                  : getUpperTextCharCount();
        if (upperTotalChars > maxCharsPerGroup)
        {
            int upperTotalGroups = (upperTotalChars + maxCharsPerGroup - 1) / maxCharsPerGroup; // 向上取整
            textState.upperIndex++;
            if (textState.upperIndex >= upperTotalGroups)
            {
                textState.upperIndex = 0;
            }
            needUpdate = true;
        }

        // 处理下半屏分组切换（优先使用动态数据）
        int lowerTotalChars = (dynamic_lower_text && dynamic_lower_char_count > 0)
                                  ? dynamic_lower_char_count
                                  : getLowerTextCharCount();
        if (lowerTotalChars > maxCharsPerGroup)
        {
            int lowerTotalGroups = (lowerTotalChars + maxCharsPerGroup - 1) / maxCharsPerGroup; // 向上取整
            textState.lowerIndex++;
            if (textState.lowerIndex >= lowerTotalGroups)
            {
                textState.lowerIndex = 0;
            }
            needUpdate = true;
        }

        if (needUpdate)
        {
            textState.lastSwitchTime = currentTime;
            textState.needUpdate = true;
        }
    }

    if (!textState.needUpdate)
    {
        return;
    }

    // 清除屏幕
    dma_display->clearScreen();

    // 分别填充上下半屏背景色
    // 上半屏背景（Y坐标0-15）
    if (colorState.upperBackgroundColor != 0x0000)
    {
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < SCREEN_WIDTH; x++)
            {
                dma_display->drawPixel(x, y, colorState.upperBackgroundColor);
            }
        }
    }

    // 下半屏背景（Y坐标16-31）
    if (colorState.lowerBackgroundColor != 0x0000)
    {
        for (int y = 16; y < 32; y++)
        {
            for (int x = 0; x < SCREEN_WIDTH; x++)
            {
                dma_display->drawPixel(x, y, colorState.lowerBackgroundColor);
            }
        }
    }

    // 显示上半屏文本（Y坐标0）
    displayTextOnHalf(0, true);

    // 显示下半屏文本（Y坐标16）
    displayTextOnHalf(16, false);

    textState.needUpdate = false;
}

// ==================== 颜色相关函数 ====================
// 处理颜色命令
void handleColorCommand(const BluetoothFrame &frame)
{
    if (frame.dataLength < BT_COLOR_DATA_LEN)
    {
        Serial.printf("错误: 颜色数据长度不足，需要%d字节，收到%d字节\n", BT_COLOR_DATA_LEN, frame.dataLength);
        return;
    }

    // 使用getColorData方法解析颜色数据
    uint8_t screenArea, target, mode, r, g, b, gradientMode;
    frame.getColorData(screenArea, target, mode, r, g, b, gradientMode);

    // 验证参数范围
    if (screenArea < BT_SCREEN_UPPER || screenArea > BT_SCREEN_BOTH)
    {
        Serial.printf("错误: 无效的屏幕区域 0x%02X\n", screenArea);
        return;
    }

    if (target != BT_COLOR_TARGET_TEXT && target != BT_COLOR_TARGET_BACKGROUND)
    {
        Serial.printf("错误: 无效的目标类型 0x%02X\n", target);
        return;
    }

    // ⚠️ 重要限制：背景不可设置渐变色
    if (target == BT_COLOR_TARGET_BACKGROUND && mode == BT_COLOR_MODE_GRADIENT)
    {
        Serial.println("错误: 背景不支持渐变色，只有文本可以设置渐变色");
        return;
    }

    const char *areaName = (screenArea == BT_SCREEN_UPPER) ? "上半屏" : (screenArea == BT_SCREEN_LOWER) ? "下半屏"
                                                                    : (screenArea == BT_SCREEN_BOTH)    ? "全屏"
                                                                                                        : "未知区域";
    const char *targetName = (target == BT_COLOR_TARGET_TEXT) ? "文本" : "背景";
    const char *modeName = (mode == BT_COLOR_MODE_FIXED) ? "固定色" : "渐变色";

    Serial.printf("设置颜色 - 区域: %s, 目标: %s, 模式: %s, RGB: (%d,%d,%d), 渐变模式: 0x%02X\n",
                  areaName, targetName, modeName, r, g, b, gradientMode);

    // 检查是否为取消渐变色命令（渐变模式为0x00）
    if (gradientMode == 0x00 && target == BT_COLOR_TARGET_TEXT && mode == BT_COLOR_MODE_GRADIENT)
    {
        Serial.println("取消渐变色，恢复最近一次固定色设置");

        if (screenArea == BT_SCREEN_UPPER || screenArea == BT_SCREEN_BOTH)
        {
            colorState.upperTextMode = BT_COLOR_MODE_FIXED;
            colorState.upperGradientMode = BT_GRADIENT_FIXED;

            // 检查是否有记录的RGB值，如果有则恢复，否则使用白色
            if (colorState.upperTextR != 0 || colorState.upperTextG != 0 || colorState.upperTextB != 0)
            {
                colorState.upperTextColor = rgb888to565(colorState.upperTextR, colorState.upperTextG, colorState.upperTextB);
                Serial.printf("上半屏恢复到最近颜色: RGB(%d,%d,%d)\n",
                              colorState.upperTextR, colorState.upperTextG, colorState.upperTextB);
            }
            else
            {
                colorState.upperTextColor = COLOR_WHITE;
                colorState.upperTextR = 255;
                colorState.upperTextG = 255;
                colorState.upperTextB = 255;
                Serial.println("上半屏无历史颜色记录，恢复默认白色");
            }
        }

        if (screenArea == BT_SCREEN_LOWER || screenArea == BT_SCREEN_BOTH)
        {
            colorState.lowerTextMode = BT_COLOR_MODE_FIXED;
            colorState.lowerGradientMode = BT_GRADIENT_FIXED;

            // 检查是否有记录的RGB值，如果有则恢复，否则使用白色
            if (colorState.lowerTextR != 0 || colorState.lowerTextG != 0 || colorState.lowerTextB != 0)
            {
                colorState.lowerTextColor = rgb888to565(colorState.lowerTextR, colorState.lowerTextG, colorState.lowerTextB);
                Serial.printf("下半屏恢复到最近颜色: RGB(%d,%d,%d)\n",
                              colorState.lowerTextR, colorState.lowerTextG, colorState.lowerTextB);
            }
            else
            {
                colorState.lowerTextColor = COLOR_WHITE;
                colorState.lowerTextR = 255;
                colorState.lowerTextG = 255;
                colorState.lowerTextB = 255;
                Serial.println("下半屏无历史颜色记录，恢复默认白色");
            }
        }

        if (screenArea == BT_SCREEN_BOTH)
        {
            colorState.textMode = BT_COLOR_MODE_FIXED;
            colorState.gradientMode = BT_GRADIENT_FIXED;

            // 检查是否有记录的RGB值，如果有则恢复，否则使用白色
            if (colorState.textR != 0 || colorState.textG != 0 || colorState.textB != 0)
            {
                colorState.textColor = rgb888to565(colorState.textR, colorState.textG, colorState.textB);
                Serial.printf("全屏恢复到最近颜色: RGB(%d,%d,%d)\n",
                              colorState.textR, colorState.textG, colorState.textB);
            }
            else
            {
                colorState.textColor = COLOR_WHITE;
                colorState.textR = 255;
                colorState.textG = 255;
                colorState.textB = 255;
                Serial.println("全屏无历史颜色记录，恢复默认白色");
            }
        }

        colorState.needColorUpdate = true;
        textState.needUpdate = true;
        return; // 直接返回，不执行后续的颜色设置逻辑
    }

    // 根据屏幕区域设置颜色
    if (screenArea == BT_SCREEN_UPPER)
    {
        // 上半屏颜色设置
        if (target == BT_COLOR_TARGET_TEXT)
        {
            colorState.upperTextR = r;
            colorState.upperTextG = g;
            colorState.upperTextB = b;
            colorState.upperTextMode = mode;
            colorState.upperGradientMode = gradientMode;
            if (mode == BT_COLOR_MODE_FIXED)
            {
                colorState.upperTextColor = rgb888to565(r, g, b);
            }
        }
        else if (target == BT_COLOR_TARGET_BACKGROUND)
        {
            colorState.upperBgR = r;
            colorState.upperBgG = g;
            colorState.upperBgB = b;
            colorState.upperBgMode = mode;
            if (mode == BT_COLOR_MODE_FIXED)
            {
                colorState.upperBackgroundColor = rgb888to565(r, g, b);
            }
        }
    }
    else if (screenArea == BT_SCREEN_LOWER)
    {
        // 下半屏颜色设置
        if (target == BT_COLOR_TARGET_TEXT)
        {
            colorState.lowerTextR = r;
            colorState.lowerTextG = g;
            colorState.lowerTextB = b;
            colorState.lowerTextMode = mode;
            colorState.lowerGradientMode = gradientMode;
            if (mode == BT_COLOR_MODE_FIXED)
            {
                colorState.lowerTextColor = rgb888to565(r, g, b);
            }
        }
        else if (target == BT_COLOR_TARGET_BACKGROUND)
        {
            colorState.lowerBgR = r;
            colorState.lowerBgG = g;
            colorState.lowerBgB = b;
            colorState.lowerBgMode = mode;
            if (mode == BT_COLOR_MODE_FIXED)
            {
                colorState.lowerBackgroundColor = rgb888to565(r, g, b);
            }
        }
    }
    else if (screenArea == BT_SCREEN_BOTH)
    {
        // 全屏颜色设置（同时设置上下半屏）
        if (target == BT_COLOR_TARGET_TEXT)
        {
            // 设置全屏文本颜色
            colorState.textR = r;
            colorState.textG = g;
            colorState.textB = b;
            colorState.textMode = mode;
            colorState.gradientMode = gradientMode;
            // 同时设置上下半屏
            colorState.upperTextR = r;
            colorState.upperTextG = g;
            colorState.upperTextB = b;
            colorState.upperTextMode = mode;
            colorState.upperGradientMode = gradientMode;
            colorState.lowerTextR = r;
            colorState.lowerTextG = g;
            colorState.lowerTextB = b;
            colorState.lowerTextMode = mode;
            colorState.lowerGradientMode = gradientMode;

            if (mode == BT_COLOR_MODE_FIXED)
            {
                uint16_t color = rgb888to565(r, g, b);
                colorState.textColor = color;
                colorState.upperTextColor = color;
                colorState.lowerTextColor = color;
            }
        }
        else if (target == BT_COLOR_TARGET_BACKGROUND)
        {
            // 设置全屏背景颜色
            colorState.bgR = r;
            colorState.bgG = g;
            colorState.bgB = b;
            colorState.bgMode = mode;
            // 同时设置上下半屏
            colorState.upperBgR = r;
            colorState.upperBgG = g;
            colorState.upperBgB = b;
            colorState.upperBgMode = mode;
            colorState.lowerBgR = r;
            colorState.lowerBgG = g;
            colorState.lowerBgB = b;
            colorState.lowerBgMode = mode;

            if (mode == BT_COLOR_MODE_FIXED)
            {
                uint16_t color = rgb888to565(r, g, b);
                colorState.backgroundColor = color;
                colorState.upperBackgroundColor = color;
                colorState.lowerBackgroundColor = color;
            }
        }
    }

    colorState.needColorUpdate = true;
    textState.needUpdate = true; // 触发文本重绘
}

// 更新颜色状态
void updateColors()
{
    if (!colorState.needColorUpdate)
    {
        return;
    }
}

// ==================== 亮度相关函数 ====================
// 处理亮度命令
void handleBrightnessCommand(const BluetoothFrame &frame)
{
    uint8_t brightness = frame.getBrightnessData();

    Serial.printf("设置亮度: %d (%.1f%%)\n", brightness, brightness * 100.0 / 255.0);

    // 更新亮度状态
    brightnessState.brightness = brightness;
    brightnessState.needBrightnessUpdate = true;

    // 立即应用亮度设置
    updateBrightness();
}

// 更新亮度设置
void updateBrightness()
{
    if (brightnessState.needBrightnessUpdate)
    {
        // 设置LED矩阵亮度
        dma_display->setBrightness8(brightnessState.brightness);

        Serial.printf("亮度已更新: %d (%.1f%%)\n",
                      brightnessState.brightness,
                      brightnessState.brightness * 100.0 / 255.0);

        brightnessState.needBrightnessUpdate = false;
        textState.needUpdate = true; // 触发重绘以应用新亮度
    }
}

// 处理特效命令
void handleEffectCommand(const BluetoothFrame &frame)
{
    if (frame.dataLength < BT_EFFECT_DATA_LEN)
    {
        Serial.printf("错误: 特效数据长度不足，需要%d字节，收到%d字节\n", BT_EFFECT_DATA_LEN, frame.dataLength);
        return;
    }

    uint8_t screenArea, effectType, speed;
    frame.getEffectData(screenArea, effectType, speed);

    // 验证参数范围
    if (speed < 1 || speed > 10)
    {
        Serial.printf("警告: 特效速度超出建议范围(1-10)，当前值: %d\n", speed);
    }

    const char *areaName = (screenArea == BT_SCREEN_UPPER) ? "上半屏" : (screenArea == BT_SCREEN_LOWER) ? "下半屏"
                                                                    : (screenArea == BT_SCREEN_BOTH)    ? "全屏"
                                                                                                        : "未知区域";

    const char *effectName = (effectType == BT_EFFECT_FIXED) ? "固定显示" : (effectType == BT_EFFECT_SCROLL_LEFT) ? "左移滚动"
                                                                        : (effectType == BT_EFFECT_SCROLL_RIGHT)  ? "右移滚动"
                                                                        : (effectType == BT_EFFECT_BLINK)         ? "闪烁特效"
                                                                        : (effectType == BT_EFFECT_BREATHE)       ? "呼吸特效"
                                                                        : (effectType == BT_EFFECT_SCROLL_UP)     ? "向上滚动"
                                                                        : (effectType == BT_EFFECT_SCROLL_DOWN)   ? "向下滚动"
                                                                                                                  : "未知特效";

    Serial.printf("处理特效命令 - 区域: %s, 特效: %s, 速度: %d\n", areaName, effectName, speed);

    bool isUpper = (screenArea == BT_SCREEN_UPPER || screenArea == BT_SCREEN_BOTH);
    bool isLower = (screenArea == BT_SCREEN_LOWER || screenArea == BT_SCREEN_BOTH);

    // 应用特效到指定区域
    if (isUpper)
    {
        clearAllEffects(true);
        switch (effectType)
        {
        case BT_EFFECT_SCROLL_LEFT:
        case BT_EFFECT_SCROLL_RIGHT:
        case BT_EFFECT_SCROLL_UP:
        case BT_EFFECT_SCROLL_DOWN:
            setScrollEffect(true, effectType, speed);
            break;
        case BT_EFFECT_BLINK:
            setBlinkEffect(true, speed);
            break;
        case BT_EFFECT_BREATHE:
            setBreatheEffect(true, speed);
            break;
        case BT_EFFECT_FIXED:
        default:
            Serial.println("上半屏特效已清除（固定显示）");
            break;
        }
    }

    if (isLower)
    {
        clearAllEffects(false);
        switch (effectType)
        {
        case BT_EFFECT_SCROLL_LEFT:
        case BT_EFFECT_SCROLL_RIGHT:
        case BT_EFFECT_SCROLL_UP:
        case BT_EFFECT_SCROLL_DOWN:
            setScrollEffect(false, effectType, speed);
            break;
        case BT_EFFECT_BLINK:
            setBlinkEffect(false, speed);
            break;
        case BT_EFFECT_BREATHE:
            setBreatheEffect(false, speed);
            break;
        case BT_EFFECT_FIXED:
        default:
            Serial.println("下半屏特效已清除（固定显示）");
            break;
        }
    }
}

// ==================== 特效相关函数 ====================
// 清除指定半屏的所有特效（实现互斥限制）
void clearAllEffects(bool isUpper)
{
    if (isUpper)
    {
        // 清除上半屏所有特效
        effectState.upperScrollActive = false;
        effectState.upperBlinkActive = false;
        effectState.upperBreatheActive = false;
        Serial.println("已清除上半屏所有特效");
    }
    else
    {
        // 清除下半屏所有特效
        effectState.lowerScrollActive = false;
        effectState.lowerBlinkActive = false;
        effectState.lowerBreatheActive = false;
        Serial.println("已清除下半屏所有特效");
    }
    textState.needUpdate = true;
}

// 设置滚动特效（自动清除其他特效）
void setScrollEffect(bool isUpper, uint8_t scrollType, uint8_t speed)
{
    // 先清除该半屏的所有特效
    clearAllEffects(isUpper);

    // 设置滚动特效
    if (isUpper)
    {
        effectState.upperScrollActive = true;
        effectState.upperScrollType = scrollType;
        effectState.upperScrollSpeed = speed;
        effectState.upperScrollOffset = 0;
        const char *scrollName = (scrollType == BT_EFFECT_SCROLL_LEFT) ? "左滚动" : (scrollType == BT_EFFECT_SCROLL_RIGHT) ? "右滚动"
                                                                                : (scrollType == BT_EFFECT_SCROLL_UP)      ? "向上滚动"
                                                                                : (scrollType == BT_EFFECT_SCROLL_DOWN)    ? "向下滚动"
                                                                                                                           : "未知滚动";
        Serial.printf("上半屏启用滚动特效 - 类型: %s, 速度: %d\n", scrollName, speed);
    }
    else
    {
        effectState.lowerScrollActive = true;
        effectState.lowerScrollType = scrollType;
        effectState.lowerScrollSpeed = speed;
        effectState.lowerScrollOffset = 0;
        const char *scrollName = (scrollType == BT_EFFECT_SCROLL_LEFT) ? "左滚动" : (scrollType == BT_EFFECT_SCROLL_RIGHT) ? "右滚动"
                                                                                : (scrollType == BT_EFFECT_SCROLL_UP)      ? "向上滚动"
                                                                                : (scrollType == BT_EFFECT_SCROLL_DOWN)    ? "向下滚动"
                                                                                                                           : "未知滚动";
        Serial.printf("下半屏启用滚动特效 - 类型: %s, 速度: %d\n", scrollName, speed);
    }
    textState.needUpdate = true;
}

// 设置闪烁特效（自动清除其他特效）
void setBlinkEffect(bool isUpper, uint8_t speed)
{
    // 先清除该半屏的所有特效
    clearAllEffects(isUpper);

    // 设置闪烁特效
    if (isUpper)
    {
        effectState.upperBlinkActive = true;
        effectState.upperBlinkSpeed = speed;
        effectState.upperBlinkVisible = true;
        Serial.printf("上半屏启用闪烁特效 - 速度: %d\n", speed);
    }
    else
    {
        effectState.lowerBlinkActive = true;
        effectState.lowerBlinkSpeed = speed;
        effectState.lowerBlinkVisible = true;
        Serial.printf("下半屏启用闪烁特效 - 速度: %d\n", speed);
    }
    textState.needUpdate = true;
}

// 设置呼吸特效（自动清除其他特效）
void setBreatheEffect(bool isUpper, uint8_t speed)
{
    // 先清除该半屏的所有特效
    clearAllEffects(isUpper);

    // 设置呼吸特效
    if (isUpper)
    {
        effectState.upperBreatheActive = true;
        effectState.upperBreatheSpeed = speed;
        effectState.upperBreathePhase = 0.0;
        Serial.printf("上半屏启用呼吸特效 - 速度: %d\n", speed);
    }
    else
    {
        effectState.lowerBreatheActive = true;
        effectState.lowerBreatheSpeed = speed;
        effectState.lowerBreathePhase = 0.0;
        Serial.printf("下半屏启用呼吸特效 - 速度: %d\n", speed);
    }
    textState.needUpdate = true;
}

// 显示滚动文本（支持水平和竖向显示，支持呼吸特效）
void displayScrollingText(const uint16_t *font_data, int char_count, int offset, int y, uint8_t scrollType)
{
    if (char_count == 0)
        return;

    bool isUpper = (y < 16);
    bool isVertical = (textState.displayDirection == BT_DIRECTION_VERTICAL);
    int textPixelWidth = char_count * CHAR_SPACING_16; // 文本总像素宽度
    int x = 0;

    // 根据滚动类型和显示方向计算位置
    if (scrollType == BT_EFFECT_SCROLL_LEFT || scrollType == BT_EFFECT_SCROLL_UP)
    {
        // 左滚动或向上滚动：从右边进入，向左移动
        x = SCREEN_WIDTH - offset;
    }
    else if (scrollType == BT_EFFECT_SCROLL_RIGHT || scrollType == BT_EFFECT_SCROLL_DOWN)
    {
        // 右滚动或向下滚动：从左边进入，向右移动
        x = offset - textPixelWidth;
    }

    // 只在屏幕范围内绘制文本
    if (x < SCREEN_WIDTH && x + textPixelWidth > 0)
    {
        // 检查是否使用渐变色
        uint8_t textMode = isUpper ? colorState.upperTextMode : colorState.lowerTextMode;
        uint8_t gradientMode = isUpper ? colorState.upperGradientMode : colorState.lowerGradientMode;
        bool useGradient = (textMode == BT_COLOR_MODE_GRADIENT && gradientMode != BT_GRADIENT_FIXED);

        if (useGradient)
        {
            // 使用渐变色显示（渐变色不支持呼吸特效）
            if (isVertical)
            {
                drawString16x16VerticalGradient(x, y, font_data, char_count, isUpper, gradientMode);
            }
            else
            {
                drawString16x16Gradient(x, y, font_data, char_count, isUpper, gradientMode);
            }
        }
        else
        {
            // 获取基础颜色
            uint16_t baseColor = isUpper ? colorState.upperTextColor : colorState.lowerTextColor;
            uint16_t textColor = baseColor;

            // 应用呼吸特效（如果激活）
            bool breatheActive = isUpper ? effectState.upperBreatheActive : effectState.lowerBreatheActive;
            if (breatheActive)
            {
                float phase = isUpper ? effectState.upperBreathePhase : effectState.lowerBreathePhase;
                float brightness = (sin(phase) + 1.0) / 2.0; // 0.0 到 1.0 的正弦波
                brightness = 0.2 + brightness * 0.8;         // 范围从0.2到1.0，避免完全黑暗

                // 提取RGB分量（RGB565格式）
                uint8_t r = (baseColor >> 11) & 0x1F; // 5位红色
                uint8_t g = (baseColor >> 5) & 0x3F;  // 6位绿色
                uint8_t b = baseColor & 0x1F;         // 5位蓝色

                // 应用呼吸亮度
                r = (uint8_t)(r * brightness);
                g = (uint8_t)(g * brightness);
                b = (uint8_t)(b * brightness);

                // 重新组合颜色
                textColor = (r << 11) | (g << 5) | b;
            }

            // 根据显示方向使用对应的绘制函数
            if (isVertical)
            {
                drawString16x16Vertical(x, y, font_data, char_count, textColor);
            }
            else
            {
                drawString16x16(x, y, font_data, char_count, textColor);
            }
        }
    }
}

// 显示32x32滚动文本（仿照16x16逻辑）
void displayScrollingText32x32(const uint16_t *font_data, int char_count, int offset, int y, uint8_t scrollType)
{
    if (char_count == 0)
        return;

    bool isVertical = (textState.displayDirection == BT_DIRECTION_VERTICAL);
    int textPixelWidth = char_count * CHAR_SPACING_32; // 32x32文本总像素宽度
    int x = 0;

    // 根据滚动类型和显示方向计算位置
    if (scrollType == BT_EFFECT_SCROLL_LEFT || scrollType == BT_EFFECT_SCROLL_UP)
    {
        // 左滚动或向上滚动：从右边进入，向左移动
        x = SCREEN_WIDTH - offset;
    }
    else if (scrollType == BT_EFFECT_SCROLL_RIGHT || scrollType == BT_EFFECT_SCROLL_DOWN)
    {
        // 右滚动或向下滚动：从左边进入，向右移动
        x = offset - textPixelWidth;
    }

    // 只在屏幕范围内绘制文本
    if (x < SCREEN_WIDTH && x + textPixelWidth > 0)
    {
        // 检查是否使用渐变色
        uint8_t textMode = colorState.textMode;
        uint8_t gradientMode = colorState.gradientMode;
        bool useGradient = (textMode == BT_COLOR_MODE_GRADIENT && gradientMode != BT_GRADIENT_FIXED);

        if (useGradient)
        {
            // 使用渐变色显示（渐变色不支持呼吸特效）
            if (isVertical)
            {
                drawString32x32VerticalGradient(x, y, font_data, char_count, gradientMode);
            }
            else
            {
                drawString32x32Gradient(x, y, font_data, char_count, gradientMode);
            }
        }
        else
        {
            // 获取基础颜色
            uint16_t baseColor = colorState.textColor;
            uint16_t textColor = baseColor;

            // 应用呼吸特效（如果激活）- 32x32使用上半屏呼吸状态
            bool breatheActive = effectState.upperBreatheActive;
            if (breatheActive)
            {
                float phase = effectState.upperBreathePhase;
                float brightness = (sin(phase) + 1.0) / 2.0; // 0.0 到 1.0 的正弦波
                brightness = 0.2 + brightness * 0.8;         // 范围从0.2到1.0，避免完全黑暗

                // 提取RGB分量（RGB565格式）
                uint8_t r = (baseColor >> 11) & 0x1F; // 5位红色
                uint8_t g = (baseColor >> 5) & 0x3F;  // 6位绿色
                uint8_t b = baseColor & 0x1F;         // 5位蓝色

                // 应用呼吸亮度
                r = (uint8_t)(r * brightness);
                g = (uint8_t)(g * brightness);
                b = (uint8_t)(b * brightness);

                // 重新组合颜色
                textColor = (r << 11) | (g << 5) | b;
            }

            // 根据显示方向使用对应的绘制函数
            if (isVertical)
            {
                drawString32x32Vertical(x, y, font_data, char_count, textColor);
            }
            else
            {
                drawString32x32(x, y, font_data, char_count, textColor);
            }
        }
    }
}

// 更新滚动特效（支持速度控制）
void updateScrollEffect()
{
    if (!effectState.upperScrollActive && !effectState.lowerScrollActive)
    {
        return;
    }

    unsigned long currentTime = millis();
    static unsigned long lastUpperScrollTime = 0;
    static unsigned long lastLowerScrollTime = 0;
    bool needUpdate = false;

    // 更新上半屏滚动
    if (effectState.upperScrollActive)
    {
        // 根据速度计算滚动间隔：速度越高间隔越短，移动距离越大
        unsigned long upperScrollInterval = 100 - (effectState.upperScrollSpeed * 8); // 速度0-10对应100ms-20ms
        int upperMoveDistance = (effectState.upperScrollSpeed / 2) + 1;               // 速度0-10对应1-6像素

        if (currentTime - lastUpperScrollTime >= upperScrollInterval)
        {
            int upperCharCount, textPixelWidth;

            // 根据字体大小计算字符数和像素宽度（优先使用动态数据）
            if (currentFontSize == BT_FONT_32x32)
            {
                // 32x32字体：优先使用动态数据
                upperCharCount = (dynamic_full_text && dynamic_full_char_count > 0)
                                     ? dynamic_full_char_count
                                     : getFullTextCharCount();
                textPixelWidth = upperCharCount * CHAR_SPACING_32;
            }
            else
            {
                // 16x16字体：优先使用动态数据
                upperCharCount = (dynamic_upper_text && dynamic_upper_char_count > 0)
                                     ? dynamic_upper_char_count
                                     : getUpperTextCharCount();
                textPixelWidth = upperCharCount * CHAR_SPACING_16;
            }

            int maxOffset = SCREEN_WIDTH + textPixelWidth; // 完全滚出屏幕的偏移量

            effectState.upperScrollOffset += upperMoveDistance;
            if (effectState.upperScrollOffset >= maxOffset)
            {
                effectState.upperScrollOffset = 0; // 重新开始滚动
            }
            lastUpperScrollTime = currentTime;
            needUpdate = true;
        }
    }

    // 更新下半屏滚动
    if (effectState.lowerScrollActive)
    {
        // 根据速度计算滚动间隔：速度越高间隔越短，移动距离越大
        unsigned long lowerScrollInterval = 100 - (effectState.lowerScrollSpeed * 8); // 速度0-10对应100ms-20ms
        int lowerMoveDistance = (effectState.lowerScrollSpeed / 2) + 1;               // 速度0-10对应1-6像素

        if (currentTime - lastLowerScrollTime >= lowerScrollInterval)
        {
            // 下半屏：优先使用动态数据
            int lowerCharCount = (dynamic_lower_text && dynamic_lower_char_count > 0)
                                     ? dynamic_lower_char_count
                                     : getLowerTextCharCount();
            int textPixelWidth = lowerCharCount * CHAR_SPACING_16;
            int maxOffset = SCREEN_WIDTH + textPixelWidth; // 完全滚出屏幕的偏移量

            effectState.lowerScrollOffset += lowerMoveDistance;
            if (effectState.lowerScrollOffset >= maxOffset)
            {
                effectState.lowerScrollOffset = 0; // 重新开始滚动
            }
            lastLowerScrollTime = currentTime;
            needUpdate = true;
        }
    }

    if (needUpdate)
    {
        textState.needUpdate = true;
    }
}

// 更新闪烁特效（支持速度控制）
void updateBlinkEffect()
{
    if (!effectState.upperBlinkActive && !effectState.lowerBlinkActive)
    {
        return;
    }

    unsigned long currentTime = millis();
    static unsigned long lastUpperBlinkTime = 0;
    static unsigned long lastLowerBlinkTime = 0;
    bool needUpdate = false;

    // 上半屏闪烁
    if (effectState.upperBlinkActive)
    {
        // 根据速度计算闪烁间隔：速度越高间隔越短
        unsigned long upperBlinkInterval = 1000 - (effectState.upperBlinkSpeed * 80); // 速度0-10对应1000ms-200ms

        if (currentTime - lastUpperBlinkTime >= upperBlinkInterval)
        {
            effectState.upperBlinkVisible = !effectState.upperBlinkVisible;
            lastUpperBlinkTime = currentTime;
            needUpdate = true;
        }
    }

    // 下半屏闪烁
    if (effectState.lowerBlinkActive)
    {
        // 根据速度计算闪烁间隔：速度越高间隔越短
        unsigned long lowerBlinkInterval = 1000 - (effectState.lowerBlinkSpeed * 80); // 速度0-10对应1000ms-200ms

        if (currentTime - lastLowerBlinkTime >= lowerBlinkInterval)
        {
            effectState.lowerBlinkVisible = !effectState.lowerBlinkVisible;
            lastLowerBlinkTime = currentTime;
            needUpdate = true;
        }
    }

    if (needUpdate)
    {
        textState.needUpdate = true;
    }
}

// 更新呼吸特效（支持速度控制）
void updateBreatheEffect()
{
    if (!effectState.upperBreatheActive && !effectState.lowerBreatheActive)
    {
        return;
    }

    unsigned long currentTime = millis();
    static unsigned long lastBreatheTime = 0;
    const unsigned long breatheInterval = 30; // 30ms更新间隔，保证平滑

    if (currentTime - lastBreatheTime >= breatheInterval)
    {
        bool needUpdate = false;

        // 上半屏呼吸
        if (effectState.upperBreatheActive)
        {
            // 根据速度计算相位增量：速度越高变化越快
            float upperSpeed = (effectState.upperBreatheSpeed + 1) * 0.08; // 速度0-10对应0.08-0.88的相位增量
            effectState.upperBreathePhase += upperSpeed;
            if (effectState.upperBreathePhase >= 2 * PI)
            {
                effectState.upperBreathePhase -= 2 * PI;
            }
            needUpdate = true;
        }

        // 下半屏呼吸
        if (effectState.lowerBreatheActive)
        {
            // 根据速度计算相位增量：速度越高变化越快
            float lowerSpeed = (effectState.lowerBreatheSpeed + 1) * 0.08; // 速度0-10对应0.08-0.88的相位增量
            effectState.lowerBreathePhase += lowerSpeed;
            if (effectState.lowerBreathePhase >= 2 * PI)
            {
                effectState.lowerBreathePhase -= 2 * PI;
            }
            needUpdate = true;
        }

        if (needUpdate)
        {
            lastBreatheTime = currentTime;
            textState.needUpdate = true;
        }
    }
}

// 更新所有特效
void updateAllEffects()
{
    // 分别更新各种特效，每种特效有自己的时间控制
    updateScrollEffect();
    updateBlinkEffect();
    updateBreatheEffect();
}

// ==================== 示例演示函数 ====================
// 演示如何使用蓝牙点阵数据
void demoBluetoothDataUsage()
{
    Serial.println("=== 蓝牙点阵数据使用示例 ===");

    // 示例1：16x16模式 - 传入自定义点阵数据
    if (currentFontSize == BT_FONT_16x16)
    {
        // 假设这是从蓝牙接收到的点阵数据
        uint16_t customUpperData[] = {
            // 自定义上半屏点阵数据 (1个字符，16个uint16_t)
            0x0000, 0x0180, 0x03C0, 0x07E0, 0x0FF0, 0x1FF8, 0x3FFC, 0x7FFE,
            0x7FFE, 0x3FFC, 0x1FF8, 0x0FF0, 0x07E0, 0x03C0, 0x0180, 0x0000};

        uint16_t customLowerData[] = {
            // 自定义下半屏点阵数据 (1个字符，16个uint16_t)
            0xFFFF, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
            0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0xFFFF};

        // 调用新的API传入点阵数据
        handleTextCommand(customUpperData, 1, customLowerData, 1);
        Serial.println("16x16自定义点阵数据已设置");
    }

    // 示例2：32x32模式 - 传入自定义点阵数据
    else if (currentFontSize == BT_FONT_32x32)
    {
        // 假设这是从蓝牙接收到的32x32点阵数据
        uint16_t custom32x32Data[64] = {0}; // 1个字符，64个uint16_t，这里简化为全0
        // 实际使用时这里应该是真正的32x32点阵数据
        for (int i = 10; i < 54; i++)
        {
            custom32x32Data[i] = 0xFFFF; // 创建一个简单的图案
        }

        // 调用新的API传入点阵数据
        handleFullScreenTextCommand(custom32x32Data, 1);
        Serial.println("32x32自定义点阵数据已设置");
    }

    Serial.println("=== 使用说明 ===");
    Serial.println("1. 接收蓝牙数据：uint16_t* bluetoothData, int charCount");
    Serial.println("2. 16x16模式调用：handleTextCommand(upperData, upperCount, lowerData, lowerCount)");
    Serial.println("3. 32x32模式调用：handleFullScreenTextCommand(fontData, charCount)");
    Serial.println("4. 数据会自动显示，无需其他操作");
}