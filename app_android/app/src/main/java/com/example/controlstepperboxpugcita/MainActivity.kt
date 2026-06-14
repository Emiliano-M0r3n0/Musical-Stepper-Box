package com.example.controlstepperboxpugcita

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.android.volley.Request
import com.android.volley.toolbox.StringRequest
import com.android.volley.toolbox.Volley
import android.widget.Button
import android.view.View

class MainActivity : AppCompatActivity() {

    // La IP fija que el ESP32 genera por defecto en modo Access Point
    private val IP_ESP32 = "http://192.168.4.1"
    private val TAG = "MecatronicaApp"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // 1. Encontrar los botones de la pantalla usando su ID de XML
        val botonConectar: Button = findViewById(R.id.btnConectar)
        val botonReproducir: Button = findViewById(R.id.btnReproducir)

        // 2. Configurar la acción del clic para el botón de Conectar
        botonConectar.setOnClickListener {
            Log.i(TAG, "Botón Conectar presionado.")
            enviarComandoConectar()
        }

        // 3. Configurar la acción del clic para el botón de Reproducir
        botonReproducir.setOnClickListener {
            Log.i(TAG, "Botón Reproducir presionado.")
            enviarComandoReproducir()
        }
    }

    /**
     * Envía una petición HTTP GET al ESP32 para verificar que el celular
     * realmente tiene comunicación con la caja musical.
     */
    fun enviarComandoConectar() {
        val url = "$IP_ESP32/conectar"
        val queue = Volley.newRequestQueue(this)

        // Solicitamos una respuesta de texto (String) desde la URL dada
        val stringRequest = StringRequest(
            Request.Method.GET, url,
            { response ->
                // Este bloque se ejecuta si el ESP32 responde con éxito
                Log.d(TAG, "Respuesta del ESP32: $response")
                if (response == "CONEXION_EXITOSA") {
                    Toast.makeText(this, "¡Caja musical conectada con éxito! 🎸", Toast.LENGTH_SHORT).show()
                }
            },
            { error ->
                // Este bloque se ejecuta si la petición falla (ej: no estás en el Wi-Fi correcto)
                Log.e(TAG, "Error de red: ${error.message}")
                Toast.makeText(this, "Error: No se pudo conectar a la caja. Revisa tu Wi-Fi.", Toast.LENGTH_LONG).show()
            }
        )

        // Añadimos la petición a la cola de Volley para que se envíe inmediatamente
        queue.add(stringRequest)
    }

    /**
     * Envía la orden inalámbrica al ESP32 para que comience a reproducir
     * la melodía a través del pin STEP del motor a pasos.
     */
    fun enviarComandoReproducir() {
        val url = "$IP_ESP32/reproducir"
        val queue = Volley.newRequestQueue(this)

        val stringRequest = StringRequest(
            Request.Method.GET, url,
            { response ->
                Log.d(TAG, "Respuesta del ESP32 al reproducir: $response")
                Toast.makeText(this, "Reproduciendo melodía en el motor... 🎼", Toast.LENGTH_SHORT).show()
            },
            { error ->
                Log.e(TAG, "Error al intentar reproducir: ${error.message}")
                Toast.makeText(this, "Error de comunicación al reproducir.", Toast.LENGTH_SHORT).show()
            }
        )

        queue.add(stringRequest)
    }
}