#include <Arduino.h>
#include "lobot_serial_servo.h"
#include "electrode.h"

int demoStart = false;
double electrodePositions[6] = {5, 20, 10, 25, 20, 45};

void setup()
{
    // put your setup code here, to run once:
    AblationElectrodeInit();
}

void loop()
{
    // put your main code here, to run repeatedly:

    if (Serial.available() > 0)
    {
        while (Serial.available() > 0)
        {
            Serial.read();
            delay(5);
        }
        // 启动展示
        demoStart = true;
    }

    if (demoStart)
    {
        PrintSeperatingLine('*');
        PrintInfo("Demo Starts!", '*');
        // 判断所给参数是否合理
        if (CheckDestinationPosition(electrodePositions))
        {
            // 先整体全部收回
            if (ElectrodePositionWithdraw(15000))
            {
                // 进行电极展开
                if (ElectrodePositionExpand(electrodePositions, 15000))
                {
                    // 再整体全部返回装配位置，等待下一次演示
                    BackToAssemblyPosition();
                }
            }
        }
        else
        {
            PrintInfo("Target position needs to be checked!", '*');
        }
        PrintInfo("Demo end!", '*');
        PrintSeperatingLine('*');
        demoStart = false;
    }
}