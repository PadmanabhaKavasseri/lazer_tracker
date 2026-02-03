#include <Servo.h>

Servo panServo;   // Pin 9 - horizontal movement
Servo tiltServo;  // Pin 10 - vertical movement

// Servo angle limits (adjust as needed)
const int MIN_ANGLE = 45;
const int MAX_ANGLE = 135;
const int CENTER_ANGLE = 90;

void setup() {
  Serial.begin(9600);
  panServo.attach(9);
  tiltServo.attach(10);
  
  // Start at center position
  panServo.write(CENTER_ANGLE);
  tiltServo.write(CENTER_ANGLE);
  
  Serial.println("Servo controller ready!");
  Serial.println("Send: pan_angle,tilt_angle");
  Serial.println("Example: 90,90");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Parse "pan,tilt" format
    int commaIndex = input.indexOf(',');
    if (commaIndex > 0) {
      int panAngle = input.substring(0, commaIndex).toInt();
      int tiltAngle = input.substring(commaIndex + 1).toInt();
      
      // Constrain angles to safe range
      panAngle = constrain(panAngle, MIN_ANGLE, MAX_ANGLE);
      tiltAngle = constrain(tiltAngle, MIN_ANGLE, MAX_ANGLE);
      
      // Move servos
      panServo.write(panAngle);
      tiltServo.write(tiltAngle);
      
      Serial.print("Moved to: ");
      Serial.print(panAngle);
      Serial.print(",");
      Serial.println(tiltAngle);
    } else {
      Serial.println("Invalid format. Use: pan,tilt");
    }
  }
}
