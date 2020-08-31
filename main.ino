#include <AccelStepper.h> 
#include <FastLED.h> 
#include <RTCZero.h>
#include <WiFiNINA.h> 

#define DATA_PIN 12
#define   LED_EN 11
#define  DIR_PIN 3
#define  STP_PIN 4
#define  SLP_PIN 5
#define HALL_PIN 13 
#define NUM_LEDS 144

CRGB leds[NUM_LEDS];
RTCZero rtc;  
AccelStepper stepper = AccelStepper(1, STP_PIN, DIR_PIN);

int steps_motor_rev = 400 * 8;                    //400 steps At 1/8 microstepping
int steps_clock_rev = steps_motor_rev * 3.5;      //Gear reduction is 3.5:1
int  max_speed      = steps_clock_rev / 20;       //20 seconds to revolve around the clock at max speed
int  acceleration   = max_speed / 2;              //2 seconds to accelerate to max speed
int  current_LED    = NUM_LEDS  / 2;
int  prev_LED       = 0; 
int  CST            = -5;
int  status         = WL_IDLE_STATUS;
int wifi_check      = 0;
long epoch_steps    = 0;
long epoch_LED      = 0;
bool isHome         = false;
char ssid[]         =       
char pass[]         =  


void setup() { 
  //Delay for restart recovery
  delay(3000);   

  //Initialize LED strip and set solid white
  pinMode(LED_EN, OUTPUT);
  digitalWrite(LED_EN, HIGH);  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(96);
  solidLED(CRGB::White);

  //Connect to the Wifi network or throw an error
  while (WiFi.status() != WL_CONNECTED && wifi_check < 5) {
    status = WiFi.begin(ssid, pass);
    wifi_check = wifi_check + 1; 
    delay(2000);
  }
  while(wifi_check == 5){
    solidLED(CRGB::Blue);
    delay(500);
    solidLED(CRGB::White);
    delay(500);
  }

  //Pin initialization 
  pinMode(STP_PIN, OUTPUT); 
  pinMode(DIR_PIN, OUTPUT);
  pinMode(SLP_PIN,  OUTPUT);
  pinMode(HALL_PIN, INPUT); 
  digitalWrite(SLP_PIN, LOW);

  //Stepper motor initialization
  stepper.setMaxSpeed(max_speed);
  stepper.setAcceleration(acceleration);    

  //Real time clock initialization
  rtc.begin();
  rtc.setEpoch(WiFi.getTime() + CST*60*60);
  delay(500);

  //Home the ring gear or throw an error
  isHome = homeRing();
  while(!isHome){
    solidLED(CRGB::Red);
    delay(500);
    solidLED(CRGB::White);
    delay(500);
  }
}
 
//Main loop:
// 11,200 steps for full clock revolution
// 43,200 secs for full clock revolution 
// 144 LEDs for a full clock revolution
float ratio_steps = (11200.0/43200.0);
float ratio_LED   = (144.0/3600.0);
void loop() {
  epoch_steps = (rtc.getHours()%12)*3600 + rtc.getMinutes()*60 + rtc.getSeconds();
  epoch_LED   =  rtc.getMinutes()*60 + rtc.getSeconds();
  
  //Stepper loop
  stepper.moveTo((int)(ratio_steps*epoch_steps));
  if(stepper.distanceToGo() > 233)                  //Update the stepper motor every 15 minutes 
  {                     
    digitalWrite(SLP_PIN, HIGH);
    delay(150);
    stepper.runToPosition();
    digitalWrite(SLP_PIN, LOW); 
  }
  
  //LED loop 
  current_LED = ((int)(ratio_LED*epoch_LED) + NUM_LEDS/2)%NUM_LEDS;
  if(prev_LED != current_LED)
  {
    prev_LED = current_LED;
    for(int x = 0; x < NUM_LEDS; x++){leds[x] = CRGB::White;}
    if(current_LED >= NUM_LEDS/2)                                           //if the current LED is in the first 30 minutes of the clock
    {
      for(int x = current_LED; x > NUM_LEDS/2; x--){leds[x] = CRGB::Green;} //then fill the strip up to that LED
    }
    if(current_LED < NUM_LEDS/2)                                            //if the current LED is in the last 30 minutes of the clock
    {
      for(int x = current_LED; x > 0; x--){leds[x] = CRGB::Green;}          //then fill back to the 30 minutes mark
      for(int x = NUM_LEDS-1; x > NUM_LEDS/2; x--){leds[x] = CRGB::Green;}  //and fill forward from the 0 mark
    }
    delay(150);
    FastLED.show();
  }
}

bool homeRing() {
 
  //Rotate until HALL_PIN triggered or one revolution
  solidLED(CRGB::Yellow);
  digitalWrite(SLP_PIN, HIGH);
  stepper.moveTo(steps_clock_rev);
  while(digitalRead(HALL_PIN) == 1 and stepper.currentPosition() < steps_clock_rev)
    stepper.run();  
  stepper.stop();
  solidLED(CRGB::Green);
  
  //Checks for fail state and then sets the new zero
  if(stepper.currentPosition() >= steps_clock_rev){
    digitalWrite(SLP_PIN, LOW);
    return false;
  }
  else
    stepper.setCurrentPosition(5140);
    stepper.moveTo(ratio_steps*((rtc.getHours()%12)*3600 + rtc.getMinutes()*60 + rtc.getSeconds()));
    stepper.runToPosition();
    digitalWrite(SLP_PIN, LOW);
    return true;
}

void solidLED(CRGB HTMLColorCode){
  for(int x = 0; x < NUM_LEDS; x++){
      leds[x] = HTMLColorCode;
  }
  delay(150);
  FastLED.show();
}
  
