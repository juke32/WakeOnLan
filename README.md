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


![ScreenshotPhone](https://github.com/user-attachments/assets/91d23589-ce1a-47f1-a090-323f21ad8113)
