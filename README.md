
# Safe-Art: RAK3172 LoRaWAN Node

**Universitat Politècnica de València**
**Máster Universitario en Ingeniería de Computadores y Redes**
**Asignatura:** Redes Inalámbricas de Sensores
**Autores:** Fabrizio Chacon Ruiz, Cesar Almeida Leon
**Fecha:** Febrero 2026

---

## Descripción del Proyecto

**Safe-Art** es un sistema IoT diseñado para la preservación preventiva del patrimonio cultural en refugios de arte rupestre.

Este repositorio contiene el **firmware** desarrollado para el nodo sensor basado en el módulo **RAK3172**. El dispositivo monitoriza condiciones ambientales críticas y transmite los datos mediante la red **LoRaWAN** hacia la infraestructura en la nube (The Things Network), permitiendo la visualización y gestión de alertas en tiempo real.

## Funcionamiento del Firmware

El código implementa una máquina de estados optimizada para bajo consumo energético utilizando la API RUI3 (RAK Unified Interface).

### 1. Estrategia de Conexión y Modulación

* **Protocolo:** LoRaWAN 1.0.4 (Región EU868).
* **Método de Activación:** OTAA (Over The Air Activation).
* **Data Rate:** Configuración inicial en **SF7 (DR5)** para priorizar una unión rápida a la red y reducir el tiempo en aire, manteniendo la ventana de recepción RX2 en SF9 como respaldo estándar.

### 2. Ciclo de Operación (Timing)

El dispositivo opera en modo de bajo consumo (Sleep Mode), despertando únicamente para la adquisición y transmisión de datos:

* **Modo Normal:** Intervalo de transmisión establecido cada **2 minutos**.
* **Modo Alarma:** Ante la detección de valores críticos, el intervalo de transmisión se reduce automáticamente a **30 segundos**.

### 3. Adquisición de Datos

* **Sensor:** Sensirion SHTC3 (Interfaz I2C, dirección `0x70`).
* **Variables:** Temperatura (ºC) y Humedad Relativa (%).

---

## Protocolo de Datos (Payload)

El nodo transmite un paquete hexadecimal optimizado. A continuación se detalla la estructura para la decodificación en el servidor de aplicaciones (The Things Network / ChirpStack).

### Uplink (Subida: Sensor a Nube)

| Byte | Variable | Decodificación |
| --- | --- | --- |
| `0-1` | ID/Tipo Canal | Identificador de Temperatura |
| `2-3` | **Temperatura** | `Valor / 10` (int16) |
| `4-5` | ID/Tipo Canal | Identificador de Humedad |
| `6` | **Humedad** | `Valor / 2` (uint8) |
| `7-8` | ID/Tipo Canal | Identificador Digital |
| `9` | **Estado Alarma** | `1 = Activa`, `0 = Normal` |

#### Decodificador JavaScript (Payload Formatter)

```javascript
function Decoder(bytes, port) {
  var decoded = {};
  
  // Decodificación de Temperatura
  var t_raw = (bytes[2] << 8) | bytes[3];
  decoded.temperature = t_raw / 10.0;

  // Decodificación de Humedad
  decoded.humidity = bytes[6] / 2.0;

  // Estado del sistema
  decoded.alarm_active = bytes[9] === 1;

  return decoded;
}

```

### Downlink (Bajada: Nube a Sensor)

El dispositivo monitoriza las ventanas de recepción (Clase A) tras cada transmisión. Admite los siguientes comandos hexadecimales en el puerto 1:

* **`01`**: Restablece el estado de alarma y activa el indicador LED.
* **`00`**: Desactiva el indicador LED.

---

## Especificaciones de Hardware

* **SoC LoRa:** RAK3172 (STM32WLE5)
* **Placa Base:** WisBlock Base Board (RAK5005-O / RAK19007)
* **Módulo Sensor:** RAK1901 (Temperatura y Humedad)

---

## Contacto

* **Fabrizio Chacon Ruiz** - [facharui@upv.edu.es](mailto:facharui@upv.edu.es)
* **Cesar Almeida Leon** - [cealmeid@upv.edu.es](mailto:cealmeid@upv.edu.es)

**Universitat Politècnica de València**