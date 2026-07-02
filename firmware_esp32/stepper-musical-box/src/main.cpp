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
String archivoSeleccionado = ""; // Aquí guardaremos la ruta de la canción elegida

// --- FUNCIÓN PARA ESCANEAR LA MEMORIA Y MANDAR LA LISTA A ANDROID ---
String obtenerListaCanciones() {
    String lista = "";
    File raiz = LittleFS.open("/");
    if (!raiz) return "ERROR_AL_LEER_RAIZ";

    File archivo = raiz.openNextFile();
    while (archivo) {
        String nombre = archivo.name();
        // Filtramos para mandar solo los archivos que sean de música (.csv)
        if (nombre.endsWith(".csv")) {
            if (lista.length() > 0) {
                lista += ","; // Separamos las canciones por comas para la App
            }
            lista += nombre;
        }
        archivo = raiz.openNextFile();
    }
    return lista;
}

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

  // Nuevo Endpoint: La app de Android llamará aquí para llenar su lista/spinner
    server.on("/lista", HTTP_GET, [](AsyncWebServerRequest *request){
        String listaCanciones = obtenerListaCanciones();
        request->send(200, "text/plain", listaCanciones);
    });

// Endpoint Modificado: Ahora recibe el nombre de la canción dinámicamente
    server.on("/reproducir", HTTP_GET, [](AsyncWebServerRequest *request){
        // Comprobamos si la App envió el parámetro de la canción (ej: ?archivo=love_grows.csv)
        if (request->hasParam("archivo")) {
            AsyncWebParameter* p = request->getParam("archivo");
            archivoSeleccionado = "/" + p->value(); // Construimos la ruta (/love_grows.csv)
            arrancarMusica = true;                  // Activamos la bandera para el loop
            request->send(200, "text/plain", "Reproduciendo: " + p->value());
        } else {
            request->send(400, "text/plain", "Falta el parámetro 'archivo'");
        }
    });

  server.end(); // Asegurar reinicio limpio de rutas
  server.begin();
}

void loop()
{
if (arrancarMusica) {
        arrancarMusica = false; 
        // Convertimos el String de C++ a una cadena clásica de C (const char*)
        reproducirDesdeCSV(archivoSeleccionado.c_str()); 
    }
}