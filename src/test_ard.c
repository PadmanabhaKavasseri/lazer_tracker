
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

int arduino_fd = -1;

int init_arduino() {
    struct termios tty;
    
    arduino_fd = open("/dev/ttyACM0", O_WRONLY | O_NOCTTY);
    if (arduino_fd < 0) {
        perror("Error opening /dev/ttyACM0");
        return -1;
    }
    
    // Configure serial port for 115200 baud, raw mode
    tcgetattr(arduino_fd, &tty);
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    tty.c_cflag = CS8 | CREAD | CLOCAL;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    
    tcsetattr(arduino_fd, TCSANOW, &tty);
    return 0;
}

void send_motor_command(char motor, int angle) {
    if (arduino_fd < 0) return;
    
    // Static variables to remember last positions
    static int last_pan = 90;   // Default starting position
    static int last_tilt = 90;  // Default starting position
    
    unsigned char command[3];
    
    if (motor == 'b') {
        // Bottom motor (pan) - update pan, keep last tilt
        last_pan = angle;
        command[0] = 0xFF;
        command[1] = (unsigned char)last_pan;
        command[2] = (unsigned char)last_tilt;  // Use stored tilt value
    } else if (motor == 't') {
        // Top motor (tilt) - update tilt, keep last pan  
        last_tilt = angle;
        command[0] = 0xFF;
        command[1] = (unsigned char)last_pan;   // Use stored pan value
        command[2] = (unsigned char)last_tilt;
    } else {
        printf("Invalid motor. Use 'b' for bottom or 't' for top\n");
        return;
    }
    
    write(arduino_fd, command, 3);
    printf("Sent: %c%d -> [0xFF] [%d] [%d] (Pan: %d, Tilt: %d)\n", 
           motor, angle, command[1], command[2], last_pan, last_tilt);
}


int main() {
    char input[100];
    char motor;
    int angle;
    
    if (init_arduino() != 0) {
        return -1;
    }
    
    printf("Arduino Motor Control\n");
    printf("Commands: b45 (bottom motor), t90 (top motor), q (quit)\n");
    printf("Angle range: 0-180\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        // Check for quit
        if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        
        // Parse input: b45, t90, etc.
        if (sscanf(input, "%c%d", &motor, &angle) == 2) {
            if (angle >= 0 && angle <= 180) {
                send_motor_command(motor, angle);
            } else {
                printf("Angle must be between 0 and 180\n");
            }
        } else {
            printf("Invalid format. Use: b45 or t90\n");
        }
    }
    
    if (arduino_fd >= 0) {
        close(arduino_fd);
    }
    
    printf("Goodbye!\n");
    return 0;
}
