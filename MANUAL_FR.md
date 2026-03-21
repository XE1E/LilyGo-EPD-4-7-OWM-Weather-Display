# Manuel Station Meteo LilyGo EPD 4.7"

## Table des Matieres

1. [Introduction](#1-introduction)
2. [Specifications Techniques](#2-specifications-techniques)
3. [Architecture du Systeme](#3-architecture-du-systeme)
4. [Installation et Compilation](#4-installation-et-compilation)
5. [Configuration](#5-configuration)
6. [Utilisation de l'Appareil](#6-utilisation-de-lappareil)
7. [Ecrans de Navigation](#7-ecrans-de-navigation)
8. [API OpenWeatherMap](#8-api-openweathermap)
9. [Gestion de l'Energie](#9-gestion-de-lenergie)
10. [Boitier Imprime en 3D](#10-boitier-imprime-en-3d)
11. [Resolution de Problemes](#11-resolution-de-problemes)
12. [Fonctions Speciales](#12-fonctions-speciales)
    - [12.1 Calendrier](#121-calendrier)
    - [12.2 Carte SD](#122-carte-sd)
    - [12.3 Acces Web](#123-acces-web)
    - [12.4 Configuration Bluetooth](#124-configuration-bluetooth)
13. [Annexe](#13-annexe)

---

## 1. Introduction

### 1.1 Description Generale

La Station Meteo LilyGo EPD 4.7" est un appareil base sur ESP32-S3 qui affiche des informations meteorologiques en temps reel obtenues d'OpenWeatherMap. Elle utilise un ecran e-paper (encre electronique) de 4,7 pouces offrant une excellente visibilite dans toutes les conditions d'eclairage et une faible consommation d'energie.

### 1.2 Caracteristiques Principales

- **Ecran e-paper de 4,7 pouces** - 960x540 pixels, niveaux de gris
- **Navigation tactile** - Controleur GT911 avec 11 ecrans navigables
- **Multi-WiFi** - Support jusqu'a 3 reseaux WiFi configurables
- **Mode AP (Point d'Acces)** - Portail captif pour la configuration initiale
- **Serveur web integre** - Configuration depuis n'importe quel navigateur sur le reseau local
- **Configuration Bluetooth** - Application Android pour configuration via BLE
- **Veille profonde** - Faible consommation pour fonctionnement sur batterie
- **Multi-langue** - Espagnol, Anglais et Francais
- **Mise a jour automatique** - Intervalle configurable (5-120 minutes)
- **Historique des donnees** - Stocke les lectures reelles pour les tendances (SD: ~1 an, interne: ~7 jours)

---

## 2. Specifications Techniques

### 2.1 Materiel

#### Microcontroleur
| Parametre | Specification |
|-----------|---------------|
| Puce | ESP32-S3 |
| CPU | Dual-core Xtensa LX7 @ 240MHz |
| RAM | 512KB SRAM + 8MB PSRAM (OPI) |
| Flash | 16MB |
| WiFi | 802.11 b/g/n (2,4GHz) |

#### Ecran E-Paper
| Parametre | Specification |
|-----------|---------------|
| Type | E-Ink (encre electronique) |
| Taille | 4,7 pouces diagonale |
| Resolution | 960 x 540 pixels |
| Couleurs | 16 niveaux de gris |
| Technologie | ED047TC1 |
| Controleur | IT8951 |
| Temps de rafraichissement | ~0,5 secondes |
| Angle de vision | ~180 degres |
| Consommation en veille | ~0 mW (image statique) |

#### Panneau Tactile
| Parametre | Specification |
|-----------|---------------|
| Controleur | GT911 |
| Interface | I2C |
| Broches | SDA=18, SCL=17, INT=47 |
| Adresse I2C | 0x5D ou 0x14 |
| Points tactiles | Jusqu'a 5 simultanement |

#### Alimentation
| Parametre | Specification |
|-----------|---------------|
| Tension entree USB | 5V |
| Tension batterie | 3,7V LiPo (3,2V-4,2V) |
| Consommation active | ~150mA |
| Consommation veille profonde | ~10uA |
| Broche ADC batterie | GPIO 14 |

#### Boutons
| Parametre | Specification |
|-----------|---------------|
| Bouton BOOT | GPIO 0 (mode bootloader) |
| Bouton RST | Reset materiel |

#### Lecteur Carte SD
| Parametre | Specification |
|-----------|---------------|
| Interface | SPI |
| CS (Chip Select) | GPIO 42 |
| MOSI | GPIO 15 |
| MISO | GPIO 16 |
| CLK | GPIO 11 |
| Formats supportes | FAT32, exFAT |
| Taille maximale | Sans limite (teste jusqu'a 64GB) |

### 2.2 Principes de la Technologie E-Paper

#### Fonctionnement

L'ecran e-paper utilise des **microspheres bicolores** en suspension dans un fluide:

```
+------------------+
|  @@  @@  @@  @@ |  <-- Particules blanches (chargees +)
|  oo  oo  oo  oo |  <-- Particules noires (chargees -)
|==================|  <-- Electrode superieure (transparente)
|  Fluide          |
|==================|  <-- Electrode inferieure
+------------------+
```

- **Tension positive**: Les particules blanches montent, apparait blanc
- **Tension negative**: Les particules noires montent, apparait noir
- **Sans tension**: L'image reste indefiniment (memoire bistable)

#### Avantages
1. **Visibilite** - Parfaite sous lumiere solaire directe
2. **Angle de vision** - Presque 180 degres
3. **Consommation** - Consomme de l'energie uniquement lors du changement d'image
4. **Confort visuel** - Pas de retroeclairage, ne fatigue pas les yeux

#### Limitations
1. **Vitesse** - Rafraichissement plus lent que LCD (~0,5s)
2. **Ghosting** - Des images residuelles peuvent rester
3. **Couleur** - Niveaux de gris uniquement (pas de couleur)
4. **Temperature** - Fonctionnement optimal 0-50C

### 2.3 Echelle de Gris

L'ecran supporte 16 niveaux de gris definis dans le code:

```cpp
#define White      0xFF   // Blanc pur
#define LightGrey  0xBB   // Gris clair
#define Grey       0x88   // Gris moyen
#define DarkGrey   0x44   // Gris fonce
#define Black      0x00   // Noir pur
```

---

## 3. Architecture du Systeme

### 3.1 Diagramme de Flux Principal

```
                    +----------------+
                    |     DEBUT      |
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
                    | FORCE_AP_MODE? |----Oui----> Mode AP
                    +----------------+             |
                           |Non                    |
                           v                       |
                    +----------------+             |
                    | StartWiFi()    |             |
                    | - Scan reseaux |             |
                    | - Meilleur sig.|             |
                    +----------------+             |
                           |                       |
                      Connecte?                    |
                      /        \                   |
                    Oui         Non----------------+
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
              | - 30s timeout  |           |   chaque 60s   |
              +----------------+           +----------------+
                    |
                    v (timeout)
              +----------------+
              | BeginSleep()   |
              | - deep_sleep   |
              +----------------+
```

### 3.2 Structure des Fichiers

```
LilyGo-EPD-4-7-OWM-Weather-Display/
|
+-- LilyGo-EPD-4-7-OWM-Weather-Display.ino  # Sketch principal
|
+-- owm_credentials.h     # Identifiants WiFi et API (defauts)
|
+-- wifi_manager.h        # Mode AP, portail web, stockage NVS
|
+-- forecast_record.h     # Structure de donnees meteo
|
+-- lang.h                # Systeme multi-langue (ES/EN/FR)
|
+-- touch_handler.h       # Controleur tactile GT911
|
+-- weather_history.h     # Systeme d'historique (SD + FFat + PSRAM)
|
+-- opensans*.h           # Polices (6, 8B, 9B, 10B, 12B, 14B, 16B, 18B, 24B, 28B)
|
+-- moon.h                # Image de la lune
+-- sunrise.h             # Icone lever du soleil
+-- sunset.h              # Icone coucher du soleil
```

### 3.3 Structure de Donnees Meteo

```cpp
typedef struct {
  int      Dt;           // Timestamp Unix
  String   Period;       // Periode de temps
  String   Icon;         // Code icone (01d, 02n, etc.)
  String   Trend;        // Tendance pression (+, -, 0)
  String   Main0;        // Condition principale
  String   Forecast0;    // Description meteo
  String   Description;  // Description detaillee
  float    Temperature;  // Temperature actuelle
  float    Feelslike;    // Temperature ressentie
  float    Humidity;     // Humidite %
  float    High;         // Temperature max
  float    Low;          // Temperature min
  float    Winddir;      // Direction du vent (degres)
  float    Windspeed;    // Vitesse du vent
  float    Rainfall;     // Pluie mm
  float    Snowfall;     // Neige mm
  float    Pop;          // Probabilite de precipitation
  float    Pressure;     // Pression atmospherique hPa
  int      Cloudcover;   // Couverture nuageuse %
  int      Visibility;   // Visibilite metres
  int      Sunrise;      // Timestamp Unix lever
  int      Sunset;       // Timestamp Unix coucher
  int      Timezone;     // Decalage fuseau horaire
} Forecast_record_type;
```

### 3.4 Stockage de Configuration (NVS)

La configuration est stockee dans l'espace "weather" des Preferences ESP32:

| Cle | Type | Description |
|-----|------|-------------|
| ssid1, pass1 | String | Reseau WiFi principal |
| ssid2, pass2 | String | Reseau WiFi secondaire |
| ssid3, pass3 | String | Reseau WiFi tertiaire |
| apikey | String | Cle API OpenWeatherMap |
| fcdays | Int | Jours de prevision (3 ou 5) |
| city | String | Nom de la ville |
| lat, lon | String | Coordonnees geographiques |
| lang | String | Langue (ES, EN, FR) |
| hemi | String | Hemisphere (north, south) |
| units | String | Unites (M=metrique, I=imperial) |
| tz | String | Fuseau horaire POSIX |
| gmt | Int | Decalage GMT en secondes |
| dst | Int | Decalage heure d'ete |
| updint | Int | Intervalle mise a jour (min) |
| sleept | Int | Temps avant veille (sec) |
| keepscr | Bool | Garder ecran actuel en veille |
| wakeuph | Int | Heure debut activite (0-23) |
| sleeph | Int | Heure fin activite (0-23) |

---

## 4. Installation et Compilation

### 4.1 Prerequis Logiciels

#### Arduino IDE
- Version 1.8.x ou 2.x

#### Board Manager
- **URL**: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- **Package**: esp32 by Espressif Systems **version 2.0.17**

#### Bibliotheques Requises
Installez uniquement ces deux bibliotheques dans le dossier `libraries`:

1. **EPD47-master**
   - URL: https://github.com/DFRobotdl/EPD47/archive/refs/heads/master.zip
   - Extraire comme `EPD47-master`

2. **ArduinoJson**
   - Auteur: Benoit Blanchon
   - Version: 6.19.0

> **IMPORTANT**: N'installez pas d'autres bibliotheques pour eviter les conflits.

### 4.2 Configuration Arduino IDE

Dans `Outils`, configurer:

| Parametre | Valeur |
|-----------|--------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enable |
| USB DFU On Boot | Disable |
| Flash Size | 16MB (128Mb) |
| Flash Mode | QIO 80MHz |
| Partition Scheme | 16M Flash (3M APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0/Hardware CDC |
| USB Mode | Hardware CDC and JTAG |

### 4.3 Televersement du Firmware

1. Connecter l'appareil via USB
2. Selectionner le bon port COM
3. Cliquer sur Televerser

#### Mode Bootloader Force

Si le televersement echoue, entrer en mode bootloader:

1. **Appuyer et maintenir** le bouton BOOT (IO0)
2. **Tout en maintenant** BOOT, appuyer sur RST
3. **Relacher** RST
4. **Relacher** BOOT
5. Reessayer le televersement

---

## 5. Configuration

### 5.1 Configuration par Fichier (owm_credentials.h)

Valeurs par defaut pouvant etre remplacees via web:

```cpp
// Reseaux WiFi (jusqu'a 3)
const WiFiCredentials wifiNetworks[] = {
  {"MonReseauPrincipal", "motdepasse123"},
  {"ReseauSecondaire", "autremotdepasse"},
  {"ReseauTertiaire", "troisiemedepasse"},
};

// API OpenWeatherMap
String apikey = "votre_cle_api_ici";

// API Groq (pour Meteo Narrative - gratuit sur console.groq.com)
String groq_apikey = "votre_cle_groq_ici";

// Emplacement
String City      = "Paris";
String Latitude  = "48.8566";
String Longitude = "2.3522";

// Preferences
String Language   = "FR";        // ES, EN, FR
String Hemisphere = "north";     // north, south
String Units      = "M";         // M=metrique, I=imperial

// Fuseau horaire
const char* Timezone    = "CET-1CEST";
int gmtOffset_sec       = 3600;   // +1 heure
int daylightOffset_sec  = 3600;   // Heure d'ete
```

### 5.2 Configuration Portail Web (Mode AP)

#### Acces au Mode AP

Le mode AP s'active automatiquement quand:
- Aucun reseau WiFi configure n'est disponible
- `FORCE_AP_MODE = true` est defini dans le code

#### Donnees de Connexion
| Parametre | Valeur |
|-----------|--------|
| SSID | WeatherStation-Setup |
| Mot de passe | weather123 |
| URL | http://192.168.4.1 |

#### Ecran Mode AP

Quand le mode AP est active, l'ecran affiche:

```
    WiFi Setup Mode

    Connect to WiFi network:
    WeatherStation-Setup

    Password: weather123

    Then open in browser:
    http://192.168.4.1
```

### 5.3 Page de Configuration Web

La page web est organisee en 4 onglets:

#### Onglet 1: WiFi
- **Reseau Principal**: SSID et mot de passe
- **Reseau Secondaire**: SSID et mot de passe (optionnel)
- **Reseau Tertiaire**: SSID et mot de passe (optionnel)

#### Onglet 2: Meteo
- **Cle API**: Cle OpenWeatherMap
- **Jours de Prevision**: 3 ou 5 jours
- **Ville**: Nom a afficher
- **Latitude/Longitude**: Coordonnees exactes
- **Hemisphere**: Nord ou Sud (affecte phases lunaires)

#### Onglet 3: Affichage
- **Langue**: Espagnol, Anglais, Francais
- **Unites**: Metrique (C, m/s, hPa) ou Imperial (F, mph, inHg)
- **Fuseau horaire**: Fuseau POSIX (ex: CET-1CEST)
- **Decalage GMT**: Decalage en secondes
- **Decalage Heure d'ete**: Decalage en secondes

#### Onglet 4: Systeme
- **Intervalle de Mise a Jour**: 5-120 minutes
- **Temps avant Veille**: 10-300 secondes
- **En veille**: Retour ecran principal ou garder ecran actuel
- **Heure Debut (reveil)**: Heure a partir de laquelle mettre a jour (0-23)
- **Heure Fin (veille)**: Heure a partir de laquelle arreter les mises a jour (0-23)
- **Style Narratif**: Style du texte genere par IA (voir section 12.5.5)
- **Enregistrer**: Enregistre uniquement les modifications
- **Enregistrer et Redemarrer**: Enregistre et applique les modifications
- **Reinitialisation Usine**: Efface toute la configuration

### 5.4 Fuseaux Horaires Courants

| Ville | Fuseau | Decalage GMT |
|-------|--------|--------------|
| Mexico | CST6 | -21600 |
| New York | EST5EDT | -18000 |
| Los Angeles | PST8PDT | -28800 |
| Madrid | CET-1CEST | 3600 |
| Londres | GMT0BST | 0 |
| Tokyo | JST-9 | 32400 |
| Sydney | AEST-10AEDT | 36000 |

---

## 6. Utilisation de l'Appareil

### 6.1 Demarrage Normal

1. **Mise sous tension** - L'appareil demarre automatiquement
2. **Connexion WiFi** - Recherche des reseaux configures
3. **Synchronisation** - Obtient l'heure via NTP
4. **Donnees meteo** - Telecharge depuis OpenWeatherMap
5. **Historique** - Enregistre la lecture dans l'historique local
6. **Affichage** - Montre l'ecran configure (principal ou dernier visite)
7. **Serveur web** - Disponible pour configuration a http://[IP_LOCALE]
8. **Navigation** - 30 secondes pour interagir
9. **Veille** - Entre en mode basse consommation

### 6.2 Cycle de Mise a Jour

```
[REVEIL] --> [WiFi] --> [NTP] --> [API] --> [Affichage] --> [Veille]
    ^                                                          |
    |                                                          |
    +------ (intervalle configure, ex: 30 minutes) ------------+
```

### 6.3 Indicateurs a l'Ecran

#### Barre d'Etat (coin superieur droit)

```
  [SD]  [Batterie]  [WiFi]
        85% 4,1v    -65dB
```

| Indicateur | Description |
|------------|-------------|
| SD | Icone carte SD (visible uniquement si SD inseree) |
| Batterie | Icone avec niveau de charge, pourcentage et tension |
| WiFi | Barres de signal (1-5) avec valeur RSSI en dB |

---

## 7. Ecrans de Navigation

### 7.1 Systeme de Navigation Tactile

L'appareil possede **11 ecrans navigables** en touchant differentes zones:

```
Ecran Principal (SCREEN_MAIN)
+-------------------------------------+
| [Ville]        [SD][Bat][WiFi]<-|  Zone Info (coin sup-droit)
| [Date] @ [Heure]                |  -> Ecran 4: Info Systeme
+==================================+
|                                  |
| [Rose Vents]   [Temp] [Hum]     |  Zone superieure
|                [Max/Min]        |  -> Ecran 1: Conditions
| [Lune][Sol] <- [Description]    |     Actuelles
|  Calendrier    [ICONE]          |  Zone Lune/Sol -> Calendrier
|                                  |
+==================================+
| [+3h] [+6h] [+9h] ... [+21h]    |  Zone centrale (previsions horaires)
|  Previsions horaires avec icones |  -> Ecran 2: Previsions
+================+=================+
| [Temp] [Pres]  | [Hum] [Pluie]  |  Zone inferieure DIVISEE:
|   Graphiques   |   Graphiques   |  Gauche -> Ecran 5: Historique
|   (3 jours)    |   (3 jours)    |  Droite -> Ecran 3: Tendances
+----------------+-----------------+
```

**Retour**: Toucher n'importe quelle partie d'un ecran secondaire retourne au principal.

### 7.2 Ecran Principal

```
+--[Ville]--------------[SD][Batterie][WiFi]--+
|                            85% 4,1v         |
|  [Date]  @ [Heure]                          |
+----------------------------------------------+
|                                              |
| [Rose des Vents]  [Temperature] [Humidite]   |
|     [Direction]   [Max/Min]                  |
|     [Vitesse]     [Description meteo]        |
|                   [Ressenti]                 |
|                                              |
| [Lune]  [Lever]                  [ICONE]     |
| [Phase] [Coucher]                [Grande]    |
|                                              |
+----------------------------------------------+
| [+3h] [+6h] [+9h] [+12h] [+15h] [+18h] [+21h]|
|  Previsions horaires (7 colonnes)            |
+----------------------------------------------+
| [Temperature] [Pression] [Humidite] [Pluie]  |
|    4 mini-graphiques de tendances (3 jours)  |
+----------------------------------------------+
```

### 7.3 Ecran 1: Conditions Actuelles

**Acces**: Toucher la zone superieure de l'ecran principal (zone temperature/meteo).

### 7.4 Ecran 2: Previsions Etendues

**Acces**: Toucher la zone centrale de l'ecran principal (zone previsions horaires).

### 7.5 Ecran 3: Tendances Meteo

**Acces**: Toucher la moitie DROITE des graphiques de l'ecran principal.

**Note**: Ces graphiques montrent des TENDANCES (predictions futures) de l'API OpenWeatherMap, pas des donnees historiques reelles.

### 7.6 Ecran 4: Information Systeme

**Acces**: Toucher le coin superieur droit (zone batterie/WiFi).

La section Information comprend **5 sous-ecrans navigables** (0/4 a 4/4).

### 7.7 Ecran 5: Historique des Donnees

**Acces**: Toucher la moitie GAUCHE des graphiques de l'ecran principal.

Affiche les donnees historiques reelles enregistrees par l'appareil.

### 7.8 Ecran 6: Qualite de l'Air

**Acces**: Toucher la zone **IQA** dans l'ecran Conditions Actuelles.

---

## 8. API OpenWeatherMap

### 8.1 Obtention d'une Cle API

1. Aller sur https://openweathermap.org/
2. Creer un compte gratuit
3. Aller dans "API Keys" dans le profil
4. Copier ou generer une nouvelle cle

### 8.2 Limites du Plan Gratuit

| Caracteristique | Limite |
|-----------------|--------|
| Appels/minute | 60 |
| Appels/mois | 1 000 000 |
| Donnees historiques | Non |
| Previsions | 5 jours / 3 heures |

### 8.3 Points d'Acces Utilises

#### Meteo Actuelle
```
GET /data/2.5/weather
Parametres:
  lat={latitude}
  lon={longitude}
  appid={cle_api}
  units=metric|imperial
  lang={code_langue}
```

#### Previsions
```
GET /data/2.5/forecast
Parametres:
  lat={latitude}
  lon={longitude}
  appid={cle_api}
  units=metric|imperial
  lang={code_langue}
  cnt=40  (8 lectures/jour x 5 jours)
```

---

## 9. Gestion de l'Energie

### 9.1 Modes de Fonctionnement

| Mode | Consommation | Description |
|------|--------------|-------------|
| Actif | ~150mA | WiFi + Affichage + CPU |
| Affichage Eteint | ~80mA | WiFi + CPU |
| Veille Legere | ~2mA | CPU en pause |
| Veille Profonde | ~10uA | RTC uniquement |

### 9.2 Cycle d'Energie Typique

```
         150mA        0mA (e-paper)      10uA
           |            |                  |
[Actif]----+--[Affich.]+---[Veille Prof]---+
  ~15s        ~0,5s       ~30 minutes

Moyenne: ~0,1mA (avec batterie 2000mAh = ~20 000 heures)
```

### 9.3 Surveillance Batterie

- **Broche ADC**: GPIO 14
- **Formule**: `voltage = analogRead(14) / 4096.0 * 6.566 * (vref / 1000.0)`
- **Plage**: 3,2V (0%) a 4,2V (100%)

---

## 10. Boitier Imprime en 3D

### 10.1 Modele Recommande

**Simple case for LilyGO T5 e-Paper 4.7" ESP32-S3 Board [H716]**
- **Auteur**: n602
- **URL**: https://www.thingiverse.com/thing:6897183
- **Licence**: Creative Commons BY-NC-SA 4.0

---

## 11. Resolution de Problemes

### 11.1 Ne Se Connecte Pas au WiFi

**Symptomes**: Entre en mode AP a chaque fois

**Solutions**:
1. Verifier que le SSID est ecrit exactement pareil
2. Verifier le mot de passe
3. S'assurer que le reseau est en 2,4GHz (pas 5GHz)
4. Rapprocher l'appareil du routeur

### 11.2 Pas de Donnees Meteo

**Symptomes**: Ecran vide ou "?"

**Solutions**:
1. Verifier la Cle API OpenWeatherMap
2. Verifier les coordonnees de localisation
3. Verifier la connexion internet
4. Consulter le Moniteur Serie pour les erreurs

### 11.3 Ghosting a l'Ecran

**Symptomes**: Images precedentes visibles

**Solutions**:
1. Utiliser le bouton [NETTOYER ECRAN] dans l'ecran Info Systeme
2. Le prochain rafraichissement complet le nettoiera
3. Redemarrer l'appareil pour forcer le nettoyage

### 11.4 Tactile Ne Repond Pas

**Symptomes**: Ne navigue pas entre les ecrans

**Solutions**:
1. Verifier dans Serial que GT911 a ete detecte
2. Nettoyer l'ecran de la poussiere/graisse
3. Toucher fermement pendant au moins 50ms
4. Attendre 500ms entre les touches (debounce)

### 11.5 Ne Televerse Pas le Firmware

**Symptomes**: Erreur de connexion dans Arduino IDE

**Solutions**:
1. Utiliser le mode bootloader force (section 4.3)
2. Essayer un autre cable USB
3. Verifier que les pilotes USB sont installes
4. Fermer le Moniteur Serie avant de televerser

---

## 12. Fonctions Speciales

### 12.1 Calendrier

L'appareil inclut un systeme de calendrier complet avec vues mensuelle et annuelle.

#### 12.1.1 Acces au Calendrier

Depuis l'ecran principal:

1. **Localiser la zone d'activation**: Zone icone lune, lever et coucher du soleil (coin superieur gauche)
2. **Toucher la zone**: Un toucher normal active le calendrier
3. **Activation**: L'ecran change vers le calendrier mensuel

#### 12.1.2 Calendrier Mensuel

Affiche un mois complet avec:

| Element | Description |
|---------|-------------|
| Titre | Nom du mois et annee (ex: "Mars 2026") |
| Fleches doubles | Navigation entre mois (gauche/droite) |
| En-tetes | LUN, MAR, MER, JEU, VEN, SAM, DIM |
| Jours | Numeros de 1 a 28/29/30/31 selon le mois |
| Jour actuel | Marque avec double soulignement |
| Semaine | Commence le lundi |

#### 12.1.3 Calendrier Annuel

Affiche 12 mois dans une grille 3 colonnes x 4 lignes.

### 12.2 Carte SD

L'appareil supporte le stockage etendu via carte MicroSD.

#### 12.2.1 Caracteristiques

| Caracteristique | Description |
|-----------------|-------------|
| Format | FAT32 (jusqu'a 32GB) ou exFAT (64GB+) |
| Fichier | `/weather_history.csv` |
| Capacite | ~52 560 lectures (~1 an a 10 min) |
| Repli | Si pas de SD, utilise FFat interne (~7 jours) |

#### 12.2.2 Format du Fichier CSV

```csv
timestamp,temp,humidity,pressure,wind_speed,wind_dir,rain,description
1710000000,22.5,65,1013,5.2,180,0.0,Partiellement nuageux
```

### 12.3 Acces Web

L'appareil inclut un serveur web pour la configuration et l'acces aux donnees.

#### 12.3.1 Points d'Acces Disponibles

| Point d'acces | Description |
|---------------|-------------|
| `http://[IP]/` | Page de configuration (4 onglets) |
| `http://[IP]/history` | 50 dernieres lectures en tableau HTML |
| `http://[IP]/history.csv` | Telecharger CSV complet (necessite SD) |

### 12.4 Configuration Bluetooth

L'appareil supporte la configuration via Bluetooth Low Energy (BLE) en utilisant une application Android.

#### 12.4.1 Activation du Mode BLE

1. Naviguer vers l'ecran **Configuration Systeme** (Info)
2. Toucher le bouton **Bluetooth** (coin inferieur gauche)
3. L'ecran affichera "CONFIGURATION BLUETOOTH"
4. L'appareil apparaitra comme **WeatherStation-BLE**

### 12.5 Ecrans Caches

#### 12.5.1 Chemin d'Acces

```
Ecran Principal ---------> Meteo Narrative (IA)
      |                    (toucher grande icone centre)
      v
Info (batterie/WiFi) -----> Features 1 -----> Features 2
                                                  |
                                                  v
                                             Help -----> Credits (QR)
                                                              |
                                                              v
                                                         Callsign (XE1E)
                                                              |
                                                              v
                                                         Horloge Mondiale
                                                              |
                                                              v
                                                    Pensee du Jour
```

#### 12.5.2 Ecran Callsign

Acces: Credits (QR) -> toucher zone inferieure

Affiche l'indicatif radioamateur de l'auteur.

#### 12.5.3 Ecran Horloge Mondiale

Acces: Callsign -> toucher zone inferieure

Affiche une carte du monde avec les fuseaux horaires.

#### 12.5.4 Ecran Pensee du Jour

Acces: Horloge Mondiale -> toucher cote droit

Affiche une citation inspirante quotidienne obtenue d'internet:

| Element | Description |
|---------|-------------|
| Cadre | Decoratif avec coins et bordures elegants |
| Titre | "~ Pensee du Jour ~" |
| Citation | Texte de la citation |
| Auteur | Nom de l'auteur de la citation |
| Pied | "Touchez pour revenir" |

Caracteristiques:
- Citation du jour (API: frasedeldia.azurewebsites.net)
- Meme citation affichee toute la journee, change toutes les 24 heures
- Persiste en veille profonde
- Format elegant avec guillemets decoratifs

#### 12.5.5 Ecran Meteo Narrative (IA)

Acces: Ecran principal -> toucher grande icone meteo (centre de l'ecran)

Genere une description naturelle de la meteo en utilisant l'intelligence artificielle (Groq/Llama):

| Element | Description |
|---------|-------------|
| Cadre | Decoratif avec coins et bordures elegants |
| Titre | "~ La Meteo d'Aujourd'hui ~" |
| Ville | Nom de l'emplacement configure |
| Narratif | Description meteo en 4-5 phrases |
| Pied | "Touchez pour revenir" |

##### Styles de Narratif

Le style se configure depuis l'interface web (Section Systeme -> Meteo Narrative):

| Style | Description | Exemple |
|-------|-------------|---------|
| Radio | Bulletin radio, concis | "Bonjour chers auditeurs, nous avons 22 degres..." |
| Formel | Comme journal TV | "Les conditions meteorologiques actuelles indiquent..." |
| Poetique | Metaphores litteraires | "Le soleil baigne la ville de son etreinte chaleureuse..." |
| Technique | Style meteorologique | "Systeme de haute pression. Temperature: 22C..." |
| Humoristique | Avec touches d'humour | "Ni trop froid ni trop chaud, le temps hesite..." |
| Grand-mere | Conseils de grand-mere | "Mon petit, mets un gilet, il fait 22 degres dehors..." |

##### Comment Configurer

1. Acceder a l'interface web:
   - Reseau AP: `WeatherStation-Setup` (mdp: `weather123`) -> `http://192.168.4.1`
   - Ou utiliser l'IP de l'appareil s'il est deja connecte a votre reseau
2. Aller a l'onglet **Systeme**
3. Trouver la section **"Meteo Narrative (IA)"**
4. Selectionner le style desire dans le menu deroulant
5. Enregistrer la configuration

##### Donnees Utilisees

L'IA genere le texte en utilisant les donnees reelles d'OpenWeatherMap (n'invente pas de chiffres):
- Temperature actuelle et ressenti
- Humidite et pression atmospherique
- Condition du ciel (nuageux, ensoleille, etc.)
- Vitesse du vent
- Previsions des prochaines heures

##### Prerequis

- Connexion WiFi active (pour appeler l'API Groq)
- Cle API Groq configuree dans `owm_credentials.h` ou via web
- Obtenir une cle gratuite sur: https://console.groq.com

##### Caracteristiques

- Le texte est genere a chaque entree dans l'ecran
- **Multilingue**: Genere dans la langue configuree (ES/EN/FR)
- Persiste en veille profonde
- Format elegant avec cadre decoratif
- Police OpenSans10B pour meilleure lisibilite
- Jusqu'a 9 lignes de ~69 caracteres chacune
- Modele IA: Llama 3.1 8B (rapide et gratuit)

Navigation:
- Toucher n'importe quelle zone -> Retour a l'ecran principal

---

## 13. Annexe

### 13.1 Polices Disponibles

| Nom | Taille | Utilisation |
|-----|--------|-------------|
| OpenSans6 | 6pt | Texte tres petit |
| OpenSans8B | 8pt | Petit texte, indices, uptime |
| OpenSans9B | 9pt | Etiquettes graphiques |
| OpenSans10B | 10pt | Date/heure, details, info systeme |
| OpenSans12B | 12pt | Texte moyen, boutons |
| OpenSans14B | 14pt | Ville, descriptions |
| OpenSans18B | 18pt | Titres d'ecrans |
| OpenSans24B | 24pt | Temperature, donnees principales |
| OpenSans28B | 28pt | Nombres extra grands |

### 13.2 Coordonnees d'Ecran

```
(0,0)--------------------------(960,0)
  |                               |
  |        960 x 540 pixels       |
  |                               |
(0,540)-----------------------(960,540)
```

### 13.3 Directions du Vent

Les abreviations varient selon la langue configuree:

| Degres | Espagnol | Anglais | Francais |
|--------|----------|---------|----------|
| 0 / 360 | N | N | N |
| 45 | NE | NE | NE |
| 90 | E | E | E |
| 135 | SE | SE | SE |
| 180 | S | S | S |
| 225 | SO | SW | SO |
| 270 | O | W | O |
| 315 | NO | NW | NO |

### 13.4 Phases Lunaires

| Phase | Illumination | Nom FR | Nom ES |
|-------|--------------|--------|--------|
| 0 | 0% | Nouvelle Lune | Luna Nueva |
| 1 | 25% | Premier Croissant | Luna Creciente |
| 2 | 50% | Premier Quartier | Cuarto Creciente |
| 3 | 75% | Gibbeuse Croissante | Gibosa Creciente |
| 4 | 100% | Pleine Lune | Luna Llena |
| 5 | 75% | Gibbeuse Decroissante | Gibosa Menguante |
| 6 | 50% | Dernier Quartier | Cuarto Menguante |
| 7 | 25% | Dernier Croissant | Luna Menguante |

### 13.5 Textes Multi-langues

Le systeme supporte 3 langues configurables:

| Texte | Espagnol | Anglais | Francais |
|-------|----------|---------|----------|
| Conditions | Condiciones Actuales | Current Conditions | Conditions Actuelles |
| Previsions | Pronostico 5 Dias | 5-Day Forecast | Previsions 5 Jours |
| Tendances | Tendencias del Clima | Weather Trends | Tendances Meteo |
| Historique | Historial | History | Historique |
| Info Systeme | Info del Sistema | System Info | Info Systeme |
| Lever | Amanecer | Sunrise | Lever |
| Coucher | Anochecer | Sunset | Coucher |
| Humidite | Humedad | Humidity | Humidite |
| Pression | Presion | Pressure | Pression |
| Vent | Viento | Wind | Vent |
| Nettoyer | Limpiar Pantalla | Clean Screen | Nettoyer Ecran |
| 48 Heures | 48H | 48H | 48H |
| 1 Semaine | 1 Semana | 1 Week | 1 Semaine |

### 13.6 Licence et Credits

**Logiciel Original**: David Bird 2021
**Modifications**: XE1E 2024

Ce projet est open source. Consultez le depot pour les details de licence.

---

*Manuel Version 2.1 - Mars 2026*
