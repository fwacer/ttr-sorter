/*
 * Colour sensor and RGB LED handling
 * This code is part of my MECH 400 Capstone project.
 * Bryce Dombrowski
 * 2021-07-13
 * 
 * Based on these references: 
 * - adafruit rgb sensor library example code
 * 
 */

#ifndef COLOUR_DETECTION_H
#define COLOUR_DETECTION_H

#include <Wire.h> // I2C library
#include "Adafruit_TCS34725.h" // Colour sensor library
/**
 * I2C protocol requires the following pins on an Arduino Pro Micro:
 * SDA : pin 2
 * SCL : pin 3
 * 
 * The red, blue, and green channels of the RGB LED must be controlled by PWM pins to produce an equivalent analog voltage between 0-5V. 
*/
// Global variables that are meant to be only used by this function. Pseudo-private.

uint16_t _PIN_LED_RED;
uint16_t _PIN_LED_GREEN;
uint16_t _PIN_LED_BLUE;
bool _ENABLE_RGB_LED = false;

typedef enum {None, Red, Green, Blue, Yellow, Black} ColourEnum;
ColourEnum _LAST_MEASURED_COLOUR = None;

volatile boolean _COLOUR_SENSOR_READY = false;

void isrColour()
{
  _COLOUR_SENSOR_READY = true;
}

/* Initialise with default values (int time = 2.4ms, gain = 1x) */
// Adafruit_TCS34725 tcs = Adafruit_TCS34725();
/* Initialise with specific int time and gain values */
//Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_60MS, TCS34725_GAIN_1X); // Take a 60ms measurement at 1x gain

void setColourRGB (int r, int g, int b, int delayTime=0){ // r,g,b values should be in range 0-255
  // This function causes the RGB LED to light up the specified colour.
  // Note: analogWrite() initiates a PWM signal to control the brightness of each colour, by modifying the average voltage. The pins used here must be PWM.
  if(_ENABLE_RGB_LED){ // Only run the following code if there exists an RGB LED
    analogWrite(_PIN_LED_RED, r*4);
    analogWrite(_PIN_LED_GREEN, g*4);
    analogWrite(_PIN_LED_BLUE, b*4);
  }
  delay(delayTime); // blocks for specified number of milliseconds
}

ColourEnum mapToColourFromRGBCReadings(uint16_t r, uint16_t g, uint16_t b, uint16_t c, uint16_t colourTemp, uint16_t lux){
  // This function maps the measured readings into 5 discrete colours. The categorization criteria were selected from test results.
  if (lux>50000){
    setColourRGB(255,0,0); // Red
    Serial.print("RED");
    return Red;
  } else if (colourTemp>8000){
    setColourRGB(0,0,255); // Blue
    Serial.print("BLUE");
    return Blue;
  } else if (g<200){
    setColourRGB(0,0,0); // Off
    Serial.print("BLACK");
    return Black;
  } else if (colourTemp>2900){
    setColourRGB(0,255,0); // Green
    Serial.print("GREEN");
    return Green;
  } else {
    setColourRGB(255,120,0); // Yellow
    Serial.print("YELLOW");
    return Yellow;
  }
}

/* tcs.getRawData() does a delay(Integration_Time) after the sensor readout.
  We don't need to wait for the next integration cycle because we receive an interrupt when the integration cycle is complete*/
void getRawData_noDelay(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
  *c = tcs.read16(TCS34725_CDATAL);
  *r = tcs.read16(TCS34725_RDATAL);
  *g = tcs.read16(TCS34725_GDATAL);
  *b = tcs.read16(TCS34725_BDATAL);
}

ColourEnum getColour(){
  // Returns the enum integer corresponding to the identified colour (from enum Colours defined at the top of this header file).
  // Also updates the RGB LED with the current measured colour
  if (_COLOUR_SENSOR_READY){
    uint16_t r, g, b, c, colourTemp, lux;
    
    // Should consider using this following snippet to have non-blocking code instead https://forum.arduino.cc/t/solved-reading-from-tcs34725-slows-down-subsequent-functions/603544/10
    getRawData_noDelay(&r, &g, &b, &c);
    //tcs.getRawData(&r, &g, &b, &c);
    // colourTemp = tcs.calculateColorTemperature(r, g, b);
    colourTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
    lux = tcs.calculateLux(r, g, b);
    _LAST_MEASURED_COLOUR = mapToColourFromRGBCReadings(r,g,b,c, colourTemp,lux);
    _COLOUR_SENSOR_READY = false;
    tcs.clearInterrupt();
  }
  return _LAST_MEASURED_COLOUR;
}

void colourSensorSetup(const int pinInterrupt, const int pinLedRed, const int pinLedGreen, const int pinLedBlue){
  _PIN_LED_RED = pinLedRed;
  _PIN_LED_GREEN = pinLedGreen;
  _PIN_LED_BLUE = pinLedBlue;
  pinMode(_PIN_LED_RED, OUTPUT);
  pinMode(_PIN_LED_GREEN, OUTPUT);
  pinMode(_PIN_LED_BLUE, OUTPUT);
  _ENABLE_RGB_LED = true; // Allow the function "setColourRGB" to work now.

  pinMode(pinInterrupt, INPUT_PULLUP); //TCS interrupt output is Active-LOW and Open-Drain
  attachInterrupt(digitalPinToInterrupt(pinInterrupt), isrColour, FALLING);

  if (tcs.begin()) {
    // Colour sensor found
    //Serial.println("Found colour sensor");
  } else {
    // No colour sensor found.
    //Serial.println("No TCS34725 found.");
    const int delayTimeMS = 500;
    while (1){ // Flash RGB LED red every 500ms to show there is an error.
      setColourRGB(255,0,0, delayTimeMS);
      setColourRGB(  0,0,0, delayTimeMS);
    }
  }
  tcs.write8(TCS34725_PERS, TCS34725_PERS_NONE); // Set persistence filter to generate an interrupt for every RGB Cycle, regardless of the integration limits
  tcs.setInterrupt(true);
}

#endif
