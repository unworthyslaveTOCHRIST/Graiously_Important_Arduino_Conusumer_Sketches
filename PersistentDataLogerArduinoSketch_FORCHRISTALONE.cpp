// ALL THANKS AND GLORY TO THE AND my ONLY GOD AND LORD JESUS CHRIST ALONE

// ===================== Includes =====================
#include <WiFi.h>
#include <ESPAsyncWebServer.h>  //Key Statement 1: Ensure to use latest version providing tcp_alloc fixes
#include <Preferences.h>

// ===================== WiFi / Server Setup =====================
const char apSSID[] = "PersistentDataLogger";
const char apPass[] = "persist_1234";
IPAddress ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);
Preferences preferences;

// ===================== Global Variables =====================
String hotspotName = "";
String hotspotPassword = "";
bool shouldConnectToHotspot = false;   // Flag for non-blocking switch
int currentNumOfStation = 0;

// ===================== HTML Page =====================
const char webpage[] PROGMEM = R"=====(  
<!DOCTYPE html>
<html>
  <head>
    <title>Persistent Data Logger</title>
    <style>
      body {
        margin: 0;
        font-family: Arial, sans-serif;
        background: linear-gradient(135deg, #5f9df7, #ff6b6b);
        height: 100vh;
        display: flex;
        justify-content: center;
        align-items: center;
      }

      .container {
        background: rgba(255,255,255,0.15);
        backdrop-filter: blur(12px);
        padding: 25px 40px;
        border-radius: 15px;
        text-align: center;
        color: white;
        width: 90%;
        max-width: 380px;
        box-shadow: 0 8px 20px rgba(0,0,0,0.2);
      }

      h1 {
        font-size: 22px;
        margin-bottom: 20px;
      }

      h3 {
        font-size: 18px;
        margin-bottom: 15px;
      }

      .input-group {
        text-align: left;
        margin-bottom: 18px;
      }

      .input-group label {
        font-weight: bold;
        display: block;
        margin-bottom: 5px;
        font-size: 15px;
      }

      .input-group input {
        width: 100%;
        padding: 10px;
        border-radius: 8px;
        border: none;
        font-size: 15px;
        outline: none;
      }

      button {
        margin-top: 10px;
        width: 100%;
        padding: 10px;
        border: none;
        border-radius: 8px;
        font-size: 16px;
        font-weight: bold;
        background: rgba(255,255,255,0.85);
        cursor: pointer;
      }

      button:hover {
        background: white;
      }
    </style>
  </head>

  <body>
    <div class="container">
      <h1>Persistent Data Logger</h1>

      <h3>Provide your hotspot credentials</h3>

      <div class="input-group">
        <label>Hotspot Name</label>
        <input type="text" id="hotspot_name" placeholder="Enter SSID">
      </div>

      <div class="input-group">
        <label>Hotspot Password</label>
        <input type="password" id="hotspot_password" placeholder="Enter password">
      </div>

      <button onclick="sendDataToEsp32()">Send Credentials</button>
    </div>

    <script>
      function sendDataToEsp32() {
        var hotspotName = document.getElementById("hotspot_name").value;
        var hotspotPassword = document.getElementById("hotspot_password").value;

        sendHotspotName(hotspotName);
        sendHotspotPassword(hotspotPassword);
      }

      function sendHotspotName(value) {
        var xhttp = new XMLHttpRequest();
        xhttp.open("POST", "/hotspot-name", true);
        xhttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhttp.send("value=" + encodeURIComponent(value));
      }

      function sendHotspotPassword(value) {
        var xhttp = new XMLHttpRequest();
        xhttp.open("POST", "/hotspot-pass", true);
        xhttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhttp.send("value=" + encodeURIComponent(value));
      }
    </script>

  </body>
</html>
)=====";


// ===================== Function to connect to hotspot =====================
void connectToProvidedHotspot() {
  if (hotspotName.isEmpty() || hotspotPassword.isEmpty()) return;

  Serial.println("== Switching from AP to STA Mode ==");

  // Stop the server before mode switch
  server.end();
  delay(200);

  // Disconnect AP
  WiFi.softAPdisconnect(true);
  delay(300);

  // Set STA mode
  WiFi.mode(WIFI_STA);
  delay(300);

  // Begin connection
  Serial.printf("Connecting to hotspot: %s\n", hotspotName.c_str());
  WiFi.begin(hotspotName.c_str(), hotspotPassword.c_str());

  // Non-blocking WiFi wait with yield
  unsigned long startAttempt = millis();
  const unsigned long timeout = 30000;  // 30s timeout
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeout) {
    Serial.print(".");
    delay(100);
    yield();  // Key Statement 2: Ensure to use yield as it allows other tasks and AsyncWebServer to run when ESP32 WiFi mode is switched from WIFI_AP to WIFI_STA
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected successfully!");
    Serial.print("STA IP: "); Serial.println(WiFi.localIP());

    // Save credentials
    preferences.begin("wifi", false);
    preferences.putString("ssid", hotspotName);
    preferences.putString("pass", hotspotPassword);
    preferences.end();

    // Restart server in STA mode
    server.begin();
  }
  else {
    Serial.println("Connection failed, reverting to AP mode.");
    WiFi.mode(WIFI_AP);
    delay(300);
    WiFi.softAP(apSSID, apPass);
    delay(300);
    Serial.print("AP restored. IP: "); Serial.println(WiFi.softAPIP());
    server.begin();
  }
}

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);

  // Load stored credentials if available
  preferences.begin("wifi", true);
  hotspotName = preferences.getString("ssid", "");
  hotspotPassword = preferences.getString("pass", "");
  preferences.end();

  // Setup AP
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(apSSID, apPass);

  Serial.printf("AP running. IP: %s\n", WiFi.softAPIP().toString().c_str());

  // Setup AsyncWebServer routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200,"text/html",webpage);
  });

  server.on("/hotspot-name", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      hotspotName = request->getParam("value", true)->value();
      Serial.println("Received Hotspot Name: " + hotspotName);
      shouldConnectToHotspot = true;  // set flag instead of immediate connect
      request->send(200);
    }
  });

  server.on("/hotspot-pass", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      hotspotPassword = request->getParam("value", true)->value();
      Serial.println("Received Hotspot Password: " + hotspotPassword);
      shouldConnectToHotspot = true;  // set flag
      request->send(200);
    }
  });

  server.begin();
}

// ===================== Main Loop =====================
void loop() {
  // Print connected stations count
  int stationNum = WiFi.softAPgetStationNum();
  if (stationNum != currentNumOfStation) {
    currentNumOfStation = stationNum;
    Serial.printf("Stations connected to AP: %d\n", currentNumOfStation);
  }

  // Check if we should switch to STA
  if (shouldConnectToHotspot) {
    shouldConnectToHotspot = false;
    connectToProvidedHotspot();  // safe call outside server task
  }
}


