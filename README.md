# Home_Weather_Station
This build is for taking humidity, temperature, and pressure readings from a remote sensor and to send them via WiFi to a server. The server post the data into a SQL database and host a website to display the data.

**Remote sensor**
Humidity and temperature are taken with DHT22 sensor. Pressure is measured with a BME280 sensor. Both of these sensors are hooked up to an ESP32-S3-DevKitC-1 micro WiFi chip. Readings are taken every 15 minutes. The ESP32 is powered by 2 18650 LiFePO4 rechargeable cells. These batteries were chosen for excellent cold weather performance as well as being very safe. 

**Server**
The server is a Raspberry Pi Zero WH (selected for low power usage) running Raspbian GNU/Linux. The server collects the data from the sensor, stores it in a database, and host a website to present it.
