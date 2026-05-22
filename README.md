# ESP32 Capacitor Value and Leakage Current Tester

Capacitor value and leakage current testing system using ESP32.

## Overview
This project aims to develop a capacitor velue and leakage current testing system based on ESP32 for electronics research and validation purposes. The system is designed to measure capacitor leakage behavior, monitor test voltage, and provide a simple and practical testing workflow for evaluating capacitor quality. The project also focuses on embedded firmware development, measurement accuracy, and modular hardware design while building hands-on experience in electronics experimentation and embedded systems engineering.

Capacitor leakage current testing is important to evaluate the insulation quality and reliability of capacitors, especially for electrolytic capacitors used in power supply and electronic circuits. Excessive leakage current may indicate dielectric degradation, aging, contamination, or internal damage that can affect circuit stability, increase power loss, generate heat, or reduce product lifetime. By performing leakage current testing, defective or degraded capacitors can be identified before they cause malfunction or reliability issues in electronic systems.

## Features
- Capaciror value measurement
- Leakage current measurement
- UART logging
- Python data visualitation

## Hardware
- ESP32
- ADC measurement
- Opamp comparator (optional)

## Roadmap
### Project Initialization
- [x] Create GitHub repository
- [x] Setup ESP-IDF environment
- [x] Create firmware project structure

### Core Features
- [ ] Implement ADC voltage measurement
- [ ] Add capacitor value calculation
- [ ] Add leakage current calculation
- [ ] Add UART debug output

### Hardware
- [ ] Build voltage sensing circuit
- [ ] Design current sensing stage
- [ ] Prototype leakage test fixture


### Software Tools
- [ ] Develop Python serial logger
- [ ] Export measurement data to CSV

### Future Improvements
- [ ] Add OLED display
- [ ] Add automatic calibration
- [ ] Add PC visualization tool

## Status
Work in progress
