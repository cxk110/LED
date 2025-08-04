#ifndef BLUETOOTH_PROTOCOL_H
#define BLUETOOTH_PROTOCOL_H

#include "Arduino.h"
#include "config.h"

enum class ParseState
{
    WAITING_HEADER1,
    WAITING_HEADER2,
    WAITING_COMMAND,
    WAITING_LENGTH_HIGH,
    WAITING_LENGTH_LOW,
    WAITING_DATA,
    WAITING_TAIL1,
    WAITING_TAIL2,
    FRAME_COMPLETE
};

enum class ParseResult
{
    NEED_MORE_DATA,
    FRAME_COMPLETE,
    FRAME_ERROR,
    INVALID_COMMAND,
    DATA_TOO_LONG
};

struct BluetoothFrame
{
    uint8_t command;
    uint16_t dataLength;
    uint8_t *data;
    bool isValid;
    uint32_t timestamp;              // 接收时间戳
    mutable uint16_t *convertedData; // 缓存转换后的16位数据
    mutable bool isConverted;        // 转换标志

    BluetoothFrame() : command(0), dataLength(0), data(nullptr), isValid(false), timestamp(0), convertedData(nullptr), isConverted(false) {}
    ~BluetoothFrame()
    {
        if (convertedData)
        {
            delete[] convertedData;
            convertedData = nullptr;
        }
    }

    // 数据解析辅助方法
    String getTextData() const;
    void getColorData(uint8_t &screenArea, uint8_t &target, uint8_t &mode, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &gradientMode) const;
    uint8_t getBrightnessData() const;
    void getEffectData(uint8_t &screenArea, uint8_t &type, uint8_t &speed) const;
    bool isValidCommand() const;
    // 字体数据转换方法
    const uint16_t *getFontData16x16(uint8_t &screenArea, int &charCount) const;
    const uint16_t *getFontData32x32(uint8_t &screenArea, int &charCount) const;
};

class BluetoothProtocolParser
{
private:
    ParseState currentState;
    uint8_t command;
    uint16_t dataLength;
    uint16_t dataReceived;
    uint8_t *dataBuffer;
    uint32_t frameStartTime;      // 帧开始时间
    uint32_t errorCount;          // 错误计数
    uint16_t timeoutCheckCounter; // 超时检查计数器
    static const uint16_t MAX_DATA_LENGTH = 8192;
    static const uint32_t FRAME_TIMEOUT_MS = 5000; // 帧超时时间

public:
    BluetoothProtocolParser();
    ~BluetoothProtocolParser();

    ParseResult parseByte(uint8_t byte, BluetoothFrame &frame);
    ParseResult parseBuffer(uint8_t *buffer, size_t length, BluetoothFrame frames[], size_t maxFrames, size_t &frameCount);
    void reset();
    bool isFrameComplete() const;
    bool isFrameTimeout() const;
    uint32_t getErrorCount() const { return errorCount; }
    ParseState getCurrentState() const { return currentState; }

    // 调试功能
    String getStateString() const;
    void printDebugInfo() const;
};

#endif // BLUETOOTH_PROTOCOL_H
