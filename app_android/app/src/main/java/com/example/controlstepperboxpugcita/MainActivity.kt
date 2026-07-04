package com.example.controlstepperboxpugcita

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import android.widget.ArrayAdapter
import android.widget.Spinner
import androidx.appcompat.app.AppCompatActivity
import com.android.volley.Request
import com.android.volley.toolbox.StringRequest
import com.android.volley.toolbox.Volley
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView

class MainActivity : AppCompatActivity() {

    private val IP_ESP32 = "http://192.168.4.1"
    private val TAG = "MecatronicaApp"

    private lateinit var btnConectar: Button
    private lateinit var spinnerCanciones: Spinner
    private lateinit var btnReproducir: Button
    private lateinit var btnPausa: Button
    private lateinit var btnDetener: Button
    private lateinit var seekBarVelocidad: SeekBar
    private lateinit var txtVelocidad: TextView

    private var listaDeCanciones: List<String> = ArrayList()

    // Mapeo del SeekBar: Posiciones de 0 a 3 mapeadas a floats de velocidad
    private val valoresVelocidad = floatArrayOf(0.5f, 1.0f, 1.5f, 2.0f)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Vincular componentes del XML
        btnConectar = findViewById(R.id.btnConectar)
        spinnerCanciones = findViewById(R.id.spinnerCanciones)
        btnReproducir = findViewById(R.id.btnReproducir)
        btnPausa = findViewById(R.id.btnPausa)
        btnDetener = findViewById(R.id.btnDetener)
        seekBarVelocidad = findViewById(R.id.seekBarVelocidad)
        txtVelocidad = findViewById(R.id.txtVelocidad)

        // Estado inicial de los botones de control (Congelados hasta conectar)
        setControlesActivos(false)

        // 1. Botón Conectar
        btnConectar.setOnClickListener { enviarComandoConectar() }

        // 2. Botón Play (Manda la canción seleccionada o reanuda si estaba en pausa)
        btnReproducir.setOnClickListener {
            if (listaDeCanciones.isNotEmpty()) {
                val cancionSeleccionada = spinnerCanciones.selectedItem.toString()
                enviarComandoDinamico("$IP_ESP32/reproducir?archivo=$cancionSeleccionada")
            }
        }

        // 3. Botón Pausa
        btnPausa.setOnClickListener {
            enviarComandoDinamico("$IP_ESP32/pausa")
        }

        // 4. Botón Stop
        btnDetener.setOnClickListener {
            enviarComandoDinamico("$IP_ESP32/detener")
        }

        // 5. Configuración del Slider de Velocidad
        seekBarVelocidad.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val factor = valoresVelocidad[progress]
                txtVelocidad.text = "Velocidad: ${factor}x"

                // Manda el cambio inmediatamente mientras arrastras el dedo
                enviarComandoDinamico("$IP_ESP32/velocidad?factor=$factor")
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    // Función auxiliar para habilitar o deshabilitar los botones de control de golpe
    private fun setControlesActivos(activar: Boolean) {
        btnReproducir.isEnabled = activar
        btnPausa.isEnabled = activar
        btnDetener.isEnabled = activar
        seekBarVelocidad.isEnabled = activar
    }

    private fun enviarComandoConectar() {
        val url = "$IP_ESP32/conectar"
        val queue = Volley.newRequestQueue(this)

        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                if (response.trim() == "CONEXION_EXITOSA") {
                    Toast.makeText(this, "¡Caja conectada con éxito! 🎸", Toast.LENGTH_SHORT).show()
                    cargarListaDeCanciones() // Si conecta, descarga la lista
                }
            },
            { error ->
                Log.e(TAG, "Error de red: ${error.message}")
                Toast.makeText(this, "Error de enlace. Revisa tu conexión Wi-Fi.", Toast.LENGTH_LONG).show()
            }
        )
        queue.add(stringRequest)
    }

    private fun cargarListaDeCanciones() {
        val queue = Volley.newRequestQueue(this)
        val url = "$IP_ESP32/lista"

        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                if (response.isNotEmpty() && !response.contains("ERROR")) {
                    listaDeCanciones = response.split(",")

                    val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, listaDeCanciones)
                    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
                    spinnerCanciones.adapter = adapter

                    // Activamos todos los botones porque la caja está lista para la acción
                    setControlesActivos(true)
                }
            },
            { error ->
                Toast.makeText(this, "Error al descargar catálogo", Toast.LENGTH_SHORT).show()
                error.printStackTrace()
            })
        queue.add(stringRequest)
    }

    /**
     * Función genérica simplificada para enviar cualquier comando GET al ESP32
     * (reproducir, pausar, detener, velocidad). Reduce repetición de código.
     */
    private fun enviarComandoDinamico(url: String) {
        val queue = Volley.newRequestQueue(this)
        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                Log.d(TAG, "ESP32 dice: $response")
            },
            { error ->
                Log.e(TAG, "Fallo de comando a URL: $url")
                error.printStackTrace()
            }
        )
        queue.add(stringRequest)
    }
}