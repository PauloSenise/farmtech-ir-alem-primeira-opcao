# Documentação Técnica - FIAP - Ir Além 2025

## Introdução
O **FarmTech** nasce da necessidade de levar tecnologia acessível ao campo, auxiliando pequenos produtores no controle e automação da irrigação.  
O projeto foi desenvolvido como parte do desafio **Ir Além – FIAP 2025**, com foco em **IoT (Internet of Things)**, **automação agrícola** e **monitoramento remoto**.

---

## Objetivos do Projeto
- Melhorar a eficiência do uso da água na agricultura.  
- Oferecer uma solução de **baixo custo e fácil implementação**.  
- Automatizar o processo de irrigação, reduzindo desperdício.  
- Disponibilizar informações em tempo real, localmente (OLED) e remotamente (Ubidots).  

---

## Componentes Utilizados

### Hardware
- **ESP32 NodeMCU** – Microcontrolador com Wi-Fi integrado.  
- **DHT11** – Sensor de temperatura e umidade do ar.  
- **LDR** – Sensor de luminosidade (claro/escuro).  
- **Sensor de Umidade do Solo** – Mede o percentual de umidade no solo.  
- **Display OLED** – Exibição local dos dados.  
- **Módulo Relé** – Acionamento automático da bomba de irrigação.  

### Software
- **Arduino IDE** – Desenvolvimento e upload do código.  
- **Ubidots** – Plataforma IoT para armazenamento e visualização dos dados.  

---
## Funcionamento Técnico

- O ESP32 lê os sensores (DHT11, LDR, Solo).  
- Aplica **filtro EMA** para suavizar ruídos no sensor de solo.  
- Usa **histerese** para controle do relé (liga bomba ≤ 40%, desliga ≥ 50%).  
- Mostra valores no **display OLED**.  
- Envia dados ao **Ubidots via MQTT**.  

---

## Protótipo e Componentes

### Circuito Completo
<img src="/assets/circuitoEsp32sensores.png" alt="Circuito completo ESP32 com sensores" width="500"/>

### Display OLED
<img src="/assets/displayOLED.png" alt="Display OLED mostrando dados" width="150"/>

### Módulo Relé
<img src="/assets/moduloRele.png" alt="Módulo Relé" width="150"/>

### Sensor DHT11
<img src="/assets/sensorDHT11.png" alt="Sensor DHT11" width="150"/>

### Sensor LDR
<img src="/assets/sensorLDR.png" alt="Sensor LDR" width="150"/>

### Sensor de Umidade do Solo
<img src="/assets/sensorSolo.png" alt="Sensor de Umidade do Solo Capacitivo" width="400"/>

---

## Calibração do Sensor de Solo

O sensor capacitivo de umidade do solo não fornece valores em **percentual direto (%)**.  
Em vez disso, ele retorna valores **analógicos (0–4095)** no ESP32, que variam conforme a umidade presente.

Para que o sistema possa **converter corretamente os valores em %**, é necessário um **procedimento de calibração**.

### Procedimento de Calibração

1. **Solo seco**  
   - Coloque o sensor em solo **completamente seco**.  
   - Aguarde alguns segundos até a leitura estabilizar.  
   - Registre a **média** das leituras exibidas no monitor serial.  
   - Esse valor será usado como **ponto de referência mínimo** (ex.: 3200).  

2. **Solo úmido (condição ideal de irrigação)**  
   - Coloque o sensor em solo **levemente úmido**, que representa a condição ideal da planta.  
   - Novamente aguarde estabilizar e registre a média.  
   - Esse valor pode ser usado como **faixa de referência ideal**.  

3. **Solo encharcado**  
   - Coloque o sensor em um solo **molhado/encharcado** (com água visível).  
   - Aguarde estabilizar e registre a média.  
   - Esse valor será o **ponto máximo de saturação** (ex.: 1200).  

---

## Fluxo de Dados e Processamento

1. **Leitura dos Sensores**  
   O ESP32 coleta, a cada 5 segundos, dados de temperatura, umidade do ar, luminosidade e umidade do solo.

2. **Tratamento de Sinal**  
   - Aplicação de **Filtro EMA (Exponential Moving Average)** para suavizar leituras do sensor de solo.  
   - Uso de **histerese** para evitar acionamento/desligamento frequente da bomba.

3. **Decisão Automática**  
   - Se **umidade do solo ≤ 40% → liga a bomba**.  
   - Se **umidade do solo ≥ 50% → desliga a bomba**.  

4. **Exibição Local**  
   O display OLED mostra os valores lidos e o estado atual da bomba.

5. **Envio para a Nuvem**  
   Dados são enviados ao **Ubidots**, onde podem ser analisados em dashboards e gráficos.

---

### Exemplo de Resultados

| Condição      | Média (ADC 0–4095) | Interpretação |
|---------------|--------------------|---------------|
| Solo Seco     | ~3200              | 0% umidade    |
| Solo Ideal    | ~2200              | 50% umidade   |
| Solo Encharc. | ~1200              | 100% umidade  |

> Esses valores variam conforme o tipo de solo, a temperatura e até o modelo do sensor. Por isso **cada sensor deve ser calibrado individualmente**.

---

### Conversão para Percentual

Uma vez obtidos os valores de referência, a leitura pode ser normalizada para porcentagem:

Umidade(%) = (ValorSeco - ValorLido) / (ValorSeco - ValorEncharcado) × 100

- Se `ValorLido` ≥ `ValorSeco` → 0% (solo seco).  
- Se `ValorLido` ≤ `ValorEncharcado` → 100% (solo saturado).  
- Caso contrário → cálculo proporcional.  

---

### Código Utilizado para Calibração

```cpp
// Sketch de calibração do sensor capacitivo (ESP32)
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
```

## Integração com Ubidots

O **Ubidots** é a plataforma IoT escolhida para este projeto, permitindo o envio, armazenamento e visualização dos dados coletados pelo ESP32.

### Comunicação
- Conexão via **MQTT** com biblioteca `UbidotsEsp32Mqtt`.  
- Autenticação por **Token** do workspace.  
- Envio periódico a cada 5 segundos.  

### Variáveis enviadas
- `temperatura` – leitura do DHT11 (°C)  
- `umidade` – leitura do DHT11 (%)  
- `luz` – leitura do LDR (0 = claro, 1 = escuro)  
- `solo_percent` – leitura do sensor de solo (%)  

### Criação automática
- Na primeira publicação, o Ubidots cria automaticamente:  
  - **Device** com o label definido em `DEVICE_LABEL` (ex.: `esp32-farmtech`)  
  - **Variáveis** correspondentes às adicionadas no código (`ubidots.add(...)`)  

### Dashboard
- Dashboard configurado com gráficos e indicadores para:  
  - Temperatura (linha)  
  - Umidade (linha)  
  - Solo (%) (gauge e linha)  
  - Estado da bomba (indicador ON/OFF)  

### Benefícios
- Monitoramento remoto centralizado.  
- Histórico de dados armazenado na nuvem.  
- Facilidade de criar alertas automáticos.  
- Integração futura com APIs externas e IA preditiva.  

---

## Referências
- [Documentação Ubidots](https://ubidots.com/docs/)  
- [Biblioteca Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)  
- [Biblioteca DHT](https://github.com/adafruit/DHT-sensor-library)  

---

---

### Conclusão
A calibração é essencial para transformar os valores crus do sensor em **informações confiáveis**.  
Depois de calibrado, o sistema pode **interpretar corretamente a umidade do solo em %** e acionar a bomba de irrigação de forma precisa.

---

## Resultados Esperados
- Monitoramento contínuo da lavoura.  
- Redução de desperdício de água.  
- Histórico de leituras disponível no Ubidots.  
- Base inicial para análises futuras com **Machine Learning**.  

---

## Equipe
Projeto desenvolvido pelo **Grupo 36 – FIAP 2025**  
Curso de **Inteligência Artificial** – 2º Semestre  
