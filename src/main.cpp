#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"

AsyncWebServer server(80);

struct Estado {
  int  valorSensor   = 0;
  bool nivelBaixo    = false; 
  unsigned long ultimaLeitura = 0;
} estado;

void lerSensor();
void verificarNivel();
String atualizarDashboard(); 
void ativarAlerta();
void desativarAlerta();
void configurarRotas();
void adicionarCORS(AsyncWebServerResponse* response);

void setup() {
  pinMode(PIN_RELE,         OUTPUT);
  pinMode(PIN_LED_VERDE,    OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER,       OUTPUT);

  desativarAlerta();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(PIN_LED_VERMELHO, HIGH);
    delay(200);
    digitalWrite(PIN_LED_VERMELHO, LOW);
    delay(200);
  }

  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_VERDE, HIGH);
    delay(200);
    digitalWrite(PIN_LED_VERDE, LOW);
    delay(200);
  }

  configurarRotas();
  server.begin();
}

void loop() {
  unsigned long agora = millis();

  if (agora - estado.ultimaLeitura >= INTERVALO_LEITURA) {
    estado.ultimaLeitura = agora;
    lerSensor();
    verificarNivel();
  }
}

void lerSensor() {
  estado.valorSensor = analogRead(PIN_SENSOR);

  if (estado.valorSensor > LIMITE_NIVEL) {
    estado.nivelBaixo = true;  
  } else {
    estado.nivelBaixo = false; 
  }
}


void verificarNivel() {
  if (estado.nivelBaixo) {
    ativarAlerta();    
  } else {
    desativarAlerta(); 
  }
}

void ativarAlerta() {
  digitalWrite(PIN_LED_VERMELHO, HIGH);
  digitalWrite(PIN_LED_VERDE,    LOW);  
  digitalWrite(PIN_RELE,         LOW);  
  tone(PIN_BUZZER, 1000);               
}

void desativarAlerta() {
  digitalWrite(PIN_LED_VERDE,    HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW); 
  digitalWrite(PIN_RELE,         HIGH);
  noTone(PIN_BUZZER);                   
}

String atualizarDashboard() {
  JsonDocument doc;

  if (estado.nivelBaixo) {
    doc["status"] = "alerta";
    doc["nivel"]  = "baixo";
    doc["rele"]   = true;
  } else {
    doc["status"] = "normal";
    doc["nivel"]  = "adequado";
    doc["rele"]   = false;
  }

  doc["sensor"] = estado.valorSensor;

  String output;
  serializeJson(doc, output);
  return output;
}

void adicionarCORS(AsyncWebServerResponse* response) {
  response->addHeader("Access-Control-Allow-Origin",  "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void configurarRotas() {
  server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(204);
    adicionarCORS(response);
    request->send(response);
  });


  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(
      200, "application/json", atualizarDashboard()
    );
    adicionarCORS(response);
    request->send(response);
  });
}