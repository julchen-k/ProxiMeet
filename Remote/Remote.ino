//Buzzer
#define buzzer 13

//LED Stripe
#include <FastLED.h>
#define LED_PIN 14
#define COLOR_ORDER GRB
#define LED_TYPE    WS2811
#define NUM_LEDS    8
#define BRIGHTNESS          30
#define FRAMES_PER_SECOND  120

CRGB leds[NUM_LEDS];

//Firebase Database
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#define FIREBASE_HOST "masters-thesis-c1928-default-rtdb.europe-west1.firebasedatabase.app"  
#define FIREBASE_AUTH "NpcrETS4dXwcNqjBiKsdxiZeG76EL78u5rrYDmyl"  
//#define WIFI_SSID "AlexNet (2)"
//#define WIFI_PASSWORD "qqqqqqqq"
#define WIFI_SSID "Julias iPhone"
#define WIFI_PASSWORD "magi1212"

//CAP1188
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_CAP1188.h>
#define CAP1188_RESET  9
#define CAP1188_CS  10
#define CAP1188_MOSI  11
#define CAP1188_MISO  12
#define CAP1188_CLK  13

Adafruit_CAP1188 cap = Adafruit_CAP1188();

bool currentTouch = false; //pushed to Firebase, indicating whether the person is touching right NOW
bool sentTouch = false; //pushed to Firebase, indicating whether the person sent a touch
bool user2_sentTouch = false; //pulling from Firebase, indicating whether the other user sent a touch
bool user2_currentTouch = false; //pulling from Firebase, indicating whether the other user is touching right NOW

unsigned long previousMillis = 0;
unsigned long sharedTouchMillis = 0;
const long interval = 1000;

int brightness = 0; 
int fadeAmount = 5;

bool timerSet = false;


void setup() {
  Serial.begin(115200);

  //LED SETUP
  delay(3000); // 3 second delay for recovery
  FastLED.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  //Firebase Database Setup
  //Wifi Connection
  //ESP.eraseConfig(); //try this out to fix restarting bug, and remember to put on the debug stuff -> didnt work
  //ESP.wdtDisable(); //try out disabling watchdog to fix the restarting bug -> didnt work
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) 
  {
    for(int i=0; i<=NUM_LEDS; i++)
    {
    leds[i] = CRGB::White; 
    FastLED.show();
    delay(200);
    FastLED.clear();
    FastLED.show();
    }
    //Serial.print(".");
    //delay(200);

  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  //Firebase Connection
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //setting default starting state of touches
  Firebase.setBool("user1_currentTouch", false); 
  Firebase.setBool("user1_sentTouch", false);

  //CAP1188 Setup
    Serial.println("CAP1188 test!");

  // Initialize the sensor, if using i2c you can pass in the i2c address
  // if (!cap.begin(0x28)) {
  if (!cap.begin()) {
    Serial.println("CAP1188 not found");
    while (1);
  }
  Serial.println("CAP1188 found!");

  //Buzzer Setup
  pinMode(buzzer, OUTPUT);


  
//fill all LEDs when ready to start
    for (int i=0; i<=NUM_LEDS; i++)
    {
      leds[i] = CRGB::White;  
    }
    FastLED.show();
  
  Serial.println("Ready To Start");

}

void loop() {
  uint8_t touched = cap.touched(); //checks if this device is touched
  user2_sentTouch = Firebase.getBool("user2_sentTouch"); //checks if the other device was touched
  user2_currentTouch = Firebase.getBool("user2_currentTouch"); //checks if the other device is currently touched


  //No touch detected on this device 
  if (touched == 0) 
  {
    noCurrentTouch();
  }


  //When touch is detected on this device
  //for (uint8_t i=0; i<8; i++) 
  //{
    if (touched) // & (1 << i)) 
    {
      //Serial.print("C"); Serial.print(i+1); Serial.print("\t");
      currentTouch = true;
      sentTouch = true;
      Firebase.setBool ("user1_currentTouch",true);
      Firebase.setBool ("user1_sentTouch",true);
      Serial.println("user 1 touching");
      //delay(10);
       
    }
    
    //only this user has sent a touch
    if (sentTouch == true & user2_sentTouch == false)
    {
      blueLightState();
    }
    

    //both users have sent a touch
    if (sentTouch == true & user2_sentTouch == true)
    {
      yellowLightState();
      previousMillis = millis();

      if(timerSet == false)
      {
        sharedTouchMillis = millis(); //get the current time in milliseconds
        timerSet = true;
        Serial.println("Setting the timer for shared touch.");
      }
      Serial.println(sharedTouchMillis);
      Serial.println(previousMillis);
      if(previousMillis - sharedTouchMillis >= 30000) //if there is no mutual touch within 30 seconds, the device goes back to base state
      {
        timerSet = false;
        sharedTouchMillis = 0;
        baseState();
      }
    }


    if(currentTouch & user2_currentTouch)
    {
      mutualTouch();
      touched = cap.touched(); //updates if this device is touched
      //delay(10);
    }


    if(!currentTouch || !user2_currentTouch)
    {
      digitalWrite(buzzer, LOW); // Set buzzer off if one user puts their hand away
      //delay(10);
    }

  //}

//the following part is my try for a breathing animation. Didn't work this way (yet)
                    /*unsigned long currentMillis = millis(); //get the current time in milliseconds
                    if(currentMillis - previousMillis >= interval) //make the lights pulsate if the interval has passed
                    {
                      if(brightness)
                      {
                        FastLED.setBrightness(5);
                        FastLED.show();
                        brightness = false;
                        delay(10);
                      }
                      else
                      {
                        FastLED.setBrightness(30);
                        FastLED.show();
                        brightness = true;
                        delay(10);      
                      }
                      
                    }*/


  if(Firebase.failed())
    { 
      Serial.println("Firebase log sending failed");
      Serial.println(Firebase.error());
      return;
    }
  
  yield();
  delay(1000);
}

void mutualTouch()
{
	Serial.println("Mutual touch started");
  orangeLightState();
	digitalWrite(buzzer, HIGH); // Set buzzer on
  yield();
	delay(5000); //giving the vibration feedback for 5 seconds before returning to base state
	baseState();
}

void noCurrentTouch()
{
	currentTouch = false;
	Firebase.setBool ("user1_currentTouch",false);
	//yield();
  //delay(10);

	if(user2_sentTouch == true & !sentTouch)
	{
		blueLightState();
    //yield();
    //delay(10);
	}
}

  void blueLightState()
  {
    for (int i=0; i<=NUM_LEDS; i++)
    {
      leds[i] = CRGB::Blue;  
    }
    FastLED.show();
    //yield();
    //delay(10);
  }

  void yellowLightState()
  {
    for (int i=0; i<=NUM_LEDS; i++)
    {
      leds[i] = CRGB::Yellow;  
    }
    FastLED.show();
    //yield();
    //delay(10);
  }

 void orangeLightState()
  {
    for (int i=0; i<=NUM_LEDS; i++)
    {
      leds[i] = CRGB::Orange;  
    }
    FastLED.show();
    //yield();
    //delay(10);
  }

  void baseState()
  {
    //wdt_reset();

    for (int i=0; i<=NUM_LEDS; i++)
    {
      leds[i] = CRGB::White;  
    }
    FastLED.show();

    sentTouch = false;
    Firebase.setBool("user1_sentTouch", false);
    Firebase.setBool("user2_sentTouch", false);

    currentTouch = false; //do we also need to adjust current touch here? I suppose not since they are reset as soon as it is not touched
    Firebase.setBool("user1_currentTouch", false); //but actually it didn't so here we are
    Firebase.setBool("user2_currentTouch", false);

    digitalWrite(buzzer, LOW); // Set buzzer off
    Serial.println("Mutual Touch over. Back to Base State.");
    
    //yield();
    delay(8000);
  
  }



