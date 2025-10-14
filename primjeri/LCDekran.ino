/******************************************************************************
 *
 * PROJEKT: DEMONSTRACIJA RADA LCD EKRANA
 *
 * @file        LCD_Demo_Mega.ino
 * @brief       Edukativni kod za Arduino Mega pločicu.
 *
 * @description
 * Ovaj program služi isključivo za demonstraciju rada 16x2 LCD ekrana
 * spojenog preko I2C sučelja. Program će ispisati pozdravnu poruku
 * u prvom redu, te brojač koji se povećava svake sekunde u drugom redu.
 *
 * SVE UPUTE ZA RAD NALAZE SE U OVOM KODU KAO KOMENTARI.
 *
 ******************************************************************************
 *
 * UPUTE ZA UČENIKE - KAKO POKRENUTI PROJEKT (Četiri koraka)
 *
 ******************************************************************************
 *
 * KORAK 1: POTREBNE KOMPONENTE
 * 1. Arduino Mega 2560 pločica
 * 2. 16x2 LCD ekran s I2C modulom na poleđini
 * 3. Spojne žice (4 komada)
 *
 ******************************************************************************
 *
 * KORAK 2: SPAJANJE ŽICA
 *
 * Spajanje I2C LCD ekrana na Arduino Mega pločicu je jednostavno.
 *
 * VAŽNO: Na Arduino Mega ploči, I2C pinovi su pin 20 (SDA) i 21 (SCL).
 * Ovo je drugačije nego na Arduino Uno/Nano pločicama!
 *
 * Shema spajanja:
 * +----------------+--------------------+
 * | PIN NA LCD-u   | ARDUINO MEGA PIN   |
 * +----------------+--------------------+
 * | VCC (ili 5V)   | 5V                 |
 * | GND            | GND                |
 * | SDA            | 20                 |
 * | SCL            | 21                 |
 * +----------------+--------------------+
 *
 ******************************************************************************
 *
 * KORAK 3: INSTALACIJA BIBLIOTEKE
 *
 * Da bi Arduino znao "razgovarati" s ekranom, treba nam biblioteka.
 *
 * 1. Otvorite Arduino IDE.
 * 2. Idite na: Sketch > Include Library > Manage Libraries...
 * 3. U tražilicu upišite "LiquidCrystal I2C".
 * 4. Pronađite biblioteku od autora "Frank de Brabander" i instalirajte je.
 *
 ******************************************************************************
 *
 * KORAK 4: PRONALAZAK I UČITAVANJE KODA
 *
 * 1. Prije učitavanja ovog koda, morate saznati I2C adresu vašeg ekrana.
 * Koristite "I2C Scanner" kod da je pronađete (najčešće je 0x27 ili 0x3F).
 * 2. U redu ispod, pronađite liniju "LiquidCrystal_I2C lcd(0x27, 16, 2);"
 * 3. ZAMIJENITE 0x27 s adresom koju ste saznali.
 * 4. Učitajte ovaj kod na vašu Arduino Mega pločicu.
 *
 ******************************************************************************/


// ------- POČETAK KODA -------

// UKLJUČIVANJE POTREBNIH BIBLIOTEKA
#include <Wire.h>                      // Uključujemo biblioteku za I2C komunikaciju.
#include <LiquidCrystal_I2C.h>         // Uključujemo biblioteku za upravljanje LCD-om preko I2C.

// STVARANJE OBJEKTA ZA LCD EKRAN
// Ovdje "stvaramo" naš ekran u kodu i dajemo mu važne informacije:
// 1. parametar: I2C adresa ekrana (saznajemo je pomoću I2C skenera).
// 2. parametar: Broj stupaca (karaktera) u jednom redu (za ovaj ekran to je 16).
// 3. parametar: Broj redaka (za ovaj ekran to je 2).
//
// !!! VAŽNO: Ovdje promijenite 0x27 u adresu vašeg LCD ekrana !!!
//
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Varijabla koju ćemo koristiti kao brojač.
// Deklariramo je izvan funkcija kako bi bila dostupna svugdje u kodu.
int brojac = 0;

// SETUP FUNKCIJA - Ovaj dio koda se izvršava samo jednom, na početku.
void setup() {
    // INICIJALIZACIJA EKRANA
    // Ovom naredbom pokrećemo komunikaciju s ekranom.
    lcd.init();

    // UKLJUČIVANJE POZADINSKOG OSVJETLJENJA
    // Bez ove naredbe, ekran bi radio, ali ne bismo ništa vidjeli na njemu.
    lcd.backlight();

    // ISPISIVANJE POČETNE PORUKE
    // Prije ispisa, moramo reći gdje na ekranu želimo pisati.
    // Koristimo naredbu lcd.setCursor(stupac, redak).
    // VAŽNO: Računalo broji od 0!
    // Prvi red je redak 0, drugi red je redak 1.
    // Prvi stupac je stupac 0, drugi stupac je stupac 1, itd.

    // Postavljamo kursor na početak prvog reda (stupac 0, redak 0).
    lcd.setCursor(0, 0);

    // Ispisujemo tekst na trenutnoj poziciji kursora.
    lcd.print("LCD Ekran radi!");
}


// LOOP FUNKCIJA - Ovaj dio koda se ponavlja u krug, beskonačno.
void loop() {
    // Postavljamo kursor na početak drugog reda (stupac 0, redak 1).
    lcd.setCursor(0, 1);

    // Ispisujemo tekst "Brojac: " i odmah nakon toga vrijednost naše varijable 'brojac'.
    lcd.print("Brojac: ");
    lcd.print(brojac);

    // Dodajemo mali trik: ispisujemo nekoliko razmaka nakon broja.
    // Ovo je korisno da "obrišemo" stare znamenke ako se broj smanji.
    // Npr. kada brojač pređe s 10 na 9, bez ovoga bi ostao "90" na ekranu.
    lcd.print("   ");

    // Povećavamo vrijednost brojača za 1 za sljedeći krug petlje.
    brojac++;

    // PAUZA
    // Čekamo 1000 milisekundi (1 sekundu) prije nego što `loop` funkcija krene ispočetka.
    delay(1000);
}
