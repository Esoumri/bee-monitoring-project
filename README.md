 🐝 IoT-Based Beehive Monitoring System

This project is a smart beehive monitoring system based on ESP32. It tracks bee activity (entry/exit), temperature & humidity (inside and outisde the beehive), and light level to detect unusual behavior like swarming or colony collapse (CCD).

 📦 Project Overview

- Microcontroller: ESP32 ESP-32S CH9102X
- Sensors:
  - DHT22 (external temperature & humidity)
  - DHT11 (internal temperature & humidity)
  - IR Break Beam (bee counting)
  - LDR (light detection for night mode)
- Actuators:
- Relay (to disable IR sensor at night)
- 16x2 LCD display (shows real-time temp&humi data)
- LEDs (status indicators: flow normal or abnormal)
- **Connectivity**: Wi-Fi (data sent to a web API)
- **Software**:
  - Arduino IDE (ESP32 code)
  - Fritzing (circuit design)
  - VS code
  - Neon
  - Vercel
  - Next.js


⚙️ Features

- Real-time temperature and humidity monitoring
- Bee flow (entry/exit) counting
- Day/Night detection to power down IR sensor at night
- Alerts for abnormal activity (e.g., low bee movement)
- Data sent to remote API for analysis and logging

