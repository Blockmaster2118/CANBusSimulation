#include <stdio.h>
#include "can_messages.h"

int main(void) {
    uint8_t buf[8];

    encode_wheel_speed(buf, 87.4f);
    printf("Wheel speed: sent 87.4, decoded %.1f\n", decode_wheel_speed(buf));

    encode_battery_current(buf, -42.7f);
    printf("Battery current: sent -42.7, decoded %.1f\n", decode_battery_current(buf));

    encode_coolant_temp(buf, 65.3f, 71.8f);
    coolant_temp_t ct = decode_coolant_temp(buf);
    printf("Coolant: sent (65.3, 71.8), decoded (%.1f, %.1f)\n", ct.in_c, ct.out_c);

    encode_imu_accel(buf, 0.982f, -0.114f, 1.003f);
    vec3_t acc = decode_imu_accel(buf);
    printf("Accel: sent (0.982, -0.114, 1.003), decoded (%.3f, %.3f, %.3f)\n", acc.x, acc.y, acc.z);

    return 0;
}
