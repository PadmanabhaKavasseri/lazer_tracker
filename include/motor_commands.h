#ifndef MOTOR_COMMANDS_H
#define MOTOR_COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <glib.h>

#define LASER_PAN_OFFSET    2    // Horizontal offset in degrees (-10 to +10)
#define LASER_TILT_OFFSET   4   // Vertical offset in degrees (negative = higher)
#define LASER_MIN_ANGLE     0   // Minimum servo angle
#define LASER_MAX_ANGLE     180  // Maximum servo angle

// Pan motor bounds (left/right)
#define LASER_PAN_LEFT      125  // Leftmost position
#define LASER_PAN_RIGHT     61   // Rightmost position

// Tilt motor bounds (up/down)  
#define LASER_TILT_BOTTOM   83   // Bottom position
#define LASER_TILT_TOP      125  // Top position

// Calibration structure
typedef struct {
    float bbox_x, bbox_y;
    int pan_offset, tilt_offset;
} CalibrationPoint;

// Arduino connection constants
extern const char* ARDUINO_PORT;
extern int arduino_fd;

// Function declarations
void apply_calibration_correction(float center_x, float center_y, int *pan_angle, int *tilt_angle);
int map_to_servo_angle(float normalized_coord, int image_dimension);
int map_to_tilt_angle(float normalized_y);
int map_to_pan_angle(float normalized_x);
int init_arduino_serial(void);
void send_arduino_command(int pan_angle, int tilt_angle);
void send_arduino_command_binary(int pan_angle, int tilt_angle);
void send_arduino_command_throttled(int pan_angle, int tilt_angle);
void cleanup_arduino_serial(void);

#endif // MOTOR_COMMANDS_H
