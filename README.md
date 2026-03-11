# DIY JetSki ESP32 Wi-Fi Motor Control

This project creates a web control panel hosted directly by your ESP32 to drive two motors through an L298N.

## Files
- `WIFI-SKI.ino`: ESP32 firmware with built-in web UI

## Pin Mapping Used
- `GPIO26 -> L298N IN1`
- `GPIO25 -> L298N IN2` 
- `GPIO27 -> L298N IN3`
- `GPIO14 -> L298N IN4`

## If GPIO25 Is Actually an Enable Pin
- In `WIFI-SKI.ino`, set `M1_SECOND_PIN_IS_ENABLE = true`.
- In that mode, Motor 1 can run/stop only (no reverse) until rewired to `IN2`.

## Upload Steps
1. Open `WIFI-SKI.ino` in Arduino IDE (or PlatformIO).
2. Select your ESP32 board and COM port.
3. Upload.
4. Open Serial Monitor at `115200` baud.
5. Connect phone/laptop to Wi-Fi:
- SSID: `JETSKI-ESP32`
- Password: `jetski123`
6. Open `http://192.168.4.1`.

## Controls
- Drive pad: Forward, Backward, Left, Right, Stop, Emergency Stop
- Fine controls: Motor 1 and Motor 2 forward/reverse/stop
- Safety: automatic motor stop after command timeout

## Important Notes
- Keep all grounds common (ESP32, L298N, boost converter).
- If a motor spins opposite of expected direction, swap that motor's two output wires on the L298N.
- If your L298N really uses `ENB` on the connected pin (instead of `IN2`), rewire to `IN2` for full bidirectional control of motor 1.
- Test with wheels off the ground first.
