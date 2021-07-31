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
 * 
 * Notes:
 *   12V ~0.25A draw observed when alternating the stepper motors
 *   Vref set to 0.72V according to the fomrula Vref = 2.5*I
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
const int STEPS_PER_REV = 1600; // Dictates the number of steps per revolution of the motor. Usually is 200.

AccelStepper doserStepper(AccelStepper::DRIVER, PIN_STEP_DOSER, PIN_DIR_DOSER);
AccelStepper chuteStepper(AccelStepper::DRIVER, PIN_STEP_CHUTE, PIN_DIR_CHUTE);

cppQueue PIECE_QUEUE (sizeof(ColourEnum), /*queue length = */3, FIFO); // Instantiate queue for the colour pieces in the scooper mechanism

int getChuteAngle(ColourEnum pieceColour){
  // Returns the chute angle (in steps) for the given piece colour
  // If facing the machine, the positive direction of rotation is clockwise. The datum is set at 45 degrees CCW from a line pointing from the chute to the ground. (AKA the 4:30 position of an analog clock).
  const int quarterTurn = STEPS_PER_REV/4; // The full travel of the chute is one quarter turn (90 degrees).
  switch(pieceColour){
    case Red:
      return int(quarterTurn * 0.83); // 83% of a quarter turn
    break;
    case Green:
      return int(quarterTurn * 0.62); // 62% of a quarter turn
    break;
    case Blue:
      return int(quarterTurn * 0.42); // 42% of a quarter turn
    break;
    case Yellow:
      return int(quarterTurn * 0.21); // 21% of a quarter turn
    break;
    case Black:
    default: // default case is black
      return 0; // 0 degrees
  }
}

void zeroStepperMotors(bool zeroDoserStepper = true, bool zeroChuteStepper = true){
  // Locates the zero point of each stepper motor so that they are aligned for their task.
     
  if (zeroDoserStepper){
    // Zero the doser stepper motor
    doserStepper.setPinsInverted(true); // Set the direction pin to be inverted.
    doserStepper.setMaxSpeed(STEPS_PER_REV*8); // 20 steps per second.
    doserStepper.setAcceleration(STEPS_PER_REV*8); // X steps /s /s
    doserStepper.setSpeed(float(STEPS_PER_REV/16)); // Set the searching speed in steps per second (12.5)
    while(digitalRead(PIN_LIMITSWITCH_DOSER)==HIGH) {
     doserStepper.runSpeed(); // Move at constant searching speed.
    }
    const long doserOffset = 0L; // Offset from where the limit switch is triggered to where the zero point is.
    doserStepper.move(doserOffset); // Make a relative movement to get to zero position.
    
    doserStepper.runToPosition(); // Blocks while the motor moves.
    doserStepper.setCurrentPosition(0L); // Zero out the position.
  }
  if ( zeroChuteStepper){
    // Zero the chute stepper motor
    chuteStepper.setMaxSpeed(STEPS_PER_REV*8); // 20 steps per second.
    chuteStepper.setAcceleration(STEPS_PER_REV*8); // X steps /s /s
    chuteStepper.setSpeed(-float(STEPS_PER_REV/100)); // Set the searching speed in steps per second (12.5). This is in the "negative" direction.
    while(digitalRead(PIN_LIMITSWITCH_CHUTE)==HIGH) {
     chuteStepper.runSpeed(); // Move at constant searching speed.
    }
    const long chuteOffset = 0L; // Offset from where the limit switch is triggered to where the zero point is.
    chuteStepper.move(chuteOffset); // Make a relative movement to get to zero position.
    
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

  // Test all the positions
  chuteStepper.runToNewPosition(getChuteAngle(Red));
  delay(1000);
  chuteStepper.runToNewPosition(getChuteAngle(Black));
  delay(1000);
  chuteStepper.runToNewPosition(getChuteAngle(Green));
  delay(1000);
  chuteStepper.runToNewPosition(getChuteAngle(Blue));
  delay(1000);
  chuteStepper.runToNewPosition(getChuteAngle(Yellow));
  delay(1000);

  // Flash RGB LED green twice to show we're ready
  setColourRGB(0,255,0, /*delayTime=*/500);
  setColourRGB(0,0,0, /*delayTime=*/500);
  setColourRGB(0,255,0, /*delayTime=*/500);
  setColourRGB(0,0,0);
}

void loop(void) {
  // This is the main control loop
  
  if ( colourSensorReady() ){
    // Make sure we always have the latest reading stored and ready for when we want to categorize a game piece.
    // We will end up throwing out most of these readings, but it makes sure we capture the piece we want.
    // The colour sensor takes some time to make a measurement, so doing it this way means we get immediate access to the latest values whenever it is ready.
    updateColourRaw();
  }

  if(!doserStepper.run() && !chuteStepper.run()){ // Move each stepper motor at most one step according to the calculated acceleration and speed.
    // Runs the following code if both stepper motors are at their commanded positions.

    // Store the colour of the next game piece in the FIFO queue.
    ColourEnum mappedColour = getColour(); 
    PIECE_QUEUE.push(&mappedColour);
    
    if (PIECE_QUEUE.getCount() >= 3){ 
      // Game piece in the third position of the dosing wheel has now fallen out the bottom of the dosing mechanism, discard its info.
      ColourEnum temp;
      PIECE_QUEUE.pop(&temp); 
    }

    // Command the chute to the required position for the next game piece in the queue
    ColourEnum nextColour;
    bool success = PIECE_QUEUE.peek(&nextColour); // ?????? not sure if PIECE_QUEUE.peek() does what I think it does. I am trying to look at the data at the end of the FIFO queue without modifiying it.
    if (success){
      chuteStepper.moveTo(getChuteAngle(nextColour)); // "moveTo()" sets the absolute destination, where "move()" sets the relative destination.
    }
    doserStepper.move(STEPS_PER_REV/4); // Command the dosing wheel one quarter turn to drop the next queued game piece.

    #ifdef MANUAL_FEED_MODE
      while(digitalRead(PIN_MANUAL_FEED_BUTTON)==HIGH){
          if ( colourSensorReady() ){
            // Make sure we always have the latest reading stored and ready for when we want to categorize a game piece.
            // We will end up throwing out most of these readings, but it makes sure we capture the piece we want.
            updateColourRaw();
            getColour(true); // This updates the RGB LED. The argument "true" means the function will print debug mode values.
          }
      }
    #endif
  }
}
