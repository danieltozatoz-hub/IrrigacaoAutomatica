#pragma once

// ---------- PINOS (ESP8266) ----------
#define PIN_SENSOR       A0   
#define PIN_RELE         14    
#define PIN_LED_VERDE    12   
#define PIN_LED_VERMELHO 13   
#define PIN_BUZZER        4   

#ifndef WIFI_SSID
  #define WIFI_SSID "NomeDoHotspot"
#endif

#ifndef WIFI_PASS
  #define WIFI_PASS "SenhaDoHotspot"
#endif

#ifndef LIMITE_NIVEL

  #define LIMITE_NIVEL 500  
#endif

#ifndef INTERVALO_LEITURA
  #define INTERVALO_LEITURA 1000 
#endif