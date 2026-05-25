#pragma once

// ---------- PINOS ----------

#define PIN_SENSOR      A0
#define PIN_RELE         7
#define PIN_LED_VERDE    8
#define PIN_LED_VERMELHO 9
#define PIN_BUZZER      10

// ---------- PARÂMETROS (fallback) ----------
// Usados apenas se não definidos via build_flags

#ifndef LIMITE_SECO
  #define LIMITE_SECO 600  
#endif

#ifndef HISTERESE
  #define HISTERESE 50  
#endif

#ifndef TEMPO_MAX_BOMBA
  #define TEMPO_MAX_BOMBA 30000
#endif

#ifndef INTERVALO_LEITURA
  #define INTERVALO_LEITURA 1000 
#endif

// ---------- DEBUG ----------
// DEBUG_MODE 1 → imprime logs detalhados no Serial
// DEBUG_MODE 0 → apenas mensagens essenciais (versão final)

#ifndef DEBUG_MODE
  #define DEBUG_MODE 1
#endif

#if DEBUG_MODE
  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif