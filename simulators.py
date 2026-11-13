"""
Fake sensor nodes for the motorbike CAN bus.

One thread per DBC message. Send rates come from each message's
GenMsgCycleTime in motorbike.dbc, so changing a cycle time there is enough.

Requires libcanmsg.so to be built first:
    gcc -Wall -fPIC -shared -o libcanmsg.so can_messages.c
"""

import argparse
import ctypes
import math
import threading
import time

import can
import cantools


lib = ctypes.CDLL("./libcanmsg.so")

F, U16, I16, BUF = ctypes.c_float, ctypes.c_uint16, ctypes.c_int16, ctypes.c_char_p

lib.encode_battery_temp.argtypes = [BUF, F, F, F]
lib.encode_lean_imu.argtypes = [BUF, I16, F, F, F]
lib.encode_powertrain_rpm_raw.argtypes = [BUF, U16]
lib.encode_rear_wheel_speed.argtypes = [BUF, F]
lib.encode_front_wheel_speed.argtypes = [BUF, F]
lib.encode_brake_pressure.argtypes = [BUF, U16, U16]
lib.encode_cooling_temp.argtypes = [BUF, F, F]
lib.encode_motor_temp.argtypes = [BUF, F]
lib.encode_inverter_temp.argtypes = [BUF, F]
lib.encode_radiator_temp_raw.argtypes = [BUF, I16]
lib.encode_suspension_travel_raw.argtypes = [BUF, U16]
lib.encode_motor_rpm.argtypes = [BUF, F]
lib.encode_coolant_pump_rpm_raw.argtypes = [BUF, U16]
lib.encode_coolant_flow_raw.argtypes = [BUF, U16]
lib.encode_individual_batt_temp_raw.argtypes = [BUF, I16]
lib.encode_battery_voltage.argtypes = [BUF, F]
lib.encode_battery_current.argtypes = [BUF, F]


def encode(fn, *args):
    """Call a C encode_* function and return the 8 packed bytes."""
    buf = ctypes.create_string_buffer(8)
    fn(buf, *args)
    return bytes(buf.raw)


# ---- Signal generators: t (seconds since start) -> 8 payload bytes ----
# Keys are the message names in motorbike.dbc.

def gen_battery_temp(t):
    avg = 30 + 8 * math.sin(t / 40)
    return encode(lib.encode_battery_temp, avg + 2, avg - 2, avg)


def gen_lean_imu(t):
    lean_raw = int(3000 * math.sin(t / 4))
    ax = 0.05 * math.sin(t * 2)
    ay = 0.03 * math.cos(t * 2)
    az = 1.0
    return encode(lib.encode_lean_imu, lean_raw, ax, ay, az)


def gen_powertrain_rpm(t):
    return encode(lib.encode_powertrain_rpm_raw, int(3000 + 1500 * math.sin(t / 5)))


def gen_rear_wheel_speed(t):
    return encode(lib.encode_rear_wheel_speed, 40 + 15 * math.sin(t / 5))


def gen_front_wheel_speed(t):
    return encode(lib.encode_front_wheel_speed, 40 + 15 * math.sin(t / 5) + 0.3 * math.sin(t * 3))


def gen_brake_pressure(t):
    front = int(20000 * max(0.0, math.sin(t / 3)))
    return encode(lib.encode_brake_pressure, front, int(front * 0.4))


def gen_cooling_temp(t):
    in_c = 40 + 5 * math.sin(t / 30)
    return encode(lib.encode_cooling_temp, in_c, in_c + 6)


def gen_motor_temp(t):
    return encode(lib.encode_motor_temp, 45 + 10 * math.sin(t / 30))


def gen_inverter_temp(t):
    return encode(lib.encode_inverter_temp, 50 + 12 * math.sin(t / 25))


def gen_radiator_temp(t):
    return encode(lib.encode_radiator_temp_raw, int(400 + 60 * math.sin(t / 30)))


def gen_suspension_travel(t):
    return encode(lib.encode_suspension_travel_raw, int(30000 + 8000 * math.sin(t * 2)))


def gen_motor_rpm(t):
    return encode(lib.encode_motor_rpm, 3000 + 1500 * math.sin(t / 5))


def gen_coolant_pump_rpm(t):
    return encode(lib.encode_coolant_pump_rpm_raw, int(2500 + 400 * math.sin(t / 20)))


def gen_coolant_flow(t):
    return encode(lib.encode_coolant_flow_raw, int(1200 + 200 * math.sin(t / 20)))


def gen_individual_batt_temp(t):
    return encode(lib.encode_individual_batt_temp_raw, int(300 + 40 * math.sin(t / 40)))


def gen_battery_voltage(t):
    return encode(lib.encode_battery_voltage, max(96 - 4 * (t / 600), 70))


def gen_battery_current(t):
    return encode(lib.encode_battery_current, 20 * math.sin(t / 3))


GENERATORS = {
    "BatteryTemp":                gen_battery_temp,
    "LeanIMU":                    gen_lean_imu,
    "PowertrainRotationalSpeed":  gen_powertrain_rpm,
    "RearWheelSpeed":             gen_rear_wheel_speed,
    "FrontWheelSpeed":            gen_front_wheel_speed,
    "BrakePressure":              gen_brake_pressure,
    "CoolingTemp":                gen_cooling_temp,
    "MotorTemp":                  gen_motor_temp,
    "InverterTemp":               gen_inverter_temp,
    "RadiatorTemp":               gen_radiator_temp,
    "SuspensionTravel":           gen_suspension_travel,
    "MotorRPM":                   gen_motor_rpm,
    "CoolantPumpRPM":             gen_coolant_pump_rpm,
    "CoolantPumpFlow":            gen_coolant_flow,
    "IndividualBatteryTemp":      gen_individual_batt_temp,
    "BatteryVoltage":             gen_battery_voltage,
    "BatteryCurrent":             gen_battery_current,
}


def build_nodes(db):
    """Pair every DBC message with its generator; ID and rate come from the DBC."""
    missing = [m.name for m in db.messages if m.name not in GENERATORS]
    extra = [n for n in GENERATORS if n not in {m.name for m in db.messages}]
    if missing or extra:
        raise SystemExit(
            f"simulators.py and motorbike.dbc are out of sync.\n"
            f"  in DBC but no generator: {missing}\n"
            f"  generator but not in DBC: {extra}"
        )
    nodes = []
    for msg in db.messages:
        cycle_ms = msg.cycle_time or 100
        nodes.append((msg.name, msg.frame_id, GENERATORS[msg.name], 1000.0 / cycle_ms))
    return nodes


def run_node(bus, name, can_id, gen_fn, rate_hz, start_time, stop_event):
    period = 1.0 / rate_hz
    while not stop_event.is_set():
        t = time.time() - start_time
        msg = can.Message(arbitration_id=can_id, data=gen_fn(t), is_extended_id=False)
        try:
            bus.send(msg)
        except can.CanError as e:
            print(f"[{name}] send failed: {e}")
        time.sleep(period)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--channel", default="vcan0", help="CAN channel (default: vcan0)")
    parser.add_argument(
        "--interface",
        default="socketcan",
        help="python-can interface type (default: socketcan; use 'virtual' for no OS setup)",
    )
    parser.add_argument("--dbc", default="motorbike.dbc", help="DBC file (default: motorbike.dbc)")
    parser.add_argument("--duration", type=float, default=None, help="stop after N seconds (default: run forever)")
    args = parser.parse_args()

    db = cantools.database.load_file(args.dbc)
    nodes = build_nodes(db)

    bus = can.interface.Bus(channel=args.channel, interface=args.interface)
    start_time = time.time()
    stop_event = threading.Event()

    threads = [
        threading.Thread(
            target=run_node,
            args=(bus, name, can_id, gen_fn, rate, start_time, stop_event),
            daemon=True,
        )
        for name, can_id, gen_fn, rate in nodes
    ]
    for th in threads:
        th.start()

    print(f"Simulating {len(nodes)} messages on {args.interface}:{args.channel}. Ctrl+C to stop.")
    try:
        if args.duration:
            time.sleep(args.duration)
        else:
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        time.sleep(0.1)
        bus.shutdown()


if __name__ == "__main__":
    main()
