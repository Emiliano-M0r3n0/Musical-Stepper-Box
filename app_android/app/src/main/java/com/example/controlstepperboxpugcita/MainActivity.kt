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

class MainActivity : AppCompatActivity() {

    private val IP_ESP32 = "http://192.168.4.1"
    private val TAG = "MecatronicaApp"

    private lateinit var btnConectar: Button
    private lateinit var spinnerCanciones: Spinner
    private lateinit var btnReproducir: Button
    private var listaDeCanciones: List<String> = ArrayList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Inicializar TODOS los componentes de la interfaz
        btnConectar = findViewById(R.id.btnConectar)
        spinnerCanciones = findViewById(R.id.spinnerCanciones)
        btnReproducir = findViewById(R.id.btnReproducir)

        // Al inicio, deshabilitamos el botón de reproducir hasta que estemos conectados
        btnReproducir.isEnabled = false

        // 1. El botón Conectar valida la red y jala la lista si todo sale bien
        btnConectar.setOnClickListener {
            enviarComandoConectar()
        }

        // 2. Configurar la acción del botón de reproducir
        btnReproducir.setOnClickListener {
            if (listaDeCanciones.isNotEmpty()) {
                val cancionSeleccionada = spinnerCanciones.selectedItem.toString()
                enviarComandoReproducir(cancionSeleccionada)
            } else {
                Toast.makeText(this, "La lista de canciones está vacía. Intenta conectar de nuevo.", Toast.LENGTH_SHORT).show()
            }
        }
    }

    /**
     * Envía una petición HTTP GET al ESP32 para verificar la comunicación.
     * Si tiene éxito, manda a llamar automáticamente a cargarListaDeCanciones().
     */
    private fun enviarComandoConectar() {
        val url = "$IP_ESP32/conectar"
        val queue = Volley.newRequestQueue(this)

        val stringRequest = StringRequest(
            Request.Method.GET, url,
            { response ->
                Log.d(TAG, "Respuesta del ESP32: $response")
                if (response.trim() == "CONEXION_EXITOSA") {
                    Toast.makeText(this, "¡Caja musical detectada! 🎸 Cargando canciones...", Toast.LENGTH_SHORT).show()

                    // PASO CLAVE: Si la conexión es exitosa, descargamos la lista de LittleFS
                    cargarListaDeCanciones()
                }
            },
            { error ->
                Log.e(TAG, "Error de red: ${error.message}")
                Toast.makeText(this, "Error: No se pudo conectar a la caja. Revisa que estés en el Wi-Fi de la caja.", Toast.LENGTH_LONG).show()
            }
        )
        queue.add(stringRequest)
    }

    // Función HTTP GET para obtener las canciones y rellenar el Spinner
    private fun cargarListaDeCanciones() {
        val queue = Volley.newRequestQueue(this)
        val url = "$IP_ESP32/lista"

        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                if (response.isNotEmpty() && !response.contains("ERROR")) {
                    // Romper la cadena por comas y guardarla en la lista de Kotlin
                    listaDeCanciones = response.split(",")

                    // Llenar el adaptador visual del Spinner
                    val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, listaDeCanciones)
                    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
                    spinnerCanciones.adapter = adapter

                    // Habilitar el botón de reproducir ya que hay música lista
                    btnReproducir.isEnabled = true
                    Toast.makeText(this, "¡Lista de canciones actualizada! 🎼", Toast.LENGTH_SHORT).show()
                } else {
                    Toast.makeText(this, "No se encontraron archivos .csv en el ESP32", Toast.LENGTH_LONG).show()
                }
            },
            { error ->
                Toast.makeText(this, "Error al descargar la lista de canciones", Toast.LENGTH_SHORT).show()
                error.printStackTrace()
            })

        queue.add(stringRequest)
    }

    // Función HTTP GET dinámica para enviar la canción elegida
    private fun enviarComandoReproducir(archivoCsv: String) {
        val queue = Volley.newRequestQueue(this)
        val url = "$IP_ESP32/reproducir?archivo=$archivoCsv"

        val stringRequest = StringRequest(Request.Method.GET, url,
            { response ->
                Toast.makeText(this, response, Toast.LENGTH_SHORT).show()
            },
            { error ->
                Toast.makeText(this, "Error al enviar comando de reproducción", Toast.LENGTH_SHORT).show()
                error.printStackTrace()
            })

        queue.add(stringRequest)
    }
}