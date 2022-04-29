#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPSRedirect.h>

struct Button {
    const uint8_t PIN;
    bool detected;
    unsigned long detectedAt;
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

const char* sheet = "StartData";

// Gscript ID and required credentials
const char *GScriptId = "AKfycbwTftthY7XpBuDkBwgv_A49HahBlJgsGIk_GGKaug3dnWjZYpXrOk0FSieGG1lo8865";    // change Gscript ID
const int httpsPort =  443;
const char* host = "script.google.com";
// const char* googleRedirHost = "script.googleusercontent.com";
String url = String("/macros/s/") + GScriptId + "/exec?";
HTTPSRedirect* client = nullptr;

void ARDUINO_ISR_ATTR isr() {
    laserDetector.detected = true;
    laserDetector.detectedAt = millis();
}

void startWifi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // flush() is needed to print the above (connecting...) message reliably, 
  // in case the wireless connection doesn't go through
  Serial.flush();
  
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
 
  Serial.print(String("Connecting to "));
  Serial.println(host);

   // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  bool WiFiFlag = false;
  for (int i=0; i<5; i++){
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
       WiFiFlag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  // Connection Status, 1 = Connected, 0 is not.
  Serial.println("Connection Status: " + String(client->connected()));
  Serial.flush();
  
  if (!WiFiFlag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    Serial.flush();
    return;
  }
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

  startWifi();
    
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
    smartDelay(500);
    laserDetector.detected = false;
    laserDetector.detectedAt = 0;
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
  Serial.println(laserDetector.detectedAt - lastMillis);

   if (client != nullptr){
    if (!client->connected()){
      Serial.println("Connecting to client again..."); 
      client->connect(host, httpsPort);
    }
  }
  String urlFinal = url + "tag=" + sheet + "&year=" + String(yearNow);
  urlFinal += "&month=" + String(monthNow);
  urlFinal += "&date=" + String(dayNow);
  urlFinal += "&hours=" + String(hourNow);
  urlFinal += "&minutes=" + String(minuteNow);
  urlFinal += "&seconds=" + String(secondNow);
  urlFinal += "&ms=" + String(laserDetector.detectedAt - lastMillis);
  client->GET(url, host);
  //client.printRedir(urlFinal, host, googleRedirHost);

  Serial.println();
  Serial.println("data sent");
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
