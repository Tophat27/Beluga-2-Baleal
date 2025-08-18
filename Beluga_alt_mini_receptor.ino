#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

// URL do Google Apps Script (você vai criar isso)
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbzEyJNkQL3nqI3uXv-M2hCOBinF4iIVG2vJKTQxJ9KpwNFuKnitNvxQC342DAL4XjOU/exec";


// Estrutura para receber dados (deve ser idêntica à transmissora)
typedef struct {
  bool human_detected;
  uint8_t num_targets;
  float targets_x[10];
  float targets_y[10];
  int targets_dop[10];
  int targets_cluster[10];
  float targets_speed[10];
} radar_data_t;

radar_data_t receivedData;
bool newDataReceived = false;
bool wifiConnected = false;
WiFiManager wifiManager;

// Callback quando dados são recebidos (compatível com ESP32 v3.3.0)
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len == sizeof(radar_data_t)) {
    memcpy(&receivedData, data, sizeof(radar_data_t));
    newDataReceived = true;
    
    Serial.println("\n=== DADOS RECEBIDOS VIA ESP-NOW ===");
    Serial.printf("MAC Remetente: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  info->src_addr[0], info->src_addr[1], info->src_addr[2], 
                  info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.printf("Humano detectado: %s\n", receivedData.human_detected ? "SIM" : "NÃO");
    Serial.printf("Número de alvos: %d\n", receivedData.num_targets);
    
    if (receivedData.num_targets > 0) {
      Serial.println("\n--- DETALHES DOS ALVOS ---");
      for (int i = 0; i < receivedData.num_targets; i++) {
        Serial.printf("Alvo %d:\n", i+1);
        Serial.printf("  Posição X: %.2f cm\n", receivedData.targets_x[i]);
        Serial.printf("  Posição Y: %.2f cm\n", receivedData.targets_y[i]);
        Serial.printf("  Índice Doppler: %d\n", receivedData.targets_dop[i]);
        Serial.printf("  Índice Cluster: %d\n", receivedData.targets_cluster[i]);
        Serial.printf("  Velocidade: %.2f cm/s\n", receivedData.targets_speed[i]);
        Serial.println();
      }
    }
    
    Serial.println("================================");
  } else {
    Serial.printf("Erro: Tamanho dos dados incorreto. Esperado: %d, Recebido: %d\n", 
                  sizeof(radar_data_t), len);
  }
}

// Função para configurar WiFi usando WiFiManager
void setupWiFi() {
  Serial.println("Configurando WiFi...");
  
  // Configurar nome do portal de configuração
  wifiManager.setConfigPortalTimeout(180); // 3 minutos de timeout
  wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
    Serial.println("Portal de configuração WiFi ativo");
    Serial.println("Conecte-se à rede 'ESP_Radar_Config'");
    Serial.println("Acesse: 192.168.4.1");
  });
  
  // Tentar conectar ao WiFi salvo
  if (!wifiManager.autoConnect("ESP_Radar_Config")) {
    Serial.println("Falha na conexão WiFi. Reiniciando...");
    delay(3000);
    ESP.restart();
  }
  
  wifiConnected = true;
  Serial.println("WiFi conectado!");
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
}

// Função para enviar dados para o Google Sheets
void sendToGoogleSheets() {
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado. Tentando reconectar...");
    setupWiFi();
    return;
  }
  
  HTTPClient http;
  http.begin(googleScriptURL);
  http.addHeader("Content-Type", "application/json");
  
  // Criar JSON com os dados
  DynamicJsonDocument doc(1024);
  doc["timestamp"] = millis();
  doc["human_detected"] = receivedData.human_detected;
  doc["num_targets"] = receivedData.num_targets;
  
  JsonArray targets = doc.createNestedArray("targets");
  for (int i = 0; i < receivedData.num_targets; i++) {
    JsonObject target = targets.createNestedObject();
    target["x"] = receivedData.targets_x[i];
    target["y"] = receivedData.targets_y[i];
    target["dop_index"] = receivedData.targets_dop[i];
    target["cluster_index"] = receivedData.targets_cluster[i];
    target["speed"] = receivedData.targets_speed[i];
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("Enviando dados para Google Sheets...");
  Serial.println(jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("Resposta HTTP: %d\n", httpResponseCode);
    Serial.println("Resposta: " + response);
    
    if (response.indexOf("success") != -1) {
      Serial.println("✅ Dados enviados com sucesso para Google Sheets!");
    } else {
      Serial.println("❌ Erro ao enviar dados");
    }
  } else {
    Serial.printf("Erro HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  
  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP RECEPTORA DE RADAR + GOOGLE SHEETS ===");
  
  // Configurar WiFi usando WiFiManager
  setupWiFi();
  
  // Mostrar MAC address desta ESP
  Serial.printf("MAC Address desta ESP: %02X:%02X:%02X:%02X:%02X:%02X\n",
                WiFi.macAddress()[0], WiFi.macAddress()[1], WiFi.macAddress()[2],
                WiFi.macAddress()[3], WiFi.macAddress()[4], WiFi.macAddress()[5]);
  Serial.println("Use este MAC address na ESP transmissora!");
  
  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  
  // Registrar callback de recebimento
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("ESP-NOW configurado como receptor");
  Serial.println("Aguardando dados do radar...\n");
}

void loop() {
  // Verificar se há novos dados
  if (newDataReceived) {
    Serial.println("🔄 Processando novos dados recebidos...");
    
    // Enviar dados para Google Sheets
    sendToGoogleSheets();
    
    // Lógica adicional baseada nos dados recebidos
    if (receivedData.human_detected) {
      Serial.println("⚠️  ALERTA: Humano detectado!");
    }
    
    // Verificar velocidade dos alvos
    for (int i = 0; i < receivedData.num_targets; i++) {
      if (receivedData.targets_speed[i] > 50.0) {
        Serial.printf("🚨 Alvo %d movendo-se rapidamente: %.2f cm/s\n", 
                     i+1, receivedData.targets_speed[i]);
      }
    }
    
    newDataReceived = false;
    Serial.println("✅ Dados processados e enviados");
  }
  
  // Verificar conexão WiFi periodicamente
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000) { // A cada 30 segundos
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      Serial.println("WiFi desconectado. Reconectando...");
      setupWiFi();
    }
    lastWiFiCheck = millis();
  }
  
  delay(100);
}
