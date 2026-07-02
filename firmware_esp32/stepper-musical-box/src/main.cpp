#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h" // Librería obligatoria para el sistema de archivos

// Definimos un tipo de dato llamado 'NotaMusical' para organizar las frecuencias físicas de cada nota
namespace Notas {
    const uint32_t SILENCIO = 0;
    const uint32_t DO5  = 16744; // Frecuencias ajustadas para motores configurados a 1/32 micropasos
    const uint32_t RE5  = 18795;
    const uint32_t MI5  = 21096;
    const uint32_t FA5  = 22351;
    const uint32_t SOL5 = 25091;
    const uint32_t LA5  = 28160;
    const uint32_t SI5  = 31609;
    const uint32_t DO6  = 33488;
}

// Estructura para cada pulso de la canción (Dueto)
struct PulsoMusical {
    uint32_t frecuenciaMotor1; // Nota para el motor 1
    uint32_t frecuenciaMotor2; // Nota para el motor 2
    uint32_t duracionMs;       // Tiempo que durará el pulso en milisegundos
};

// Arreglo que simula lo que eventualmente leeremos desde un archivo de texto .txt
const PulsoMusical cancionPrueba[] = {
    // { Motor 1, Motor 2, Duración }
    { Notas::DO5,  Notas::MI5,  400 }, // Acorde DO - MI
    { Notas::MI5,  Notas::SOL5, 400 }, // Acorde MI - SOL
    { Notas::SOL5, Notas::DO6,  400 }, // Acorde SOL - DO (Octava arriba)
    { Notas::SILENCIO, Notas::SILENCIO, 100 }, // Breve silencio
    
    { Notas::DO6,  Notas::SOL5, 300 },
    { Notas::FA5,  Notas::LA5,  300 },
    { Notas::RE5,  Notas::FA5,  600 },
    { Notas::SILENCIO, Notas::SILENCIO, 200 },

    { Notas::DO5,  Notas::DO6,  800 }  // Nota sostenida final (Octavas juntas)
};

// Calculamos automáticamente cuántos pulsos tiene la canción para no hardcodear el límite
const size_t longitudCancion = sizeof(cancionPrueba) / sizeof(cancionPrueba[0]);

// --- CONFIGURACIÓN MOTOR 1 (Existente) ---
#define PIN_DIR_1     22
#define PIN_STEP_1    23
#define PIN_ENABLE_1  19
const uint8_t CANAL_LEDC_1 = 0;

// --- CONFIGURACIÓN MOTOR 2 (Nuevo) ---
// Nota: Asegúrate de conectar estos pines físicos de tu ESP32 a tu segundo driver
#define PIN_DIR_2     18
#define PIN_STEP_2    5
#define PIN_ENABLE_2  17
const uint8_t CANAL_LEDC_2 = 1; // Canal PWM independiente

const uint8_t RESOLUCION_BITS = 8;
// Configuración de la red Wi-Fi del ESP32
const char* ssid = "Musical_stepper_box";
const char* password = "Pugcitabb"; // Mínimo 8 caracteres

AsyncWebServer server(80);

// Función modificada para controlar ambos motores simultáneamente
void tocarDuetoPrueba(uint32_t freq1, uint32_t freq2, uint32_t duracion) {
  // Encender ambos drivers (LOW activa el DRV8825)
  digitalWrite(PIN_ENABLE_1, LOW);
  digitalWrite(PIN_ENABLE_2, LOW);
  
  // Generar las dos frecuencias en canales separados por hardware
  ledcWriteTone(CANAL_LEDC_1, freq1);
  ledcWriteTone(CANAL_LEDC_2, freq2);
  
  delay(duracion);
  
  // Apagar frecuencias
  ledcWriteTone(CANAL_LEDC_1, 0);
  ledcWriteTone(CANAL_LEDC_2, 0);
  
  // Apagar drivers para que no consuman corriente en reposo
  digitalWrite(PIN_ENABLE_1, HIGH);
  digitalWrite(PIN_ENABLE_2, HIGH);
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar Pines Motor 1
  pinMode(PIN_DIR_1, OUTPUT);
  pinMode(PIN_ENABLE_1, OUTPUT);
  digitalWrite(PIN_DIR_1, LOW);
  digitalWrite(PIN_ENABLE_1, HIGH);

  // Inicializar Pines Motor 2
  pinMode(PIN_DIR_2, OUTPUT);
  pinMode(PIN_ENABLE_2, OUTPUT);
  digitalWrite(PIN_DIR_2, LOW);
  digitalWrite(PIN_ENABLE_2, HIGH);

  // Configurar Canales PWM independientes
  ledcSetup(CANAL_LEDC_1, 2000, RESOLUCION_BITS);
  ledcAttachPin(PIN_STEP_1, CANAL_LEDC_1);

  ledcSetup(CANAL_LEDC_2, 2000, RESOLUCION_BITS);
  ledcAttachPin(PIN_STEP_2, CANAL_LEDC_2);

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/conectar", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "CONEXION_EXITOSA");
  });

server.on("/reproducir", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Iniciando reproducción de la melodía completa...");
    request->send(200, "text/plain", "Reproduciendo melodía...");
    
    // Recorremos la partitura pulso por pulso
    for (size_t i = 0; i < longitudCancion; i++) {
        PulsoMusical pulsoActual = cancionPrueba[i];
        
        // 1. Encender los drivers de potencia
        digitalWrite(PIN_ENABLE_1, LOW);
        digitalWrite(PIN_ENABLE_2, LOW);
        
        // 2. Modular las frecuencias de hardware (Maneja silencios automáticamente)
        if (pulsoActual.frecuenciaMotor1 == Notas::SILENCIO) {
            ledcWriteTone(CANAL_LEDC_1, 0);
        } else {
            ledcWriteTone(CANAL_LEDC_1, pulsoActual.frecuenciaMotor1);
        }

        if (pulsoActual.frecuenciaMotor2 == Notas::SILENCIO) {
            ledcWriteTone(CANAL_LEDC_2, 0);
        } else {
            ledcWriteTone(CANAL_LEDC_2, pulsoActual.frecuenciaMotor2);
        }
        
        // 3. Mantener la nota el tiempo que indica la partitura
        delay(pulsoActual.duracionMs);
        
        // Un brevísimo espacio de 20ms entre notas para que no se escuchen "pegadas"
        ledcWriteTone(CANAL_LEDC_1, 0);
        ledcWriteTone(CANAL_LEDC_2, 0);
        delay(20);
    }
    
    // Apagar etapas de potencia al finalizar la canción para proteger los motores
    digitalWrite(PIN_ENABLE_1, HIGH);
    digitalWrite(PIN_ENABLE_2, HIGH);
    Serial.println("Melodía finalizada.");
  });

  server.begin();
}

void loop() {}