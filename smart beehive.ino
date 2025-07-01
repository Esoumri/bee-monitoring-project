#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <LiquidCrystal.h>

// --- PINOUTS ---
// LCD Pins: RS=14, E=27, D4=26, D5=25, D6=33, D7=32
// DHT11 inside sensor: GPIO 21
// DHT22 outside sensor: GPIO 19
// LDR light sensor: GPIO 34 (analog input)
// IR break beam sensor: GPIO 18
// Relay for IR sensor power: GPIO 5
// LEDs:
//   RED LED: GPIO 4
//   GREEN LED: GPIO 2

LiquidCrystal lcd(14, 27, 26, 25, 33, 32);

// Wi-Fi credentials
const char* ssid = "khaled";
const char* password = "12345678";

const char* serverURL = "https://beemonitor.vercel.app/api";
const char* apiKey = "API_ALA2025_KEY";
const char* deviceID = "dre";
const int beehiveID = 21
;

String sensorUrl = String(serverURL) + "/sensor-data";
String beeFlowUrl = String(serverURL) + "/bee-flow";

// Pins
#define DHTPIN_INSIDE 21
#define DHTPIN_OUTSIDE 19
#define DHTTYPE_INSIDE DHT11
#define DHTTYPE_OUTSIDE DHT22



#define LDR_PIN 34
#define IR_SENSOR_PIN 18
#define RELAY_PIN 5

#define RED_LED_PIN 4
#define GREEN_LED_PIN 2

DHT insideDHT(DHTPIN_INSIDE, DHTTYPE_INSIDE);
DHT outsideDHT(DHTPIN_OUTSIDE, DHTTYPE_OUTSIDE);

volatile int beeCount = 0;
volatile unsigned long lastIRTrigger = 0;
const unsigned long debounceTime = 700;
unsigned long lastBeePostTime = 0;
const unsigned long beePostInterval = 30000;

int lightThreshold = 3500;
bool irActive = true;

unsigned long lastLCDToggle = 0;
bool showInside = true;
const unsigned long lcdToggleInterval = 10000; // 10 seconds toggle

unsigned long ledOnTime = 0;
const unsigned long ledDuration = 15000; // 15 seconds ON duration
bool ledsOn = false;

void IRAM_ATTR countBee() {
  unsigned long now = millis();
  if (now - lastIRTrigger > debounceTime && irActive) {
    beeCount++;
    lastIRTrigger = now;
  }
}

void setFlowLEDs(bool flowGood) {
  if (flowGood) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
  } else {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
  }
  ledsOn = true;
  ledOnTime = millis();
}

void setup() {
  Serial.begin(115200);
  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Beehive: Test");

  insideDHT.begin();
  outsideDHT.begin();

  pinMode(LDR_PIN, INPUT);
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), countBee, FALLING);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("System ready.");
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);
  irActive = (ldrValue <= lightThreshold);
  digitalWrite(RELAY_PIN, irActive ? HIGH : LOW);

  float temp_in = insideDHT.readTemperature();
  float hum_in = insideDHT.readHumidity();

  float temp_out = NAN;
  float hum_out = NAN;
  for (int i = 0; i < 3; i++) {
    temp_out = outsideDHT.readTemperature();
    hum_out = outsideDHT.readHumidity();
    if (!isnan(temp_out) && !isnan(hum_out)) break;
    delay(500);
  }

  Serial.println("======== SENSOR READINGS ========");
  Serial.print("LDR Value: "); Serial.print(ldrValue);
  Serial.println(irActive ? " → DAY ☀️" : " → NIGHT 🌙");

  if (!isnan(temp_in) && !isnan(hum_in)) {
    Serial.printf("Inside Temp: %.2f °C\nInside Humidity: %.2f %%\n", temp_in, hum_in);
  } else {
    Serial.println("Failed to read DHT11 (inside)");
  }

  if (!isnan(temp_out) && !isnan(hum_out)) {
    Serial.printf("Outside Temp: %.2f °C\nOutside Humidity: %.2f %%\n", temp_out, hum_out);
  } else {
    Serial.println("Failed to read DHT22 (outside)");
  }

  unsigned long now = millis();
  if (now - lastLCDToggle > lcdToggleInterval) {
    showInside = !showInside;
    lastLCDToggle = now;
  }

  lcd.setCursor(0, 0);
  lcd.print("Beehive: Test  ");
  lcd.setCursor(0, 1);
  if (showInside) {
    if (!isnan(temp_in) && !isnan(hum_in)) {
      lcd.print("In T:");
      lcd.print((int)temp_in);
      lcd.print((char)223);
      lcd.print("C H:");
      lcd.print((int)hum_in);
      lcd.print("%   ");
    } else {
      lcd.print("In sensor err   ");
    }
  } else {
    if (!isnan(temp_out) && !isnan(hum_out)) {
      lcd.print("Out T:");
      lcd.print((int)temp_out);
      lcd.print((char)223);
      lcd.print("C H:");
      lcd.print((int)hum_out);
      lcd.print("%   ");
    } else {
      lcd.print("Out sensor err  ");
    }
  }

  if (!isnan(temp_in) && !isnan(hum_in) && !isnan(temp_out) && !isnan(hum_out)) {
    StaticJsonDocument<256> doc;
    doc["beehive_id"] = beehiveID;
    doc["device_id"] = deviceID;
    doc["inside_temp"] = temp_in;
    doc["inside_humidity"] = hum_in;
    doc["outside_temp"] = temp_out;
    doc["outside_humidity"] = hum_out;
    doc["light_level"] = ldrValue;
    doc["system_active"] = irActive;

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(sensorUrl);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("x-api-key", apiKey);

      String jsonStr;
      serializeJson(doc, jsonStr);

      int httpCode = http.POST(jsonStr);
      Serial.println("Sensor Data Response: " + String(httpCode));
      http.end();
    }
  }

  if (millis() - lastBeePostTime > beePostInterval && irActive) {
    StaticJsonDocument<128> beeDoc;
    beeDoc["beehive_id"] = beehiveID;
    beeDoc["device_id"] = deviceID;
    beeDoc["bee_count"] = beeCount;

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(beeFlowUrl);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("x-api-key", apiKey);

      String beeJson;
      serializeJson(beeDoc, beeJson);

      int httpCode = http.POST(beeJson);
      Serial.println("Bee Count Response: " + String(httpCode));
      http.end();

      bool flowGood = (beeCount > 10);
      setFlowLEDs(flowGood);
    }

    Serial.printf("Bee count in the last 1 minute: %d\n", beeCount);
    beeCount = 0;
    lastBeePostTime = millis();
  }

  if (ledsOn && millis() - ledOnTime >= ledDuration) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    ledsOn = false;
  }

  delay(30000);
}


