#include <Arduino.h>
#include "config.h"

// ---------- VARIÁVEIS GLOBAIS ----------

int           valorSensor  = 0;
int           porcentagem  = 0;
bool          irrigando    = false;
unsigned long inicioBomba  = 0;
unsigned long ultimaLeitura = 0;

// ---------- PROTÓTIPOS ----------

void ligarIrrigacao();
void desligarIrrigacao();
void verificarUmidade();
void alertaSonoro();
void mostrarMonitorSerial();
void mostrarBarraUmidade(int pct);

// SETUP

void setup() {
  pinMode(PIN_RELE,         OUTPUT);
  pinMode(PIN_LED_VERDE,    OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER,       OUTPUT);

  Serial.begin(9600);

  desligarIrrigacao();

  Serial.println(F("============================================"));
  Serial.println(F("   SISTEMA DE IRRIGACAO AUTOMATICA ATIVO   "));
  Serial.println(F("============================================"));

  DEBUG_PRINT(F("  Limite seco   : ")); DEBUG_PRINTLN(LIMITE_SECO);
  DEBUG_PRINT(F("  Histerese     : ")); DEBUG_PRINTLN(HISTERESE);
  DEBUG_PRINT(F("  Tempo max     : "));
  DEBUG_PRINT(TEMPO_MAX_BOMBA / 1000);
  DEBUG_PRINTLN(F("s"));
  DEBUG_PRINT(F("  Modo debug    : "));
  DEBUG_PRINTLN(DEBUG_MODE ? F("ATIVO") : F("INATIVO"));

  Serial.println(F("============================================"));
  Serial.println();
}

// LOOP PRINCIPAL

void loop() {
  unsigned long agora = millis();

  if (agora - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = agora;

    valorSensor = analogRead(PIN_SENSOR);
    porcentagem = map(valorSensor, 1023, 0, 0, 100);
    porcentagem = constrain(porcentagem, 0, 100);

    mostrarMonitorSerial();
    verificarUmidade();
  }

  if (irrigando && (agora - inicioBomba >= TEMPO_MAX_BOMBA)) {
    Serial.println(F(">>> AVISO: Tempo maximo atingido! Bomba desligada por seguranca."));
    desligarIrrigacao();
  }
}

// VERIFICAR UMIDADE (COM HISTERESE)

void verificarUmidade() {
  if (!irrigando && valorSensor > LIMITE_SECO) {
    ligarIrrigacao();
  } else if (irrigando && valorSensor < (LIMITE_SECO - HISTERESE)) {
    desligarIrrigacao();
  }
}

// LIGAR IRRIGAÇÃO

void ligarIrrigacao() {
  irrigando   = true;
  inicioBomba = millis();

  digitalWrite(PIN_RELE,         LOW);
  digitalWrite(PIN_LED_VERMELHO, HIGH);
  digitalWrite(PIN_LED_VERDE,    LOW);

  alertaSonoro();

  Serial.println(F(">>> STATUS : SOLO SECO"));
  Serial.println(F(">>> BOMBA  : LIGADA"));
  Serial.println(F("--------------------------------------------"));
}

// DESLIGAR IRRIGAÇÃO

void desligarIrrigacao() {
  irrigando = false;

  digitalWrite(PIN_RELE,         HIGH);
  digitalWrite(PIN_LED_VERDE,    HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  noTone(PIN_BUZZER);

  Serial.println(F(">>> STATUS : SOLO UMIDO"));
  Serial.println(F(">>> BOMBA  : DESLIGADA"));
  Serial.println(F("--------------------------------------------"));
}

// ALERTA SONORO (3 bipes curtos)

void alertaSonoro() {
  for (int i = 0; i < 3; i++) {
    tone(PIN_BUZZER, 1000);
    delay(150);
    noTone(PIN_BUZZER);
    delay(100);
  }
}

// MONITOR SERIAL

void mostrarMonitorSerial() {
  Serial.println(F("============================================"));

  Serial.print(F("  Sensor bruto : "));
  Serial.print(valorSensor);
  Serial.print(F(" / 1023   |   Umidade: "));
  Serial.print(porcentagem);
  Serial.println(F("%"));

  Serial.print(F("  "));
  mostrarBarraUmidade(porcentagem);

  if (irrigando) {
    unsigned long segundos = (millis() - inicioBomba) / 1000;
    Serial.print(F("  Irrigando ha  : "));
    Serial.print(segundos);
    Serial.print(F("s / "));
    Serial.print(TEMPO_MAX_BOMBA / 1000);
    Serial.println(F("s max"));
  }

  Serial.println(F("--------------------------------------------"));
}

// BARRA DE PROGRESSO DA UMIDADE

void mostrarBarraUmidade(int pct) {
  int barras = pct / 5; // 20 blocos = 100%
  Serial.print(F("["));
  for (int i = 0; i < 20; i++) {
    Serial.print(i < barras ? F("#") : F("-"));
  }
  Serial.print(F("] "));
  Serial.print(pct);
  Serial.println(F("%"));
}