#include <FastLED.h>
//#include <Library.h>

#define NUM_LEDS 144
#define DATA_PIN  2
#define COLOR_ORDER GRB
#define CHIPSET WS2812B
#define BRIGHTNESS 60
#define VOLTS 5
#define MAX_AMPS 500

CRGB leds[NUM_LEDS];
int G = 0;
int R = 0;
int B = 0;
int redPin = 3;        
int redState = 0;
int orangePin = 4;       
int orangeState = 0;

void setup() {
  // put your setup code here, to run once:
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER> (leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_AMPS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  pinMode(redPin, INPUT_PULLUP);
  pinMode(orangePin, INPUT_PULLUP);
  //new part
  for(int i = 0; i < NUM_LEDS; i++){      //When starting up, LED strip will be blue
    leds[i] = CRGB(0, 0, 255);
    FastLED.show();
    delay(100);
    //leds[i] = CRGB(0, 0, 0);
  }
  //end of new part
  
}

void loop() {
  // put your main code here, to run repeatedly:
  redState = digitalRead(redPin);
  orangeState = digitalRead(orangePin);
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB(R, G, B);
    FastLED.show();
    delay(100);
    //leds[i] = CRGB(0, 0, 0);
    
    if(redState == HIGH){          //dispenser empty
      G = 0;
      R = 255;
      B = 0;                      //turn strip red
    }
    else if(orangeState == HIGH){  //dispener low
      R = 255;
      G = 50;                      //turn strip orange
      B = 0;
    }
    else{                         //otherwise stay green
      G = 255;
      R = 0;
      B = 0;
    }
    
  }

}
