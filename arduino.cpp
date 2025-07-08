#define BLYNK_TEMPLATE_ID "TMPL2oatTA_0_"
#define BLYNK_TEMPLATE_NAME "Estação Meteorológica"
#define BLYNK_AUTH_TOKEN "vo3kL1co1K8Pl12xO-heZMu1Nm29xPeI"

#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// === CONEXÃO COM WIFI E BLYNK ===
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "ROBOTICA 1";
char pass[] = "";

// === LCD I2C ===
LiquidCrystal_I2C lcd(0x27, 16, 2); // Se não funcionar, teste com 0x3F

// === SENSOR DHT11 ===
#define DHTPIN D5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// === SENSOR DE VENTO (ENCODER) ===
const int pinSensor = D6; // Use D5 (GPIO14) para evitar conflito com I2C
volatile unsigned int contagem = 0;
unsigned long tempoAnterior = 0;
const float raio = 0.15;              // Raio da hélice em metros
const int VoltaCompleta = 20;       // Pulsos por volta

// === INTERRUPÇÃO CORRIGIDA COM IRAM ===
ICACHE_RAM_ATTR void contarPulso() {
  contagem++;
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(pinSensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinSensor), contarPulso, RISING);

  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();
  Blynk.begin(auth, ssid, pass);

  lcd.setCursor(0, 0);
  lcd.print("Estacao Online");
  delay(2000);
  lcd.clear();

  tempoAnterior = millis();
}

void loop() {
  Blynk.run();
  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoAnterior >= 1000) {
    noInterrupts();
    unsigned int pulsos = contagem;
    contagem = 0;
    interrupts();

    float voltas = pulsos / (float)VoltaCompleta;
    float freq = voltas / 1.0;
    float velocidade = 2 * 3.1416 * raio * freq;
    float velocidade_kmh = velocidade * 3.6;

    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();

    // === LCD ===
    lcd.setCursor(0, 0);
    lcd.print("Vento:");
    lcd.setCursor(6, 0);
    lcd.print("      ");
    lcd.setCursor(6, 0);
    lcd.print(velocidade_kmh, 1);
    lcd.print("km/h");

    lcd.setCursor(0, 1);
    if (!isnan(temperatura) && !isnan(umidade)) {
      lcd.print("T:");
      lcd.print(temperatura, 1);
      lcd.write(223); // símbolo °
      lcd.print("C ");
      lcd.print("U:");
      lcd.print(umidade, 0);
      lcd.print("%");
    } else {
      lcd.print("Erro no DHT22");
    }

    // === Serial Monitor (Debug) ===
    Serial.print("Velocidade: ");
    Serial.print(velocidade_kmh);
    Serial.println(" km/h");

    if (!isnan(temperatura) && !isnan(umidade)) {
      Serial.print("Temperatura: ");
      Serial.print(temperatura);
      Serial.println(" *C");

      Serial.print("Umidade: ");
      Serial.print(umidade);
      Serial.println(" %");
    } else {
      Serial.println("Falha ao ler DHT.");
    }

    // === BLYNK ===
    if (!isnan(temperatura)) Blynk.virtualWrite(V0, temperatura);
    if (!isnan(umidade)) Blynk.virtualWrite(V1, umidade);
    Blynk.virtualWrite(V2, velocidade_kmh);

    tempoAnterior = tempoAtual;
  }
}
