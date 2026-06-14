# Caja Musical IoT con Motores a Pasos 🎼🤖

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Firmware-orange.svg)](https://platformio.org/)
[![Android](https://img.shields.io/badge/Android-Kotlin-green.svg)](https://developer.android.com/)

Este proyecto consiste en el diseño y desarrollo de una **caja musical mecatrónica ciberfísica** controlada de forma inalámbrica. El sistema utiliza un microcontrolador ESP32 para generar frecuencias acústicas precisas a través de un motor a pasos (NEMA 17) comandado mediante señales de hardware, integrando una aplicación móvil nativa en Android para la gestión y reproducción de melodías en tiempo real.

---

## 🛠️ Arquitectura de Ingeniería del Sistema

El proyecto está diseñado bajo un enfoque mecatrónico integral que abarca cuatro áreas clave:

### 1. Desarrollo Móvil (Android Nativo)
* **Lenguaje:** Kotlin (API 29+).
* **Conectividad:** Enlace inalámbrico directo al Access Point generado por el microcontrolador.
* **Protocolo de Red:** Gestión de peticiones HTTP asíncronas utilizando la librería **Volley** de Google para interactuar con la API REST del microcontrolador sin bloquear la interfaz de usuario.
* **Buenas Prácticas:** Internacionalización y manejo de recursos centralizados mediante `strings.xml`.

### 2. Firmware y Control Embebido (ESP32)
* **Entorno de Desarrollo:** PlatformIO bajo Ubuntu OS.
* **Arquitectura:** Servidor web asíncrono (`ESPAsyncWebServer`) corriendo en el **Core 0**, permitiendo la escucha de peticiones de red en segundo plano.
* **Control de Movimiento:** Generación de frecuencias musicales mediante el periférico de hardware `LEDC` en el **Core 1**, modulando la velocidad del motor por hardware sin retardos bloqueantes (`delay`).
* **Sistema de Archivos:** Almacenamiento local de partituras (archivos de texto estructurados en frecuencias y tiempos) dentro de la memoria Flash del chip utilizando **LittleFS**.

### 3. Diseño Electrónico y Potencia
* **Voltaje de Operación:** Entrada de 12V DC para la etapa de potencia.
* **Regulación:** Uso de un convertidor Buck **Mini560** de alta eficiencia (95%) para bajar el voltaje a 5V de manera fría y segura, alimentando el ESP32 sin disipación térmica lineal excesiva.
* **Etapa de Control:** Driver **DRV8825** configurado a 1/32 de micropasos para suavizar el movimiento del motor y optimizar la resonancia acústica.
* **Diseño de PCB:** Desarrollado íntegramente en **KiCad**, utilizando layouts de etiquetas (*Net Labels*) para un esquemático limpio y profesional.

### 4. Manufactura y Diseño Mecánico
* **Modelado 3D:** Carcasa y andamios de soporte diseñados en **FreeCAD** considerando tolerancias mecánicas exactas para el montaje a presión de los componentes.
* **Optimización Acústica:** Geometría calculada para la dispersión de ondas sonoras provocadas por la vibración magnética de las bobinas del motor.

---

## 📂 Estructura del Repositorio

El repositorio está organizado en bloques modulares independientes:

* `/firmware_esp32`: Código fuente de PlatformIO para el microcontrolador.
* `/app_android`: Proyecto nativo de Android Studio en Kotlin.
* `/diseño_electrónico`: Esquemáticos y layouts de la PCB en KiCad.
* `/diseño_mecánico`: Archivos fuente de modelado `.FCStd` en FreeCAD.

---

## 🔧 Requisitos para Replicar el Proyecto

### Software Requerido
* Ubuntu Linux (o compatible).
* VS Code con la extensión **PlatformIO IDE**.
* **Android Studio** (versión moderna con Gradle actualizado).
* **KiCad 7.0+** y **FreeCAD**.

### Pasos Rápidos de Despliegue
1. Clona este repositorio:
   ```bash
   git clone [https://github.com/Emiliano-M0r3n0/Musical-Stepper-Box.git](https://github.com/Emiliano-M0r3n0/Musical-Stepper-Box.git)
 Abre la carpeta firmware_esp32 en PlatformIO, compila y carga el código al ESP32.

Abre la carpeta app_android en Android Studio, compila e instala el archivo de la aplicación en tu dispositivo Android.

Conéctate a la red Wi-Fi de la caja desde tu teléfono, abre la aplicación y presiona Reproducir.
