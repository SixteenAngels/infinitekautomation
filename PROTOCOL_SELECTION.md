# Protocol Selection: MQTT vs Matter

Your firmware now supports selectable protocols via config flags in `my_user_config.h`:

- `USE_MQTT` (default: true) — Enables MQTT protocol
- `USE_MATTER` (default: false) — Enables Matter protocol

## How to Select Protocol

1. Open `infinitek/my_user_config.h`
2. Set the desired protocol flag:
   - To enable MQTT: `#define USE_MQTT true`
   - To enable Matter: `#define USE_MATTER true`
   - Only one should be true for exclusive operation. If both are true, a warning will be logged.
3. Build and flash your firmware.

## What Happens
- During startup, only the selected protocol is initialized.
- If both are enabled, a warning is logged and both may run (not recommended).

## Next Steps
- Add your MQTT or Matter initialization code in `infinitek.ino` where indicated.
- Extend the web UI or config system to allow runtime selection if desired.

---

For further customization, update the protocol logic and UI as needed.
