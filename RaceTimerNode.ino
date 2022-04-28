#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <WiFi.h>

struct Button {
    const uint8_t PIN;
    bool detected;
};


Button laserDetector = {16, false};

int secondNow = 0;
int hourNow = 0;
int minuteNow = 0;
int dayNow = 0;
int monthNow = 0;
int yearNow = 0;

unsigned long lastMillis = 0;

// 9600-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
static const int RXPin = 13, TXPin = 12, StatusLEDRedPin = 15, StatusLEDGreenPin = 13, StatusLEDBluePin = 2;
static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

const char* ssid     = "4 The Grove";
const char* password = "trebilcock";
const char* host = "Google Sheets";

const char* sheet = "start";

void ARDUINO_ISR_ATTR isr() {
    laserDetector.detected = true;
}

void setup()
{
  pinMode(laserDetector.PIN, INPUT);
  pinMode(StatusLEDRedPin, OUTPUT);
  pinMode(StatusLEDGreenPin, OUTPUT);
  pinMode(StatusLEDBluePin, OUTPUT);

  Serial.begin(115200);
  ss.begin(GPSBaud);
  Serial.println("Connecting to GPS");
  smartDelay(6000);

  while (!gps.date.isValid() or !gps.time.isValid()) {
    LEDBlue();
    smartDelay(500);
    LEDOff();
    smartDelay(500);
    Serial.print(".");
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.println("");
      Serial.println(F("No GPS data received: check wiring"));
      blockingFault();
    }
  }

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    LEDRed();
    smartDelay(500);
    LEDOff();
    smartDelay(500);
    Serial.print(".");
    if (millis() > 15000 && WiFi.status() != WL_CONNECTED) {
      Serial.println("");
      Serial.println(F("Could not connect to Wifi"));
      blockingFault();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
    
  attachInterrupt(laserDetector.PIN, isr, RISING);
  updateTime();
}

void loop()
{
  LEDGreen();
  updateTime();
  if (laserDetector.detected)
  {
    LEDWhite();

    sendDetection();
    laserDetector.detected = false;
  }
  smartDelay(1);
}

void sendDetection()
{
  Serial.print("connecting to ");
  Serial.println(host);

  Serial.print(sheet);
  Serial.print(" ");
  Serial.print(yearNow);
  Serial.print("-");
  Serial.print(monthNow);
  Serial.print("-");
  Serial.print(dayNow);
  Serial.print(" ");
  Serial.print(hourNow);
  Serial.print(":");
  Serial.print(minuteNow);
  Serial.print(":");
  Serial.print(secondNow);
  Serial.print(":");
  Serial.println(millis() - lastMillis);
  // Use WiFiClient class to create TCP connections
//  WiFiClient client;
//  const int httpPort = 80;
//  if (!client.connect(host, httpPort)) {
//      Serial.println("connection failed");
//      return;
//  }

//  // We now create a URI for the request
//  String url = "/input/";
//  url += streamId;
//  url += "?private_key=";
//  url += privateKey;
//  url += "&value=";
//  url += value;
//
//  Serial.print("Requesting URL: ");
//  Serial.println(url);
//
//  // This will send the request to the server
//  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//               "Host: " + host + "\r\n" +
//               "Connection: close\r\n\r\n");
//  unsigned long timeout = millis();
//  while (client.available() == 0) {
//      if (millis() - timeout > 5000) {
//          Serial.println(">>> Client Timeout !");
//          client.stop();
//          return;
//      }
//  }
//
//  // Read all the lines of the reply from server and print them to Serial
//  while(client.available()) {
//      String line = client.readStringUntil('\r');
//      Serial.print(line);
//  }

  Serial.println();
  Serial.println("closing connection");
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

void updateTime() {
  if (secondNow != gps.time.second())
  {
    hourNow = gps.time.hour();
    minuteNow = gps.time.minute();
    secondNow = gps.time.second();

    dayNow = gps.date.day();
    monthNow = gps.date.month();
    yearNow = gps.date.year();
    lastMillis = millis();
  }
}

void blockingFault() {
  while (1) {
    LEDRed();
    delay(500);
    LEDOff();
    delay(500);
  }
}

void LEDBlue() {
  digitalWrite(StatusLEDRedPin, LOW);
  digitalWrite(StatusLEDGreenPin, LOW);
  digitalWrite(StatusLEDBluePin, HIGH);
}

void LEDRed() {
  digitalWrite(StatusLEDRedPin, HIGH);
  digitalWrite(StatusLEDGreenPin, LOW);
  digitalWrite(StatusLEDBluePin, LOW);
}

void LEDGreen() {
  digitalWrite(StatusLEDRedPin, LOW);
  digitalWrite(StatusLEDGreenPin, HIGH);
  digitalWrite(StatusLEDBluePin, LOW);
}

void LEDWhite() {
  digitalWrite(StatusLEDRedPin, HIGH);
  digitalWrite(StatusLEDGreenPin, HIGH);
  digitalWrite(StatusLEDBluePin, HIGH);
}

void LEDOff() {
  digitalWrite(StatusLEDRedPin, LOW);
  digitalWrite(StatusLEDGreenPin, LOW);
  digitalWrite(StatusLEDBluePin, LOW);
}
