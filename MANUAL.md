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
10. [Carcasa Impresa en 3D](#10-carcasa-impresa-en-3d)
11. [Solucion de Problemas](#11-solucion-de-problemas)
12. [Funciones Especiales](#12-funciones-especiales)
    - [12.1 Calendario](#121-calendario)
    - [12.2 Tarjeta SD](#122-tarjeta-sd)
    - [12.3 Acceso Web](#123-acceso-web)
    - [12.4 Configuracion Bluetooth](#124-configuracion-bluetooth)
13. [Apendice](#13-apendice)

---

## 1. Introduccion

### 1.1 Descripcion General

La Estacion Meteorologica LilyGo EPD 4.7" es un dispositivo basado en ESP32-S3 que muestra informacion meteorologica en tiempo real obtenida de OpenWeatherMap. Utiliza una pantalla e-paper (tinta electronica) de 4.7 pulgadas que ofrece excelente visibilidad bajo cualquier condicion de luz y bajo consumo de energia.

### 1.2 Caracteristicas Principales

- **Pantalla e-paper de 4.7 pulgadas** - 960x540 pixeles, escala de grises
- **Navegacion tactil** - Controlador GT911 con 11 pantallas navegables
- **Multi-WiFi** - Soporte para hasta 3 redes WiFi configurables
- **Modo AP (Access Point)** - Portal cautivo para configuracion inicial
- **Servidor web integrado** - Configuracion desde cualquier navegador en la red local
- **Configuracion Bluetooth** - App Android para configurar via BLE
- **Deep Sleep** - Bajo consumo para operacion con bateria
- **Multi-idioma** - Espanol, Ingles y Frances
- **Actualizacion automatica** - Intervalo configurable (5-120 minutos)
- **Historial de datos** - Almacena lecturas reales para ver tendencias (SD: ~1 año, interno: ~7 dias)

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

#### Botones
| Parametro | Especificacion |
|-----------|----------------|
| Boton BOOT | GPIO 0 (modo bootloader) |
| Boton RST | Reset hardware |

#### Lector SD Card
| Parametro | Especificacion |
|-----------|----------------|
| Interfaz | SPI |
| CS (Chip Select) | GPIO 42 |
| MOSI | GPIO 15 |
| MISO | GPIO 16 |
| CLK | GPIO 11 |
| Formatos soportados | FAT32, exFAT |
| Tamano maximo | Sin limite (probado hasta 64GB) |

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
+-- weather_history.h     # Sistema de historial de datos (SD + FFat + PSRAM)
|
+-- opensans*.h           # Fuentes (6, 8B, 9B, 10B, 12B, 14B, 16B, 18B, 24B, 28B)
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
| keepscr | Bool | Mantener pantalla actual al dormir |
| wakeuph | Int | Hora de inicio de actividad (0-23) |
| sleeph | Int | Hora de fin de actividad (0-23) |

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

// API Groq (para Clima Narrativo - gratis en console.groq.com)
String groq_apikey = "tu_groq_api_key_aqui";

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
- **Unidades**: Metrico (C, m/s, mb) o Imperial (F, mph, inHg)
- **Timezone**: Zona horaria POSIX (ej: CST6)
- **GMT Offset**: Offset en segundos
- **DST Offset**: Horario de verano en segundos

#### Tab 4: Sistema
- **Intervalo de Actualizacion**: 5-120 minutos
- **Tiempo antes de Sleep**: 10-300 segundos
- **Al dormir**: Regresar a pantalla principal o mantener pantalla actual
- **Hora de Inicio (despertar)**: Hora a partir de la cual actualiza el clima (0-23)
- **Hora de Fin (dormir)**: Hora a partir de la cual deja de actualizar (0-23)
- **Estilo de Narrativa**: Estilo del texto generado por IA (ver seccion 12.5.5)
- **Guardar**: Solo guarda cambios
- **Guardar y Reiniciar**: Guarda y aplica cambios
- **Restablecer Fabrica**: Borra toda la configuracion

#### Comportamiento "Al dormir" (detalle)

Esta opcion controla que pantalla se muestra despues de cada ciclo de actualizacion:

**Opcion 1: Regresar a pantalla principal**
```
[Pantalla X] --timeout 30s--> [Pantalla Principal] --> [Sleep]
     |                                                     |
     +--<-- [Despertar] <-- [WiFi+API] <-- [30 min] <------+
```
Siempre regresa a la pantalla principal antes de dormir y al despertar.

**Opcion 2: Mantener pantalla actual**
```
[Pantalla X] --timeout 30s--> [Guarda en NVS] --> [Sleep]
     ^                                               |
     |                                               v
     +--[Muestra Pantalla X]<--[WiFi+API]<--[30 min]+
```
El ciclo completo:
1. Navegas a una pantalla secundaria (ej: Historial)
2. Despues de 30 segundos sin tocar, guarda la pantalla actual
3. Entra en deep sleep
4. Despierta segun el intervalo configurado (ej: 30 min)
5. Conecta WiFi, sincroniza hora, obtiene datos del clima
6. Guarda lectura en historial
7. Muestra la MISMA pantalla donde estabas (Historial)
8. Despues de 30 segundos, vuelve a dormir
9. El ciclo se repite indefinidamente en esa pantalla

**Uso tipico**: Si prefieres ver siempre la pantalla de Historial o Graficas, navega a ella una vez y configura "Mantener pantalla actual". El dispositivo mostrara esa pantalla en cada actualizacion.

#### Horario de Actividad (Ahorro de Bateria)

Esta funcion permite definir un horario en el que el dispositivo NO actualiza el clima, ahorrando bateria durante las horas en que no se necesita (ej: mientras duermes).

**Configuracion**:
- **Hora de Inicio**: Hora a partir de la cual el dispositivo COMIENZA a actualizar
- **Hora de Fin**: Hora a partir de la cual el dispositivo DEJA de actualizar

**Ejemplos**:

| Inicio | Fin | Comportamiento |
|--------|-----|----------------|
| 7 | 23 | Activo de 7:00 a 23:00 (duerme de 23:00 a 7:00) |
| 6 | 22 | Activo de 6:00 a 22:00 (duerme de 22:00 a 6:00) |
| 22 | 6 | Activo de 22:00 a 6:00 - modo nocturno |
| 0 | 24 | Siempre activo (sin ahorro) |

**Nota**: Si Hora de Inicio > Hora de Fin, el dispositivo funciona durante la noche (cruza medianoche).

```
Ejemplo: Inicio=7, Fin=23

        00:00       07:00                   23:00       24:00
          |-----------|------------------------|-----------|
          |  DUERME   |       ACTIVO           |  DUERME   |
          | (no WiFi) | (actualiza cada X min) | (no WiFi) |
```

Durante las horas de "DUERME", el dispositivo:
- NO conecta WiFi
- NO actualiza el clima
- Mantiene la ultima imagen en pantalla
- Consume minima energia (~10uA)

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
5. **Historial** - Guarda lectura en el historial local
6. **Visualizacion** - Muestra la pantalla configurada (principal o la ultima visitada)
7. **Servidor web** - Disponible para configuracion en http://[IP_LOCAL]
8. **Navegacion** - 30 segundos para interactuar
9. **Sleep** - Entra en modo de bajo consumo (ver seccion 5.3 "Al dormir")

### 6.2 Ciclo de Actualizacion

```
[WAKE UP] --> [WiFi] --> [NTP] --> [API] --> [Display] --> [Sleep]
    ^                                                          |
    |                                                          |
    +------ (intervalo configurado, ej: 30 minutos) -----------+
```

**Nota**: La pantalla mostrada en [Display] depende de la configuracion "Al dormir":
- **Regresar a principal**: Siempre muestra la pantalla principal
- **Mantener pantalla**: Muestra la ultima pantalla visitada antes de dormir

Ver seccion 5.3 para detalles completos del comportamiento.

### 6.3 Indicadores en Pantalla

#### Barra de Estado (esquina superior derecha)

```
  [SD]  [Bateria]  [WiFi]
        85% 4.1v   -65dB
```

| Indicador | Descripcion |
|-----------|-------------|
| SD | Icono de tarjeta SD (solo visible si hay SD insertada) |
| Bateria | Icono con nivel de carga, porcentaje y voltaje debajo |
| WiFi | Barras de intensidad (1-5) con valor RSSI en dB |

#### Colores/Tonos
- **Negro**: Informacion principal
- **Gris oscuro**: Valores secundarios
- **Gris claro**: Lineas divisoras, graficas
- **Blanco**: Fondo

---

## 7. Pantallas de Navegacion

### 7.1 Sistema de Navegacion Tactil

El dispositivo tiene **11 pantallas** navegables tocando diferentes zonas:

```
Pantalla Principal (SCREEN_MAIN)
+-------------------------------------+
| [Ciudad]       [SD][Bat][WiFi]<-|  Zona Info (esquina sup-der)
| [Fecha] @ [Hora]                 |  -> Pantalla 4: Info Sistema
+==================================+
|                                  |
| [Rosa Vientos]   [Temp] [Hum]   |  Zona superior
|                  [Max/Min]      |  -> Pantalla 1: Condiciones
| [Luna][Sol] <-   [Descripcion]  |     Actuales
|  Calendario      [ICONO]        |  Zona Luna/Sol -> Calendario
|                                  |
+==================================+
| [+3h] [+6h] [+9h] ... [+21h]    |  Zona media (pronostico horas)
|  Pronostico por horas con iconos |  -> Pantalla 2: Pronostico
+================+=================+
| [Temp] [Pres]  | [Hum] [Lluvia] |  Zona inferior DIVIDIDA:
|   Graficas     |    Graficas    |  Izquierda -> Pant 5: Historial
|   (3 dias)     |    (3 dias)    |  Derecha   -> Pant 3: Tendencias
+----------------+-----------------+
```

**Regresar**: Tocar cualquier parte de una pantalla secundaria regresa a la principal.

### 7.2 Pantalla Principal (Main)

```
+--[Ciudad]---------------[SD][Bateria][WiFi]--+
|                            85% 4.1v          |
|  [Fecha]  @ [Hora]                           |
+----------------------------------------------+
|                                               |
| [Rosa de Vientos]  [Temperatura] [Humedad]    |
|     [Direccion]    [Max/Min]                  |
|     [Velocidad]    [Descripcion clima]        |
|                    [Sensacion termica]        |
|                                               |
| [Luna]  [Salida sol]              [ICONO]     |
| [Fase]  [Puesta sol]              [Grande]    |
|                                               |
+-----------------------------------------------+
| [+3h] [+6h] [+9h] [+12h] [+15h] [+18h] [+21h] |
|  Pronostico por horas (7 columnas)            |
+-----------------------------------------------+
| [Temperatura] [Presion] [Humedad] [Lluvia]    |
|    4 mini-graficas de tendencias (3 dias)     |
+-----------------------------------------------+
```

#### Elementos de la Pantalla Principal

| Elemento | Descripcion |
|----------|-------------|
| Ciudad | Nombre de la ubicacion configurada |
| SD | Icono de tarjeta SD (solo visible si hay SD insertada) |
| Bateria | Icono con nivel de carga, porcentaje y voltaje debajo |
| WiFi | Barras de señal WiFi con nivel en dB |
| Fecha/Hora | Fecha actual y hora de ultima actualizacion |
| Rosa de Vientos | Direccion del viento con brujula visual |
| Temperatura | Valor actual con max/min del dia |
| Humedad | Porcentaje de humedad relativa |
| Sensacion termica | Temperatura percibida (feels like) |
| Luna | Fase lunar con icono grafico |
| Sol | Horas de amanecer y anochecer |
| Icono clima | Representacion visual del clima actual |
| Pronostico | 7 columnas con clima de las proximas horas |
| Tendencias | 4 mini-graficas: temp, presion, humedad, lluvia/nieve (predicciones) |

### 7.3 Pantalla 1: Condiciones Actuales

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|           CONDICIONES ACTUALES                |
+-----------------------------------------------+
|                                               |
|   [Temp]  [^]    [Presion] [>]   [Humedad][v] |
|   25.5 C         1013 mb         65%          |
|   Max 28/Min 22                               |
|                                               |
+-----------------------------------------------+
|                                               |
|  [ICONO GRANDE]    Viento: NNE 5.2 m/s        |
|                    Visibilidad: 10.5 km       |
|  "Parcialmente     Indice UV: 5.2             |
|   nublado"         Nubosidad: 45%             |
|                    Sens. Termica: 24.5 C      |
|                                               |
+-----------------------------------------------+
| [Amanecer] [Anochecer] [Prob.Lluvia] [ICA ->] |
|   06:45       18:32        15%       Bueno    |
+-----------------------------------------------+
```

**Acceso**: Tocar la zona superior de la pantalla principal (area de temperatura/clima).

**Nota**: Tocar la zona **ICA** (Indice de Calidad del Aire) abre la pantalla detallada de contaminantes.

#### Flechas de Tendencia

Cada valor principal (temperatura, presion, humedad) incluye una flecha indicando la tendencia esperada:

| Flecha | Significado |
|--------|-------------|
| ↑ (arriba) | El valor subira en las proximas horas |
| → (derecha) | El valor se mantendra estable |
| ↓ (abajo) | El valor bajara en las proximas horas |

- **Temperatura**: Compara el valor actual con el pronostico del siguiente periodo
- **Presion**: Basada en la diferencia entre el pronostico actual y 6 horas despues
- **Humedad**: Compara el valor actual con el pronostico del siguiente periodo

### 7.4 Pantalla 2: Pronostico Extendido

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|            PRONOSTICO 5 DIAS                  |
+-----------------------------------------------+
|              Proximas 24 Horas                |
| [09:00][12:00][15:00][18:00][21:00][00:00]... |
|  [ico]  [ico]  [ico]  [ico]  [ico]  [ico]     |
|  25/22  28/24  27/23  25/22  24/21  23/20     |
|         (8 columnas de 3 horas)               |
+-----------------------------------------------+
|            Perspectiva 5 Dias                 |
|  [Lun]    [Mar]    [Mie]    [Jue]    [Vie]    |
|  [ico]    [ico]    [ico]    [ico]    [ico]    |
|  30/22    28/21    27/20    29/22    31/23    |
|  2.5mm            0.5mm                        |
+-----------------------------------------------+
```

**Acceso**: Tocar la zona media de la pantalla principal (area del pronostico por horas).

### 7.5 Pantalla 3: Tendencia del Clima

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|          TENDENCIAS DEL CLIMA                 |
+-----------------------------------------------+
|                                               |
|  [Grafica Temperatura]   [Grafica Presion]    |
|  ~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~ |
|  |    ____/\____        ||    ___/\___       ||
|  |___/          \____   ||___/        \___   ||
|                                               |
|  [Grafica Humedad]       [Grafica Lluvia]     |
|  ~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~ |
|  |   /\    /\           || ||  |  ||         ||
|  |__/  \__/  \____      || ||  |  ||  |      ||
|                                               |
+-----------------------------------------------+
|            Proximos 5 dias                    |
+-----------------------------------------------+
```

**Acceso**: Tocar la mitad DERECHA de las graficas en la pantalla principal.

**Nota**: Estas graficas muestran TENDENCIAS (predicciones futuras) obtenidas de la API de OpenWeatherMap, no datos historicos reales.

### 7.6 Pantalla 4: Informacion del Sistema

**Acceso**: Tocar la esquina superior derecha (zona bateria/WiFi).

La seccion de Informacion consta de **5 sub-pantallas navegables** (0/4 a 4/4). Usar las flechas de navegacion en la esquina superior derecha para avanzar entre ellas.

#### 7.6.1 Configuracion del Sistema (0/4)

```
+-----------------------------------------------+
|       CONFIGURACION DEL SISTEMA          >>   |
+===============================================+
| Version: v2.0      | Idioma: Espanol          |
| Compilado: Mar 2026| Unidades: Metrico        |
| Free Heap: 245 KB  | Hemisferio: Norte        |
| Total Heap: 320 KB | Pronostico: 5 dias       |
| PSRAM: 4096 KB     |                          |
|                    | Intervalo: 30 min        |
| WiFi: MiRedWiFi    | Timeout: 30 seg          |
| IP: 192.168.1.100  | Al dormir: Pant. princ.  |
| Senal: -65 dBm     |                          |
|                    | API Key: a1b2c3d4...     |
| Ciudad: CDMX       | Web Config:              |
| Coords: 19.4, -99.1|  http://192.168.1.100    |
| Zona: CST6         |                          |
+-----------------------------------------------+
|          [LIMPIAR PANTALLA]                   |
+-----------------------------------------------+
|                   0 / 4                       |
+-----------------------------------------------+
```

| Columna Izquierda | Columna Derecha |
|-------------------|-----------------|
| Version firmware | Idioma configurado |
| Fecha compilacion | Sistema de unidades |
| Memoria libre (Heap) | Hemisferio |
| Memoria total | Dias de pronostico |
| PSRAM disponible | Intervalo actualizacion |
| Red WiFi conectada | Timeout antes de sleep |
| Direccion IP | Comportamiento al dormir |
| Intensidad senal | API Key (parcial) |
| Ciudad configurada | URL configuracion web |
| Coordenadas | |
| Zona horaria | |

**Boton Limpiar Pantalla**: Ejecuta ciclo de limpieza del display para eliminar ghosting.

#### 7.6.2 Caracteristicas - Hardware (1/4)

```
+-----------------------------------------------+
|       CARACTERISTICAS - HARDWARE         >>   |
+===============================================+
| Microcontrolador: ESP32-S3  | Pantalla: E-Paper 4.7"   |
| CPU: Dual-core @ 240MHz     | Resolucion: 960x540 px   |
| Memoria Flash: 16 MB        | Colores: 16 niveles gris |
| PSRAM: 8 MB OPI             | Tiempo refresco: ~0.5 seg|
|                             | Angulo vision: ~180 grados|
| WiFi: 802.11 b/g/n 2.4GHz   | Touch: GT911 Capacitivo  |
| Bluetooth: BLE 5.0          |                          |
|                             | Bateria: LiPo 3.7V       |
| Consumo Activo: ~150mA      | MicroSD: SPI FAT32/exFAT |
| Consumo Deep Sleep: ~10uA   |                          |
+-----------------------------------------------+
|                   1 / 4                       |
+-----------------------------------------------+
```

#### 7.6.3 Caracteristicas - Software (2/4)

```
+-----------------------------------------------+
|       CARACTERISTICAS - SOFTWARE         >>   |
+===============================================+
| Pantallas: 13 navegables    | API: OpenWeatherMap      |
| Multi-WiFi: Hasta 3 redes   |                          |
| Historial SD: ~1 año datos  | Clima: /weather          |
| Historial Int: ~7 dias FFat | Pronostico: /forecast    |
| Actualizacion: 5-120 min    | UV Index: /uvi           |
| Idiomas: ES / EN / FR       | Calidad Aire: /air_poll  |
|                             |                          |
| Modo AP: WeatherStation-Setup| Limite: 1M llamadas/mes |
| Portal: http://192.168.4.1  |                          |
| Password: weather123        | Almacen: NVS Preferences |
+-----------------------------------------------+
|                   2 / 4                       |
+-----------------------------------------------+
```

#### 7.6.4 Ayuda Rapida (3/4)

```
+-----------------------------------------------+
|            AYUDA RAPIDA                  >>   |
+===============================================+
| Navegacion - (touch)        | Solucion de Problemas    |
| ----------------------      | ------------------------ |
| * Zona superior: Condiciones| Sin WiFi: Verifica SSID  |
| * Iconos: Pronostico extend.| Sin datos: Verifica API  |
| * Graficas izq: Historial   | Ghosting: Boton Limpiar  |
| * Graficas der: Tendencias  | Touch falla: Limpia pant |
| * Bateria/WiFi: Esta pant.  |                          |
| * Luna/Sol: Calendario      |                          |
|   (touch largo)             | MODO BOOTLOADER          |
|                             | ------------------------ |
| Configuracion Inicial       | 1. Mantener BOOT         |
| ----------------------      | 2. Presionar RST         |
| 1. Conectar: WeatherStation | 3. Soltar RST, luego BOOT|
| 2. Abrir: 192.168.4.1       | 4. Cargar firmware       |
| 3. Configurar WiFi, API, ubi|                          |
| 4. Configurar demas params  |                          |
| 5. Guardar y reiniciar      |                          |
+-----------------------------------------------+
|                   3 / 4                       |
+-----------------------------------------------+
```

#### 7.6.5 Creditos (4/4)

```
+-----------------------------------------------+
|              CREDITOS                    >>   |
+===============================================+
| Autor Original              | Hardware                 |
| ----------------------      | ------------------------ |
| David Bird (G6EJD)          | LilyGo T5 4.7" S3 EPD    |
| ESP32 e-Paper Weather       |   touch                  |
| Copyright 2014-2021         |                          |
| github.com/G6EJD            | Librerias                |
|                             | ------------------------ |
| Adaptaciones                | * EPD47 - LilyGO/DFRobot |
| ----------------------      | * ArduinoJson            |
| markbirss - LilyGo EPD      | * ESP32 Arduino          |
| Xinyuan-LilyGO - Fork       |                          |
| Stefan Maetschke 2025       | Enlaces                  |
|                             | ------------------------ |
| Modificaciones XE1E 2026    | github.com/xe1e          |
| ----------------------      |                          |
| * Navegacion tactil 13 pant |    +-------+             |
| * Multi-idioma ES/EN/FR     |    | QR    |             |
| * Portal cautivo y web      |    | CODE  |             |
| * Historial SD + FFat       |    +-------+             |
| * UV Index y Calidad Aire   |                          |
| * Calendario mensual/anual  |                          |
+-----------------------------------------------+
| Solo uso personal. Original: David Bird - Licence.txt |
+-----------------------------------------------+
```

El codigo QR enlaza a github.com/xe1e para acceso rapido al repositorio.

### 7.7 Pantalla 5: Historial de Datos

**Acceso**: Tocar la mitad IZQUIERDA de las graficas en la pantalla principal.

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]              [48H/1SEM]<-  |
+-----------------------------------------------+
|                HISTORIAL                      |
+-----------------------------------------------+
|                                               |
|  [Grafica Temperatura]   [Grafica Presion]    |
|  ~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~ |
|  |    ____/\____        ||    ___/\___       ||
|  |___/          \____   ||___/        \___   ||
|                                               |
|  [Grafica Humedad]       [Grafica Lluvia]     |
|  ~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~ |
|  |   /\    /\           || ||  |  ||         ||
|  |__/  \__/  \____      || ||  |  ||  |      ||
|                                               |
+-----------------------------------------------+
|     150 lecturas / 2.5 dias [SD: 52560]       |
+-----------------------------------------------+
```

El footer muestra el conteo de lecturas en buffer y, si hay SD insertada, el total de lecturas almacenadas en la tarjeta SD entre corchetes.

#### Diferencia con Pantalla de Tendencias

| Pantalla Tendencias (3) | Pantalla Historial (5) |
|-------------------------|------------------------|
| Datos de TENDENCIAS (predicciones) | Datos HISTORICOS reales |
| Obtenidos de la API | Grabados por el dispositivo |
| Proximos 5 dias | Ultimas 48h o 1 semana |
| Se actualizan con cada llamada API | Se acumulan con el tiempo |

#### Boton Toggle 48H / 1 Semana

El boton **[48H/1SEM]** en la esquina superior derecha alterna entre dos vistas:

- **48H**: Muestra las ultimas 48 horas de datos (mayor detalle)
- **1SEM**: Muestra la ultima semana completa (vision general)

#### Almacenamiento de Datos

El sistema utiliza almacenamiento dual con fallback automatico:

**Sin tarjeta SD (modo basico)**:
- Datos en FFat (flash interna) con buffer en PSRAM
- Capacidad maxima: ~1000 lecturas (~7 dias a intervalos de 10 min)
- Persisten entre reinicios y deep sleep

**Con tarjeta SD (modo extendido)**:
- Historial extendido en SD card (archivo CSV)
- Capacidad: ~52,560 lecturas (~1 año a intervalos de 10 min)
- Fallback automatico: si se retira la SD, continua con FFat
- El archivo `/weather_history.csv` es legible en cualquier computadora
- FFat siempre mantiene respaldo de las ultimas 1000 lecturas

| Almacenamiento | Capacidad | Formato | Uso |
|----------------|-----------|---------|-----|
| FFat (interno) | ~1000 lecturas | Binario | Fallback/respaldo |
| SD Card | ~52,560 lecturas | CSV | Historial extendido |

**Requisitos de tarjeta SD**:
- Formato: FAT32 (tarjetas hasta 32GB) o exFAT (64GB+)
- Tamano recomendado: 1GB-32GB (no necesita mas)
- El dispositivo detecta automaticamente la tarjeta al iniciar

### 7.8 Pantalla 6: Calidad del Aire

**Acceso**: Tocar la zona **ICA** en la pantalla de Condiciones Actuales.

```
+-----------------------------------------------+
|  [Fecha]  @ [Hora]                            |
+-----------------------------------------------+
|            CALIDAD DEL AIRE                   |
+-----------------------------------------------+
|                                               |
|         ICA: 2 - Aceptable                    |
|   1=Bueno 2=Aceptable 3=Moderado 4=Malo 5=Muy Malo |
|                                               |
+-----------------------------------------------+
|                                               |
|   PM2.5: 12.5 ug/m3      CO: 234.5 ug/m3      |
|   PM10:  25.3 ug/m3      NO2: 15.2 ug/m3      |
|   O3:    45.6 ug/m3      SO2: 8.3 ug/m3       |
|                                               |
+-----------------------------------------------+
|           Indice UV: 5.2 Moderado             |
+-----------------------------------------------+
|           Toca para volver                    |
+-----------------------------------------------+
```

#### Indice de Calidad del Aire (ICA)

| Valor | Descripcion | Recomendacion |
|-------|-------------|---------------|
| 1 | Bueno | Sin restricciones |
| 2 | Aceptable | Grupos sensibles pueden tener molestias |
| 3 | Moderado | Limitar actividad al aire libre |
| 4 | Malo | Evitar actividad al aire libre |
| 5 | Muy Malo | Permanecer en interiores |

#### Contaminantes Medidos

| Contaminante | Descripcion |
|--------------|-------------|
| PM2.5 | Particulas finas (< 2.5 micrometros) |
| PM10 | Particulas gruesas (< 10 micrometros) |
| O3 | Ozono troposferico |
| CO | Monoxido de carbono |
| NO2 | Dioxido de nitrogeno |
| SO2 | Dioxido de azufre |

#### Indice UV

| Rango | Nivel | Proteccion recomendada |
|-------|-------|------------------------|
| 0-2 | Bajo | No requiere proteccion |
| 3-5 | Moderado | Usar protector solar |
| 6-7 | Alto | Protector + sombrero |
| 8-10 | Muy Alto | Evitar exposicion directa |
| 11+ | Extremo | Permanecer en interiores |

### 7.9 Timeout y Sleep

| Condicion | Timeout | Comportamiento |
|-----------|---------|----------------|
| Sin tocar pantalla | 30 seg (configurable) | Entra en deep sleep |
| Navegacion continua | 5 min maximo | Proteccion, fuerza sleep |
| Configuracion web activa | 2 min sin solicitudes | Mantiene despierto mientras se configura |

#### Comportamiento del Servidor Web

- El servidor web se inicia automaticamente al conectar WiFi
- **Solo si alguien accede** a la pagina de configuracion, se activa el timeout de 2 minutos
- Si nadie accede a la web, el dispositivo duerme a los 30 segundos normales
- Cada vez que se carga o guarda la configuracion, el timeout de 2 min se reinicia
- Esto permite configurar tranquilamente sin que el dispositivo se duerma

#### Endpoints Web Disponibles

| Endpoint | Descripcion |
|----------|-------------|
| `http://[IP]/` | Pagina principal de configuracion |
| `http://[IP]/history` | Ver ultimas 50 lecturas del historial en HTML |
| `http://[IP]/history.csv` | Descargar archivo CSV completo (requiere SD) |

**Acceso al historial via web:**
- `/history` muestra una tabla con las ultimas 50 lecturas del buffer
- `/history.csv` descarga el archivo CSV directamente de la tarjeta SD
- Si no hay SD insertada, `/history.csv` devuelve error 404

La pantalla mostrada al despertar depende de la configuracion "Al dormir" en Tab Sistema

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

## 10. Carcasa Impresa en 3D

### 10.1 Modelo Recomendado

Se recomienda utilizar la carcasa disenada especificamente para la placa LilyGo T5 e-Paper 4.7":

**Simple case for LilyGO T5 e-Paper 4.7" ESP32-S3 Board [H716]**
- **Autor**: n602
- **URL**: https://www.thingiverse.com/thing:6897183
- **Licencia**: Creative Commons BY-NC-SA 4.0

Esta carcasa esta disenada para la version TOUCH (modelo H716, V2.4) que es la utilizada en este proyecto.

### 10.2 Archivos Incluidos

| Archivo | Descripcion |
|---------|-------------|
| Bottom.stl | Base inferior de la carcasa |
| Top.stl | Tapa superior |
| Frame.stl | Marco para la pantalla |
| Button.stl | Boton de acceso |
| Heatsink.stl | Disipador de calor (opcional) |
| *.STEP | Archivo CAD editable |

### 10.3 Materiales Necesarios

#### Para Impresion
- Filamento PLA o PETG (~50g)
- Impresora 3D con cama de al menos 150x150mm

#### Hardware de Ensamblaje
| Cantidad | Componente |
|----------|------------|
| 4 | Tornillos M3 x 15mm |
| 4 | Tuercas M3 |

### 10.4 Configuracion de Impresion

| Parametro | Valor Recomendado |
|-----------|-------------------|
| Altura de capa | 0.2mm |
| Relleno (Infill) | 20-30% |
| Paredes | 3 perimetros |
| Material | PLA o PETG |
| Soportes | Si (necesarios) |
| Brim/Raft | Opcional (mejora adherencia) |

### 10.5 Instrucciones de Impresion

#### Paso 1: Preparacion
1. Descargar todos los archivos STL desde Thingiverse
2. Importar en el slicer (Cura, PrusaSlicer, etc.)
3. Orientar las piezas con la cara plana hacia abajo

#### Paso 2: Configuracion del Slicer
```
Material:        PLA (mas facil) o PETG (mas resistente)
Temperatura:     PLA 200-210C / PETG 230-245C
Cama:            PLA 60C / PETG 70-80C
Velocidad:       50-60 mm/s
Soportes:        Activados (especialmente para Frame y Top)
```

#### Paso 3: Impresion
1. Imprimir cada pieza por separado para mejores resultados
2. Tiempo estimado total: 4-6 horas
3. Dejar enfriar las piezas antes de retirar de la cama

#### Paso 4: Post-Procesado
1. Retirar cuidadosamente los soportes
2. Lijar las rebabas con lija fina (grano 200-400)
3. Verificar que los agujeros para tornillos esten limpios

### 10.6 Ensamblaje

```
+------------------+
|    [Top.stl]     |  <- Tapa superior
+------------------+
|  [Frame.stl]     |  <- Marco de pantalla
|  +------------+  |
|  |  PANTALLA  |  |
|  +------------+  |
+------------------+
|   [Bottom.stl]   |  <- Base con ESP32
+------------------+
     [Button.stl]     <- Boton lateral
```

#### Pasos de Ensamblaje

1. **Colocar la placa en Bottom.stl**
   - Orientar la placa con los conectores USB accesibles
   - Verificar que los agujeros de montaje coincidan

2. **Instalar el Frame.stl**
   - Colocar sobre la pantalla e-paper
   - Asegurar que no presione la pantalla

3. **Colocar Top.stl**
   - Alinear con la base
   - Verificar que la pantalla sea visible

4. **Fijar con tornillos**
   - Insertar los 4 tornillos M3 x 15mm
   - Ajustar las tuercas M3 sin apretar demasiado

5. **Instalar Button.stl** (opcional)
   - Colocar en el hueco lateral
   - Debe permitir acceso al boton BOOT

### 10.7 Consejos Adicionales

- **Tolerancias**: Si las piezas no encajan bien, escalar ligeramente (99-101%) en el slicer
- **Color**: Usar filamento oscuro (negro/gris) mejora el contraste con la pantalla
- **Ventilacion**: El disipador de calor es opcional pero recomendado para uso continuo
- **Acceso SD**: Verificar que la ranura de tarjeta SD sea accesible despues del ensamblaje

---

## 11. Solucion de Problemas

### 11.1 No Se Conecta a WiFi

**Sintomas**: Entra en modo AP cada vez

**Soluciones**:
1. Verificar que el SSID esta escrito exactamente igual
2. Verificar la contrasena
3. Asegurar que la red es 2.4GHz (no 5GHz)
4. Mover el dispositivo mas cerca del router

### 11.2 No Muestra Datos del Clima

**Sintomas**: Pantalla en blanco o con "?"

**Soluciones**:
1. Verificar API Key de OpenWeatherMap
2. Verificar coordenadas de ubicacion
3. Verificar conexion a internet
4. Revisar Serial Monitor para errores

### 11.3 Pantalla con Ghosting

**Sintomas**: Imagenes anteriores visibles

**Soluciones**:
1. Usar el boton [LIMPIAR PANTALLA] en la pantalla de Info Sistema
2. El proximo refresco completo lo limpiara
3. Reiniciar el dispositivo para forzar limpieza

### 11.4 Touch No Responde

**Sintomas**: No navega entre pantallas

**Soluciones**:
1. Verificar en Serial que GT911 fue detectado
2. Limpiar la pantalla de polvo/grasa
3. Tocar con firmeza por al menos 50ms
4. Esperar 500ms entre toques (debounce)

### 11.5 No Carga el Firmware

**Sintomas**: Error de conexion en Arduino IDE

**Soluciones**:
1. Usar modo bootloader forzado (seccion 4.3)
2. Probar otro cable USB
3. Verificar drivers USB instalados
4. Cerrar Serial Monitor antes de cargar

### 11.6 Consumo de Bateria Alto

**Sintomas**: Bateria se agota rapido

**Soluciones**:
1. Aumentar intervalo de actualizacion
2. Verificar que entra en deep sleep
3. Revisar que no hay WiFi activo durante sleep

---

## 12. Funciones Especiales

Esta seccion documenta las funciones ocultas o especiales del dispositivo que no son evidentes en la navegacion normal.

### 12.1 Calendario

El dispositivo incluye un sistema de calendario completo con vistas mensual y anual.

#### 12.1.1 Acceso al Calendario

Para acceder al calendario desde la pantalla principal:

1. **Ubicar la zona de activacion**: Area del icono de la luna, amanecer y atardecer (esquina superior izquierda)
2. **Tocar la zona**: Un toque normal activa el calendario
3. **Activacion**: La pantalla cambiara al calendario mensual

#### 12.1.2 Calendario Mensual

Muestra un mes completo con las siguientes caracteristicas:

| Elemento | Descripcion |
|----------|-------------|
| Titulo | Nombre del mes y ano (ej: "Marzo 2026") |
| Flechas dobles | Navegacion entre meses (izquierda/derecha) |
| Encabezados | LUN, MAR, MIE, JUE, VIE, SAB, DOM |
| Dias | Numeros del 1 al 28/29/30/31 segun el mes |
| Dia actual | Marcado con subrayado doble |
| Semana | Inicia en lunes |

**Navegacion del Calendario Mensual:**

| Accion | Zona de toque | Resultado |
|--------|---------------|-----------|
| Mes anterior | Flechas izquierda (esquina superior izquierda) | Retrocede un mes |
| Mes siguiente | Flechas derecha (esquina superior derecha) | Avanza un mes |
| Ver ano | Titulo central (nombre del mes) | Cambia a vista anual |
| Salir | Cualquier otra zona | Regresa a pantalla principal |

#### 12.1.3 Calendario Anual

Muestra los 12 meses del ano en una cuadricula de 3 columnas x 4 filas:

| Elemento | Descripcion |
|----------|-------------|
| Titulo | Ano (ej: "2026") |
| Flechas dobles | Navegacion entre anos |
| Mini-calendarios | 12 meses con dias compactos |
| Separadores | Lineas grises entre meses |
| Dia actual | Marcado con subrayado (solo en ano actual) |

**Navegacion del Calendario Anual:**

| Accion | Zona de toque | Resultado |
|--------|---------------|-----------|
| Ano anterior | Flechas izquierda | Retrocede un ano |
| Ano siguiente | Flechas derecha | Avanza un ano |
| Salir | Cualquier otra zona | Regresa a calendario mensual |

#### 12.1.4 Estructura del Codigo

El codigo del calendario esta separado en su propio modulo:

| Archivo | Contenido |
|---------|-----------|
| `calendar.h` | Funciones de dibujo de calendarios |
| `touch_handler.h` | Zonas tactiles y navegacion |

**Funciones principales:**
- `DisplayCalendarScreen()` - Renderiza el calendario mensual
- `DisplayCalendarYearScreen()` - Renderiza el calendario anual

**Variables de navegacion:**
- `calendarMonthOffset` - Desplazamiento de meses (0 = mes actual)
- `calendarYearOffset` - Desplazamiento de anos (0 = ano actual)

### 12.2 Tarjeta SD

El dispositivo soporta almacenamiento extendido mediante tarjeta MicroSD.

#### 12.2.1 Caracteristicas

| Caracteristica | Descripcion |
|----------------|-------------|
| Formato | FAT32 (hasta 32GB) o exFAT (64GB+) |
| Archivo | `/weather_history.csv` |
| Capacidad | ~52,560 lecturas (~1 año a 10 min) |
| Fallback | Si no hay SD, usa FFat interno (~7 dias) |

#### 12.2.2 Formato del Archivo CSV

```csv
timestamp,temp,humidity,pressure,wind_speed,wind_dir,rain,description
1710000000,22.5,65,1013,5.2,180,0.0,Parcialmente nublado
```

| Campo | Descripcion |
|-------|-------------|
| timestamp | Unix timestamp (segundos desde 1970) |
| temp | Temperatura en unidades configuradas |
| humidity | Humedad relativa (%) |
| pressure | Presion atmosferica (mb) |
| wind_speed | Velocidad del viento |
| wind_dir | Direccion del viento (grados) |
| rain | Precipitacion (mm o in) |
| description | Descripcion del clima |

#### 12.2.3 Indicador en Pantalla

Cuando hay una tarjeta SD insertada, aparece un icono de SD en la barra de estado (esquina superior derecha). El icono no aparece si no hay tarjeta.

### 12.3 Acceso Web

El dispositivo incluye un servidor web que permite configuracion y acceso a datos desde cualquier navegador en la red local.

#### 12.3.1 Activacion

El servidor web se activa automaticamente al conectar WiFi. No requiere configuracion adicional.

#### 12.3.2 Endpoints Disponibles

| Endpoint | Descripcion |
|----------|-------------|
| `http://[IP]/` | Pagina de configuracion (4 pestanas) |
| `http://[IP]/history` | Ultimas 50 lecturas en tabla HTML |
| `http://[IP]/history.csv` | Descarga CSV completo (requiere SD) |

#### 12.3.3 Pagina de Historial (/history)

Muestra una tabla con las ultimas 50 lecturas del buffer en memoria:

- Fecha y hora de cada lectura
- Temperatura, humedad, presion
- Enlace para descargar CSV (si hay SD)

#### 12.3.4 Descarga CSV (/history.csv)

- Descarga el archivo completo de la tarjeta SD
- Nombre del archivo: `weather_history.csv`
- Si no hay SD insertada, devuelve error 404
- Util para analisis en Excel, Google Sheets, etc.

#### 12.3.5 Timeout del Servidor

| Condicion | Comportamiento |
|-----------|----------------|
| Sin acceso web | Sleep a los 30 seg (configurable) |
| Acceso a pagina de config | Timeout extendido a 2 min |
| Guardar configuracion | Reinicia timeout de 2 min |

El servidor permanece activo mientras se este usando la pagina de configuracion, evitando que el dispositivo entre en sleep durante la configuracion.

### 12.4 Configuracion Bluetooth

El dispositivo soporta configuracion via Bluetooth Low Energy (BLE) usando una aplicacion Android.

#### 12.4.1 Activacion del Modo BLE

1. Navegar a la pantalla **Configuracion del Sistema** (Info)
2. Tocar el boton **Bluetooth** (esquina inferior izquierda)
3. La pantalla mostrara "CONFIGURACION BLUETOOTH"
4. El dispositivo aparecera como **WeatherStation-BLE**

#### 12.4.2 App Android

La aplicacion Android se encuentra en la carpeta `android-app/` del repositorio.

**Compilacion:**
1. Abrir la carpeta `android-app` en Android Studio
2. Sincronizar Gradle
3. Build > Make Project
4. Instalar en dispositivo Android

**Requisitos:**
- Android 8.0 (API 26) o superior
- Bluetooth Low Energy (BLE)
- Permisos de ubicacion (requerido para BLE)

#### 12.4.3 Uso de la App

1. **Buscar Dispositivo** - Escanea dispositivos BLE cercanos
2. **Conectar** - Establece conexion con la estacion
3. **Configurar** - Abre pantalla de configuracion
4. Ingresar los parametros deseados
5. **Enviar Configuracion** - Envia todos los valores
6. **Guardar** - Guarda en memoria del dispositivo
7. **Reiniciar** - Reinicia para aplicar cambios

#### 12.4.4 Parametros Configurables via BLE

| Categoria | Parametros |
|-----------|------------|
| WiFi | Hasta 3 redes (SSID + password) |
| API | API Key de OpenWeatherMap |
| Ubicacion | Ciudad, Latitud, Longitud |
| Regional | Idioma, Hemisferio, Unidades |
| Zona Horaria | Timezone, GMT offset, DST offset |
| Comportamiento | Intervalo actualizacion, dias pronostico |
| Sleep | Timeout, hora despertar/dormir |

#### 12.4.5 Comandos BLE

| Comando | Funcion |
|---------|---------|
| Leer Config | Obtiene configuracion actual del dispositivo |
| Enviar Config | Envia nuevos valores (sin guardar) |
| Probar WiFi | Verifica conexion WiFi con credenciales enviadas |
| Guardar | Guarda configuracion en memoria permanente |
| Reiniciar | Reinicia el dispositivo |

#### 12.4.6 Ventajas sobre Web Config

- No requiere conexion WiFi previa
- Util para configuracion inicial
- Funciona aunque no haya red disponible
- Configuracion desde cualquier ubicacion

### 12.5 Pantallas Ocultas

El dispositivo incluye una serie de pantallas ocultas accesibles mediante navegacion secuencial. Estas pantallas no son visibles en la navegacion normal y proporcionan funciones adicionales.

#### 12.5.1 Ruta de Acceso

```
Pantalla Principal ---------> Clima Narrativo (AI)
      |                       (tocar icono grande centro)
      v
Info (bateria/WiFi) -----> Features 1 -----> Features 2
                                                  |
                                                  v
                                             Help -----> Credits (QR)
                                                              |
                                                              v
                                                         Callsign (XE1E)
                                                              |
                                                              v
                                                         World Clock
                                                              |
                                                              v
                                                    Pensamiento del Dia
```

#### 12.5.2 Pantalla Callsign

Acceso: Credits (QR) → tocar zona inferior

Muestra el indicativo de radioaficionado del autor:

| Elemento | Descripcion |
|----------|-------------|
| Indicativo | XE1E en fuente extra grande |
| Ciudad | Ciudad de Mexico |
| Localizador | Grid locator del operador |

Navegacion:
- Tocar zona inferior → World Clock
- Tocar otra zona → Regresa a Credits

#### 12.5.3 Pantalla World Clock (Reloj Mundial)

Acceso: Callsign → tocar zona inferior

Muestra un mapa mundial con las zonas horarias:

| Elemento | Descripcion |
|----------|-------------|
| Mapa | Proyeccion del mundo con zonas horarias |
| Lineas | Divisiones de zonas UTC |
| Visualizacion | Mapa completo en escala de grises |

Navegacion:
- Tocar lado derecho → Pensamiento del Dia
- Tocar lado izquierdo → Regresa a Callsign

#### 12.5.4 Pantalla Pensamiento del Dia

Acceso: World Clock → tocar lado derecho

Muestra una frase inspiracional diaria obtenida de internet:

| Elemento | Descripcion |
|----------|-------------|
| Marco | Decorativo con esquinas y bordes elegantes |
| Titulo | "~ Pensamiento del Dia ~" |
| Frase | Texto de la cita en espanol |
| Autor | Nombre del autor de la frase |
| Footer | "Toca para volver" |

Caracteristicas:
- Frase del dia en espanol (API: frasedeldia.azurewebsites.net)
- La misma frase se muestra todo el dia, cambia cada 24 horas
- Persiste en deep sleep (se conserva al despertar)
- Formato elegante con comillas decorativas

Navegacion:
- Tocar cualquier zona → Regresa a Credits (QR)

#### 12.5.5 Pantalla Clima Narrativo (AI)

Acceso: Pantalla principal → tocar el icono grande del clima (centro de la pantalla)

Genera una descripcion natural del clima usando inteligencia artificial (Groq/Llama):

| Elemento | Descripcion |
|----------|-------------|
| Marco | Decorativo con esquinas y bordes elegantes |
| Titulo | "~ El Clima de Hoy ~" |
| Ciudad | Nombre de la ubicacion configurada |
| Narrativa | Descripcion del clima en 4-5 oraciones |
| Footer | "Toca para volver" |

##### Estilos de Narrativa

El estilo se configura desde la interfaz web (Seccion Sistema → Clima Narrativo):

| Estilo | Descripcion | Ejemplo |
|--------|-------------|---------|
| Radio | Boletin de radio, conciso | "Buenos dias radioescuchas, hoy tenemos 22 grados..." |
| Formal | Como noticiero de TV | "Las condiciones meteorologicas actuales indican..." |
| Poetico | Metaforas literarias | "El sol bana la ciudad con su calido abrazo..." |
| Tecnico | Estilo meteorologico | "Sistema de alta presion. Temperatura: 22C..." |
| Humoristico | Con toques de humor | "Ni muy frio ni muy caliente, el clima esta indeciso..." |
| Abuelita | Consejos de abuela | "Mijo, ponte suetercito que andan los 22 grados..." |

##### Como Configurar

1. Accede a la interfaz web:
   - Red AP: `WeatherStation-Setup` (pass: `weather123`) → `http://192.168.4.1`
   - O usa la IP del dispositivo si ya esta conectado a tu red
2. Ve a la pestana **Sistema**
3. Busca la seccion **"Clima Narrativo (IA)"**
4. Selecciona el estilo deseado del menu desplegable
5. Guarda la configuracion

##### Datos Utilizados

La IA genera el texto usando datos reales de OpenWeatherMap (no inventa numeros):
- Temperatura actual y sensacion termica
- Humedad y presion atmosferica
- Condicion del cielo (nublado, soleado, etc.)
- Velocidad del viento
- Pronostico de proximas horas

##### Requisitos

- Conexion WiFi activa (para llamar a Groq API)
- API key de Groq configurada en `owm_credentials.h` o via web
- Obtener key gratis en: https://console.groq.com

##### Caracteristicas

- El texto se genera cada vez que entras a la pantalla
- **Multilingue**: Genera en el idioma configurado (ES/EN/FR)
- Persiste en deep sleep (se conserva al despertar)
- Formato elegante con marco decorativo
- Fuente OpenSans10B para mejor legibilidad
- Hasta 9 lineas de ~69 caracteres cada una
- Modelo IA: Llama 3.1 8B (rapido y gratuito)

Navegacion:
- Tocar cualquier zona → Regresa a pantalla principal

---

## 13. Apendice

### 13.1 Fuentes Disponibles

| Nombre | Tamano | Uso |
|--------|--------|-----|
| OpenSans6 | 6pt | Texto muy pequeno |
| OpenSans8B | 8pt | Texto pequeno, hints, uptime |
| OpenSans9B | 9pt | Etiquetas graficas |
| OpenSans10B | 10pt | Fecha/hora, detalles, info sistema |
| OpenSans12B | 12pt | Textos medios, botones |
| OpenSans14B | 14pt | Ciudad, descripciones |
| OpenSans18B | 18pt | Titulos de pantallas |
| OpenSans24B | 24pt | Temperatura, datos principales |
| OpenSans28B | 28pt | Numeros extra grandes |

### 13.2 Coordenadas de Pantalla

```
(0,0)--------------------------(960,0)
  |                               |
  |        960 x 540 pixeles      |
  |                               |
(0,540)-----------------------(960,540)
```

### 13.3 Direcciones del Viento

Las abreviaciones varian segun el idioma configurado:

| Grados | Espanol | English | Francais |
|--------|---------|---------|----------|
| 0 / 360 | N | N | N |
| 45 | NE | NE | NE |
| 90 | E | E | E |
| 135 | SE | SE | SE |
| 180 | S | S | S |
| 225 | SO | SW | SO |
| 270 | O | W | O |
| 315 | NO | NW | NO |

### 13.4 Fases Lunares

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

### 13.5 Textos Multi-idioma

El sistema soporta 3 idiomas configurables:

| Texto | Espanol | English | Francais |
|-------|---------|---------|----------|
| Condiciones | Condiciones Actuales | Current Conditions | Conditions Actuelles |
| Pronostico | Pronostico 5 Dias | 5-Day Forecast | Previsions 5 Jours |
| Tendencias | Tendencias del Clima | Weather Trends | Tendances Meteo |
| Historial | Historial | History | Historique |
| Info Sistema | Info del Sistema | System Info | Info Systeme |
| Amanecer | Amanecer | Sunrise | Lever |
| Anochecer | Anochecer | Sunset | Coucher |
| Humedad | Humedad | Humidity | Humidite |
| Presion | Presion | Pressure | Pression |
| Viento | Viento | Wind | Vent |
| Limpiar | Limpiar Pantalla | Clean Screen | Nettoyer Ecran |
| 48 Horas | 48H | 48H | 48H |
| 1 Semana | 1 Semana | 1 Week | 1 Semaine |

### 13.6 Licencia y Creditos

**Software Original**: David Bird 2021
**Modificaciones**: XE1E 2024

Este proyecto es de codigo abierto. Consultar el repositorio para detalles de licencia.

---

*Manual Version 2.1 - Marzo 2026*

### Cambios en Version 2.1
- Corregido orden barra de estado: SD, Bateria, WiFi
- Corregida numeracion de pantallas (11 pantallas navegables)
- Actualizada seccion de direcciones del viento con multi-idioma
- Corregido acceso al calendario (toque normal, no prolongado)
- Agregada zona de calendario en diagrama de navegacion
- Rosa de vientos con direcciones en idioma configurado (O, NO, SO en español)
- Mejorado espaciado y alineacion en pantalla Condiciones Actuales
- Mejoradas fuentes en calendarios
- Nueva seccion 12.2: Tarjeta SD (almacenamiento extendido, formato CSV)
- Nueva seccion 12.3: Acceso Web (endpoints, historial, configuracion)

### Cambios en Version 2.0
- Indice UV en pantalla Condiciones Actuales
- Calidad del Aire (ICA) con pantalla detallada de contaminantes
- Nueva pantalla "Calidad del Aire" con PM2.5, PM10, CO, NO2, O3, SO2
- Soporte SD card con historial extendido (~1 año de datos)
- Fallback automatico SD/FFat (funciona con o sin tarjeta SD)
- Archivo CSV en SD legible desde cualquier computadora
- Acceso web al historial: `/history` y `/history.csv`
- Icono SD en barra de estado (visible cuando hay tarjeta insertada)
- Contador SD en pantalla de historial
- Nueva disposicion barra de estado: SD, Bateria, WiFi
- Documentada nueva pantalla de Historial (datos reales grabados)
- Actualizadas zonas tactiles de la pantalla principal
- Agregado boton "Limpiar Pantalla" en Info Sistema
- Agregada configuracion "Mantener pantalla al dormir"
- Actualizada lista de fuentes disponibles
- Corregidos textos multi-idioma
