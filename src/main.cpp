// ==========================================
// main.cpp — Irrigação Automática (ESP8266)
// ==========================================
// Placa  : ESP8266 (NodeMCU / ESP-12E)
// IDE    : PlatformIO
// Comunicação: WiFi + API REST (HTTP)
// ==========================================
//
// ROTAS DA API (para o frontend):
//
//   GET  /status            → estado completo do sistema
//   POST /bomba/ligar       → liga a bomba (modo manual)
//   POST /bomba/desligar    → desliga a bomba (modo manual)
//   POST /modo              → troca entre auto e manual
//   POST /config            → atualiza limite e tempo máximo
//   POST /emergencia        → para tudo imediatamente
//
// ==========================================

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"


AsyncWebServer server(80);


struct Estado {
  int           valorSensor     = 0;
  int           porcentagem     = 0;
  bool          irrigando       = false;
  bool          modoAuto        = true;
  bool          alertaSeguranca = false;
  int           limiteSeco      = LIMITE_SECO;
  unsigned long tempoMaxBomba   = TEMPO_MAX_BOMBA;
  unsigned long inicioBomba     = 0;
  unsigned long ultimaLeitura   = 0;
} estado;


void ligarIrrigacao();
void desligarIrrigacao();
void verificarUmidade();
void alertaSonoro();
void lerSensor();
String montarJsonStatus();
void configurarRotas();
void adicionarCORS(AsyncWebServerResponse* response);

void setup() {
  pinMode(PIN_VCC_SENSOR,   OUTPUT);
  digitalWrite(PIN_VCC_SENSOR, LOW);

  pinMode(PIN_RELE,         OUTPUT);
  pinMode(PIN_LED_VERDE,    OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER,       OUTPUT);

  desligarIrrigacao();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
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

    if (estado.modoAuto) {
      verificarUmidade();
    }
  }

  if (estado.irrigando && (agora - estado.inicioBomba >= estado.tempoMaxBomba)) {
    estado.alertaSeguranca = true;
    desligarIrrigacao();
  }
}

void lerSensor() {
  digitalWrite(PIN_VCC_SENSOR, HIGH); 
  delay(10);

  estado.valorSensor = analogRead(PIN_SENSOR);
  
  digitalWrite(PIN_VCC_SENSOR, LOW);  

  estado.porcentagem = map(estado.valorSensor, 1023, 0, 0, 100);
  estado.porcentagem = constrain(estado.porcentagem, 0, 100);
}

void verificarUmidade() {
  if (!estado.irrigando && estado.valorSensor > estado.limiteSeco) {
    ligarIrrigacao();
  } else if (estado.irrigando && estado.valorSensor < (estado.limiteSeco - HISTERESE)) {
    desligarIrrigacao();
  }
}

void ligarIrrigacao() {
  estado.irrigando       = true;
  estado.alertaSeguranca = false;
  estado.inicioBomba     = millis();

  digitalWrite(PIN_RELE,         LOW);
  digitalWrite(PIN_LED_VERMELHO, HIGH);
  digitalWrite(PIN_LED_VERDE,    LOW);

  alertaSonoro();
}

void desligarIrrigacao() {
  estado.irrigando = false;

  digitalWrite(PIN_RELE,         HIGH);
  digitalWrite(PIN_LED_VERDE,    HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  noTone(PIN_BUZZER);
}

void alertaSonoro() {
  for (int i = 0; i < 3; i++) {
    tone(PIN_BUZZER, 1000);
    delay(150);
    noTone(PIN_BUZZER);
    delay(100);
  }
}

String montarJsonStatus() {
  JsonDocument doc;

  doc["umidade"]         = estado.porcentagem;
  doc["valorSensor"]     = estado.valorSensor;
  doc["irrigando"]       = estado.irrigando;
  doc["modoAuto"]        = estado.modoAuto;
  doc["alertaSeguranca"] = estado.alertaSeguranca;
  doc["limiteSeco"]      = estado.limiteSeco;
  doc["tempoMaxBomba"]   = estado.tempoMaxBomba;
  doc["ip"]              = WiFi.localIP().toString();

  if (estado.irrigando) {
    doc["tempoIrrigando"] = (millis() - estado.inicioBomba) / 1000;
  } else {
    doc["tempoIrrigando"] = 0;
  }

  String output;
  serializeJson(doc, output);
  return output;
}

void adicionarCORS(AsyncWebServerResponse* response) {
  response->addHeader("Access-Control-Allow-Origin",  "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void configurarRotas() {

  server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Origin",  "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(
      200, "application/json", montarJsonStatus()
    );
    adicionarCORS(response);
    request->send(response);
  });

  server.on("/bomba/ligar", HTTP_POST, [](AsyncWebServerRequest* request) {
    ligarIrrigacao();
    AsyncWebServerResponse* response = request->beginResponse(
      200, "application/json", "{\"ok\":true,\"acao\":\"bomba_ligada\"}"
    );
    adicionarCORS(response);
    request->send(response);
  });

  server.on("/bomba/desligar", HTTP_POST, [](AsyncWebServerRequest* request) {
    desligarIrrigacao();
    AsyncWebServerResponse* response = request->beginResponse(
      200, "application/json", "{\"ok\":true,\"acao\":\"bomba_desligada\"}"
    );
    adicionarCORS(response);
    request->send(response);
  });

  server.addHandler(new AsyncCallbackJsonWebHandler("/modo",
    [](AsyncWebServerRequest* request, JsonVariant& json) {
      String modo = json["modo"].as<String>();
      if (modo == "auto") {
        estado.modoAuto = true;
      } else if (modo == "manual") {
        estado.modoAuto = false;
        desligarIrrigacao();
      } else {
        AsyncWebServerResponse* err = request->beginResponse(
          400, "application/json", "{\"ok\":false,\"erro\":\"modo invalido\"}"
        );
        adicionarCORS(err);
        request->send(err);
        return;
      }
      AsyncWebServerResponse* response = request->beginResponse(
        200, "application/json", "{\"ok\":true}"
      );
      adicionarCORS(response);
      request->send(response);
    }
  ));

  server.addHandler(new AsyncCallbackJsonWebHandler("/config",
    [](AsyncWebServerRequest* request, JsonVariant& json) {
      if (json.containsKey("limiteSeco")) {
        int novoLimite = json["limiteSeco"].as<int>();
        if (novoLimite >= 0 && novoLimite <= 1023) {
          estado.limiteSeco = novoLimite;
        }
      }
      if (json.containsKey("tempoMaxBomba")) {
        unsigned long novoTempo = json["tempoMaxBomba"].as<unsigned long>();
        if (novoTempo >= 5000 && novoTempo <= 300000) {
          estado.tempoMaxBomba = novoTempo;
        }
      }
      AsyncWebServerResponse* response = request->beginResponse(
        200, "application/json", "{\"ok\":true}"
      );
      adicionarCORS(response);
      request->send(response);
    }
  ));

  server.on("/emergencia", HTTP_POST, [](AsyncWebServerRequest* request) {
    estado.modoAuto = false;
    desligarIrrigacao();
    AsyncWebServerResponse* response = request->beginResponse(
      200, "application/json", "{\"ok\":true,\"acao\":\"emergencia_ativada\"}"
    );
    adicionarCORS(response);
    request->send(response);
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"ok\":false,\"erro\":\"rota nao encontrada\"}");
  });
}