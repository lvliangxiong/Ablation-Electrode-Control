#include <Arduino.h>
#include "lobot_serial_servo.h"
#include "electrode.h"


// 10 step/s = 10/1000 step/ms <======> 100 ms/step
// 20 step/s                   <======> 50 ms/step
// 30 step/s                   <======> 33 ms/step
// 50 step/s                   <======> 20 ms/step
// 100 step/s                  <======> 10 ms/step

#define WORKING_SPEED 20

bool start = false;
double positions[6];
int i = 0;

void setup()
{
    AblationElectrodeInit();
}

void loop()
{
    // 读取串口位置指令
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

    // 判断是否接收到 6 个位置指令
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
            if (ElectrodePositionWithdraw(WORKING_SPEED))
            {
                PrintInfo("Preparing to puncture...", '*');
                PauseAndWaitForCommand();
                // 穿刺进入人体，电极展开，开始消融
                if (ElectrodePositionExpand(positions, WORKING_SPEED))
                {
                    PrintInfo("Ablating...", '*');
                    delay(3000); // 消融 ing

                    PauseAndWaitForCommand();

                    // 消融结束，收回电极，退出人体
                    if (ElectrodePositionWithdraw(WORKING_SPEED))
                    {
                        PrintInfo("Ablation Done!", '*');
                        PauseAndWaitForCommand();
                        // 再整体全部返回装配位置，等待下一次演示
                        BackToAssemblyPosition();
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