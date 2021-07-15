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

#define MANUAL_FEED_MODE 1 // if defined, sets the dosing wheel to only accept the next piece when a button is pressed.
#ifdef MANUAL_FEED_MODE
  const int PIN_MANUAL_FEED_BUTTON = 10;
#endif
  
// Pins for the RGB LED
const int PIN_LED_RED   = 5;
const int PIN_LED_GREEN = 6;
const int PIN_LED_BLUE  = 9;
const int PIN_COLOUR_INTERRUPT = 7;

// Pins for the colour sensor dosing wheel stepper motor
const int PIN_STEP_DOSER = A2;
const int PIN_DIR_DOSER  = A3;
const int PIN_LIMITSWITCH_DOSER = 14;
// Pins for the rotating chute stepper motor
const int PIN_STEP_CHUTE = A0;
const int PIN_DIR_CHUTE  = A1;
const int PIN_LIMITSWITCH_CHUTE = 16;
const int STEPS_PER_REV = 200; // Dictates the number of steps per revolution of the motor. Usually is 200.

AccelStepper doserStepper(AccelStepper::DRIVER, PIN_STEP_DOSER, PIN_DIR_DOSER);
AccelStepper chuteStepper(AccelStepper::DRIVER, PIN_STEP_CHUTE, PIN_DIR_CHUTE);

cppQueue PIECE_QUEUE (sizeof(ColourEnum), /*queue length = */3, FIFO); // Instantiate queue for the colour pieces in the scooper mechanism

int getChuteAngle(ColourEnum pieceColour){
  // Returns the chute angle (in steps) for the given piece colour
  switch(pieceColour){
    case Red:
      return 50;
    break;
    case Green:
      return 35;
    break;
    case Blue:
      return 25;
    break;
    case Yellow:
      return 15;
    break;
    case Black:
    default: // default case is black
      return 0;
  }
}

void zeroStepperMotors(bool zeroDoserStepper = true, bool zeroChuteStepper = true){
  // Locates the zero point of each stepper motor so that they are aligned for their task.
     
  if (zeroDoserStepper){
    // Zero the doser stepper motor
    doserStepper.setSpeed(float(STEPS_PER_REV/16)); // Set the searching speed in steps per second (12.5)
    while(digitalRead(PIN_LIMITSWITCH_DOSER)==HIGH) {
     doserStepper.runSpeed(); // Move at constant searching speed.
    }
    const long doserOffset = 0L; // Offset from where the limit switch is triggered to where the zero point is.
    doserStepper.move(long(doserOffset)); // Make a relative movement to get to zero position.
    doserStepper.setMaxSpeed(STEPS_PER_REV/10); // 20 steps per second.
    doserStepper.runToPosition(); // Blocks while the motor moves.
    doserStepper.setCurrentPosition(0L); // Zero out the position.
  }
  if ( zeroChuteStepper){
    // Zero the chute stepper motor
    chuteStepper.setSpeed(float(STEPS_PER_REV/16)); // Set the searching speed in steps per second (12.5)
    while(digitalRead(PIN_LIMITSWITCH_CHUTE)==HIGH) {
     chuteStepper.runSpeed(); // Move at constant searching speed.
    }
    const long chuteOffset = 0L; // Offset from where the limit switch is triggered to where the zero point is.
    chuteStepper.move(long(chuteOffset)); // Make a relative movement to get to zero position.
    chuteStepper.setMaxSpeed(STEPS_PER_REV/10); // 20 steps per second.
    chuteStepper.runToPosition(); // Blocks while the motor moves.
    chuteStepper.setCurrentPosition(0L); // Zero out the position.
  }
}

void setup(void) {
  // This is where pins are assigned and setup takes place.
  Serial.begin(9600);
  
  colourSensorSetup(PIN_COLOUR_INTERRUPT, PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE); // This function will block indefinitely if the colour sensor is not found.
  pinMode(PIN_DIR_DOSER, OUTPUT);
  pinMode(PIN_STEP_DOSER, OUTPUT);
  pinMode(PIN_LIMITSWITCH_DOSER, INPUT_PULLUP);
  pinMode(PIN_DIR_CHUTE, OUTPUT);
  pinMode(PIN_STEP_CHUTE, OUTPUT);
  pinMode(PIN_LIMITSWITCH_CHUTE, INPUT_PULLUP);
  
  zeroStepperMotors();
  #ifdef MANUAL_FEED_MODE
    pinMode(PIN_MANUAL_FEED_BUTTON, INPUT_PULLUP);
  #endif
}

void loop(void) {
  // This is the main control loop
  
  if ( colourSensorReady() ){
    // Make sure we always have the latest reading stored and ready for when we want to categorize a game piece.
    // We will end up throwing out most of these readings, but it makes sure we capture the piece we want.
    updateColourRaw();
  }
  
  // Need to call .run() as often as possible. Each call results in at most one step.
  if(!chuteStepper.run()){ // Move the chute at most one step.
    // Runs the following code if the chute is at the commanded position.
    if(!doserStepper.isRunning()){
      #ifdef MANUAL_FEED_MODE
        while(digitalRead(PIN_MANUAL_FEED_BUTTON)==HIGH);
      #endif
      doserStepper.move(STEPS_PER_REV/4); // Command the dosing wheel one quarter turn to drop the queued game piece.
    }
  }
  
  if(!doserStepper.run()){ // Move the dosing wheel at most one step.
    // Runs the following code if the doser is at the commanded position.

    // Store the colour of the next game piece in the FIFO queue.
    ColourEnum mappedColour = getColour(); 
    PIECE_QUEUE.push(&mappedColour);
    
    if (PIECE_QUEUE.getCount()<3){ 
      // Command the dosing wheel one quarter turn to queue another game piece.
      #ifdef MANUAL_FEED_MODE
        while(digitalRead(PIN_MANUAL_FEED_BUTTON)==HIGH);
      #endif
      doserStepper.move(STEPS_PER_REV/4);
    } else {
      // Game piece at the end of the queue has now fallen out the bottom of the dosing mechanism, discard its info.
      ColourEnum temp;
      PIECE_QUEUE.pop(&temp); 
    }

    // Command the chute to the required position for the next game piece in the queue
    ColourEnum nextColour;
    bool success = PIECE_QUEUE.peek(&nextColour); // ?????? not sure if PIECE_QUEUE.peek() does what I think it does
    if (success){
      chuteStepper.moveTo(getChuteAngle(nextColour)); // "moveTo()" sets the absolute destination, where "move()" sets the relative destination.
    }
  }

}
