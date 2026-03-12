#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>

// AP Mode Configuration
const char* AP_SSID = "WeatherStation-Setup";
const char* AP_PASSWORD = "weather123";  // Min 8 characters
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// DNS server for captive portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Web server
WebServer webServer(80);

// Preferences for persistent storage
Preferences preferences;

// Flag to force AP mode (stored in preferences)
bool forceAPMode = false;

// Runtime config - loaded from preferences or defaults
struct ConfigData {
  char wifi_ssid[33];
  char wifi_password[65];
  char wifi_ssid2[33];
  char wifi_password2[65];
  char wifi_ssid3[33];
  char wifi_password3[65];
  char api_key[65];
  char city[33];
  char latitude[16];
  char longitude[16];
  char language[8];
  char hemisphere[8];
  char units[4];
  char timezone[32];
  int gmt_offset;
  int dst_offset;
} config;

// HTML page for configuration
const char CONFIG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Weather Station Setup</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
    .container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    h1 { color: #333; text-align: center; }
    h2 { color: #666; border-bottom: 1px solid #ddd; padding-bottom: 10px; margin-top: 25px; }
    label { display: block; margin-top: 15px; color: #555; font-weight: bold; }
    input, select { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
    input:focus, select:focus { outline: none; border-color: #4CAF50; }
    button { width: 100%; padding: 15px; margin-top: 25px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
    button:hover { background: #45a049; }
    .info { font-size: 12px; color: #888; margin-top: 3px; }
    .section { margin-bottom: 20px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Weather Station Setup</h1>
    <form action="/save" method="POST">

      <h2>WiFi Networks</h2>
      <div class="section">
        <label>WiFi SSID 1 (Primary)</label>
        <input type="text" name="ssid1" value="%SSID1%" maxlength="32">
        <label>Password 1</label>
        <input type="password" name="pass1" value="%PASS1%" maxlength="64">

        <label>WiFi SSID 2 (Optional)</label>
        <input type="text" name="ssid2" value="%SSID2%" maxlength="32">
        <label>Password 2</label>
        <input type="password" name="pass2" value="%PASS2%" maxlength="64">

        <label>WiFi SSID 3 (Optional)</label>
        <input type="text" name="ssid3" value="%SSID3%" maxlength="32">
        <label>Password 3</label>
        <input type="password" name="pass3" value="%PASS3%" maxlength="64">
      </div>

      <h2>OpenWeatherMap</h2>
      <div class="section">
        <label>API Key</label>
        <input type="text" name="apikey" value="%APIKEY%" maxlength="64">
        <div class="info">Get free API key at openweathermap.org</div>
      </div>

      <h2>Location</h2>
      <div class="section">
        <label>City Name</label>
        <input type="text" name="city" value="%CITY%" maxlength="32">

        <label>Latitude</label>
        <input type="text" name="lat" value="%LAT%" maxlength="15">

        <label>Longitude</label>
        <input type="text" name="lon" value="%LON%" maxlength="15">

        <label>Hemisphere</label>
        <select name="hemisphere">
          <option value="north" %NORTH_SEL%>North</option>
          <option value="south" %SOUTH_SEL%>South</option>
        </select>
      </div>

      <h2>Display Settings</h2>
      <div class="section">
        <label>Language</label>
        <input type="text" name="lang" value="%LANG%" maxlength="7">
        <div class="info">e.g., EN, ES, DE, FR, IT, PT</div>

        <label>Units</label>
        <select name="units">
          <option value="M" %METRIC_SEL%>Metric (Celsius)</option>
          <option value="I" %IMPERIAL_SEL%>Imperial (Fahrenheit)</option>
        </select>
      </div>

      <h2>Timezone</h2>
      <div class="section">
        <label>Timezone String</label>
        <input type="text" name="tz" value="%TZ%" maxlength="31">
        <div class="info">e.g., CST6, EST5EDT, GMT0, CET-1</div>

        <label>GMT Offset (seconds)</label>
        <input type="number" name="gmt" value="%GMT%">
        <div class="info">e.g., -18000 for EST, 0 for GMT, 3600 for CET</div>

        <label>DST Offset (seconds)</label>
        <input type="number" name="dst" value="%DST%">
        <div class="info">Usually 0 or 3600</div>
      </div>

      <button type="submit">Save Configuration</button>
    </form>

    <form action="/reboot" method="POST" style="margin-top: 10px;">
      <button type="submit" style="background: #2196F3;">Save & Restart</button>
    </form>

    <form action="/reset" method="POST" style="margin-top: 10px;">
      <button type="submit" style="background: #f44336;">Factory Reset</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

const char SAVE_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Settings Saved</title>
  <style>
    body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background: #f0f0f0; }
    .container { text-align: center; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    h1 { color: #4CAF50; }
    a { display: inline-block; margin-top: 20px; padding: 10px 20px; background: #2196F3; color: white; text-decoration: none; border-radius: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Settings Saved!</h1>
    <p>Your configuration has been saved.</p>
    <a href="/">Back to Settings</a>
  </div>
</body>
</html>
)rawliteral";

const char REBOOT_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Restarting...</title>
  <style>
    body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background: #f0f0f0; }
    .container { text-align: center; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    h1 { color: #2196F3; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Restarting...</h1>
    <p>The device will restart and connect to your WiFi network.</p>
    <p>If configuration was successful, the weather display will update.</p>
  </div>
</body>
</html>
)rawliteral";

// Load configuration from preferences
void loadConfig() {
  preferences.begin("weather", true);  // Read-only

  strlcpy(config.wifi_ssid, preferences.getString("ssid1", "").c_str(), sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, preferences.getString("pass1", "").c_str(), sizeof(config.wifi_password));
  strlcpy(config.wifi_ssid2, preferences.getString("ssid2", "").c_str(), sizeof(config.wifi_ssid2));
  strlcpy(config.wifi_password2, preferences.getString("pass2", "").c_str(), sizeof(config.wifi_password2));
  strlcpy(config.wifi_ssid3, preferences.getString("ssid3", "").c_str(), sizeof(config.wifi_ssid3));
  strlcpy(config.wifi_password3, preferences.getString("pass3", "").c_str(), sizeof(config.wifi_password3));
  strlcpy(config.api_key, preferences.getString("apikey", "").c_str(), sizeof(config.api_key));
  strlcpy(config.city, preferences.getString("city", "").c_str(), sizeof(config.city));
  strlcpy(config.latitude, preferences.getString("lat", "").c_str(), sizeof(config.latitude));
  strlcpy(config.longitude, preferences.getString("lon", "").c_str(), sizeof(config.longitude));
  strlcpy(config.language, preferences.getString("lang", "EN").c_str(), sizeof(config.language));
  strlcpy(config.hemisphere, preferences.getString("hemi", "north").c_str(), sizeof(config.hemisphere));
  strlcpy(config.units, preferences.getString("units", "M").c_str(), sizeof(config.units));
  strlcpy(config.timezone, preferences.getString("tz", "GMT0").c_str(), sizeof(config.timezone));
  config.gmt_offset = preferences.getInt("gmt", 0);
  config.dst_offset = preferences.getInt("dst", 0);
  forceAPMode = preferences.getBool("forceAP", false);

  preferences.end();
}

// Save configuration to preferences
void saveConfig() {
  preferences.begin("weather", false);  // Read-write

  preferences.putString("ssid1", config.wifi_ssid);
  preferences.putString("pass1", config.wifi_password);
  preferences.putString("ssid2", config.wifi_ssid2);
  preferences.putString("pass2", config.wifi_password2);
  preferences.putString("ssid3", config.wifi_ssid3);
  preferences.putString("pass3", config.wifi_password3);
  preferences.putString("apikey", config.api_key);
  preferences.putString("city", config.city);
  preferences.putString("lat", config.latitude);
  preferences.putString("lon", config.longitude);
  preferences.putString("lang", config.language);
  preferences.putString("hemi", config.hemisphere);
  preferences.putString("units", config.units);
  preferences.putString("tz", config.timezone);
  preferences.putInt("gmt", config.gmt_offset);
  preferences.putInt("dst", config.dst_offset);
  preferences.putBool("forceAP", false);  // Clear force AP flag after save

  preferences.end();
}

// Reset to factory defaults
void resetConfig() {
  preferences.begin("weather", false);
  preferences.clear();
  preferences.end();
}

// Set force AP mode flag
void setForceAPMode(bool force) {
  preferences.begin("weather", false);
  preferences.putBool("forceAP", force);
  preferences.end();
  forceAPMode = force;
}

// Check if configuration exists
bool hasValidConfig() {
  return strlen(config.wifi_ssid) > 0 && strlen(config.api_key) > 0;
}

// Process template placeholders
String processTemplate(const char* page) {
  String html = String(page);

  html.replace("%SSID1%", config.wifi_ssid);
  html.replace("%PASS1%", config.wifi_password);
  html.replace("%SSID2%", config.wifi_ssid2);
  html.replace("%PASS2%", config.wifi_password2);
  html.replace("%SSID3%", config.wifi_ssid3);
  html.replace("%PASS3%", config.wifi_password3);
  html.replace("%APIKEY%", config.api_key);
  html.replace("%CITY%", config.city);
  html.replace("%LAT%", config.latitude);
  html.replace("%LON%", config.longitude);
  html.replace("%LANG%", config.language);
  html.replace("%TZ%", config.timezone);
  html.replace("%GMT%", String(config.gmt_offset));
  html.replace("%DST%", String(config.dst_offset));

  // Hemisphere selection
  html.replace("%NORTH_SEL%", strcmp(config.hemisphere, "north") == 0 ? "selected" : "");
  html.replace("%SOUTH_SEL%", strcmp(config.hemisphere, "south") == 0 ? "selected" : "");

  // Units selection
  html.replace("%METRIC_SEL%", strcmp(config.units, "M") == 0 ? "selected" : "");
  html.replace("%IMPERIAL_SEL%", strcmp(config.units, "I") == 0 ? "selected" : "");

  return html;
}

// Web server handlers
void handleRoot() {
  String html = processTemplate(CONFIG_PAGE);
  webServer.send(200, "text/html", html);
}

void handleSave() {
  // Get form values
  strlcpy(config.wifi_ssid, webServer.arg("ssid1").c_str(), sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, webServer.arg("pass1").c_str(), sizeof(config.wifi_password));
  strlcpy(config.wifi_ssid2, webServer.arg("ssid2").c_str(), sizeof(config.wifi_ssid2));
  strlcpy(config.wifi_password2, webServer.arg("pass2").c_str(), sizeof(config.wifi_password2));
  strlcpy(config.wifi_ssid3, webServer.arg("ssid3").c_str(), sizeof(config.wifi_ssid3));
  strlcpy(config.wifi_password3, webServer.arg("pass3").c_str(), sizeof(config.wifi_password3));
  strlcpy(config.api_key, webServer.arg("apikey").c_str(), sizeof(config.api_key));
  strlcpy(config.city, webServer.arg("city").c_str(), sizeof(config.city));
  strlcpy(config.latitude, webServer.arg("lat").c_str(), sizeof(config.latitude));
  strlcpy(config.longitude, webServer.arg("lon").c_str(), sizeof(config.longitude));
  strlcpy(config.language, webServer.arg("lang").c_str(), sizeof(config.language));
  strlcpy(config.hemisphere, webServer.arg("hemisphere").c_str(), sizeof(config.hemisphere));
  strlcpy(config.units, webServer.arg("units").c_str(), sizeof(config.units));
  strlcpy(config.timezone, webServer.arg("tz").c_str(), sizeof(config.timezone));
  config.gmt_offset = webServer.arg("gmt").toInt();
  config.dst_offset = webServer.arg("dst").toInt();

  saveConfig();

  webServer.send(200, "text/html", SAVE_PAGE);
}

void handleReboot() {
  // Save first
  strlcpy(config.wifi_ssid, webServer.arg("ssid1").c_str(), sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, webServer.arg("pass1").c_str(), sizeof(config.wifi_password));
  strlcpy(config.wifi_ssid2, webServer.arg("ssid2").c_str(), sizeof(config.wifi_ssid2));
  strlcpy(config.wifi_password2, webServer.arg("pass2").c_str(), sizeof(config.wifi_password2));
  strlcpy(config.wifi_ssid3, webServer.arg("ssid3").c_str(), sizeof(config.wifi_ssid3));
  strlcpy(config.wifi_password3, webServer.arg("pass3").c_str(), sizeof(config.wifi_password3));
  strlcpy(config.api_key, webServer.arg("apikey").c_str(), sizeof(config.api_key));
  strlcpy(config.city, webServer.arg("city").c_str(), sizeof(config.city));
  strlcpy(config.latitude, webServer.arg("lat").c_str(), sizeof(config.latitude));
  strlcpy(config.longitude, webServer.arg("lon").c_str(), sizeof(config.longitude));
  strlcpy(config.language, webServer.arg("lang").c_str(), sizeof(config.language));
  strlcpy(config.hemisphere, webServer.arg("hemisphere").c_str(), sizeof(config.hemisphere));
  strlcpy(config.units, webServer.arg("units").c_str(), sizeof(config.units));
  strlcpy(config.timezone, webServer.arg("tz").c_str(), sizeof(config.timezone));
  config.gmt_offset = webServer.arg("gmt").toInt();
  config.dst_offset = webServer.arg("dst").toInt();

  saveConfig();

  webServer.send(200, "text/html", REBOOT_PAGE);
  delay(1000);
  ESP.restart();
}

void handleReset() {
  resetConfig();
  webServer.send(200, "text/html", "<html><body><h1>Factory Reset Complete</h1><p>Device will restart...</p></body></html>");
  delay(1000);
  ESP.restart();
}

void handleNotFound() {
  // Redirect all requests to root (captive portal behavior)
  webServer.sendHeader("Location", "http://" + AP_IP.toString(), true);
  webServer.send(302, "text/plain", "");
}

// Start AP mode with captive portal
void startAPMode() {
  Serial.println("Starting AP Mode...");

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", AP_IP);

  // Setup web server routes
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/save", HTTP_POST, handleSave);
  webServer.on("/reboot", HTTP_POST, handleReboot);
  webServer.on("/reset", HTTP_POST, handleReset);
  webServer.onNotFound(handleNotFound);

  webServer.begin();
  Serial.println("Web server started");
  Serial.printf("Connect to WiFi: %s (password: %s)\n", AP_SSID, AP_PASSWORD);
  Serial.println("Then open http://192.168.4.1 in your browser");
}

// Run AP mode loop (call this in main loop when in AP mode)
void handleAPMode() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}

// Stop AP mode
void stopAPMode() {
  webServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
}

// Get WiFi credentials as array for connection
struct WiFiCredentialEntry {
  const char* ssid;
  const char* password;
};

int getConfiguredNetworks(WiFiCredentialEntry* networks, int maxNetworks) {
  int count = 0;

  if (strlen(config.wifi_ssid) > 0 && count < maxNetworks) {
    networks[count].ssid = config.wifi_ssid;
    networks[count].password = config.wifi_password;
    count++;
  }
  if (strlen(config.wifi_ssid2) > 0 && count < maxNetworks) {
    networks[count].ssid = config.wifi_ssid2;
    networks[count].password = config.wifi_password2;
    count++;
  }
  if (strlen(config.wifi_ssid3) > 0 && count < maxNetworks) {
    networks[count].ssid = config.wifi_ssid3;
    networks[count].password = config.wifi_password3;
    count++;
  }

  return count;
}

#endif // WIFI_MANAGER_H
