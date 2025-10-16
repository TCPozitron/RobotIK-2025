/********************************************************************************
 * @file    I2C_Scanner.ino
 * @author  AI Assistant (Gemini)
 * @version 1.0
 *
 * @brief   Jednostavan dijagnostički program za skeniranje I2C sabirnice.
 * Program prolazi kroz sve moguće I2C adrese (od 1 do 127) i
 * ispisuje adrese svih uređaja koji odgovore. Ovo je izuzetno
 * korisno za provjeru jesu li senzori (poput IMU ili senzora za boje)
 * ispravno spojeni i koju adresu koriste.
 *
 * @upute
 * 1. Spojite vaše I2C uređaje na SDA (pin 20) i SCL (pin 21) na Arduino Mega.
 * 2. Učitajte ovaj kod.
 * 3. Otvorite Serial Monitor na brzini 9600.
 * 4. Program će ispisati adrese svih pronađenih uređaja.
 ********************************************************************************/

// Uključivanje Wire biblioteke za I2C komunikaciju
#include <Wire.h>

void setup() {
  // Pokreni serijsku komunikaciju za ispis rezultata
  Serial.begin(9600);
  
  // Pokreni I2C sabirnicu kao "master"
  Wire.begin(); 
  
  Serial.println("\nI2C Skener Pokrenut...");
  Serial.println("Skeniram adrese od 1 do 127...");
}

void loop() {
  byte greska, adresa;
  int brojPronadjenihUredjaja = 0;

  // Petlja koja prolazi kroz sve moguće adrese.
  // I2C adrese su 7-bitne, što znači da mogu biti od 1 do 127.
  // Adresa 0 je rezervirana.
  for (adresa = 1; adresa < 127; adresa++) {
    // Wire.beginTransmission() pokušava uspostaviti komunikaciju s uređajem
    // na zadanoj adresi.
    Wire.beginTransmission(adresa);
    
    // Wire.endTransmission() završava komunikaciju i vraća status:
    // 0: Uspjeh (uređaj je pronađen i odgovorio je)
    // 1: Greška - podaci predugi za slanje
    // 2: Greška - primljen NACK (uređaj nije odgovorio)
    // 3: Greška - primljen NACK (uređaj nije odgovorio)
    // 4: Druga greška
    greska = Wire.endTransmission();

    // Ako je status 0, znači da smo pronašli uređaj!
    if (greska == 0) {
      Serial.print("I2C uređaj pronađen na adresi 0x");
      // Ispisujemo adresu u heksadecimalnom formatu, što je standard.
      if (adresa < 16) {
        Serial.print("0"); // Dodaj vodeću nulu za ljepši ispis (npr. 0x0F)
      }
      Serial.println(adresa, HEX);

      brojPronadjenihUredjaja++;
    }
    // Ako uređaj ne odgovori, jednostavno nastavljamo na iduću adresu.
  }

  // Nakon što smo prošli sve adrese, ispišimo sažetak.
  if (brojPronadjenihUredjaja == 0) {
    Serial.println("\nNijedan I2C uređaj nije pronađen. Provjerite spojeve!");
  } else {
    Serial.print("\nSkeniranje završeno. Pronađeno je ukupno ");
    Serial.print(brojPronadjenihUredjaja);
    Serial.println(" uređaja.");
  }

  // Pauziraj 5 sekundi prije ponovnog skeniranja.
  // Nema potrebe da se ovo vrti prebrzo.
  delay(5000); 
}