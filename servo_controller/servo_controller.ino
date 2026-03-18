#include <Servo.h>

Servo panServo;   // Pin 9 - horizontal movement
Servo tiltServo;  // Pin 10 - vertical movement

const long SERIAL_BAUD_RATE = 115200;
// Servo angle limits (adjust as needed)
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;
const int CENTER_ANGLE = 90;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(1);  // Fast timeout
  panServo.attach(6);
  tiltServo.attach(10);
  
  // Start at center position
  panServo.write(CENTER_ANGLE);
  tiltServo.write(CENTER_ANGLE);
  
  Serial.println("Binary servo controller ready!");
  Serial.println("Expecting binary protocol: [0xFF][pan][tilt]");
}

void loop() {
  // Check if we have at least 3 bytes available
  if (Serial.available() >= 3) {
    
    // Read the start marker
    byte startMarker = Serial.read();
    
    if (startMarker == 0xFF) {
      // Read pan and tilt angles
      byte panAngle = Serial.read();
      byte tiltAngle = Serial.read();
      
      // Constrain to safe range (optional - could be done in C code)
      panAngle = constrain(panAngle, MIN_ANGLE, MAX_ANGLE);
      tiltAngle = constrain(tiltAngle, MIN_ANGLE, MAX_ANGLE);
      
      // Move servos immediately
      panServo.write(panAngle);
      tiltServo.write(tiltAngle);
      
      // Optional: minimal feedback (comment out for maximum speed)
      // Serial.print("B:");
      // Serial.print(panAngle);
      // Serial.print(",");
      // Serial.println(tiltAngle);
      
    } else {
      // Invalid start marker - could be noise or sync issue
      // Optionally flush buffer to resync
      while (Serial.available() && Serial.read() != 0xFF) {
        // Keep reading until we find a start marker or buffer is empty
      }
    }
  }
  
  // Buffer overflow protection - if buffer gets too full, clear it
  if (Serial.available() > 50) {
    while (Serial.available()) {
      Serial.read();
    }
    Serial.println("Buffer cleared");
  }
}
