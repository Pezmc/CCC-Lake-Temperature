#include <ArduinoJson.h>
#include <WiFi.h>
#include <FastLED.h>
#include <math.h>

#include "font4x8.h"
#include "pixel_font.h"
#include "font5x8.h"

// WIFI
const char* ssid     = "Camp2019-things";
const char* password = "camp2019";

const char* host = "marekventur.com";
const char* path = "/metrics/";
const int port = 8973;

void connectToWifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// LEDS
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

#define NUM_STRIPS 3
#define NUM_LEDS_PER_STRIP 288
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

#define LED_PIN_1 2
#define LED_PIN_2 4
#define LED_PIN_3 13

void setUpLEDS() {
  Serial.println("Setting up LEDs");
  
  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds[0], NUM_LEDS_PER_STRIP).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds[1], NUM_LEDS_PER_STRIP).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<LED_TYPE, LED_PIN_3, COLOR_ORDER>(leds[2], NUM_LEDS_PER_STRIP).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);

  // Clear display
  fill_solid(leds[0], NUM_LEDS_PER_STRIP, CRGB::Black);
  fill_solid(leds[1], NUM_LEDS_PER_STRIP, CRGB::Black);
  fill_solid(leds[2], NUM_LEDS_PER_STRIP, CRGB::Black);
  FastLED.show();
}

CRGB colors[3] = { CRGB::Red, CRGB::Green, CRGB::Blue };

void flashMatrixRGB() {
  Serial.println("Flashing RGB");

  int numberOfColors = sizeof(colors) / sizeof(CRGB);
  for (int i = 0; i < numberOfColors; i++) {
    drawAllLEDS(colors[i]);
    FastLED.show();
    FastLED.delay(500);
  }

  drawMatrixRows(CRGB::White);
  FastLED.show();
  FastLED.delay(1000);

  drawAllLEDS(CRGB::Black);
  FastLED.show();
  FastLED.delay(1);
}

void drawAllLEDS(const struct CRGB & color) {
  fill_solid(leds[0], NUM_LEDS_PER_STRIP, color);
  fill_solid(leds[1], NUM_LEDS_PER_STRIP, color);
  fill_solid(leds[2], NUM_LEDS_PER_STRIP, color);
}

// Matrix
// Matrix width and height
const uint8_t kMatrixWidth = 144;
const uint8_t kMatrixHeight = 6;

// Matrix pixel layouts
const bool kMatrixSerpentineLayout = true; // zig zagging data line
const bool kMatrixXFlipped = true; // rows wired 2,1,4,3

// XY Position helpers
uint16_t XY(uint8_t x, uint8_t y)
{
  uint16_t i;

  if (kMatrixXFlipped) {
    if( y & 0x01) {
      y -= 1;
    } else {
      y += 1;
    }
  }

  if(kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if(kMatrixSerpentineLayout == true) {
    if(y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  
  return i;
}

void setFontPixel(uint8_t row, uint8_t column, const struct CRGB & color){
  int ledNumber = XY(column, row); // 0 - 864

  int arrayNumber = floor(ledNumber / NUM_LEDS_PER_STRIP); // 0 - 2
  int ledNumberInArray = ledNumber % NUM_LEDS_PER_STRIP;
  
  Serial.println(String("Setting ") + row + ":" + column + " led no=" + ledNumberInArray + " in array " + arrayNumber + " to " + color);
  
  leds[arrayNumber][ledNumberInArray] = color;
}

void drawMatrixRows(const struct CRGB & color) {
  for (int y = 0; y < kMatrixHeight; y++) {
    for (int x = 0; x <= y; x++) {
      setFontPixel(y, x, color);
    }
  }
}

// JSON
DynamicJsonDocument doc(300);
DynamicJsonDocument* getData() {  
  Serial.print("Making request to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port)) {
      Serial.println("Connection failed");
      return nullptr;
  }

  Serial.print("Requesting URL: ");
  Serial.println(path);

  // This will send the request to the server
  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return nullptr;
    }
  }
  
  String responseString = "";
  while(client.available()) {
      responseString = client.readStringUntil('\r');
  }

  DeserializationError error = deserializeJson(doc, responseString);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    Serial.print(responseString);
    return nullptr;
  }

  /*
  rh: 33.17,
  rssi: -81,
  t: 32.34,
  status_message: "STALE",
  status: 1,
  updated: 1566569145722
  */

  return &doc;
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  setUpLEDS();
  flashMatrixRGB();

  connectToWifi();    
}

void loop() {
  DynamicJsonDocument* dataPointer = getData();
  if (!dataPointer) {
    Serial.print("No data returned!");
    delay(5000);
    return;
  }
  
  DynamicJsonDocument json = *getData();

  double temperature = json["t"];
  double relative_humidity = json["rh"];

  Serial.print("temperature: ");
  Serial.println(temperature);

  Serial.print("humidity: ");
  Serial.println(relative_humidity);

  delay(1000);
}
