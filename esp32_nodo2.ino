//UNIVERSIDAD ESTATAL AMAZONICA
//Johnny Edith Conde Paucar   
//José Marcelo Cañar Juncal 
//Yunny Viviana Cuevas Santacruz
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "DHT.h"
// =====================================================
// NODO 1 para la primera placa, NODO 2 para la segunda
#define NODO 1
// =====================================================
// ---------- WiFi ----------
const char* ssid     = "BASE#1";
const char* password = "mirella240819";

// ---------- HiveMQ Cloud ----------
const char* mqtt_server = "97ab06eb161342cfa472cf20f448f413.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "Marcelo";
const char* mqtt_pass   = "veronica18";

// ---------- Pines ----------
#define DHTPIN    5     // D4
#define DHTTYPE   DHT11
#define PIN_SUELO 34    // D34 (ADC1, solo entrada)
#define PIN_LDR   35    // D35 (ADC1, solo entrada)

// ---------- Calibración del sensor de suelo (cada placa la suya) ----------
#if NODO == 1
  const int SUELO_SECO = 3500, SUELO_MOJADO = 1400;
#else
  const int SUELO_SECO = 3500, SUELO_MOJADO = 1400;
#endif

// ---------- Topics y Client ID generados automáticamente ----------
String base     = "ESP32S3/nodo" + String(NODO) + "/";
String clientId = "ESP32-nodo" + String(NODO);
String topic_temp  = base + "temperatura";
String topic_hum   = base + "humedad";
String topic_suelo = base + "suelo";
String topic_luz   = base + "luz";

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;

void setup_wifi() {
  Serial.println("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT como ");
    Serial.print(clientId);
    Serial.print("... ");
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Conectado");
    } else {
      Serial.print("Error, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

int leerAnalogico(int pin) {
  long suma = 0;
  for (int i = 0; i < 10; i++) { suma += analogRead(pin); delay(5); }
  return suma / 10;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - ultimoEnvio >= 5000) {
    ultimoEnvio = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      client.publish(topic_temp.c_str(), String(t, 1).c_str());
      client.publish(topic_hum.c_str(),  String(h, 0).c_str());
    } else {
      Serial.println("Error al leer DHT11");
    }

    int rawSuelo = leerAnalogico(PIN_SUELO);
    int suelo = constrain(map(rawSuelo, SUELO_SECO, SUELO_MOJADO, 0, 100), 0, 100);
    client.publish(topic_suelo.c_str(), String(suelo).c_str());

    int rawLdr = leerAnalogico(PIN_LDR);
    int luz = map(rawLdr, 0, 4095, 0, 100);
    client.publish(topic_luz.c_str(), String(luz).c_str());

    Serial.printf("[Nodo %d] Temp: %.1f °C | Hum: %.0f %% | Suelo: %d %% (raw %d) | Luz: %d %% (raw %d)\n",
                  NODO, t, h, suelo, rawSuelo, luz, rawLdr);
  }
}
