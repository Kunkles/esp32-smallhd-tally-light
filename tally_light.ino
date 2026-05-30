#include <ETH.h>
#include <Network.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <Preferences.h>

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

// --- DEFAULT DEVICE IDENTITY ---
// Used only on first boot. After that, the name saved via the web portal takes over.
const char* DEFAULT_HOSTNAME = "tally-stage06b";

// --- WIFI CREDENTIALS ---
const char* WIFI_SSID = "YOUR_SSID";     /// Change this to your local network
const char* WIFI_PASS = "YOUR_PASSWORD"; /// if you arent hardlined

Preferences prefs;
String activeHostname;

WebServer server(80);
bool eth_connected = false;
bool wifi_connected = false;

// -----------------------------------------------------------------------

void NetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(activeHostname.c_str());
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

// -----------------------------------------------------------------------

String buildConfigPage(String message = "") {
  String ip     = eth_connected ? ETH.localIP().toString() : WiFi.localIP().toString();
  String iface  = eth_connected ? "Ethernet" : "WiFi";
  String tstate = digitalRead(TALLY_PIN) ? "ON" : "OFF";

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Tally Bridge Config</title>
  <style>
    body { font-family: sans-serif; background: #1a1a1a; color: #eee;
           max-width: 480px; margin: 40px auto; padding: 0 20px; }
    h1   { color: #ff4444; font-size: 1.4em; margin-bottom: 4px; }
    .sub { color: #888; font-size: 0.85em; margin-bottom: 24px; }
    .card { background: #2a2a2a; border-radius: 8px; padding: 20px; margin-bottom: 16px; }
    .card h2 { margin: 0 0 12px; font-size: 0.85em; color: #888;
               text-transform: uppercase; letter-spacing: 0.08em; }
    .stat { display: flex; justify-content: space-between; margin: 6px 0; font-size: 0.95em; }
    .stat span:last-child { color: #fff; font-weight: bold; }
    label { display: block; margin-bottom: 6px; color: #aaa; font-size: 0.9em; }
    input[type=text] { width: 100%; box-sizing: border-box; padding: 10px;
                       background: #333; border: 1px solid #444; border-radius: 6px;
                       color: #fff; font-size: 1em; }
    input[type=text]:focus { outline: none; border-color: #ff4444; }
    button { width: 100%; margin-top: 12px; padding: 12px; background: #ff4444;
             color: #fff; border: none; border-radius: 6px; font-size: 1em; cursor: pointer; }
    button:hover { background: #cc3333; }
    .msg  { background: #2a4a2a; border: 1px solid #4a7a4a; border-radius: 6px;
            padding: 10px 14px; margin-bottom: 16px; color: #88cc88; font-size: 0.9em; }
    .err  { background: #4a2a2a; border: 1px solid #7a4a4a; border-radius: 6px;
            padding: 10px 14px; margin-bottom: 16px; color: #cc8888; font-size: 0.9em; }
    .note { color: #666; font-size: 0.8em; margin-top: 8px; }
  </style>
</head>
<body>
  <h1>&#9679; Tally Bridge</h1>
  <div class="sub">SmallHD Tally Controller</div>
)rawliteral";

  if (message.length() > 0) {
    bool isErr = message.startsWith("&#10007;");
    html += "<div class=\"" + String(isErr ? "err" : "msg") + "\">" + message + "</div>";
  }

  html += "<div class=\"card\"><h2>Status</h2>";
  html += "<div class=\"stat\"><span>Device Name</span><span>" + activeHostname + "</span></div>";
  html += "<div class=\"stat\"><span>Interface</span><span>" + iface + "</span></div>";
  html += "<div class=\"stat\"><span>IP Address</span><span>" + ip + "</span></div>";
  html += "<div class=\"stat\"><span>Tally State</span><span>" + tstate + "</span></div>";
  html += "</div>";

  html += R"rawliteral(
  <div class="card">
    <h2>Configure</h2>
    <form method="POST" action="/config">
      <label>Device Hostname</label>
)rawliteral";

  html += "      <input type=\"text\" name=\"hostname\" value=\"" + activeHostname
        + "\" maxlength=\"32\" pattern=\"[a-zA-Z0-9\\-]+\" required>";
  html += R"rawliteral(
      <p class="note">Letters, numbers, and hyphens only. Device will reboot to apply.</p>
      <button type="submit">Save &amp; Reboot</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

  return html;
}

// -----------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  // Load hostname from flash — falls back to DEFAULT_HOSTNAME on first boot
  prefs.begin("tally", false);
  activeHostname = prefs.getString("hostname", DEFAULT_HOSTNAME);
  Serial.println("Hostname: " + activeHostname);

  pinMode(TALLY_PIN, OUTPUT);
  digitalWrite(TALLY_PIN, HIGH); // default HIGH — tally OFF

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Network.onEvent(NetworkEvent);

  ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDR,
            ETH_CS_PIN, ETH_INT_PIN, ETH_RST_PIN,
            SPI2_HOST, ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN);

  WiFi.setHostname(activeHostname.c_str());
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

  MDNS.begin(activeHostname.c_str());

  // --- Config portal ---
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", buildConfigPage());
  });

  server.on("/config", HTTP_GET, []() {
    server.send(200, "text/html", buildConfigPage());
  });

  server.on("/config", HTTP_POST, []() {
    if (server.hasArg("hostname") && server.arg("hostname").length() > 0) {
      String newName = server.arg("hostname");
      prefs.putString("hostname", newName);
      server.send(200, "text/html",
        buildConfigPage("&#10003; Saved as &ldquo;" + newName + "&rdquo; — rebooting now..."));
      delay(2000);
      ESP.restart();
    } else {
      server.send(400, "text/html",
        buildConfigPage("&#10007; Invalid hostname — please try again."));
    }
  });

  // --- Tally endpoints ---
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
    String ip    = eth_connected ? ETH.localIP().toString() : WiFi.localIP().toString();
    String iface = eth_connected ? "Ethernet" : "WiFi";
    String s     = digitalRead(TALLY_PIN) ? "ON" : "OFF";
    server.send(200, "text/plain",
      "Tally Bridge v5.2 | " + iface + " IP: " + ip
      + " | Tally: " + s + " | Host: " + activeHostname);
  });

  server.begin();

  String ip = eth_connected ? ETH.localIP().toString() : WiFi.localIP().toString();
  Serial.println("HTTP server running.");
  Serial.println("  CONFIG: http://" + activeHostname + ".local/");
  Serial.println("  ON:     http://" + activeHostname + ".local/tally/on");
  Serial.println("  OFF:    http://" + activeHostname + ".local/tally/off");
  Serial.println("  TEST:   http://" + activeHostname + ".local/tally/test");
  Serial.println("  STATUS: http://" + activeHostname + ".local/status");
  Serial.println("  IP:     http://" + ip);
}

void loop() {
  server.handleClient();

  if (digitalRead(BUTTON_PIN) == LOW) {
    triggerTest();
    while (digitalRead(BUTTON_PIN) == LOW); // wait for release
  }
}
