#pragma once

#define PIN_SENSOR       A0 
#define PIN_RELE         14 
#define PIN_LED_VERDE    12  
#define PIN_LED_VERMELHO 13
#define PIN_BUZZER        4
#define PIN_VCC_SENSOR    5

#ifndef WIFI_SSID
  #define WIFI_SSID "NomeDoHotspot"
#endif

#ifndef WIFI_PASS
  #define WIFI_PASS "SenhaDoHotspot"
#endif

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