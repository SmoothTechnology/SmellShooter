#include <Wire.h>
#include "SparkFunMPL3115A2.h"
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif


#define LEDPIN 17
int led_height = 75;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(led_height * 4, LEDPIN, NEO_GRB + NEO_KHZ800);

// Power by connecting Vin to 3-5V, GND to GND
// Uses I2C - connect SCL to the SCL pin, SDA to SDA pin

//Create an instance of the object
MPL3115A2 myPressure;

#define POTPIN A0
#define MIDI_CHANNEL 1
#define CC_PRESSURE 1
#define CC_TEMPERATURE 2
#define CC_INTEGRAL 3

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void OnNoteOn(byte channel, byte note, byte velocity)
{
  colorFill(strip.Color(0,0,0), 127);

  if(velocity == 127)
  {
    for(int i = 0; i < 100; i+= 5)
    {
        colorFill(strip.Color(i,i,i), 127);
    }

    for(int i = 100; i > 0; i-= 5)
    {
      colorFill(strip.Color(i,i,i), 127);
    }
  }
  else
  {
    velocity = map(velocity, 0, 126, 0, 255);
    uint32_t myColor = Wheel(velocity);
    strip.setBrightness(0);
    colorFill( myColor, 127);

    for(int i = 0; i < 100; i+= 5)
    {
      int curBrightness = map(i, 0, 100, 0, 255);
      colorFill( myColor, 127);
      strip.setBrightness(curBrightness);
    }

    for(int i = 100; i > 0; i-= 5)
    {
      int curBrightness = map(i, 0, 100, 0, 255);
      colorFill( myColor, 127);
      strip.setBrightness(curBrightness);
    }

    strip.setBrightness(255);
  }
  
}

void setup() {

  usbMIDI.setHandleNoteOn(OnNoteOn);

  Serial.begin(9600);
  Serial.println("Dream Machine!");
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  //TODO, have it reconnect on loop if failure
  myPressure.begin(); // Get sensor online

  // Configure the sensor
  //myPressure.setModeAltimeter(); // Measure altitude above sea level in meters
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  
  myPressure.setOversampleRate(4); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags 
  
}


int prevPressure = -1;
int prevTemp = -1;
int prevIntegral = 0;

int midiPressure = 0;
int setIntegral = 0;
int midiPressureVelocity = 0;
int midiTemperature = 0;


unsigned long curTime = 0;
unsigned long prevTime = 0;

void loop() {

  midiPressure = getScaledPressure(setIntegral);
  // Serial.print("\t");
  midiTemperature = getScaledTemperature();
  // Serial.println();
//  Serial.println(midiTemperature);

   //fill to scaled height
   //colorFill(strip.Color(255,0,0), midiPressure);

   colorFill(Wheel(map(setIntegral, 0, 127, 20, 0)), setIntegral);

   if(setIntegral == 127)
   {
    sparkleSpecial();
   }

  //Only send on value change

  // CAUSES ISSUES
  if (midiPressure != prevPressure) {
    curTime = millis();
    usbMIDI.sendControlChange(CC_PRESSURE, midiPressure, MIDI_CHANNEL);

    float pressureVelocity = abs( float( midiPressure - prevPressure ) / ( 52.0 ) );
    //Serial.println(pressureVelocity*100);
    prevTime = curTime;
    prevPressure = midiPressure;
  }
  // CAUSES ISSUES

  if(setIntegral != prevIntegral)
  {
    usbMIDI.sendControlChange(CC_INTEGRAL, setIntegral, MIDI_CHANNEL);
    prevIntegral = setIntegral;
  }

  if (midiTemperature != prevTemp) {
      usbMIDI.sendControlChange(CC_TEMPERATURE, midiTemperature, MIDI_CHANNEL);
      prevTemp = midiTemperature;
    }

  
  // MIDI Controllers should discard incoming MIDI messages.
  // http://forum.pjrc.com/threads/24179-Teensy-3-Ableton-Analog-CC-causes-midi-crash
  while (usbMIDI.read()) {
    // ignore incoming messages
  }

}

int threshold = 2000;
int decrementVal = 3000;
float sumVal = 0;
unsigned long samplePeriod = 20;
unsigned long lastTime = 0;

int getScaledPressure(int &integral){
  
  
  float pascals = myPressure.readPressure();
  int pressure = pascals - 100000;

  if( (millis() - lastTime > samplePeriod)  )
  {
    lastTime = millis();
    if((pressure > threshold))
      sumVal += (pressure - threshold);

    sumVal = sumVal - decrementVal;

    if(sumVal < 0) 
      sumVal = 0;
    else if(sumVal > 60000)
      sumVal = 60000;
  }

  integral = constrain(sumVal, 0, 50000);
  integral = map(integral, 0, 50000, 0, 127);

  Serial.print("SumVal: ");
  Serial.print(integral);
  Serial.print("   Pascals: ");
  Serial.println(pascals);

  int minPressure = map( analogRead(POTPIN), 0, 1023, 1500, 4000 );
  int maxPressure = minPressure + 6000;

  if(pressure < minPressure)  pressure = minPressure;
  if(pressure > maxPressure)  pressure = maxPressure;

  return map(pressure, minPressure, maxPressure, 0, 127);

}

int getScaledTemperature(){

  float tempF = myPressure.readTempF();
  int minTemp = 65;
  int maxTemp = 95;

//  Serial.print(tempC);

  if(tempF < minTemp)  tempF = minTemp;
  if(tempF > maxTemp)  tempF = maxTemp;

  return map(tempF, minTemp, maxTemp, 0, 127);
}

void setLEDColor(int i, uint32_t c) {
    strip.setPixelColor(i, c);    //turn every third pixel on
    strip.setPixelColor(led_height * 2 - i - 1, c);    //turn every third pixel on
    strip.setPixelColor(led_height * 2 + i, c);    //turn every third pixel on
    strip.setPixelColor(led_height * 4 - i - 1, c);    //turn every third pixel on
}

// Fill the dots one after the other with a color
void colorFill(uint32_t c, uint8_t val) {
  int poleScale = map(val, 0, 127, 0, led_height);
  for(uint16_t i=0; i<led_height; i++) {
    if(i < poleScale) setLEDColor(i, c);
    //Sparkle that biatch
    else 
    {
      if(!random(20)) setLEDColor(i,strip.Color(255,123,0));
        else setLEDColor(i,0);
    }
  }
    strip.show();
}

void sparkleSpecial() {
  for(uint16_t i=0; i<led_height; i++) {
      if(!random(6)) setLEDColor(i,strip.Color(0,random(255),random(255)));
  }
    strip.show();
}


