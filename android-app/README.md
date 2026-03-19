# Weather Station BLE - Android App

**Estado: PENDIENTE** - App en desarrollo, pendiente pruebas de compilacion.

Aplicación Android para configurar la estación meteorológica LilyGo EPD 4.7" via Bluetooth Low Energy.

## Requisitos

- Android 8.0 (API 26) o superior
- Bluetooth Low Energy (BLE)
- Android Studio Hedgehog (2023.1.1) o superior

## Compilación

1. Abre el proyecto en Android Studio
2. Sincroniza Gradle
3. Ejecuta en dispositivo o emulador (con BLE)

```bash
./gradlew assembleDebug
```

### Notas de Compilación

**Versiones utilizadas:**
- Android Gradle Plugin: 8.1.0
- Gradle: 8.0
- Kotlin: 1.9.0
- Target SDK: 34

**Problemas conocidos:**

Si encuentras errores de `jlink.exe` o `JdkImageTransform`:
1. Verificar que JAVA_HOME apunte al JDK de Android Studio:
   ```
   set JAVA_HOME=C:\Program Files\Android\Android Studio\jbr
   ```
2. Limpiar cache de Gradle:
   ```bash
   ./gradlew clean
   rm -rf ~/.gradle/caches
   ```
3. Cerrar Android Studio y cualquier proceso de Gradle antes de recompilar

Si encuentras errores de Kotlin daemon:
- Ya está configurado `kotlin.compiler.execution.strategy=in-process` en gradle.properties

## Uso

### En el ESP32:
1. Navega a la pantalla **Info** (toca el icono de bateria/WiFi)
2. Toca el botón **Bluetooth** en la esquina inferior izquierda
3. La pantalla mostrará "CONFIGURACION BLUETOOTH"
4. El dispositivo aparecerá como **WeatherStation-BLE**

### En la App:
1. **Buscar Dispositivo** - Escanea dispositivos BLE cercanos
2. **Conectar** - Establece conexión con la estación
3. **Configurar** - Abre pantalla de configuración
4. Ingresa los parámetros deseados
5. **Enviar Configuración** - Envía todos los valores
6. **Guardar** - Guarda en memoria del dispositivo
7. **Reiniciar** - Reinicia para aplicar cambios

## Parámetros Configurables

### WiFi (hasta 3 redes)
- SSID y contraseña para cada red
- La estación conectará a la primera disponible

### API y Ubicación
- API Key de OpenWeatherMap
- Ciudad
- Latitud y Longitud

### Regional
- Idioma: Español, English, Français
- Hemisferio: Norte/Sur (afecta fase lunar)
- Unidades: Métrico/Imperial
- Zona horaria y offsets GMT/DST

### Comportamiento
- Intervalo de actualización (minutos)
- Días de pronóstico
- Timeout de navegación
- Horas de despertar/dormir

## Permisos Requeridos

- `BLUETOOTH_SCAN` - Buscar dispositivos BLE
- `BLUETOOTH_CONNECT` - Conectar a dispositivos
- `ACCESS_FINE_LOCATION` - Requerido para BLE en Android

## Estructura del Proyecto

```
app/
├── src/main/
│   ├── java/net/xe1e/weatherstation/
│   │   ├── MainActivity.kt       # Pantalla principal - escaneo y conexión
│   │   ├── ConfigActivity.kt     # Pantalla de configuración
│   │   └── BleManager.kt         # Manejo de Bluetooth LE
│   └── res/
│       ├── layout/
│       │   ├── activity_main.xml
│       │   └── activity_config.xml
│       └── values/
│           ├── strings.xml
│           └── themes.xml
```

## Licencia

MIT License - Ver archivo LICENSE en el proyecto principal.
