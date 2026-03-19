package net.xe1e.weatherstation

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import com.google.gson.Gson
import net.xe1e.weatherstation.databinding.ActivityConfigBinding

class ConfigActivity : AppCompatActivity(), BleManager.Callback {

    private lateinit var binding: ActivityConfigBinding
    private val bleManager: BleManager? get() = BleHolder.bleManager
    private val handler = Handler(Looper.getMainLooper())
    private var pendingWrites = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityConfigBinding.inflate(layoutInflater)
        setContentView(binding.root)

        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.title = "Configuracion"

        bleManager?.setCallback(this)

        setupSpinners()
        setupButtons()

        // Read current config from device
        handler.postDelayed({
            bleManager?.readConfig()
            updateStatus("Leyendo configuracion...")
        }, 500)
    }

    private fun setupSpinners() {
        // Language spinner
        ArrayAdapter.createFromResource(
            this,
            R.array.languages,
            android.R.layout.simple_spinner_item
        ).also { adapter ->
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            binding.spinnerLanguage.adapter = adapter
        }

        // Hemisphere spinner
        ArrayAdapter.createFromResource(
            this,
            R.array.hemispheres,
            android.R.layout.simple_spinner_item
        ).also { adapter ->
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            binding.spinnerHemisphere.adapter = adapter
        }

        // Units spinner
        ArrayAdapter.createFromResource(
            this,
            R.array.units,
            android.R.layout.simple_spinner_item
        ).also { adapter ->
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
            binding.spinnerUnits.adapter = adapter
        }
    }

    private fun setupButtons() {
        binding.btnSendConfig.setOnClickListener {
            sendAllConfig()
        }

        binding.btnTestWifi.setOnClickListener {
            bleManager?.testWifi()
            updateStatus("Probando WiFi...")
        }

        binding.btnSave.setOnClickListener {
            bleManager?.saveConfig()
            updateStatus("Guardando...")
        }

        binding.btnRestart.setOnClickListener {
            bleManager?.restart()
            updateStatus("Reiniciando dispositivo...")
            handler.postDelayed({ finish() }, 2000)
        }

        binding.btnReadConfig.setOnClickListener {
            bleManager?.readConfig()
            updateStatus("Leyendo configuracion...")
        }
    }

    private fun sendAllConfig() {
        val ble = bleManager ?: return
        pendingWrites = 0

        // WiFi 1
        val ssid1 = binding.etWifi1Ssid.text.toString()
        val pass1 = binding.etWifi1Pass.text.toString()
        if (ssid1.isNotEmpty()) {
            ble.writeWifi1(ssid1, pass1)
            pendingWrites++
            delay(200)
        }

        // WiFi 2
        val ssid2 = binding.etWifi2Ssid.text.toString()
        val pass2 = binding.etWifi2Pass.text.toString()
        if (ssid2.isNotEmpty()) {
            handler.postDelayed({ ble.writeWifi2(ssid2, pass2) }, 200)
            pendingWrites++
        }

        // WiFi 3
        val ssid3 = binding.etWifi3Ssid.text.toString()
        val pass3 = binding.etWifi3Pass.text.toString()
        if (ssid3.isNotEmpty()) {
            handler.postDelayed({ ble.writeWifi3(ssid3, pass3) }, 400)
            pendingWrites++
        }

        // API Key
        val apiKey = binding.etApiKey.text.toString()
        if (apiKey.isNotEmpty()) {
            handler.postDelayed({ ble.writeApiKey(apiKey) }, 600)
            pendingWrites++
        }

        // City
        val city = binding.etCity.text.toString()
        if (city.isNotEmpty()) {
            handler.postDelayed({ ble.writeCity(city) }, 800)
            pendingWrites++
        }

        // Location
        val lat = binding.etLatitude.text.toString()
        val lon = binding.etLongitude.text.toString()
        if (lat.isNotEmpty() && lon.isNotEmpty()) {
            handler.postDelayed({ ble.writeLocation(lat, lon) }, 1000)
            pendingWrites++
        }

        // Language & Hemisphere
        val langIndex = binding.spinnerLanguage.selectedItemPosition
        val lang = arrayOf("es", "en", "fr")[langIndex]
        val hemiIndex = binding.spinnerHemisphere.selectedItemPosition
        val hemi = arrayOf("north", "south")[hemiIndex]
        handler.postDelayed({ ble.writeLanguage(lang, hemi) }, 1200)
        pendingWrites++

        // Units
        val unitsIndex = binding.spinnerUnits.selectedItemPosition
        val units = arrayOf("M", "I")[unitsIndex]
        handler.postDelayed({ ble.writeUnits(units) }, 1400)
        pendingWrites++

        // Timezone
        val tz = binding.etTimezone.text.toString()
        val gmt = binding.etGmtOffset.text.toString().toIntOrNull() ?: -21600
        val dst = binding.etDstOffset.text.toString().toIntOrNull() ?: 0
        if (tz.isNotEmpty()) {
            handler.postDelayed({ ble.writeTimezone(tz, gmt, dst) }, 1600)
            pendingWrites++
        }

        // Intervals
        val updateInt = binding.etUpdateInterval.text.toString().toIntOrNull() ?: 30
        val fcDays = binding.etForecastDays.text.toString().toIntOrNull() ?: 3
        handler.postDelayed({ ble.writeIntervals(updateInt, fcDays) }, 1800)
        pendingWrites++

        // Sleep settings
        val sleepTimeout = binding.etSleepTimeout.text.toString().toIntOrNull() ?: 30
        val keepScreen = binding.cbKeepScreen.isChecked
        val wakeupHour = binding.etWakeupHour.text.toString().toIntOrNull() ?: 7
        val sleepHour = binding.etSleepHour.text.toString().toIntOrNull() ?: 23
        handler.postDelayed({ ble.writeSleep(sleepTimeout, keepScreen, wakeupHour, sleepHour) }, 2000)
        pendingWrites++

        updateStatus("Enviando configuracion...")
    }

    private fun delay(ms: Long) {
        Thread.sleep(ms)
    }

    private fun updateStatus(status: String) {
        binding.tvStatus.text = status
    }

    private fun populateFields(json: String) {
        try {
            val config = Gson().fromJson(json, DeviceConfig::class.java)

            binding.etWifi1Ssid.setText(config.wifi1 ?: "")
            binding.etWifi2Ssid.setText(config.wifi2 ?: "")
            binding.etWifi3Ssid.setText(config.wifi3 ?: "")
            binding.etCity.setText(config.city ?: "")
            binding.etLatitude.setText(config.lat ?: "")
            binding.etLongitude.setText(config.lon ?: "")

            // Language
            val langIndex = when (config.lang) {
                "en" -> 1
                "fr" -> 2
                else -> 0 // es
            }
            binding.spinnerLanguage.setSelection(langIndex)

            // Hemisphere
            val hemiIndex = if (config.hemi == "south") 1 else 0
            binding.spinnerHemisphere.setSelection(hemiIndex)

            // Units
            val unitsIndex = if (config.units == "I") 1 else 0
            binding.spinnerUnits.setSelection(unitsIndex)

            binding.etTimezone.setText(config.tz ?: "")
            binding.etGmtOffset.setText((config.gmt ?: -21600).toString())
            binding.etDstOffset.setText((config.dst ?: 0).toString())
            binding.etUpdateInterval.setText((config.updint ?: 30).toString())
            binding.etForecastDays.setText((config.fcdays ?: 3).toString())
            binding.etSleepTimeout.setText((config.sleept ?: 30).toString())
            binding.cbKeepScreen.isChecked = config.keepscr ?: false
            binding.etWakeupHour.setText((config.wakeuph ?: 7).toString())
            binding.etSleepHour.setText((config.sleeph ?: 23).toString())

            updateStatus("Configuracion cargada")
        } catch (e: Exception) {
            updateStatus("Error al leer: ${e.message}")
        }
    }

    // BleManager.Callback
    override fun onDeviceFound(device: android.bluetooth.BluetoothDevice) {}
    override fun onConnected() {}
    override fun onDisconnected() {
        runOnUiThread {
            Toast.makeText(this, "Desconectado", Toast.LENGTH_SHORT).show()
            finish()
        }
    }
    override fun onServicesDiscovered() {}

    override fun onStatusReceived(status: String) {
        runOnUiThread {
            updateStatus("Estado: $status")
            if (status == "SAVED") {
                Toast.makeText(this, "Configuracion guardada", Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onConfigReceived(config: String) {
        runOnUiThread {
            populateFields(config)
        }
    }

    override fun onError(message: String) {
        runOnUiThread {
            Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
        }
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }
}

// Data class for JSON parsing
data class DeviceConfig(
    val wifi1: String?,
    val wifi2: String?,
    val wifi3: String?,
    val city: String?,
    val lat: String?,
    val lon: String?,
    val lang: String?,
    val hemi: String?,
    val units: String?,
    val tz: String?,
    val gmt: Int?,
    val dst: Int?,
    val updint: Int?,
    val fcdays: Int?,
    val sleept: Int?,
    val keepscr: Boolean?,
    val wakeuph: Int?,
    val sleeph: Int?
)
