/****************************************************
 * Projeto: ESP32 - Estação FarmTech
 * Sensores: DHT11 (temp/umidade), LDR (luz),
 *           Sensor de Umidade do Solo (ADC)
 * Atuação:  Módulo de relé (bomba de água)
 * IoT:      Ubidots (via MQTT)
 * Display:  OLED SSD1306
 ****************************************************/

// --- Bibliotecas ---
#include <Wire.h>                 // Comunicação I2C (OLED)
#include <Adafruit_GFX.h>         // Gráficos básicos do display
#include <Adafruit_SSD1306.h>     // Driver do display OLED
#include <DHT.h>                  // Biblioteca para sensor DHT
#include <WiFi.h>                 // Conexão Wi-Fi
#include <UbidotsEsp32Mqtt.h>     // Comunicação MQTT com Ubidots

// --- Configurações de Wi-Fi e Ubidots ---
const char* WIFI_SSID = "SEU NOME DA REDE";    
const char* WIFI_PASS = "SUA SENHA";
const char* UBIDOTS_TOKEN = "SEU TOKEN UBIDOTS";
const char* DEVICE_LABEL = "esp32-farmtech"; 

// Variáveis no Ubidots
const char* VARIABLE_LABEL_TEMP = "temperatura";
const char* VARIABLE_LABEL_UMID = "umidade";
const char* VARIABLE_LABEL_LUZ  = "luz";
const char* VARIABLE_LABEL_SOLO = "solo_percent";

// --- Pinos ---
#define DHTPIN 4
#define DHTTYPE DHT11
#define LDR_PIN 34
#define SOIL_PIN 35
#define RELAY_PIN 23     // Relé ativo-baixo
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MSG_INTERVAL 5000   // Intervalo de envio (ms)

// --- Objetos ---
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Ubidots ubidots(UBIDOTS_TOKEN);

// --- Controle de tempo ---
long lastMsg = 0;

// --- Calibração do Sensor de Solo (valores em ADC) ---
const int ADC_DRY = 2252; // Solo seco
const int ADC_OK  = 1486; // Umidade intermediária
const int ADC_WET = 1074; // Solo encharcado

// --- Controle da bomba ---
const int THRESHOLD_PERCENT = 40;   // Liga bomba abaixo de 40%
const int HYSTERESIS_PERCENT = 10;  // Histerese: desliga 10% acima
float emaSoil = -1;                 // Filtro EMA (exponencial)

// =========================================================
// Função: Conectar ao Wi-Fi
// =========================================================
void setup_wifi() {
  Serial.print("Conectando a ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =========================================================
// Função: Converte leitura ADC → percentual de umidade
// =========================================================
int soilPercent(int rawADC) {
  int percent = map(rawADC, ADC_DRY, ADC_WET, 0, 100);
  return constrain(percent, 0, 100); // Garante 0–100%
}

// =========================================================
// Função: Faz média de N leituras para reduzir ruído
// =========================================================
int readSoilAvg(int N=6) {
  long sum = 0;
  for(int i=0;i<N;i++){
    sum += analogRead(SOIL_PIN);
    delay(10);
  }
  return sum / N;
}

// =========================================================
// Função: Controle da bomba (com filtro + histerese)
// =========================================================
bool pumpControl() {
  int raw = readSoilAvg();          // Leitura do solo (ADC)
  int pct = soilPercent(raw);       // Converte p/ %
  
  // Aplica filtro EMA
  if(emaSoil < 0) emaSoil = pct;
  const float ALPHA = 0.2;          // Peso do filtro
  emaSoil = ALPHA*pct + (1-ALPHA)*emaSoil;
  int smoothPct = (int)round(emaSoil);

  // Lógica de histerese
  static bool pumpOn = false;
  if(!pumpOn && smoothPct <= THRESHOLD_PERCENT){
    pumpOn = true;
  } else if(pumpOn && smoothPct >= (THRESHOLD_PERCENT + HYSTERESIS_PERCENT)){
    pumpOn = false;
  }

  return pumpOn;
}

// =========================================================
// Setup inicial
// =========================================================
void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Relé ativo-baixo → inicia desligado
  digitalWrite(RELAY_PIN, HIGH);

  // Inicializa OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Falha ao inicializar OLED");
    for(;;); // trava se falhar
  }

  // Tela de inicialização
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Iniciando...");
  display.display();
  delay(2000);
  display.clearDisplay();

  // Conexão Wi-Fi e Ubidots
  setup_wifi();
  ubidots.connect();
}

// =========================================================
// Loop principal
// =========================================================
void loop() {
  ubidots.loop();
  long now = millis();

  // Executa a cada MSG_INTERVAL
  if(now - lastMsg > MSG_INTERVAL){
    lastMsg = now;

    // --- Leitura sensores ---
    float umidade = dht.readHumidity();
    float temperatura = dht.readTemperature();
    if(isnan(umidade) || isnan(temperatura)){
      Serial.println("Falha ao ler DHT!");
      return;
    }

    int ldrRaw = digitalRead(LDR_PIN);        // LDR (digital)
    String statusLuz = (ldrRaw==0) ? "Claro" : "Escuro";

    int soilPct = soilPercent(readSoilAvg()); // Solo %
    bool pumpState = pumpControl();           // Controle bomba

    // --- Atuação no relé ---
    digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH); // LOW → liga bomba

    // --- Display OLED ---
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("Temp: "); display.print(temperatura); display.println(" C");
    display.print("Umid: "); display.print(umidade); display.println(" %");
    display.print("Luz: "); display.println(statusLuz);
    display.print("Solo: "); display.print(soilPct); display.println(" %");
    display.print("Bomba: "); display.println(pumpState ? "ON" : "OFF");
    display.display();

    // --- Serial Monitor ---
    Serial.print("Temp: "); Serial.print(temperatura);
    Serial.print(" C | Umid: "); Serial.print(umidade);
    Serial.print(" % | Luz: "); Serial.print(statusLuz);
    Serial.print(" | Solo: "); Serial.print(soilPct);
    Serial.print(" % | Bomba: "); Serial.println(pumpState ? "ON" : "OFF");

    // --- Envio ao Ubidots ---
    ubidots.add(VARIABLE_LABEL_TEMP, temperatura);
    ubidots.add(VARIABLE_LABEL_UMID, umidade);
    ubidots.add(VARIABLE_LABEL_LUZ, ldrRaw==0 ? 1 : 0);
    ubidots.add(VARIABLE_LABEL_SOLO, soilPct);
    ubidots.publish(DEVICE_LABEL);
  }
}
