/********************************************************************************
 * @file      Zadatak 1 (Praćenje Linije - Pojednostavljeno)
 * @author    Partner u pisanju koda (prilagođeno za učenje)
 * @version   1.0
 * @date      13.10.2025.
 *
 * @brief     Pojednostavljeni upravljački softver za robota koji prati liniju.
 * Kod se temelji na "stroju stanja" (State Machine) kako je opisano
 * u uputama za učenike. Koristi P-regulator za praćenje linije i
 * precizno izvršava sve faze zadatka.
 *
 * @hardware
 * - Mikrokontroler: Arduino Mega
 * - Pogonski sustav: Robot s 4 DC motora (upravljani kao dva seta)
 * - Senzori: 5-kanalni infracrveni (IR) senzor za detekciju linije [cite: 9]
 * - Ulaz: 1x Tipkalo za nastavak zadatka [cite: 12]
 ********************************************************************************/

//==============================================================================
// UKLJUČIVANJE BIBLIOTEKA
//==============================================================================
// U ovom jednostavnom primjeru nisu potrebne dodatne biblioteke.

//==============================================================================
// GLOBALNE KONSTANTE I POSTAVKE
//==============================================================================

// --- Postavke motora ---
// Pretpostavka je da su motori spojeni tako da se lijeva i desna strana
// mogu kontrolirati zajedno. Ovdje definiramo pinove za driver motora.
// PRILAGODITE OVE PINOVE VAŠEM DRIVERU MOTORA (npr. L298N)
const int pinMotorLijeviNaprijed = 2;
const int pinMotorLijeviNazad = 3;
const int pinMotorDesniNaprijed = 4;
const int pinMotorDesniNazad = 5;

// --- Postavke senzora i tipkala ---
const int BROJ_GLAVNIH_SENZORA = 5;
const int pinoviSenzora[BROJ_GLAVNIH_SENZORA] = {28, 29, 30, 31, 32}; // Pinovi kako je navedeno u uputama [cite: 11]
const float tezine[BROJ_GLAVNIH_SENZORA] = {-6, -3, 0, 3, 6}; // Težine za P-regulator 
const int PIN_TIPKALO = 15; // Pin za tipkalo [cite: 12]

// --- Parametri za P-regulator ---
// Ove vrijednosti učenici trebaju podesiti [cite: 70]
float Kp = 12.0;
int osnovnaBrzina = 80; // Početna vrijednost [cite: 71]

// --- Parametri za specifične akcije ---
// Ove vrijednosti učenici trebaju podesiti [cite: 66, 68]
const int VRIJEME_PROVJERE_KRAJA_MS = 250;
const int BRZINA_PROVJERE_KRAJA = 70;
const int BRZINA_PORAVNANJA_NAPRIJED = 70;
const int BRZINA_PORAVNANJA_OKRET = 60;

//==============================================================================
// DEFINICIJA STANJA ROBOTA (STATE MACHINE)
//==============================================================================
enum StanjeRobota {
  PRACENJE_LINIJE,      // Robot prati crnu liniju [cite: 30]
  PROVJERA_KRAJA,       // Robot provjerava je li došao do kraja linije [cite: 33]
  CEKANJE_NA_TIPKU,     // Robot čeka pritisak tipke za nastavak [cite: 37]
  PORAVNANJE_KRETANJE,  // Robot se kreće naprijed do kose linije [cite: 40]
  PORAVNANJE_OKRETANJE, // Robot se okreće kako bi se poravnao s kosom linijom [cite: 43]
  ZADATAK_ZAVRSEN       // Robot je završio zadatak i stoji [cite: 46]
};

//==============================================================================
// GLOBALNE VARIJABLE
//==============================================================================
int ocitanjaGlavnihSenzora[BROJ_GLAVNIH_SENZORA];
StanjeRobota trenutnoStanje = PRACENJE_LINIJE; // Početno stanje robota
unsigned long vrijemePocetkaStanja = 0;      // Varijabla za praćenje vremena u stanjima
float zadnjePoznatoOdstupanje = 0;           // "Pamćenje" zadnje pozicije linije [cite: 56]

//==============================================================================
// SETUP FUNCIJA - Izvršava se jednom na početku
//==============================================================================
void setup() {
  Serial.begin(9600);
  Serial.println("Inicijalizacija sustava...");

  // Postavljanje pinova motora kao izlaza
  pinMode(pinMotorLijeviNaprijed, OUTPUT);
  pinMode(pinMotorLijeviNazad, OUTPUT);
  pinMode(pinMotorDesniNaprijed, OUTPUT);
  pinMode(pinMotorDesniNazad, OUTPUT);

  // Postavljanje pinova senzora kao ulaza [cite: 23]
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    pinMode(pinoviSenzora[i], INPUT);
  }
  
  // Postavljanje pina tipkala kao ulaza s internim pull-up otpornikom
  pinMode(PIN_TIPKALO, INPUT_PULLUP);
  
  // Kratka pauza prije početka
  delay(500);
  zaustavi(); // Za svaki slučaj, osiguraj da motori stoje
  
  Serial.println("Sustav spreman. Cekam na pritisak tipkala za start...");

  // Petlja koja čeka pritisak tipkala za početak zadatka
  while (digitalRead(PIN_TIPKALO) == HIGH) {
    // Ne radi ništa, samo čekaj
  }
  delay(500); // Pauza da se izbjegne lažni start
  Serial.println("Krecem sa zadatkom: PRACENJE_LINIJE");
}


//==============================================================================
// GLAVNA PETLJA - Upravlja strojem stanja, izvršava se neprestano
//==============================================================================
void loop() {
  ocitajSenzore(); // Prvo uvijek očitaj nove vrijednosti sa senzora

  // Glavni prekidač koji određuje ponašanje robota ovisno o trenutnom stanju [cite: 60]
  switch (trenutnoStanje) {
    
    case PRACENJE_LINIJE:
      pratiLiniju(); // Poziva funkciju koja upravlja motorima za praćenje linije
      
      // Uvjet za prijelaz u iduće stanje [cite: 32]
      if (jelSviSenzoriNaCrnom()) {
        prebaciStanje(PROVJERA_KRAJA);
      }
      break;

    case PROVJERA_KRAJA:
      naprijed(BRZINA_PROVJERE_KRAJA); // Kreći se kratko naprijed [cite: 34]
      
      // Provjeri je li prošlo dovoljno vremena
      if (millis() - vrijemePocetkaStanja > VRIJEME_PROVJERE_KRAJA_MS) {
        // Ako su i dalje svi senzori na crnom, kraj je potvrđen [cite: 35]
        if (jelSviSenzoriNaCrnom()) {
          zaustavi();
          prebaciStanje(CEKANJE_NA_TIPKU);
        } else {
          // Ako nisu, bio je lažni alarm, vrati se na praćenje [cite: 36]
          prebaciStanje(PRACENJE_LINIJE);
        }
      }
      break;

    case CEKANJE_NA_TIPKU:
      // U ovom stanju robot miruje [cite: 38]
      // Uvjet za prijelaz: pritisak tipkala [cite: 39]
      if (digitalRead(PIN_TIPKALO) == LOW) {
        delay(50); // Kratka pauza za debounce
        prebaciStanje(PORAVNANJE_KRETANJE);
      }
      break;

    case PORAVNANJE_KRETANJE:
      naprijed(BRZINA_PORAVNANJA_NAPRIJED); // Kreći se polako naprijed [cite: 41]
      
      // Uvjet za prijelaz: središnji senzor vidi kosu liniju [cite: 42]
      if (ocitanjaGlavnihSenzora[2] == 0) { // Senzor na indeksu 2 je središnji
        zaustavi();
        prebaciStanje(PORAVNANJE_OKRETANJE);
      }
      break;

    case PORAVNANJE_OKRETANJE:
      okreniDesno(BRZINA_PORAVNANJA_OKRET); // Okreći se u mjestu [cite: 44]
      
      // Uvjet za prijelaz: krajnji lijevi i desni senzor su na liniji [cite: 45]
      if (ocitanjaGlavnihSenzora[0] == 0 && ocitanjaGlavnihSenzora[4] == 0) {
        zaustavi();
        prebaciStanje(ZADATAK_ZAVRSEN);
      }
      break;

    case ZADATAK_ZAVRSEN:
      // Ne radi ništa. Zadatak je gotov. [cite: 47]
      // Robot stoji mirno.
      break;
  }
}

//==============================================================================
// POMOĆNE FUNKCIJE
//==============================================================================

/**
 * @brief Funkcija za promjenu stanja i ispis u Serial Monitor.
 * @param novoStanje Stanje u koje robot treba prijeći.
 */
void prebaciStanje(StanjeRobota novoStanje) {
  trenutnoStanje = novoStanje;
  vrijemePocetkaStanja = millis(); // Resetiraj timer za novo stanje
  
  // Ispis trenutnog stanja za lakše debugiranje [cite: 75]
  Serial.print("-> Novo stanje: ");
  switch (novoStanje) {
    case PRACENJE_LINIJE: Serial.println("PRACENJE_LINIJE"); break;
    case PROVJERA_KRAJA: Serial.println("PROVJERA_KRAJA"); break;
    case CEKANJE_NA_TIPKU: Serial.println("CEKANJE_NA_TIPKU"); break;
    case PORAVNANJE_KRETANJE: Serial.println("PORAVNANJE_KRETANJE"); break;
    case PORAVNANJE_OKRETANJE: Serial.println("PORAVNANJE_OKRETANJE"); break;
    case ZADATAK_ZAVRSEN: Serial.println("ZADATAK_ZAVRSEN"); break;
  }
}

/**
 * @brief Očitava vrijednosti sa svih 5 glavnih senzora.
 * Sprema 1 za bijelu, 0 za crnu podlogu. [cite: 21]
 */
void ocitajSenzore() {
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    ocitanjaGlavnihSenzora[i] = digitalRead(pinoviSenzora[i]);
  }
}

/**
 * @brief Provjerava jesu li svih 5 senzora na crnoj podlozi.
 * @return `true` ako su svi na crnom, inače `false`.
 */
bool jelSviSenzoriNaCrnom() {
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    if (ocitanjaGlavnihSenzora[i] != 0) { // Ako je ijedan senzor na bijelom (1)
      return false; // Onda nisu svi na crnom
    }
  }
  return true; // Ako petlja završi, znači da su svi na crnom
}

/**
 * @brief Izračunava odstupanje robota od linije (grešku).
 * Koristi zbroj težina senzora koji vide crnu liniju. 
 * Sadrži logiku za "pamćenje" linije kod oštrih zavoja. [cite: 56]
 * @return Vrijednost odstupanja. Negativno = linija lijevo, Pozitivno = linija desno.
 */
float izracunajOdstupanje() {
  float tezinskaSuma = 0;
  int senzoraNaLiniji = 0;

  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    if (ocitanjaGlavnihSenzora[i] == 0) { // Ako je senzor na crnoj liniji (0) [cite: 54]
      tezinskaSuma += tezine[i];
      senzoraNaLiniji++;
    }
  }

  if (senzoraNaLiniji > 0) {
    // Ako vidimo liniju, normalno izračunaj odstupanje i spremi ga.
    float odstupanje = tezinskaSuma / senzoraNaLiniji;
    zadnjePoznatoOdstupanje = odstupanje;
    return odstupanje;
  } else {
    // Ako smo izgubili liniju, donesi odluku na temelju zadnje poznate pozicije.
    if (zadnjePoznatoOdstupanje < -3) { // Ako je linija zadnje bila daleko lijevo
      return -10;  // Vrati veliku negativnu vrijednost za oštro skretanje ulijevo
    } else if (zadnjePoznatoOdstupanje > 3) { // Ako je linija zadnje bila daleko desno
      return 10;   // Vrati veliku pozitivnu vrijednost za oštro skretanje udesno
    } else {
      // Ako je bila blizu centra, koristi zadnju poznatu vrijednost
      return zadnjePoznatoOdstupanje;
    }
  }
}

/**
 * @brief Glavna funkcija za praćenje linije koristeći P-regulator.
 * Izračunava odstupanje i postavlja brzine motora.
 */
void pratiLiniju() {
  float odstupanje = izracunajOdstupanje();
  float skretanje = Kp * odstupanje;

  int brzinaLijevo = osnovnaBrzina - skretanje;
  int brzinaDesno = osnovnaBrzina + skretanje;

  postaviBrzine(brzinaLijevo, brzinaDesno);
}


//==============================================================================
// OSNOVNE FUNKCIJE ZA UPRAVLJANJE MOTORIMA
//==============================================================================

/**
 * @brief Postavlja brzinu i smjer za lijevu i desnu stranu robota.
 * @param lijevaBrzina Brzina za lijeve motore (-255 do 255).
 * @param desnaBrzina Brzina za desne motore (-255 do 255).
 */
void postaviBrzine(int lijevaBrzina, int desnaBrzina) {
  // Ograniči vrijednosti na raspon od -255 do 255
  lijevaBrzina = constrain(lijevaBrzina, -255, 255);
  desnaBrzina = constrain(desnaBrzina, -255, 255);

  // Upravljanje lijevim motorima
  if (lijevaBrzina > 0) {
    analogWrite(pinMotorLijeviNaprijed, lijevaBrzina);
    analogWrite(pinMotorLijeviNazad, 0);
  } else {
    analogWrite(pinMotorLijeviNaprijed, 0);
    analogWrite(pinMotorLijeviNazad, -lijevaBrzina);
  }

  // Upravljanje desnim motorima
  if (desnaBrzina > 0) {
    analogWrite(pinMotorDesniNaprijed, desnaBrzina);
    analogWrite(pinMotorDesniNazad, 0);
  } else {
    analogWrite(pinMotorDesniNaprijed, 0);
    analogWrite(pinMotorDesniNazad, -desnaBrzina);
  }
}

/**
 * @brief Zaustavlja sve motore.
 */
void zaustavi() {
  postaviBrzine(0, 0);
}

/**
 * @brief Pokreće robota ravno naprijed zadanom brzinom.
 * @param brzina Brzina kretanja (0 do 255).
 */
void naprijed(int brzina) {
  postaviBrzine(brzina, brzina);
}

/**
 * @brief Okreće robota u mjestu udesno zadanom brzinom.
 * @param brzina Brzina okretanja (0 do 255).
 */
void okreniDesno(int brzina) {
  postaviBrzine(brzina, -brzina);
}
