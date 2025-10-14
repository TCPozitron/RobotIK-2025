/******************************************************************************
 *
 * PROJEKT: 06 - Upravljanje robotom pomoću Android aplikacije (Bluetooth)
 * PLATFORMA: Dasduino Connect plus (ESP32 Wrover)
 *
 * @file        Robot_Bluetooth_ESP32.ino
 * @brief       Edukativni kod za upravljanje robotom s dva motora
 * putem Bluetooth veze s pametnog telefona.
 * @author      lovronix (prilagodio i komentirao Coding Partner)
 * @date        20.11.2022. (ažurirano 14.10.2025.)
 *
 * @description
 * Ovaj program pretvara Dasduino Connect plus u Bluetooth prijemnik.
 * Kada se na njega spoji pametni telefon preko odgovarajuće aplikacije,
 * robotom se može upravljati slanjem pojedinačnih znakova (naredbi).
 *
 ******************************************************************************
 *
 * UPUTE ZA UČENIKE
 *
 ******************************************************************************
 *
 * KORAK 1: POTREBNE KOMPONENTE
 * 1. Dasduino Connect plus (ili bilo koja druga ESP32 pločica)
 * 2. Robotska šasija s dva DC motora
 * 3. Driver za motore (npr. L298N, DRV8833)
 * 4. Baterije za napajanje motora
 * 5. Pametni telefon s instaliranom Bluetooth terminal aplikacijom
 * (npr. "Serial Bluetooth Terminal" s Google Play Storea)
 *
 ******************************************************************************
 *
 * KORAK 2: SPAJANJE ŽICA
 *
 * Pinovi na Dasduinu moraju se spojiti na ulazne pinove drivera za motore.
 * Oznake IN1, IN2, ENA su tipične za L298N driver.
 *
 * Dasduino PIN -> L298N Driver PIN
 * --------------------------------
 * 32            -> IN1 (Lijevi motor, smjer 1)
 * 33            -> IN2 (Lijevi motor, smjer 2)
 * 25            -> IN3 (Desni motor, smjer 1)
 * 26            -> IN4 (Desni motor, smjer 2)
 * 14            -> ENA (Lijevi motor, kontrola brzine PWM)
 * 13            -> ENB (Desni motor, kontrola brzine PWM)
 *
 * VAŽNO: Ne zaboravite spojiti GND (masu) s Dasduina na GND na driveru
 * motora kako bi signali bili ispravno prepoznati! Motori se napajaju
 * iz zasebnog izvora (baterija), a ne s Dasduina.
 *
 ******************************************************************************
 *
 * KORAK 3: KORIŠTENJE
 *
 * 1. Učitajte ovaj kod na vaš Dasduino Connect plus.
 * 2. Na pametnom telefonu, otvorite Bluetooth postavke i potražite novi
 * uređaj. Trebao bi se pojaviti pod nazivom "Juraj Dobrila I".
 * 3. Uparite telefon s Dasduinom.
 * 4. Otvorite aplikaciju "Serial Bluetooth Terminal".
 * 5. U aplikaciji se spojite na "Juraj Dobrila I".
 * 6. Sada možete slati naredbe (pojedinačne znakove) za upravljanje robotom.
 *
 ******************************************************************************/


// ------- POČETAK KODA -------

// UKLJUČIVANJE BIBLIOTEKE ZA BLUETOOTH
#include "BluetoothSerial.h"

// PROVJERA JE LI BLUETOOTH OMOGUĆEN U POSTAVKAMA PROJEKTA
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// STVARANJE OBJEKTA ZA BLUETOOTH KOMUNIKACIJU
BluetoothSerial SerialBT;

// VARIJABLA ZA SPREMANJE NAREDBI
char naredba = '0';

// DEFINIRANJE PINOVA ZA UPRAVLJANJE MOTORIMA
int motorLnatrag = 32;   // Spojeno na IN1 na L298N driveru
int motorLnaprijed = 33;   // Spojeno na IN2 na L298N driveru
int motorDnatrag = 25;   // Spojeno na IN3 na L298N driveru
int motorDnaprijed = 26;   // Spojeno na IN4 na L298N driveru
int brzinaL = 14;        // Spojeno na ENA (Enable A) za PWM kontrolu brzine lijevog motora
int brzinaD = 13;        // Spojeno na ENB (Enable B) za PWM kontrolu brzine desnog motora

// VARIJABLA ZA BRZINU MOTORA
int brzina = 200;

// SETUP FUNKCIJA - Izvršava se samo jednom, na početku.
void setup() {
  // Kratka pauza na početku, dobra praksa za stabilizaciju sustava.
  delay(2000);

  // POSTAVLJANJE PINOVA KAO IZLAZNIH (OUTPUT)
  pinMode(motorLnatrag, OUTPUT);
  pinMode(motorLnaprijed, OUTPUT);
  pinMode(motorDnatrag, OUTPUT);
  pinMode(motorDnaprijed, OUTPUT);
  pinMode(brzinaL, OUTPUT);
  pinMode(brzinaD, OUTPUT);

  // Pokretanje serijske komunikacije preko USB-a za ispis poruka na računalo (debugging).
  Serial.begin(115200);
  Serial.println("-------------------------------------");
  Serial.println("             START ROBOT             ");
  Serial.println("-------------------------------------");

  // POKRETANJE BLUETOOTH-a
  // `SerialBT.begin("Naziv")` pokreće Bluetooth modul i postavlja naziv
  // koji će biti vidljiv drugim uređajima prilikom skeniranja.
  SerialBT.begin("Juraj Dobrila I"); // <-- OVDJE JE PROMJENA
  Serial.println("-------------------------------------");
  Serial.println(" Bluetooth: Juraj Dobrila I ");
  Serial.println("-------------------------------------");
}

// LOOP FUNKCIJA - Ponavlja se u krug beskonačno.
void loop() {
  // PROVJERA JESU LI PODACI STIGLI PREKO BLUETOOTHA
  if (SerialBT.available()) {
    // Ako je podatak stigao, čitamo ga.
    naredba = (char)SerialBT.read();

    // Ispisujemo primljenu naredbu na Serial Monitor (preko USB-a)
    Serial.print ("Naredba: ");
    Serial.println(naredba);

    // SWITCH NAREDBA - Odabir akcije na temelju primljenog znaka
    switch (naredba) {
      // Naredbe za kretanje
      case 'F': naprijed(); break;
      case 'B': natrag(); break;
      case 'L': lijevoFull(); break;
      case 'R': desnoFull(); break;
      case 'S': stani(); break;
      case 'G': lijevoNaprijed(); break;
      case 'I': desnoNaprijed(); break;
      case 'H': lijevoNatrag(); break;
      case 'J': desnoNatrag(); break;

      // Naredbe za postavljanje brzine
      case '0': brzina = 0; break;
      case '1': brzina = 25; break;
      case '2': brzina = 50; break;
      case '3': brzina = 75; break;
      case '4': brzina = 100; break;
      case '5': brzina = 125; break;
      case '6': brzina = 150; break;
      case '7': brzina = 175; break;
      case '8': brzina = 200; break;
      case '9': brzina = 225; break;
      case 'q': brzina = 255; break;
    }
  }
}

// --- POMOĆNE FUNKCIJE ZA UPRAVLJANJE MOTORIMA ---

void naprijed() {
  Serial.println("Naprijed");
  analogWrite(brzinaD, brzina);
  analogWrite(brzinaL, brzina);
  digitalWrite(motorDnaprijed, HIGH);
  digitalWrite(motorDnatrag, LOW);
  digitalWrite(motorLnaprijed, HIGH);
  digitalWrite(motorLnatrag, LOW);
}

void natrag() {
  Serial.println("Rikverc");
  analogWrite(brzinaD, brzina);
  analogWrite(brzinaL, brzina);
  digitalWrite(motorDnaprijed, LOW);
  digitalWrite(motorDnatrag, HIGH);
  digitalWrite(motorLnaprijed, LOW);
  digitalWrite(motorLnatrag, HIGH);
}

void stani() {
  Serial.println("Stop");
  digitalWrite(motorDnaprijed, LOW);
  digitalWrite(motorDnatrag, LOW);
  digitalWrite(motorLnaprijed, LOW);
  digitalWrite(motorLnatrag, LOW);
}

void lijevoFull() {
  Serial.println("Lijevo (okret u mjestu)");
  analogWrite(brzinaD, brzina);
  analogWrite(brzinaL, brzina);
  digitalWrite(motorDnaprijed, HIGH);
  digitalWrite(motorDnatrag, LOW);
  digitalWrite(motorLnaprijed, LOW);
  digitalWrite(motorLnatrag, HIGH);
}

void desnoFull() {
  Serial.println("Desno (okret u mjestu)");
  analogWrite(brzinaD, brzina);
  analogWrite(brzinaL, brzina);
  digitalWrite(motorDnaprijed, LOW);
  digitalWrite(motorDnatrag, HIGH);
  digitalWrite(motorLnaprijed, HIGH);
  digitalWrite(motorLnatrag, LOW);
}

void lijevoNaprijed() {
  Serial.println("Lijevo naprijed (blagi okret)");
  analogWrite(brzinaD, brzina);
  analogWrite(brzinaL, brzina/2);
  digitalWrite(motorDnaprijed, HIGH);
  digitalWrite(motorDnatrag, LOW);
  digitalWrite(motorLnaprijed, HIGH);
  digitalWrite(motorLnatrag, LOW);
}

void desnoNaprijed() {
  Serial.println("Desno naprijed (blagi okret)");
  analogWrite(brzinaD, brzina/2);
  analogWrite(brzinaL, brzina);
  digitalWrite(motorDnaprijed, HIGH);
  digitalWrite(motorDnatrag, LOW);
  digitalWrite(motorLnaprijed, HIGH);
  digitalWrite(motorLnatrag, LOW);
}

void lijevoNatrag() {
  Serial.println("Lijevo natrag (blagi okret)");
  analogWrite(brzinaD, brzina);
  analogWrite(brzinaL, brzina / 2);
  digitalWrite(motorDnaprijed, LOW);
  digitalWrite(motorDnatrag, HIGH);
  digitalWrite(motorLnaprijed, LOW);
  digitalWrite(motorLnatrag, HIGH);
}

void desnoNatrag() {
  Serial.println("Desno natrag (blagi okret)");
  analogWrite(brzinaD, brzina / 2);
  analogWrite(brzinaL, brzina);
  digitalWrite(motorDnaprijed, LOW);
  digitalWrite(motorDnatrag, HIGH);
  digitalWrite(motorLnaprijed, LOW);
  digitalWrite(motorLnatrag, HIGH);
}
