/*
Test the basic operation logic of ablation electrode.

HARDWARE:
  Attach the power input (Vin & GND) with the power source.
  Connect the servo to the BusLinker board.

SERIAL CONTROL:

  BusLinker-V2.2        ARDUINO_MEGA2560
  -------------------------------
  GND(Power)            GND

  5V                    5V
  TX                    RX1
  RX                    TX1
  GND(Control)          GND
  -------------------------------

  AVARIABLE CONTROL SERIAL ON MEGA2560
        SERIAL1    SERIAL2    SERIAL3
  RX      D19        D17        D15
  TX      D18        D16        D14

  USING Serial to control will interfere with with the program uploading,
  so serial connections should be temperarily removed until uploading was done.
  */

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
            delay(2);
        }
        // 启动展示
        demoStart = true;
    }

    if (demoStart)
    {
        Serial.println("#####################################################");
        Serial.println("*********** Demo Starts! ***********");
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

                    Serial.println("*********** Demo end! ***********");
                    Serial.println("#####################################################");
                }
            }
        }
        else
        {
            Serial.println("*********** Target position needs to be checked! ***********");
            Serial.println("*********** Demo end! ***********");
            Serial.println("#####################################################");
        }
        demoStart = false;
    }
}