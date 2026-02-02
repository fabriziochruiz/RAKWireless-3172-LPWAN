/**
 * ============================================================================
 * PROJECT: SAFE ART IOT
 * COURSE:  INTELLIGENT NETWORKS AND SENSORS
 * * AUTHORS:
 * - FABRIZIO CHACON RUIZ
 * - CESAR ALMEIDA LEON
 * * UNIVERSITY:
 * UNIVERSIDAD POLITECNICA DE VALENCIA (UPV)
 * * DESCRIPTION:
 * Firmware for RAK3172 (RUI3) LoRaWAN Node.
 * Monitors environmental data using SHTC3 sensor.
 * ============================================================================
 */

/**
 * Safe-Art IoT - RAK3172
 * Version: 1.0
 */

#include <Wire.h>

// ============================================================================
// 1. CREDENTIALS 
// ============================================================================

uint8_t nodeDevEui[8] = {0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x17, 0x87, 0xB9};
uint8_t nodeAppEui[8] = {0xAC, 0x1F, 0x09, 0xFF, 0xF8, 0x68, 0x31, 0x72};
uint8_t nodeAppKey[16] = {0xC4, 0x25, 0x28, 0xBD, 0xA4, 0x6E, 0xA0, 0xD5, 0x4E, 0x72, 0x0B, 0x4C, 0xCA, 0xDB, 0x6C, 0x40};

// ============================================================================
// 2. SYSTEM CONFIGURATION
// ============================================================================
#define NORMAL_INTERVAL 120000  // 2 minutes
#define ALARM_INTERVAL  30000   // 30 seconds during alarm state
#define TEMP_THRESHOLD  30.0    // Temperature threshold

#define SHTC3_ADDR 0x70

bool alarmActive = false;
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastSendTime = 0;

// ============================================================================
// 3. CALLBACKS
// ============================================================================

void recvCallback(SERVICE_LORA_RECEIVE_T * data) {
  if (data->BufferSize > 0) {
    Serial.printf("[INFO] Downlink: %02X\n", data->Buffer[0]);
    if (data->Buffer[0] == 0x01) {
      alarmActive = false;
      digitalWrite(LED_BUILTIN, HIGH);
    } else if (data->Buffer[0] == 0x00) {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

void joinCallback(int32_t status) {
  if (status == 0) {
    Serial.println("[SUCCESS] CONNECTED TO LORAWAN!");
    lastSendTime = millis() - NORMAL_INTERVAL; 
  } else {
    Serial.printf("[ERROR] Join Failed (Error: %d)\n", status);
  }
}

void sendCallback(int32_t status) {
  if (status == 0) Serial.println("[INFO] Packet Sent OK");
  else Serial.printf("[ERROR] Send Failed (St: %d)\n", status);
}

// ============================================================================
// 4. SENSOR MANAGEMENT
// ============================================================================

void readSensor() {
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(0x35); Wire.write(0x17);
  Wire.endTransmission();
  delay(1);

  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(0x7C); Wire.write(0xA2);
  Wire.endTransmission();
  delay(20);

  Wire.requestFrom(SHTC3_ADDR, 6);
  if (Wire.available() == 6) {
    uint16_t t_raw = (Wire.read() << 8) | Wire.read(); Wire.read();
    uint16_t h_raw = (Wire.read() << 8) | Wire.read(); Wire.read();

    temperature = -45.0 + (175.0 * (float)t_raw / 65535.0);
    humidity = 100.0 * ((float)h_raw / 65535.0);
    
    Wire.beginTransmission(SHTC3_ADDR);
    Wire.write(0xB0); Wire.write(0x98);
    Wire.endTransmission();
  } else {
    Serial.println("[ERROR] I2C Sensor Failure");
  }
}

// ============================================================================
// 5. SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(3000); 
  
  Serial.println("--- SYSTEM START (SF7 CONFIGURATION) ---");
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();

  // 1. Basic Configuration
  api.lorawan.band.set(RAK_REGION_EU868);
  api.lorawan.deviceClass.set(RAK_LORA_CLASS_A);
  api.lorawan.njm.set(RAK_LORA_OTAA);

  // 2. Credentials
  api.lorawan.deui.set(nodeDevEui, 8);
  api.lorawan.appeui.set(nodeAppEui, 8);
  api.lorawan.appkey.set(nodeAppKey, 16);

  // 3. STANDARD STRATEGY
  // Set Data Rate 5 (SF7 - Faster, less range)
  api.lorawan.dr.set(5); 
  
  // Maintain RX2 at SF9 as backup (TTN Standard)
  api.lorawan.rx2dr.set(3); 
  
  // (Factory RUI3 timing settings are used)

  // 4. Register Callbacks
  api.lorawan.registerRecvCallback(recvCallback);
  api.lorawan.registerJoinCallback(joinCallback);
  api.lorawan.registerSendCallback(sendCallback);

  Serial.println("[INFO] JOIN SF7 (Attempting fast join)...");
  api.lorawan.join();
}

// ============================================================================
// 6. MAIN LOOP
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  uint32_t currentInterval = alarmActive ? ALARM_INTERVAL : NORMAL_INTERVAL;

  if (currentTime - lastSendTime >= currentInterval) {
    lastSendTime = currentTime;
    readSensor();
    Serial.printf("Measuring: %.2fC | %.2f%%\n", temperature, humidity);
    
    // Payload Preparation
    uint8_t payload[11];
    uint8_t i = 0;

    int16_t t_send = (int16_t)(temperature * 10);
    payload[i++] = 0x01; payload[i++] = 0x67;
    payload[i++] = (t_send >> 8) & 0xFF; payload[i++] = t_send & 0xFF;

    uint8_t h_send = (uint8_t)(humidity * 2);
    payload[i++] = 0x02; payload[i++] = 0x68;
    payload[i++] = h_send;

    payload[i++] = 0x03; payload[i++] = 0x00;
    payload[i++] = alarmActive ? 0x01 : 0x00;

    // VERIFICATION AND RETRY
    if (api.lorawan.njs.get()) {
       Serial.println("[INFO] Sending data...");
       api.lorawan.send(i, payload, 1);
    } else {
       Serial.println("[WARN] NOT CONNECTED YET.");
       Serial.println("[INFO] Retrying Join...");
       api.lorawan.join(); // Force retry
    }
  }
  
  api.system.sleep.all(1000); 
}