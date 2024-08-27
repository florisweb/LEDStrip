#include "FastLED.h"
#include "connectionManager.h";

const int signalPin = 5;
#define RGB_LED_NUM  300            // 10 LEDs [0...9]
#define BRIGHTNESS   255           // brightness range [0..255]
#define CHIP_SET     WS2812B       // types of RGB LEDs
#define COLOR_CODE   GRB           // sequence of colors in data stream

CRGB LEDs[RGB_LED_NUM];


const char* ssid = "";
const char* password = "";
const String deviceId = "LEDStrip";
const String deviceKey = "";

connectionManager ConnectionManager;

void onMessage(DynamicJsonDocument message) {
  String error = message["error"];
  int packetType = message["type"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  int r = 0;
  int g = 0;
  int b = 0;
  int inten = 0;


  switch (packetType) {
    case 1: Toggle_RED_LED();
      Serial.println("1.Toggle LED Complete");
      break;
    case 2: Scrolling_RED_LED();
      Serial.println("2.Scrolling RED LED Complete");
      break;
    case 3: O_W_G_scroll();
      Serial.println("3.Flag Show Complete");
      break;
    case 4:  Rotate_color();
      Serial.println("4.Rotate color Complete");
      break;
    case 5: r_g_b();
      Serial.println("5.R_G_B color Complete");
      break;
    case 6: random_color();
      Serial.println("6.Random color Show Complete");
      break;
    case 7: charge_bat();
      Serial.println("7.charge animation");
      break;
    case 8: charge_bat_test();
      Serial.println("8.charge test animation");
      break;

    case 9:
      r = message["data"][0];
      g = message["data"][1];
      b = message["data"][2];
      inten = message["data"][3];
      Serial.println( r);
      Serial.println( g);
      Serial.println( b);
      Serial.println( inten);

      FastLED.setBrightness( inten);
      for (int i = 0; i < RGB_LED_NUM; i++) {
        LEDs[i] = CRGB ( r, g, b);
      }
      FastLED.show();
      break;

    default:
      Serial.println("No animation found");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setting up LEDS...");

  FastLED.addLeds<CHIP_SET, signalPin, COLOR_CODE>(LEDs, RGB_LED_NUM);
  randomSeed(analogRead(0));
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();

  Serial.println("Setting up WiFi...");
  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);
}


void sendData() {
  String dataString = "{\"type\": \"sensorState\", \"data\": []}";
  ConnectionManager.send(dataString);
}

void loop() {
  ConnectionManager.loop();
}

















// RED LED TOGGLE
void Toggle_RED_LED(void) {
  // Red Green Blue
  for (int i = 0; i < RGB_LED_NUM; i++)
    LEDs[i] = CRGB(255, 0, 0 );
  FastLED.show();
  delay(1000);
  for (int i = 0; i < RGB_LED_NUM; i++)
    LEDs[i] = CRGB(0, 0, 0 );
  FastLED.show();
  delay(1000);
}

// Move the Red LED
void Scrolling_RED_LED(void)
{
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::Red;
    FastLED.show();
    delay(500);
    LEDs[i] = CRGB::Black;
    FastLED.show();
    delay(500);
  }
}

// Orange/White/Green color green
void O_W_G_scroll() {
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::Orange;
    delay(20);
    FastLED.show();
  }
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::Black;
    delay(20);
    FastLED.show();
  }
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::White;
    delay(20);
    FastLED.show();
  }
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::Black;
    delay(20);
    FastLED.show();
  }
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::Green;
    delay(20);
    FastLED.show();
  }
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB::Black;
    delay(20);
    FastLED.show();
  }
}

// Red/Green/Blue color Rotate
void Rotate_color(void) {
  for (int clr = 0; clr < RGB_LED_NUM; clr++) {
    LEDs[clr]     = CRGB::Red;
    LEDs[clr + 1] = CRGB::Green;
    LEDs[clr + 2] = CRGB::Blue;
    FastLED.show();
    delay(100);
    for (int clr = 0; clr < RGB_LED_NUM; clr++) {
      LEDs[clr] = CRGB::Black;
      delay(5);
    }
  }
}

// Blue, Green , Red
void r_g_b() {
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB ( 0, 0, 255);
    FastLED.show();
    delay(50);
  }
  for (int i = RGB_LED_NUM; i >= 0; i--) {
    LEDs[i] = CRGB ( 0, 255, 0);
    FastLED.show();
    delay(50);
  }
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB ( 255, 0, 0);
    FastLED.show();
    delay(50);
  }
  for (int i = RGB_LED_NUM; i >= 0; i--) {
    LEDs[i] = CRGB ( 0, 0, 0);
    FastLED.show();
    delay(50);
  }
}

byte  a, b, c;
// random color show
void random_color(void) {
  // loop over the NUM_LEDS
  for (int i = 0; i < RGB_LED_NUM; i++) {
    // choose random value for the r/g/b
    a = random(0, 255);
    b = random(0, 255);
    c = random(0, 255);
    // Set the value to the led
    LEDs[i] = CRGB (a, b, c);
    // set the colors set into the physical LED
    FastLED.show();

    // delay 50 millis
    FastLED.delay(200);
  }
}











void charge_bat_test(void) {
  FastLED.setBrightness(255);
  //
  //  for (int t = 0; t < 255; t++)
  //  {
  //    byte intensity = 255 - t;
  //    Serial.println(intensity);
  //    for (int i = 0; i < RGB_LED_NUM; i++) {
  //      LEDs[i] = CRGB (0, t, 0);
  //    }
  //    FastLED.show();
  //    FastLED.delay(100);
  //  }
  //  for (int t = 0; t < 255; t++)
  //  {
  //    byte intensity = t;
  //    Serial.println(intensity);
  //    for (int i = 0; i < RGB_LED_NUM; i++) {
  //      LEDs[i] = CRGB (0, t, 0);
  //    }
  //    FastLED.show();
  //    FastLED.delay(100);
  //  }

  FastLED.delay(2000);
  for (int t = 0; t < 255; t += 1)
  {
    float val = 255 - t;
    int base = 100;
    float powPart = (pow(base, val / 255) - 1);
    byte intensity = 255 / (1 - 1 / base) * 1/base * powPart;

    Serial.println(intensity);
    for (int i = 0; i < RGB_LED_NUM; i++) {
      LEDs[i] = CRGB (0, intensity, 0);
    }
    FastLED.show();
    FastLED.delay(20);
  }
  for (int t = 0; t < 255; t += 1)
  {
    float val =  t;
    int base = 100;
    float powPart = (pow(base, val / 255) - 1);
    byte intensity = 255 / (1 - 1 / base) * 1/base * powPart;
    Serial.println(intensity);
    for (int i = 0; i < RGB_LED_NUM; i++) {
      LEDs[i] = CRGB (0, intensity, 0);
    }
    FastLED.show();
    FastLED.delay(20);
  }
}




void charge_bat(void) {
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB (0, 0, 0);
  }
  FastLED.show();

  const int wingSize = 50;
  for (int i = wingSize; i < RGB_LED_NUM - wingSize; i++) {
    LEDs[i] = CRGB (0, 255, 0);
  }
  FastLED.show();




  int wingIndex = 0;
  for (int t = 0; t < 255; t += 5)
  {
    byte intensity = t;
    Serial.println(intensity);

    FastLED.setBrightness(t);

    if (t % 5 == 0)
    {
      wingIndex++;
      LEDs[wingSize - wingIndex] = CRGB (0, 255, 0);
      LEDs[RGB_LED_NUM - wingSize + wingIndex] = CRGB (0, 255, 0);
      FastLED.show();
    }
    FastLED.delay(1);
  }


  for (int t = 0; t < 100; t++)
  {
    byte intensity = 255 - t;
    Serial.println(intensity);
    FastLED.setBrightness(t);
    FastLED.delay(50);
  }
  for (int t = 0; t < 100; t++)
  {
    byte intensity = 155 + t;
    Serial.println(intensity);
    FastLED.setBrightness(t);
    FastLED.delay(50);
  }
}
