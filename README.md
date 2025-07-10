# WakeOnLan
## Once a power outage gets restored an esp32 controller will...

1. start a 3min timer
2. send all set devices a magic packet with their MAC address
3. continue a reoccuring timer to keep waking devices in-case of edge cases or if a ups doesn't fully turn off

## Features:
- Home Assistant integration through EspHome
- Works on esp32,esp32c3,etc controllers
- Made for the esp32c3 0.42LCD oled model with a screen shows ip information, timing information, and more
- Will keep your server running if it has good support for ethernet WOL :)  (haven't tested wifi WOL)

<img width="1253" height="875" alt="image" src="https://github.com/user-attachments/assets/ae17bbf1-aa28-4a98-b645-51d1d07be46b" />
