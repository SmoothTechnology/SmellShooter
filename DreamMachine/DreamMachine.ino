#include <Wire.h>
#include "SparkFunMPL3115A2.h"
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

///////////////////////////////////////////////////
//////////////////// BEGIN OPTIONS ///////////////
///////////////////////////////////////////////////
#define LEDPIN 17

#define SPARKLE_COLOR strip.Color(100,100,100)
// #define SPARKLE_COLOR strip.Color(255,123,0)
#define MAX_PRESSURE_COLOR strip.Color(0,random(255),random(255))
// #define MAX_INFLATE_COLOR 20
// #define MIN_INFLATE_COLOR 0

// #define TEST_LEDS

// SET LED HEIGHT, COLOR, AND CHANGE USB NAME
#define ID_5

#ifdef ID_1 //GOOD
    //1
    #define MAX_INFLATE_COLOR myColor(50,100,100)
    #define MIN_INFLATE_COLOR myColor(20,50,25)
    int led_height = 54;
    #define MIDI_CHANNEL 1
#endif    
#ifdef ID_2 //GOOD, update led length in doc to 61
    //2
    #define MAX_INFLATE_COLOR myColor(70,120,20)
    #define MIN_INFLATE_COLOR myColor(30,50,20)
    int led_height = 61;
    #define MIDI_CHANNEL 2
#endif    
// #ifdef ID_3 // update led_length in doc to 60
//     //3
//     #define MAX_INFLATE_COLOR myColor(0,100,0)
//     #define MIN_INFLATE_COLOR myColor(0,50,0)
//     int led_height = 60;
//     #define MIDI_CHANNEL 3
// #endif    
#ifdef ID_4 
    //4
    #define MAX_INFLATE_COLOR myColor(150,0,0)
    #define MIN_INFLATE_COLOR myColor(50,0,0)
    int led_height = 65;
    #define MIDI_CHANNEL 4
#endif    
#ifdef ID_5
    //5
    #define MAX_INFLATE_COLOR myColor(120,30,0)
    #define MIN_INFLATE_COLOR myColor(30,10,0)
    int led_height = 70;
    #define MIDI_CHANNEL 5
#endif    
#ifdef ID_6 //GOOD
    //6
    #define MAX_INFLATE_COLOR myColor(40,15,0)
    #define MIN_INFLATE_COLOR myColor(40,15,0)
    int led_height = 70;
    #define MIDI_CHANNEL 6
#endif     
#ifdef ID_7 //GOOD
    // 7 - B HOPE
    #define MAX_INFLATE_COLOR myColor(125,65,0)
    #define MIN_INFLATE_COLOR myColor(80,40,0)
    int led_height = 75;
    #define MIDI_CHANNEL 7
#endif    
#ifdef ID_8
    //8
    #define MAX_INFLATE_COLOR myColor(155,0,85)
    #define MIN_INFLATE_COLOR myColor(80,0,60)
    int led_height = 76;
    #define MIDI_CHANNEL 8
#endif    
#ifdef ID_9 //GOOD
    //9
    #define MAX_INFLATE_COLOR myColor(125,60,85)
    #define MIN_INFLATE_COLOR myColor(80,40,60)
    int led_height = 93;
    #define MIDI_CHANNEL 9
#endif    
#ifdef ID_10
    //10
    #define MAX_INFLATE_COLOR myColor(0,0,150)
    #define MIN_INFLATE_COLOR myColor(30,30,70)
    int led_height = 94;
    #define MIDI_CHANNEL 10
#endif

// Control Pressure Inflate and Deflate
int threshold = 5000; // Pressure must exceed this value to inflate
int decrementVal = 2500; // Speed of pressure deflate
///////////////////////////////////////////////////
//////////////////// END OPTIONS //////////////////
///////////////////////////////////////////////////

Adafruit_NeoPixel strip = Adafruit_NeoPixel(led_height * 4, LEDPIN, NEO_GRB + NEO_KHZ800);

MPL3115A2 myPressure;

#define POTPIN A0
#define CC_PRESSURE 1
#define CC_INTEGRAL 2

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
  uint32_t myColor = 0;

  if(velocity == 127)
  {
    myColor = strip.Color(100, 100, 100);
  }
  else
  {
    velocity = map(velocity, 0, 126, 0, 255);
    myColor = Wheel(velocity);
  }

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

void setup() {

  usbMIDI.setHandleNoteOn(OnNoteOn);

  Serial.begin(9600);
  Serial.println("Dream Machine!");
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.println(led_height);

  #ifdef TEST_LEDS

  while(true){
    for(int i = 0; i < led_height; i++){
      setLEDColor(i, strip.Color(25,0,0));
      delay(50);
      strip.show();
    }
    for(int i = 0; i < led_height; i++){
      setLEDColor(i, strip.Color(25,25,25));
      delay(50);
      strip.show();
    }
  }

  #endif

  myPressure.begin(); // Get sensor online

  // Configure the sensor
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(4); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags 
  
}

// State Variables
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

   //fill to scaled height
   float percentFilled = float( map(setIntegral, 0, 127, 0, 100) ) / 100.0;
   Serial.println(percentFilled);
   colorFill(lerpColor(MIN_INFLATE_COLOR,MAX_INFLATE_COLOR , percentFilled), setIntegral);

   if(setIntegral == 127)
   {
    sparkleSpecial();
   }

  if (midiPressure != prevPressure) {
    curTime = millis();
    usbMIDI.sendControlChange(CC_PRESSURE, midiPressure, MIDI_CHANNEL);

    float pressureVelocity = abs( float( midiPressure - prevPressure ) / ( 52.0 ) );
    prevTime = curTime;
    prevPressure = midiPressure;
  }

  if(setIntegral != prevIntegral)
  {
    usbMIDI.sendControlChange(CC_INTEGRAL, setIntegral, MIDI_CHANNEL);
    prevIntegral = setIntegral;
  }


  while (usbMIDI.read()) {
  }

}

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
      if(!random(20)) setLEDColor(i,SPARKLE_COLOR);
        else setLEDColor(i,0);
    }
  }
    strip.show();
}

void sparkleSpecial() {
  for(uint16_t i=0; i<led_height; i++) {
      if(!random(6)) setLEDColor(i,MAX_PRESSURE_COLOR);
  }
    strip.show();
}


uint8_t red(uint32_t c) {
  return ((c & 0xFF0000) >> 16);
}
uint8_t green(uint32_t c) {
  return ((c & 0x00FF00) >> 8);
}
uint8_t blue(uint32_t c) {
  return (c & 0x0000FF);
}

byte lerp(byte a, byte b, float t) {
  return a + (b - a)*t; 
}

uint32_t myColor(uint8_t r , uint8_t g , uint8_t b){
  return ((uint32_t)(r) << 16) | ((uint32_t)(g ) <<  8) | (b );
}

uint32_t lerpColor(uint32_t a, uint32_t b, float t) {
  return myColor(lerp(red(a), red(b), t), 
  lerp(green(a), green(b), t), 
  lerp(blue(a), blue(b), t));
}

