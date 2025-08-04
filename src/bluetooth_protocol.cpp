#include "bluetooth_protocol.h"

BluetoothProtocolParser::BluetoothProtocolParser()
{
    dataBuffer = new uint8_t[MAX_DATA_LENGTH];
    errorCount = 0;
    timeoutCheckCounter = 0;
    reset();
}

BluetoothProtocolParser::~BluetoothProtocolParser()
{
    delete[] dataBuffer;
}

void BluetoothProtocolParser::reset()
{
    currentState = ParseState::WAITING_HEADER1;
    command = 0;
    dataLength = 0;
    dataReceived = 0;
    timeoutCheckCounter = 0;
    frameStartTime = millis();
}

bool BluetoothProtocolParser::isFrameComplete() const
{
    return currentState == ParseState::FRAME_COMPLETE;
}

ParseResult BluetoothProtocolParser::parseByte(uint8_t byte, BluetoothFrame &frame)
{
    // 优化：每100字节检查一次超时，减少millis()调用开销
    if (++timeoutCheckCounter >= 100)
    {
        timeoutCheckCounter = 0;
        if (isFrameTimeout())
        {
            errorCount++;
            reset();
            return ParseResult::FRAME_ERROR;
        }
    }

    switch (currentState)
    {
    case ParseState::WAITING_HEADER1:
        if (byte == BT_FRAME_HEADER_1)
        {
            currentState = ParseState::WAITING_HEADER2;
            frameStartTime = millis();
        }
        break;

    case ParseState::WAITING_HEADER2:
        if (byte == BT_FRAME_HEADER_2)
        {
            currentState = ParseState::WAITING_COMMAND;
        }
        else
        {
            errorCount++;
            reset();
            return ParseResult::FRAME_ERROR;
        }
        break;

    case ParseState::WAITING_COMMAND:
        if (byte >= BT_CMD_SET_DIRECTION && byte <= BT_CMD_SET_EFFECT)
        {
            command = byte;
            currentState = ParseState::WAITING_LENGTH_HIGH;
        }
        else
        {
            errorCount++;
            reset();
            return ParseResult::INVALID_COMMAND;
        }
        break;

    case ParseState::WAITING_LENGTH_HIGH:
        dataLength = (uint16_t)byte << 8;
        currentState = ParseState::WAITING_LENGTH_LOW;
        break;

    case ParseState::WAITING_LENGTH_LOW:
        dataLength |= byte;
        dataReceived = 0;

        if (dataLength > MAX_DATA_LENGTH)
        {
            errorCount++;
            reset();
            return ParseResult::DATA_TOO_LONG;
        }

        if (dataLength == 0)
        {
            currentState = ParseState::WAITING_TAIL1;
        }
        else
        {
            currentState = ParseState::WAITING_DATA;
        }
        break;

    case ParseState::WAITING_DATA:
        dataBuffer[dataReceived++] = byte;

        if (dataReceived >= dataLength)
        {
            currentState = ParseState::WAITING_TAIL1;
        }
        break;

    case ParseState::WAITING_TAIL1:
        if (byte == BT_FRAME_TAIL_1)
        {
            currentState = ParseState::WAITING_TAIL2;
        }
        else
        {
            errorCount++;
            reset();
            return ParseResult::FRAME_ERROR;
        }
        break;

    case ParseState::WAITING_TAIL2:
        if (byte == BT_FRAME_TAIL_2)
        {
            frame.command = command;
            frame.dataLength = dataLength;
            frame.data = dataBuffer;
            frame.isValid = true;
            frame.timestamp = millis();

            currentState = ParseState::FRAME_COMPLETE;
            return ParseResult::FRAME_COMPLETE;
        }
        else
        {
            errorCount++;
            reset();
            return ParseResult::FRAME_ERROR;
        }
        break;

    case ParseState::FRAME_COMPLETE:
        reset();
        return parseByte(byte, frame);
    }

    return ParseResult::NEED_MORE_DATA;
}

// 检查帧是否超时
bool BluetoothProtocolParser::isFrameTimeout() const
{
    return (currentState != ParseState::WAITING_HEADER1) &&
           (millis() - frameStartTime > FRAME_TIMEOUT_MS);
}

// 批量解析缓冲区数据
ParseResult BluetoothProtocolParser::parseBuffer(uint8_t *buffer, size_t length,
                                                 BluetoothFrame frames[], size_t maxFrames, size_t &frameCount)
{
    frameCount = 0;
    ParseResult lastResult = ParseResult::NEED_MORE_DATA;

    for (size_t i = 0; i < length && frameCount < maxFrames; i++)
    {
        lastResult = parseByte(buffer[i], frames[frameCount]);
        if (lastResult == ParseResult::FRAME_COMPLETE)
        {
            frameCount++;
            reset(); // 准备解析下一帧
        }
    }

    return lastResult;
}

// 获取状态字符串
String BluetoothProtocolParser::getStateString() const
{
    switch (currentState)
    {
    case ParseState::WAITING_HEADER1:
        return "WAITING_HEADER1";
    case ParseState::WAITING_HEADER2:
        return "WAITING_HEADER2";
    case ParseState::WAITING_COMMAND:
        return "WAITING_COMMAND";
    case ParseState::WAITING_LENGTH_HIGH:
        return "WAITING_LENGTH_HIGH";
    case ParseState::WAITING_LENGTH_LOW:
        return "WAITING_LENGTH_LOW";
    case ParseState::WAITING_DATA:
        return "WAITING_DATA";
    case ParseState::WAITING_TAIL1:
        return "WAITING_TAIL1";
    case ParseState::WAITING_TAIL2:
        return "WAITING_TAIL2";
    case ParseState::FRAME_COMPLETE:
        return "FRAME_COMPLETE";
    default:
        return "UNKNOWN";
    }
}

// 打印调试信息
void BluetoothProtocolParser::printDebugInfo() const
{
    Serial.printf("Parser State: %s\n", getStateString().c_str());
    Serial.printf("Command: 0x%02X\n", command);
    Serial.printf("Data Length: %d\n", dataLength);
    Serial.printf("Data Received: %d\n", dataReceived);
    Serial.printf("Error Count: %d\n", errorCount);
    Serial.printf("Frame Start Time: %d\n", frameStartTime);
}

// BluetoothFrame 方法实现
String BluetoothFrame::getTextData() const
{
    if (!isValid || data == nullptr || dataLength == 0)
        return "";
    return String((char *)data, dataLength);
}

void BluetoothFrame::getColorData(uint8_t &screenArea, uint8_t &target, uint8_t &mode, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &gradientMode) const
{
    if (!isValid || data == nullptr || dataLength < BT_COLOR_DATA_LEN)
    {
        screenArea = target = mode = r = g = b = gradientMode = 0;
        return;
    }
    screenArea = data[0];   // 屏幕区域：0x01=上半屏，0x02=下半屏，0x03=全屏
    target = data[1];       // 目标：0x01=文本，0x02=背景
    mode = data[2];         // 模式：0x01=固定色，0x02=渐变色
    r = data[3];            // 红色分量
    g = data[4];            // 绿色分量
    b = data[5];            // 蓝色分量
    gradientMode = data[6]; // 渐变模式
}

uint8_t BluetoothFrame::getBrightnessData() const
{
    if (!isValid || data == nullptr || dataLength < BT_BRIGHTNESS_DATA_LEN)
        return 0;
    return data[0];
}

void BluetoothFrame::getEffectData(uint8_t &screenArea, uint8_t &type, uint8_t &speed) const
{
    if (!isValid || data == nullptr || dataLength < BT_EFFECT_DATA_LEN)
    {
        screenArea = type = speed = 0;
        return;
    }
    screenArea = data[0]; // 屏幕区域
    type = data[1];       // 特效类型
    speed = data[2];      // 速度值
}

bool BluetoothFrame::isValidCommand() const
{
    return isValid && (command >= BT_CMD_SET_DIRECTION && command <= BT_CMD_SET_EFFECT);
}

// 转换8位数据为16位字体数据 (高性能版本)
const uint16_t *BluetoothFrame::getFontData16x16(uint8_t &screenArea, int &charCount) const
{
    if (!isValid || dataLength < 1)
        return nullptr;

    screenArea = data[0];                // 第一个字节是屏幕区域
    int fontDataLength = dataLength - 1; // 减去屏幕区域字节
    charCount = fontDataLength / 32;     // 每个16x16字符32字节

    if (charCount == 0)
        return nullptr;

    // 如果还没转换，进行转换
    if (!isConverted && fontDataLength > 0)
    {
        convertedData = new uint16_t[fontDataLength / 2]; // 16位数据数量是8位的一半

        // 高性能转换：按字节顺序组合 0x12,0x34 -> 0x1234
        for (int i = 0; i < fontDataLength && i + 1 < fontDataLength; i += 2)
        {
            convertedData[i / 2] = ((uint16_t)data[1 + i] << 8) | data[1 + i + 1]; // data[1+i]是高字节，data[1+i+1]是低字节
        }
        isConverted = true;
    }

    return convertedData;
}

// 转换8位数据为32x32字体数据
const uint16_t *BluetoothFrame::getFontData32x32(uint8_t &screenArea, int &charCount) const
{
    if (!isValid || dataLength < 1)
        return nullptr;

    screenArea = data[0];                // 第一个字节是屏幕区域
    int fontDataLength = dataLength - 1; // 减去屏幕区域字节
    charCount = fontDataLength / 128;    // 每个32x32字符128字节

    if (charCount == 0)
        return nullptr;

    // 如果还没转换，进行转换
    if (!isConverted && fontDataLength > 0)
    {
        convertedData = new uint16_t[fontDataLength / 2]; // 16位数据数量是8位的一半

        // 高性能转换：按字节顺序组合 0x12,0x34 -> 0x1234
        for (int i = 0; i < fontDataLength && i + 1 < fontDataLength; i += 2)
        {
            convertedData[i / 2] = ((uint16_t)data[1 + i] << 8) | data[1 + i + 1]; // data[1+i]是高字节，data[1+i+1]是低字节
        }
        isConverted = true;
    }

    return convertedData;
}
