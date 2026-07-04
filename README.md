# Caja Musical IoT con Motores a Pasos Duales 🎼🤖

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Firmware-orange.svg)](https://platformio.org/)
[![Android](https://img.shields.io/badge/Android-Kotlin-green.svg)](https://developer.android.com/)
[![Python](https://img.shields.io/badge/Python-Tools-blue.svg)](https://www.python.org/)

*Read this documentation in other languages: [English](README.md)*

Este proyecto consiste en el diseño y desarrollo de una **caja musical mecatrónica ciberfísica** controlada de forma inalámbrica. El sistema utiliza una arquitectura multihilo sobre un microcontrolador ESP32 para generar armonías acústicas complejas a través de dos motores a pasos independientes (NEMA 17) actuando como transductores inductivos. Integra una aplicación móvil nativa en Android para la gestión rítmica en tiempo real y herramientas automatizadas de conversión MIDI en Python.

---

## 🛠️ Arquitectura de Ingeniería del Sistema

El proyecto está diseñado bajo un enfoque mecatrónico integral que abarca cinco áreas clave:

### 1. Desarrollo Móvil (Android Nativo & UX)
* **Lenguaje:** Kotlin (API 29+).
* **Conectividad:** Enlace inalámbrico directo al Access Point generado por el microcontrolador.
* **Protocolo de Red:** Gestión de peticiones HTTP asíncronas utilizando la librería **Volley** de Google para interactuar con la API REST del microcontrolador sin bloquear la interfaz de usuario.
* **Diseño de Interfaz Optimizado:** Rediseño compacto que prioriza un panel de progreso (`ProgressBar` / tiempo restante) alimentado en segundo plano por consultas cíclicas al endpoint `/progreso` del ESP32, y controles sutiles de escalado de velocidad (`+` y `-` en pasos de 0.25x), relegándola a una función secundaria para maximizar el área útil.
* **Buenas Prácticas:** Internacionalización y manejo de recursos centralizados mediante `strings.xml`.

### 2. Firmware y Control Embebido (ESP32 - RTOS)
* **Entorno de Desarrollo:** PlatformIO bajo Ubuntu OS.
* **Arquitectura Multiclúcleo:** Servidor web asíncrono (`ESPAsyncWebServer`) corriendo de forma exclusiva en el **Core 0**, permitiendo atender peticiones de red y telemetría en segundo plano sin interrumpir los hilos de control.
* **Control Armónico de Movimiento:** Un hilo en tiempo real de **FreeRTOS** fijado al **Core 1** procesa el archivo CSV línea por línea y genera frecuencias musicales mediante el periférico de hardware `LEDC`. Gobierna dos canales independientes (Canal 0 para Melodía/Acorde y Canal 1 para Bajo) modulando el tono por hardware de forma limpia sin retardos bloqueantes.
* **Monitoreo Eficiente de Progreso:** Cálculo en tiempo real del porcentaje de reproducción analizando los bytes físicos del puntero del sistema de archivos (`archivo.position() * 100 / archivo.size()`). Al ser consultas directas de bajo nivel a LittleFS, se procesan en microsegundos sin degradar la precisión rítmica de los motores.
* **Sistema de Archivos:** Almacenamiento local de partituras de texto estructurado en frecuencias y tiempos dentro de la memoria Flash del chip utilizando **LittleFS**.

### 3. Herramientas de Conversión Automatizada (Python Core)
* **Procesamiento de Partituras Digitales:** Diseño de un script maestro automatizado (`midi_converter_pro.py`) capaz de parsear tracks de archivos MIDI exportados en JSON. Extrae canales armónicos, sincroniza deltas de tiempo a milisegundos y calcula de manera exacta las frecuencias equivalentes.
* **Escalado Ultrasónico Inductivo:** Implementa un multiplicador matemático constante (**64x**) que transfiere la música al espectro de los 20 kHz - 50 kHz. Esto evita el bloqueo mecánico por inercia del rotor del motor, permitiendo que las bobinas actúen puros como tweeters de alta potencia resonante.
* **Dualidad de Despliegue:** * **Modo GUI:** Interfaz gráfica limpia y moderna con ventanas nativas (`Tkinter`) y explorador de archivos para uso accesible de todo público.
  * **Modo CLI:** Herramienta de línea de comandos de alta velocidad y control para desarrolladores, automatizando conversiones en un solo paso.

### 4. Diseño Electrónico y Potencia
* **Voltaje de Operación:** Entrada de 12V DC para la etapa de potencia.
* **Regulación:** Uso de un convertidor Buck **Mini560** de alta eficiencia (95%) para bajar el voltaje a 5V de manera fría y segura, alimentando la lógica del ESP32 sin disipación térmica lineal excesiva.
* **Etapa de Control:** Dos drivers **DRV8825** configurados a modo de *Full-Step* (Paso Completo) para potenciar el impacto natural de conmutación magnética de las bobinas, maximizando la presión acústica y el volumen del sonido.
* **Diseño de PCB:** Desarrollado íntegramente en **KiCad**, utilizando layouts de etiquetas (*Net Labels*) para un esquemático limpio y profesional.

### 5. Manufactura y Diseño Mecánico
* **Modelado 3D:** Carcasa y andamios de soporte diseñados en **FreeCAD** considerando tolerancias mecánicas exactas para el montaje a presión de los componentes y la fijación del par de motores NEMA 17.
* **Optimización Acústica:** Geometría calculada para que la estructura actúe como caja de resonancia mecánica, amplificando las ondas de presión acústica generadas por las vibraciones del motor.

---

## 📂 Estructura del Repositorio

El repositorio está organizado en bloques modulares independientes:

* `/firmware_esp32`: Código fuente de PlatformIO para el microcontrolador (C++).
* `/app_android`: Proyecto nativo de Android Studio en Kotlin.
* `/python_tools`: Herramienta de conversión unificada MIDI-JSON a CSV (`midi_converter_pro.py`).
* `/diseño_electrónico`: Esquemáticos y layouts de la PCB en KiCad.
* `/diseño_mecánico`: Archivos fuente de modelado `.FCStd` en FreeCAD.

---

## 🔧 Requisitos para Replicar el Proyecto

### Software Requerido
* Ubuntu Linux (o compatible).
* Python 3.x (sin dependencias externas para el script base).
* VS Code con la extensión **PlatformIO IDE**.
* **Android Studio** (versión moderna con Gradle actualizado).
* **KiCad 7.0+** y **FreeCAD**.

### Pasos Rápidos de Despliegue

#### 1. Clonar el repositorio
```bash
git clone [https://github.com/Emiliano-M0r3n0/Musical-Stepper-Box.git](https://github.com/Emiliano-M0r3n0/Musical-Stepper-Box.git)

#### 2. Convertir música (MIDI a CSV)
Ejecuta el asistente gráfico o procesa un archivo directo por terminal a alta velocidad:
```bash
python3 python_tools/midi_converter_pro.py mi_cancion.json mi_cancion.csv

3. Cargar el Firmware y la App
* Abre la carpeta firmware_esp32 en PlatformIO, compila y carga el código junto con los archivos CSV en la partición LittleFS del ESP32.

* Abre la carpeta app_android en Android Studio, compila e instala la aplicación en tu dispositivo.

* Conéctate al Wi-Fi de la caja, abre la app, ¡y disfruta de la melodía!