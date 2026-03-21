const bool DebugDisplayUpdate = false;

// ============================================================================
// NOTE: These are DEFAULT/FALLBACK values. You can configure all settings
// via the web interface when:
//   1. No configured WiFi network is found (automatic), OR
//   2. FORCE_AP_MODE is set to true in the main .ino file
// The device will create a WiFi hotspot "WeatherStation-Setup" for configuration.
// Web-configured values will override these defaults.
// ============================================================================

// Set to your WiFi credentials (multiple SSIDs supported)
// The device will scan and connect to the one with the highest signal strength
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

const WiFiCredentials wifiNetworks[] = {
  {"Your_WiFi_SSID", "Your_WiFi_Password"},
  // Add more networks below:
  // {"Network2", "password2"},
  // {"Network3", "password3"},
};

const int wifiNetworkCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// Get API key by signing up for a free developer account at https://openweathermap.org/
String apikey       = "YOUR_OPENWEATHERMAP_API_KEY";              // See: https://openweathermap.org/
const char server[] = "api.openweathermap.org";

// Groq API key for weather narrative generation (free tier)
// Get key at: console.groq.com
String groq_apikey = "YOUR_GROQ_API_KEY";

//Set your location
String City             = "Your City";                            // Your home city name
String Latitude         = "0.0000";                               // Latitude of your location in decimal degrees
String Longitude        = "0.0000";                               // Longitude of your location in decimal degrees

String Language         = "ES";                            // NOTE: Only the weather description is translated by OWM
                                                           // Examples: Arabic (AR) Czech (CZ) English (EN) Greek (EL) Persian(Farsi) (FA) Galician (GL) Hungarian (HU) Japanese (JA)
                                                           // Korean (KR) Latvian (LA) Lithuanian (LT) Macedonian (MK) Slovak (SK) Slovenian (SL) Vietnamese (VI)
String Hemisphere       = "north";                         // or "south"
String Units            = "M";                             // Use 'M' for Metric or I for Imperial

const char* Timezone    = "CST6";    // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char* ntpServer   = "time.cloudflare.com";                  // Choose a time server close to you, or use pool.ntp.org to find an NTP server
int  gmtOffset_sec      = -21600;                               // UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
int  daylightOffset_sec = 0;                            // In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset
