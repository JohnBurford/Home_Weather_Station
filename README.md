# Home_Weather_Station
This build is for taking humidity, temperature, and pressure readings from a remote sensor and to send them via WiFi to a server. The server post the data into a SQL database and host a website to display the data.

**Remote sensor**
Humidity and temperature are taken with DHT22 sensor. Pressure is measured with a BME280 sensor. Both of these sensors are hooked up to an ESP32-S3-DevKitC-1 micro WiFi chip. Readings are taken every 15 minutes. The ESP32 is powered by 2 18650 LiFePO4 rechargeable cells. These batteries were chosen for excellent cold weather performance as well as being very safe. 

**Server**
The server is a Raspberry Pi Zero WH (selected for low power usage) running Raspbian GNU/Linux. The server collects the data from the sensor, stores it in a database, and host a website to present it.

when installing the server for the first time be sure to include a blank .txt file named 'ssh' in the root directory 
Additionally, a file called wpa_supplicant.conf should be in the root directory with the below code. Incorrect formatting or the path in ln2 can cause the wifi not to work. 

country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
ssid="__________"
scan_ssid=1
psk="__________"
key_mgmt=WPA-PSK
}
