"""
Checks that can_messages.c agrees with motorbike.dbc.

For every message, encodes random in-range values with the C library and with
cantools (which reads the DBC), and requires identical bytes. Also checks the
CAN_ID_* defines in can_messages.h against the DBC IDs, and that C decode
returns what cantools decodes.

    gcc -Wall -fPIC -shared -o libcanmsg.so can_messages.c
    python3 test_dbc_sync.py
"""

import ctypes
import random
import re
import sys

import cantools

db = cantools.database.load_file("motorbike.dbc")
lib = ctypes.CDLL("./libcanmsg.so")

F, U16, I16 = ctypes.c_float, ctypes.c_uint16, ctypes.c_int16

# DBC message name -> (C function suffix, [signal names in C argument order], [ctypes arg types])
SPEC = {
    "BatteryTemp":               ("battery_temp",              ["Batt_Temp_Max_C", "Batt_Temp_Min_C", "Batt_Temp_Avg_C"], [F, F, F]),
    "LeanIMU":                   ("lean_imu",                  ["Lean_Angle_Raw", "Accel_X", "Accel_Y", "Accel_Z"], [I16, F, F, F]),
    "PowertrainRotationalSpeed": ("powertrain_rpm_raw",        ["Powertrain_RPM_Raw"], [U16]),
    "RearWheelSpeed":            ("rear_wheel_speed",          ["Rear_Speed_kmh"], [F]),
    "FrontWheelSpeed":           ("front_wheel_speed",         ["Front_Speed_kmh"], [F]),
    "BrakePressure":             ("brake_pressure",            ["Front_Brake_Pressure_Raw", "Rear_Brake_Pressure_Raw"], [U16, U16]),
    "CoolingTemp":               ("cooling_temp",              ["Coolant_In_C", "Coolant_Out_C"], [F, F]),
    "MotorTemp":                 ("motor_temp",                ["Motor_Temp_C"], [F]),
    "InverterTemp":              ("inverter_temp",             ["Inverter_Temp_C"], [F]),
    "RadiatorTemp":              ("radiator_temp_raw",         ["Radiator_Temp_Raw"], [I16]),
    "SuspensionTravel":          ("suspension_travel_raw",     ["Front_Suspension_Travel_Raw"], [U16]),
    "MotorRPM":                  ("motor_rpm",                 ["Motor_RPM"], [F]),
    "CoolantPumpRPM":            ("coolant_pump_rpm_raw",      ["Coolant_Pump_RPM_Raw"], [U16]),
    "CoolantPumpFlow":           ("coolant_flow_raw",          ["Coolant_Flow_Raw"], [U16]),
    "IndividualBatteryTemp":     ("individual_batt_temp_raw",  ["Individual_Batt_Temp_Raw"], [I16]),
    "BatteryVoltage":            ("battery_voltage",           ["Pack_Voltage_V"], [F]),
    "BatteryCurrent":            ("battery_current",           ["Pack_Current_A"], [F]),
}


def quantised(sig):
    """Random physical value exactly representable by the signal (a whole raw step)."""
    lo = int(round((sig.minimum - sig.offset) / sig.scale)) if sig.minimum is not None else None
    hi = int(round((sig.maximum - sig.offset) / sig.scale)) if sig.maximum is not None else None
    bits = sig.length
    if sig.is_signed:
        tmin, tmax = -(1 << (bits - 1)), (1 << (bits - 1)) - 1
    else:
        tmin, tmax = 0, (1 << bits) - 1
    lo = tmin if lo is None else max(lo, tmin)
    hi = tmax if hi is None else min(hi, tmax)
    raw = random.randint(lo, hi)
    return raw, raw * sig.scale + sig.offset


def main():
    random.seed(1)
    failures = 0

    # 1. every DBC message is covered, and every spec entry exists
    dbc_names = {m.name for m in db.messages}
    if dbc_names != set(SPEC):
        print(f"FAIL  message set differs. DBC-only: {dbc_names - set(SPEC)}  spec-only: {set(SPEC) - dbc_names}")
        return 1

    # 2. CAN_ID_* defines match the DBC (header order == DBC order of SPEC keys)
    header = open("can_messages.h").read()
    header_ids = sorted(int(v, 0) for v in re.findall(r"#define\s+CAN_ID_\w+\s+(\w+)", header))
    dbc_ids = sorted(m.frame_id for m in db.messages)
    if header_ids != dbc_ids:
        print(f"FAIL  CAN IDs differ. header={header_ids} dbc={dbc_ids}")
        failures += 1

    # 3. encode bytes and decode values match cantools
    for msg_name, (suffix, sig_names, argtypes) in SPEC.items():
        msg = db.get_message_by_name(msg_name)
        enc = getattr(lib, f"encode_{suffix}")
        enc.argtypes = [ctypes.c_char_p] + argtypes
        bad = 0
        for _ in range(500):
            raws, phys = {}, {}
            for n in sig_names:
                raws[n], phys[n] = quantised(msg.get_signal_by_name(n) if hasattr(msg, "get_signal_by_name") else next(s for s in msg.signals if s.name == n))
            buf = ctypes.create_string_buffer(8)
            c_args = [
                (phys[n] if t is F else raws[n]) for n, t in zip(sig_names, argtypes)
            ]
            enc(buf, *c_args)
            expected = bytes(msg.encode(phys, scaling=True, padding=False, strict=False))
            if bytes(buf.raw) != expected:
                bad += 1
                if bad == 1:
                    print(f"      e.g. {msg_name}: C={bytes(buf.raw).hex()} cantools={expected.hex()} phys={phys}")
        status = "ok  " if bad == 0 else "FAIL"
        print(f"{status}  ID {msg.frame_id:>2}  {msg_name:<26} ({500 - bad}/500 encodes match)")
        failures += bad > 0

    # 4. spot check of round-trip accuracy for a value that isn't a raw-step multiple
    buf = ctypes.create_string_buffer(8)
    lib.encode_rear_wheel_speed.argtypes = [ctypes.c_char_p, F]
    lib.encode_rear_wheel_speed(buf, 87.4)
    got = db.get_message_by_name("RearWheelSpeed").decode(bytes(buf.raw))["Rear_Speed_kmh"]
    ok = abs(got - 87.4) < 1e-6
    print(f"{'ok  ' if ok else 'FAIL'}  87.4 km/h encodes to {got} (rounding, not truncation)")
    failures += not ok

    print("\nALL IN SYNC" if failures == 0 else f"\n{failures} problem(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
