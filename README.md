# Motorbike CAN bus starter project

## Files

| File | What it is |
|---|---|
| `motorbike.dbc` | The message/signal definitions - the shared contract everything else codes against |
| `can_messages.h/.c` | C encode/decode functions matching the DBC. This is the "real" firmware-facing logic |
| `vcu_listener.c` | A stand-in VCU: opens vcan0, reads frames, decodes them, prints values |
| `simulators.py` | Fakes all 17 messages (one thread each), calling into the compiled C library to pack bytes. IDs and send rates are read from the DBC |
| `setup_vcan.sh` | One-time setup of the virtual CAN interface |
| `test_roundtrip.c` | Optional sanity check that encode -> decode gives back the original value |
| `test_dbc_sync.py` | Checks that `can_messages.c/.h` produce exactly the bytes `motorbike.dbc` describes (uses `cantools`) |

## Prerequisites

- Linux (native or a VM - not WSL2 without a custom kernel, since it doesn't ship `vcan`)
- `gcc`, `python3`
- `pip install python-can cantools`
- Optional but recommended: `sudo apt install can-utils` (gives you `candump`, `cansend`, `cangen`)

## Build

```bash
gcc -Wall -fPIC -shared -o libcanmsg.so can_messages.c      # for the Python simulator
gcc -Wall -o vcu_listener vcu_listener.c can_messages.c      # the C "VCU"
gcc -Wall -o test_roundtrip test_roundtrip.c can_messages.c  # optional sanity check

python3 test_dbc_sync.py   # run after ANY change to the DBC or can_messages.c
```

## Run

```bash
chmod +x setup_vcan.sh
sudo ./setup_vcan.sh          # once per reboot

# terminal 1
./vcu_listener

# terminal 2
python3 simulators.py

# terminal 3 (optional) - watch raw traffic
candump vcan0
```

You should see `vcu_listener` printing decoded values for all 17 messages as `simulators.py` sends fake data.

If you don't have a Linux machine handy yet, `simulators.py` also works with no OS setup at all:

```bash
python3 simulators.py --interface virtual --channel test
```

This uses python-can's in-process virtual bus instead of real SocketCAN - useful for quickly testing changes to the simulator logic itself, but `vcu_listener` (being C + SocketCAN) can't see it, since the virtual bus only exists inside the Python process. Switch back to `--interface socketcan --channel vcan0` (the defaults) once you want the C listener to receive it.

## Editing the DBC - quick reference

```
BO_ 4 RearWheelSpeed: 8 Rear
 SG_ Rear_Speed_kmh : 0|16@1+ (0.1,0) [0|300] "km/h" VCU
```

| Part | Meaning | If you change it... |
|---|---|---|
| `4` | CAN ID (decimal) | Must stay unique. Lower = higher bus priority. Must also update the matching `#define` in `can_messages.h` |
| `RearWheelSpeed` | Message name | Cosmetic, but must match what you reference elsewhere (e.g. in tooling) |
| `8` | Payload length in bytes | Must be ≥ the highest signal's byte range, max 8 for classic CAN |
| `Rear` | Transmitting node | Must be declared in the `BU_` line |
| `Rear_Speed_kmh` | Signal name | Cosmetic |
| `0` | Start bit | Where in the payload this signal begins |
| `16` | Length in bits | How many bits it occupies |
| `@1+` | Byte order + sign | `@1`=little-endian(Intel), `@0`=big-endian(Motorola); `+`=unsigned, `-`=signed |
| `(0.1,0)` | Scale, offset | `physical = raw * scale + offset`. Changing these means you MUST change the matching math in `can_messages.c` |
| `[0\|300]` | Min/max physical range | Documentation only - not enforced automatically by this hand-written C |
| `"km/h"` | Unit | Documentation only |
| `VCU` | Receiving node(s) | Documentation only in this setup |

**Important**: because `can_messages.c` is hand-written rather than auto-generated from the DBC, the DBC and the C code are two separate files that need to be kept in sync manually. If you change a scale factor, start bit, or ID in one, you must mirror it in the other. `python3 test_dbc_sync.py` compares the C encoders against the DBC (via `cantools`) and fails if they drift apart. `simulators.py` also refuses to start if a DBC message has no generator (or vice versa). (Later, `cantools`' C code generator can generate `can_messages.c` directly from the DBC, removing the manual sync step entirely.)

## Messages (from `motorbike.dbc`)

| ID | Message | Sender | Cycle |
|---|---|---|---|
| 1 | BatteryTemp | BMS | 100 ms |
| 2 | LeanIMU | LeanAngle | 10 ms |
| 3 | PowertrainRotationalSpeed | Motor | 10 ms |
| 4 | RearWheelSpeed | Rear | 20 ms |
| 5 | FrontWheelSpeed | Front | 20 ms |
| 6 | BrakePressure | BrakeSensors | 20 ms |
| 7 | CoolingTemp | Radiator | 100 ms |
| 8 | MotorTemp | Motor | 100 ms |
| 9 | InverterTemp | Inverter | 100 ms |
| 10 | RadiatorTemp | Radiator | 100 ms |
| 11 | SuspensionTravel | Front | 20 ms |
| 12 | MotorRPM | Motor | 10 ms |
| 13 | CoolantPumpRPM | Radiator | 100 ms |
| 14 | CoolantPumpFlow | Radiator | 100 ms |
| 15 | IndividualBatteryTemp | Temperature | 100 ms |
| 16 | BatteryVoltage | BMS | 100 ms |
| 17 | BatteryCurrent | BMS | 20 ms |

Signals named `*_Raw` have scale 1 and unit `raw` in the DBC, so the C code and the listener pass them through as unscaled integers.
