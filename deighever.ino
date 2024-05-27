#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

// Definerer pinner
#define TRIG_PIN 6 // Ultrasonisk sensor
#define ECHO_PIN 7 // Ultrasonisk sensor
#define DHTPIN 5 // Temperatur- og fuktighetssensor
#define DHTTYPE DHT11
#define VARMEPUTE_PIN 10 // Varmepute (kontrollert via transistor)
#define FUKTIGHETSDYSE_PIN 11 // Fuktighetsdyse (kontrollert via transistor)
#define POT_PIN A0 // Potensiometer
#define LED_PIN 3 // LED-stripe
#define ANTALL_LED 10 // Antall LED-lys i stripen

// Initialiserer komponenter
Adafruit_NeoPixel stripe = Adafruit_NeoPixel(ANTALL_LED, LED_PIN, NEO_GRB + NEO_KHZ800);
DHT dht(DHTPIN, DHTTYPE);

long varighet;
int avstand;
int startAvstand = 25; // Startavstand i cm
int potVerdi;
unsigned long startTid;
unsigned long tidsgrense;
unsigned long forrigeMillis = 0;
unsigned long utskriftIntervall = 10000; // 10 sekunder

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(VARMEPUTE_PIN, OUTPUT);
  pinMode(FUKTIGHETSDYSE_PIN, OUTPUT);
  dht.begin();
  stripe.begin();
  stripe.show();
  startTid = millis();
  Serial.println("Oppsett fullfort. Starter lopen.");
}

void loop() {
  unsigned long naavaerendeMillis = millis();

  // Leser temperatur og fuktighet
  float temperatur = dht.readTemperature();
  float fuktighet = dht.readHumidity();

  // Sjekker om lesingene feilet og hopper over resten av koden i denne lopen hvis de gjorde
  if (isnan(temperatur) || isnan(fuktighet)) {
    Serial.println("Feil ved lesing av DHT-sensor. Prover paa nytt...");
    return; 
  }

  // Leser avstand fra ultrasonisk sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  varighet = pulseIn(ECHO_PIN, HIGH);
  avstand = varighet * 0.034 / 2;

  // Leser potensiometerverdi og setter tidsgrense
  potVerdi = analogRead(POT_PIN);
  if (potVerdi >= 1023) {
    tidsgrense = 0; // Setter timeren til null
  } else {
    // Mapper potensiometerverdi til en av de fire onskede tidene
    if (potVerdi < 256) {
      tidsgrense = 7200000; // 2 timer (120 minutter)
    } else if (potVerdi < 512) {
      tidsgrense = 5400000; // 1,5 timer (90 minutter)
    } else if (potVerdi < 768) {
      tidsgrense = 3600000; // 1 time (60 minutter)
    } else {
      tidsgrense = 1800000; // 0,5 time (30 minutter)
    }
  }

  // Sjekker om timeren maa tilbakestilles
  if (millis() - startTid >= tidsgrense) {
    startTid = millis();
  }

  // Beregner gjenstaaende tid
  unsigned long gjenstaaendeTid = tidsgrense - (millis() - startTid);

  // LED-fargedisplay for avstand (trafikklysstil)
  int avstandSteg = (startAvstand - 5) / (ANTALL_LED - 1); // Beregner avstandstrinn for hver LED
  for (int i = 0; i < ANTALL_LED; i++) {
    if (avstand < 5) {
      // Tenner alle LED-lys gronn hvis avstanden er mindre enn 5 cm
      stripe.setPixelColor(i, stripe.Color(0, 255, 0));
    } else if (i == 0) {
      // Det forste LED-lyset er alltid rodt
      stripe.setPixelColor(i, stripe.Color(255, 0, 0));
    } else if (avstand < (startAvstand - i * avstandSteg)) {
      if (i < 4) {
        // Rodt lys
        stripe.setPixelColor(i, stripe.Color(255, 0, 0));
      } else if (i < 7) {
        // Gult lys
        stripe.setPixelColor(i, stripe.Color(255, 255, 0));
      } else {
        // Gront lys
        stripe.setPixelColor(i, stripe.Color(0, 255, 0));
      }
    } else {
      // Slukker de resterende LED-lysene
      stripe.setPixelColor(i, stripe.Color(0, 0, 0));
    }
  }
  stripe.show();

  // Sjekker betingelser og kontrollerer varmepute og fuktighetsdyse
  if (avstand < 5) {
    digitalWrite(VARMEPUTE_PIN, LOW);
    digitalWrite(FUKTIGHETSDYSE_PIN, LOW);
    Serial.println("Ferdig - Avstanden er mindre enn 5 cm.");
  } else if (temperatur < 50 && millis() - startTid < tidsgrense) {
    digitalWrite(VARMEPUTE_PIN, HIGH);
    digitalWrite(FUKTIGHETSDYSE_PIN, HIGH);
  } else {
    digitalWrite(VARMEPUTE_PIN, LOW);
    digitalWrite(FUKTIGHETSDYSE_PIN, LOW);
  }

  // Overopphetingsbeskyttelse
  if (temperatur >= 50) {
    while (temperatur >= 35) {
      temperatur = dht.readTemperature();
      delay(1000);
    }
  }

  // Skriver ut tabellen hver 10. sekund
  if (naavaerendeMillis - forrigeMillis >= utskriftIntervall) {
    forrigeMillis = naavaerendeMillis;

    Serial.println("---------------------------------------------------");
    Serial.println("| Temperatur (C) | Fuktighet (%) | Avstand (cm) | Timer igjen |");
    Serial.println("---------------------------------------------------");
    Serial.print("| ");
    Serial.print(temperatur);
    Serial.print("            | ");
    Serial.print(fuktighet);
    Serial.print("           | ");
    Serial.print(avstand);
    Serial.print("          | ");
    Serial.print(gjenstaaendeTid / 60000); // Timer igjen i minutter
    Serial.println(" min   |");
    Serial.println("---------------------------------------------------");
  }

  delay(1000); // Ekstra forsinkelse mellom avlesninger for aa sikre DHT-sensoren
}