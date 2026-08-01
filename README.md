# Motorbike CAN bus starter project

## Files

| File | What it is |
|---|---|
| `motorbike.dbc` | The message/signal definitions - the shared contract everything else codes against |
| `can_messages.h/.c` | C encode/decode functions matching the DBC. This is the "real" firmware-facing logic |
| `vcu_listener.c` | A stand-in VCU: opens vcan0, reads frames, decodes them, prints values |
| `simulators.py` | Fakes all 9 sensor nodes, calling into the compiled C library to pack bytes |
| `setup_vcan.sh` | One-time setup of the virtual CAN interface |
| `test_roundtrip.c` | Optional sanity check that encode -> decode gives back the original value |

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

You should see `vcu_listener` printing decoded values for all 9 sensors as `simulators.py` sends fake data.

If you don't have a Linux machine handy yet, `simulators.py` also works with no OS setup at all:

```bash
python3 simulators.py --interface virtual --channel test
```

This uses python-can's in-process virtual bus instead of real SocketCAN - useful for quickly testing changes to the simulator logic itself, but `vcu_listener` (being C + SocketCAN) can't see it, since the virtual bus only exists inside the Python process. Switch back to `--interface socketcan --channel vcan0` (the defaults) once you want the C listener to receive it.

## Editing the DBC - quick reference

```
BO_ 256 WheelSpeed: 8 WheelSpeedSensor
 SG_ Speed_kmh : 0|16@1+ (0.1,0) [0|300] "km/h" VCU
```

| Part | Meaning | If you change it... |
|---|---|---|
| `256` | CAN ID (decimal; `0x100` in the C header) | Must stay unique. Lower = higher bus priority. Must also update the matching `#define` in `can_messages.h` |
| `WheelSpeed` | Message name | Cosmetic, but must match what you reference elsewhere (e.g. in tooling) |
| `8` | Payload length in bytes | Must be ≥ the highest signal's byte range, max 8 for classic CAN |
| `WheelSpeedSensor` | Transmitting node | Must be declared in the `BU_` line |
| `Speed_kmh` | Signal name | Cosmetic |
| `0` | Start bit | Where in the payload this signal begins |
| `16` | Length in bits | How many bits it occupies |
| `@1+` | Byte order + sign | `@1`=little-endian(Intel), `@0`=big-endian(Motorola); `+`=unsigned, `-`=signed |
| `(0.1,0)` | Scale, offset | `physical = raw * scale + offset`. Changing these means you MUST change the matching math in `can_messages.c` |
| `[0\|300]` | Min/max physical range | Documentation only - not enforced automatically by this hand-written C |
| `"km/h"` | Unit | Documentation only |
| `VCU` | Receiving node(s) | Documentation only in this setup |

**Important**: because `can_messages.c` is hand-written rather than auto-generated from the DBC, the DBC and the C code are two separate files that need to be kept in sync manually. If you change a scale factor, start bit, or ID in one, you must mirror it in the other. (Later, once you're comfortable with the format, `cantools`' C code generator can generate `can_messages.c` directly from the DBC, removing this manual sync step - worth doing once you're past the learning phase.)
