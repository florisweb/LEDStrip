#include "FastLED.h"
#include "connectionManager.h"

#define RGB_LED_NUM  300
#define PIANO_LED_START_INDEX  223 // The index at which the strip is behind the piano
#define PIANO_LED_NUM (RGB_LED_NUM - PIANO_LED_START_INDEX) // The index at which the strip is behind the piano

#define CHIP_SET     WS2812B       // types of RGB LEDs
#define COLOR_CODE   GRB           // sequence of colors in data stream

CRGB LEDs[RGB_LED_NUM];




// Sensors
const int IRSensorPin = 18;
bool IRSensorState = false;

const int LDRHistoryLength = 50;
const int LDRInsidePin = 32;
const int LDROutsidePin = 33;

float LDRInsideRunningAverage = 0;
float LDROutsideRunningAverage = 0;
int insideLightLevel = 0;
int outsideLightLevel = 0;

const int signalPin = 5;



const char* ssid = "";
const char* password = "";
const String deviceId = "LEDStrip";
const String deviceKey = "";

byte curAlarmRed, curAlarmGreen, curAlarmBlue;
float baseRed = 0;
float baseGreen = 0;
float baseBlue = 0;

enum Animations { NONE, RAIN };
Animations curAnimation = NONE;




#define PIANO_KEY_NUM 87
const float maxKeyDuration = 3000; // ms
const int LEDSPerKey = 3;
const int pianoSleepTimeout = 2 * 60 * 1000;
unsigned int lastPianoEvent = 0;
bool pianoConnected = false;

int pianoKeyVelocities[PIANO_KEY_NUM];
int pianoKeyDurations[PIANO_KEY_NUM];




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
        setLED(curLEDData[0], curLEDData[1], curLEDData[2], curLEDData[3]);
      }
    }
    FastLED.show();
  } else if (packetType == "setPianoKeys") {
    if (!pianoConnected) setPianoConnectState(true);
    lastPianoEvent = millis();
    JsonArray LEDDataJSON = message["data"];

    int curKeyData[3]; // Key-index, velocity, duration
    int curIndex = 0;
    for (JsonVariant v : LEDDataJSON) {
      curKeyData[curIndex % 3] = v.as<int>();
      curIndex++;
      if (curIndex % 3 == 0)
      {
        pianoKeyVelocities[curKeyData[0]] = curKeyData[1];
        pianoKeyDurations[curKeyData[0]] = curKeyData[2];
      }
    }
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
    setPianoConnectState(message["data"]);
  } else if (packetType == "curState") {
    setBaseColor(message["data"]["baseColor"][0], message["data"]["baseColor"][1], message["data"]["baseColor"][2]);
    // Explicitly not setting the pianoConnectState as the LEDStrip can put it into 'sleep' mode itself
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
  pinMode(LDRInsidePin, INPUT);
  pinMode(LDROutsidePin, INPUT);

  Serial.println("Setting up WiFi...");

  ConnectionManager.defineEventDocs("["
                                    "{"
                                    "\"type\": \"IRSensorEvent\","
                                    "\"data\": \"bool\","
                                    "\"description\": \"True on person detect, false on no-person detect\""
                                    "},"
                                    "{"
                                    "\"type\": \"InsideLightLevelChangeEvent\","
                                    "\"data\": \"int 0-10\","
                                    "\"description\": \"Fired when the lightlevel changes. Light level from 0 (dark) to 10 (dim).\""
                                    "},"
                                    "{"
                                    "\"type\": \"OutsideLightLevelChangeEvent\","
                                    "\"data\": \"int 0-10\","
                                    "\"description\": \"Fired when the lightlevel changes. Light level from 0 (dark) to 10 (dim).\""
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
                                          "},"
                                          "{"
                                          "\"type\": \"setPianoKeys\","
                                          "\"data\": \"[key-index 1, velocity, duration (ms) ...]\","
                                          "\"description\": \"Animates/fades the respective keys with the correct color, determined by the velocity, over the provided duration.\""
                                          "}"
                                          "]");
  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);
}




unsigned int prevMillis = 0;
bool curIRSensorState = false;
int prevInsideLightLevel = 0;
int prevOutsideLightLevel = 0;
void loop() {
  ConnectionManager.loop();

  if (millis() - prevMillis > 60000 / 30 && curAlarmRed + curAlarmGreen + curAlarmBlue > 0)
  {
    prevMillis = millis();
    heartBeat(curAlarmRed, curAlarmGreen, curAlarmBlue, 0, RGB_LED_NUM, 5);
  }


  updateIRSensor();
  updateLDRs();


  if (millis() - lastPianoEvent > pianoSleepTimeout && pianoConnected) setPianoConnectState(false);

  // Animations
  updateBaseColorAnimation();
  updatePianoKeyLights();

  switch (curAnimation)
  {
    case RAIN: animateRain(); break;
  }
}


void updateIRSensor() {
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

void updateLDRs() {
  LDRInsideRunningAverage = LDRInsideRunningAverage * (LDRHistoryLength - 1) / LDRHistoryLength + analogRead(LDRInsidePin) / LDRHistoryLength;
  LDROutsideRunningAverage = LDROutsideRunningAverage * (LDRHistoryLength - 1) / LDRHistoryLength + analogRead(LDROutsidePin) / LDRHistoryLength;

  insideLightLevel = LDRInsideRunningAverage / 410; // Scale it to a 0-10 range
  outsideLightLevel = LDROutsideRunningAverage / 410; // Scale it to a 0-10 range

  if (millis() % 100 != 0) return;
  if (insideLightLevel != prevInsideLightLevel)
  {
    String dataString = "{\"type\": \"InsideLightLevelChangeEvent\", \"data\": ";
    dataString.concat(insideLightLevel);
    dataString.concat("}");
    ConnectionManager.send(dataString);
  }
  if (outsideLightLevel != prevOutsideLightLevel)
  {
    String dataString = "{\"type\": \"OutsideLightLevelChangeEvent\", \"data\": ";
    dataString.concat(outsideLightLevel);
    dataString.concat("}");
    ConnectionManager.send(dataString);
  }

  prevInsideLightLevel = insideLightLevel;
  prevOutsideLightLevel = outsideLightLevel;
}











// === PIANO-SECTION ===

unsigned int prevPianoLightUpdate = millis();
void updatePianoKeyLights() {
  int dt = millis() - prevPianoLightUpdate;
  prevPianoLightUpdate = millis();

  for (int k = 0; k < PIANO_KEY_NUM; k++)
  {
    if (pianoKeyDurations[k] == 0) continue;
    pianoKeyDurations[k] = max(0, pianoKeyDurations[k] - dt);

    int curLEDIndex = floor(k * PIANO_LED_NUM / PIANO_KEY_NUM) + PIANO_LED_START_INDEX;
    float perc = pianoKeyDurations[ k] / maxKeyDuration;
    float intensity = min((2.0 / (2.0 - perc * 0.5) - 1.0) / 0.3, 1.0);

    int r, g, b;
    HSVtoRGB(max(min(pianoKeyVelocities[k] / 100.0, 1.0), 0.0) * 1.2, 1.0, intensity, r, g, b);
    for (int i = 0; i < LEDSPerKey; i++) setLED(curLEDIndex + i, r, g, b);
  }
  FastLED.show();
}

void setPianoConnectState(bool _connected) {
  pianoConnected = _connected;
  lastPianoEvent = millis();
  if (pianoConnected) {
    heartBeat(0, 255, 30, PIANO_LED_START_INDEX - 2, RGB_LED_NUM + 1, 1);
  } else {
    reverseHeartBeat(255, 0, 30, PIANO_LED_START_INDEX - 2, RGB_LED_NUM + 1, 1);
  }
  setToBaseColor();
}











// === BASE COLOR ===
void setBaseColor(float r, float g, float b) {
  baseRed   = r;
  baseGreen = g;
  baseBlue  = b;
  setToBaseColor();
}

unsigned int baseColorAnimationStart = millis();
int baseColorAnimationDuration = 0;
float animateBaseColorToR = 0;
float animateBaseColorToG = 0;
float animateBaseColorToB = 0;
float animateBaseColorFromR = 0;
float animateBaseColorFromG = 0;
float animateBaseColorFromB = 0;

void updateBaseColorAnimation() {
  if (baseColorAnimationDuration == 0) return;
  if (millis() - baseColorAnimationStart >= baseColorAnimationDuration) // Finished animation
  {
    baseColorAnimationDuration = 0;
    baseRed = animateBaseColorToR;
    baseGreen = animateBaseColorToG;
    baseBlue = animateBaseColorToB;
    setToBaseColor();
  } else {
    float timePerc = (millis() - baseColorAnimationStart) * 100.0 / baseColorAnimationDuration;

    baseRed   = animateBaseColorFromR * (100.0 - timePerc) / 100.0 + animateBaseColorToR * timePerc / 100.0;
    baseGreen = animateBaseColorFromG * (100.0 - timePerc) / 100.0 + animateBaseColorToG * timePerc / 100.0;
    baseBlue  = animateBaseColorFromB * (100.0 - timePerc) / 100.0 + animateBaseColorToB * timePerc / 100.0;
    setToBaseColor();
  }
}

void animateBaseColor(int r, int g, int b, int duration) {
  baseColorAnimationStart = millis();
  baseColorAnimationDuration = duration;
  animateBaseColorToR = r;
  animateBaseColorToG = g;
  animateBaseColorToB = b;

  animateBaseColorFromR = baseRed;
  animateBaseColorFromG = baseGreen;
  animateBaseColorFromB = baseBlue;
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
    setLED(droplets[i * 2], round(durationLeft * .1), round(durationLeft * .5), durationLeft);
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
  if (pianoConnected) maxCount = PIANO_LED_START_INDEX - 1;

  int trueRed = LLV(baseRed);
  int trueGreen = LLV(baseGreen);
  int trueBlue = LLV(baseBlue);

  for (int i = 0; i < maxCount; i++) {
    LEDs[i] = CRGB(trueRed, trueGreen, trueBlue);
  }
  FastLED.show();
}

void setLED(int index, int r, int g, int b) {
  int trueRed = LLV(r);
  int trueGreen = LLV(g);
  int trueBlue = LLV(b);

  LEDs[index] = CRGB(r, g, b);
}






// === UTILITIES ===
float LLV(float val) { // Linearize Light Value
  int base = 100;
  float powPart = (pow(base, val / 255) - 1);
  return 255 / (1 - 1 / base) * 1 / base * powPart;
}


void HSVtoRGB(float h, float s, float v, int &r, int &g, int &b) {
  int i = floor(h * 6);
  float f = h * 6 - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);
  switch (i % 6) {
    case 0: r = v * 255, g = t * 255, b = p * 255; break;
    case 1: r = q * 255, g = v * 255, b = p * 255; break;
    case 2: r = p * 255, g = v * 255, b = t * 255; break;
    case 3: r = p * 255, g = q * 255, b = v * 255; break;
    case 4: r = t * 255, g = p * 255, b = v * 255; break;
    case 5: r = v * 255, g = p * 255, b = q * 255; break;
  }
}
