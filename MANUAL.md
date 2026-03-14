# Manual Estacion Meteorologica LilyGo EPD 4.7"

## Tabla de Contenidos

1. [Introduccion](#1-introduccion)
2. [Especificaciones Tecnicas](#2-especificaciones-tecnicas)
3. [Arquitectura del Sistema](#3-arquitectura-del-sistema)
4. [Instalacion y Compilacion](#4-instalacion-y-compilacion)
5. [Configuracion](#5-configuracion)
6. [Uso del Dispositivo](#6-uso-del-dispositivo)
7. [Pantallas de Navegacion](#7-pantallas-de-navegacion)
8. [API OpenWeatherMap](#8-api-openweathermap)
9. [Gestion de Energia](#9-gestion-de-energia)
10. [Solucion de Problemas](#10-solucion-de-problemas)
11. [Apendice](#11-apendice)

---

## 1. Introduccion

### 1.1 Descripcion General

La Estacion Meteorologica LilyGo EPD 4.7" es un dispositivo basado en ESP32-S3 que muestra informacion meteorologica en tiempo real obtenida de OpenWeatherMap. Utiliza una pantalla e-paper (tinta electronica) de 4.7 pulgadas que ofrece excelente visibilidad bajo cualquier condicion de luz y bajo consumo de energia.

### 1.2 Caracteristicas Principales

- **Pantalla e-paper de 4.7 pulgadas** - 960x540 pixeles, escala de grises
- **Navegacion tactil** - Controlador GT911 para interaccion con el usuario
- **Multi-WiFi** - Soporte para hasta 3 redes WiFi configurables
- **Modo AP (Access Point)** - Portal cautivo para configuracion sin necesidad de programacion
- **Deep Sleep** - Bajo consumo para operacion con bateria
- **Multi-idioma** - Espanol, Ingles y Frances
- **Actualizacion automatica** - Intervalo configurable de actualizacion de datos

---

## 2. Especificaciones Tecnicas

### 2.1 Hardware

#### Microcontrolador
| Parametro | Especificacion |
|-----------|----------------|
| Chip | ESP32-S3 |
| CPU | Dual-core Xtensa LX7 @ 240MHz |
| RAM | 512KB SRAM + 8MB PSRAM (OPI) |
| Flash | 16MB |
| WiFi | 802.11 b/g/n (2.4GHz) |

#### Pantalla E-Paper
| Parametro | Especificacion |
|-----------|----------------|
| Tipo | E-Ink (tinta electronica) |
| Tamano | 4.7 pulgadas diagonal |
| Resolucion | 960 x 540 pixeles |
| Colores | 16 niveles de gris |
| Tecnologia | ED047TC1 |
| Controlador | IT8951 |
| Tiempo de refresco | ~0.5 segundos |
| Angulo de vision | ~180 grados |
| Consumo en standby | ~0 mW (imagen estatica) |

#### Panel Tactil
| Parametro | Especificacion |
|-----------|----------------|
| Controlador | GT911 |
| Interfaz | I2C |
| Pines | SDA=18, SCL=17, INT=47 |
| Direccion I2C | 0x5D o 0x14 |
| Puntos tactiles | Hasta 5 simultaneos |

#### Alimentacion
| Parametro | Especificacion |
|-----------|----------------|
| Voltaje entrada USB | 5V |
| Voltaje bateria | 3.7V LiPo (3.2V-4.2V) |
| Consumo activo | ~150mA |
| Consumo deep sleep | ~10uA |
| Pin ADC bateria | GPIO 14 |

### 2.2 Principios de la Tecnologia E-Paper

#### Como Funciona

La pantalla e-paper utiliza **microesferas bicolores** suspendidas en un fluido:

```
+------------------+
|  @@  @@  @@  @@ |  <-- Particulas blancas (cargadas +)
|  oo  oo  oo  oo |  <-- Particulas negras (cargadas -)
|==================|  <-- Electrodo superior (transparente)
|  Fluido          |
|==================|  <-- Electrodo inferior
+------------------+
```

- **Voltaje positivo**: Las particulas blancas suben, se ve blanco
- **Voltaje negativo**: Las particulas negras suben, se ve negro
- **Sin voltaje**: La imagen se mantiene indefinidamente (memoria biestable)

#### Ventajas
1. **Visibilidad** - Perfecta bajo luz solar directa
2. **Angulo de vision** - Casi 180 grados
3. **Consumo** - Solo consume energia al cambiar la imagen
4. **Confort visual** - Sin retroiluminacion, no cansa la vista

#### Limitaciones
1. **Velocidad** - Refresco mas lento que LCD (~0.5s)
2. **Ghosting** - Pueden quedar imagenes residuales
3. **Color** - Solo escala de grises (sin color)
4. **Temperatura** - Funcionamiento optimo 0-50C

### 2.3 Escala de Grises

El display soporta 16 niveles de gris definidos en el codigo:

```cpp
#define White      0xFF   // Blanco puro
#define LightGrey  0xBB   // Gris claro
#define Grey       0x88   // Gris medio
#define DarkGrey   0x44   // Gris oscuro
#define Black      0x00   // Negro puro
```

---

## 3. Arquitectura del Sistema

### 3.1 Diagrama de Flujo Principal

```
                    +----------------+
                    |    INICIO      |
                    +----------------+
                           |
                           v
                    +----------------+
                    | InitialiseSystem|
                    | - Serial.begin  |
                    | - epd_init      |
                    | - framebuffer   |
                    +----------------+
                           |
                           v
                    +----------------+
                    | InitializeTouch |
                    | - GT911 I2C     |
                    +----------------+
                           |
                           v
                    +----------------+
                    | loadConfig()    |
                    | - Preferences   |
                    | - applyConfig   |
                    +----------------+
                           |
                           v
                    +----------------+
                    | FORCE_AP_MODE? |----Si----> Modo AP
                    +----------------+             |
                           |No                     |
                           v                       |
                    +----------------+             |
                    | StartWiFi()    |             |
                    | - Scan redes   |             |
                    | - Mejor senal  |             |
                    +----------------+             |
                           |                       |
                      Conectado?                   |
                      /        \                   |
                    Si          No-----------------+
                    |                              |
                    v                              v
              +----------------+           +----------------+
              | SetupTime()    |           | startAPMode()  |
              | - NTP sync     |           | - DNS server   |
              +----------------+           | - Web server   |
                    |                      +----------------+
                    v                              |
              +----------------+                   |
              | obtainWeather  |                   |
              | - API weather  |                   |
              | - API forecast |                   |
              +----------------+                   |
                    |                              |
                    v                              |
              +----------------+                   |
              | DisplayWeather |                   |
              | - Render       |                   |
              +----------------+                   |
                    |                              |
                    v                              v
              +----------------+           +----------------+
              |   LOOP()       |           |   LOOP()       |
              | - Touch nav    |           | - handleAPMode |
              | - Web server   |           | - Retry WiFi   |
              | - 30s timeout  |           |   cada 60s     |
              +----------------+           +----------------+
                    |
                    v (timeout)
              +----------------+
              | BeginSleep()   |
              | - deep_sleep   |
              +----------------+
```

### 3.2 Estructura de Archivos

```
LilyGo-EPD-4-7-OWM-Weather-Display/
|
+-- LilyGo-EPD-4-7-OWM-Weather-Display.ino  # Sketch principal
|
+-- owm_credentials.h     # Credenciales WiFi y API (valores por defecto)
|
+-- wifi_manager.h        # Modo AP, portal web, almacenamiento NVS
|
+-- forecast_record.h     # Estructura de datos meteorologicos
|
+-- lang.h                # Sistema multi-idioma (ES/EN/FR)
|
+-- touch_handler.h       # Controlador tactil GT911
|
+-- opensans*.h           # Fuentes (8B, 9B, 10B, 12B, 14B, 16B, 18B, 24B)
|
+-- moon.h                # Imagen de la luna
+-- sunrise.h             # Icono amanecer
+-- sunset.h              # Icono anochecer
```

### 3.3 Estructura de Datos Meteorologicos

```cpp
typedef struct {
  int      Dt;           // Unix timestamp
  String   Period;       // Periodo de tiempo
  String   Icon;         // Codigo icono (01d, 02n, etc.)
  String   Trend;        // Tendencia presion (+, -, 0)
  String   Main0;        // Condicion principal
  String   Forecast0;    // Descripcion del clima
  String   Description;  // Descripcion detallada
  float    Temperature;  // Temperatura actual
  float    Feelslike;    // Sensacion termica
  float    Humidity;     // Humedad %
  float    High;         // Temperatura maxima
  float    Low;          // Temperatura minima
  float    Winddir;      // Direccion del viento (grados)
  float    Windspeed;    // Velocidad del viento
  float    Rainfall;     // Lluvia mm
  float    Snowfall;     // Nieve mm
  float    Pop;          // Probabilidad de precipitacion
  float    Pressure;     // Presion atmosferica hPa
  int      Cloudcover;   // Cobertura de nubes %
  int      Visibility;   // Visibilidad metros
  int      Sunrise;      // Unix timestamp amanecer
  int      Sunset;       // Unix timestamp anochecer
  int      Timezone;     // Offset zona horaria
} Forecast_record_type;
```

### 3.4 Almacenamiento de Configuracion (NVS)

La configuracion se almacena en el namespace "weather" de ESP32 Preferences:

| Clave | Tipo | Descripcion |
|-------|------|-------------|
| ssid1, pass1 | String | Red WiFi principal |
| ssid2, pass2 | String | Red WiFi secundaria |
| ssid3, pass3 | String | Red WiFi terciaria |
| apikey | String | API Key de OpenWeatherMap |
| fcdays | Int | Dias de pronostico (3 o 5) |
| city | String | Nombre de la ciudad |
| lat, lon | String | Coordenadas geograficas |
| lang | String | Idioma (ES, EN, FR) |
| hemi | String | Hemisferio (north, south) |
| units | String | Unidades (M=metrico, I=imperial) |
| tz | String | Zona horaria POSIX |
| gmt | Int | Offset GMT en segundos |
| dst | Int | Offset horario de verano |
| updint | Int | Intervalo actualizacion (min) |
| sleept | Int | Tiempo antes de sleep (seg) |

---

## 4. Instalacion y Compilacion

### 4.1 Requisitos de Software

#### Arduino IDE
- Version 1.8.x o 2.x

#### Board Manager
- **URL**: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- **Paquete**: esp32 by Espressif Systems **version 2.0.17**

#### Librerias Requeridas
Solo instalar estas dos librerias en la carpeta `libraries`:

1. **EPD47-master**
   - URL: https://github.com/DFRobotdl/EPD47/archive/refs/heads/master.zip
   - Descomprimir como `EPD47-master`

2. **ArduinoJson**
   - Autor: Benoit Blanchon
   - Version: 6.19.0

> **IMPORTANTE**: No instalar otras librerias para evitar conflictos.

### 4.2 Configuracion del Arduino IDE

En `Tools` (Herramientas), configurar:

| Parametro | Valor |
|-----------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enable |
| USB DFU On Boot | Disable |
| Flash Size | 16MB (128Mb) |
| Flash Mode | QIO 80MHz |
| Partition Scheme | 16M Flash (3M APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0/Hardware CDC |
| USB Mode | Hardware CDC and JTAG |

### 4.3 Carga del Firmware

1. Conectar el dispositivo por USB
2. Seleccionar el puerto COM correcto
3. Click en Upload (Subir)

#### Modo Bootloader Forzado

Si la carga falla, entrar en modo bootloader:

1. **Presionar y mantener** el boton BOOT (IO0)
2. **Mientras se mantiene** BOOT, presionar RST
3. **Soltar** RST
4. **Soltar** BOOT
5. Intentar cargar nuevamente

---

## 5. Configuracion

### 5.1 Configuracion por Archivo (owm_credentials.h)

Valores por defecto que pueden ser sobrescritos via web:

```cpp
// Redes WiFi (hasta 3)
const WiFiCredentials wifiNetworks[] = {
  {"MiRedPrincipal", "password123"},
  {"RedSecundaria", "otrapassword"},
  {"RedTerciaria", "tercerpass"},
};

// API OpenWeatherMap
String apikey = "tu_api_key_aqui";

// Ubicacion
String City      = "CDMX";
String Latitude  = "19.4326";
String Longitude = "-99.1332";

// Preferencias
String Language   = "ES";        // ES, EN, FR
String Hemisphere = "north";     // north, south
String Units      = "M";         // M=metrico, I=imperial

// Zona horaria
const char* Timezone    = "CST6";
int gmtOffset_sec       = -21600;  // -6 horas
int daylightOffset_sec  = 0;       // Sin horario de verano
```

### 5.2 Configuracion por Portal Web (Modo AP)

#### Acceso al Modo AP

El modo AP se activa automaticamente cuando:
- No hay redes WiFi configuradas disponibles
- Se configura `FORCE_AP_MODE = true` en el codigo

#### Datos de Conexion
| Parametro | Valor |
|-----------|-------|
| SSID | WeatherStation-Setup |
| Password | weather123 |
| URL | http://192.168.4.1 |

#### Pantalla Modo AP

Cuando se activa el modo AP, la pantalla muestra:

```
    WiFi Setup Mode

    Connect to WiFi network:
    WeatherStation-Setup

    Password: weather123

    Then open in browser:
    http://192.168.4.1
```

### 5.3 Pagina de Configuracion Web

La pagina web esta organizada en 4 pestanas:

#### Tab 1: WiFi
- **Red Principal**: SSID y contrasena
- **Red Secundaria**: SSID y contrasena (opcional)
- **Red Terciaria**: SSID y contrasena (opcional)

#### Tab 2: Clima
- **API Key**: Clave de OpenWeatherMap
- **Dias de Pronostico**: 3 o 5 dias
- **Ciudad**: Nombre para mostrar
- **Latitud/Longitud**: Coordenadas exactas
- **Hemisferio**: Norte o Sur (afecta fases lunares)

#### Tab 3: Display
- **Idioma**: Espanol, English, Francais
- **Unidades**: Metrico (C, m/s, hPa) o Imperial (F, mph, inHg)
- **Timezone**: Zona horaria POSIX (ej: CST6)
- **GMT Offset**: Offset en segundos
- **DST Offset**: Horario de verano en segundos

#### Tab 4: Sistema
- **Intervalo de Actualizacion**: 5-120 minutos
- **Tiempo antes de Sleep**: 10-300 segundos
- **Guardar**: Solo guarda cambios
- **Guardar y Reiniciar**: Guarda y aplica cambios
- **Restablecer Fabrica**: Borra toda la configuracion

### 5.4 Zonas Horarias Comunes

| Ciudad | Timezone | GMT Offset |
|--------|----------|------------|
| Mexico City | CST6 | -21600 |
| New York | EST5EDT | -18000 |
| Los Angeles | PST8PDT | -28800 |
| Madrid | CET-1CEST | 3600 |
| London | GMT0BST | 0 |
| Tokyo | JST-9 | 32400 |
| Sydney | AEST-10AEDT | 36000 |

---

## 6. Uso del Dispositivo

### 6.1 Inicio Normal

1. **Encendido** - El dispositivo inicia automaticamente
2. **Conexion WiFi** - Busca redes configuradas
3. **Sincronizacion** - Obtiene hora via NTP
4. **Datos clima** - Descarga de OpenWeatherMap
5. **Visualizacion** - Muestra la pantalla principal
6. **Navegacion** - 30 segundos para interactuar
7. **Sleep** - Entra en modo de bajo consumo

### 6.2 Ciclo de Actualizacion

```
[WAKE UP] --> [WiFi] --> [NTP] --> [API] --> [Display] --> [Sleep]
    ^                                                          |
    |                                                          |
    +------ (intervalo configurado, ej: 30 minutos) -----------+
```

### 6.3 Indicadores en Pantalla

#### Barra de Estado (esquina superior derecha)
- **Barras WiFi**: Intensidad de senal (1-5 barras)
- **dB**: Valor RSSI en decibeles
- **Bateria**: Porcentaje y voltaje

#### Colores/Tonos
- **Negro**: Informacion principal
- **Gris oscuro**: Valores secundarios
- **Gris claro**: Lineas divisoras, graficas
- **Blanco**: Fondo

---

## 7. Pantallas de Navegacion

### 7.1 Sistema de Navegacion Tactil

El dispositivo tiene 5 pantallas navegables tocando diferentes zonas:

```
Pantalla Principal (SCREEN_MAIN)
+--------------------------------+
| [Ciudad]        [Bat%][WiFi]  |  <-- Zona Bateria/WiFi (esquina sup-der)
|                                |      -> Pantalla 4: Informacion Sistema
+--------------------------------+
|                                |  <-- Zona superior (40%)
|    Toca aqui para             |      -> Pantalla 1: Detalles
|    Condiciones Actuales        |
|                                |
+--------------------------------+
|                                |  <-- Zona media (30%)
|    Toca aqui para             |      -> Pantalla 2: Pronostico
|    Pronostico Extendido        |
|                                |
+--------------------------------+
|    [Graficas]                  |  <-- Zona inferior (30%)
|    Toca aqui para             |      -> Pantalla 3: Graficas
|    Graficas Expandidas         |
+--------------------------------+
```

**Regresar**: Tocar cualquier parte de una pantalla secundaria regresa a la principal.

### 7.2 Pantalla Principal (Main)

```
+--[Ciudad]--------------------[Bateria][WiFi]--+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|                                               |
| [Rosa de Vientos]  [Temperatura] [Humedad]    |
|     [Direccion]    [Max/Min]                  |
|     [Velocidad]    [Descripcion clima]        |
|                    [Presion] [Visib] [Nubes]  |
|                                               |
| [Luna]  [Salida sol]              [ICONO]     |
| [Fase]  [Puesta sol]              [Grande]    |
|                                               |
+-----------------------------------------------+
| [+3h] [+6h] [+9h] [+12h] [+15h] [+18h] [+21h] |
|  Pronostico por horas con iconos              |
+-----------------------------------------------+
| [Presion]  [Temperatura]  [Humedad]  [Lluvia] |
|    Graficas de tendencias (3 dias)            |
+-----------------------------------------------+
```

### 7.3 Pantalla 1: Condiciones Actuales

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|           CONDICIONES ACTUALES                |
+-----------------------------------------------+
|                                               |
|   [Temp]        [Presion]       [Humedad]     |
|   25.5 C        1013 hPa        65%           |
|   Max 28/Min 22                               |
|                                               |
+-----------------------------------------------+
|                                               |
|  [ICONO GRANDE]    Visibilidad: 10.5 km       |
|                    Dir. Viento: NNE 5.2 m/s   |
|  "Parcialmente     Nubosidad: 45%             |
|   nublado"                                    |
|                                               |
+-----------------------------------------------+
|   [Amanecer]      [Anochecer]    [Prob.Lluvia]|
|     06:45           18:32           15%       |
+-----------------------------------------------+
```

### 7.4 Pantalla 2: Pronostico Extendido

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|              PRONOSTICO                       |
+-----------------------------------------------+
|              Proximas 24 Horas                |
| [12:00] [15:00] [18:00] [21:00] [00:00] ...   |
|  [ico]   [ico]   [ico]   [ico]   [ico]        |
|  28/24   27/23   25/22   24/21   23/20        |
+-----------------------------------------------+
|              Pronostico 5 Dias                |
|  [Lun]    [Mar]    [Mie]    [Jue]    [Vie]    |
|  [ico]    [ico]    [ico]    [ico]    [ico]    |
|  30/22    28/21    27/20    29/22    31/23    |
|  2.5mm            0.5mm                        |
+-----------------------------------------------+
```

### 7.5 Pantalla 3: Graficas de Pronostico

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|           PRONOSTICO DEL CLIMA                |
+-----------------------------------------------+
|                                               |
|  [Grafica Temperatura]   [Grafica Presion]    |
|  ~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~ |
|  |    ____/\____        ||    ___/\___       ||
|  |___/          \____   ||___/        \___   ||
|  |0d    1d    2d   3d   ||0d    1d   2d  3d  ||
|                                               |
|  [Grafica Humedad]       [Grafica Lluvia]     |
|  ~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~ |
|  |   /\    /\           || ||  |  ||         ||
|  |__/  \__/  \____      || ||  |  ||  |      ||
|  |0d    1d    2d   3d   ||0d    1d   2d  3d  ||
|                                               |
+-----------------------------------------------+
```

### 7.6 Pantalla 4: Informacion del Sistema

**Acceso**: Tocar la esquina superior derecha donde se muestra la bateria y senal WiFi.

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|           INFORMACION DEL SISTEMA             |
+-----------------------------------------------+
|                                               |
|   Firmware:        v2.0                       |
|   Compilado:       Mar 13 2026                |
|                                               |
|   WiFi SSID:       MiRedWiFi                  |
|   WiFi IP:         192.168.1.100              |
|   Senal:           -65 dBm                    |
|                                               |
|   Bateria:         85% (4.1v)                 |
|                                               |
|   Ubicacion:       Ciudad, Pais               |
|   Coordenadas:     19.4326, -99.1332          |
|   Zona horaria:    UTC-6                      |
|                                               |
|   Proxima actualizacion: 30 min               |
|                                               |
+-----------------------------------------------+
```

Esta pantalla muestra informacion tecnica del dispositivo util para diagnostico y verificacion de configuracion.

### 7.7 Timeout y Sleep

- **30 segundos** sin tocar -> Regresa a pantalla principal y entra en deep sleep
- **5 minutos** maximo de navegacion continua como proteccion

---

## 8. API OpenWeatherMap

### 8.1 Obtencion de API Key

1. Ir a https://openweathermap.org/
2. Crear cuenta gratuita
3. Ir a "API Keys" en el perfil
4. Copiar o generar nueva clave

### 8.2 Limites del Plan Gratuito

| Caracteristica | Limite |
|---------------|--------|
| Llamadas/minuto | 60 |
| Llamadas/mes | 1,000,000 |
| Datos historicos | No |
| Pronostico | 5 dias / 3 horas |

### 8.3 Endpoints Utilizados

#### Clima Actual
```
GET /data/2.5/weather
Parametros:
  lat={latitud}
  lon={longitud}
  appid={api_key}
  units=metric|imperial
  lang={codigo_idioma}
```

#### Pronostico
```
GET /data/2.5/forecast
Parametros:
  lat={latitud}
  lon={longitud}
  appid={api_key}
  units=metric|imperial
  lang={codigo_idioma}
  cnt=40  (8 lecturas/dia x 5 dias)
```

### 8.4 Codigos de Iconos

| Codigo | Dia | Noche | Descripcion |
|--------|-----|-------|-------------|
| 01d | 01n | Sol/Luna | Despejado |
| 02d | 02n | Nubes | Algunas nubes |
| 03d | 03n | Nubes | Nubes dispersas |
| 04d | 04n | Nubes | Muy nublado |
| 09d | 09n | Lluvia | Lluvia ligera |
| 10d | 10n | Lluvia | Lluvia |
| 11d | 11n | Rayo | Tormenta |
| 13d | 13n | Nieve | Nieve |
| 50d | 50n | Niebla | Neblina/Niebla |

---

## 9. Gestion de Energia

### 9.1 Modos de Operacion

| Modo | Consumo | Descripcion |
|------|---------|-------------|
| Activo | ~150mA | WiFi + Display + CPU |
| Display Off | ~80mA | WiFi + CPU |
| Light Sleep | ~2mA | CPU pausado |
| Deep Sleep | ~10uA | Solo RTC |

### 9.2 Ciclo de Energia Tipico

```
         150mA        0mA (e-paper)      10uA
           |            |                  |
[Activo]---+--[Display]+---[Deep Sleep]----+
  ~15s        ~0.5s       ~30 minutos

Promedio: ~0.1mA (con bateria de 2000mAh = ~20,000 horas)
```

### 9.3 Configuracion de Sleep

En el codigo:
```cpp
long SleepDuration = 10;  // Minutos entre actualizaciones
int  WakeupHour    = 8;   // No despertar antes de las 8am
int  SleepHour     = 1;   // Dormir despues de la 1am
```

### 9.4 Monitoreo de Bateria

- **Pin ADC**: GPIO 14
- **Formula**: `voltage = analogRead(14) / 4096.0 * 6.566 * (vref / 1000.0)`
- **Rango**: 3.2V (0%) a 4.2V (100%)

---

## 10. Solucion de Problemas

### 10.1 No Se Conecta a WiFi

**Sintomas**: Entra en modo AP cada vez

**Soluciones**:
1. Verificar que el SSID esta escrito exactamente igual
2. Verificar la contrasena
3. Asegurar que la red es 2.4GHz (no 5GHz)
4. Mover el dispositivo mas cerca del router

### 10.2 No Muestra Datos del Clima

**Sintomas**: Pantalla en blanco o con "?"

**Soluciones**:
1. Verificar API Key de OpenWeatherMap
2. Verificar coordenadas de ubicacion
3. Verificar conexion a internet
4. Revisar Serial Monitor para errores

### 10.3 Pantalla con Ghosting

**Sintomas**: Imagenes anteriores visibles

**Soluciones**:
1. El proximo refresco completo lo limpiara
2. Reiniciar el dispositivo para forzar limpieza

### 10.4 Touch No Responde

**Sintomas**: No navega entre pantallas

**Soluciones**:
1. Verificar en Serial que GT911 fue detectado
2. Limpiar la pantalla de polvo/grasa
3. Tocar con firmeza por al menos 50ms
4. Esperar 500ms entre toques (debounce)

### 10.5 No Carga el Firmware

**Sintomas**: Error de conexion en Arduino IDE

**Soluciones**:
1. Usar modo bootloader forzado (seccion 4.3)
2. Probar otro cable USB
3. Verificar drivers USB instalados
4. Cerrar Serial Monitor antes de cargar

### 10.6 Consumo de Bateria Alto

**Sintomas**: Bateria se agota rapido

**Soluciones**:
1. Aumentar intervalo de actualizacion
2. Verificar que entra en deep sleep
3. Revisar que no hay WiFi activo durante sleep

---

## 11. Apendice

### 11.1 Fuentes Disponibles

| Nombre | Tamano | Uso |
|--------|--------|-----|
| OpenSans8B | 8pt | Texto pequeno, hints |
| OpenSans9B | 9pt | Etiquetas graficas |
| OpenSans10B | 10pt | Fecha/hora, detalles |
| OpenSans12B | 12pt | Textos medios |
| OpenSans14B | 14pt | Ciudad, descripciones |
| OpenSans18B | 18pt | Titulos secundarios |
| OpenSans24B | 24pt | Temperatura, datos principales |

### 11.2 Coordenadas de Pantalla

```
(0,0)--------------------------(960,0)
  |                               |
  |        960 x 540 pixeles      |
  |                               |
(0,540)-----------------------(960,540)
```

### 11.3 Direcciones del Viento

| Grados | Direccion |
|--------|-----------|
| 0 / 360 | N (Norte) |
| 45 | NE (Noreste) |
| 90 | E (Este) |
| 135 | SE (Sureste) |
| 180 | S (Sur) |
| 225 | SW (Suroeste) |
| 270 | W (Oeste) |
| 315 | NW (Noroeste) |

### 11.4 Fases Lunares

| Fase | Iluminacion | Nombre ES | Nombre EN |
|------|-------------|-----------|-----------|
| 0 | 0% | Luna Nueva | New Moon |
| 1 | 25% | Luna Creciente | Waxing Crescent |
| 2 | 50% | Cuarto Creciente | First Quarter |
| 3 | 75% | Gibosa Creciente | Waxing Gibbous |
| 4 | 100% | Luna Llena | Full Moon |
| 5 | 75% | Gibosa Menguante | Waning Gibbous |
| 6 | 50% | Cuarto Menguante | Third Quarter |
| 7 | 25% | Luna Menguante | Waning Crescent |

### 11.5 Textos Multi-idioma

El sistema soporta 3 idiomas configurables:

| Texto | Espanol | English | Francais |
|-------|---------|---------|----------|
| Condiciones | Condiciones Actuales | Current Conditions | Conditions Actuelles |
| Pronostico | Pronostico | Forecast | Previsions |
| Graficas | Pronostico del Clima | Weather Forecast | Previsions Meteo |
| Amanecer | Amanecer | Sunrise | Lever |
| Anochecer | Anochecer | Sunset | Coucher |
| Humedad | Humedad | Humidity | Humidite |
| Presion | Presion | Pressure | Pression |
| Viento | Viento | Wind | Vent |

### 11.6 Licencia y Creditos

**Software Original**: David Bird 2021
**Modificaciones**: XE1E 2024

Este proyecto es de codigo abierto. Consultar el repositorio para detalles de licencia.

---

*Manual Version 1.0 - Marzo 2026*
