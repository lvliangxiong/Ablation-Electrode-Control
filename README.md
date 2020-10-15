# Ablation Electrode Control

This project is used for a demo of test the basic operation logic of ablation electrode.

## HARDWARE

- Arduino Mega 2560

- BusLinker

- Usbtinyisp

## Configuration

1. Attach the power input (Vin & GND) with the power source.

2. Connect the servo to the BusLinker board.

3. Connect the serial pin

| BusLinker-V2.2 | ARDUINO_MEGA2560 |
| -------------- | ---------------- |
| GND(Power)     | GND              |
| 5V             | 5V               |
| TX             | RX1              |
| RX             | TX1              |
| GND(Control)   | GND              |

AVARIABLE CONTROL SERIAL ON MEGA2560

|     | SERIAL1 | SERIAL2 | SERIAL3 |
| --- | ------- | ------- | ------- |
| RX  | D19     | D17     | D15     |
| TX  | D18     | D16     | D14     |

USING Serial to control will interfere with with the program uploading, so serial connections should be temperarily removed until uploading was done.
