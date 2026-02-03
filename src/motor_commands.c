#include "motor_commands.h"

// Global variables
const char* ARDUINO_PORT = "/dev/ttyACM0";
int arduino_fd = -1;

// Rate limiting variables
static time_t last_command_time = 0;
static const int MIN_COMMAND_INTERVAL_MS = 100;  // 10 Hz max

int map_to_servo_angle(float normalized_coord, int image_dimension) {
    // Convert normalized coordinate (0.0-1.0) to pixel coordinate
    float pixel_coord = normalized_coord * image_dimension;
    
    // Map pixel coordinate to servo angle (45°-135° range)
    // 0 pixels → 45°, max pixels → 135°, center → 90°
    int servo_angle = 45 + (int)((pixel_coord / image_dimension) * 90);
    
    // Constrain to safe servo range
    if (servo_angle < 45) servo_angle = 45;
    if (servo_angle > 135) servo_angle = 135;
    
    return servo_angle;
}

int init_arduino_serial(void) {
    // Configure serial port
    system("stty -F /dev/ttyACM0 9600 raw -echo");
    
    // Open serial port for writing
    arduino_fd = open(ARDUINO_PORT, O_WRONLY | O_NOCTTY);
    if (arduino_fd < 0) {
        g_printerr("Failed to open Arduino serial port: %s\n", ARDUINO_PORT);
        return -1;
    }
    
    // Give Arduino time to reset after serial connection
    sleep(2);
    
    printf("Arduino serial connection established on %s\n", ARDUINO_PORT);
    return 0;
}

void send_arduino_command(int pan_angle, int tilt_angle) {
    if (arduino_fd < 0) return;
    
    char command[32];
    snprintf(command, sizeof(command), "%d,%d\n", pan_angle, tilt_angle);
    
    ssize_t bytes_written = write(arduino_fd, command, strlen(command));
    if (bytes_written > 0) {
        printf("Sent to Arduino: %s", command);
    }
}

void send_arduino_command_throttled(int pan_angle, int tilt_angle) {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    time_t current_ms = current_time.tv_sec * 1000 + current_time.tv_nsec / 1000000;
    
    if (current_ms - last_command_time >= MIN_COMMAND_INTERVAL_MS) {
        send_arduino_command(pan_angle, tilt_angle);
        last_command_time = current_ms;
    }
}

void cleanup_arduino_serial(void) {
    if (arduino_fd >= 0) {
        close(arduino_fd);
        arduino_fd = -1;
        printf("Arduino serial connection closed\n");
    }
}
