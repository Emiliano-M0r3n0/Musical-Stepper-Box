#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Pines del motor
#define PIN_DIR     22
#define PIN_STEP    23
#define PIN_ENABLE  19

const uint8_t CANAL_LEDC = 0;
const uint8_t RESOLUCION_BITS = 8;

// Configuración de la red Wi-Fi del ESP32
const char* ssid = "Musical_stepper_box";
const char* password = "Pugcitabb"; // Mínimo 8 caracteres

AsyncWebServer server(80);

// Función para inicializar el motor con una nota de prueba
void tocarNotaPrueba(uint32_t frecuencia, uint32_t duracion) {
  digitalWrite(PIN_ENABLE, LOW); // Enciende el driver
  ledcWriteTone(CANAL_LEDC, frecuencia);
  delay(duracion);
  ledcWriteTone(CANAL_LEDC, 0);
  digitalWrite(PIN_ENABLE, HIGH); // Apaga el driver para que no se caliente
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_DIR, LOW);
  digitalWrite(PIN_ENABLE, HIGH); // Apagado por defecto

  // Inicializar hardware PWM
  ledcSetup(CANAL_LEDC, 2000, RESOLUCION_BITS);
  ledcAttachPin(PIN_STEP, CANAL_LEDC);

  // 1. Configurar ESP32 como Access Point
  Serial.print("Configurando Red Wi-Fi...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Red lista. IP del servidor: ");
  Serial.println(IP); // Por defecto será 192.168.4.1

  // 2. Definir rutas del Servidor Web (Endpoints)
  
  // Ruta de prueba para verificar conexión desde Android
  server.on("/conectar", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "CONEXION_EXITOSA");
  });

  // Ruta que llamará Android para activar la música
  server.on("/reproducir", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Comando de reproducción recibido desde el celular!");
    request->send(200, "text/plain", "Tocando melodia...");
    
    // Toca una nota rápida de confirmación (ej: Nota LA4 a 1/32 pasos = 14080 Hz)
    tocarNotaPrueba(14080, 500); 
  });

  // Iniciar el servidor
  server.begin();
}

void loop() {
  // El loop se queda vacío porque el servidor asíncrono corre en segundo plano
}