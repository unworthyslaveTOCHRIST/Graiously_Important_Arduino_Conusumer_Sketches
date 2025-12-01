// ALL THANKS AND GLORY TO THE AND my ONLY GOD AND LORD JESUS CHRIST ALONE      
// ALL THANKS AND GLORY TO THE AND my ONLY GOD AND LORD JESUS CHRIST ALONE                                                          

#define BLYNK_TEMPLATE_ID "TMPL2NgarV-bB"
#define BLYNK_TEMPLATE_NAME "Persistent Sensor Data Storage"
#define BLYNK_AUTH_TOKEN "GvSbhN7aDoKmocndRitpAZ5_h9C-_2SV"
#define DHTPin 32
#define DHTTYPE DHT22  

// ===================== Includes =====================
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <BlynkSimpleEsp32.h>
#include <Arduino.h>
#include <esp_system.h>


#include <SoftwareSerial.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include <WiFiClientSecure.h> 
#include "DHT.h"
#include "FS.h"
#include "SD.h"
#include "esp_wifi.h"


// Graphics and font library
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library


#define BLYNK_PRINT Serial

char auth[] = BLYNK_AUTH_TOKEN;

BlynkTimer timer;

// ===================== WiFi / Server Setup =====================
const char apSSID[] = "PersistentDataLogger";
const char apPass[] = "persist_1234";
IPAddress ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);


AsyncWebServer server(80);
Preferences preferences;

// Sensor references
const int MQ135_SENSOR_PIN = 34;
const int Light_Intensity_SENSOR_PIN = 35; // To be graciously moved to 33
const int Rainfall_SENSOR_PIN = 33; // To be graciously moved to 35=8


// Graciously setting DHT pin and type
DHT dht(DHTPin, DHTTYPE);


// Mock sensor readings
int current_ambient_light_intensity = 1267;
int current_ambient_temperature = 1267;
int current_ambient_relative_humidity = 1267;
int current_degree_of_rainfall = 1267;
int current_air_quality_reading = 1267;


// ===================== Global Variables =====================
String hotspotName = "";
String hotspotPassword = "";
bool shouldConnectToHotspot = false;   // Flag for non-blocking switch
int noOfTimesOfHotspotConnection = 0;
int currentNumOfStation = 0;
bool AP_STA_SWITCH_SUCCESSFUL = false;
bool shouldInteractWithBlynk = false;
bool wifiScreenRendered = false;



const char* GTLJC_host = "persistentdataloggersystem.pythonanywhere.com";
const int GTLJC_httpsPort = 443;
const char* GTLJC_storage_route = "/storage/expand/";

const int timeLimit = 10000;

// Gracious routine for sending Json
String GTLJC_sendJsonBatch(const String& rawBatch, const String& GTLJC_url) {        
          WiFiClientSecure client;
          client.setInsecure(); // ❗ Trusts all certificates — for development/testing only
          // HTTPClient http;
          
          Serial.print("🌐 Connecting to ");
          Serial.println(GTLJC_host);

          if(!client.connect(GTLJC_host, GTLJC_httpsPort)){
            Serial.println("❌ HTTPS connection failed");
            // lcd.clear();
            // lcd.setCursor(0,0);
            // lcd.print("HTTPS connection");
            // lcd.setCursor(0,1);
            // lcd.print("failed");
            // delay(3000);
            // backend_connection_established = true;
            return "";
          }

          // Graciously sending HTTP headers and body
          client.println("POST " + String(GTLJC_url) + " HTTP/1.1");
          client.println("Host: " + String(GTLJC_host));
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.print("Content-Length: ");
          client.println(rawBatch.length());
          client.println();  // End of headers
          
          client.write((const uint8_t *)rawBatch.c_str(), rawBatch.length());
          Serial.print(rawBatch);
          // client.print(rawBatch); // Send raw data
          
          // Graciously reading server response
         Serial.println("📨 Server Response:");
          String response = "";

          while (client.connected() || client.available()) {
            if (client.available()) {
              char c = client.read();
              response += c;
              Serial.print(c);
            }
          }
          // ✅ Always close the connection
          client.stop();
          Serial.println("\n✅ HTTPS text POST complete.");
        
          return response;
}

String interpret_rainfall_sensor_reading(int sensor_value){
  if (sensor_value < 3001) {
    return "Rain Detected!";
  }else{
    return "No Rainfall!";
  } 
}

String interpret_light_sensor_reading(int sensor_value) {
  if (sensor_value < 2001) {
    return "Brighter Atmosphere!";
  }else{
    return "Dark/Shadowy Atmosphere!";
  } 
}

String interpret_air_quality(int sensor_value) {
  if (sensor_value < 1001) {
    return "Good";
  } else if (sensor_value < 1501) {
    return "Moderate";
  } else{
    return "Poor";
  } 
}



void handleSensorReadings(){

  // Get raw sensor readings
  current_ambient_light_intensity = analogRead(Light_Intensity_SENSOR_PIN);
  Serial.print("current_ambient_light_intensity: ");
  Serial.println(current_ambient_light_intensity);

  current_ambient_temperature = dht.readTemperature();
  current_ambient_relative_humidity = dht.readHumidity();
  current_degree_of_rainfall = analogRead(Rainfall_SENSOR_PIN);
  Serial.print("current_degree_of_rainfall: ");
  Serial.println(current_degree_of_rainfall);
  current_air_quality_reading = analogRead(MQ135_SENSOR_PIN);
  Serial.print("current_air_quality_reading: ");
  Serial.println(current_air_quality_reading);

  String intepreted_light_intensity_reading = String(current_ambient_light_intensity) + " (" + interpret_light_sensor_reading(current_ambient_light_intensity) + ")";
  String intepreted_degree_of_rainfall = String(current_degree_of_rainfall) + " (" + interpret_rainfall_sensor_reading(current_degree_of_rainfall) + ")";
  String intepreted_air_quality_reading = String(current_air_quality_reading) + " (" +  interpret_air_quality(current_air_quality_reading) + ")";
  
  String stringified_temp = String(current_ambient_temperature);
  String stringified_hum = String(current_ambient_relative_humidity);

  String timestamp = "unavailable";
  String backend_data = stringified_temp + "," + stringified_hum + "," + intepreted_light_intensity_reading + "," + intepreted_degree_of_rainfall + "," + intepreted_air_quality_reading + "\n";
  String local_data = stringified_temp + "," + stringified_hum + "," + intepreted_light_intensity_reading + "," + intepreted_degree_of_rainfall + "," + intepreted_air_quality_reading;
  // // Sending to backend
    String GTLJC_backend_response = GTLJC_sendJsonBatch(backend_data, GTLJC_storage_route);
    String refined_response = GTLJC_backend_response.substring(GTLJC_backend_response.length() - 94, GTLJC_backend_response.length() - 35);
    // Get the timestamp of sent row if available
    Serial.println("Graciously Sending cloud-storage..");
    int idx = -33;
    // idx = GTLJC_backend_response.indexOf("@");
    timestamp = GTLJC_backend_response.substring(GTLJC_backend_response.length() - 33,GTLJC_backend_response.length() - 1 );
    if(timestamp == ""){
      timestamp = "unavailable";
    }else{
      timestamp = timestamp;
    }
    local_data += "," + timestamp + "\n";
  

    appendFile(SD, "/GTLJC_data.txt", local_data);

    GTLJC_backend_response = "";
    Serial.print("Refined Backend response: ");
    Serial.print(refined_response);
    Serial.println("Graciously ends here.");
    Serial.print("Extracted timestamp: ");
    Serial.println(timestamp);

    char str[12];
    tft.setTextSize(2);
    if (timestamp == "unavailable"){
      timestamp = "Failed!";
      sprintf(str, "%s", timestamp);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString(str,10, 90,2);
    }
    else {
      timestamp = "Successful!";
      sprintf(str, "%s", timestamp);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString(str,10, 90,2);
    }

  return ;
}

void SD_INITIALIZATION(){
  // Initialise the SD library before the TFT so the chip select is defined
  SPI.begin(14, 2, 15, 13);

  if (!SD.begin(13)) {
    Serial.println("Card Mount Failed");
    return;
  }

  Serial.println("✅ SD mounted successfully");
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);


}

// Gracious functions for writing into sd card

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, String message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, String message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}




void sendReadingsToBlynk()
{

    Blynk.virtualWrite(V0, current_ambient_light_intensity);
    Blynk.virtualWrite(V1, current_ambient_temperature);
    Blynk.virtualWrite(V2, current_ambient_relative_humidity);
    Blynk.virtualWrite(V3, current_degree_of_rainfall );
    Blynk.virtualWrite(V4, current_air_quality_reading);
    handleSensorReadings();
}

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
        // align-items: center;
        padding-top: 50px;
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
        <input type="text" id="hotspot_password" placeholder="Enter password">
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
  // >>> TFT DISPLAY (ADDED)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("Connecting to: ");
  tft.setTextSize(2);
  tft.setCursor(10, 45);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.println(hotspotName);

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
  while (WiFi.status() != WL_CONNECTED ) {
    // && millis() - startAttempt < timeout
    Serial.print(".");
    delay(100);
    yield();  // Allow other tasks and AsyncWebServer to run
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected successfully!");
    Serial.print("STA IP: "); Serial.println(WiFi.localIP());
    AP_STA_SWITCH_SUCCESSFUL = true; // A check verifying a fulfilled switch from WIFI_AP to WIFI_STA
    Blynk.begin(auth, hotspotName.c_str(), hotspotPassword.c_str());
    timer.setInterval(100L, sendReadingsToBlynk);
    if (!wifiScreenRendered) {
        Serial.println("wifi screen graciously rendering");
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, 20);
        tft.println("Hotspot Connected :");
        

        tft.setTextSize(2);
        tft.setCursor(10, 50);
        tft.println(hotspotName.c_str());

        wifiScreenRendered = true;
        delay(3000);

        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);

        // Set text datum to middle centre
        tft.setTextDatum(MC_DATUM);

        tft.setCursor(10, 20);
        tft.println("      NOW UPLOADING");

        tft.setCursor(10, 60);
        tft.println("          TO");

        tft.setCursor(10, 90);
        tft.println("       BACKEND");

        tft.setCursor(10, 120 );
        tft.println("         ...");

        tft.setCursor(10, 140 );
        tft.println("     View data on");

        tft.setCursor(10, 160 );
        tft.println("   Mobile and Web App");


        // tft.fillScreen(TFT_BLACK);

        

    }


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
    wifiScreenRendered = false;

    // >>> TFT DISPLAY (ADDED)
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 20);
    tft.println("WiFi Failed");
    tft.setTextSize(1);
    tft.setCursor(10, 45);
    tft.println("Reverting to AP");
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
  dht.begin();
  // Initialise the TFT after the SD card!
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  // // tft.fillScreen(TFT_ORANGE);
   // >>> TFT DISPLAY (ADDED)
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.println("  Persistent Data Logger");
  delay(5000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("System Booting...");
  delay(2000);
  // SD_INITIALIZATION();
  SPI.begin(14, 2, 15, 13);
  // >>> TFT DISPLAY (ADDED)
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 55);
  tft.println("Initializing SD...");
  delay(2000);
  if (!SD.begin(13)) {
    Serial.println("Card Mount Failed");
    // >>> TFT DISPLAY (ADDED)
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 95);
    tft.println("SD Card FAILED");
    tft.setCursor(10, 125);
    tft.println("Restarting...");
    delay(5000);
    esp_restart();
    return;
  }

  // >>> TFT DISPLAY (ADDED)
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 75);
  tft.println("SD Card OK");
  Serial.println("✅ SD mounted successfully");

  // Load stored credentials if available
  preferences.begin("wifi", true);
  hotspotName = preferences.getString("ssid", "");
  hotspotPassword = preferences.getString("pass", "");
  preferences.end();

  // Setup AP
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(apSSID, apPass);
  

  Serial.printf("AP running. IP: %s\n", WiFi.softAPIP().toString().c_str());
  // >>> TFT DISPLAY (ADDED)
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 95);
  tft.println("Provide Hotspot Creds:");
  tft.setCursor(10, 115);
  tft.print("Open Browser and Type");
  tft.println("");
  tft.setCursor(10, 135);
  tft.print(WiFi.softAPIP());
  tft.println(":80");
 

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
    if (request->hasParam("value", true)){
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

  // // Check if we should switch to STA
  if (shouldConnectToHotspot) {
    shouldConnectToHotspot = false;
    Serial.println("hotSpot connection re-initiated.");
    connectToProvidedHotspot();  // safe call outside server task
  }

  if (AP_STA_SWITCH_SUCCESSFUL){
    Blynk.run();
    timer.run();
        
  }

}

