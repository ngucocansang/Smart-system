// Chân kết nối relay
#define RELAY_PIN 2  // GPIO5 (D1 trên ESP8266 hoặc có thể là một chân khác trên ESP32)

void setup() {
  pinMode(RELAY_PIN, OUTPUT); // Đặt chân relay là OUTPUT
  Serial.begin(115200); // Bắt đầu Serial Monitor
}

void loop() {
  Serial.println("Bật relay");
  digitalWrite(RELAY_PIN, HIGH); // Bật relay
  delay(5000); // Chờ 2 giây
  
  Serial.println("Tắt relay");
  digitalWrite(RELAY_PIN, LOW); // Tắt relay
  delay(5000); // Chờ 2 giây
}
