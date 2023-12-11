# LORA_ESP32_humedad_1nivel

## Descripción del Proyecto
Este sketch de Arduino para ESP32 se utiliza en la medición de la humedad del suelo, integrando tecnología LoRa para la transmisión de datos. Actualmente, el sketch está configurado para trabajar con un solo sensor de humedad, aunque el sistema está diseñado para manejar hasta dos niveles de medición. Es importante destacar que existe un problema conocido con la asignación de pines en la placa, que está actualmente en revisión.

## Hardware Requerido
- ESP32 (TTGO LoRa32 V2.1.6)
- Módulo LoRa
- Sensor(es) de humedad del suelo
- [Otros componentes si son utilizados]

## Problema con los Pines
Actualmente, hay un problema con la configuración de los pines en la placa que puede limitar la funcionalidad. Se está trabajando en una revisión para resolver este problema.

## Configuración del Hardware
Descripción de cómo conectar los componentes hardware, incluyendo:
- Conexiones del módulo LoRa al ESP32.
- Conexión de los sensores de humedad.

## Instalación y Uso
1. Instala el entorno de desarrollo de Arduino.
2. Abre el archivo `LORA_ESP32_humedad_1nivel.ino`.
3. Conecta tu ESP32 a la computadora.
4. Selecciona el modelo correcto de ESP32 en el IDE de Arduino.
5. Sube el sketch al ESP32.

## Contribuciones
Las contribuciones son bienvenidas, especialmente en lo que respecta a la mejora de la asignación de pines y la expansión de la funcionalidad del sistema.

## Licencia
Este proyecto está distribuido bajo la Licencia Creative Commons.

