#include <Arduino.h>
#include "Seeed_Arduino_mmWave.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// If the board is an ESP32, include the HardwareSerial library and create a
// HardwareSerial object for the mmWave serial communication
#ifdef ESP32
#  include <HardwareSerial.h>
HardwareSerial mmWaveSerial(0);
#else
// Otherwise, define mmWaveSerial as Serial1
#  define mmWaveSerial Serial1
#endif

SEEED_MR60BHA2 mmWave;

// Configurações de Deep Sleep para XIAO ESP32-C6
#define WAKEUP_PIN GPIO_NUM_0        // Botão BOOT da XIAO ESP32-C6
#define WORK_TIME_MINUTES 30         // Tempo de trabalho ativo (30 minutos)
#define SLEEP_TIME_MINUTES 1         // Tempo de deep sleep (1 minuto)
#define MOTION_TIMEOUT_MS 10000      // Timeout sem movimento para entrar em deep sleep (10s)
#define DEEP_SLEEP_ENABLED true      // Habilitar/desabilitar deep sleep

// Estrutura para enviar dados via ESP-NOW
typedef struct {
  bool human_detected;
  uint8_t num_targets;
  float targets_x[10];  // Máximo 10 alvos
  float targets_y[10];
  int targets_dop[10];
  int targets_cluster[10];
  float targets_speed[10];
} radar_data_t;

radar_data_t radarData;
esp_now_peer_info_t peerInfo;

// Variáveis para controle de deep sleep
unsigned long lastMotionTime = 0;
bool motionDetected = false;
unsigned long bootTime = 0;

// Endereço MAC da ESP32 WROOM receptora
uint8_t targetAddress[] = {0x43, 0x34, 0x3A, 0x44, 0x45, 0x3A};

// Callback quando dados são enviados (ajustado para ESP32-C6)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Dados enviados com sucesso via ESP-NOW");
  } else {
    Serial.println("Erro ao enviar dados via ESP-NOW");
  }
}

// Função para configurar deep sleep
void setupDeepSleep() {
  Serial.println("Configurando Deep Sleep...");
  
  // Para ESP32-C6, usar ext1 em vez de ext0
  // Configurar wake-up por GPIO (botão BOOT)
  esp_sleep_enable_ext1_wakeup((1ULL << WAKEUP_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);
  
  // Configurar timer de wake-up para 1 minuto
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_MINUTES * 60 * 1000000ULL); // Converter para microssegundos
  
  Serial.printf("Deep Sleep configurado para %d minuto(s)\n", SLEEP_TIME_MINUTES);
  Serial.printf("Ciclo de trabalho: %d minutos ativo, %d minuto(s) dormindo\n", WORK_TIME_MINUTES, SLEEP_TIME_MINUTES);
  Serial.printf("Wake-up por botão BOOT (GPIO %d) ou timer\n", WAKEUP_PIN);
}

// Função para entrar em deep sleep
void enterDeepSleep() {
  Serial.println("Entrando em Deep Sleep...");
  Serial.printf("Tempo de sono: %d minuto(s)\n", SLEEP_TIME_MINUTES);
  Serial.printf("Próximo ciclo de trabalho: %d minutos\n", WORK_TIME_MINUTES);
  Serial.println("Pressione o botão BOOT para acordar imediatamente ou aguarde o timer");
  
  // Pequeno delay para permitir que a mensagem seja exibida
  delay(1000);
  
  // Entrar em deep sleep
  esp_deep_sleep_start();
}

// Função para verificar se deve entrar em deep sleep
void checkDeepSleep() {
  if (!DEEP_SLEEP_ENABLED) return;
  
  unsigned long currentTime = millis();
  unsigned long workTime = currentTime - bootTime;
  unsigned long workTimeMinutes = workTime / (60 * 1000); // Converter para minutos
  
  // Se não há movimento há muito tempo, entrar em deep sleep
  if (motionDetected && (currentTime - lastMotionTime) > MOTION_TIMEOUT_MS) {
    Serial.println("Sem movimento detectado. Entrando em Deep Sleep...");
    enterDeepSleep();
  }
  
  // Se completou o tempo de trabalho (30 minutos), entrar em deep sleep
  if (workTimeMinutes >= WORK_TIME_MINUTES) {
    Serial.printf("Ciclo de trabalho completo (%d minutos). Entrando em Deep Sleep...\n", workTimeMinutes);
    enterDeepSleep();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Verificar se acordou do deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Acordou pelo botão BOOT");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.printf("Acordou pelo timer após %d minuto(s) de sono\n", SLEEP_TIME_MINUTES);
      break;
    default:
      Serial.println("Acordou por outro motivo");
      break;
  }
  
  Serial.println("=== XIAO ESP32-C6 Radar com Deep Sleep ===");
  
  // Configurar deep sleep
  setupDeepSleep();
  
  // Inicializar radar
  mmWave.begin(&mmWaveSerial);
  
  // Configurar ESP-NOW
  WiFi.mode(WIFI_STA);
  
  // Configurar canal WiFi para melhor compatibilidade ESP-NOW
  WiFi.begin();
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  
  // Configurar peer (receptor ESP-NOW)
  // MAC address da ESP32 WROOM receptora
  memcpy(peerInfo.peer_addr, targetAddress, 6);
  peerInfo.channel = WiFi.channel(); // Usar o mesmo canal da rede WiFi
  peerInfo.encrypt = false;
  
  Serial.printf("Canal WiFi: %d\n", WiFi.channel());
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Erro ao adicionar peer");
    return;
  }
  
  Serial.println("ESP-NOW configurado como transmissor");
  Serial.printf("Enviando dados para ESP: %02X:%02X:%02X:%02X:%02X:%02X\n",
                targetAddress[0], targetAddress[1], targetAddress[2],
                targetAddress[3], targetAddress[4], targetAddress[5]);
  Serial.printf("Deep Sleep ativo: %s\n", DEEP_SLEEP_ENABLED ? "SIM" : "NÃO");
  Serial.printf("Ciclo de trabalho: %d min ativo, %d min dormindo\n", WORK_TIME_MINUTES, SLEEP_TIME_MINUTES);
  Serial.printf("Timeout sem movimento: %d ms\n", MOTION_TIMEOUT_MS);
  
  // Mostrar MAC address desta ESP
  Serial.printf("MAC Address desta ESP: %02X:%02X:%02X:%02X:%02X:%02X\n",
                WiFi.macAddress()[0], WiFi.macAddress()[1], WiFi.macAddress()[2],
                WiFi.macAddress()[3], WiFi.macAddress()[4], WiFi.macAddress()[5]);
  
  bootTime = millis();
}

void loop() {
  if (mmWave.update(100)) {
    Serial.println("--- Radar detectou dados ---");
    
    // Limpar dados anteriores
    memset(&radarData, 0, sizeof(radar_data_t));
    
    // Resetar timer de movimento
    lastMotionTime = millis();
    motionDetected = true;
    
    if (mmWave.isHumanDetected()) {
      Serial.printf("-----Human Detected-----\n");
      radarData.human_detected = true;
    }

    PeopleCounting target_info;
    if (mmWave.getPeopleCountingTartgetInfo(target_info)) {
      Serial.printf("-----Got Target Info-----\n");
      Serial.printf("Number of targets: %zu\n", target_info.targets.size());
      
      radarData.num_targets = (uint8_t)min(target_info.targets.size(), (size_t)10);
      
      for (size_t i = 0; i < target_info.targets.size() && i < 10; i++) {
        const auto& target = target_info.targets[i];
        Serial.printf("Target %zu:\n", i + 1);
        Serial.printf("  x_point: %.2f\n", target.x_point);
        Serial.printf("  y_point: %.2f\n", target.y_point);
        Serial.printf("  dop_index: %d\n", target.dop_index);
        Serial.printf("  cluster_index: %d\n", target.cluster_index);
        Serial.printf("  move_speed: %.2f cm/s\n", target.dop_index * RANGE_STEP);
        
        // Armazenar dados para envio
        radarData.targets_x[i] = target.x_point;
        radarData.targets_y[i] = target.y_point;
        radarData.targets_dop[i] = target.dop_index;
        radarData.targets_cluster[i] = target.cluster_index;
        radarData.targets_speed[i] = target.dop_index * RANGE_STEP;
      }
      
      // Enviar dados via ESP-NOW
      Serial.println("Tentando enviar dados via ESP-NOW...");
      esp_err_t result = esp_now_send(targetAddress, (uint8_t*)&radarData, sizeof(radar_data_t));
      if (result == ESP_OK) {
        Serial.println("✅ Dados enviados via ESP-NOW");
      } else {
        Serial.printf("❌ Erro ao enviar dados via ESP-NOW: %s\n", esp_err_to_name(result));
      }
    } else {
      Serial.println("❌ Falha ao obter dados do radar");
    }
    
    delay(500);
  } else {
    // Se não há dados do radar, verificar deep sleep
    checkDeepSleep();
  }
  
  // Mostrar tempo de trabalho a cada 5 minutos
  static unsigned long lastTimeDisplay = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastTimeDisplay > 300000) { // A cada 5 minutos
    unsigned long workTimeMinutes = (currentTime - bootTime) / (60 * 1000);
    Serial.printf("⏱️  Tempo de trabalho: %lu minutos / %d minutos\n", workTimeMinutes, WORK_TIME_MINUTES);
    lastTimeDisplay = currentTime;
  }
}