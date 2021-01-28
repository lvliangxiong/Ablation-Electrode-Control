#define GET_LOW_BYTE(A) (uint8_t)((A))
//宏函数 获得A的低八位
#define GET_HIGH_BYTE(A) (uint8_t)((A) >> 8)
//宏函数 获得A的高八位
#define BYTE_TO_HW(A, B) ((((uint16_t)(A)) << 8) | (uint8_t)(B))
//宏函数 以A为高八位 B为低八位 合并为16位整形

#define LOBOT_SERVO_FRAME_HEADER 0x55
#define LOBOT_SERVO_MOVE_TIME_WRITE 1
#define LOBOT_SERVO_MOVE_TIME_READ 2
#define LOBOT_SERVO_MOVE_TIME_WAIT_WRITE 7
#define LOBOT_SERVO_MOVE_TIME_WAIT_READ 8
#define LOBOT_SERVO_MOVE_START 11
#define LOBOT_SERVO_MOVE_STOP 12
#define LOBOT_SERVO_ID_WRITE 13
#define LOBOT_SERVO_ID_READ 14
#define LOBOT_SERVO_ANGLE_OFFSET_ADJUST 17
#define LOBOT_SERVO_ANGLE_OFFSET_WRITE 18
#define LOBOT_SERVO_ANGLE_OFFSET_READ 19
#define LOBOT_SERVO_ANGLE_LIMIT_WRITE 20
#define LOBOT_SERVO_ANGLE_LIMIT_READ 21
#define LOBOT_SERVO_VIN_LIMIT_WRITE 22
#define LOBOT_SERVO_VIN_LIMIT_READ 23
#define LOBOT_SERVO_TEMP_MAX_LIMIT_WRITE 24
#define LOBOT_SERVO_TEMP_MAX_LIMIT_READ 25
#define LOBOT_SERVO_TEMP_READ 26
#define LOBOT_SERVO_VIN_READ 27
#define LOBOT_SERVO_POS_READ 28
#define LOBOT_SERVO_OR_MOTOR_MODE_WRITE 29
#define LOBOT_SERVO_OR_MOTOR_MODE_READ 30
#define LOBOT_SERVO_LOAD_OR_UNLOAD_WRITE 31
#define LOBOT_SERVO_LOAD_OR_UNLOAD_READ 32
#define LOBOT_SERVO_LED_CTRL_WRITE 33
#define LOBOT_SERVO_LED_CTRL_READ 34
#define LOBOT_SERVO_LED_ERROR_WRITE 35
#define LOBOT_SERVO_LED_ERROR_READ 36

// #define LOBOT_DEBUG 1 /*调试使用，打印调试数据*/

// 计算校验位
byte LobotCheckSum(byte buf[])
{
    byte i;
    uint16_t temp = 0;
    for (i = 2; i < buf[3] + 2; i++)
    {
        temp += buf[i];
    }
    temp = ~temp;
    i = (byte)temp;
    return i;
}

// 转动到指定位置函数
// step range: [0, 1000] correspond to [0, 240]
// time range: [0, 30000] unit: ms
void LobotSerialServoMove(HardwareSerial &SerialX, uint8_t id, int16_t step, uint16_t time)
{
    byte buf[10];
    step = constrain(step, 0, 1000);
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 7;
    buf[4] = LOBOT_SERVO_MOVE_TIME_WRITE;
    buf[5] = GET_LOW_BYTE(step);
    buf[6] = GET_HIGH_BYTE(step);
    buf[7] = GET_LOW_BYTE(time);
    buf[8] = GET_HIGH_BYTE(time);
    buf[9] = LobotCheckSum(buf);
    SerialX.write(buf, 10);
}

// 调整指定id的servo的offset值，设定该值将导致servo立即发生转动，但是此值在掉电后不保存
// offset range: [-125, 125] correspond to [-30, 30] unit: degree
void LobotSerialServoOffsetAdjust(HardwareSerial &SerialX, uint8_t id, int8_t offset)
{
    byte buf[7];
    offset = constrain(offset, -125, 125);
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 4;
    buf[4] = LOBOT_SERVO_ANGLE_OFFSET_ADJUST;
    buf[5] = offset;
    buf[6] = LobotCheckSum(buf);
    SerialX.write(buf, 7);
}

// 将当前servo的offset写入FLASH,实现掉电保存
void LobotSerialServoOffsetWrite(HardwareSerial &SerialX, uint8_t id)
{
    byte buf[6];
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 4;
    buf[4] = LOBOT_SERVO_ANGLE_OFFSET_WRITE;
    buf[5] = LobotCheckSum(buf);
    SerialX.write(buf, 6);
}

// 命令指定id舵机停止运动
void LobotSerialServoStopMove(HardwareSerial &SerialX, uint8_t id)
{
    byte buf[6];
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = LOBOT_SERVO_MOVE_STOP;
    buf[5] = LobotCheckSum(buf);
    SerialX.write(buf, 6);
}

// 设置舵机ID
void LobotSerialServoSetID(HardwareSerial &SerialX, uint8_t oldID, uint8_t newID)
{
    byte buf[7];
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = oldID;
    buf[3] = 4;
    buf[4] = LOBOT_SERVO_ID_WRITE;
    buf[5] = newID;
    buf[6] = LobotCheckSum(buf);
    SerialX.write(buf, 7);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO ID WRITE");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif
}

// 设置舵机运动模式
void LobotSerialServoSetMode(HardwareSerial &SerialX, uint8_t id, uint8_t Mode, int16_t Speed)
{
    byte buf[10];

    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 7;
    buf[4] = LOBOT_SERVO_OR_MOTOR_MODE_WRITE;
    buf[5] = Mode;
    buf[6] = 0;
    buf[7] = GET_LOW_BYTE((uint16_t)Speed);
    buf[8] = GET_HIGH_BYTE((uint16_t)Speed);
    buf[9] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO Set Mode");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    SerialX.write(buf, 10);
}

// 给指定舵机上电
void LobotSerialServoLoad(HardwareSerial &SerialX, uint8_t id)
{
    byte buf[7];
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 4;
    buf[4] = LOBOT_SERVO_LOAD_OR_UNLOAD_WRITE;
    buf[5] = 1;
    buf[6] = LobotCheckSum(buf);

    SerialX.write(buf, 7);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO LOAD WRITE");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif
}

// 给指定舵机卸载
void LobotSerialServoUnload(HardwareSerial &SerialX, uint8_t id)
{
    byte buf[7];
    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 4;
    buf[4] = LOBOT_SERVO_LOAD_OR_UNLOAD_WRITE;
    buf[5] = 0;
    buf[6] = LobotCheckSum(buf);

    SerialX.write(buf, 7);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO LOAD WRITE");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif
}

// 舵机串口接受处理函数
int LobotSerialServoReceiveHandle(HardwareSerial &SerialX, byte *ret)
{
    bool frameStarted = false;
    // bool receiveFinished = false;
    byte frameCount = 0;
    byte dataCount = 0;
    byte dataLength = 2;
    byte rxBuf;
    byte recvBuf[32];
    // byte i;

    while (SerialX.available())
    {
        rxBuf = SerialX.read();
        delayMicroseconds(100);
        // execute when the frame is not started
        if (!frameStarted)
        {
            if (rxBuf == LOBOT_SERVO_FRAME_HEADER)
            {
                frameCount++;
                if (frameCount == 2)
                {
                    // the frame is corectly detected
                    frameStarted = true;
                    frameCount = 0;
                    // index of ID
                    dataCount = 2;
                    recvBuf[0] = LOBOT_SERVO_FRAME_HEADER;
                    recvBuf[1] = LOBOT_SERVO_FRAME_HEADER;
                    continue;
                }
            }
            else
            {
                frameStarted = false;
                frameCount = 0;
                dataCount = 0;
            }
        }
        else
        // process the frame when two headers are detected
        {
            // fill the received data into recvBuff
            recvBuf[dataCount] = (uint8_t)rxBuf;
            // Serial.print(dataCount);
            // Serial.print(' ');
            // Serial.println(rxBuf);

            // obtain the data length
            if (dataCount == 3)
            {
                dataLength = recvBuf[dataCount];
                // execute when data length was outside the range [3,7]
                if (!(dataLength >= 3 && dataCount <= 7))
                {
                    dataLength = 2;
                    frameStarted = false;
                    return -1;
                }
            }
            // execute when the ending is reached
            if (dataCount == (dataLength + 3) - 1)
            {

#ifdef LOBOT_DEBUG
                Serial.print("RECEIVE DATA:");
                for (i = 0; i < dataCount; i++)
                {
                    Serial.print(recvBuf[i], HEX);
                    Serial.print(":");
                }
                Serial.println(" ");
#endif

                if (LobotCheckSum(recvBuf) == recvBuf[dataCount])
                {

#ifdef LOBOT_DEBUG
                    Serial.println("Check SUM OK!!");
                    Serial.println("");
#endif

                    frameStarted = false;
                    memcpy(ret, recvBuf, dataLength + 3);
                    return 1;
                }
                return -1;
            }
            dataCount++;
        }
    }
    return -1;
}

// 读取舵机的位置
// 返回值：-2048代表无响应；-2049代表串口返回数据处理出现问题
int LobotSerialServoReadStep(HardwareSerial &SerialX, uint8_t id)
{
    int count = 10000;
    int ret = -2048;
    byte buf[6];

    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = LOBOT_SERVO_POS_READ;
    buf[5] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO Pos READ");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    while (SerialX.available())
        SerialX.read();
    // 发送读取指令
    SerialX.write(buf, 6);

    // 发送读取指令后，立即进入等待指令返回的状态
    while (!SerialX.available())
    {
        count -= 1;
        if (count < 0)
            return ret;
    }

    // 成功获得返回值，开始处理返回的数据包

    // SerialReceivedPrintln(SerialX);

    byte rxBuf[8];
    if (LobotSerialServoReceiveHandle(SerialX, rxBuf) > 0)
    {
        if (rxBuf[4] == LOBOT_SERVO_POS_READ && rxBuf[2] == id)
        {
            ret = (signed short int)BYTE_TO_HW(rxBuf[6], rxBuf[5]);
        }
    }
    else
        ret = -2049;

#ifdef LOBOT_DEBUG
    Serial.println(ret);
#endif
    return ret;
}

// 读取舵机输入电压
int LobotSerialServoReadVin(HardwareSerial &SerialX, uint8_t id)
{
    int count = 10000;
    int ret = -2048;
    byte buf[6];

    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = LOBOT_SERVO_VIN_READ;
    buf[5] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO VIN READ");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    while (SerialX.available())
        SerialX.read();

    SerialX.write(buf, 6);

    while (!SerialX.available())
    {
        count -= 1;
        if (count < 0)
            return ret;
    }

    byte rxBuf[8];
    if (LobotSerialServoReceiveHandle(SerialX, rxBuf) > 0)
    {
        if (rxBuf[4] == LOBOT_SERVO_VIN_READ && rxBuf[2] == id)
        {
            ret = (int16_t)BYTE_TO_HW(rxBuf[6], rxBuf[5]);
        }
    }
    else
        ret = -2049;

#ifdef LOBOT_DEBUG
    Serial.println(ret);
#endif
    return ret;
}

// 读取舵机角度Offset
int LobotSerialServoReadOffset(HardwareSerial &SerialX, uint8_t id)
{
    int count = 10000;
    int ret = -2048;
    byte buf[6];

    buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = LOBOT_SERVO_ANGLE_OFFSET_READ;
    buf[5] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO VIN READ");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
    {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    while (SerialX.available())
        SerialX.read();

    SerialX.write(buf, 6);

    while (!SerialX.available())
    {
        count -= 1;
        if (count < 0)
            return ret;
    }

    if (LobotSerialServoReceiveHandle(SerialX, buf) > 0)
    {
        if (buf[4] == LOBOT_SERVO_ANGLE_OFFSET_READ && buf[2] == id)
        {
            ret = (signed short int)buf[5];
        }
    }
    else
        ret = -2049;

#ifdef LOBOT_DEBUG
    Serial.println(ret);
#endif
    return ret;
}