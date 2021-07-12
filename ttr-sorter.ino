/*
 * doser test
 * Bryce Dombrowski
 * 2021-06-13
 * 
 * Based on these references: 
 * - https://www.youtube.com/watch?v=KM-5PYfRlso&ab_channel=HowToElectronics
 * - Code: https://how2electronics.com/control-stepper-motor-with-a4988-driver-arduino/#Stepper_Motor_Acceleration_038_deceleration_Code
 * - adafruit rgb sensor library example code
 */


// 12V ~0.25A draw observed when alternating the stepper motors


#include <Wire.h>
#include "Adafruit_TCS34725.h"

/* Example code for the Adafruit TCS34725 breakout library */

/* Connect SCL    to analog 5
   Connect SDA    to analog 4
   Connect VDD    to 3.3V DC
   Connect GROUND to common ground */

/* Initialise with default values (int time = 2.4ms, gain = 1x) */
// Adafruit_TCS34725 tcs = Adafruit_TCS34725();

/* Initialise with specific int time and gain values */
//Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_60MS, TCS34725_GAIN_1X);

#define PIN_RED   5
#define PIN_GREEN 6
#define PIN_BLUE  9
#define PIN_STEP_DOSER 7
#define PIN_DIR_DOSER 8
#define PIN_STEP_CHUTE 14
#define PIN_DIR_CHUTE 15
#define STEPS_PER_REV 200 // usually 200

void setColourRGB (int r, int g, int b, int delayTime=0){ // r,g,b values should be in range 0-255
  // Note: analogWrite() initiates a PWM signal to control the brightness of each colour, by modifying the average voltage. The pins used here must be PWM.
  analogWrite(PIN_RED, r*4);
  analogWrite(PIN_GREEN, g*4);
  analogWrite(PIN_BLUE, b*4);
  delay(delayTime);
}

void pickColourFromRGBC(uint16_t r, uint16_t g, uint16_t b, uint16_t c, uint16_t colourTemp, uint16_t lux){
  if (lux>50000){
    setColourRGB(255,0,0); // Red
    Serial.print("RED");
  } else if (colourTemp>8000){
    setColourRGB(0,0,255); // Blue
    Serial.print("BLUE");
  } else if (g<200){
    setColourRGB(0,0,0); // Off
    Serial.print("BLACK");
  } else if (colourTemp>2900){
    setColourRGB(0,255,0); // Green
    Serial.print("GREEN");
  } else {
    setColourRGB(255,120,0); // Yellow
    Serial.print("YELLOW");
  }
}

void updateColour(){
  uint16_t r, g, b, c, colorTemp, lux;

  tcs.getRawData(&r, &g, &b, &c);
  // colorTemp = tcs.calculateColorTemperature(r, g, b);
  colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs.calculateLux(r, g, b);
  pickColourFromRGBC(r,g,b,c, colorTemp,lux);
}

void setup(void) {
  Serial.begin(9600);

  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_DIR_DOSER, OUTPUT);
  pinMode(PIN_STEP_DOSER, OUTPUT);
  pinMode(PIN_DIR_CHUTE, OUTPUT);
  pinMode(PIN_STEP_CHUTE, OUTPUT);
  // Now we're ready to get readings!
}

void loop(void) {
  
  // Set motor direction clockwise
  digitalWrite(PIN_DIR_DOSER, LOW);
 
  // Spin motor slowly
  for(int x = 0; x < STEPS_PER_REV/4; x++)
  {
    digitalWrite(PIN_STEP_DOSER, HIGH);
    delayMicroseconds(1000);
    digitalWrite(PIN_STEP_DOSER, LOW);
    delayMicroseconds(1000);
    //updateColour();
  }
  updateColour();
  /*for(int x = 0; x < 10; x++){
    updateColour();
    delay(100);
  }*/
  digitalWrite(PIN_DIR_CHUTE, LOW);
  for(int x = 0; x < STEPS_PER_REV/8; x++)
  {
    digitalWrite(PIN_STEP_CHUTE, HIGH);
    delayMicroseconds(2000);
    digitalWrite(PIN_STEP_CHUTE, LOW);
    delayMicroseconds(2000);
    //updateColour();
  }
  delay(200);
  digitalWrite(PIN_DIR_CHUTE, HIGH);
  for(int x = 0; x < STEPS_PER_REV/8; x++)
  {
    digitalWrite(PIN_STEP_CHUTE, HIGH);
    delayMicroseconds(2000);
    digitalWrite(PIN_STEP_CHUTE, LOW);
    delayMicroseconds(2000);
    //updateColour();
  }
  //delay(200); // Wait a second
  /*
  // Set motor direction counterclockwise
  digitalWrite(PIN_DIR, HIGH);
 
  // Spin motor quickly
  for(int x = 0; x < STEPS_PER_REV/4; x++)
  {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(1000);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(1000);
    //updateColour();
  }
  updateColour();
  delay(200); // Wait a second
  */
}
