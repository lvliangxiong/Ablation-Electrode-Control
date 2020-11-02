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
            // 从装配位置开始，收回电极，准备消融
            if (ElectrodePositionWithdraw(15000))
            {
                // 穿刺进入人体，电极展开，开始消融
                if (ElectrodePositionExpand(electrodePositions, 15000))
                {
                    delay(3000); // 消融 ing

                    // 消融结束，收回电极，退出人体
                    if (ElectrodePositionWithdraw(15000))
                    {
                        // 再整体全部返回装配位置，等待下一次演示
                        BackToAssemblyPosition(10000);
                    }
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