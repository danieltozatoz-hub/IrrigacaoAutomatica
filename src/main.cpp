#include <Arduino.h>
#include <SoftwareSerial.h>
#include "WiFiEsp.h"
#include <ArduinoJson.h>
#include "config.h"

SoftwareSerial SerialESP(2, 3); 
WiFiEspServer server(80);

struct Estado {
  int umidade = 0;
  bool bombaLigada = false;
  bool modoAuto = true;
  int limiteSeco = 30;
  unsigned long ultimaLeitura = 0;
} estado;

void lerSensor();
void controlarBomba();
String montarJson();
void processarRequisicao(WiFiEspClient &client);

void setup() {
  Serial.begin(9600);   
  SerialESP.begin(9600);  

  pinMode(PIN_VCC_SENSOR, OUTPUT);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_VCC_SENSOR, LOW);
  digitalWrite(PIN_RELE, HIGH); 
  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);

  WiFi.init(&SerialESP);
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ERRO: Módulo ESP-01 não encontrado!");
    while (true); 
  }

  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado! IP do Servidor:");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void loop() {
  if (millis() - estado.ultimaLeitura > 2000) {
    estado.ultimaLeitura = millis();
    lerSensor();
    controlarBomba();
  }

  WiFiEspClient client = server.available();
  if (client) {
    processarRequisicao(client);
  }
}

void lerSensor() {
  digitalWrite(PIN_VCC_SENSOR, HIGH); 
  delay(10); 
  
  int valorBruto = analogRead(PIN_SENSOR);
  
  digitalWrite(PIN_VCC_SENSOR, LOW); 

  estado.umidade = map(valorBruto, 1023, 0, 0, 100);
  

  if (estado.umidade < 0) estado.umidade = 0;
  if (estado.umidade > 100) estado.umidade = 100;
}

void controlarBomba() {
  if (estado.modoAuto) {
    if (estado.umidade < estado.limiteSeco) {
      estado.bombaLigada = true;
    } else if (estado.umidade >= estado.limiteSeco + 10) { 
      estado.bombaLigada = false;
    }
  }

  if (estado.bombaLigada) {
    digitalWrite(PIN_RELE, LOW);          
    digitalWrite(PIN_LED_VERMELHO, HIGH); 
    digitalWrite(PIN_LED_VERDE, LOW);     
    // tone(PIN_BUZZER, 1000);           
  } else {
    digitalWrite(PIN_RELE, HIGH);        
    digitalWrite(PIN_LED_VERMELHO, LOW);  
    digitalWrite(PIN_LED_VERDE, HIGH);    
    noTone(PIN_BUZZER);                  
  }
}

String montarJson() {
  JsonDocument doc;
  doc["umidade"] = estado.umidade;
  doc["bombaLigada"] = estado.bombaLigada;
  doc["modoAuto"] = estado.modoAuto;
  doc["limiteSeco"] = estado.limiteSeco;

  String output;
  serializeJson(doc, output);
  return output;
}

void processarRequisicao(WiFiEspClient &client) {
  
  String req = client.readStringUntil('\r');
  client.flush();

  if (req.indexOf("GET /ligar") != -1) {
    estado.bombaLigada = true;
    estado.modoAuto = false;
  } else if (req.indexOf("GET /desligar") != -1) {
    estado.bombaLigada = false;
    estado.modoAuto = false;
  } else if (req.indexOf("GET /auto") != -1) {
    estado.modoAuto = true;
  } else if (req.indexOf("GET /manual") != -1) {
    estado.modoAuto = false;
  }
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println(); 
  
  client.println(montarJson());

  delay(10);
  client.stop();
}