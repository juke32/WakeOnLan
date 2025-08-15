# juke32/WakeOnLan
## Once a power outage gets restored an esp32 controller will...

1. start a 3min timer
2. send all set devices a magic packet with their MAC address
3. continue an incremental timer to keep waking devices in-case of edge cases or accidental power offs

## Features:
- Home Assistant integration through EspHome
- Works on esp32,esp32c3,etc controllers
- Made for the esp32c3 0.42LCD oled model with a screen shows ip information, timing information, and more
- Will keep your server running if it has good support for ethernet WOL :)  (haven't tested wifi WOL)

## Setup:

ALL SETTINGS WILL GET RESET WHEN FLASHING FIRMWARE
- hold boot button while plugging device into computer
- Flash the latest release bin with https://web.esphome.io/
- after flash, power cycle device
- join wireless access point from phone/computer "WakeOnLan-APmode" : "password" (may take 1 min to show up)
- go to http://192.168.4.1 in a browser and log into network on webpage
- find the device's ip on the new network, check display or use an ipscanner
- type the ip in the address bar of a browser
- change mac settings and timing to desired values
- power cycle and make sure settings are static


----
![ScreenshotPhone](https://github.com/user-attachments/assets/91d23589-ce1a-47f1-a090-323f21ad8113)
