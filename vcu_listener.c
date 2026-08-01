#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "can_messages.h"

int main(void) {
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl (is vcan0 up?)");
        close(sock);
        return 1;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    printf("VCU listener running on vcan0. Waiting for frames...\n");

    struct can_frame frame;
    while (1) {
        ssize_t nbytes = read(sock, &frame, sizeof(frame));
        if (nbytes < 0) {
            perror("read");
            break;
        }
        if (nbytes < (ssize_t)sizeof(struct can_frame)) {
            continue; /* incomplete frame, skip */
        }

        switch (frame.can_id) {
            case CAN_ID_WHEEL_SPEED:
                printf("[0x%03X] Wheel speed:      %.1f km/h\n",
                       frame.can_id, decode_wheel_speed(frame.data));
                break;

            case CAN_ID_MOTOR_RPM:
                printf("[0x%03X] Motor RPM:        %.0f rpm\n",
                       frame.can_id, decode_motor_rpm(frame.data));
                break;

            case CAN_ID_MOTOR_TEMP:
                printf("[0x%03X] Motor temp:       %.1f C\n",
                       frame.can_id, decode_motor_temp(frame.data));
                break;

            case CAN_ID_COOLANT_TEMP: {
                coolant_temp_t c = decode_coolant_temp(frame.data);
                printf("[0x%03X] Coolant in/out:   %.1f C / %.1f C\n",
                       frame.can_id, c.in_c, c.out_c);
                break;
            }

            case CAN_ID_BATTERY_TEMP: {
                battery_temp_t b = decode_battery_temp(frame.data);
                printf("[0x%03X] Battery temp:     max %.1f / min %.1f / avg %.1f C\n",
                       frame.can_id, b.max_c, b.min_c, b.avg_c);
                if (b.max_c > 55.0f) {
                    printf("         WARNING: battery over-temp, consider derating power\n");
                }
                break;
            }

            case CAN_ID_BATTERY_VOLTAGE:
                printf("[0x%03X] Battery voltage:  %.1f V\n",
                       frame.can_id, decode_battery_voltage(frame.data));
                break;

            case CAN_ID_BATTERY_CURRENT:
                printf("[0x%03X] Battery current:  %.1f A\n",
                       frame.can_id, decode_battery_current(frame.data));
                break;

            case CAN_ID_INVERTER_TEMP:
                printf("[0x%03X] Inverter temp:    %.1f C\n",
                       frame.can_id, decode_inverter_temp(frame.data));
                break;

            case CAN_ID_IMU_ACCEL: {
                vec3_t a = decode_imu_accel(frame.data);
                printf("[0x%03X] IMU accel:        x=%.3f y=%.3f z=%.3f g\n",
                       frame.can_id, a.x, a.y, a.z);
                break;
            }

            case CAN_ID_IMU_GYRO: {
                vec3_t g = decode_imu_gyro(frame.data);
                printf("[0x%03X] IMU gyro:         x=%.2f y=%.2f z=%.2f deg/s\n",
                       frame.can_id, g.x, g.y, g.z);
                break;
            }

            default:
                /* Unknown ID - ignore, just like a real ECU would */
                break;
        }
    }

    close(sock);
    return 0;
}
