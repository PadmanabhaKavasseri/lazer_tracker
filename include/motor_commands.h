#ifndef MOTOR_COMMANDS_H
#define MOTOR_COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <glib.h>

// Arduino connection constants
extern const char* ARDUINO_PORT;
extern int arduino_fd;

// Function declarations
int map_to_servo_angle(float normalized_coord, int image_dimension);
int init_arduino_serial(void);
void send_arduino_command(int pan_angle, int tilt_angle);
void send_arduino_command_throttled(int pan_angle, int tilt_angle);
void cleanup_arduino_serial(void);

#endif // MOTOR_COMMANDS_H
