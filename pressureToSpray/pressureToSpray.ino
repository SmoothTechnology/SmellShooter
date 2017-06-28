#include <Wire.h>
#include <Adafruit_MPL115A2.h>

Adafruit_MPL115A2 mpl115a2;

int solenoidPin = 8;
int ledPin = 13;

void setup(void) 
{
  pinMode(solenoidPin, OUTPUT);

  Serial.begin(9600);
  Serial.println("Hello!");
  
  Serial.println("Getting barometric pressure ...");
  mpl115a2.begin();

}

int delaySpeed = 0;
void loop(void) 
{
  float pressureKPA = 0;  

  pressureKPA = mpl115a2.getPressure(); 
  pressureKPA -= 100.0;
  pressureKPA *= 1000.0; 

   Serial.println(pressureKPA);


  if(pressureKPA > 2200){

    // Serial.print(pressureKPA);

    // delaySpeed = map(pressureKPA, 1800, 11000, 10, 30);

    // Serial.println(delaySpeed);

    // digitalWrite(solenoidPin, HIGH);
    // delay(delaySpeed);
    // digitalWrite(solenoidPin, LOW);
    // delay(100);


    triggerSolenoid(true);
    delay(20);
    triggerSolenoid(false);
    delay(125);

  }


  delay(20);
}


void triggerSolenoid(bool state){
    digitalWrite(solenoidPin, state);
    digitalWrite(ledPin, state);
}
