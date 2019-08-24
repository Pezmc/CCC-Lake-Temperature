#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <FastLED.h>
#include <math.h>

#include <font5x8.h>
#include "pixel_font.h"

const bool DEBUG = false;

///------ WIFI
const char* ssid     = "Camp2019-things";
const char* password = "camp2019";
const char* localDNS = "LiveLakeDisplay";

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

// ------ OTA Updates

WebServer server(80);
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

void setUpOTAUpdates() {
  /*use mdns for host name resolution*/
  if (!MDNS.begin(localDNS)) {
    Serial.println("Error setting up MDNS responder!");
    delay(15000);
    return;
  }
  Serial.println("mDNS responder started");

  /*return index page which is stored in serverIndex */
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  Serial.println("Server configured");
  
  server.begin();
}

void handleClientRequest() {
  delay(1); // safety delay
  server.handleClient();
  delay(1); // safety delay
}

///------ LEDS
#define BRIGHTNESS  255
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
    if (DEBUG) {
      Serial.print("Tried to write to array number bigger than arrays; stripNumber: ");
      Serial.println(arrayNumber);
    }
    return;
  }

  if (ledNumberInArray >= NUM_LEDS_PER_STRIP) {
    if (DEBUG) {
      Serial.print("Tried to write to led number longer than strip; ledNumber: ");
      Serial.println(ledNumberInArray);
    }
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

  Serial.print("Displaying ");
  Serial.println(msg);
  
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
DynamicJsonDocument* getJSONData(const char* jsonPath) {
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
  client.print(String("GET ") + jsonPath + " HTTP/1.1\r\n" +
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

// ------ Graphing
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

void displayLakeStats() {
  DynamicJsonDocument* statsPointer = getJSONData(statsPath);
  if (!statsPointer) {
    Serial.print("No stats returned! Trying again in 15 seconds");
    delay(15000);
    return;
  }
  DynamicJsonDocument stats = *statsPointer;

  Serial.println(stats.as<String>());

  int count = stats["count"];
  long maxTime = stats["maxTime"];
  long minTime = stats["minTime"];
  double maxVbat = stats["maxVbat"];
  double minVbat = stats["minVbat"]; 
  double maxTemp = stats["maxTemp"];
  double minTemp = stats["minTemp"];

  Serial.println(maxTemp);
  Serial.println(stats["maxTemp"].as<String>());

  clearScreen();

  String message = "LAKE MAX: " + String(maxTemp, 2) + ", MIN: " + String(minTemp, 2) + " C" ;
  const char* messageChars = message.c_str();
  displayCenteredString(messageChars, -1, Font5x8, CRGB::Red, CRGB::Black);
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

  int number = random(0, 7);

  if (number ==  0)  {
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
  }
  if (number == 1)  {
    currentPalette = RainbowStripeColors_p;
    currentBlending = NOBLEND;
  }
  if (number == 2)  {
    currentPalette = RainbowStripeColors_p;
    currentBlending = LINEARBLEND;
  }
  if (number == 3)  {
    SetupPurpleAndGreenPalette();
    currentBlending = LINEARBLEND;
  }
  if (number == 4)  {
    SetupTotallyRandomPalette();
    currentBlending = LINEARBLEND;
  }
  if (number == 5)  {
    currentPalette = CloudColors_p;
    currentBlending = LINEARBLEND;
  }
  if (number == 6)  {
    currentPalette = PartyColors_p;
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

  for (int second = 0; second < displaySeconds; second++) {
    handleClientRequest();
    for (int i = 0; i < updatesPerSecond; i++) {
      fillLEDsFromPaletteColors(i * 3);
      FastLED.delay(1000 / updatesPerSecond);
    }
  }
}

int flagWidth = 12;
void drawEnglishFlag(int xPos) {
  if (DEBUG) {
    Serial.print("Drawing British flag at ");
    Serial.println(xPos);
  }
  for (int x = 0; x < flagWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      if (x == flagWidth/2 || x == flagWidth / 2 - 1) {
        setFontPixel(y, x + xPos, CRGB::Red);
      } else if(y == kMatrixHeight/2 || y == kMatrixHeight / 2 - 1) {
        setFontPixel(y, x + xPos, CRGB::Red);
      } else {
        setFontPixel(y, x + xPos, CRGB::White);
      }
    }
  }
}

void drawWelshFlag(int xPos) {
  if (DEBUG) {
    Serial.print("Drawing WelshFlag flag at ");
    Serial.println(xPos);
  }
  for (int x = 0; x < flagWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      if ((x == 5 && y == 1) || (x == 6 && y == 1) || (x == 8 && y == 2) || (x == 7 && y == 3) ||  (x == 3 && y == 4) || (x == 7 && y == 4)) {
        setFontPixel(y, x + xPos, CRGB::DarkRed);
      } else if ((x == 5 && y == 1) || (x == 7 && y == 1) || (x == 5 && y == 2) || (x == 6 && y == 2) || (x == 4 && y == 3) || (x == 5 && y == 3) || (x == 6 && y == 3)) {
        setFontPixel(y, x + xPos, CRGB::Red);
      } else if (y < kMatrixHeight/2) {
        setFontPixel(y, x + xPos, CRGB::White);
      } else {
        setFontPixel(y, x + xPos, CRGB::Green);
      }
    }
  }
}

void displayBritishFlags(int startPos = 0) {
  clearScreen();

  int flagNo = 0;
  for (int xPos = startPos; xPos < kMatrixWidth; xPos += (flagWidth + 2)) {
    if (flagNo % 2) {
      drawEnglishFlag(xPos);
    } else {
      drawWelshFlag(xPos);
    }

    flagNo++;
  }

  FastLED.show();
}

void scrollBritishFlags(int displaySeconds) {
  int updatesPerSecond = 50;

  int timePerLoop = ((float) 1000 / updatesPerSecond) * (flagWidth + 2);
  int numberOfLoopsToDisplay = (displaySeconds * 1000) / timePerLoop;
  Serial.print("Displaying flags: ");
  Serial.println(numberOfLoopsToDisplay);
  for (int i = 0; i < numberOfLoopsToDisplay; i++) {
    handleClientRequest();
    for (int xPos = -(flagWidth + 2) * 2; xPos++; xPos <= 0) {
      displayBritishFlags(xPos);
      FastLED.delay(1000 / updatesPerSecond);
    }
  }
}

// ------ MAIN
void setup()
{
  Serial.begin(115200);
  delay(100);

  connectToWifiWithStatusUpdates();

  setUpOTAUpdates();

  setUpLEDS();
  flashMatrixRGB();
}

int currentScreen = 0;
int totalScreens = 5;

unsigned long nextScreenAt = 0;

void loop() {
  handleClientRequest();

  if (!connectedToWifi()) {
    displayCenteredStringFor("Wifi Disconnected...", -1, Font5x8, CRGB::White, CRGB::Black, 500);
    connectToWifiWithStatusUpdates();
  }

  if (millis() >= nextScreenAt) {
    Serial.println("Going to next screen");
    if (currentScreen % totalScreens == 0) {
      // British flags for 7.5s
      scrollBritishFlags(7);
      nextScreenAt = millis() + 7500;
    }

    else if(currentScreen % totalScreens == 1) {
      // Display lake temp for 15 seconds
      displayLakeTemperature(false);
      nextScreenAt = millis() + 15000;
    }

    else if(currentScreen % totalScreens == 2) {
      // Random colors for 5 seconds
      changePalette();
      clearScreen();
      scrollThroughColors(5);
      nextScreenAt = millis() + 5000;
    }
    
    else if(currentScreen % totalScreens == 3) {
      // Humidity for 7.5 seconds
      displayLakeTemperature(true);
      nextScreenAt = millis() + 7500;
    }

    else {
      // Max/min temp for 7.5 seconds
      displayLakeStats();
      nextScreenAt = millis() + 7500;
    }

    currentScreen++;
  }
}


// TODO DEAD LAKE TEMP.
// Graph
// Color based on min max
