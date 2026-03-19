# Weather Station BLE ProGuard Rules

# Keep Gson classes
-keepattributes Signature
-keepattributes *Annotation*
-keep class net.xe1e.weatherstation.DeviceConfig { *; }

# Keep BLE callbacks
-keep class net.xe1e.weatherstation.BleManager$* { *; }
