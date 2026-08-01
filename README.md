OVERVIEW

MushroomESP32 is an Internet of Things (IoT) project for monitoring and controlling the environmental conditions inside a mushroom growing box. The system collects real-time sensor data using ESP32 and allows users to remotely control actuators through a web dashboard.

The project integrates ESP32, MQTT, Node-RED, and MySQL to provide both environmental sensing and device tasking capabilities, enabling real-time monitoring, historical data visualization, and remote device control.

FEATURES
- Real-time temperature monitoring
- Real-time humidity monitoring
- Light intensity monitoring
- Substrate moisture monitoring
- Remote actuator control
- Historical data visualization
- MQTT-based communication
- MySQL data storage
- Web dashboard using Node-RED

INSTALLATION

Step 1: Clone project 

git clone https://github.com/HungB2303818/MushroomESP32.git

Step 2: Import database

database/mushroom_db.sql

Step 3: Start MQTT Broker

docker compose up -d

Step 4: Open Node-red on http://localhost:1880

Step 5: Import flow node-red/flows.json

Step 6: Configure MySQL including username, password, database

Step 7: Configure MQTT port, username, password

Step 8: Open ESP32 project using Platform IO extension on VSCode and upload firmware

Step 9: Configure WIFI including SSID and password
