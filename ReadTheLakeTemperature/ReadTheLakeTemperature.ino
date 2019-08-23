#include <ArduinoJson.h>
#include <WiFi.h>


const char* ssid     = "Camp2019-things";
const char* password = "camp2019";

const char* host = "marekventur.com";
const char* path = "/metrics/";
const int port = 8973;

void setup()
{
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

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

int value = 0;

void loop()
{
    delay(5000);
    ++value;

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    if (!client.connect(host, port)) {
        Serial.println("connection failed");
        return;
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
            return;
        }
    }

    
    String responseString = "";
    while(client.available()) {
        responseString = client.readStringUntil('\r');
    }

    DynamicJsonDocument doc(300);
    DeserializationError error = deserializeJson(doc, responseString);
  
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      Serial.print(responseString);
      return;
    }

    /*
    rh: 33.17,
    rssi: -81,
    t: 32.34,
    status_message: "STALE",
    status: 1,
    updated: 1566569145722
    */
    double temperature = doc["t"];
    double relative_humidity = doc["rh"];

    Serial.print("temperature: ");
    Serial.println(temperature);

    Serial.print("humidity: ");
    Serial.println(relative_humidity);
}
