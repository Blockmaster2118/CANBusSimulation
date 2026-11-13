#include <stdio.h>
#include "can_messages.h"

int main(void) {
    uint8_t buf[8];

    encode_rear_wheel_speed(buf, 87.4f);
    printf("Rear speed: sent 87.4, decoded %.1f\n", decode_rear_wheel_speed(buf));

    encode_battery_current(buf, -42.7f);
    printf("Battery current: sent -42.7, decoded %.1f\n", decode_battery_current(buf));

    encode_cooling_temp(buf, 65.3f, 71.8f);
    cooling_temp_t ct = decode_cooling_temp(buf);
    printf("Coolant: sent (65.3, 71.8), decoded (%.1f, %.1f)\n", ct.in_c, ct.out_c);

    encode_lean_imu(buf, -1234, 0.982f, -0.114f, 1.003f);
    lean_imu_t li = decode_lean_imu(buf);
    printf("Lean IMU: sent (-1234, 0.982, -0.114, 1.003), decoded (%d, %.3f, %.3f, %.3f)\n",
           li.lean_angle_raw, li.ax, li.ay, li.az);

    encode_brake_pressure(buf, 12000, 800);
    brake_pressure_t bp = decode_brake_pressure(buf);
    printf("Brake: sent (12000, 800), decoded (%u, %u)\n", bp.front_raw, bp.rear_raw);

    return 0;
}
