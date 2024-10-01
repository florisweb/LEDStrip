#include "FastLED.h"
#include "connectionManager.h"

const int IRSensorPin = 18;
const int signalPin = 5;

#define RGB_LED_NUM  300
#define PIANO_LED_START_INDEX  222 // The index at which the strip is behind the piano
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

enum Animations { NONE, RAIN };
Animations curAnimation = NONE;

bool pianoConnected = false;




connectionManager ConnectionManager;

void onMessage(DynamicJsonDocument message) {
  String packetType = message["type"];
  String error = message["error"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "setLEDs") {
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
  } else if (packetType == "playChargePhoneAnimation")
  {
    return chargePhoneAnimation(message["data"]);
  } else if (packetType == "startAlarmAnimation") {
    curAlarmRed   = message["data"][0];
    curAlarmGreen = message["data"][1];
    curAlarmBlue  = message["data"][2];
  } else if (packetType == "stopAlarm") {
    curAlarmRed = curAlarmGreen = curAlarmBlue = 0;
    setToBaseColor();
  } else if (packetType == "setBaseColor") {
    setBaseColor(message["data"][0], message["data"][1], message["data"][2]);
  } else if (packetType == "animateBaseColor") {
    animateBaseColor(message["data"][0], message["data"][1], message["data"][2], message["data"][3]);
  } else if (packetType == "setPianoOnlineState") {
    pianoConnected = message["data"];
    if (message["data"]) {
      heartBeat(0, 255, 30, PIANO_LED_START_INDEX, RGB_LED_NUM, 1);
    } else {
      reverseHeartBeat(255, 0, 30, PIANO_LED_START_INDEX, RGB_LED_NUM, 1);
    }
    setToBaseColor();
  } else if (packetType == "curState") {
    setBaseColor(message["data"]["baseColor"][0], message["data"]["baseColor"][1], message["data"]["baseColor"][2]);
  } else if (packetType == "playAnimation") {
    if (curAnimation != NONE && static_cast<Animations>(message["data"]) == NONE) setToBaseColor();
    curAnimation = static_cast<Animations>(message["data"]);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setting up LEDS...");

  FastLED.addLeds<CHIP_SET, signalPin, COLOR_CODE>(LEDs, RGB_LED_NUM);
  randomSeed(analogRead(0));
  FastLED.setBrightness(255);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 9000);
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
                                          "\"type\": \"setPianoOnlineState\","
                                          "\"description\": \"Sets the piano state and shows the corresponding animation.\""
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
                                          "\"type\": \"playAnimation\","
                                          "\"data\": \"ENum int index of animation to play\","
                                          "\"description\": \"Plays a hardcoded animation constantly.\""
                                          "},"
                                          "{"
                                          "\"type\": \"setBaseColor\","
                                          "\"data\": \"[r, g, b]\","
                                          "\"description\": \"Sets the homogenious color of the LEDS.\""
                                          "},"
                                          "{"
                                          "\"type\": \"animateBaseColor\","
                                          "\"data\": \"[r, g, b, duration]\","
                                          "\"description\": \"Animates towards the new base color.\""
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
    heartBeat(curAlarmRed, curAlarmGreen, curAlarmBlue, 0, RGB_LED_NUM, 5);
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


  switch (curAnimation)
  {
    case RAIN: animateRain(); break;
  }
}





void setBaseColor(int r, int g, int b) {
  baseRed   = r;
  baseGreen = g;
  baseBlue  = b;
  setToBaseColor();
}



void animateBaseColor(int r, int g, int b, int duration) {
  int startRed = baseRed;
  int startGreen = baseGreen;
  int startBlue = baseBlue;
  int progress = 100;
  const int stepSize = 2;

  int progressSteps = ceil(100 / (duration / stepSize));
  if (progressSteps < 1) progressSteps = 1;

  while (progress > 0)
  {
    progress -= progressSteps;
    float perc = LLV((100 - progress) * 255 / 100) * 100 / 255;
    
    baseRed   = round(startRed * (100 - perc) / 100 +   r * perc / 100);
    baseGreen = round(startGreen * (100 - perc) / 100 + g * perc / 100);
    baseBlue  = round(startBlue * (100 - perc) / 100 +  b * perc / 100);
    
    setToBaseColor();
    FastLED.delay(stepSize);
  }
  setBaseColor(r, g, b);
}



// === LIGHT FUNCTION ===

const int maxDropletCount = 30;
const int dropletDuration = 400;
int droplets[maxDropletCount * 2];

int prevRainMillis = millis();
int calcDropletCount() {
  int count = 0;
  for (int i = 0; i < maxDropletCount; i++)
  {
    if (droplets[i * 2 + 1] != 0) count++;
  }
  return count;

}

void animateRain() {
  int dropletCount = calcDropletCount();
  if (dropletCount < maxDropletCount && random(0, 100) < (maxDropletCount - dropletCount) * 2) {
    int dropletIndex = 0;
    for (int i = 0; i < maxDropletCount; i++)
    {
      if (droplets[i * 2 + 1] == 0)
      {
        dropletIndex = i;
        break;
      }
    }
    int LEDIndex = random(0, PIANO_LED_START_INDEX);
    droplets[dropletIndex * 2] = LEDIndex;
    droplets[dropletIndex * 2 + 1] = dropletDuration;
  }

  int dt = millis() - prevRainMillis;
  prevRainMillis = millis();

  for (int i = 0; i < maxDropletCount; i++)
  {
    droplets[i * 2 + 1] -= dt;
    if (droplets[i * 2 + 1] <= 0)
    {
      droplets[i * 2 + 1] = 0;
    }
  }


  for (int i = 0; i < maxDropletCount; i++)
  {
    int durationLeft = round(droplets[i * 2 + 1] * 255 / dropletDuration);
    LEDs[droplets[i * 2]] = CRGB (round(durationLeft * .1), round(durationLeft * .5), durationLeft);
  }
  FastLED.show();

  if (random(0, 20000) < 1) animateLightning();
}

void animateLightning() {
  int count = random(1, 3);
  for (int i = 0; i < count; i++)
  {
    flashLightning(random(100, 200));
    FastLED.delay(random(50, 100));
  }
}

void flashLightning(int duration) {
  int progress = 100;
  while (progress > 0)
  {
    progress -= ceil(100 / (duration / 10));
    for (int i = 0; i < PIANO_LED_START_INDEX; i++)
    {
      LEDs[i] = CRGB (round(progress * 100 / 100), round(progress * 220 / 100), round(progress * 255 / 100));
    }

    FastLED.show();
    FastLED.delay(10);
  }

  for (int i = 0; i < PIANO_LED_START_INDEX; i++)
  {
    LEDs[i] = CRGB (0, 0, 0);
  }
  FastLED.show();
}





void heartBeat(byte r, byte g, byte b, int minLED, int maxLED, int batchSize) {
  float delaySize = 3;
  FastLED.show();

  const int LEDCount = ceil(maxLED - minLED);
  const int halfLEDCount = ceil(LEDCount / 2);

  for (int t = 0; t < halfLEDCount; t += batchSize)
  {
    float val = t * 2 * 255 / LEDCount;
    byte intensity = LLV(val);
    int curR = ceil(r * intensity / 255);
    int curG = ceil(g * intensity / 255);
    int curB = ceil(b * intensity / 255);
    for (int i = 0; i < t; i++)
    {
      LEDs[minLED + halfLEDCount - i] = CRGB (curR, curG, curB);
      LEDs[minLED + halfLEDCount + i] = CRGB (curR, curG, curB);
    }
    FastLED.show();
    FastLED.delay(delaySize);
  }

  for (int t = 0; t < ceil(LEDCount / 2); t += batchSize)
  {
    float val = 255 - t * 2 * 255 / LEDCount;
    byte intensity = LLV(val);
    for (int i = 0; i < t; i++)
    {
      LEDs[minLED + halfLEDCount - i] = CRGB (0, 0, 0);
      LEDs[minLED + halfLEDCount + i] = CRGB (0, 0, 0);
    }

    for (int i = t; i < ceil(LEDCount / 2); i++)
    {
      int curR = ceil(r * intensity / 255);
      int curG = ceil(g * intensity / 255);
      int curB = ceil(b * intensity / 255);

      LEDs[minLED + halfLEDCount - i] = CRGB (curR, curG, curB);
      LEDs[minLED + halfLEDCount + i] = CRGB (curR, curG, curB);
    }

    FastLED.show();
    FastLED.delay(delaySize);
  }
}

void reverseHeartBeat(byte r, byte g, byte b, int minLED, int maxLED, int batchSize) {
  float delaySize = 3;
  FastLED.show();

  const int LEDCount = ceil(maxLED - minLED);
  const int halfLEDCount = ceil(LEDCount / 2);

  for (int t = 0; t < halfLEDCount; t += batchSize)
  {
    float val = t * 2 * 255 / LEDCount;
    byte intensity = LLV(val);
    int curR = ceil(r * intensity / 255);
    int curG = ceil(g * intensity / 255);
    int curB = ceil(b * intensity / 255);
    for (int i = 0; i < t; i++)
    {
      LEDs[minLED + i] = CRGB (curR, curG, curB);
      LEDs[maxLED - i] = CRGB (curR, curG, curB);
    }
    FastLED.show();
    FastLED.delay(delaySize);
  }

  for (int t = 0; t < ceil(LEDCount / 2); t += batchSize)
  {
    float val = 255 - t * 2 * 255 / LEDCount;
    byte intensity = LLV(val);
    for (int i = 0; i < t; i++)
    {
      LEDs[minLED + i] = CRGB (0, 0, 0);
      LEDs[maxLED - i] = CRGB (0, 0, 0);
    }

    for (int i = t; i < ceil(LEDCount / 2); i++)
    {
      int curR = ceil(r * intensity / 255);
      int curG = ceil(g * intensity / 255);
      int curB = ceil(b * intensity / 255);

      LEDs[minLED + i] = CRGB (curR, curG, curB);
      LEDs[maxLED - i] = CRGB (curR, curG, curB);
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
  int maxCount = RGB_LED_NUM;
  if (pianoConnected) maxCount = PIANO_LED_START_INDEX;

  for (int i = 0; i < maxCount; i++) {
    LEDs[i] = CRGB ( baseRed, baseGreen, baseBlue);
  }
  FastLED.show();
}




float LLV(float val) { // Linearize Light Value
  int base = 100;
  float powPart = (pow(base, val / 255) - 1);
  return 255 / (1 - 1 / base) * 1 / base * powPart;
}
