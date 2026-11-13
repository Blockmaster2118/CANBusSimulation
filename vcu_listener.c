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
            case CAN_ID_BATTERY_TEMP: {
                battery_temp_t b = decode_battery_temp(frame.data);
                printf("[0x%03X] Battery temp:      max %.1f / min %.1f / avg %.1f C\n",
                       frame.can_id, b.max_c, b.min_c, b.avg_c);
                if (b.max_c > 55.0f) {
                    printf("         WARNING: battery over-temp, consider derating power\n");
                }
                break;
            }

            case CAN_ID_LEAN_IMU: {
                lean_imu_t l = decode_lean_imu(frame.data);
                printf("[0x%03X] Lean IMU:          lean_raw=%d  accel x=%.3f y=%.3f z=%.3f g\n",
                       frame.can_id, l.lean_angle_raw, l.ax, l.ay, l.az);
                break;
            }

            case CAN_ID_POWERTRAIN_RPM:
                printf("[0x%03X] Powertrain RPM:    %u (raw)\n",
                       frame.can_id, decode_powertrain_rpm_raw(frame.data));
                break;

            case CAN_ID_REAR_WHEEL_SPEED:
                printf("[0x%03X] Rear wheel speed:  %.1f km/h\n",
                       frame.can_id, decode_rear_wheel_speed(frame.data));
                break;

            case CAN_ID_FRONT_WHEEL_SPEED:
                printf("[0x%03X] Front wheel speed: %.1f km/h\n",
                       frame.can_id, decode_front_wheel_speed(frame.data));
                break;

            case CAN_ID_BRAKE_PRESSURE: {
                brake_pressure_t b = decode_brake_pressure(frame.data);
                printf("[0x%03X] Brake pressure:    front %u / rear %u (raw)\n",
                       frame.can_id, b.front_raw, b.rear_raw);
                break;
            }

            case CAN_ID_COOLING_TEMP: {
                cooling_temp_t c = decode_cooling_temp(frame.data);
                printf("[0x%03X] Coolant in/out:    %.1f C / %.1f C\n",
                       frame.can_id, c.in_c, c.out_c);
                break;
            }

            case CAN_ID_MOTOR_TEMP:
                printf("[0x%03X] Motor temp:        %.1f C\n",
                       frame.can_id, decode_motor_temp(frame.data));
                break;

            case CAN_ID_INVERTER_TEMP:
                printf("[0x%03X] Inverter temp:     %.1f C\n",
                       frame.can_id, decode_inverter_temp(frame.data));
                break;

            case CAN_ID_RADIATOR_TEMP:
                printf("[0x%03X] Radiator temp:     %d (raw)\n",
                       frame.can_id, decode_radiator_temp_raw(frame.data));
                break;

            case CAN_ID_SUSPENSION_TRAVEL:
                printf("[0x%03X] Front susp travel: %u (raw)\n",
                       frame.can_id, decode_suspension_travel_raw(frame.data));
                break;

            case CAN_ID_MOTOR_RPM:
                printf("[0x%03X] Motor RPM:         %.0f rpm\n",
                       frame.can_id, decode_motor_rpm(frame.data));
                break;

            case CAN_ID_COOLANT_PUMP_RPM:
                printf("[0x%03X] Coolant pump RPM:  %u (raw)\n",
                       frame.can_id, decode_coolant_pump_rpm_raw(frame.data));
                break;

            case CAN_ID_COOLANT_PUMP_FLOW:
                printf("[0x%03X] Coolant flow:      %u (raw)\n",
                       frame.can_id, decode_coolant_flow_raw(frame.data));
                break;

            case CAN_ID_INDIVIDUAL_BATT_TEMP:
                printf("[0x%03X] Cell/indiv. temp:  %d (raw)\n",
                       frame.can_id, decode_individual_batt_temp_raw(frame.data));
                break;

            case CAN_ID_BATTERY_VOLTAGE:
                printf("[0x%03X] Battery voltage:   %.1f V\n",
                       frame.can_id, decode_battery_voltage(frame.data));
                break;

            case CAN_ID_BATTERY_CURRENT:
                printf("[0x%03X] Battery current:   %.1f A\n",
                       frame.can_id, decode_battery_current(frame.data));
                break;

            default:
                /* Unknown ID - ignore, just like a real ECU would */
                break;
        }
    }

    close(sock);
    return 0;
}
