#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFiClientSecureBearSSL.h>

// RUBAH DISINI

String deviceID = "2";

// RUBAH DISINI

#define SS_PIN 4  //D2
#define RST_PIN 2 //D4
const byte buzer = 5;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
MFRC522 mfrc522(SS_PIN, RST_PIN);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
WiFiClientSecure client;

byte readcard[4];
char str[32] = "";
String StrUID;
int counterDissconnect = 0;
String urlApi = "https://caltechsmartfarm.com/deviceScanner?id=";
void setup() {
  pinMode(buzer, OUTPUT);
  Serial.begin(115200);
  delay(1000);

  SPI.begin();
  mfrc522.PCD_Init();
  client.setInsecure();
  urlApi += deviceID;
  urlApi += "&UIDresult=";


  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Caltymart", "calty2021");
  WiFiMulti.addAP("Zexceed", "insyaallahrezeki");
  digitalWrite(buzer, HIGH);
}

void loop() {
  if ((WiFiMulti.run() != WL_CONNECTED)) {
    digitalWrite(buzer, HIGH);
    Serial.println("Wifi not connected");
    counterDissconnect++;
    if (counterDissconnect > 10) {
      Serial.println("Failed connect wifi more than 10 times, restart.");
      delay(500);
      ESP.restart();
    }
  } else {
    digitalWrite(buzer, LOW);
    if (getid()) {
      Serial.print("RFID found: ");
      Serial.println(StrUID);

      for (int x = 0; x < 2; x++) {
        digitalWrite(buzer, HIGH);
        delay(300);
        digitalWrite(buzer, LOW);
        delay(300);
      }

      if (webRequestSecure(StrUID).indexOf("success") > -1) {
        Serial.println("Send to server success");
        for (int x = 0; x < 3; x++) {
          digitalWrite(buzer, HIGH);
          delay(1000);
          digitalWrite(buzer, LOW);
          delay(500);
        }
      } else {
        Serial.println("fail send to server");
        for (int x = 0; x < 4; x++) {
          digitalWrite(buzer, HIGH);
          delay(2000);
          digitalWrite(buzer, LOW);
          delay(500);
        }
      }
      delay(3000);
    }
  }
}

String webRequestSecure(String data) {
  String ret = "";
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.print("[HTTPS] begin...\n");
    if (http.begin(client, urlApi + data)) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
        // file found at server
        if (httpCode == HTTP_CODE_OK) {
          ret = http.getString();
          Serial.println(ret);
        } else {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
  return ret;
}

boolean getid() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  for (int i = 0; i < 4; i++) {
    readcard[i] = mfrc522.uid.uidByte[i];
    konversi_ke_str(readcard, 4, str);
    StrUID = str;
  }
  mfrc522.PICC_HaltA();
  return 1;
}

void konversi_ke_str(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  digitalWrite(buzer, LOW);
  Serial.println("Connected to Wi-Fi sucessfully.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi, trying to connect...");
  digitalWrite(buzer, HIGH);
}
