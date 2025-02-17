# LiPo Charger with Arduino Uno

An Arduino Uno-based LiPo battery charging system with automatic calibration, menu navigation, and real-time monitoring.

## Features
- **Automatic Calibration**: Using internal AREF (1.1V) for precise ADC readings.
- **Menu System**: Scrollable menu for presets, calibration, and settings.
- **Real-Time Monitoring**: Displays charging voltage, current, and time on a 16x2 LCD.
- **Safety Features**: Over-voltage and over-current protection.
- **Save/Reset Settings**: Save and reset configurations via the menu system.

## Hardware Requirements
- Arduino Uno
- 16x2 LCD with I2C Module
- Rotary Encoder
- LM2596 Buck Converter
- Shunt Resistor (0.1 Ω)
- Buzzer
- Capacitors, Resistors, Inductor (see [Component List](docs/ComponentList.md))

## Software Requirements
- Arduino IDE
- Libraries:
  - `LiquidCrystal_I2C`
  - `EEPROM`

## Installation
1. Clone this repository:
   ```bash
   git clone https://github.com/navasare/LiPo-Charger-Arduino.git
