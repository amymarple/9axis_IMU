#include <Servo.h>

Servo servoMotor;
int servoPosition = 0; // Initial position of the servo motor
int rotationStep = 10; // Amount to rotate the servo motor per iteration
int delayTime = 1000; // Delay time in milliseconds (5 seconds)
bool isIncreasing = true; // Flag to indicate the direction of rotation
int repetitions = 0; // Number of repetitions completed
int num_rep=10;
char incomingByte;
bool operation=false;
void setup() {
  Serial.begin(9600); // Initialize the serial communication
  servoMotor.attach(13); // Attach the servo motor to the specified pin
}

void loop() {
  unsigned long startTime = millis(); // Get the starting time of the iteration
if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    //open the pump
    if (incomingByte=='h')
    {
  // Rotate the servo motor by the specified step
  operation=true;

    }

         if (incomingByte=='l')
    {
       operation=false;
       repetitions = 0; // Number of repetitions completed
       servoPosition = 0;
    }
}
if (operation){
  if (isIncreasing) {
    servoPosition += rotationStep;
  } else {
    servoPosition -= rotationStep;
  }
  servoMotor.write(servoPosition);

  // Output the current timestamp and servo position
  Serial.print(millis());
  Serial.print(",");
  Serial.print(servoPosition);
   Serial.print(",");
   Serial.println(repetitions);
  // Delay for the specified time
  delay(delayTime);

  // Check if the servo has reached the maximum position (180 degrees)
  if (servoPosition >= 180) {
    // Set the flag to indicate decreasing rotation
    isIncreasing = false;
  } else if (servoPosition <= 0) {
    // Set the flag to indicate increasing rotation
    isIncreasing = true;
    repetitions++; // Increment the number of repetitions completed
  }

  // Check if the desired number of repetitions is completed
  if (repetitions >= num_rep) {
    while (true) {
      // Stay in an infinite loop to pause the program
      if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
     if (incomingByte=='l')
    {

       operation=false;
       repetitions = 0; // Number of repetitions completed
       repetitions=0;
       servoPosition = 0;
       break;
    }
      }
}
      
    }
  }
     }
      //close the pump
  
