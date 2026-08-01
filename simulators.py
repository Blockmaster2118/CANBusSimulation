"""
Requires libcanmsg.so to be built first:
gcc -Wall -fPIC -shared -o libcanmsg.so can_messages.c
"""

import argparse
import ctypes
import math
import threading
import time

import can


lib = ctypes.CDLL("./libcanmsg.so")

lib.encode_wheel_speed.argtypes = [ctypes.c_char_p, ctypes.c_float]
lib.encode_motor_rpm.argtypes = [ctypes.c_char_p, ctypes.c_float]
lib.encode_motor_temp.argtypes = [ctypes.c_char_p, ctypes.c_float]
lib.encode_coolant_temp.argtypes = [ctypes.c_char_p, ctypes.c_float, ctypes.c_float]
lib.encode_battery_temp.argtypes = [ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]
lib.encode_battery_voltage.argtypes = [ctypes.c_char_p, ctypes.c_float]
lib.encode_battery_current.argtypes = [ctypes.c_char_p, ctypes.c_float]
lib.encode_inverter_temp.argtypes = [ctypes.c_char_p, ctypes.c_float]
lib.encode_imu_accel.argtypes = [ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]
lib.encode_imu_gyro.argtypes = [ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]


def encode(fn, *args):
    """Call a C encode_* function and return the 8 packed bytes."""
    buf = ctypes.create_string_buffer(8)
    fn(buf, *args)
    return bytes(buf.raw)



def gen_wheel_speed(t):
    speed = 40 + 15 * math.sin(t / 5)
    return 0x100, encode(lib.encode_wheel_speed, speed)


def gen_motor_rpm(t):
    rpm = 3000 + 1500 * math.sin(t / 5)
    return 0x101, encode(lib.encode_motor_rpm, rpm)


def gen_motor_temp(t):
    temp = 45 + 10 * math.sin(t / 30)
    return 0x102, encode(lib.encode_motor_temp, temp)


def gen_coolant_temp(t):
    in_c = 40 + 5 * math.sin(t / 30)
    out_c = in_c + 6
    return 0x103, encode(lib.encode_coolant_temp, in_c, out_c)


def gen_battery_temp(t):
    avg = 30 + 8 * math.sin(t / 40)
    return 0x104, encode(lib.encode_battery_temp, avg + 2, avg - 2, avg)


def gen_battery_voltage(t):
    voltage = 96 - 4 * (t / 600)
    return 0x105, encode(lib.encode_battery_voltage, max(voltage, 70))


def gen_battery_current(t):
    current = 20 * math.sin(t / 3) 
    return 0x106, encode(lib.encode_battery_current, current)


def gen_inverter_temp(t):
    temp = 50 + 12 * math.sin(t / 25)
    return 0x107, encode(lib.encode_inverter_temp, temp)


def gen_imu_accel(t):
    ax = 0.05 * math.sin(t * 2)
    ay = 0.03 * math.cos(t * 2)
    az = 1.0  
    return 0x108, encode(lib.encode_imu_accel, ax, ay, az)


def gen_imu_gyro(t):
    gx = 5 * math.sin(t * 1.5)
    gy = 2 * math.cos(t * 1.5)
    gz = 0.0
    return 0x109, encode(lib.encode_imu_gyro, gx, gy, gz)


NODES = [
    ("WheelSpeed",     gen_wheel_speed,     50),
    ("MotorRPM",       gen_motor_rpm,       100),
    ("MotorTemp",      gen_motor_temp,      10),
    ("CoolantTemp",    gen_coolant_temp,    10),
    ("BatteryTemp",    gen_battery_temp,    10),
    ("BatteryVoltage", gen_battery_voltage, 10),
    ("BatteryCurrent", gen_battery_current, 50),
    ("InverterTemp",   gen_inverter_temp,   10),
    ("ImuAccel",       gen_imu_accel,       100),
    ("ImuGyro",        gen_imu_gyro,        100),
]


def run_node(bus, name, gen_fn, rate_hz, start_time, stop_event):
    period = 1.0 / rate_hz
    while not stop_event.is_set():
        t = time.time() - start_time
        can_id, data = gen_fn(t)
        msg = can.Message(arbitration_id=can_id, data=data, is_extended_id=False)
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
    parser.add_argument("--duration", type=float, default=None, help="stop after N seconds (default: run forever)")
    args = parser.parse_args()

    bus = can.interface.Bus(channel=args.channel, interface=args.interface)
    start_time = time.time()
    stop_event = threading.Event()

    threads = [
        threading.Thread(target=run_node, args=(bus, name, gen_fn, rate, start_time, stop_event), daemon=True)
        for name, gen_fn, rate in NODES
    ]
    for th in threads:
        th.start()

    print(f"Simulating {len(NODES)} nodes on {args.interface}:{args.channel}. Ctrl+C to stop.")
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
