/* 
Test the basic operation logic of ablation electrode.

HARDWARE:
  Attach the power input (Vin & GND) with the power source.
  Connect the servo to the BusLinker board.

SERIAL CONTROL:
1. hardware serial control

  BusLinker-V2.2        ARDUINO_MEGA2560 or NANO
  -------------------------------
  GND(Power)            GND

  5V                    5V
  TX                    RX0
  RX                    TX0
  GND(Control)          GND
  -------------------------------

  AVARIABLE CONTROL SERIAL
    MEGA  Serial, Serial1, Serial2, Serial3
    NANO  Serial

  USING Serial to control will interfere with with the program uploading, 
  so serial connections should be temperarily removed until uploading was done.

2. software serial control (using SoftwareSerial lib)

  BusLinker-V2.2        ARDUINO_NANO
  -------------------------------
  GND(Power)            GND

  5V                    5V
  TX                    D4
  RX                    D3
  GND(Control)          GND
  -------------------------------

NOTES:
  Following code are written based on approach 1. */

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
      ElectrodePositionWithdraw(15000);
      // 进行电极展开
      ElectrodePositionExpand(electrodePositions, 15000);
      // 再整体全部返回装配位置，等待下一次演示
      BackToAssemblyPosition();

      // 运行一遍后停止
      demoStart = false;
      Serial.println("*********** Demo end! ***********");
      Serial.println("#####################################################");
    }
  }
}
