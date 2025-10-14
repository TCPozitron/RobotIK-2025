/**
 **************************************************
 *
 * @file        ColorSensor_Edukativni_9600.ino
 * @brief       Edukativni primjer za čitanje boja pomoću senzora APDS-9960.
 * Program očitava RGB vrijednosti, prepoznaje je li boja
 * crvena, zelena, plava ili žuta, te ispisuje rezultat
 * na Serial Monitor brzinom od 9600 bps.
 *
 * Ovaj kod je prilagođen za učenje i rad s učenicima.
 ***************************************************/

// UKLJUČIVANJE BIBLIOTEKE (eng. Library)
// Biblioteka je kao kutija s alatom. Sadrži već gotov kod koji nam olakšava
// komunikaciju sa senzorom, tako da ne moramo pisati sve od nule.
// "APDS9960-SOLDERED.h" je specifična biblioteka za ovaj senzor.
#include "APDS9960-SOLDERED.h"

// STVARANJE OBJEKTA
// Stvaramo "objekt" iz naše biblioteke. Možemo ga zamisliti kao da smo
// iz kutije s alatom (biblioteke) uzeli jedan specifičan alat (senzor)
// i dali mu ime "APDS". Preko tog imena ćemo mu davati naredbe.
APDS_9960 APDS;


// SETUP FUNKCIJA - POKREĆE SE SAMO JEDNOM
// Sve što se nalazi unutar setup() funkcije izvršit će se samo jednom,
// i to na samom početku, kada se Arduino upali ili resetira.
// Ovdje pripremamo sve što nam je potrebno za rad.
void setup()
{
    // POKRETANJE SERIJSKE KOMUNIKACIJE
    // Ovom linijom otvaramo komunikacijski kanal između Arduino pločice i računala.
    // To nam omogućuje da vidimo poruke i vrijednosti koje nam Arduino šalje.
    // Broj 9600 je brzina komunikacije (baud rate). Mora biti ista i u Arduino IDE.
    Serial.begin(9600); // <-- OVDJE JE PROMJENA

    // ČEKANJE DA SE SERIAL MONITOR OTVORI
    // Ova linija osigurava da Arduino neće nastaviti s programom
    // sve dok ne otvorimo Serial Monitor na računalu.
    while (!Serial);

    // POKRETANJE SENZORA
    // Naređujemo našem senzoru (koji smo nazvali APDS) da se pokrene.
    // Ako senzor odgovori uspješno, APDS.begin() će biti 'true' (istina).
    // Znak '!' znači 'NE', pa "if (!APDS.begin())" čitamo kao:
    // "Ako se senzor NIJE uspješno pokrenuo..."
    if (!APDS.begin())
    {
      // Ako je došlo do greške, ispisujemo poruku na Serial Monitor.
      Serial.println("Greska pri inicijalizaciji APDS-9960 senzora.");
      
      // Zaustavljamo program u beskonačnoj petlji kako bismo signalizirali
      // da nešto nije u redu sa senzorom (npr. nije dobro spojen).
      while(1);
    }
}


// LOOP FUNKCIJA - POKREĆE SE NEPREKIDNO
// Kod unutar loop() funkcije se ponavlja u krug, beskonačno,
// sve dok je Arduino upaljen. Ovdje se odvija glavna logika našeg programa.
void loop()
{
    // ČEKANJE DA SENZOR OČITA BOJU
    // Senzoru treba malo vremena da očita boju. Ova `while` petlja čeka
    // sve dok senzor ne javi da je očitanje spremno (`colorAvailable`).
    while (!APDS.colorAvailable())
    {
        delay(5); // Mala pauza od 5 milisekundi da ne opterećujemo procesor.
    }

    // STVARANJE VARIJABLI ZA POHRANU BOJA
    // Stvaramo tri cjelobrojne varijable (int) koje će čuvati
    // vrijednosti za crvenu (r), zelenu (g) i plavu (b) boju.
    int r, g, b;

    // ČITANJE VRIJEDNOSTI SA SENZORA
    // Naređujemo senzoru da nam da očitane vrijednosti i da ih
    // spremi u naše varijable r, g i b.
    APDS.readColor(r, g, b);

    // POZIVANJE NAŠE FUNKCIJE ZA PREPOZNAVANJE BOJE
    // Ovdje pozivamo funkciju `getColorName` koju smo sami napisali ispod.
    // Dajemo joj vrijednosti r, g i b, a ona će nam vratiti tekst (String)
    // s nazivom boje. Taj naziv spremamo u varijablu `colorName`.
    String colorName = getColorName(r, g, b);

    // ISPISIVANJE REZULTATA NA SERIAL MONITOR
    // Ove linije služe za lijepo formatiran ispis koji možemo pratiti.
    
    // Prvo ispisujemo sirove RGB vrijednosti. Ovo je jako korisno
    // dok testiramo i podešavamo program (kalibracija).
    Serial.print("R: ");
    Serial.print(r);
    Serial.print(" G: ");
    Serial.print(g);
    Serial.print(" B: ");
    Serial.print(b);

    // Zatim ispisujemo naziv boje koji smo prepoznali.
    Serial.print(" -> Boja: ");
    Serial.println(colorName); // println dodaje prijelaz u novi red na kraju.
    Serial.println(); // Ispisujemo prazan red radi bolje čitljivosti.

    // PAUZA
    // Pauziramo program na 1000 milisekundi (1 sekunda) prije nego što
    // loop krene ispočetka. Time sprječavamo da nam se Serial Monitor
    // prebrzo puni podacima.
    delay(1000);
}


/**
 * @brief       Analizira RGB vrijednosti i vraća naziv boje.
 * @details     Ovo je naša vlastita, pomoćna funkcija. Ona prima tri cijela
 * broja (r, g, b) i na temelju logičkih provjera vraća
 * tekst (String) koji predstavlja naziv boje.
 * @param[in]   r  - Intenzitet crvene boje (obično 0-255).
 * @param[in]   g  - Intenzitet zelene boje (obično 0-255).
 * @param[in]   b  - Intenzitet plave boje (obično 0-255).
 * @retval      String - Naziv prepoznate boje ("Crvena", "Zelena", "Plava", "Žuta" ili "Nepoznata").
 */
String getColorName(int r, int g, int b) {
  
  // PRAGOVI ZA KALIBRACIJU - OVE VRIJEDNOSTI MOŽETE MIJENJATI!
  // Svaki senzor i svako osvjetljenje su drugačiji. Možda ćete trebati
  // malo prilagoditi ove brojeve kako bi prepoznavanje bilo točnije.
  
  // Minimalna svjetlina: Ako su sve komponente ispod ove vrijednosti,
  // smatramo da je objekt pretaman da bismo pouzdano odredili boju.
  int minBrightness = 40;

  // 1. PROVJERA ZA ŽUTU BOJU
  // Žuta boja u RGB modelu je mješavina crvene i zelene, bez plave.
  // Uvjeti: "Ako je crvena JAKA, i zelena JAKA, a plava SLABA,
  // i ako su crvena i zelena međusobno slične..."
  if (r > 80 && g > 80 && b < 60 && abs(r - g) < 50) {
    return "Žuta";
  }

  // 2. PROVJERA ZA CRVENU BOJU
  // Uvjeti: "Ako je crvena komponenta jača od zelene I jača od plave,
  // I ako je dovoljno svijetla..."
  if (r > g && r > b && r > minBrightness) {
    return "Crvena";
  }

  // 3. PROVJERA ZA ZELENU BOJU
  // Uvjeti: "Ako je zelena komponenta jača od crvene I jača od plave,
  // I ako je dovoljno svijetla..."
  if (g > r && g > b && g > minBrightness) {
    return "Zelena";
  }

  // 4. PROVJERA ZA PLAVU BOJU
  // Uvjeti: "Ako je plava komponenta jača od crvene I jača od zelene,
  // I ako je dovoljno svijetla..."
  if (b > r && b > g && b > minBrightness) {
    return "Plava";
  }

  // AKO NIJEDAN UVJET NIJE ZADOVOLJEN
  // Ako boja nije prepoznata kao jedna od gore navedenih,
  // funkcija će vratiti tekst "Nepoznata".
  return "Nepoznata";
}
