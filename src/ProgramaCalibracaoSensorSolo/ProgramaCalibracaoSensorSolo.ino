// Calibração do sensor capacitivo (ESP32)
const int SOIL_PIN = 35;     // ajuste se necessário
const int READS = 20;        // leituras por etapa
const unsigned long INTERVAL = 1500; // ms entre leituras

void setup() {
  Serial.begin(9600);
  analogReadResolution(12); // 0..4095
  delay(2000);
  Serial.println("Calibracao sensor solo - iniciando...");
  Serial.println("Procedimento:");
  Serial.println("1) Coloque sensor em solo SECO -> aguarde estabilizar -> colecione os valores mostrados");
  Serial.println("2) Coloque sensor em solo UMIDO (estado ideal) -> colecione");
  Serial.println("3) Coloque sensor ENCHARCADO -> colecione");
  Serial.println("-------------------------------------------");
  delay(2000);
}

void loop() {
  long sum = 0;
  for (int i = 0; i < READS; i++) {
    int v = analogRead(SOIL_PIN);
    sum += v;
    Serial.printf("Leitura %02d: %d\n", i+1, v);
    delay(INTERVAL / READS);
  }
  int avg = sum / READS;
  Serial.printf("Media de %d leituras = %d\n", READS, avg);
  Serial.println("-------------------------------------------");
  delay(3000); // espera antes do próximo bloco de leituras
}
