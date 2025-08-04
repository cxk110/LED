#include "BluetoothSerial.h"
#include "bluetooth_protocol.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "DisplayDriver.h"
#include "LEDController.h"
#include "config.h"
#include "FontData.h"

String device_name = "ESP32-BT-Slave";
BluetoothProtocolParser btParser; // 蓝牙协议解析器
BluetoothFrame currentFrame;      // 当前解析的帧

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

// 函数声明
void handleParseResult(ParseResult result);
void processBluetoothCommand(const BluetoothFrame &frame);
void handleTextCommand(const BluetoothFrame &frame);
void handleUpperTextCommand(const uint16_t *fontData, int charCount); // 独立处理上半屏
void handleLowerTextCommand(const uint16_t *fontData, int charCount); // 独立处理下半屏
void handleColorCommand(const BluetoothFrame &frame);

const char *getEffectName(uint8_t type);

void setup()
{
    Serial.begin(115200);

    // 初始化显示硬件
    if (!initializeDisplay())
    {
        Serial.println("显示硬件初始化失败！");
        return;
    }

    Serial.println("=== ESP32 LED屏控制器 ===");
    Serial.println("硬件初始化完成");

    // 启动蓝牙串口
    SerialBT.begin(device_name);
    Serial.printf("蓝牙设备已启动，设备名: %s\n", device_name.c_str());
    Serial.println("可以配对连接了");

    // 根据字体大小设置初始点阵数据（使用FontData中的示例数据）
    if (currentFontSize == BT_FONT_32x32)
    {
        handleFullScreenTextCommand(full_text, getFullTextCharCount());
    }
    else
    {
        handleTextCommand(upper_text, getUpperTextCharCount(), lower_text, getLowerTextCharCount());
    }
}

void loop()
{
    // 处理蓝牙数据接收
    while (SerialBT.available())
    {
        uint8_t receivedByte = SerialBT.read();
        ParseResult result = btParser.parseByte(receivedByte, currentFrame);

        if (result != ParseResult::NEED_MORE_DATA)
        {
            handleParseResult(result);
        }
    }

    updateAllEffects();  // 更新所有特效
    updateBrightness();  // 更新亮度设置
    updateColors();      // 更新颜色状态
    updateTextDisplay(); // 更新文本显示
}

// 处理解析结果
void handleParseResult(ParseResult result)
{
    switch (result)
    {
    case ParseResult::FRAME_COMPLETE:
        Serial.printf("收到完整帧 - 命令: 0x%02X, 数据长度: %d\n",
                      currentFrame.command, currentFrame.dataLength);
        processBluetoothCommand(currentFrame);
        btParser.reset(); // 重置解析器准备下一帧
        break;

    case ParseResult::FRAME_ERROR:
        Serial.println("错误: 帧格式错误");
        break;

    case ParseResult::INVALID_COMMAND:
        Serial.println("错误: 无效命令");
        break;

    case ParseResult::DATA_TOO_LONG:
        Serial.println("错误: 数据过长");
        break;

    case ParseResult::NEED_MORE_DATA:
        // 继续等待更多数据，无需处理
        break;

    default:
        Serial.printf("未知解析结果: %d\n", (int)result);
        break;
    }
}

// 处理蓝牙命令
void processBluetoothCommand(const BluetoothFrame &frame)
{
    if (!frame.isValidCommand())
    {
        Serial.println("错误: 无效的命令帧");
        return;
    }

    switch (frame.command)
    {
    case BT_CMD_SET_DIRECTION: // 0x00
        handleDirectionCommand(BT_DIRECTION_HORIZONTAL);
        Serial.println("设置文本显示方向: 正向显示");
        break;

    case BT_CMD_SET_VERTICAL: // 0x01
        handleDirectionCommand(BT_DIRECTION_VERTICAL);
        Serial.println("设置文本显示方向: 竖向显示");
        break;

    case BT_CMD_SET_FONT_16x16: // 0x02
        currentFontSize = BT_FONT_16x16;
        Serial.println("设置字体: 16x16");
        break;

    case BT_CMD_SET_FONT_32x32: // 0x03
        currentFontSize = BT_FONT_32x32;
        Serial.println("设置字体: 32x32");
        break;

    case BT_CMD_SET_TEXT: // 0x04
        handleTextCommand(frame);
        break;

    case BT_CMD_SET_COLOR: // 0x06
        handleColorCommand(frame);
        break;

    case BT_CMD_SET_BRIGHTNESS: // 0x07
        handleBrightnessCommand(frame);
        break;

    case BT_CMD_SET_EFFECT: // 0x08
        handleEffectCommand(frame);
        break;

    default:
        Serial.printf("未支持的命令: 0x%02X\n", frame.command);
        break;
    }
}

// 处理文本命令 (0x04)
void handleTextCommand(const BluetoothFrame &frame)
{
    if (currentFontSize == BT_FONT_32x32)
    {
        // 32x32字体处理
        uint8_t screenArea;
        int charCount;
        const uint16_t *fontData = frame.getFontData32x32(screenArea, charCount);

        if (fontData && charCount > 0)
        {
            Serial.printf("处理32x32文本命令 - 屏幕区域: 0x%02X, 字符数: %d\n", screenArea, charCount);
            handleFullScreenTextCommand(fontData, charCount);
        }
        else
        {
            Serial.println("错误: 32x32字体数据无效");
        }
    }
    else
    {
        // 16x16字体处理
        uint8_t screenArea;
        int charCount;
        const uint16_t *fontData = frame.getFontData16x16(screenArea, charCount);

        if (fontData && charCount > 0)
        {
            Serial.printf("处理16x16文本命令 - 屏幕区域: 0x%02X, 字符数: %d\n", screenArea, charCount);

            switch (screenArea)
            {
            case BT_SCREEN_UPPER: // 上半屏
                handleUpperTextCommand(fontData, charCount);
                break;
            case BT_SCREEN_LOWER: // 下半屏
                handleLowerTextCommand(fontData, charCount);
                break;
            case BT_SCREEN_BOTH: // 全屏 (分为上下两部分)
            {
                int halfCount = charCount / 2;
                const uint16_t *upperData = fontData;
                const uint16_t *lowerData = fontData + (halfCount * 16); // 每个字符16个uint16_t
                handleTextCommand(upperData, halfCount, lowerData, charCount - halfCount);
            }
            break;
            default:
                Serial.printf("错误: 无效的屏幕区域 0x%02X\n", screenArea);
                break;
            }
        }
        else
        {
            Serial.println("错误: 16x16字体数据无效");
        }
    }
}

// 独立处理上半屏文本（保持下半屏不变）
void handleUpperTextCommand(const uint16_t *fontData, int charCount)
{
    if (!fontData || charCount <= 0)
        return;

    Serial.printf("设置上半屏文本 - 字符数: %d\n", charCount);

    // 只释放上半屏数据
    if (dynamic_upper_text)
    {
        free(dynamic_upper_text);
        dynamic_upper_text = nullptr;
    }

    // 分配并复制上半屏数据
    int upperDataSize = charCount * 16 * sizeof(uint16_t); // 16x16字体每字符16个uint16_t
    dynamic_upper_text = (uint16_t *)malloc(upperDataSize);
    if (dynamic_upper_text)
    {
        memcpy(dynamic_upper_text, fontData, upperDataSize);
        dynamic_upper_char_count = charCount;
        Serial.printf("上半屏数据已更新: %d字符, %d字节\n", charCount, upperDataSize);
    }
    else
    {
        Serial.println("错误: 上半屏数据内存分配失败");
        dynamic_upper_char_count = 0;
    }

    // 更新显示状态（只重置上半屏索引）
    textState.upperIndex = 0;
    textState.lastSwitchTime = millis();
    textState.needUpdate = true;
}

// 独立处理下半屏文本（保持上半屏不变）
void handleLowerTextCommand(const uint16_t *fontData, int charCount)
{
    if (!fontData || charCount <= 0)
        return;

    Serial.printf("设置下半屏文本 - 字符数: %d\n", charCount);

    // 只释放下半屏数据
    if (dynamic_lower_text)
    {
        free(dynamic_lower_text);
        dynamic_lower_text = nullptr;
    }

    // 分配并复制下半屏数据
    int lowerDataSize = charCount * 16 * sizeof(uint16_t); // 16x16字体每字符16个uint16_t
    dynamic_lower_text = (uint16_t *)malloc(lowerDataSize);
    if (dynamic_lower_text)
    {
        memcpy(dynamic_lower_text, fontData, lowerDataSize);
        dynamic_lower_char_count = charCount;
        Serial.printf("下半屏数据已更新: %d字符, %d字节\n", charCount, lowerDataSize);
    }
    else
    {
        Serial.println("错误: 下半屏数据内存分配失败");
        dynamic_lower_char_count = 0;
    }

    // 更新显示状态（只重置下半屏索引）
    textState.lowerIndex = 0;
    textState.lastSwitchTime = millis();
    textState.needUpdate = true;
}
