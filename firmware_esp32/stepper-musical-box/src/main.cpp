#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"

// --- PINES DE LOS MOTORES (Se mantienen igual) ---
#define PIN_DIR_1     22
#define PIN_STEP_1    23
#define PIN_ENABLE_1  19
const uint8_t CANAL_LEDC_1 = 0;

#define PIN_DIR_2     18
#define PIN_STEP_2    5
#define PIN_ENABLE_2  17
const uint8_t CANAL_LEDC_2 = 1;

const uint8_t RESOLUCION_BITS = 8;

const char* ssid = "Caja_Musical_Mecatronica";
const char* password = "876543210_password";

AsyncWebServer server(80);

// ==========================================
// 1. DEFINICIÓN DE TIPOS (Moldes limpios)
// ==========================================
enum EstadoMusica { 
    STOPPED, 
    PLAYING, 
    PAUSED 
};

// ==========================================
// 2. VARIABLES GLOBALES DE CONTROL
// ==========================================
// Ponemos una línea en blanco intermedia para romper el contexto que confunde al IDE
volatile EstadoMusica estadoActual = STOPPED;

String archivoParaReproducir = "";
volatile float factorVelocidad = 1.0; // 1.0 = normal, 2.0 = doble rápido, 0.5 = mitad de velocidad

// 'Handle' o identificador de la tarea de música para poder controlarla desde la red
TaskHandle_t xTareaMusicaHandle = NULL;

void iniciarLittleFS() {
    if (!LittleFS.begin(true)) { Serial.println("Error al montar LittleFS"); return; }
    Serial.println("LittleFS listo.");
}

// --- TAREA DE FREERTOS: REPRODUCTOR DE MÚSICA ---
void tareaReproductorMusica(void *pvParameters) {
    // Esta función se ejecuta de forma independiente e infinita en su propio hilo
    for (;;) {
        // Si el estado no es PLAYING, la tarea cede el procesador para no gastar recursos
        if (estadoActual != PLAYING) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Esperar 100ms de forma no bloqueante
            continue;
        }

        File archivo = LittleFS.open(archivoParaReproducir, "r");
        if (!archivo) {
            Serial.println("Error: Archivo no encontrado.");
            estadoActual = STOPPED;
            continue;
        }

        digitalWrite(PIN_ENABLE_1, LOW);
        digitalWrite(PIN_ENABLE_2, LOW);

        while (archivo.available() && estadoActual != STOPPED) {
            // MECANISMO DE PAUSA: Si desde la app mandan PAUSE, nos congelamos aquí
            while (estadoActual == PAUSED) {
                ledcWriteTone(CANAL_LEDC_1, 0);
                ledcWriteTone(CANAL_LEDC_2, 0);
                vTaskDelay(pdMS_TO_TICKS(100)); // Dormir la tarea de 100ms en 100ms sin congelar el chip
            }

            // Si el estado cambió a STOPPED a mitad de la canción, salimos del bucle
            if (estadoActual == STOPPED) break;

            String linea = archivo.readStringUntil('\n');
            linea.trim();
            if (linea.length() == 0 || linea.startsWith("#")) continue;

            int primeraComa = linea.indexOf(',');
            int segundaComa = linea.indexOf(',', primeraComa + 1);

            if (primeraComa != -1 && segundaComa != -1) {
                uint32_t freq1 = linea.substring(0, primeraComa).toInt();
                uint32_t freq2 = linea.substring(primeraComa + 1, segundaComa).toInt();
                uint32_t duracionOriginal = linea.substring(segundaComa + 1).toInt();

                // CONTROL DE VELOCIDAD MATEMÁTICO:
                // Modificamos el tiempo que dura la nota dividiendo entre el factor de velocidad
                uint32_t duracionAjustada = (uint32_t)(duracionOriginal / factorVelocidad);

                if (freq1 == 0) ledcWriteTone(CANAL_LEDC_1, 0);
                else ledcWriteTone(CANAL_LEDC_1, freq1);

                if (freq2 == 0) ledcWriteTone(CANAL_LEDC_2, 0);
                else ledcWriteTone(CANAL_LEDC_2, freq2);

                // Reemplazamos delay() por vTaskDelay() nativo de FreeRTOS.
                // Esto permite que el procesador atienda la red MIENTRAS la nota está sonando.
                vTaskDelay(pdMS_TO_TICKS(duracionAjustada));

                // Breve silencio de separación
                ledcWriteTone(CANAL_LEDC_1, 0);
                ledcWriteTone(CANAL_LEDC_2, 0);
                vTaskDelay(pdMS_TO_TICKS(15));
            }
        }

        archivo.close();
        digitalWrite(PIN_ENABLE_1, HIGH);
        digitalWrite(PIN_ENABLE_2, HIGH);
        estadoActual = STOPPED;
        Serial.println("Canción terminada o detenida.");
    }
}

// --- FUNCIÓN PARA ESCANEAR ARCHIVOS (Igual a la anterior) ---
String obtenerListaCanciones() {
    String lista = "";
    File raiz = LittleFS.open("/");
    if (!raiz) return "ERROR";
    File archivo = raiz.openNextFile();
    while (archivo) {
        String nombre = archivo.name();
        if (nombre.endsWith(".csv")) {
            if (lista.length() > 0) lista += ",";
            lista += nombre;
        }
        archivo = raiz.openNextFile();
    }
    return lista;
}

void setup() {
    Serial.begin(115200); //Inicia el monitor serial
    //Configuracion de Pines
    pinMode(PIN_DIR_1, OUTPUT); pinMode(PIN_ENABLE_1, OUTPUT);
    pinMode(PIN_DIR_2, OUTPUT); pinMode(PIN_ENABLE_2, OUTPUT);
    digitalWrite(PIN_ENABLE_1, HIGH); digitalWrite(PIN_ENABLE_2, HIGH);
    //Configuracion de canales LEDC
    ledcSetup(CANAL_LEDC_1, 2000, RESOLUCION_BITS); ledcAttachPin(PIN_STEP_1, CANAL_LEDC_1);
    ledcSetup(CANAL_LEDC_2, 2000, RESOLUCION_BITS); ledcAttachPin(PIN_STEP_2, CANAL_LEDC_2);
    //Iniciamos el sistema de archivos y la red wifi
    iniciarLittleFS();
    WiFi.softAP(ssid, password);

    // --- CONFIGURACIÓN DE LOS ENDPOINTS DEL SERVIDOR WEB ---
    server.on("/conectar", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "CONEXION_EXITOSA");
    });

    server.on("/lista", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", obtenerListaCanciones());
    });

    // 1. REPRODUCIR (O reanudar)
    server.on("/reproducir", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("archivo")) {
            archivoParaReproducir = "/" + request->getParam("archivo")->value();
            estadoActual = PLAYING;
            request->send(200, "text/plain", "Reproduciendo archivo");
        } else if (estadoActual == PAUSED) {
            estadoActual = PLAYING; // Si estaba pausado, simplemente reanuda
            request->send(200, "text/plain", "Música reanudada");
        } else {
            request->send(400, "text/plain", "Error en petición");
        }
    });

    // 2. PAUSAR
    server.on("/pausa", HTTP_GET, [](AsyncWebServerRequest *request){
        if (estadoActual == PLAYING) {
            estadoActual = PAUSED;
            request->send(200, "text/plain", "MÚSICA_PAUSADA");
        } else {
            request->send(200, "text/plain", "No hay música sonando");
        }
    });

    // 3. DETENER (STOP)
    server.on("/detener", HTTP_GET, [](AsyncWebServerRequest *request){
        estadoActual = STOPPED;
        request->send(200, "text/plain", "MÚSICA_DETENIDA");
    });

    // 4. CAMBIAR VELOCIDAD (Ej: /velocidad?factor=1.5)
    server.on("/velocidad", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("factor")) {
            String val = request->getParam("factor")->value();
            factorVelocidad = val.toFloat();
            request->send(200, "text/plain", "Velocidad cambiada a " + val);
        } else {
            request->send(400, "text/plain", "Falta factor");
        }
    });

    server.begin();

    // --- INVOCACIÓN CRÍTICA DE FREERTOS ---
    // Creamos la tarea de música y la "clavamos" explícitamente en el CORE 1.
    // De esta forma, el CORE 0 queda 100% libre para procesar el WiFi y las peticiones asíncronas de la App.
    xTaskCreatePinnedToCore(
        tareaReproductorMusica,    // Función que contiene la tarea
        "TareaMusica",             // Nombre interno de la tarea
        4096,                      // Tamaño de memoria asignado (Stack size en bytes)
        NULL,                      // Parámetros de entrada
        1,                         // Prioridad de la tarea
        &xTareaMusicaHandle,       // Handle de seguimiento
        1                          // ID del Núcleo del ESP32 (Core 1)
    );
}

// Como FreeRTOS maneja todo a través de sus propias tareas en segundo plano,
// el loop() de Arduino se queda completamente vacío y sin usar.
void loop() {}