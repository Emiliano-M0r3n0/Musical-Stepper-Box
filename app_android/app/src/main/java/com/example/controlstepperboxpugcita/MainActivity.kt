package com.example.controlstepperboxpugcita

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import com.android.volley.Request
import com.android.volley.toolbox.StringRequest
import com.android.volley.toolbox.Volley

class MainActivity : AppCompatActivity() {

    private val IP_ESP32 = "http://192.168.4.1"
    private val TAG = "MecatronicaApp"

    private lateinit var btnConectar: Button
    private lateinit var spinnerCanciones: Spinner
    private lateinit var btnReproducir: Button
    private lateinit var btnPausa: Button
    private lateinit var btnDetener: Button
    private lateinit var btnBajarVelocidad: Button
    private lateinit var btnSubirVelocidad: Button
    private lateinit var txtVelocidad: TextView
    private lateinit var txtProgreso: TextView
    private lateinit var progressCancion: ProgressBar

    private var listaDeCanciones: List<String> = ArrayList()
    private var velocidadActual = 1.0f

    // Temporizador para consultar el progreso cada segundo
    private val handlerProgreso = Handler(Looper.getMainLooper())
    private lateinit var runnableProgreso: Runnable

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Vincular componentes
        btnConectar = findViewById(R.id.btnConectar)
        spinnerCanciones = findViewById(R.id.spinnerCanciones)
        btnReproducir = findViewById(R.id.btnReproducir)
        btnPausa = findViewById(R.id.btnPausa)
        btnDetener = findViewById(R.id.btnDetener)
        btnBajarVelocidad = findViewById(R.id.btnBajarVelocidad)
        btnSubirVelocidad = findViewById(R.id.btnSubirVelocidad)
        txtVelocidad = findViewById(R.id.txtVelocidad)
        txtProgreso = findViewById(R.id.txtProgreso)
        progressCancion = findViewById(R.id.progressCancion)

        setControlesActivos(false)

        // Configurar el loop de consulta de progreso
        runnableProgreso = object : Runnable {
            override fun run() {
                obtenerProgresoESP32()
                handlerProgreso.postDelayed(this, 1000) // Se ejecuta cada 1000 ms
            }
        }

        btnConectar.setOnClickListener { enviarComandoConectar() }

        btnReproducir.setOnClickListener {
            if (listaDeCanciones.isNotEmpty()) {
                val cancionSeleccionada = spinnerCanciones.selectedItem.toString()
                enviarComandoDinamico("$IP_ESP32/reproducir?archivo=$cancionSeleccionada")
                startProgresoLoop() // Iniciar monitoreo visual
            }
        }

        btnPausa.setOnClickListener {
            enviarComandoDinamico("$IP_ESP32/pausa")
            stopProgresoLoop() // Pausar monitoreo visual
        }

        btnDetener.setOnClickListener {
            enviarComandoDinamico("$IP_ESP32/detener")
            stopProgresoLoop()
            progressCancion.progress = 0
            txtProgreso.text = "Progreso: 0%"
        }

        // Lógica de botones de velocidad (+ y - de 0.25 en 0.25)
        btnBajarVelocidad.setOnClickListener {
            if (velocidadActual > 0.5f) {
                velocidadActual -= 0.25f
                actualizarVelocidad()
            }
        }

        btnSubirVelocidad.setOnClickListener {
            if (velocidadActual < 2.0f) {
                velocidadActual += 0.25f
                actualizarVelocidad()
            }
        }
    }

    private fun actualizarVelocidad() {
        txtVelocidad.text = "Velocidad: ${velocidadActual}x"
        enviarComandoDinamico("$IP_ESP32/velocidad?factor=$velocidadActual")
    }

    private fun startProgresoLoop() {
        handlerProgreso.removeCallbacks(runnableProgreso)
        handlerProgreso.post(runnableProgreso)
    }

    private fun stopProgresoLoop() {
        handlerProgreso.removeCallbacks(runnableProgreso)
    }

    private fun obtenerProgresoESP32() {
        val queue = Volley.newRequestQueue(this)
        val url = "$IP_ESP32/progreso"

        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                try {
                    // El ESP32 debe responder un número entero plano (ej. "45")
                    val porcentaje = response.trim().toInt()
                    progressCancion.progress = porcentaje
                    txtProgreso.text = "Progreso: $porcentaje%"

                    if (porcentaje >= 100) {
                        stopProgresoLoop()
                    }
                } catch (e: Exception) {
                    Log.e(TAG, "Error parseando progreso")
                }
            },
            { error -> Log.e(TAG, "Fallo al consultar progreso") }
        )
        queue.add(stringRequest)
    }

    private fun setControlesActivos(activar: Boolean) {
        btnReproducir.isEnabled = activar
        btnPausa.isEnabled = activar
        btnDetener.isEnabled = activar
        btnBajarVelocidad.isEnabled = activar
        btnSubirVelocidad.isEnabled = activar
    }

    private fun enviarComandoConectar() {
        val url = "$IP_ESP32/conectar"
        val queue = Volley.newRequestQueue(this)

        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                if (response.trim() == "CONEXION_EXITOSA") {
                    Toast.makeText(this, "¡Caja conectada con éxito! 🎸", Toast.LENGTH_SHORT).show()
                    cargarListaDeCanciones()
                }
            },
            { error ->
                Toast.makeText(this, "Error de enlace. Revisa el Wi-Fi.", Toast.LENGTH_LONG).show()
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
                    setControlesActivos(true)
                }
            },
            { error -> Toast.makeText(this, "Error al descargar catálogo", Toast.LENGTH_SHORT).show() })
        queue.add(stringRequest)
    }

    private fun enviarComandoDinamico(url: String) {
        val queue = Volley.newRequestQueue(this)
        val stringRequest = StringRequest(Request.Method.GET, url,
            { response -> Log.d(TAG, "ESP32: $response") },
            { error -> Log.e(TAG, "Fallo URL: $url") }
        )
        queue.add(stringRequest)
    }

    override fun onDestroy() {
        super.onDestroy()
        stopProgresoLoop() // Evita fugas de memoria si se cierra la app
    }
}