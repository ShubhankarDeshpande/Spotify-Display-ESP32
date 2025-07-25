#include <Arduino.h>
#include <WiFi.h>
#include <TJpg_Decoder.h>
#include <SpotifyEsp32.h>
#include <HTTPClient.h>
#include "secrets.h" //make a secret file to put your ssid, pwd, and cloudflare worker
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <FS.h>
#include "LittleFS.h"


#if defined(ARDUINO_FEATHER_ESP32)  // Feather Huzzah32
#define TFT_CS 14
#define TFT_RST 15
#define TFT_DC 32

#elif defined(ESP8266)
#define TFT_CS 14
#define TFT_RST 32
#define TFT_DC 25

#else
// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
#define TFT_CS 14
#define TFT_RST 32  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 25
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const char* apiEndpoint = WORKER;



//AsyncWebServer server(80);
SPIClass spi = SPIClass(VSPI);
Adafruit_ST7735 tft = Adafruit_ST7735(&spi, TFT_CS, TFT_DC, TFT_RST);

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;  
  tft.drawRGBBitmap(x, y, bitmap, w, h);
  return 1;
}

void showAlbumArt(const char* url) {
  Serial.println(String("Downloading ") + url);
  uint16_t jpgWidth, jpgHeight;

  WiFiClientSecure client;
  client.setInsecure();  

  HTTPClient http;
  http.begin(client, url);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("art http error: %d\n", httpCode);
    http.end();
    return;
  }

  int len = http.getSize();  
  if (len <= 0 || len > 60000) {
    Serial.println("Wrong img size");
    len = 50000;
  }

  Serial.printf("img expected size: %d bytes\n", len);

  uint8_t* jpgBuffer = (uint8_t*)malloc(len);
  if (!jpgBuffer) {
    Serial.println("Not enough memory");
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  int bytesRead = 0;

  unsigned long startTime = millis();
  while (http.connected() && (bytesRead < len)) {
    int available = stream->available();
    if (available) {
      int r = stream->readBytes(jpgBuffer + bytesRead, available);
      bytesRead += r;
    }

    if (millis() - startTime > 8000) {
      Serial.println("Timeout");
      break;
    }

    delay(1);
  }

  if (bytesRead > 0) {
    TJpgDec.drawJpg(25, 20, jpgBuffer, bytesRead);

    
  } else {
    Serial.println("Failed data reading");
  }

  free(jpgBuffer);
  http.end();
}

String LastSongDraw = "Blank";
void scrollSong(String text) {
  static int cursor = 25;                   
  static unsigned long lastUpdate = 0;       
  
  if (text.length() * 6 <= 60) {
    if(LastSongDraw != text){
      tft.fillRect(0, 100, 128, 8, ST77XX_BLACK);
      tft.setCursor(25, 100);
      tft.print(text);
      LastSongDraw = text;
    }
    return;

  }

  if (millis() - lastUpdate > 50) {
    lastUpdate = millis();

    tft.fillRect(0, 100, 128, 8, ST77XX_BLACK);

    tft.setCursor(cursor, 100);
    tft.print(text);

    cursor-= 1;

    int textWidth = text.length() * 6;

    if (cursor < -textWidth) {
      tft.fillRect(0, 100, 128, 8, ST77XX_BLACK);
      cursor = 128;  
    }

  }
}


void setup(void) {


  Serial.begin(9600);
  spi.begin();
  Serial.print(WIFI_SSID);
  Serial.print(WIFI_PASSWORD);
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  WiFi.begin(ssid, password);

  int dotCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    tft.print(".");
    Serial.print(".");

    dotCount++;
    if (dotCount > 20) {
      tft.println();
      dotCount = 0;
    }
  }
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.print("Connected!");
  tft.fillScreen(ST77XX_BLACK);

  TJpgDec.setJpgScale(4);
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(tft_output);
}

String lastArt = "Blank";
String line = "Blank";
String LastSong = "Blank";
unsigned long lastApiUpdate = 0;
const unsigned long API_REFRESH_INTERVAL = 5000;

void loop() {
scrollSong(line); 

if(millis() - lastApiUpdate >= API_REFRESH_INTERVAL){
  lastApiUpdate = millis();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiEndpoint);

    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();


      const size_t capacity = 10 * 1024; 
      DynamicJsonDocument doc(capacity);

      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        //song and album art
        const char* song = doc["item"]["name"] | "Unknown Song";
        const char* albumArtUrl = doc["item"]["album"]["images"][1]["url"];

        //artist name and set to unknown artist
        const char* artist = "Unknown Artist";
        if (doc["item"]["artists"].size() > 0) {
          artist = doc["item"]["artists"][0]["name"] | "Unknown Artist";
        }


        bool isPlaying = doc["is_playing"] | false;

        line = String(song);
        String line2 = String(artist);
        if(line != LastSong){
          tft.setTextColor(ST77XX_WHITE);
          Serial.print(line);
          Serial.println(line2);
          Serial.print(String(albumArtUrl));
          tft.fillRect(24, 99, 128, 25, ST77XX_BLACK);
          tft.setCursor(25, 100);
          tft.setTextSize(1);
          tft.setTextColor(0x8410);
          tft.setCursor(25, 110);
          tft.print(line2);
          if(String(albumArtUrl) != lastArt){
            showAlbumArt(albumArtUrl);
            lastArt = albumArtUrl;
          }
          LastSong = line;
        }



        

      } else {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.printf("HTTP GET failed, error: %d\n", httpCode);
      delay(1000);
      tft.fillScreen(ST77XX_BLACK);
 
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

}
}
