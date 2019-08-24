#include <ArduinoJson.h>
#include <WiFi.h>
#include <FastLED.h>
#include <math.h>

#include <font5x8.h>
#include "pixel_font.h"

///------ WIFI
const char* ssid     = "Camp2019-things";
const char* password = "camp2019";

const char* host = "marekventur.com";
const int port = 8973;

const char* path = "/metrics/";
const char* historyPath = "/metrics/history?size=144";
const char* statsPath = "/metrics/stats";

void connectToWifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
}

void waitForWifi() {
  while (!connectedToWifi()) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

bool connectedToWifi() {
  return WiFi.status() == WL_CONNECTED;
}

///------ LEDS
#define BRIGHTNESS  96
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
    FastLED.delay(250);
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

void clearScreen() {
  drawAllLEDS(CRGB::Black);
}

void blankScreen() {
  clearScreen();
  FastLED.delay(1);
}

///------ Matrix
// Matrix width and height
const uint8_t kMatrixWidth = 144;
const uint8_t kMatrixHeight = 6;

// Matrix pixel layouts
const bool kMatrixSerpentineLayout = true; // zig zagging data line
const bool kMatrixXFlipped = true; // rows wired 2,1,4,3

PixelFont Font5x8 = PixelFont(5, 8, 8, font5x8, setFontPixel);

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

  //Serial.println(String("Setting ") + row + ":" + column + " led no=" + ledNumberInArray + " in array " + arrayNumber + " to " + color);  

  if (arrayNumber >= NUM_STRIPS) {
    Serial.print("Tried to write to array number bigger than arrays; stripNumber: ");
    Serial.println(arrayNumber);
    return;
  }

  if (ledNumberInArray >= NUM_LEDS_PER_STRIP) {
    Serial.print("Tried to write to led number longer than strip; ledNumber: ");
    Serial.println(ledNumberInArray);
    return;
  }
  
  //delay(1);
  
  leds[arrayNumber][ledNumberInArray] = color;
}

void drawMatrixRows(const struct CRGB & color) {
  for (int y = 0; y < kMatrixHeight; y++) {
    for (int x = 0; x <= y; x++) {
      setFontPixel(y, x, color);
    }
  }
}

void displayCenteredStringFor(const char *msg, uint8_t row, PixelFont font, const struct CRGB & color, const struct CRGB & background, int timeMs) {
  displayCenteredString(msg, row, font, color, background);
  FastLED.delay(timeMs);
}


void displayCenteredString(const char *msg, uint8_t row, PixelFont font, const struct CRGB & color, const struct CRGB & background) {
  drawCenteredString(msg, row, font, color, background);
  FastLED.show();
}

void drawCenteredString(const char *msg, uint8_t row, PixelFont font, const struct CRGB & color, const struct CRGB & background){
  int messageLength = strlen(msg) * font.width;
  int startingColumn = (kMatrixWidth - messageLength) / 2; 
  
  drawString(msg, row, startingColumn, font, color, CRGB::Black);
}

void drawString(const char *msg, uint8_t row, uint8_t col, PixelFont font, const struct CRGB & color, const struct CRGB & background){
  for(uint8_t i = 0; i < strlen(msg); i++){
    drawChar(msg[i], row, col + i * font.width, color, font);
  }
}

void drawChar(uint8_t ascii, uint8_t row, uint8_t col, const struct CRGB & color, PixelFont font) {
  byte *data = font.font + ascii * font.bytes_per_char;
  for(uint8_t r=0; r<font.height; r++){
    for(uint8_t c=0; c<font.width; c++){
      if((data[r] >> (8 - 1 - c)) & 1){
        setFontPixel(row + r, col + c, color);
      }
    }
  }
}

///------ JSON
DynamicJsonDocument doc(300);
DynamicJsonDocument* getJSONData(jsonPath) {
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
  Serial.println(jsonPath);

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

void displayLakeTemperature(bool humidity) {
  DynamicJsonDocument* dataPointer = getJSONData(path);
  if (!dataPointer) {
    Serial.print("No data returned! Trying again in 15 seconds");
    delay(15000);
    return;
  }
  
  DynamicJsonDocument json = *dataPointer;

  double temperature = json["t"];
  double relative_humidity = json["rh"];

  Serial.print("temperature: ");
  Serial.println(temperature);

  Serial.print("humidity: ");
  Serial.println(relative_humidity);

  clearScreen();

  if (!humidity) {
    String message = "LIVE LAKE TEMP: " + String(temperature, 2) + " C!";
    const char* messageChars = message.c_str();
    displayCenteredString(messageChars, -1, Font5x8, CRGB::Blue, CRGB::Black);
  } else { 
    String message = "LIVE LAKE HUMIDITY: " + String(relative_humidity, 2) + "%!";
    const char* messageChars = message.c_str();
    displayCenteredString(messageChars, -1, Font5x8, CRGB::Blue, CRGB::Black);
  }
}

DynamicJsonDocument* getGraphData() {  
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

void connectToWifiWithStatusUpdates() {
  connectToWifi();
  displayCenteredString("Connecting to wifi...", -1, Font5x8, CRGB::White, CRGB::Black);
  waitForWifi();
  blankScreen();
  displayCenteredStringFor("Connected!", -1, Font5x8, CRGB::White, CRGB::Black, 2500);
  blankScreen();
}

// ------ Colors

CRGBPalette16 currentPalette = RainbowColors_p;
TBlendType    currentBlending = LINEARBLEND;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
  for ( int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV( random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;

}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;

  currentPalette = CRGBPalette16(
                     green,  green,  black,  black,
                     purple, purple, black,  black,
                     green,  green,  black,  black,
                     purple, purple, black,  black );
}

void changePalette()
{
  randomSeed(analogRead(0));

  int number = random(0, 11);

  if ( number ==  0)  {
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
  }
  if ( number == 1)  {
    currentPalette = RainbowStripeColors_p;
    currentBlending = NOBLEND;
  }
  if ( number == 2)  {
    currentPalette = RainbowStripeColors_p;
    currentBlending = LINEARBLEND;
  }
  if ( number == 3)  {
    SetupPurpleAndGreenPalette();
    currentBlending = LINEARBLEND;
  }
  if ( number == 4)  {
    SetupTotallyRandomPalette();
    currentBlending = LINEARBLEND;
  }
  if ( number == 5)  {
    SetupBlackAndWhiteStripedPalette();
    currentBlending = NOBLEND;
  }
  if ( number == 6)  {
    SetupBlackAndWhiteStripedPalette();
    currentBlending = LINEARBLEND;
  }
  if ( number == 7)  {
    currentPalette = CloudColors_p;
    currentBlending = LINEARBLEND;
  }
  if ( number == 8)  {
    currentPalette = PartyColors_p;
    currentBlending = LINEARBLEND;
  }
  if ( number == 9)  {
    currentPalette = myRedWhiteBluePalette_p;
    currentBlending = NOBLEND;
  }
  if ( number == 10)  {
    currentPalette = myRedWhiteBluePalette_p;
    currentBlending = LINEARBLEND;
  }
}

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Red,
  CRGB::Gray,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black
};


void fillLEDsFromPaletteColors(int startColor)
{
  uint8_t brightness = 255;

  int colorIndex = startColor;
  for (int x = 0; x < kMatrixWidth; x++) {  
    for (int y = 0; y < kMatrixHeight; y++) {
      int ledNumber = XY(x, y);
      int arrayNumber = floor(ledNumber / NUM_LEDS_PER_STRIP); // 0 - 2
      int ledNumberInArray = ledNumber % NUM_LEDS_PER_STRIP;
  
      leds[arrayNumber][ledNumberInArray] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    }

    colorIndex += 3;
  }
}

void scrollThroughColors(int displaySeconds) {
  int updatesPerSecond = 50;

  int numberOfUpdatesToDisplay = displaySeconds * updatesPerSecond;
  for (int i = 0; i < numberOfUpdatesToDisplay; i++) {
    fillLEDsFromPaletteColors(i * 3);
    FastLED.delay(1000 / updatesPerSecond);
  }
}

// ------ MAIN
void setup()
{
  Serial.begin(115200);
  delay(10);

  setUpLEDS();
  flashMatrixRGB();

  connectToWifiWithStatusUpdates();
}


void loop() {
  delay(5); // safety delay

  if (!connectedToWifi()) {
    displayCenteredStringFor("Wifi Disconnected...", -1, Font5x8, CRGB::White, CRGB::Black, 500);
    connectToWifiWithStatusUpdates();
  }

  for (int i = 0; i < 3; i++) {
    displayLakeTemperature(false);
    FastLED.delay(5000);
  }
 
  changePalette();
  clearScreen();
  scrollThroughColors(5);

  displayLakeTemperature(true);
  FastLED.delay(5000);
}


// TODO DEAD LAKE TEMP.
