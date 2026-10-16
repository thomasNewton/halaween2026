// Pumpkin Lights - Phase 3: light shows, blinking, automatic idle mode,
// and a captive portal that opens the control page when a phone connects
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

// ---------- Settings you can change ----------
const char* WIFI_NAME = "Pumpkin-Lights";
const char* WIFI_PASSWORD = "boo12345";   // must be 8+ characters

const int NUM_LEDS = 4;

// GPIO pin for each color of each LED: {Red, Green, Blue}
const int LED_PINS[NUM_LEDS][3] = {
  {13, 14, 16},   // LED 1
  {17, 18, 19},   // LED 2
  {21, 22, 23},   // LED 3
  {25, 26, 27}    // LED 4
};

const int PWM_FREQ = 5000;   // how fast the pins flicker on/off (Hz)
const int PWM_BITS = 8;      // 8 bits = brightness values from 0 to 255

// Auto mode: if nobody touches the controls for this long, the pumpkin
// starts playing the light shows on its own.
const unsigned long IDLE_TIMEOUT_MS = 60000;   // 60 seconds
const unsigned long SHOW_LENGTH_MS  = 20000;   // each show plays 20 seconds in auto mode

const int MAX_BLINK_RATE = 20;   // fastest blink: 20 blinks per second

// Chase show settings
const float CHASE_SLOWEST = 1.0;                    // slowest speed (hops per second)
const float CHASE_FASTEST = 5.0;                    // fastest speed (hops per second)
const unsigned long CHASE_SPEED_CYCLE_MS = 12000;   // time for slow -> fast -> slow again
const unsigned long CHASE_COLOR_CYCLE_MS = 30000;   // time for the colors to go around the whole color wheel
const int CHASE_BACKGROUND_PERCENT = 40;            // how bright the resting lights are (0-100)

// The different things the lights can be doing
const int MODE_MANUAL  = 0;   // the colors a person picked
const int MODE_RAINBOW = 1;   // show 1
const int MODE_CANDLE  = 2;   // show 2
const int MODE_CHASE   = 3;   // show 3
const int NUM_SHOWS    = 3;

// ---------- Things the program remembers ----------
int colors[NUM_LEDS][3];     // the colors a person picked (red, green, blue: 0-255)
int blinkRate[NUM_LEDS];     // 0 = steady, 1 to 20 = blinks per second
bool powerOn = true;         // master on/off switch
int brightness = 255;        // master brightness (0-255)
int lightMode = MODE_MANUAL; // which mode a person chose

bool autoMode = true;        // true = running shows by itself (starts this way)
unsigned long autoStart = 0; // when auto mode began
unsigned long lastTouch = 0; // last time a person used the controls

int frame[NUM_LEDS][3];      // the colors to show right now (before brightness)

// Memory for the candle flicker show
int flicker[NUM_LEDS];
unsigned long nextFlicker[NUM_LEDS];

// Memory for the chase show
float chasePos = 0;              // where the hopping light is (0 up to NUM_LEDS)
unsigned long lastChaseTime = 0; // when we last moved it

WebServer server(80);
DNSServer dnsServer;   // answers every web address with the ESP32's own address

// ---------- Helpers ----------

// Call this whenever a person uses a control. It restarts the idle timer
// and stops auto mode.
void userTouched() {
  lastTouch = millis();
  autoMode = false;
}

// Which mode is actually showing right now?
int currentMode() {
  if (autoMode) {
    // Rotate through shows 1, 2, 3, 1, 2, 3...
    return 1 + (int)(((millis() - autoStart) / SHOW_LENGTH_MS) % NUM_SHOWS);
  }
  return lightMode;
}

// If nobody has touched anything for a while, start auto mode
void checkIdle() {
  if (!autoMode && millis() - lastTouch > IDLE_TIMEOUT_MS) {
    autoMode = true;
    autoStart = millis();
    powerOn = true;   // make sure the lights come back on
  }
}

// Turn a color wheel position (0-359 degrees) into red, green, blue.
// The answer goes into rgb[0] (red), rgb[1] (green), rgb[2] (blue).
void hueToRgb(int hue, int* rgb) {
  int region = hue / 60;                  // which sixth of the wheel (0-5)
  int rising = (hue % 60) * 255 / 60;     // goes 0 up to 255 across a region
  int falling = 255 - rising;             // goes 255 down to 0
  switch (region) {
    case 0:  rgb[0] = 255;     rgb[1] = rising;  rgb[2] = 0;       break;
    case 1:  rgb[0] = falling; rgb[1] = 255;     rgb[2] = 0;       break;
    case 2:  rgb[0] = 0;       rgb[1] = 255;     rgb[2] = rising;  break;
    case 3:  rgb[0] = 0;       rgb[1] = falling; rgb[2] = 255;     break;
    case 4:  rgb[0] = rising;  rgb[1] = 0;       rgb[2] = 255;     break;
    default: rgb[0] = 255;     rgb[1] = 0;       rgb[2] = falling; break;
  }
}

// ---------- The light modes ----------

// Mode 0: the colors a person picked, with optional blinking
void showManual(unsigned long now) {
  for (int led = 0; led < NUM_LEDS; led++) {
    bool visible = true;
    if (blinkRate[led] > 0) {
      unsigned long period = 1000 / blinkRate[led];   // milliseconds per blink
      visible = (now % period) < (period / 2);        // on for half, off for half
    }
    for (int c = 0; c < 3; c++) {
      frame[led][c] = visible ? colors[led][c] : 0;
    }
  }
}

// Show 1: Rainbow Roll - colors slide around the wheel, each LED a different color
void showRainbow(unsigned long now) {
  for (int led = 0; led < NUM_LEDS; led++) {
    int hue = (now / 20 + led * 90) % 360;
    hueToRgb(hue, frame[led]);
  }
}

// Show 2: Candle Flicker - each LED flickers like a candle,
// using the color a person picked for that LED
void showCandle(unsigned long now) {
  for (int led = 0; led < NUM_LEDS; led++) {
    if (now >= nextFlicker[led]) {
      flicker[led] = random(60, 256);           // pick a new random brightness
      nextFlicker[led] = now + random(40, 140); // and when to change again
    }
    for (int c = 0; c < 3; c++) {
      frame[led][c] = colors[led][c] * flicker[led] / 255;   // dim the picked color
    }
  }
}

// Show 3: Chase - one bright light hops around while the others rest.
// The colors slowly travel around the color wheel, and the hopping speed
// slowly rises and falls.
void showChase(unsigned long now) {
  // 1. Work out the speed. A sine wave makes it glide smoothly
  //    between CHASE_SLOWEST and CHASE_FASTEST.
  float angle = TWO_PI * (now % CHASE_SPEED_CYCLE_MS) / (float)CHASE_SPEED_CYCLE_MS;
  float middle = (CHASE_FASTEST + CHASE_SLOWEST) / 2;
  float swing  = (CHASE_FASTEST - CHASE_SLOWEST) / 2;
  float speed  = middle + swing * sinf(angle);

  // 2. Move the hopping light forward: distance = speed x time passed
  float seconds = (now - lastChaseTime) / 1000.0;
  lastChaseTime = now;
  chasePos = fmodf(chasePos + speed * seconds, NUM_LEDS);
  int active = (int)chasePos % NUM_LEDS;

  // 3. Pick the colors. The hopping light slowly travels around the color
  //    wheel, and the resting lights use the opposite color.
  int hue = (now % CHASE_COLOR_CYCLE_MS) * 360 / CHASE_COLOR_CYCLE_MS;
  int hopColor[3];
  int restColor[3];
  hueToRgb(hue, hopColor);
  hueToRgb((hue + 180) % 360, restColor);

  // 4. Give each LED its color
  for (int led = 0; led < NUM_LEDS; led++) {
    for (int c = 0; c < 3; c++) {
      if (led == active) {
        frame[led][c] = hopColor[c];
      } else {
        frame[led][c] = restColor[c] * CHASE_BACKGROUND_PERCENT / 100;
      }
    }
  }
}

// Work out the colors for this moment and send them to the 12 pins
void updateLeds() {
  unsigned long now = millis();
  int mode = currentMode();

  if (mode == MODE_RAINBOW)      showRainbow(now);
  else if (mode == MODE_CANDLE)  showCandle(now);
  else if (mode == MODE_CHASE)   showChase(now);
  else                           showManual(now);

  for (int led = 0; led < NUM_LEDS; led++) {
    for (int c = 0; c < 3; c++) {
      int duty = 0;                                   // off by default
      if (powerOn) {
        duty = frame[led][c] * brightness / 255;      // scale by brightness
      }
      ledcWrite(led * 3 + c, duty);                   // PWM channel 0-11
    }
  }
}

// ---------- Web page handlers ----------
// These only change the settings. The loop() below does the actual drawing.

void handleRoot() {
  File page = LittleFS.open("/index.html", "r");
  if (!page) {
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  server.streamFile(page, "text/html");
  page.close();
}

// Phones ask for test pages (like /generate_204) to check for internet.
// Anything we don't recognize gets sent to our main page, which makes
// the phone pop up the "sign in to network" window.
void handleNotFound() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(302, "text/plain", "");
}

// /state - tells the web page what the lights are doing right now.
// (This does NOT count as a person touching the controls.)
void handleState() {
  String json = "{\"on\":";
  json += powerOn ? "true" : "false";
  json += ",\"brightness\":" + String(brightness);
  json += ",\"mode\":" + String(lightMode);
  json += ",\"showing\":" + String(currentMode());
  json += ",\"auto\":";
  json += autoMode ? "true" : "false";

  json += ",\"colors\":[";
  for (int led = 0; led < NUM_LEDS; led++) {
    char hex[8];
    snprintf(hex, sizeof(hex), "#%02x%02x%02x",
             colors[led][0], colors[led][1], colors[led][2]);
    json += "\"" + String(hex) + "\"";
    if (led < NUM_LEDS - 1) json += ",";
  }

  json += "],\"blink\":[";
  for (int led = 0; led < NUM_LEDS; led++) {
    json += String(blinkRate[led]);
    if (led < NUM_LEDS - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// /power?on=1  (or on=0)
void handlePower() {
  powerOn = server.arg("on") == "1";
  userTouched();
  server.send(200, "text/plain", "ok");
}

// /brightness?value=128
void handleBrightness() {
  brightness = constrain(server.arg("value").toInt(), 0, 255);
  userTouched();
  server.send(200, "text/plain", "ok");
}

// /mode?value=2   (0 = my colors, 1-3 = light shows)
void handleMode() {
  lightMode = constrain(server.arg("value").toInt(), 0, NUM_SHOWS);
  userTouched();
  server.send(200, "text/plain", "ok");
}

// /color?led=0&r=255&g=100&b=0   (led numbers start at 0)
void handleColor() {
  int led = server.arg("led").toInt();
  if (led < 0 || led >= NUM_LEDS) {
    server.send(400, "text/plain", "bad led number");
    return;
  }
  colors[led][0] = constrain(server.arg("r").toInt(), 0, 255);
  colors[led][1] = constrain(server.arg("g").toInt(), 0, 255);
  colors[led][2] = constrain(server.arg("b").toInt(), 0, 255);
  lightMode = MODE_MANUAL;   // picking a color switches to "my colors"
  userTouched();
  server.send(200, "text/plain", "ok");
}

// /blink?led=0&rate=5   (rate 0 = steady, 1-20 = blinks per second)
void handleBlink() {
  int led = server.arg("led").toInt();
  if (led < 0 || led >= NUM_LEDS) {
    server.send(400, "text/plain", "bad led number");
    return;
  }
  blinkRate[led] = constrain(server.arg("rate").toInt(), 0, MAX_BLINK_RATE);
  lightMode = MODE_MANUAL;   // blinking only works in "my colors" mode
  userTouched();
  server.send(200, "text/plain", "ok");
}

// ---------- Startup ----------
void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);

  // Set up each of the 12 PWM channels. Every LED starts pumpkin orange.
  for (int led = 0; led < NUM_LEDS; led++) {
    colors[led][0] = 255;
    colors[led][1] = 100;
    colors[led][2] = 0;
    blinkRate[led] = 0;
    flicker[led] = 255;
    nextFlicker[led] = 0;
    for (int c = 0; c < 3; c++) {
      int channel = led * 3 + c;
      ledcSetup(channel, PWM_FREQ, PWM_BITS);
      ledcAttachPin(LED_PINS[led][c], channel);
    }
  }

  autoStart = millis();   // we begin in auto mode, so the pumpkin
  lastTouch = millis();   // runs its shows as soon as it's plugged in

  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Web page address: http://");
  Serial.println(WiFi.softAPIP());

  // Captive portal: answer every web address with the ESP32's own address
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/power", handlePower);
  server.on("/brightness", handleBrightness);
  server.on("/mode", handleMode);
  server.on("/color", handleColor);
  server.on("/blink", handleBlink);
  server.onNotFound(handleNotFound);   // anything else -> redirect to main page
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();   // answer phones' address lookups
  server.handleClient();            // listen for phones
  checkIdle();                      // has it been quiet for a while?

  // Redraw the lights every 5 milliseconds (fast enough for 20 blinks/second)
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw >= 5) {
    lastDraw = millis();
    updateLeds();
  }
}