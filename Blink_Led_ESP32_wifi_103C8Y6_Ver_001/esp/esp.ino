#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "beaglebone";
const char* password = "12345678910";

WebServer server(80);

/* ================= MAIN WEB PAGE ================= */
void rootPage() {
  server.send(200, "text/html",
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>STM32 LED Control</title>"
    "<script>"
    "function ledOn(){ fetch('/on'); }"
    "function ledOff(){ fetch('/off'); }"
    "</script>"
    "</head>"
    "<body>"
    "<h2>STM32 LED Control</h2>"
    "<button onclick='ledOn()'>LED ON</button><br><br>"
    "<button onclick='ledOff()'>LED OFF</button>"
    "</body>"
    "</html>"
  );
}

/* ================= LED ON ================= */
void ledOn() {
  Serial.println("[WEB] LED ON");
  Serial2.write('1');      // UART command
  server.send(204);        // No content, no page change
}

/* ================= LED OFF ================= */
void ledOff() {
  Serial.println("[WEB] LED OFF");
  Serial2.write('0');      // UART command
  server.send(204);        // No content, no page change
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", rootPage);
  server.on("/on", ledOn);
  server.on("/off", ledOff);
  server.begin();
}

/* ================= LOOP ================= */
void loop() {
  server.handleClient();
}
