#include <ETH.h>
#include <Network.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

// --- TALLY PIN ---
const int TALLY_PIN = 18;

// --- BUTTON PIN ---
#define BUTTON_PIN 3

// --- W5500 SPI ETHERNET PINS ---
#define ETH_MOSI_PIN   11
#define ETH_MISO_PIN   12
#define ETH_SCK_PIN    13
#define ETH_CS_PIN     14
#define ETH_INT_PIN    10
#define ETH_RST_PIN     9
#define ETH_PHY_ADDR    1

// --- DEVICE IDENTITY ---
const char* HOSTNAME = "tally-stage06b";  /// Change this per device access device with: 
                                          /// http://tally-stage06b/status
                                          ///  http://tally-stage06b/tally/on
                                          ///  http://tally-stage06b/tally/off
// --- WIFI CREDENTIALS ---
const char* WIFI_SSID = "YOUR_SSID"; /// Change this to your local network
const char* WIFI_PASS = "YOUR_PASSWORD"; /// if you arent hardlined

WebServer server(80);
bool eth_connected = false;
bool wifi_connected = false;

void NetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(HOSTNAME);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet cable connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("Ethernet IP: ");
      Serial.println(ETH.localIP().toString());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi IP: ");
      Serial.println(WiFi.localIP().toString());
      wifi_connected = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      wifi_connected = false;
      break;
    default:
      break;
  }
}

void triggerTest() {
  digitalWrite(TALLY_PIN, HIGH);
  delay(1000);
  digitalWrite(TALLY_PIN, LOW);
  Serial.println("TALLY TEST");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TALLY_PIN, OUTPUT);
  digitalWrite(TALLY_PIN, HIGH); // default HIGH — tally OFF

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Network.onEvent(NetworkEvent);

  ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDR,
            ETH_CS_PIN, ETH_INT_PIN, ETH_RST_PIN,
            SPI2_HOST, ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN);

  WiFi.setHostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("Waiting for network...");

  unsigned long start = millis();
  while (!eth_connected && !wifi_connected) {
    delay(100);
    if (millis() - start > 15000) {
      Serial.println("Network timeout — check connections");
      break;
    }
  }

  if (eth_connected) Serial.println("Connected via Ethernet.");
  if (wifi_connected) Serial.println("Connected via WiFi.");

  MDNS.begin(HOSTNAME);

  server.on("/tally/on", HTTP_GET, []() {
    digitalWrite(TALLY_PIN, HIGH);
    server.send(200, "text/plain", "Tally ON");
    Serial.println("TALLY ON");
  });

  server.on("/tally/off", HTTP_GET, []() {
    digitalWrite(TALLY_PIN, LOW);
    server.send(200, "text/plain", "Tally OFF");
    Serial.println("TALLY OFF");
  });

  server.on("/tally/test", HTTP_GET, []() {
    server.send(200, "text/plain", "Tally TEST");
    triggerTest();
  });

  server.on("/status", HTTP_GET, []() {
    String ip = eth_connected ? ETH.localIP().toString() : WiFi.localIP().toString();
    String iface = eth_connected ? "Ethernet" : "WiFi";
    String s = digitalRead(TALLY_PIN) ? "ON" : "OFF";
    server.send(200, "text/plain",
      "Tally Bridge v5.1 | " + iface + " IP: " + ip
      + " | Tally: " + s);
  });

  server.begin();

  String ip = eth_connected ? ETH.localIP().toString() : WiFi.localIP().toString();
  Serial.println("HTTP server running.");
  Serial.println("  ON:     http://" + String(HOSTNAME) + ".local/tally/on");
  Serial.println("  OFF:    http://" + String(HOSTNAME) + ".local/tally/off");
  Serial.println("  TEST:   http://" + String(HOSTNAME) + ".local/tally/test");
  Serial.println("  STATUS: http://" + String(HOSTNAME) + ".local/status");
  Serial.println("  IP:     http://" + ip);
}

void loop() {
  server.handleClient();

  if (digitalRead(BUTTON_PIN) == LOW) {
    triggerTest();
    while (digitalRead(BUTTON_PIN) == LOW); // wait for release
  }
}