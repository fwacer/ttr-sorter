/*
 * Ticket to Ride: Rails and Sails Game Piece Sorter
 * This code is part of my MECH 400 Capstone project.
 * Bryce Dombrowski
 * 2021-07-13
 * 
 * Based on these references: 
 * - https://www.youtube.com/watch?v=KM-5PYfRlso&ab_channel=HowToElectronics
 * - Code: https://how2electronics.com/control-stepper-motor-with-a4988-driver-arduino/#Stepper_Motor_Acceleration_038_deceleration_Code
 * - adafruit rgb sensor library example code
 * 
 * 
 * http://www.airspayce.com/mikem/arduino/AccelStepper/index.html
 * https://www.pjrc.com/teensy/td_libs_AccelStepper.html
 */

// 12V ~0.25A draw observed when alternating the stepper motors

#include <AccelStepper.h>
#include <cppQueue.h> // https://github.com/SMFSW/Queue
#include "colour-detection.h"

// Pins for the RGD LED
#define PIN_LED_RED   5
#define PIN_LED_GREEN 6
#define PIN_LED_BLUE  9

#define PIN_COLOUR_INTERRUPT 7

// Pins for the colour sensor dosing wheel stepper motor
#define PIN_STEP_DOSER A0 //7
#define PIN_DIR_DOSER A1 //8
// Pins for the rotating chute stepper motor
#define PIN_STEP_CHUTE A2 //14
#define PIN_DIR_CHUTE A3 //15

#define STEPS_PER_REV 200 // Dictates the number of steps per revolution of the motor. Usually is 200.

volatile ColourEnum MAPPED_COLOUR;
bool READY_FOR_NEXT_PIECE = true;
AccelStepper doserStepper(AccelStepper::DRIVER, PIN_STEP_DOSER, PIN_DIR_DOSER);
AccelStepper chuteStepper(AccelStepper::DRIVER, PIN_STEP_CHUTE, PIN_DIR_CHUTE);

cppQueue PIECE_QUEUE (sizeof(ColourEnum), /*queue length = */4, FIFO); // Instantiate queue for the colour pieces in the scooper mechanism

void zeroMotors(){
  // Locates the zero point of each stepper motor so that they are aligned for their task
  // To do later
  /*

   * // Zero the doser stepper motor
   * doserStepper.setSpeed(float(STEPS_PER_REV/8)); // Steps per second (25)
   * while(LIMIT_SWITCH_NOT_PRESSED (rising or falliong edge) ) {
   *   doserStepper.runSpeed(); // Move at constant speed
   * }
   * const long doserOffset = 0L; // Offset from where the limit switch is triggered to where the zero point is
   * doserStepper.move(long(doserOffset)); // Make a relative movement to get to zero position
   * doserStepper.runToPosition(); // Blocks while the motor moves
   * doserStepper.setCurrentPosition(0L); // Zero out the position
   * 
   * // Zero the chute stepper motor
   * chuteStepper.setSpeed(float(STEPS_PER_REV/16)); // Steps per second (12.5)
   * while(LIMIT_SWITCH_NOT_PRESSED (rising or falliong edge) ) {
   *   chuteStepper.runSpeed(); // Move at constant speed
   * }
   * const long chuteOffset = 0L; // Offset from where the limit switch is triggered to where the zero point is
   * chuteStepper.move(long(doserOffset)); // Make a relative movement to get to zero position
   * chuteStepper.runToPosition(); // Blocks while the motor moves
   * chuteStepper.setCurrentPosition(0L); // Zero out the position
   */
}

void setup(void) {
  Serial.begin(9600);
  
  colourSensorSetup(PIN_COLOUR_INTERRUPT, PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE); // This function will block indefinitely if the colour sensor is not found.
  pinMode(PIN_DIR_DOSER, OUTPUT);
  pinMode(PIN_STEP_DOSER, OUTPUT);
  pinMode(PIN_DIR_CHUTE, OUTPUT);
  pinMode(PIN_STEP_CHUTE, OUTPUT);
  zeroMotors();
}

void loop(void) {

  // Need to call these as often as possible. Each call results in at most one step.
  if(!chuteStepper.run()){ // Move the chute
    // Runs the following code if the chute is at the desired position

    MAPPED_COLOUR = getColour();
    if(!doserStepper.run()){
    // Runs the following code if the doser is at the desired position
      // LOGIC TO CHECK FOR IF PART HAS FALLEN THROUGH CHUTE
      // PIECE_QUEUE.pop(&MAPPED_COLOUR);
      PIECE_QUEUE.push(&MAPPED_COLOUR);
      nextColour = PIECE_QUEUE.peek(); // ??????
      chuteStepper.moveTo(chuteAngle[nextColour]);
    }
  }

}
