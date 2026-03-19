package net.xe1e.weatherstation

import android.Manifest
import android.bluetooth.BluetoothDevice
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import net.xe1e.weatherstation.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity(), BleManager.Callback {

    private lateinit var binding: ActivityMainBinding
    private lateinit var bleManager: BleManager
    private var connectedDevice: BluetoothDevice? = null

    companion object {
        private const val REQUEST_PERMISSIONS = 100
        const val EXTRA_DEVICE = "device"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        bleManager = BleManager(this)
        bleManager.setCallback(this)

        binding.btnScan.setOnClickListener {
            if (checkPermissions()) {
                startScanning()
            }
        }

        binding.btnConnect.setOnClickListener {
            connectedDevice?.let { device ->
                updateStatus("Conectando...")
                bleManager.connect(device)
            }
        }

        binding.btnConfigure.setOnClickListener {
            startActivity(Intent(this, ConfigActivity::class.java))
        }

        binding.btnDisconnect.setOnClickListener {
            bleManager.disconnect()
            updateUI(connected = false)
        }

        updateUI(connected = false)
    }

    override fun onDestroy() {
        super.onDestroy()
        bleManager.disconnect()
    }

    private fun checkPermissions(): Boolean {
        val permissions = mutableListOf<String>()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                permissions.add(Manifest.permission.BLUETOOTH_SCAN)
            }
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
            }
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            permissions.add(Manifest.permission.ACCESS_FINE_LOCATION)
        }

        return if (permissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(this, permissions.toTypedArray(), REQUEST_PERMISSIONS)
            false
        } else {
            true
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_PERMISSIONS) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                startScanning()
            } else {
                Toast.makeText(this, "Permisos requeridos para BLE", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun startScanning() {
        if (!bleManager.isBluetoothEnabled()) {
            Toast.makeText(this, "Activa Bluetooth", Toast.LENGTH_SHORT).show()
            return
        }

        updateStatus("Buscando dispositivo...")
        binding.btnScan.isEnabled = false
        bleManager.startScan()
    }

    private fun updateStatus(status: String) {
        binding.tvStatus.text = status
    }

    private fun updateUI(connected: Boolean) {
        binding.btnConnect.isEnabled = connectedDevice != null && !connected
        binding.btnConfigure.isEnabled = connected
        binding.btnDisconnect.isEnabled = connected
        binding.btnScan.isEnabled = !connected

        if (!connected) {
            binding.tvDeviceName.text = connectedDevice?.name ?: "No encontrado"
            updateStatus("Desconectado")
        }
    }

    // BleManager.Callback
    override fun onDeviceFound(device: BluetoothDevice) {
        connectedDevice = device
        binding.tvDeviceName.text = device.name
        updateStatus("Dispositivo encontrado")
        binding.btnScan.isEnabled = true
        binding.btnConnect.isEnabled = true
    }

    override fun onConnected() {
        updateStatus("Conectado - Descubriendo servicios...")
    }

    override fun onDisconnected() {
        updateUI(connected = false)
    }

    override fun onServicesDiscovered() {
        updateStatus("Conectado y listo")
        updateUI(connected = true)
        // Store BLE manager reference for ConfigActivity
        BleHolder.bleManager = bleManager
    }

    override fun onStatusReceived(status: String) {
        updateStatus("Estado: $status")
    }

    override fun onConfigReceived(config: String) {
        // Handled in ConfigActivity
    }

    override fun onError(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
        binding.btnScan.isEnabled = true
    }
}

// Simple holder to share BLE manager between activities
object BleHolder {
    var bleManager: BleManager? = null
}
