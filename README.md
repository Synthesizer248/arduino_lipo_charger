# Arduino-Based LiPo Battery Charger

An Arduino Uno-based LiPo battery charging system with automatic calibration, real-time monitoring, and internal resistance measurement.

![Demo Image](assets/images/lcd_menu.png)

## Features
- **Automatic Calibration**: Fully automatic calibration using the Arduino's internal 1.1V reference and known shunt resistor value.
- **Menu System**: Scrollable menu for presets, calibration, settings, and internal resistance measurement.
- **Real-Time Monitoring**: Displays charging voltage, current, and time on a 16x2 LCD.
- **Internal Resistance Measurement**: Assesses battery health by measuring internal resistance.
- **Save/Reset Settings**: Save and reset configurations via the menu system.
- **Safety Features**: Over-voltage and over-current protection.

## Hardware Requirements
- Arduino Uno
- 16x2 LCD with I2C Module
- Rotary Encoder
- LM2596 Buck Converter
- Shunt Resistor (0.1 Ω)
- Buzzer
- Load Resistor (1 Ω, 5W)
- MOSFET (N-channel) or Relay
- Capacitors, Resistors, Inductor (see [Component List](Docs/ComponentList.md))

## Software Requirements
- Arduino IDE
- Libraries:
  - `LiquidCrystal_I2C`
  - `EEPROM`

## Installation
1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/arduino_lipo_charger.git
