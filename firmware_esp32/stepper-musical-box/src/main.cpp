#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h" // Librería obligatoria para el sistema de archivos

// --- CONFIGURACIÓN DE PINES ---
#define PIN_DIR_1 22
#define PIN_STEP_1 23
#define PIN_ENABLE_1 19
const uint8_t CANAL_LEDC_1 = 0;

#define PIN_DIR_2 18
#define PIN_STEP_2 5
#define PIN_ENABLE_2 17
const uint8_t CANAL_LEDC_2 = 1;

const uint8_t RESOLUCION_BITS = 8;

// Configuración de la red Wi-Fi del ESP32
const char *ssid = "Musical_stepper_box";
const char *password = "Pugcitabb"; // Mínimo 8 caracteres

AsyncWebServer server(80);

// Función para inicializar LittleFS y verificar que funcione el hardware de memoria
void iniciarLittleFS()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("¡Error crítico! No se pudo montar el sistema LittleFS.");
    return;
  }
  Serial.println("LittleFS montado con éxito.");
}

// Función que lee el CSV línea por línea y hace sonar los motores en tiempo real
void reproducirDesdeCSV(const char *rutaArchivo)
{
  // Abrir el archivo en modo lectura ("r")
  File archivo = LittleFS.open(rutaArchivo, "r");
  if (!archivo)
  {
    Serial.println("Error: No se pudo abrir el archivo de música.");
    return;
  }

  Serial.println("Reproduciendo archivo...");

  // Habilitar etapas de potencia de los drivers (LOW es activo)
  digitalWrite(PIN_ENABLE_1, LOW);
  digitalWrite(PIN_ENABLE_2, LOW);

  // Leer el archivo línea por línea hasta el final
  while (archivo.available())
  {
    String linea = archivo.readStringUntil('\n');
    linea.trim(); // Limpiar espacios ocultos o saltos de línea de Windows (\r)

    // Ignorar líneas vacías o comentarios que empiecen con '#'
    if (linea.length() == 0 || linea.startsWith("#"))
    {
      continue;
    }

    // --- PROCESAR LA LÍNEA (Parsing de comas) ---
    int primeraComa = linea.indexOf(',');
    int segundaComa = linea.indexOf(',', primeraComa + 1);

    if (primeraComa != -1 && segundaComa != -1)
    {
      // Recortar y convertir los fragmentos de texto a números enteros
      uint32_t freq1 = linea.substring(0, primeraComa).toInt();
      uint32_t freq2 = linea.substring(primeraComa + 1, segundaComa).toInt();
      uint32_t duracion = linea.substring(segundaComa + 1).toInt();

      // Ejecutar frecuencias en los motores mediante hardware LEDC
      if (freq1 == 0)
        ledcWriteTone(CANAL_LEDC_1, 0);
      else
        ledcWriteTone(CANAL_LEDC_1, freq1);

      if (freq2 == 0)
        ledcWriteTone(CANAL_LEDC_2, 0);
      else
        ledcWriteTone(CANAL_LEDC_2, freq2);

      // Mantener la nota el tiempo indicado en el CSV
      delay(duracion);

      // Brevísimo espacio de separación de 15ms para que las notas no se empasten
      ledcWriteTone(CANAL_LEDC_1, 0);
      ledcWriteTone(CANAL_LEDC_2, 0);
      delay(15);
    }
  }

  // Cerrar el archivo y apagar motores al terminar la canción
  archivo.close();
  digitalWrite(PIN_ENABLE_1, HIGH);
  digitalWrite(PIN_ENABLE_2, HIGH);
  Serial.println("Fin de la canción.");
}

// --- Agrega esta variable global arriba de tu setup ---
volatile bool arrancarMusica = false;

void setup()
{
  Serial.begin(115200);

  // Configuración de salidas digitales
  pinMode(PIN_DIR_1, OUTPUT);
  pinMode(PIN_ENABLE_1, OUTPUT);
  pinMode(PIN_DIR_2, OUTPUT);
  pinMode(PIN_ENABLE_2, OUTPUT);
  digitalWrite(PIN_ENABLE_1, HIGH);
  digitalWrite(PIN_ENABLE_2, HIGH);

  // Configuración de canales de audio por hardware
  ledcSetup(CANAL_LEDC_1, 2000, RESOLUCION_BITS);
  ledcAttachPin(PIN_STEP_1, CANAL_LEDC_1);
  ledcSetup(CANAL_LEDC_2, 2000, RESOLUCION_BITS);
  ledcAttachPin(PIN_STEP_2, CANAL_LEDC_2);

  // Inicializar memoria Flash local
  iniciarLittleFS();

  WiFi.softAP(ssid, password);

  server.on("/conectar", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "CONEXION_EXITOSA"); });

  // --- Cambia el endpoint dentro de tu setup() para que quede así ---
  server.on("/reproducir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              request->send(200, "text/plain", "Procesando archivo CSV de música...");
              arrancarMusica = true; // Solo activamos la señal, no bloqueamos el servidor
            });

  server.end(); // Asegurar reinicio limpio de rutas
  server.begin();
}

void loop()
{
  if (arrancarMusica) {
        arrancarMusica = false; // Apagar la bandera inmediatamente
        reproducirDesdeCSV("/cancion.csv"); 
    }
}