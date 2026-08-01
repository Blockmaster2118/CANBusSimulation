#include "can_messages.h"
#include <string.h>

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

/* ============================================================
 * 0x100 WheelSpeed - Speed_kmh: unsigned, scale 0.1, offset 0
 * ============================================================ */
void encode_wheel_speed(uint8_t *data, float speed_kmh) {
    memset(data, 0, 8);
    uint16_t raw = (uint16_t)(speed_kmh / 0.1f);
    pack_u16(data, 0, raw);
}

float decode_wheel_speed(const uint8_t *data) {
    return unpack_u16(data, 0) * 0.1f;
}

/* ============================================================
 * 0x101 MotorRPM - Motor_RPM: unsigned, scale 1, offset 0
 * ============================================================ */
void encode_motor_rpm(uint8_t *data, float rpm) {
    memset(data, 0, 8);
    pack_u16(data, 0, (uint16_t)rpm);
}

float decode_motor_rpm(const uint8_t *data) {
    return (float)unpack_u16(data, 0);
}

/* ============================================================
 * 0x102 MotorTemp - Motor_Temp_C: signed, scale 0.1, offset 0
 * ============================================================ */
void encode_motor_temp(uint8_t *data, float temp_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(temp_c / 0.1f));
}

float decode_motor_temp(const uint8_t *data) {
    return unpack_i16(data, 0) * 0.1f;
}

/* ============================================================
 * 0x103 CoolantTemp - Coolant_In_C (bytes 0-1), Coolant_Out_C (bytes 2-3)
 * ============================================================ */
void encode_coolant_temp(uint8_t *data, float in_c, float out_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(in_c / 0.1f));
    pack_i16(data, 2, (int16_t)(out_c / 0.1f));
}

coolant_temp_t decode_coolant_temp(const uint8_t *data) {
    coolant_temp_t v;
    v.in_c = unpack_i16(data, 0) * 0.1f;
    v.out_c = unpack_i16(data, 2) * 0.1f;
    return v;
}

/* ============================================================
 * 0x104 BatteryTemp - Max (bytes 0-1), Min (bytes 2-3), Avg (bytes 4-5)
 * ============================================================ */
void encode_battery_temp(uint8_t *data, float max_c, float min_c, float avg_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(max_c / 0.1f));
    pack_i16(data, 2, (int16_t)(min_c / 0.1f));
    pack_i16(data, 4, (int16_t)(avg_c / 0.1f));
}

battery_temp_t decode_battery_temp(const uint8_t *data) {
    battery_temp_t v;
    v.max_c = unpack_i16(data, 0) * 0.1f;
    v.min_c = unpack_i16(data, 2) * 0.1f;
    v.avg_c = unpack_i16(data, 4) * 0.1f;
    return v;
}

/* ============================================================
 * 0x105 BatteryVoltage - Pack_Voltage_V: unsigned, scale 0.1
 * ============================================================ */
void encode_battery_voltage(uint8_t *data, float voltage_v) {
    memset(data, 0, 8);
    pack_u16(data, 0, (uint16_t)(voltage_v / 0.1f));
}

float decode_battery_voltage(const uint8_t *data) {
    return unpack_u16(data, 0) * 0.1f;
}

/* ============================================================
 * 0x106 BatteryCurrent - Pack_Current_A: SIGNED, scale 0.1
 * ============================================================ */
void encode_battery_current(uint8_t *data, float current_a) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(current_a / 0.1f));
}

float decode_battery_current(const uint8_t *data) {
    return unpack_i16(data, 0) * 0.1f;
}

/* ============================================================
 * 0x107 InverterTemp - Inverter_Temp_C: signed, scale 0.1
 * ============================================================ */
void encode_inverter_temp(uint8_t *data, float temp_c) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(temp_c / 0.1f));
}

float decode_inverter_temp(const uint8_t *data) {
    return unpack_i16(data, 0) * 0.1f;
}

/* ============================================================
 * 0x108 ImuAccel - Accel_X/Y/Z: signed, scale 0.001 (units of g)
 * ============================================================ */
void encode_imu_accel(uint8_t *data, float ax, float ay, float az) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(ax / 0.001f));
    pack_i16(data, 2, (int16_t)(ay / 0.001f));
    pack_i16(data, 4, (int16_t)(az / 0.001f));
}

vec3_t decode_imu_accel(const uint8_t *data) {
    vec3_t v;
    v.x = unpack_i16(data, 0) * 0.001f;
    v.y = unpack_i16(data, 2) * 0.001f;
    v.z = unpack_i16(data, 4) * 0.001f;
    return v;
}

/* ============================================================
 * 0x109 ImuGyro - Gyro_X/Y/Z: signed, scale 0.01 (units of deg/s)
 * ============================================================ */
void encode_imu_gyro(uint8_t *data, float gx, float gy, float gz) {
    memset(data, 0, 8);
    pack_i16(data, 0, (int16_t)(gx / 0.01f));
    pack_i16(data, 2, (int16_t)(gy / 0.01f));
    pack_i16(data, 4, (int16_t)(gz / 0.01f));
}

vec3_t decode_imu_gyro(const uint8_t *data) {
    vec3_t v;
    v.x = unpack_i16(data, 0) * 0.01f;
    v.y = unpack_i16(data, 2) * 0.01f;
    v.z = unpack_i16(data, 4) * 0.01f;
    return v;
}
