#include "FastLED.h"
#include "connectionManager.h";

const int IRSensorPin = 18;
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

byte curAlarmRed, curAlarmGreen, curAlarmBlue;
byte baseRed = 0;
byte baseGreen = 0;
byte baseBlue = 0;

connectionManager ConnectionManager;

void onMessage(DynamicJsonDocument message) {
  String error = message["error"];
  String packetType = message["type"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "playChargePhoneAnimation")
  {
    return chargePhoneAnimation(message["data"]);
  } else if (packetType == "startAlarmAnimation") {
    curAlarmRed   = message["data"][0];
    curAlarmGreen = message["data"][1];
    curAlarmBlue  = message["data"][2];
  } else if (packetType == "stopAlarm") {
    curAlarmRed = curAlarmGreen = curAlarmBlue = 0;
    setToBaseColor();
  } else if (packetType == "setColor") {
    baseRed   = message["data"][0];
    baseGreen = message["data"][1];
    baseBlue  = message["data"][2];
    setToBaseColor();
  } else if (packetType == "setLEDs") {
    FastLED.setBrightness(255);
    JsonArray LEDDataJSON = message["data"];

    int curLEDData[4];
    int curIndex = 0;
    for (JsonVariant v : LEDDataJSON) {
      curLEDData[curIndex % 4] = v.as<int>();
      curIndex++;
      if (curIndex % 4 == 0)
      {
        LEDs[curLEDData[0]] = CRGB (curLEDData[1], curLEDData[2], curLEDData[3]);
      }
    }

    FastLED.show();
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

  pinMode(IRSensorPin, INPUT);

  Serial.println("Setting up WiFi...");

  ConnectionManager.defineEventDocs("["
                                    "{"
                                    "\"type\": \"IRSensorEvent\","
                                    "\"data\": \"bool\","
                                    "\"description\": \"True on person detect, false on no-person detect\""
                                    "}"
                                    "]");
  ConnectionManager.defineAccessPointDocs("["
                                          "{"
                                          "\"type\": \"playChargePhoneAnimation\","
                                          "\"description\": \"Shows the charge-animation.\""
                                          "},"
                                          "{"
                                          "\"type\": \"startAlarmAnimation\","
                                          "\"data\": \"[r, g, b]\","
                                          "\"description\": \"Shows a pulsating animation with the color as specified in data.\""
                                          "},"
                                          "{"
                                          "\"type\": \"stopAlarm\","
                                          "\"description\": \"Stops the animation from startAlarmAnimation.\""
                                          "},"
                                          "{"
                                          "\"type\": \"setColor\","
                                          "\"data\": \"[r, g, b]\","
                                          "\"description\": \"Sets the homogenious color of the LEDS.\""
                                          "},"
                                          "{"
                                          "\"type\": \"setLEDs\","
                                          "\"data\": \"[led-index 1, r1, g1, b1...]\","
                                          "\"description\": \"Allows for per LED color control.\""
                                          "}"
                                          "]");


  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);
}



unsigned int prevMillis = 0;
bool IRSensorState = false;
bool curIRSensorState = false;
void loop() {
  ConnectionManager.loop();

  if (millis() - prevMillis > 60000 / 30 && curAlarmRed + curAlarmGreen + curAlarmBlue > 0)
  {
    prevMillis = millis();
    heartBeat(curAlarmRed, curAlarmGreen, curAlarmBlue);
  }

  // IR-Sensor
  IRSensorState = digitalRead(IRSensorPin);
  if (IRSensorState != curIRSensorState)
  {
    if (IRSensorState) { // On-enter
      String dataString = "{\"type\": \"IRSensorEvent\", \"data\": true}";
      ConnectionManager.send(dataString);
      Serial.println("Enter");
    } else { // On-leave
      String dataString = "{\"type\": \"IRSensorEvent\", \"data\": false}";
      ConnectionManager.send(dataString);
      Serial.println("Leave");
    }
  }
  curIRSensorState = IRSensorState;
}















void heartBeat(byte r, byte g, byte b) {
  //  int batchSize = 15;
  //  float delaySize = _duration * batchSize / RGB_LED_NUM;
  //  for (int i = 0; i < RGB_LED_NUM; i++) {
  //    LEDs[i] = CRGB (0, 0, 0);
  //  }
  int batchSize = 5;
  float delaySize = 3;
  FastLED.show();

  for (int t = 1; t < RGB_LED_NUM / 2; t += batchSize)
  {
    float val = t * 2 * 255 / RGB_LED_NUM;
    byte intensity = LLV(val);
    FastLED.setBrightness(intensity);
    for (int i = 0; i < t; i++)
    {
      LEDs[RGB_LED_NUM / 2 - i] = CRGB (r, g, b);
      LEDs[RGB_LED_NUM / 2 + i] = CRGB (r, g, b);
    }
    FastLED.show();
    FastLED.delay(delaySize);
  }

  for (int t = 1; t < RGB_LED_NUM / 2; t += batchSize)
  {
    float val = 255 - t * 2 * 255 / RGB_LED_NUM;
    byte intensity = LLV(val);
    FastLED.setBrightness(intensity);
    for (int i = 0; i < t; i++)
    {
      LEDs[RGB_LED_NUM / 2 - i] = CRGB (0, 0, 0);
      LEDs[RGB_LED_NUM / 2 + i] = CRGB (0, 0, 0);
    }
    FastLED.show();
    FastLED.delay(delaySize);
  }
}










void chargePhoneAnimation(int batPercentage) {
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB (0, 0, 0);
  }
  FastLED.show();

  byte gVal = 2.55 * batPercentage;
  byte rVal = 255 - 2.55 * batPercentage;
  Serial.print("Green");
  Serial.println(gVal);



  for (int t = 1; t < RGB_LED_NUM / 2; t += 5)
  {
    float val = t * 2 * 255 / RGB_LED_NUM;
    byte intensity = LLV(val);
    FastLED.setBrightness(intensity);
    for (int i = 0; i < t; i++)
    {
      LEDs[RGB_LED_NUM / 2 - i] = CRGB (rVal, gVal, 0);
      LEDs[RGB_LED_NUM / 2 + i] = CRGB (rVal, gVal, 0);
    }
    FastLED.show();
    FastLED.delay(15);
  }

  for (int t = 1; t < RGB_LED_NUM / 2; t += 5)
  {
    float val = 255 - t * 2 * 255 / RGB_LED_NUM;
    byte intensity = LLV(val);
    FastLED.setBrightness(intensity);
    for (int i = 0; i < t; i++)
    {
      if (i > t - 25 || i < t - 35)
      {
        LEDs[RGB_LED_NUM / 2 - i] = CRGB (0, 0, 0);
        LEDs[RGB_LED_NUM / 2 + i] = CRGB (0, 0, 0);
      } else {
        LEDs[RGB_LED_NUM / 2 - i] = CRGB (255, 150, 0);
        LEDs[RGB_LED_NUM / 2 + i] = CRGB (255, 150, 0);
      }
    }
    FastLED.show();
    FastLED.delay(15);
  }

  setToBaseColor();
}


void setToBaseColor() {
  FastLED.setBrightness(255);
  for (int i = 0; i < RGB_LED_NUM; i++) {
    LEDs[i] = CRGB ( baseRed, baseGreen, baseBlue);
  }
  FastLED.show();
}




float LLV(float val) { // Linearize Light Value
  int base = 100;
  float powPart = (pow(base, val / 255) - 1);
  return 255 / (1 - 1 / base) * 1 / base * powPart;
}
