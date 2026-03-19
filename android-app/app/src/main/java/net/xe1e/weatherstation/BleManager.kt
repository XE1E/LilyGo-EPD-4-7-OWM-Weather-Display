package net.xe1e.weatherstation

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import android.util.Log
import java.util.*

@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {

    companion object {
        private const val TAG = "BleManager"

        // Service UUID
        val SERVICE_UUID: UUID = UUID.fromString("4fafc201-1fb5-459e-8fcc-c5c9c331914b")

        // Characteristic UUIDs
        val CHAR_WIFI1_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2601")
        val CHAR_WIFI2_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2602")
        val CHAR_WIFI3_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2603")
        val CHAR_API_KEY_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2610")
        val CHAR_CITY_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2611")
        val CHAR_LOCATION_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2612")
        val CHAR_LANGUAGE_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2620")
        val CHAR_UNITS_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2621")
        val CHAR_TIMEZONE_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2622")
        val CHAR_INTERVALS_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2630")
        val CHAR_SLEEP_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b2631")
        val CHAR_STATUS_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26f0")
        val CHAR_COMMAND_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26f1")
        val CHAR_CONFIG_READ_UUID: UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26f2")

        val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

        const val DEVICE_NAME = "WeatherStation-BLE"
    }

    interface Callback {
        fun onDeviceFound(device: BluetoothDevice)
        fun onConnected()
        fun onDisconnected()
        fun onServicesDiscovered()
        fun onStatusReceived(status: String)
        fun onConfigReceived(config: String)
        fun onError(message: String)
    }

    private val bluetoothAdapter: BluetoothAdapter? by lazy {
        val manager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        manager.adapter
    }

    private var bluetoothGatt: BluetoothGatt? = null
    private var callback: Callback? = null
    private val handler = Handler(Looper.getMainLooper())
    private var isScanning = false

    fun setCallback(cb: Callback) {
        callback = cb
    }

    fun isBluetoothEnabled(): Boolean = bluetoothAdapter?.isEnabled == true

    // Scanning
    fun startScan() {
        val scanner = bluetoothAdapter?.bluetoothLeScanner ?: return

        val filter = ScanFilter.Builder()
            .setServiceUuid(ParcelUuid(SERVICE_UUID))
            .build()

        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        isScanning = true
        scanner.startScan(listOf(filter), settings, scanCallback)

        // Stop scan after 30 seconds
        handler.postDelayed({ stopScan() }, 30000)
    }

    fun stopScan() {
        if (isScanning) {
            bluetoothAdapter?.bluetoothLeScanner?.stopScan(scanCallback)
            isScanning = false
        }
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            Log.d(TAG, "Found device: ${device.name} - ${device.address}")
            if (device.name == DEVICE_NAME) {
                stopScan()
                callback?.onDeviceFound(device)
            }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.e(TAG, "Scan failed: $errorCode")
            callback?.onError("Scan failed: $errorCode")
        }
    }

    // Connection
    fun connect(device: BluetoothDevice) {
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }

    fun disconnect() {
        bluetoothGatt?.disconnect()
        bluetoothGatt?.close()
        bluetoothGatt = null
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            handler.post {
                when (newState) {
                    BluetoothProfile.STATE_CONNECTED -> {
                        Log.d(TAG, "Connected")
                        callback?.onConnected()
                        gatt.discoverServices()
                    }
                    BluetoothProfile.STATE_DISCONNECTED -> {
                        Log.d(TAG, "Disconnected")
                        callback?.onDisconnected()
                    }
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            handler.post {
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    Log.d(TAG, "Services discovered")
                    enableNotifications()
                    callback?.onServicesDiscovered()
                } else {
                    callback?.onError("Service discovery failed")
                }
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            handler.post {
                val strValue = String(value)
                when (characteristic.uuid) {
                    CHAR_STATUS_UUID -> callback?.onStatusReceived(strValue)
                    CHAR_CONFIG_READ_UUID -> callback?.onConfigReceived(strValue)
                }
            }
        }

        @Deprecated("Deprecated in API 33")
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            handler.post {
                val strValue = String(characteristic.value)
                when (characteristic.uuid) {
                    CHAR_STATUS_UUID -> callback?.onStatusReceived(strValue)
                    CHAR_CONFIG_READ_UUID -> callback?.onConfigReceived(strValue)
                }
            }
        }
    }

    private fun enableNotifications() {
        val service = bluetoothGatt?.getService(SERVICE_UUID) ?: return

        // Enable status notifications
        service.getCharacteristic(CHAR_STATUS_UUID)?.let { char ->
            bluetoothGatt?.setCharacteristicNotification(char, true)
            char.getDescriptor(CCCD_UUID)?.let { desc ->
                desc.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                bluetoothGatt?.writeDescriptor(desc)
            }
        }

        // Enable config read notifications
        handler.postDelayed({
            service.getCharacteristic(CHAR_CONFIG_READ_UUID)?.let { char ->
                bluetoothGatt?.setCharacteristicNotification(char, true)
                char.getDescriptor(CCCD_UUID)?.let { desc ->
                    desc.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    bluetoothGatt?.writeDescriptor(desc)
                }
            }
        }, 500)
    }

    // Write functions
    private fun writeCharacteristic(uuid: UUID, value: String) {
        val service = bluetoothGatt?.getService(SERVICE_UUID) ?: return
        val char = service.getCharacteristic(uuid) ?: return
        char.value = value.toByteArray()
        bluetoothGatt?.writeCharacteristic(char)
    }

    fun writeWifi1(ssid: String, password: String) = writeCharacteristic(CHAR_WIFI1_UUID, "$ssid|$password")
    fun writeWifi2(ssid: String, password: String) = writeCharacteristic(CHAR_WIFI2_UUID, "$ssid|$password")
    fun writeWifi3(ssid: String, password: String) = writeCharacteristic(CHAR_WIFI3_UUID, "$ssid|$password")
    fun writeApiKey(key: String) = writeCharacteristic(CHAR_API_KEY_UUID, key)
    fun writeCity(city: String) = writeCharacteristic(CHAR_CITY_UUID, city)
    fun writeLocation(lat: String, lon: String) = writeCharacteristic(CHAR_LOCATION_UUID, "$lat|$lon")
    fun writeLanguage(lang: String, hemi: String) = writeCharacteristic(CHAR_LANGUAGE_UUID, "$lang|$hemi")
    fun writeUnits(units: String) = writeCharacteristic(CHAR_UNITS_UUID, units)
    fun writeTimezone(tz: String, gmt: Int, dst: Int) = writeCharacteristic(CHAR_TIMEZONE_UUID, "$tz|$gmt|$dst")
    fun writeIntervals(update: Int, forecastDays: Int) = writeCharacteristic(CHAR_INTERVALS_UUID, "$update|$forecastDays")
    fun writeSleep(timeout: Int, keepScreen: Boolean, wakeup: Int, sleep: Int) {
        val keep = if (keepScreen) 1 else 0
        writeCharacteristic(CHAR_SLEEP_UUID, "$timeout|$keep|$wakeup|$sleep")
    }

    // Commands
    fun sendCommand(cmd: String) = writeCharacteristic(CHAR_COMMAND_UUID, cmd)
    fun saveConfig() = sendCommand("SAVE")
    fun restart() = sendCommand("RESTART")
    fun testWifi() = sendCommand("TEST_WIFI")
    fun readConfig() = sendCommand("READ_CONFIG")
}
