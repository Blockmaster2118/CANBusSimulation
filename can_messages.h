#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <stdint.h>

/* ---- CAN IDs ----
 * These MUST match the BO_ numbers in motorbike.dbc exactly.
 * If you change an ID in the DBC, change it here too
 * (test_dbc_sync.py will catch a mismatch).
 */
#define CAN_ID_BATTERY_TEMP            1
#define CAN_ID_LEAN_IMU                2
#define CAN_ID_POWERTRAIN_RPM          3
#define CAN_ID_REAR_WHEEL_SPEED        4
#define CAN_ID_FRONT_WHEEL_SPEED       5
#define CAN_ID_BRAKE_PRESSURE          6
#define CAN_ID_COOLING_TEMP            7
#define CAN_ID_MOTOR_TEMP              8
#define CAN_ID_INVERTER_TEMP           9
#define CAN_ID_RADIATOR_TEMP          10
#define CAN_ID_SUSPENSION_TRAVEL      11
#define CAN_ID_MOTOR_RPM              12
#define CAN_ID_COOLANT_PUMP_RPM       13
#define CAN_ID_COOLANT_PUMP_FLOW      14
#define CAN_ID_INDIVIDUAL_BATT_TEMP   15
#define CAN_ID_BATTERY_VOLTAGE        16
#define CAN_ID_BATTERY_CURRENT        17

/* ---- Structs for decoded multi-signal messages ---- */
typedef struct { float max_c; float min_c; float avg_c; } battery_temp_t;
typedef struct { float in_c; float out_c; } cooling_temp_t;
typedef struct { uint16_t front_raw; uint16_t rear_raw; } brake_pressure_t;
typedef struct {
    int16_t lean_angle_raw;   /* unscaled, as defined in the DBC */
    float   ax, ay, az;       /* g */
} lean_imu_t;

/* ---- Encode: physical (or raw) value(s) -> 8-byte CAN payload ----
 * Every encode function writes into a caller-provided 8-byte buffer.
 * Physical values are rounded to the nearest raw step and saturated to the
 * signal's bit width. Signals the DBC defines as "_Raw" (scale 1, unit "raw")
 * take/return the raw integer directly.
 */
void encode_battery_temp(uint8_t *data, float max_c, float min_c, float avg_c);
void encode_lean_imu(uint8_t *data, int16_t lean_angle_raw, float ax, float ay, float az);
void encode_powertrain_rpm_raw(uint8_t *data, uint16_t raw);
void encode_rear_wheel_speed(uint8_t *data, float speed_kmh);
void encode_front_wheel_speed(uint8_t *data, float speed_kmh);
void encode_brake_pressure(uint8_t *data, uint16_t front_raw, uint16_t rear_raw);
void encode_cooling_temp(uint8_t *data, float in_c, float out_c);
void encode_motor_temp(uint8_t *data, float temp_c);
void encode_inverter_temp(uint8_t *data, float temp_c);
void encode_radiator_temp_raw(uint8_t *data, int16_t raw);
void encode_suspension_travel_raw(uint8_t *data, uint16_t raw);
void encode_motor_rpm(uint8_t *data, float rpm);
void encode_coolant_pump_rpm_raw(uint8_t *data, uint16_t raw);
void encode_coolant_flow_raw(uint8_t *data, uint16_t raw);
void encode_individual_batt_temp_raw(uint8_t *data, int16_t raw);
void encode_battery_voltage(uint8_t *data, float voltage_v);
void encode_battery_current(uint8_t *data, float current_a);

/* ---- Decode: 8-byte CAN payload -> physical (or raw) value(s) ---- */
battery_temp_t   decode_battery_temp(const uint8_t *data);
lean_imu_t       decode_lean_imu(const uint8_t *data);
uint16_t         decode_powertrain_rpm_raw(const uint8_t *data);
float            decode_rear_wheel_speed(const uint8_t *data);
float            decode_front_wheel_speed(const uint8_t *data);
brake_pressure_t decode_brake_pressure(const uint8_t *data);
cooling_temp_t   decode_cooling_temp(const uint8_t *data);
float            decode_motor_temp(const uint8_t *data);
float            decode_inverter_temp(const uint8_t *data);
int16_t          decode_radiator_temp_raw(const uint8_t *data);
uint16_t         decode_suspension_travel_raw(const uint8_t *data);
float            decode_motor_rpm(const uint8_t *data);
uint16_t         decode_coolant_pump_rpm_raw(const uint8_t *data);
uint16_t         decode_coolant_flow_raw(const uint8_t *data);
int16_t          decode_individual_batt_temp_raw(const uint8_t *data);
float            decode_battery_voltage(const uint8_t *data);
float            decode_battery_current(const uint8_t *data);

#endif /* CAN_MESSAGES_H */
