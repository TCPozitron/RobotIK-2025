/******************************************************************************
 *
 * PROJEKT: DETEKTOR BOJA S LCD EKRANOM
 *
 * @file        DetektorBoja_LCD_Mega.ino
 * @brief       Edukativni kod za Arduino Mega pločicu.
 *
 * @description
 * Ovaj program koristi APDS-9960 senzor za prepoznavanje boja (crvena,
 * zelena, plava, žuta) i ispisuje rezultat na 16x2 LCD ekran spojen
 * preko I2C sučelja.
 *
 * SVE UPUTE ZA RAD NALAZE SE U OVOM KODU KAO KOMENTARI.
 *
 ******************************************************************************
 *
 * UPUTE ZA UČENIKE - KAKO POKRENUTI PROJEKT (Četiri koraka)
 *
 ******************************************************************************
 *
 * POTREBNE KOMPONENTE:
 * 1. Arduino Mega 2560 pločica
 * 2. APDS-9960 senzor boje (na pločici/modulu)
 * 3. 16x2 LCD ekran s I2C modulom na poleđini
 * 4. Spojne žice (Jumper wires)
 *
 ******************************************************************************
 *
 * KORAK 1: SPAJANJE ŽICA
 *
 * Oba modula (senzor i LCD) koriste I2C komunikaciju. To znači da se
 * spajaju na iste pinove na Arduino Mega ploči.
 *
 * VAŽNO: Na Arduino Mega ploči, I2C pinovi su pin 20 (SDA) i 21 (SCL).
 * Zauzeti analogni pinovi (A0-A7) nam ne smetaju.
 *
 * Shema spajanja:
 *
 * +-----------------+---------------------+----------------------+
 * | PIN NA MODULIMA | SPOJITI NA          | SPOJITI NA           |
 * | (LCD i Senzor)  | ARDUINO MEGA PIN... | ... ILI NA ISTU LINIJU |
 * +-----------------+---------------------+----------------------+
 * | VCC (ili 5V)    | 5V                  | VCC od drugog modula |
 * | GND             | GND                 | GND od drugog modula |
 * | SDA             | 20                  | SDA od drugog modula |
 * | SCL             | 21                  | SCL od drugog modula |
 * +-----------------+---------------------+----------------------+
 *
 * Jednostavnije rečeno, oba modula moraju biti spojena na 5V, GND, pin 20 i pin 21.
 *
 ******************************************************************************
 *
 * KORAK 2: INSTALACIJA BIBLIOTEKA
 *
 * Da bi Arduino znao "razgovarati" sa senzorom i ekranom, trebaju nam
 * dvije biblioteke (kao "driveri" za računalo).
 *
 * 1. Otvorite Arduino IDE.
 * 2. Idite na: Sketch > Include Library > Manage Libraries...
 * 3. U tražilicu upišite "APDS9960-SOLDERED" i instalirajte je.
 * 4. U tražilicu upišite "LiquidCrystal I2C" (od autora Frank de Brabander) i instalirajte je.
 *
 ******************************************************************************
 *
 * KORAK 3: PRONALAZAK I2C ADRESE EKRANA
 *
 * Svaki I2C modul ima svoju adresu. Morate saznati adresu VAŠEG ekrana.
 * 1. Provjerite je li sve spojeno prema KORAKU 1.
 * 2. Učitajte na Arduino poseban kod za skeniranje (možete ga lako naći
 * online pod nazivom "I2C Scanner Arduino").
 * 3. Otvorite Serial Monitor (brzina 9600) i on će ispisati adresu,
 * npr. "I2C device found at address 0x27".
 * 4. ZAPIŠITE TU ADRESU! Najčešće su 0x27 ili 0x3F.
 *
 ******************************************************************************
 *
 * KORAK 4: UČITAVANJE GLAVNOG KODA (ovog koda)
 *
 * 1. U redu ispod, pronađite liniju "LiquidCrystal_I2C lcd(0x27, 16, 2);"
 * 2. ZAMIJENITE 0x27 s adresom koju ste saznali u KORAKU 3.
 * 3. Učitajte ovaj kod na vašu Arduino Mega pločicu.
 *
 * Ako je sve ispravno, na LCD ekranu ćete vidjeti ispis očitane boje!
 *
 ******************************************************************************/


// ------- POČETAK KODA -------

// UKLJUČIVANJE POTREBNIH BIBLIOTEKA
#include <Wire.h>                      // Biblioteka za I2C komunikaciju
#include "APDS9960-SOLDERED.h"         // Biblioteka za senzor boje
#include <LiquidCrystal_I2C.h>         // Biblioteka za LCD ekran

// STVARANJE OBJEKTA ZA SENZOR
// Dajemo ime našem senzoru kako bismo mu mogli davati naredbe.
APDS_9960 APDS;

// STVARANJE OBJEKTA ZA LCD EKRAN
// Unesite adresu, broj stupaca i broj redaka vašeg ekrana.
//
// !!! VAŽNO: Ovdje promijenite 0x27 u adresu vašeg LCD ekrana !!!
//
LiquidCrystal_I2C lcd(0x27, 16, 2);

// SETUP FUNKCIJA - POKREĆE SE SAMO JEDNOM NA POČETKU
void setup() {
    // Pokrećemo serijsku komunikaciju (korisno za testiranje ako nešto ne radi)
    Serial.begin(9600);
    while (!Serial);

    // Inicijalizacija senzora boje
    if (!APDS.begin()) {
        Serial.println("Greska pri inicijalizaciji APDS-9960 senzora.");
        // Ako senzor ne radi, ispisujemo poruku i na LCD
        lcd.init();
        lcd.backlight();
        lcd.setCursor(0, 0);
        lcd.print("Greska senzora!");
        while (1); // Zaustavljamo program
    }

    // Inicijalizacija LCD ekrana
    lcd.init();
    lcd.backlight(); // Uključujemo pozadinsko osvjetljenje

    // Ispisujemo početnu poruku na ekran
    lcd.setCursor(0, 0); // Postavi kursor na prvi red, prvi stupac
    lcd.print("Detektor Boja");
    lcd.setCursor(0, 1); // Postavi kursor na drugi red, prvi stupac
    lcd.print("Spreman...");

    // Čekamo 2 sekunde da se poruka može pročitati
    delay(2000);
}

// LOOP FUNKCIJA - PONAVLJA SE U KRUG BESKONAČNO
void loop() {
    // Čekamo dok senzor ne očita dostupnu boju
    while (!APDS.colorAvailable()) {
        delay(5);
    }

    // Varijable za spremanje RGB vrijednosti
    int r, g, b;
    // Očitaj boju i spremi vrijednosti
    APDS.readColor(r, g, b);

    // Pozovi našu pomoćnu funkciju da prepozna naziv boje
    String colorName = getColorName(r, g, b);

    // Očisti ekran prije novog ispisa da ne ostanu stari znakovi
    lcd.clear();

    // ISPIS REZULTATA NA LCD EKRAN

    // 1. red: Ispisujemo sirove RGB vrijednosti (korisno za kalibraciju)
    lcd.setCursor(0, 0);
    lcd.print("R:");
    lcd.print(r);
    lcd.print(" G:");
    lcd.print(g);
    lcd.print(" B:");
    lcd.print(b);

    // 2. red: Ispisujemo prepoznati naziv boje
    lcd.setCursor(0, 1);
    lcd.print("Boja: ");
    lcd.print(colorName);

    // Pauza od jedne sekunde prije sljedećeg očitanja
    delay(1000);
}

/**
 * @brief       Analizira RGB vrijednosti i vraća naziv boje.
 * @details     Ova funkcija prima RGB vrijednosti i na temelju
 * jednostavnih pravila određuje o kojoj se boji radi.
 * @param[in]   r  - Intenzitet crvene boje.
 * @param[in]   g  - Intenzitet zelene boje.
 * @param[in]   b  - Intenzitet plave boje.
 * @retval      String - Naziv boje ("Crvena", "Zelena", "Plava", "Žuta" ili "Nepoznata").
 */
String getColorName(int r, int g, int b) {
    // Pragovi za kalibraciju - ove vrijednosti se mogu mijenjati
    // za bolje prepoznavanje u drugačijim uvjetima osvjetljenja.
    int minBrightness = 40;

    // 1. PROVJERA ZA ŽUTU BOJU
    // Žuta je mješavina crvene i zelene, s malo plave.
    if (r > 80 && g > 80 && b < 60 && abs(r - g) < 50) {
        return "Zuta";
    }

    // 2. PROVJERA ZA CRVENU BOJU
    // Crvena je dominantna nad zelenom i plavom.
    if (r > g && r > b && r > minBrightness) {
        return "Crvena";
    }

    // 3. PROVJERA ZA ZELENU BOJU
    // Zelena je dominantna nad crvenom i plavom.
    if (g > r && g > b && g > minBrightness) {
        return "Zelena";
    }

    // 4. PROVJERA ZA PLAVU BOJU
    // Plava je dominantna nad crvenom i zelenom.
    if (b > r && b > g && b > minBrightness) {
        return "Plava";
    }

    // Ako nijedan uvjet nije zadovoljen
    return "Nepoznata";
}
