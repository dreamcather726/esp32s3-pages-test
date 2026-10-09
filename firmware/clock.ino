#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
Servo hourServo, minuteServo, secondServo;
const int hourPin = 13, minutePin = 12, secondPin = 14;
WiFiUDP ntpUDP;
// The browser's virtual UDP/NTP service answers this hostname locally.
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000);
unsigned long lastUpdate = 0;
bool ready = false;
bool synchronized = false;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 servo clock: real firmware booting");
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("WIFI EVENT %d\n", static_cast<int>(event));
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) Serial.printf("WIFI disconnect reason %d\n", info.wifi_sta_disconnected.reason);
  });
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setMinSecurity(WIFI_AUTH_OPEN);
  WiFi.begin(ssid, password);
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi timeout. Check emulator SSID settings and restart.");
    return;
  }
  Serial.print("\nWiFi connected: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  hourServo.setPeriodHertz(50);
  minuteServo.setPeriodHertz(50);
  secondServo.setPeriodHertz(50);
  hourServo.attach(hourPin, 544, 2400);
  minuteServo.attach(minutePin, 544, 2400);
  secondServo.attach(secondPin, 544, 2400);
  hourServo.write(90);
  minuteServo.write(90);
  secondServo.write(90);
  ready = true;
  Serial.println("Servos ready: GPIO13 / GPIO12 / GPIO14. Waiting for NTP.");
}

void loop() {
  if (!ready) { delay(100); return; }
  if (millis() - lastUpdate < 1000) { delay(10); return; }
  lastUpdate = millis();
  if (timeClient.update()) synchronized = true;
  if (!synchronized) {
    Serial.println("Waiting for virtual NTP reply...");
    return;
  }
  int hours = timeClient.getHours() % 12;
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
  int hourAngle = (hours * 60 + minutes) * 180 / 720;
  int minuteAngle = (minutes * 60 + seconds) * 180 / 3600;
  int secondAngle = seconds * 180 / 60;
  hourServo.write(hourAngle);
  minuteServo.write(minuteAngle);
  secondServo.write(secondAngle);
  Serial.printf("TIME %02d:%02d:%02d | SERVO %d %d %d\n",
    timeClient.getHours(), minutes, seconds, hourAngle, minuteAngle, secondAngle);
}
