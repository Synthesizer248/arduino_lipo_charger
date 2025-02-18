# Arduino-Based LiPo Battery Charger

An Arduino Uno/Atmega328 compatible board -based LiPo battery charging system with automatic calibration,
real-time monitoring, and internal resistance measurement.

![Demo Image](assets/images/lcd_menu.png)

## Features
- **Automatic Calibration**: Fully automatic calibration using the Arduino's internal 1.1V reference and known shunt resistor value.
- **CC/CV Control**: PWM-based constant current and constant voltage control for safe and efficient charging.
- **Buzzer Alerts**: Distinct audible alerts for charging start (high-pitched tone) and charging stop (low-pitched tone).
- **Menu System**: Scrollable menu for presets, calibration, settings, and internal resistance measurement.
- **Real-Time Monitoring**: Displays charging voltage, current, and time on a 16x2 LCD.
- **Internal Resistance Measurement**: Assesses battery health by measuring internal resistance.
- **Save/Reset Settings**: Save and reset configurations via the menu system.
- **Safety Features**: Over-voltage and over-current protection with watchdog timer and soft-start functionality.

## Hardware Requirements
- Arduino Uno
- 16x2 LCD with I2C Module
- INA219 Current Sensor
- Rotary Encoder
- LM2596 Buck Converter
- Buzzer
- Load Resistor (1 Ω, 10W)
- MOSFET (N-channel)
- Capacitors, Resistors, Inductor (see [Component List](Docs/ComponentList.md))

## Software Requirements
- Arduino IDE
- Libraries:
  - `LiquidCrystal_I2C`
  - `EEPROM`
  - `Adafruit_INA219`

## Installation
1. Clone this repository:
   ```bash
   git clone https://github.com/Synthesizer248/arduino_lipo_charger.git
   Open the src/Main.ino file  in the same directory of all other files using Arduino IDE
2. build the circuit first !.
2. Install required libraries via the Library Manager in Arduino IDE.
3. Upload the code to your Arduino Uno/Atmega328 compatible board.
