# 🚀 Sistema de Radar com ESP-NOW, Sinais Vitais e Google Sheets

Sistema inteligente de detecção de movimento e monitoramento de sinais vitais usando radar mmWave MR60BHA2, comunicação ESP-NOW entre ESPs e armazenamento automático em Google Sheets.

## 📋 Descrição do Projeto

Este projeto implementa um sistema de monitoramento de movimento e sinais vitais em tempo real que:
- **Detecta presença humana** usando radar mmWave de alta precisão
- **Mede sinais vitais** (batimento cardíaco, respiração) quando pessoa está próxima (≤ 1.5m)
- **Transmite dados** via ESP-NOW para economia de bateria
- **Armazena informações** automaticamente no Google Sheets
- **Funciona com deep sleep** para máxima eficiência energética

## 🏗️ Arquitetura do Sistema

```
┌─────────────────┐    ESP-NOW    ┌─────────────────┐    WiFi    ┌─────────────────┐
│   ESP32-C6      │ ────────────→ │   ESP32 WROOM   │ ──────────→ │  Google Sheets  │
│  (Transmissora) │               │   (Receptora)   │            │                 │
│                 │               │                 │            │                 │
│ • Radar MR60BHA2│               │ • ESP-NOW       │            │ • Armazenamento │
│ • ESP-NOW       │               │ • WiFi          │            │ • Histórico     │
│ • Deep Sleep    │               │ • HTTP Client   │            │ • Análise       │
│ • Sinais Vitais │               │ • JSON Parser   │            │ • Sinais Vitais │
└─────────────────┘               └─────────────────┘            └─────────────────┘
```

## 🔧 Componentes Necessários

### Hardware
- **1x XIAO ESP32-C6** (ESP transmissora com radar)
- **1x ESP32 WROOM** (ESP receptora com WiFi)
- **1x Módulo Radar MR60BHA2** (detecção de movimento)
- **Cabo USB-C** para XIAO ESP32-C6
- **Cabo USB** para ESP32 WROOM
- **Jumpers** para conexões

### Software
- **Arduino IDE** 2.0+
- **Core ESP32** (versão 3.3.0+)
- **Biblioteca Seeed Arduino mmWave**
- **Biblioteca ArduinoJson**
- **Biblioteca WiFiManager**

## 📁 Estrutura dos Arquivos

```
Beluga_alt_mini/
├── Beluga_alt_mini.ino          # ESP32-C6 transmissora (radar + ESP-NOW)
└── Beluga_alt_mini_receptor/
    └── Beluga_alt_mini_receptor.ino  # ESP32 WROOM receptora (ESP-NOW + WiFi + Google Sheets)
```

## 🔌 Conexões do Hardware

### ESP32-C6 (Transmissora) + Radar MR60BHA2
```
MR60BHA2    →    XIAO ESP32-C6
TX          →    GPIO 16 (RX)
RX          →    GPIO 17 (TX)
3V3         →    3V3
GND         →    GND
```

### ESP32 WROOM (Receptora)
- **USB**: Conectado ao computador para programação e alimentação
- **WiFi**: Configurado via portal WiFiManager

## ⚙️ Instalação e Configuração

### 1. Preparar o Ambiente Arduino
1. Abrir Arduino IDE
2. Adicionar core ESP32: `Arduino → Ferramentas → Placa → Gerenciar Placas`
3. Procurar por "esp32 by Espressif" e instalar versão 3.3.0+

### 2. Instalar Bibliotecas
```
Arduino → Ferramentas → Gerenciar Bibliotecas
```
- **Seeed Arduino mmWave** (para radar MR60BHA2)
- **ArduinoJson** (para criação de JSON)
- **WiFiManager** (para configuração WiFi)

### 3. Configurar Google Apps Script
1. Criar nova planilha no Google Drive
2. Ir em `Extensões → Apps Script`
3. Colar o código JavaScript fornecido
4. Publicar como aplicativo web
5. Copiar a URL gerada

### 4. Configurar ESPs

#### ESP32-C6 (Transmissora)
1. Abrir `Beluga_alt_mini.ino`
2. Selecionar placa: `XIAO ESP32-C6`
3. Configurar porta serial
4. Compilar e carregar

#### ESP32 WROOM (Receptora)
1. Abrir `Beluga_alt_mini_receptor.ino`
2. Substituir `googleScriptURL` pela URL do Google Apps Script
3. Selecionar placa: `ESP32 Dev Module`
4. Compilar e carregar

## 🔧 Configurações Personalizáveis

### Deep Sleep (ESP32-C6)
```cpp
#define WORK_TIME_MINUTES 30         // Tempo ativo (30 min)
#define SLEEP_TIME_MINUTES 1         // Tempo dormindo (1 min)
#define MOTION_TIMEOUT_MS 10000      // Timeout sem movimento (10s)
```

### ESP-NOW
```cpp
// MAC da ESP receptora (configurar automaticamente)
uint8_t targetAddress[] = {0x43, 0x34, 0x3A, 0x44, 0x45, 0x3A};
```

### WiFi
- Configurado automaticamente via WiFiManager
- Portal de configuração: `ESP_Radar_Config`
- IP de acesso: `192.168.4.1`

## 📊 Estrutura dos Dados

### Dados Transmitidos via ESP-NOW
```cpp
typedef struct {
  bool human_detected;           // Presença humana detectada
  uint8_t num_targets;          // Número de alvos
  float targets_x[10];          // Posição X dos alvos (cm)
  float targets_y[10];          // Posição Y dos alvos (cm)
  int targets_dop[10];          // Índice Doppler
  int targets_cluster[10];      // Índice Cluster
  float targets_speed[10];      // Velocidade dos alvos (cm/s)
  // Dados de sinais vitais (quando pessoa está próxima)
  bool vital_signs_available;   // Sinais vitais disponíveis
  float total_phase;            // Fase total
  float breath_phase;           // Fase de respiração
  float heart_phase;            // Fase cardíaca
  float breath_rate;            // Taxa de respiração (bpm)
  float heart_rate;             // Taxa cardíaca (bpm)
  float distance;               // Distância (cm)
} radar_data_t;
```

### Dados Enviados para Google Sheets
- **Coluna A**: Timestamp (milissegundos desde boot)
- **Coluna B**: Humano detectado (true/false)
- **Coluna C**: Número de alvos
- **Coluna D**: Dados detalhados dos alvos (JSON)
- **Coluna E**: Sinais vitais disponíveis (true/false)
- **Coluna F**: Distância (cm)
- **Coluna G**: Taxa de respiração (bpm)
- **Coluna H**: Taxa cardíaca (bpm)
- **Coluna I**: Fase total
- **Coluna J**: Fase de respiração
- **Coluna K**: Fase cardíaca

## 🚀 Como Usar

### 1. Primeira Configuração
1. **ESP32-C6**: Carregar código e aguardar inicialização
2. **ESP32 WROOM**: Carregar código e configurar WiFi via portal
3. **Anotar MAC**: Copiar MAC da ESP32 WROOM que aparece no Monitor Serial
4. **Configurar MAC**: Substituir MAC na ESP32-C6 transmissora

### 2. Funcionamento Normal
1. **ESP32-C6**: Detecta movimento e envia via ESP-NOW
2. **ESP32-C6**: Se pessoa está ≤ 1.5m, mede sinais vitais automaticamente
3. **ESP32 WROOM**: Recebe dados e envia para Google Sheets
4. **Google Sheets**: Armazena dados em tempo real (movimento + sinais vitais)
5. **Deep Sleep**: ESP32-C6 dorme quando não há movimento

### 3. Monitoramento
- **Monitor Serial**: Logs em tempo real de ambas as ESPs
- **Google Sheets**: Histórico completo de detecções e sinais vitais
- **Deep Sleep**: Controle automático de energia
- **Sinais Vitais**: Medição automática quando pessoa está próxima

## 📱 Portal WiFiManager

### Configuração WiFi
1. ESP32 WROOM cria rede `ESP_Radar_Config`
2. Conectar dispositivo à rede
3. Acessar `192.168.4.1`
4. Configurar nome e senha da rede WiFi
5. Salvar configuração

### Reconexão Automática
- WiFi é salvo na memória
- Reconecta automaticamente ao reiniciar
- Portal reabre se conexão falhar

## 💓 Monitoramento de Sinais Vitais

### Funcionalidades de Saúde
O sistema agora inclui monitoramento automático de sinais vitais quando uma pessoa está próxima (≤ 1.5 metros):

#### Medições Disponíveis
- **Taxa Cardíaca**: Batimentos por minuto (BPM)
- **Taxa de Respiração**: Respirações por minuto (BPM)
- **Fases de Sinal**: Análise de fases de batimento e respiração
- **Distância**: Medição precisa da distância da pessoa

#### Ativação Automática
- **Detecção de Proximidade**: Ativa quando pessoa está ≤ 1.5m
- **Medição Contínua**: Coleta dados enquanto pessoa permanece próxima
- **Economia de Energia**: Não mede quando pessoa está distante
- **Transmissão Inteligente**: Envia dados de sinais vitais junto com detecção de movimento

#### Aplicações Médicas
- **Monitoramento de Saúde**: Acompanhamento contínuo de sinais vitais
- **Detecção de Emergências**: Identificação de valores anômalos
- **Histórico Médico**: Registro automático no Google Sheets
- **Análise de Padrões**: Identificação de tendências de saúde

### Configuração de Sinais Vitais
```cpp
// Distância máxima para ativação (em cm)
#define VITAL_SIGNS_DISTANCE 150.0  // 1.5 metros

// Função de medição automática
void measureVitalSigns() {
  // Mede fases, taxas e distância
  // Armazena dados na estrutura radar_data_t
  // Ativa flag vital_signs_available
}
```

## 🔋 Gerenciamento de Energia

### ESP32-C6 (Transmissora)
- **Modo Ativo**: ~100-200mA (durante detecção)
- **Deep Sleep**: ~10-20μA (99% economia)
- **Ciclo de Trabalho**: 30 min ativo + 1 min dormindo
- **Wake-up**: Botão BOOT, timer ou movimento

### ESP32 WROOM (Receptora)
- **Alimentação**: USB (sem limitação de energia)
- **WiFi**: Sempre ativo para receber dados
- **HTTP**: Envia dados para Google Sheets

## 🐛 Solução de Problemas

### ESP-NOW não funciona
- Verificar se ambas as ESPs estão no mesmo canal WiFi
- Confirmar MAC addresses corretos
- Verificar se ESP-NOW foi inicializado com sucesso

### Radar não detecta
- Verificar conexões TX/RX
- Confirmar alimentação 3.3V
- Verificar se biblioteca mmWave está instalada

### Google Sheets não recebe dados
- Verificar URL do Google Apps Script
- Confirmar se WiFi está funcionando
- Verificar se biblioteca ArduinoJson está instalada

### Deep Sleep não funciona
- Verificar se ESP32-C6 está selecionada como placa
- Confirmar se bibliotecas esp_sleep estão disponíveis
- Verificar configurações de wake-up

### Sinais vitais não são medidos
- Verificar se pessoa está a ≤ 1.5 metros do radar
- Confirmar se radar está detectando presença humana
- Verificar se biblioteca mmWave suporta funções de sinais vitais
- Testar com pessoa parada próxima ao radar

## 📈 Funcionalidades Avançadas

### Alertas Automáticos
- Detecção de humanos
- Monitoramento de velocidade dos alvos
- Medição automática de sinais vitais
- Logs detalhados de eventos

### Análise de Dados
- Histórico completo no Google Sheets
- Posicionamento X/Y dos alvos
- Velocidade e direção do movimento
- Sinais vitais (batimento cardíaco, respiração)
- Timestamps precisos

### Configuração Remota
- WiFi configurável via portal web
- Sem necessidade de reprogramar para mudar rede
- Interface intuitiva para configuração

## 🔒 Segurança

- **ESP-NOW**: Comunicação ponto-a-ponto (não broadcast)
- **WiFi**: Configuração segura via portal
- **Dados**: Transmissão local entre ESPs
- **Google Sheets**: Acesso controlado via Apps Script

## 📝 Logs e Debug

### ESP32-C6 (Transmissora)
```
=== XIAO ESP32-C6 Radar com Deep Sleep ===
ESP-NOW configurado como transmissor
Sinais vitais: Ativados quando pessoa está a ≤ 1.5m
--- Radar detectou dados ---
-----Human Detected-----
Distância detectada: 95.00 cm
🚨 Pessoa próxima detectada! Medindo sinais vitais...
🔍 Medindo sinais vitais...
Fases - Total: 42.15 | Respiração: 11.20 | Coração: 75.80
Taxa de Respiração: 18.00 bpm
Taxa Cardíaca: 68.00 bpm
Distância: 95.00 cm
✅ Sinais vitais medidos com sucesso!
✅ Dados enviados via ESP-NOW
```

### ESP32 WROOM (Receptora)
```
=== ESP RECEPTORA DE RADAR + GOOGLE SHEETS ===
WiFi conectado!
ESP-NOW configurado como receptor
=== DADOS RECEBIDOS VIA ESP-NOW ===
Humano detectado: SIM
Número de alvos: 1

--- SINAIS VITAIS ---
Distância: 95.00 cm
Taxa de Respiração: 18.00 bpm
Taxa Cardíaca: 68.00 bpm
Fases - Total: 42.15 | Respiração: 11.20 | Coração: 75.80

--- DETALHES DOS ALVOS ---
Alvo 1:
  Posição X: -0.22 cm
  Posição Y: 0.31 cm
  Índice Doppler: 0
  Índice Cluster: 0
  Velocidade: 0.00 cm/s

✅ Dados enviados com sucesso para Google Sheets!
```

## 🤝 Contribuições

Este projeto está aberto para melhorias e sugestões. Principais áreas de desenvolvimento:
- Interface web para configuração
- Dashboard em tempo real
- Integração com outros serviços
- Otimizações de energia
- Suporte a múltiplos radares
- Análise avançada de sinais vitais
- Alertas médicos automáticos

## 📄 Licença

Este projeto é fornecido como exemplo educacional. Sinta-se livre para usar, modificar e distribuir conforme suas necessidades.

## 📞 Suporte

Para dúvidas ou problemas:
1. Verificar logs de debug
2. Confirmar configurações de hardware
3. Verificar versões das bibliotecas
4. Testar componentes individualmente

---

**Desenvolvido para demonstração de tecnologias IoT avançadas com ESP32, radar mmWave e integração em nuvem.**
