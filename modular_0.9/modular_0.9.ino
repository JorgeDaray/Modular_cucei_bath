
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_VL53L0X.h>

#define SDA_PIN 8    // Pin SDA para la comunicación I2C (depende de tu placa)
#define SCL_PIN 9    // Pin SCL para la comunicación I2C (depende de tu placa)

Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

// Configura tus credenciales de WiFi
//const char* ssid = "MEGACABLE-C9A2";
//const char* password = "mMAFOE7rE@f$uDq";

const char* ssid = "RedmiNote13";
const char* password = "reyesdiaz";

// Dirección del servidor HTTPS
const char* serverName = "https://cucei-bath.website";
const int httpsPort = 443; // Puerto HTTPS predeterminado

WiFiClientSecure secureClient;

int personCount = 0;
bool objectDetected = false;

unsigned long objectDetectedStart = 0; // Momento en que se detectó el objeto
unsigned long objectClearedStart = 0;  // Momento en que se dejó de detectar el objeto
bool isClosed = false;                 // Estado de la puerta (cerrado o abierto)
bool stateChanged = false;             // Indica si hubo un cambio de estado

void SensorDistanciaSetup() {
  // Inicia la comunicación I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializa el sensor VL53L0X
  if (!sensor.begin()) {
    Serial.println("No se pudo encontrar el sensor VL53L0X.");
    while (1);
  }
  Serial.println("Sensor VL53L0X inicializado.");
}

// Función para enviar datos al servidor vía HTTPS (POST)
void sendDataToHTTPS() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient https;
        https.begin(secureClient, String(serverName) + "/data"); // Endpoint actualizado

        https.addHeader("Content-Type", "application/json");

        DynamicJsonDocument doc(256);
        doc["estado"] = isClosed ? "cerrado" : "abierto";
        doc["conteo"] = personCount;

        String jsonString;
        serializeJson(doc, jsonString);

        int httpResponseCode = https.POST(jsonString);
        if (httpResponseCode > 0) {
            Serial.println("Datos enviados correctamente.");
            Serial.println(https.getString());
        } else {
            Serial.print("Error en POST: ");
            Serial.println(httpResponseCode);
        }
        https.end();
    } else {
        Serial.println("Error: No conectado a WiFi.");
    }
}

// Función para actualizar el conteo de personas basado en la distancia medida
void updatePersonCount() {
    VL53L0X_RangingMeasurementData_t measure;
    sensor.rangingTest(&measure, false);

    unsigned long currentTime = millis(); // Tiempo actual

    // Detectar objetos dentro de un rango
    if (measure.RangeStatus != 4 && measure.RangeMilliMeter < 700) {
        if (!objectDetected) {
            personCount++;
            objectDetected = true;
            stateChanged = true;
            objectDetectedStart = currentTime; // Registrar el tiempo de detección
            Serial.println("Objeto detectado!");
        }

        // Verificar si ha pasado más de 1 minuto con el objeto detectado
        if (!isClosed && (currentTime - objectDetectedStart > 60000)) {
            isClosed = true;
            stateChanged = true;
            Serial.println("Estado: Esta cerrado");
        }
    } else {
        if (objectDetected) {
            objectDetected = false;
            objectClearedStart = currentTime; // Registrar el tiempo de liberación
            Serial.println("Objeto ya no detectado.");
        }

        // Abrir la puerta 1 minuto después de que se dejó de detectar un objeto
        if (isClosed && (currentTime - objectClearedStart > 60000)) {
            isClosed = false;
            stateChanged = true;
            Serial.println("Estado: Esta abierto");
        }
    }

    // Si hubo cambios en el estado, enviamos los datos al servidor HTTPS
    if (stateChanged) {
        sendDataToHTTPS();
        stateChanged = false; // Restablecer el indicador de cambio de estado
    }
}

void setup() {
    // Inicializa el puerto serial
    Serial.begin(115200);

    // Conéctate a la red WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    Serial.println("Conectado a WiFi");
    Serial.print("IP asignada: ");
    Serial.println(WiFi.localIP());

    SensorDistanciaSetup();  // Inicializa el sensor VL53L0X

    // Configurar certificado (necesario para WiFiClientSecure)
    secureClient.setInsecure(); // Usa esta línea solo si no tienes un certificado específico
}

void loop() {
    updatePersonCount();
    delay(100);
}
