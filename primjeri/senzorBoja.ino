// 1. Uključujemo Wire i Adafruit biblioteku
#include <Wire.h>
#include <Adafruit_APDS9960.h>

// 2. Kreiramo objekt iz Adafruit klase
Adafruit_APDS9960 apds;

// Varijable za boje (Adafruit koristi 16-bitne)
uint16_t r, g, b, c;

void setup() {
  Serial.begin(9600); 
  Serial.println("Test APDS-9960 senzora za boju (Adafruit biblioteka)");

  if (!apds.begin()) {
    Serial.println("Greška: Senzor APDS-9960 nije pronađen!");
    while (1)
      ;
  }



  // 1. Obavezno moramo omogućiti senzor boje
  apds.enableColor(true);

  // 2. Postavi pojačanje (Gain)
  // Vrijednosti: APDS9960_GAIN_1X, APDS9960_GAIN_4X,
  //             APDS9960_GAIN_16X, APDS9960_GAIN_64X
  apds.setADCGain(APDS9960_AGAIN_64X);  

  // 3. Postavi vrijeme integracije (koliko dugo 'gleda' svjetlo)
  // Vrijednost je u milisekundama (cca 2.78ms po koraku).
  // 100ms je dobra vrijednost (registar 0xD5)
  // Ako funkcija ne prima ms, probat ćemo s registrom:
  apds.setADCIntegrationTime(0xD5);  // Za cca 101ms

  Serial.println("Senzor inicijaliziran. Pojačanje (Gain) postavljeno na 64x.");
}

void loop() {

  // Čekaj dok podaci o boji ne budu dostupni
  while (!apds.colorDataReady()) {
    delay(5);
  }

  // Pročitaj podatke (Adafruit funkcija daje i 'c' - Clear/Intenzitet)
  apds.getColorData(&r, &g, &b, &c);

  // Ispiši vrijednosti
  Serial.print("Crvena (R): ");
  Serial.print(r);
  Serial.print("  |  Zelena (G): ");
  Serial.print(g);
  Serial.print("  |  Plava (B): ");
  Serial.print(b);
  Serial.print("  |  Intenzitet (C): ");
  Serial.println(c);
  Serial.println("---------------------------------");

  delay(1000);
}
