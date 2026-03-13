#ifndef LANG_H
#define LANG_H

#define FONT(x) x##_tf

// Language codes
#define LANG_ES 0  // Spanish
#define LANG_EN 1  // English
#define LANG_FR 2  // French

// Current language (default Spanish)
int currentLang = LANG_ES;

// Set language based on config string
void setLanguage(const char* lang) {
  if (strcmp(lang, "EN") == 0 || strcmp(lang, "en") == 0) {
    currentLang = LANG_EN;
  } else if (strcmp(lang, "FR") == 0 || strcmp(lang, "fr") == 0) {
    currentLang = LANG_FR;
  } else {
    currentLang = LANG_ES;  // Default to Spanish
  }
}

// ============== TEMPERATURE / HUMIDITY ==============
const char* TXT_TEMPERATURE_C_ARR[] = { "Temperatura (C)", "Temperature (C)", "Temperature (C)" };
const char* TXT_TEMPERATURE_F_ARR[] = { "Temperatura (F)", "Temperature (F)", "Temperature (F)" };
const char* TXT_HUMIDITY_PERCENT_ARR[] = { "Humedad (%)", "Humidity (%)", "Humidite (%)" };

#define TXT_TEMPERATURE_C String(TXT_TEMPERATURE_C_ARR[currentLang])
#define TXT_TEMPERATURE_F String(TXT_TEMPERATURE_F_ARR[currentLang])
#define TXT_HUMIDITY_PERCENT String(TXT_HUMIDITY_PERCENT_ARR[currentLang])

// ============== PRESSURE ==============
const char* TXT_PRESSURE_HPA_ARR[] = { "Presion (hPa)", "Pressure (hPa)", "Pression (hPa)" };
const char* TXT_PRESSURE_IN_ARR[] = { "Presion (in)", "Pressure (in)", "Pression (in)" };
const char* TXT_PRESSURE_STEADY_ARR[] = { "Estable", "Steady", "Stable" };
const char* TXT_PRESSURE_RISING_ARR[] = { "Subiendo", "Rising", "En hausse" };
const char* TXT_PRESSURE_FALLING_ARR[] = { "Bajando", "Falling", "En baisse" };

#define TXT_PRESSURE_HPA String(TXT_PRESSURE_HPA_ARR[currentLang])
#define TXT_PRESSURE_IN String(TXT_PRESSURE_IN_ARR[currentLang])
#define TXT_PRESSURE_STEADY String(TXT_PRESSURE_STEADY_ARR[currentLang])
#define TXT_PRESSURE_RISING String(TXT_PRESSURE_RISING_ARR[currentLang])
#define TXT_PRESSURE_FALLING String(TXT_PRESSURE_FALLING_ARR[currentLang])

// ============== RAINFALL / SNOWFALL ==============
const char* TXT_RAINFALL_MM_ARR[] = { "Lluvia (mm)", "Rainfall (mm)", "Pluie (mm)" };
const char* TXT_RAINFALL_IN_ARR[] = { "Lluvia (in)", "Rainfall (in)", "Pluie (in)" };
const char* TXT_SNOWFALL_MM_ARR[] = { "Nevada (mm)", "Snowfall (mm)", "Neige (mm)" };
const char* TXT_SNOWFALL_IN_ARR[] = { "Nevada (in)", "Snowfall (in)", "Neige (in)" };
const char* TXT_PRECIPITATION_SOON_ARR[] = { "Prec.", "Prec.", "Prec." };

#define TXT_RAINFALL_MM String(TXT_RAINFALL_MM_ARR[currentLang])
#define TXT_RAINFALL_IN String(TXT_RAINFALL_IN_ARR[currentLang])
#define TXT_SNOWFALL_MM String(TXT_SNOWFALL_MM_ARR[currentLang])
#define TXT_SNOWFALL_IN String(TXT_SNOWFALL_IN_ARR[currentLang])
#define TXT_PRECIPITATION_SOON String(TXT_PRECIPITATION_SOON_ARR[currentLang])

// ============== SUN ==============
const char* TXT_SUNRISE_ARR[] = { "Amanecer", "Sunrise", "Lever" };
const char* TXT_SUNSET_ARR[] = { "Anochecer", "Sunset", "Coucher" };

#define TXT_SUNRISE String(TXT_SUNRISE_ARR[currentLang])
#define TXT_SUNSET String(TXT_SUNSET_ARR[currentLang])

// ============== MOON PHASES ==============
const char* TXT_MOON_NEW_ARR[] = { "Luna Nueva", "New Moon", "Nouvelle Lune" };
const char* TXT_MOON_WAXING_CRESCENT_ARR[] = { "Luna Creciente", "Waxing Crescent", "Premier Croissant" };
const char* TXT_MOON_FIRST_QUARTER_ARR[] = { "Cuarto Creciente", "First Quarter", "Premier Quartier" };
const char* TXT_MOON_WAXING_GIBBOUS_ARR[] = { "Gibosa Creciente", "Waxing Gibbous", "Gibbeuse Croissante" };
const char* TXT_MOON_FULL_ARR[] = { "Luna Llena", "Full Moon", "Pleine Lune" };
const char* TXT_MOON_WANING_GIBBOUS_ARR[] = { "Gibosa Menguante", "Waning Gibbous", "Gibbeuse Decroissante" };
const char* TXT_MOON_THIRD_QUARTER_ARR[] = { "Cuarto Menguante", "Third Quarter", "Dernier Quartier" };
const char* TXT_MOON_WANING_CRESCENT_ARR[] = { "Luna Menguante", "Waning Crescent", "Dernier Croissant" };

#define TXT_MOON_NEW String(TXT_MOON_NEW_ARR[currentLang])
#define TXT_MOON_WAXING_CRESCENT String(TXT_MOON_WAXING_CRESCENT_ARR[currentLang])
#define TXT_MOON_FIRST_QUARTER String(TXT_MOON_FIRST_QUARTER_ARR[currentLang])
#define TXT_MOON_WAXING_GIBBOUS String(TXT_MOON_WAXING_GIBBOUS_ARR[currentLang])
#define TXT_MOON_FULL String(TXT_MOON_FULL_ARR[currentLang])
#define TXT_MOON_WANING_GIBBOUS String(TXT_MOON_WANING_GIBBOUS_ARR[currentLang])
#define TXT_MOON_THIRD_QUARTER String(TXT_MOON_THIRD_QUARTER_ARR[currentLang])
#define TXT_MOON_WANING_CRESCENT String(TXT_MOON_WANING_CRESCENT_ARR[currentLang])

// ============== MISC ==============
const char* TXT_POWER_ARR[] = { "Energia", "Power", "Energie" };
const char* TXT_WIFI_ARR[] = { "WiFi", "WiFi", "WiFi" };
const char* TXT_UPDATED_ARR[] = { "Actualizado:", "Updated:", "Mis a jour:" };

#define TXT_POWER String(TXT_POWER_ARR[currentLang])
#define TXT_WIFI String(TXT_WIFI_ARR[currentLang])
const char* TXT_UPDATED = "Updated:";  // Serial output always in English

// ============== WIND DIRECTIONS ==============
// Wind directions are universal abbreviations
const String TXT_N   = "N";
const String TXT_NNE = "NNE";
const String TXT_NE  = "NE";
const String TXT_ENE = "ENE";
const String TXT_E   = "E";
const String TXT_ESE = "ESE";
const String TXT_SE  = "SE";
const String TXT_SSE = "SSE";
const String TXT_S   = "S";
const String TXT_SSW = "SSW";
const String TXT_SW  = "SW";
const String TXT_WSW = "WSW";
const String TXT_W   = "W";
const String TXT_WNW = "WNW";
const String TXT_NW  = "NW";
const String TXT_NNW = "NNW";

// ============== DAYS OF WEEK ==============
const char* weekday_ES[] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
const char* weekday_EN[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char* weekday_FR[] = { "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam" };

const char* getWeekday(int day) {
  switch (currentLang) {
    case LANG_EN: return weekday_EN[day];
    case LANG_FR: return weekday_FR[day];
    default: return weekday_ES[day];
  }
}

// For compatibility - pointer to current weekday array
const char** weekday_D = weekday_ES;

void updateWeekdayPointer() {
  switch (currentLang) {
    case LANG_EN: weekday_D = weekday_EN; break;
    case LANG_FR: weekday_D = weekday_FR; break;
    default: weekday_D = weekday_ES; break;
  }
}

// ============== MONTHS ==============
const char* month_ES[] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre" };
const char* month_EN[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
const char* month_FR[] = { "Jan", "Fev", "Mar", "Avr", "Mai", "Juin", "Juil", "Aout", "Sep", "Oct", "Nov", "Dec" };

const char* getMonth(int month) {
  switch (currentLang) {
    case LANG_EN: return month_EN[month];
    case LANG_FR: return month_FR[month];
    default: return month_ES[month];
  }
}

// For compatibility
const char** month_M = month_ES;

void updateMonthPointer() {
  switch (currentLang) {
    case LANG_EN: month_M = month_EN; break;
    case LANG_FR: month_M = month_FR; break;
    default: month_M = month_ES; break;
  }
}

// ============== NAVIGATION SCREENS ==============
const char* TXT_BACK_ARR[] = { "Regresar", "Back", "Retour" };
const char* TXT_CURRENT_CONDITIONS_ARR[] = { "Condiciones Actuales", "Current Conditions", "Conditions Actuelles" };
const char* TXT_5DAY_FORECAST_ARR[] = { "Pronostico", "Forecast", "Previsions" };
const char* TXT_WEATHER_TRENDS_ARR[] = { "Historial del Clima", "Weather History", "Historique Meteo" };
const char* TXT_NEXT_24H_ARR[] = { "Proximas 24 Horas", "Next 24 Hours", "Prochaines 24 Heures" };
const char* TXT_5DAY_OUTLOOK_ARR[] = { "Proximos 5 Dias", "Next 5 Days", "Prochains 5 Jours" };
const char* TXT_SYSTEM_INFO_ARR[] = { "Informacion del Sistema", "System Information", "Informations Systeme" };

#define TXT_BACK String(TXT_BACK_ARR[currentLang])
#define TXT_CURRENT_CONDITIONS String(TXT_CURRENT_CONDITIONS_ARR[currentLang])
#define TXT_5DAY_FORECAST String(TXT_5DAY_FORECAST_ARR[currentLang])
#define TXT_WEATHER_TRENDS String(TXT_WEATHER_TRENDS_ARR[currentLang])
#define TXT_NEXT_24H String(TXT_NEXT_24H_ARR[currentLang])
#define TXT_5DAY_OUTLOOK String(TXT_5DAY_OUTLOOK_ARR[currentLang])
#define TXT_SYSTEM_INFO String(TXT_SYSTEM_INFO_ARR[currentLang])

// ============== DISPLAY LABELS ==============
const char* TXT_HUMIDITY_ARR[] = { "Humedad", "Humidity", "Humidite" };
const char* TXT_PRESSURE_ARR[] = { "Presion", "Pressure", "Pression" };
const char* TXT_VISIBILITY_ARR[] = { "Visibilidad", "Visibility", "Visibilite" };
const char* TXT_CLOUDINESS_ARR[] = { "Nubosidad", "Cloudiness", "Nebulosite" };
const char* TXT_WIND_ARR[] = { "Viento", "Wind", "Vent" };
const char* TXT_WIND_DIR_ARR[] = { "Direccion Viento", "Wind Direction", "Direction Vent" };
const char* TXT_RAIN_PROB_ARR[] = { "Prob. Lluvia", "Rain Prob.", "Prob. Pluie" };
const char* TXT_FEELS_LIKE_ARR[] = { "Sensacion termica", "Feels like", "Ressenti" };

#define TXT_HUMIDITY String(TXT_HUMIDITY_ARR[currentLang])
#define TXT_PRESSURE String(TXT_PRESSURE_ARR[currentLang])
#define TXT_VISIBILITY String(TXT_VISIBILITY_ARR[currentLang])
#define TXT_CLOUDINESS String(TXT_CLOUDINESS_ARR[currentLang])
#define TXT_WIND String(TXT_WIND_ARR[currentLang])
#define TXT_WIND_DIR String(TXT_WIND_DIR_ARR[currentLang])
#define TXT_RAIN_PROB String(TXT_RAIN_PROB_ARR[currentLang])
#define TXT_FEELS_LIKE String(TXT_FEELS_LIKE_ARR[currentLang])

// ============== INITIALIZE LANGUAGE ==============
void initLanguage(const char* lang) {
  setLanguage(lang);
  updateWeekdayPointer();
  updateMonthPointer();
}

#endif // LANG_H
