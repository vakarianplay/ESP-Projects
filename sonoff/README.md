## Firmware for Sonoff Basic R2 for control by cURL.

Lightweight firmware with minimalistic web server.
Develop for swith relay via crontab with cURL.


![alt text](https://img.shields.io/badge/Chip-ESP8266-blue?style=for-the-badge&logo=Espressif) ![alt text](https://img.shields.io/badge/Chip-ESP8265-blue?style=for-the-badge&logo=Espressif)

![alt text](https://img.shields.io/badge/Core-PlatformIO-blue?style=flat-square) ![alt text](https://img.shields.io/badge/сURL-compatible-blue?style=flat-square&logo=curl)

--------------------

**CLI client written by Golang is available here**

* Repository: https://github.com/vakarianplay/GoClients/tree/main/RelayClient
* Releases for Windows and Linux https://github.com/vakarianplay/GoRelayClient/releases/tag/v1

--------------------

* Web page

` curl http://SONOFF_IP/`

* Get state

` curl http://SONOFF_IP/st`

* Switch on

` curl http://SONOFF_IP/on`

* Switch off

` curl http://SONOFF_IP/off`

* Toggle state

` curl http://SONOFF_IP/toggleRelay1`

* System information (json format)

` curl http://SONOFF_IP/dev`

* OTA update

` curl http://SONOFF_IP/firmware`


---------------------------------------------------

![image](https://github.com/user-attachments/assets/fca3bfc0-fcf1-4779-9000-9fc1cdc5cf34)
