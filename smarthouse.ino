#define BLYNK_TEMPLATE_ID "TMPL60-3a2i9h"
#define BLYNK_TEMPLATE_NAME "ngucoc"
#define BLYNK_AUTH_TOKEN "fLxX8kSh2iAZTHVSyMXf8SLgYsIH_m28"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

// ✅ Define hardware connections
#define RFID_SS_PIN 5     // RFID module SS/SDA pin
#define RFID_RST_PIN 4    // RFID reset pin
#define SERVO_PIN 17      // Servo motor for automatic door
#define LED1_PIN 16       // First LED for lighting
#define LED2_PIN 22       // Second LED
#define FAN_PIN 15        // Fan control (PWM via ledcWrite)
#define LDR_PIN 34        // Light sensor (analog)
#define DHT_PIN 21        // Temperature sensor
#define DHT_TYPE DHT11    // Change to DHT22 if using it
#define NUMPIXELS 8  

#define FAN_CHANNEL 3

// ✅ WiFi Credentials (Triple Check These)
char ssid[] = "Haha";  // Replace with your WiFi SSID
char pass[] = "1234567899";  // Replace with your WiFi password

DHT dht(DHT_PIN, DHT_TYPE);
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);
Servo doorServo;

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUMPIXELS, LED2_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUMPIXELS, LED1_PIN, NEO_GRB + NEO_KHZ800);

bool doorState = false;
bool ledState = false;
int fanSpeed = 0;

BlynkTimer timer;

// ✅ WiFi & Blynk Reconnect Handling
void reconnectBlynk() {
    if (!Blynk.connected()) {
        Serial.println("🔴 Blynk Disconnected! Reconnecting...");
        if (Blynk.connect()) {
            Serial.println("✅ Blynk Reconnected!");
        } else {
            Serial.println("⚠️ Blynk Reconnect Failed!");
        }
    }
}

// ✅ Setup Function
void setup() {
    Serial.begin(115200);
    SPI.begin();
    mfrc522.PCD_Init();
    dht.begin();
    doorServo.attach(SERVO_PIN);
    doorServo.write(0);  // Start with door closed

    strip1.begin();
    strip1.show();  // Initialize all pixels to 'off'
    strip2.begin();
    strip2.show();

    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);

    // ✅ Setup PWM for ESP32 Fan Control
    ledcSetup(FAN_CHANNEL, 5000, 8);  // Channel 0, 5kHz, 8-bit resolution
    ledcAttachPin(FAN_PIN, FAN_CHANNEL);

    // ✅ Start WiFi + Blynk Connection
    WiFi.begin(ssid, pass);
    Serial.println("📡 Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected!");

    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();  // ✅ Non-blocking connection

    timer.setInterval(5000L, updateSensors);  // Read sensor values every 2 seconds
    timer.setInterval(4000L, reconnectBlynk);  // ✅ Reconnect Blynk every 3 seconds

    Serial.println("🏠 Smart Home System Initialized");
}

// ✅ Main Loop
void loop() {
    if (Blynk.connected()) {
        Blynk.run();
    }
    timer.run();
    checkRFID();
    controlLED();
}

const String authorizedCard = " D9 25 13 05"; // Replace with your card UID

// 🔑 RFID Door Access
void checkRFID() {
   if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {  
        String cardID = "";
        
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            cardID += (mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            cardID += String(mfrc522.uid.uidByte[i], HEX);
        }
        
        cardID.toUpperCase();
        Serial.print("Card UID:");
        Serial.println(cardID);
        
        if (cardID == authorizedCard) {
            Serial.println("Access Granted");
            openDoor();
            delay(5000); // Delay 5 giây
            closeDoor(); // Tự động đóng cửa sau delay
        } else {
            Serial.println("Access Denied");
        }
        
        mfrc522.PICC_HaltA();         
    } 
}


void openDoor() {
    Serial.println("🚪 Opening the Door...");
    doorServo.write(90);
    delay(5000);  // Smooth transition
    doorState = true;
    Blynk.virtualWrite(V2, "Open");
}

void closeDoor() {
    Serial.println("🚪 Closing the Door...");
    doorServo.write(0);
    doorState = false;
    Blynk.virtualWrite(V2, "Closed");
}



// 🌡️ Update Sensors and Control Fan
void updateSensors() {
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    int lightLevel = analogRead(LDR_PIN);

    Serial.print("🌡️ Temperature: "); Serial.print(temp); Serial.println("°C");
    Serial.print("💧 Humidity: "); Serial.print(humidity); Serial.println("%");
    Serial.print("💡 Light Level: "); Serial.println(lightLevel);

    Blynk.virtualWrite(V0, temp);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V3, lightLevel);

    controlFan(temp);
}

// 🌀 Fan Control Using PWM
void controlFan(float temp) {
    if (temp > 24) {
        ledcWrite(FAN_CHANNEL, 255);  // Full speed
        fanSpeed = 100;
    } else if (temp > 15) {
        ledcWrite(FAN_CHANNEL, 127);  // Half speed
        fanSpeed = 50;
    } else {
        ledcWrite(FAN_CHANNEL, 0);  // Turn off
        fanSpeed = 0;
    }
    Serial.print("🌀 Fan Speed: ");
    Serial.print(fanSpeed);
    Serial.println("%");
    Blynk.virtualWrite(V3, fanSpeed);
}

// 💡 Automatic Neopixel Control
void controlLED() {
    int lightLevel = analogRead(LDR_PIN);
    if (lightLevel > 300) {  // ✅ Adjust threshold based on environment
        setNeopixelColor(strip1, 255, 255, 255);  // White light
        setNeopixelColor(strip2, 255, 255, 255);
        ledState = true;
        Blynk.virtualWrite(V4, "Open");
    } else {
        setNeopixelColor(strip1, 0, 0, 0);  // Turn off
        setNeopixelColor(strip2, 0, 0, 0);
        ledState = false;
        Blynk.virtualWrite(V4, "Close");
    }
    Serial.print("💡 LED Status: ");
    Serial.println(ledState ? "ON" : "OFF");
    Blynk.virtualWrite(V4, ledState);
}

void setNeopixelColor(Adafruit_NeoPixel &strip, int r, int g, int b) {
    for (int i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

BLYNK_WRITE(V2) {  // Virtual Pin V6 for door control via Blynk
  int doorCommand = param.asInt();  // Read button value (1 = Open, 0 = Close)
  
  if (doorCommand == 1) {
    openDoor();
  } else {
    closeDoor();
  }
}

BLYNK_WRITE(V7) {  // Fan Control
    int value = param.asInt();
    ledcWrite(0, value == 100 ? 255 : value == 50 ? 127 : 0);
    Serial.print("🌀 Fan Manually Set to: ");
    Serial.println(value);
}

// 🔄 Blynk Remote Controls
BLYNK_WRITE(V8) {  // Neopixel1 Control
    int value = param.asInt();
    setNeopixelColor(strip1, value == 1 ? 255 : 0, value == 1 ? 255 : 0, value == 1 ? 255 : 0);
}

BLYNK_WRITE(V9) {  // Neopixel2 Control
    int value = param.asInt();
    setNeopixelColor(strip2, value == 1 ? 255 : 0, value == 1 ? 255 : 0, value == 1 ? 255 : 0);
}