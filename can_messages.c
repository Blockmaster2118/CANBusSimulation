#include "can_messages.h"
#include <string.h>

/* All signals in motorbike.dbc are 16-bit, little-endian (@1). */

static void pack_u16(uint8_t *buf, int byte_offset, uint16_t val) {
    buf[byte_offset]     = (uint8_t)(val & 0xFF);
    buf[byte_offset + 1] = (uint8_t)((val >> 8) & 0xFF);
}

static uint16_t unpack_u16(const uint8_t *buf, int byte_offset) {
    return (uint16_t)buf[byte_offset] | ((uint16_t)buf[byte_offset + 1] << 8);
}

static void pack_i16(uint8_t *buf, int byte_offset, int16_t val) {
    pack_u16(buf, byte_offset, (uint16_t)val);
}

static int16_t unpack_i16(const uint8_t *buf, int byte_offset) {
    return (int16_t)unpack_u16(buf, byte_offset);
}

/* physical -> raw: divide by scale, round to nearest, saturate to the type.
 * (Plain truncation turns 87.4 km/h into 87.3, and casting an out-of-range
 * float to an integer is undefined behaviour in C.) */
static uint16_t to_u16(float phys, float scale) {
    float r = phys / scale;
    if (r <= 0.0f) return 0;
    if (r >= 65535.0f) return 65535;
    return (uint16_t)(r + 0.5f);
}

static int16_t to_i16(float phys, float scale) {
    float r = phys / scale;
    if (r <= -32768.0f) return -32768;
    if (r >=  32767.0f) return 32767;
    return (int16_t)(r >= 0.0f ? r + 0.5f : r - 0.5f);
}

/* ============================================================
 * 1  BatteryTemp - Max (bytes 0-1), Min (2-3), Avg (4-5); signed, 0.1 degC
 * ============================================================ */
void encode_battery_temp(uint8_t *data, float max_c, float min_c, float avg_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, to_i16(max_c, 0.1f));
    pack_i16(data, 2, to_i16(min_c, 0.1f));
    pack_i16(data, 4, to_i16(avg_c, 0.1f));
}

battery_temp_t decode_battery_temp(const uint8_t *data) {
    battery_temp_t v;
    v.max_c = unpack_i16(data, 0) * 0.1f;
    v.min_c = unpack_i16(data, 2) * 0.1f;
    v.avg_c = unpack_i16(data, 4) * 0.1f;
    return v;
}

/* ============================================================
 * 2  LeanIMU - Lean_Angle_Raw (0-1, signed, scale 1),
 *              Accel_X/Y/Z (2-3, 4-5, 6-7; signed, 0.001 g)
 * ============================================================ */
void encode_lean_imu(uint8_t *data, int16_t lean_angle_raw, float ax, float ay, float az) {
    memset(data, 0, 8);
    pack_i16(data, 0, lean_angle_raw);
    pack_i16(data, 2, to_i16(ax, 0.001f));
    pack_i16(data, 4, to_i16(ay, 0.001f));
    pack_i16(data, 6, to_i16(az, 0.001f));
}

lean_imu_t decode_lean_imu(const uint8_t *data) {
    lean_imu_t v;
    v.lean_angle_raw = unpack_i16(data, 0);
    v.ax = unpack_i16(data, 2) * 0.001f;
    v.ay = unpack_i16(data, 4) * 0.001f;
    v.az = unpack_i16(data, 6) * 0.001f;
    return v;
}

/* ============================================================
 * 3  PowertrainRotationalSpeed - Powertrain_RPM_Raw: unsigned, scale 1
 * ============================================================ */
void encode_powertrain_rpm_raw(uint8_t *data, uint16_t raw) {
    memset(data, 0, 8);
    pack_u16(data, 0, raw);
}

uint16_t decode_powertrain_rpm_raw(const uint8_t *data) {
    return unpack_u16(data, 0);
}

/* ============================================================
 * 4  RearWheelSpeed - Rear_Speed_kmh: unsigned, 0.1
 * ============================================================ */
void encode_rear_wheel_speed(uint8_t *data, float speed_kmh) {
    memset(data, 0, 8);
    pack_u16(data, 0, to_u16(speed_kmh, 0.1f));
}

float decode_rear_wheel_speed(const uint8_t *data) {
    return unpack_u16(data, 0) * 0.1f;
}

/* ============================================================
 * 5  FrontWheelSpeed - Front_Speed_kmh: unsigned, 0.1
 * ============================================================ */
void encode_front_wheel_speed(uint8_t *data, float speed_kmh) {
    memset(data, 0, 8);
    pack_u16(data, 0, to_u16(speed_kmh, 0.1f));
}

float decode_front_wheel_speed(const uint8_t *data) {
    return unpack_u16(data, 0) * 0.1f;
}

/* ============================================================
 * 6  BrakePressure - Front_Brake_Pressure_Raw (0-1), Rear_Brake_Pressure_Raw (2-3)
 *                    unsigned, scale 1
 * ============================================================ */
void encode_brake_pressure(uint8_t *data, uint16_t front_raw, uint16_t rear_raw) {
    memset(data, 0, 8);
    pack_u16(data, 0, front_raw);
    pack_u16(data, 2, rear_raw);
}

brake_pressure_t decode_brake_pressure(const uint8_t *data) {
    brake_pressure_t v;
    v.front_raw = unpack_u16(data, 0);
    v.rear_raw  = unpack_u16(data, 2);
    return v;
}

/* ============================================================
 * 7  CoolingTemp - Coolant_In_C (0-1), Coolant_Out_C (2-3); signed, 0.1 degC
 * ============================================================ */
void encode_cooling_temp(uint8_t *data, float in_c, float out_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, to_i16(in_c, 0.1f));
    pack_i16(data, 2, to_i16(out_c, 0.1f));
}

cooling_temp_t decode_cooling_temp(const uint8_t *data) {
    cooling_temp_t v;
    v.in_c  = unpack_i16(data, 0) * 0.1f;
    v.out_c = unpack_i16(data, 2) * 0.1f;
    return v;
}

/* ============================================================
 * 8  MotorTemp - Motor_Temp_C: signed, 0.1 degC
 * ============================================================ */
void encode_motor_temp(uint8_t *data, float temp_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, to_i16(temp_c, 0.1f));
}

float decode_motor_temp(const uint8_t *data) {
    return unpack_i16(data, 0) * 0.1f;
}

/* ============================================================
 * 9  InverterTemp - Inverter_Temp_C: signed, 0.1 degC
 * ============================================================ */
void encode_inverter_temp(uint8_t *data, float temp_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, to_i16(temp_c, 0.1f));
}

float decode_inverter_temp(const uint8_t *data) {
    return unpack_i16(data, 0) * 0.1f;
}

/* ============================================================
 * 10 RadiatorTemp - Radiator_Temp_Raw: signed, scale 1
 * ============================================================ */
void encode_radiator_temp_raw(uint8_t *data, int16_t raw) {
    memset(data, 0, 8);
    pack_i16(data, 0, raw);
}

int16_t decode_radiator_temp_raw(const uint8_t *data) {
    return unpack_i16(data, 0);
}

/* ============================================================
 * 11 SuspensionTravel - Front_Suspension_Travel_Raw: unsigned, scale 1
 * ============================================================ */
void encode_suspension_travel_raw(uint8_t *data, uint16_t raw) {
    memset(data, 0, 8);
    pack_u16(data, 0, raw);
}

uint16_t decode_suspension_travel_raw(const uint8_t *data) {
    return unpack_u16(data, 0);
}

/* ============================================================
 * 12 MotorRPM - Motor_RPM: unsigned, scale 1
 * ============================================================ */
void encode_motor_rpm(uint8_t *data, float rpm) {
    memset(data, 0, 8);
    pack_u16(data, 0, to_u16(rpm, 1.0f));
}

float decode_motor_rpm(const uint8_t *data) {
    return (float)unpack_u16(data, 0);
}

/* ============================================================
 * 13 CoolantPumpRPM - Coolant_Pump_RPM_Raw: unsigned, scale 1
 * ============================================================ */
void encode_coolant_pump_rpm_raw(uint8_t *data, uint16_t raw) {
    memset(data, 0, 8);
    pack_u16(data, 0, raw);
}

uint16_t decode_coolant_pump_rpm_raw(const uint8_t *data) {
    return unpack_u16(data, 0);
}

/* ============================================================
 * 14 CoolantPumpFlow - Coolant_Flow_Raw: unsigned, scale 1
 * ============================================================ */
void encode_coolant_flow_raw(uint8_t *data, uint16_t raw) {
    memset(data, 0, 8);
    pack_u16(data, 0, raw);
}

uint16_t decode_coolant_flow_raw(const uint8_t *data) {
    return unpack_u16(data, 0);
}

/* ============================================================
 * 15 IndividualBatteryTemp - Individual_Batt_Temp_Raw: signed, scale 1
 * ============================================================ */
void encode_individual_batt_temp_raw(uint8_t *data, int16_t raw) {
    memset(data, 0, 8);
    pack_i16(data, 0, raw);
}

int16_t decode_individual_batt_temp_raw(const uint8_t *data) {
    return unpack_i16(data, 0);
}

/* ============================================================
 * 16 BatteryVoltage - Pack_Voltage_V: unsigned, 0.1 V
 * ============================================================ */
void encode_battery_voltage(uint8_t *data, float voltage_v) {
    memset(data, 0, 8);
    pack_u16(data, 0, to_u16(voltage_v, 0.1f));
}

float decode_battery_voltage(const uint8_t *data) {
    return unpack_u16(data, 0) * 0.1f;
}

/* ============================================================
 * 17 BatteryCurrent - Pack_Current_A: SIGNED, 0.1 A
 * ============================================================ */
void encode_battery_current(uint8_t *data, float current_a) {
    memset(data, 0, 8);
    pack_i16(data, 0, to_i16(current_a, 0.1f));
}

float decode_battery_current(const uint8_t *data) {
    return unpack_i16(data, 0) * 0.1f;
}
