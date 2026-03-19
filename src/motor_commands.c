#include "motor_commands.h"
#include <math.h>

// Calibration data (static to keep it private to this file)
static CalibrationPoint calibration_points[4] = {
    {0.182, 0.441, +6, -5},    // Bottom-left (was top-left)
    {0.749, 0.466, -10, -6.7}, // Bottom-right (was top-right)
    {0.199, 0.782, +6, +6},   // Top-left (was bottom-left)
    {0.730, 0.807, -10, +6}   // Top-right (was bottom-right)
};

// Global variables
const char* ARDUINO_PORT = "/dev/ttyACM0";
int arduino_fd = -1;

// Rate limiting variables
static time_t last_command_time = 0;
static const int MIN_COMMAND_INTERVAL_MS = 100;  // 10 Hz max

void apply_calibration_correction(float center_x, float center_y, int *pan_angle, int *tilt_angle) {
    printf("[DEBUG] Input coordinates: (%.6f, %.6f)\n", center_x, center_y);
    printf("[DEBUG] Before correction: Pan=%d, Tilt=%d\n", *pan_angle, *tilt_angle);
    
    // Define the bounding box of your calibration area
    float min_x = 0.182;    // Leftmost
    float max_x = 0.749;    // Rightmost  
    float min_y = 0.441;    // Bottom (smaller Y)
    float max_y = 0.807;    // Top (larger Y)
    
    // Normalize coordinates within calibration area
    float norm_x = (center_x - min_x) / (max_x - min_x);
    float norm_y = (center_y - min_y) / (max_y - min_y);

    printf("[DEBUG] Normalized: (%.3f,%.3f)\n", norm_x, norm_y);
    
    // Clamp to bounds
    norm_x = fmax(0.0, fmin(1.0, norm_x));
    norm_y = fmax(0.0, fmin(1.0, norm_y));
    
    // Bilinear interpolation for pan offset
    float top_pan = calibration_points[0].pan_offset * (1.0 - norm_x) + 
                    calibration_points[1].pan_offset * norm_x;
    float bottom_pan = calibration_points[2].pan_offset * (1.0 - norm_x) + 
                       calibration_points[3].pan_offset * norm_x;
    float pan_offset = top_pan * (1.0 - norm_y) + bottom_pan * norm_y;
    
    // Bilinear interpolation for tilt offset
    float top_tilt = calibration_points[0].tilt_offset * (1.0 - norm_x) + 
                     calibration_points[1].tilt_offset * norm_x;
    float bottom_tilt = calibration_points[2].tilt_offset * (1.0 - norm_x) + 
                        calibration_points[3].tilt_offset * norm_x;
    float tilt_offset = top_tilt * (1.0 - norm_y) + bottom_tilt * norm_y;

    printf("[DEBUG] Top pan: %.1f, Bottom pan: %.1f\n", top_pan, bottom_pan);
    printf("[DEBUG] Top tilt: %.1f, Bottom tilt: %.1f\n", top_tilt, bottom_tilt);
    printf("[DEBUG] Final offsets: Pan=%.1f, Tilt=%.1f\n", pan_offset, tilt_offset);
    
    // Apply corrections
    *pan_angle += (int)round(pan_offset);
    *tilt_angle += (int)round(tilt_offset);
    
    printf("[DEBUG] After correction: Pan=%d, Tilt=%d\n", *pan_angle, *tilt_angle);

    printf("[CALIBRATION] Position (%.3f,%.3f) -> Pan offset: %.1f, Tilt offset: %.1f\n", 
           center_x, center_y, pan_offset, tilt_offset);
}

int map_to_servo_angle(float normalized_coord, int image_dimension) {
    // Convert normalized coordinate (0.0-1.0) to pixel coordinate
    float pixel_coord = normalized_coord * image_dimension;
    
    // Use wider range: 20°-150° (120° span instead of 90°)
    int servo_angle = 10 + (int)((pixel_coord / image_dimension) * 160);
    
    // Constrain to safe servo range
    if (servo_angle < 10) servo_angle = 10;
    if (servo_angle > 170) servo_angle = 170;
    
    return servo_angle;
}

int map_to_pan_angle(float normalized_x) {
    // normalized_x: 0.0 = left, 1.0 = right
    // Servo: 125 = left, 61 = right
    int angle = LASER_PAN_LEFT + (int)((LASER_PAN_RIGHT - LASER_PAN_LEFT) * normalized_x);
    
    // Clamp to bounds
    // if (angle > LASER_PAN_LEFT) angle = LASER_PAN_LEFT;
    // if (angle < LASER_PAN_RIGHT) angle = LASER_PAN_RIGHT;
    
    return angle;
}

int map_to_tilt_angle(float normalized_y) {
    // normalized_y: 0.0 = bottom, 1.0 = top
    // Servo: 83 = bottom, 125 = top
    int angle = LASER_TILT_BOTTOM + (int)((LASER_TILT_TOP - LASER_TILT_BOTTOM) * normalized_y);
    
    // Clamp to bounds
    // if (angle > LASER_TILT_TOP) angle = LASER_TILT_TOP;
    // if (angle < LASER_TILT_BOTTOM) angle = LASER_TILT_BOTTOM;
    
    return angle;
}


int init_arduino_serial(void) {
    // Configure serial port
    system("stty -F /dev/ttyACM0 115200 raw -echo");
    
    // Open serial port for writing
    arduino_fd = open(ARDUINO_PORT, O_WRONLY | O_NOCTTY | O_NONBLOCK);
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

void send_arduino_command_binary(int pan_angle, int tilt_angle) {
    if (arduino_fd < 0) return;
    
    // Send 3 bytes: [0xFF][pan][tilt]
    unsigned char command[3] = {
        0xFF,                           // Start marker
        (unsigned char)pan_angle,       // Pan (45-135 fits in 1 byte)
        (unsigned char)tilt_angle       // Tilt (45-135 fits in 1 byte)
    };
    
    write(arduino_fd, command, 3);
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
