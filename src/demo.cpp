#include <Arduino.h>
#include "lobot_serial_servo.h"
#include "electrode.h"

bool start = false;
double positions[6];
int i = 0;

void setup()
{
    // put your setup code here, to run once:
    AblationElectrodeInit();
}

void loop()
{
    // put your main code here, to run repeatedly :
    if (Serial.available() > 0)
    {
        while (Serial.available() > 0 && i < 6)
        {
            String str = Serial.readStringUntil(',');
            positions[i] = str.toDouble();
            i++;
            delay(5);
        }
    }

    if (i == 6)
    {
        if (Serial.available() > 0)
        {
            while (Serial.available() > 0)
            {
                Serial.println(Serial.read());
                delay(5);
            }
        }
        // 启动展示
        start = true;

        PrintSeperatingLine('-');
        for (size_t i = 1; i <= 6; i++)
        {
            Serial.printf("Servo id : %d, target position: %s mm\n", i, d2str(positions[i - 1]));
        }
        PrintSeperatingLine('-');
    }

    if (start)
    {
        PrintSeperatingLine('*');
        PrintInfo("Demo Starts!", '*');
        // 判断所给参数是否合理
        if (CheckDestinationPosition(positions))
        {
            // 从装配位置开始，收回电极，准备消融
            if (ElectrodePositionWithdraw(15000))
            {
                // 穿刺进入人体，电极展开，开始消融
                if (ElectrodePositionExpand(positions, 15000))
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
        start = false;
    }
    i = 0;
}