#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <stdint.h>

/* ---- CAN IDs ----
 * These MUST match the BO_ numbers in motorbike.dbc exactly.
 * If you change an ID in the DBC, change it here too.
 */
#define CAN_ID_WHEEL_SPEED     0x100
#define CAN_ID_MOTOR_RPM       0x101
#define CAN_ID_MOTOR_TEMP      0x102
#define CAN_ID_COOLANT_TEMP    0x103
#define CAN_ID_BATTERY_TEMP    0x104
#define CAN_ID_BATTERY_VOLTAGE 0x105
#define CAN_ID_BATTERY_CURRENT 0x106
#define CAN_ID_INVERTER_TEMP   0x107
#define CAN_ID_IMU_ACCEL       0x108
#define CAN_ID_IMU_GYRO        0x109

/* ---- Structs for decoded multi-signal messages ---- */
typedef struct { float in_c; float out_c; } coolant_temp_t;
typedef struct { float max_c; float min_c; float avg_c; } battery_temp_t;
typedef struct { float x; float y; float z; } vec3_t;

/* ---- Encode: physical value(s) -> 8-byte CAN payload ----
 * Every encode function writes into a caller-provided 8-byte buffer.
 * These are what a simulated (or real) sensor node calls before
 * putting a frame on the bus.
 */
void encode_wheel_speed(uint8_t *data, float speed_kmh);
void encode_motor_rpm(uint8_t *data, float rpm);
void encode_motor_temp(uint8_t *data, float temp_c);
void encode_coolant_temp(uint8_t *data, float in_c, float out_c);
void encode_battery_temp(uint8_t *data, float max_c, float min_c, float avg_c);
void encode_battery_voltage(uint8_t *data, float voltage_v);
void encode_battery_current(uint8_t *data, float current_a);
void encode_inverter_temp(uint8_t *data, float temp_c);
void encode_imu_accel(uint8_t *data, float ax, float ay, float az);
void encode_imu_gyro(uint8_t *data, float gx, float gy, float gz);

/* ---- Decode: 8-byte CAN payload -> physical value(s) ----
 * This is what the VCU calls after receiving a frame.
 */
float decode_wheel_speed(const uint8_t *data);
float decode_motor_rpm(const uint8_t *data);
float decode_motor_temp(const uint8_t *data);
coolant_temp_t decode_coolant_temp(const uint8_t *data);
battery_temp_t decode_battery_temp(const uint8_t *data);
float decode_battery_voltage(const uint8_t *data);
float decode_battery_current(const uint8_t *data);
float decode_inverter_temp(const uint8_t *data);
vec3_t decode_imu_accel(const uint8_t *data);
vec3_t decode_imu_gyro(const uint8_t *data);

#endif /* CAN_MESSAGES_H */
