#include <ETH.h>
#include <Network.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <Preferences.h>

// --- DEFAULT TALLY PIN (can be overridden via web portal) ---
const int DEFAULT_TALLY_PIN = 18;

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

// --- FIRMWARE VERSION ---
const char* FW_VERSION = "v5.7";

// --- DEFAULTS (used only on first boot, overridden by web portal) ---
const char* DEFAULT_HOSTNAME = "tally-light";
const char* DEFAULT_WIFI_SSID = "YOUR_SSID";
const char* DEFAULT_WIFI_PASS = "YOUR_PASSWORD";

Preferences prefs;
String activeHostname;
String activeSSID;
String activePass;
String ipMode;          // "dhcp" or "static"
IPAddress staticIP, staticGW, staticSN, staticDNS;
int activePin;

WebServer server(80);
bool eth_connected = false;
bool wifi_connected = false;

// -----------------------------------------------------------------------

void NetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(activeHostname.c_str());
      if (ipMode == "static")
        ETH.config(staticIP, staticGW, staticSN, staticDNS);
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
  digitalWrite(activePin, HIGH);
  delay(1000);
  digitalWrite(activePin, LOW);
  Serial.println("TALLY TEST");
}

// -----------------------------------------------------------------------

String buildConfigPage(String message = "") {
  String ip     = eth_connected ? ETH.localIP().toString() : WiFi.localIP().toString();
  String iface  = eth_connected ? "Ethernet" : "WiFi";
  String tstate = digitalRead(activePin) ? "ON" : "OFF";
  bool   isStatic = (ipMode == "static");

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
    label { display: block; margin-top: 12px; margin-bottom: 6px; color: #aaa; font-size: 0.9em; }
    label:first-of-type { margin-top: 0; }
    input[type=text],
    input[type=password] { width: 100%; box-sizing: border-box; padding: 10px;
                           background: #333; border: 1px solid #444; border-radius: 6px;
                           color: #fff; font-size: 1em; }
    input[type=text]:focus,
    input[type=password]:focus { outline: none; border-color: #ff4444; }
    .radio-group { display: flex; gap: 12px; margin: 4px 0 8px; }
    .radio-group label { margin: 0; display: flex; align-items: center; gap: 6px;
                         color: #eee; font-size: 1em; cursor: pointer; }
    button { width: 100%; margin-top: 16px; padding: 12px; background: #ff4444;
             color: #fff; border: none; border-radius: 6px; font-size: 1em; cursor: pointer; }
    button:hover { background: #cc3333; }
    .msg  { background: #2a4a2a; border: 1px solid #4a7a4a; border-radius: 6px;
            padding: 10px 14px; margin-bottom: 16px; color: #88cc88; font-size: 0.9em; }
    .err  { background: #4a2a2a; border: 1px solid #7a4a4a; border-radius: 6px;
            padding: 10px 14px; margin-bottom: 16px; color: #cc8888; font-size: 0.9em; }
    .note { color: #666; font-size: 0.8em; margin-top: 8px; }
    .divider { border: none; border-top: 1px solid #3a3a3a; margin: 16px 0; }
    #staticFields { display: none; }
    .lock-row { display: flex; align-items: center; gap: 10px; }
    .lock-row input[type=text] { flex: 1; }
    .lock-row label { margin: 0; display: flex; align-items: center; gap: 5px;
                      color: #888; font-size: 0.85em; white-space: nowrap; cursor: pointer; }
    input:disabled { opacity: 0.4; cursor: not-allowed; }
    .tally-indicator { display: flex; align-items: center; gap: 14px; margin-bottom: 16px; }
    .tally-dot { width: 48px; height: 48px; border-radius: 50%; background: #333;
                 transition: background 0.2s; flex-shrink: 0; }
    .tally-dot.on { background: #ff2222; box-shadow: 0 0 16px #ff2222aa; }
    .tally-label { font-size: 1.4em; font-weight: bold; color: #555; }
    .tally-label.on { color: #ff4444; }
    .btn-row { display: flex; gap: 10px; }
    .btn-row button { margin-top: 0; }
    .btn-active   { background: #ff4444 !important; }
    .btn-active:hover { background: #cc3333 !important; }
    .btn-inactive { background: #444 !important; }
    .btn-inactive:hover { background: #555 !important; }
  </style>
</head>
<body>
  <h1>&#9679; Tally Bridge</h1>
  <div class="sub">SmallHD Tally Controller</div>
)rawliteral";

  html += "  <div class=\"sub\" style=\"margin-top:-18px; margin-bottom:20px;\">Firmware " + String(FW_VERSION) + "</div>";

  if (message.length() > 0) {
    bool isErr = message.startsWith("&#10007;");
    html += "<div class=\"" + String(isErr ? "err" : "msg") + "\">" + message + "</div>";
  }

  // Status card
  html += "<div class=\"card\"><h2>Status</h2>";
  html += "<div class=\"stat\"><span>Device Name</span><span>" + activeHostname + "</span></div>";
  html += "<div class=\"stat\"><span>Interface</span><span>" + iface + "</span></div>";
  html += "<div class=\"stat\"><span>IP Address</span><span>" + ip + "</span></div>";
  html += "<div class=\"stat\"><span>IP Mode</span><span>" + ipMode + "</span></div>";
  html += "<div class=\"stat\"><span>Tally Pin</span><span>GPIO" + String(activePin) + "</span></div>";
  html += "<div class=\"stat\"><span>Tally State</span><span>" + tstate + "</span></div>";
  html += "</div>";

  // Tally control card
  String dotClass   = (tstate == "ON") ? " on" : "";
  String labelClass = (tstate == "ON") ? " on" : "";
  html += "<div class=\"card\"><h2>Tally Control</h2>";
  html += "<div class=\"tally-indicator\">";
  html += "  <div class=\"tally-dot" + dotClass + "\" id=\"tallyDot\"></div>";
  html += "  <span class=\"tally-label" + labelClass + "\" id=\"tallyLabel\">TALLY " + tstate + "</span>";
  html += "</div>";
  String onClass  = (tstate == "ON")  ? "btn-active" : "btn-inactive";
  String offClass = (tstate == "OFF") ? "btn-active" : "btn-inactive";
  html += "<div class=\"btn-row\">";
  html += "  <button id=\"btnOn\"  class=\"" + onClass  + "\" onclick=\"setTally('on')\">ON</button>";
  html += "  <button id=\"btnOff\" class=\"" + offClass + "\" onclick=\"setTally('off')\">OFF</button>";
  html += "</div></div>";

  // Config card
  html += R"rawliteral(
  <div class="card">
    <h2>Configure</h2>
    <form method="POST" action="/config">

      <label>Device Hostname</label>
)rawliteral";

  html += "      <input type=\"text\" name=\"hostname\" id=\"hostnameField\" value=\"" + activeHostname
        + "\" maxlength=\"32\" pattern=\"[a-zA-Z0-9\\-]+\" oninput=\"updatePreview()\" required>";
  html += "      <p class=\"note\">Letters, numbers, and hyphens only.</p>";
  html += "      <p class=\"note\" style=\"color:#aaa;\">URL: <span id=\"urlPreview\" style=\"color:#fff;\">http://" + activeHostname + ".local/</span></p>";

  html += "      <label>Tally Output Pin</label>";
  html += "      <div class=\"lock-row\">";
  html += "        <input type=\"text\" name=\"pin\" id=\"pinField\" value=\"" + String(activePin)
        + "\" maxlength=\"2\" pattern=\"[0-9]+\" disabled>";
  html += "        <label><input type=\"checkbox\" onchange=\"document.getElementById('pinField').disabled=!this.checked\"> Unlock to edit</label>";
  html += "      </div>";
  html += "      <p class=\"note\">Default is GPIO18. Only change if that pin is damaged — GPIO15 is a confirmed substitute. Locked by default to prevent accidental changes.</p>";

  html += "      <hr class=\"divider\">";

  // IP mode radios
  html += "      <label>IP Mode</label>";
  html += "      <div class=\"radio-group\">";
  html += "        <label><input type=\"radio\" name=\"ipmode\" value=\"dhcp\""
        + String(!isStatic ? " checked" : "") + " onchange=\"toggleStatic(this)\"> DHCP</label>";
  html += "        <label><input type=\"radio\" name=\"ipmode\" value=\"static\""
        + String(isStatic ? " checked" : "") + " onchange=\"toggleStatic(this)\"> Static</label>";
  html += "      </div>";

  // Static IP fields — pre-filled if currently in static mode
  html += "      <div id=\"staticFields\">";
  html += "        <label>IP Address</label>";
  html += "        <input type=\"text\" name=\"ip\" value=\"" + staticIP.toString() + "\" placeholder=\"192.168.1.100\">";
  html += "        <label>Gateway</label>";
  html += "        <input type=\"text\" name=\"gw\" value=\"" + staticGW.toString() + "\" placeholder=\"192.168.1.1\">";
  html += "        <label>Subnet Mask</label>";
  html += "        <input type=\"text\" name=\"sn\" value=\"" + staticSN.toString() + "\" placeholder=\"255.255.255.0\">";
  html += "        <label>DNS Server</label>";
  html += "        <input type=\"text\" name=\"dns\" value=\"" + staticDNS.toString() + "\" placeholder=\"8.8.8.8\">";
  html += "      </div>";

  html += "      <hr class=\"divider\">";

  html += "      <label>WiFi SSID</label>";
  html += "      <input type=\"text\" name=\"ssid\" value=\"" + activeSSID + "\" maxlength=\"64\">";
  html += "      <label>WiFi Password</label>";
  html += "      <input type=\"password\" name=\"pass\" placeholder=\"leave blank to keep current\" maxlength=\"64\">";
  html += "      <p class=\"note\">Only used if Ethernet is not connected.</p>";

  // Show static fields immediately if currently in static mode, else JS handles it
  String initScript = isStatic
    ? "document.getElementById('staticFields').style.display='block';"
    : "";

  html += R"rawliteral(
      <button type="submit">Save &amp; Reboot</button>
    </form>
  </div>

  <div class="card">
    <h2>Factory Reset</h2>
    <p class="note" style="margin:0 0 12px;">Clears all saved settings and reboots. Device will return to firmware defaults.</p>
    <form method="POST" action="/reset" onsubmit="return confirm('Reset all settings to factory defaults?')">
      <button type="submit" style="background:#555;">Reset to Defaults</button>
    </form>
  </div>
  <script>
    function updatePreview() {
      var name = document.getElementById('hostnameField').value || 'tally-light';
      document.getElementById('urlPreview').textContent = 'http://' + name + '.local/';
    }
    function toggleStatic(el) {
      document.getElementById('staticFields').style.display =
        el.value === 'static' ? 'block' : 'none';
    }
    function setTally(state) {
      fetch('/tally/' + state)
        .then(() => {
          var on = state === 'on';
          document.getElementById('tallyDot').className   = 'tally-dot'   + (on ? ' on' : '');
          document.getElementById('tallyLabel').className = 'tally-label' + (on ? ' on' : '');
          document.getElementById('tallyLabel').textContent = 'TALLY ' + state.toUpperCase();
          document.getElementById('btnOn').className  = on ? 'btn-active' : 'btn-inactive';
          document.getElementById('btnOff').className = on ? 'btn-inactive' : 'btn-active';
        });
    }
)rawliteral";

  html += "    " + initScript;
  html += R"rawliteral(
  </script>
</body>
</html>
)rawliteral";

  return html;
}

// -----------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  // Load all settings from flash — fall back to defaults on first boot
  prefs.begin("tally", false);
  activeHostname = prefs.getString("hostname", DEFAULT_HOSTNAME);
  activeSSID     = prefs.getString("ssid",     DEFAULT_WIFI_SSID);
  activePass     = prefs.getString("pass",     DEFAULT_WIFI_PASS);
  ipMode         = prefs.getString("ipmode",   "dhcp");
  activePin      = prefs.getInt("pin", DEFAULT_TALLY_PIN);

  staticIP.fromString(prefs.getString("ip",  "192.168.1.100"));
  staticGW.fromString(prefs.getString("gw",  "192.168.1.1"));
  staticSN.fromString(prefs.getString("sn",  "255.255.255.0"));
  staticDNS.fromString(prefs.getString("dns", "8.8.8.8"));

  Serial.println("Hostname:  " + activeHostname);
  Serial.println("IP Mode:   " + ipMode);
  Serial.println("Tally Pin: GPIO" + String(activePin));
  if (ipMode == "static")
    Serial.println("Static IP: " + staticIP.toString());

  pinMode(activePin, OUTPUT);
  digitalWrite(activePin, LOW); // default LOW — tally OFF

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Network.onEvent(NetworkEvent);

  ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDR,
            ETH_CS_PIN, ETH_INT_PIN, ETH_RST_PIN,
            SPI2_HOST, ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN);

  WiFi.setHostname(activeHostname.c_str());
  if (ipMode == "static")
    WiFi.config(staticIP, staticGW, staticSN, staticDNS);
  WiFi.begin(activeSSID.c_str(), activePass.c_str());

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
  MDNS.addService("tally", "tcp", 80);

  // --- Config portal ---
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", buildConfigPage());
  });

  server.on("/config", HTTP_GET, []() {
    server.send(200, "text/html", buildConfigPage());
  });

  server.on("/config", HTTP_POST, []() {
    bool valid = server.hasArg("hostname") && server.arg("hostname").length() > 0;
    if (!valid) {
      server.send(400, "text/html", buildConfigPage("&#10007; Invalid hostname — please try again."));
      return;
    }

    prefs.putString("hostname", server.arg("hostname"));

    if (server.hasArg("pin") && server.arg("pin").length() > 0)
      prefs.putInt("pin", server.arg("pin").toInt());

    if (server.hasArg("ssid") && server.arg("ssid").length() > 0)
      prefs.putString("ssid", server.arg("ssid"));
    if (server.hasArg("pass") && server.arg("pass").length() > 0)
      prefs.putString("pass", server.arg("pass"));

    String newMode = server.hasArg("ipmode") ? server.arg("ipmode") : "dhcp";
    prefs.putString("ipmode", newMode);

    if (newMode == "static") {
      if (server.hasArg("ip"))  prefs.putString("ip",  server.arg("ip"));
      if (server.hasArg("gw"))  prefs.putString("gw",  server.arg("gw"));
      if (server.hasArg("sn"))  prefs.putString("sn",  server.arg("sn"));
      if (server.hasArg("dns")) prefs.putString("dns", server.arg("dns"));
    }

    server.send(200, "text/html",
      buildConfigPage("&#10003; Settings saved — rebooting now..."));
    delay(2000);
    prefs.end(); // flush to flash before reboot
    ESP.restart();
  });

  // --- Factory reset ---
  server.on("/reset", HTTP_POST, []() {
    prefs.clear();
    server.send(200, "text/html", buildConfigPage("&#10003; Settings cleared — rebooting to defaults..."));
    delay(2000);
    prefs.end();
    ESP.restart();
  });

  // --- Tally endpoints ---
  server.on("/tally/on", HTTP_GET, []() {
    digitalWrite(activePin, HIGH);
    server.send(200, "text/plain", "Tally ON");
    Serial.println("TALLY ON");
  });

  server.on("/tally/off", HTTP_GET, []() {
    digitalWrite(activePin, LOW);
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
    String s     = digitalRead(activePin) ? "ON" : "OFF";
    server.send(200, "text/plain",
      "Tally Bridge " + String(FW_VERSION) + " | " + iface + " IP: " + ip
      + " | Mode: " + ipMode + " | Tally: " + s + " | Host: " + activeHostname);
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
