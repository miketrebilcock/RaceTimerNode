#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

struct Button
{
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

static const int RXPin = 13, TXPin = 12, StatusLEDRedPin = 15, StatusLEDGreenPin = 13, StatusLEDBluePin = 2;
static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

const char *ssid = "4 The Grove";
const char *password = "trebilcock";

const char *sheet = "StartData";

// Gscript ID and required credentials
const char *GScriptId = "AKfycbwTftthY7XpBuDkBwgv_A49HahBlJgsGIk_GGKaug3dnWjZYpXrOk0FSieGG1lo8865"; // change Gscript ID
String url = String("https://script.google.com/macros/s/") + GScriptId + "/exec?";

void ARDUINO_ISR_ATTR isr()
{
  laserDetector.detected = true;
  laserDetector.detectedAt = millis();
}

#define WIFI_TIMEOUT_MS 20000      // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt

/**
 * Task: monitor the WiFi connection and keep it alive!
 *
 * When a WiFi connection is established, this task will check it every 10 seconds
 * to make sure it's still alive.
 *
 * If not, a reconnect is attempted. If this fails to finish within the timeout,
 * the ESP32 will wait for it to recover and try again.
 */
void keepWiFiAlive(void *parameter)
{
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println("[WIFI] Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    // Keep looping while we're not connected and haven't reached the timeout
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < WIFI_TIMEOUT_MS)
    {
    }

    // When we couldn't make a WiFi connection (or the timeout expired)
    // sleep for a while and then retry.
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("[WIFI] FAILED");
      LEDRed();
      vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println("[WIFI] Connected: " + WiFi.localIP());
    LEDGreen();
  }
}

void setup()
{
  pinMode(laserDetector.PIN, INPUT);
  pinMode(StatusLEDRedPin, OUTPUT);
  pinMode(StatusLEDGreenPin, OUTPUT);
  pinMode(StatusLEDBluePin, OUTPUT);
  pinMode(4, OUTPUT);

  Serial.begin(115200);

  xTaskCreatePinnedToCore(
      keepWiFiAlive,
      "keepWiFiAlive", // Task name
      5000,            // Stack size (bytes)
      NULL,            // Parameter
      1,               // Task priority
      NULL,            // Task handle
      ARDUINO_RUNNING_CORE);

  ss.begin(GPSBaud);
  Serial.println("Connecting to GPS");
  smartDelay(6000);

  while (!gps.date.isValid() or !gps.time.isValid())
  {
    LEDBlue();
    smartDelay(500);
    LEDOff();
    smartDelay(500);
    Serial.print(".");
    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
      Serial.println("");
      Serial.println(F("No GPS data received: check wiring"));
      blockingFault();
    }
  }

  attachInterrupt(laserDetector.PIN, isr, RISING);
  updateTime();
}

void loop()
{
  LEDGreen();
  updateTime();
  if (laserDetector.detected)
  {
    digitalWrite(4, HIGH);

    sendDetection();
    smartDelay(500);
    laserDetector.detected = false;
    laserDetector.detectedAt = 0;
    digitalWrite(4, LOW);
  }
  smartDelay(1);
}

void sendDetection()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    ;
  }
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

  String urlFinal = url + "tag=" + sheet + "&year=" + String(yearNow);
  urlFinal += "&month=" + String(monthNow);
  urlFinal += "&date=" + String(dayNow);
  urlFinal += "&hours=" + String(hourNow);
  urlFinal += "&minutes=" + String(minuteNow);
  urlFinal += "&seconds=" + String(secondNow);
  urlFinal += "&ms=" + String(laserDetector.detectedAt - lastMillis);
  Serial.println(urlFinal);
  HTTPClient http;
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);
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

void updateTime()
{
  if ((gps.date.isValid() and gps.time.isValid()))
  {
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
}

void blockingFault()
{
  Serial.println("Fault");
  while (1)
  {
    LEDRed();
    delay(500);
    LEDOff();
    delay(500);
  }
}

void LEDBlue()
{
  digitalWrite(StatusLEDRedPin, LOW);
  digitalWrite(StatusLEDGreenPin, LOW);
  digitalWrite(StatusLEDBluePin, HIGH);
}

void LEDRed()
{
  digitalWrite(StatusLEDRedPin, HIGH);
  digitalWrite(StatusLEDGreenPin, LOW);
  digitalWrite(StatusLEDBluePin, LOW);
}

void LEDGreen()
{
  digitalWrite(StatusLEDRedPin, LOW);
  digitalWrite(StatusLEDGreenPin, HIGH);
  digitalWrite(StatusLEDBluePin, LOW);
}

void LEDWhite()
{
  digitalWrite(StatusLEDRedPin, HIGH);
  digitalWrite(StatusLEDGreenPin, HIGH);
  digitalWrite(StatusLEDBluePin, HIGH);
}

void LEDOff()
{
  digitalWrite(StatusLEDRedPin, LOW);
  digitalWrite(StatusLEDGreenPin, LOW);
  digitalWrite(StatusLEDBluePin, LOW);
}
