/********************************************************************************
 * @file    Zadatak 2 (labirint - linije) - Servo Priručnik
 * @author  Tim Juraj Dobrila I
 * @version 4.5
 * @date    16.10.2025.
 *
 * @brief   Ovaj kod je istovremeno i program za rješavanje labirinta i detaljan
 * priručnik za učenike. Prilagođen je za natjecateljskog robota s
 * kontinuiranim servo motorima.
 *
 * ### STRATEGIJA RJEŠAVANJA LABIRINTA ###
 *
 * Algoritam se temelji na metodi dva prolaska, osmišljenoj da robot prvo
 * nauči labirint, a zatim ga prođe najkraćim putem.
 *
 * 1. PRVI PROLAZ (ISTRAŽIVANJE):
 * - Robot koristi "pravilo lijeve ruke" (Left-Hand Rule) za navigaciju. To
 * je jednostavno pravilo prioriteta: na svakom raskrižju, robot uvijek
 * pokuša prvo skrenuti lijevo. Ako ne može, ide ravno. Ako ne može ni to,
 * zna da je u slijepoj ulici i okreće se.
 *
 * 2. OPTIMIZACIJA "U LETU" (On-the-fly Optimization):
 * - Ovo je "mozak" našeg robota. Dok istražuje, pamti svaku odluku
 * ('L' za lijevo, 'R' za ravno, 'N' za okret nazad).
 * - Čim se vrati iz slijepe ulice, algoritam prepoznaje sekvencu pogrešnih
 * poteza i automatski je zamjenjuje jednim, ispravnim potezom.
 * Na primjer, sekvenca L-N-L (idi lijevo, slijepa ulica, vrati se, pa idi
 * opet lijevo iz nove perspektive) se pretvara u samo 'R' (ravno).
 *
 * 3. DRUGI PROLAZ (NAJKRAĆI PUT):
 * - Nakon što je stigao na cilj i optimizirao putanju, robot čeka na signal.
 * - U drugom prolasku, on više ne "razmišlja" (ne koristi pravilo lijeve
 * ruke), već samo slijepo izvršava naredbe iz svoje optimizirane memorije.
 *
 * @hardware_setup
 * - Motori: 4x Kontinuirani servo motori
 * - Senzori: 5x IR senzor u sredini (pinovi 28-32)
 * - Senzori: 2x IR senzor sa strane za raskršća (pinovi 33, 34)
 * - Ulaz: 1x Tipkalo za start drugog prolaza (pin 15)
 ********************************************************************************/

//==============================================================================
// 1. HARDVERSKE POSTAVKE (Definicija Pinova i Parametara)
//==============================================================================
// Ovdje definiramo sve "fizičke" karakteristike našeg robota.
// Mijenjanje ovih vrijednosti direktno utječe na ponašanje hardvera.

#include <Servo.h>

// --- Postavke pinova za MOTORE ---
const int BROJ_MOTORA = 4;
const int pinoviMotora[BROJ_MOTORA] = {2, 3, 4, 5};
// Dajemo imena motorima prema poziciji radi lakšeg snalaženja u kodu.
const int MOTOR_LS = 0, MOTOR_LP = 2, MOTOR_DS = 1, MOTOR_DP = 3;

// --- Postavke pinova za SENZORE ---
const int BROJ_GLAVNIH_SENZORA = 5;
const int pinoviGlavnihSenzora[BROJ_GLAVNIH_SENZORA] = {26, 27, 28, 29, 30};
// Bočni senzori detektiraju samo prisutnost linije, pa ih spajamo na digitalne pinove.
const int PIN_SENZOR_LIJEVI = 31;
const int PIN_SENZOR_DESNI = 32;
// Tipkalo za interakciju s korisnikom
const int PIN_TIPKALO = 15;

// --- Parametri za P-regulator (Fino Podešavanje Praćenja Linije) ---
// Kp (Proporcionalni koeficijent) određuje "agresivnost" robota.
// Veći Kp = oštrije i brže reakcije (dobro za oštre zavoje, loše za ravnicu).
// Manji Kp = glađe i sporije reakcije.
float Kp = 40;
// Osnovna brzina je brzina motora u skali od 0-100.
int osnovnaBrzina = 90;
// Težine određuju "važnost" svakog senzora. Krajnji senzori imaju najveću
// težinu jer signaliziraju najveće odstupanje od linije.
const int tezine[BROJ_GLAVNIH_SENZORA] = {-6, -3, 0, 3, 6};

// --- Parametri za manevre (Potrebno Kalibrirati!) ---
// Vremena za okrete ovise o brzini okretanja, podlozi i stanju baterije.
// Ove vrijednosti morate pronaći eksperimentalno!
const int BRZINA_OKRETA = 80; // U skali 0-100
const int VRIJEME_OKRETA_90_STUPNJEVA = 500; // U milisekundama
const int VRIJEME_OKRETA_180_STUPNJEVA = 1000;
const int VRIJEME_PRELASKA_RASKRSCA = 300;


//==============================================================================
// 2. STRUKTURA ALGORITMA - Stroj Stanja i Memorija
//==============================================================================
// Da bismo upravljali složenim ponašanjem, koristimo "Stroj Stanja" (State Machine).
// Robot se u svakom trenutku može nalaziti u samo jednom od ovih stanja.
// To čini kod organiziranim i lakim za praćenje.
enum StanjeRobota {
  PRVI_PROLAZ_ISTRAZIVANJE, // Glavno stanje: Prati liniju, traži raskrižja
  PRVI_PROLAZ_ODLUKA,       // Pomoćno stanje: Stao je i donosi odluku
  CEKANJE_NA_DRUGI_PROLAZ,  // Stanje mirovanja: Čeka na pritisak tipkala
  DRUGI_PROLAZ_PRACENJE,    // Glavno stanje drugog prolaska: Prati liniju po mapi
  DRUGI_PROLAZ_MANEVAR,     // Pomoćno stanje: Izvršava spremljenu naredbu
  ZADATAK_ZAVRSEN           // Kraj programa
};

// Globalne varijable koje čine "mozak" i "memoriju" robota
Servo motori[BROJ_MOTORA];
StanjeRobota trenutnoStanje = PRVI_PROLAZ_ISTRAZIVANJE; // Početno stanje
float zadnjePoznatoOdstupanje = 0; // Za pamćenje smjera kod oštrih zavoja

// Memorija za pamćenje puta. 'char' niz je jednostavan način za pohranu
// niza odluka ('L', 'R', 'N', 'D').
char putanja[100];
int duzinaPutanje = 0; // Koliko je koraka trenutno u memoriji
int korakNaPutu = 0;   // Koji korak iz memorije izvršavamo u drugom prolazu


//==============================================================================
// 3. SETUP - Inicijalizacija Robota
//==============================================================================
void setup() {
  Serial.begin(9600); // Pokretanje komunikacije s računalom za ispis poruka
  Serial.println("Inicijalizacija: Zadatak 2, v4.5 - Servo Verzija (Edukacijska)");

  // Spajanje (attach) svakog servo motora na odgovarajući pin
  for (int i = 0; i < BROJ_MOTORA; i++) {
    motori[i].attach(pinoviMotora[i]);
  }

  // Postavljanje pinova za senzore i tipkalo kao ULAZNE
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    pinMode(pinoviGlavnihSenzora[i], INPUT);
  }
  pinMode(PIN_SENZOR_LIJEVI, INPUT);
  pinMode(PIN_SENZOR_DESNI, INPUT);
  pinMode(PIN_TIPKALO, INPUT_PULLUP);
  
  zaustavi(); // Osiguraj da motori stoje na početku
  delay(1000);
  Serial.println("Sustav spreman. Krećem u prvi prolaz (istraživanje).");
}

//==============================================================================
// 4. GLAVNA PETLJA (LOOP) - Srce Programa
//==============================================================================
// Loop funkcija radi kao dirigent orkestra. U svakom ciklusu provjeri u kojem
// je robot stanju (koju "notu" svira) i pozove odgovarajuću funkciju.
void loop() {
  // Očitavanje svih senzora se uvijek radi na početku svakog ciklusa.
  // Podatke spremamo u lokalne varijable da budu isti za cijeli ciklus.
  int ocitanjaGlavnihSenzora[BROJ_GLAVNIH_SENZORA];
  ocitajSveSenzore(ocitanjaGlavnihSenzora);
  bool lijeviBocniVidiLiniju = (ocitanja[BROJ_GLAVNIH_SENZORA] == 0);
  bool desniBocniVidiLiniju = (ocitanja[BROJ_GLAVNIH_SENZORA+1] == 0);

  // 'switch' naredba je idealna za implementaciju stroja stanja.
  switch (trenutnoStanje) {
    //-----------------------------------------------------
    // LOGIKA ZA PRVI PROLAZ (ISTRAŽIVANJE)
    //-----------------------------------------------------
    case PRVI_PROLAZ_ISTRAZIVANJE:
      // Uvjet za kraj: ako svi glavni senzori vide crno, stigli smo na cilj.
      if (jelSviSenzoriNaCrnom(ocitanjaGlavnihSenzora)) {
        zaustavi();
        Serial.println("\nCILJ DOSEGNUT! PRVI PROLAZAK GOTOV.");
        ispisiPutanju("Finalna optimizirana putanja", putanja, duzinaPutanje);
        Serial.println("\nPritisni tipkalo za drugi, brzi prolazak.");
        trenutnoStanje = CEKANJE_NA_DRUGI_PROLAZ; // Prelazak u stanje čekanja
        break; // Odmah izađi iz switch-a
      }
      
      // Provjeri jesmo li na raskrižju.
      if (jelNaRaskrscu(ocitanjaGlavnihSenzora, lijeviBocniVidiLiniju, desniBocniVidiLiniju)) {
        // Centriranje: Pomakni se malo naprijed da centar robota bude na raskrižju.
        naprijed(osnovnaBrzina); delay(150);
        zaustavi(); delay(200);
        trenutnoStanje = PRVI_PROLAZ_ODLUKA; // Prelazak u stanje donošenja odluke
      } else {
        // Ako nismo na raskrižju, samo nastavi pratiti liniju.
        pratiLiniju(ocitanjaGlavnihSenzora);
      }
      break;

    case PRVI_PROLAZ_ODLUKA:
      // Očitaj senzore ponovno dok stojimo, za maksimalnu preciznost.
      ocitajSveSenzore(ocitanjaGlavnihSenzora);
      lijeviBocniVidiLiniju = (ocitanja[BROJ_GLAVNIH_SENZORA] == 0);

      // Primijeni "Pravilo Lijeve Ruke"
      if (lijeviBocniVidiLiniju) {
        zabiljeziIOptimizirajKorak('L');
        izvrsiSkretanjeLijevo();
      } else if (ocitanjaGlavnihSenzora[2] == 0) { // Ako središnji senzor vidi liniju
        zabiljeziIOptimizirajKorak('R');
        prijediRavnoPrekoRaskrsca();
      } else {
        zabiljeziIOptimizirajKorak('N');
        izvrsiOkret180();
      }
      // Nakon izvršenog manevra, ispiši trenutno stanje memorije i vrati se u istraživanje.
      ispisiPutanju("Trenutna putanja", putanja, duzinaPutanje);
      trenutnoStanje = PRVI_PROLAZ_ISTRAZIVANJE;
      break;

    //-----------------------------------------------------
    // STANJE ČEKANJA
    //-----------------------------------------------------
    case CEKANJE_NA_DRUGI_PROLAZ:
      // U ovom stanju robot ne radi ništa osim što provjerava tipkalo.
      if (digitalRead(PIN_TIPKALO) == LOW) {
        delay(50); // Debounce - kratka pauza za sprječavanje višestrukih pritisaka
        Serial.println("\nKrecem u drugi prolaz (najkraci put).");
        korakNaPutu = 0; // Resetiraj brojač koraka na početak mape
        trenutnoStanje = DRUGI_PROLAZ_PRACENJE;
      }
      break;

    //-----------------------------------------------------
    // LOGIKA ZA DRUGI PROLAZ (IZVRŠAVANJE)
    //-----------------------------------------------------
    case DRUGI_PROLAZ_PRACENJE:
      if (jelSviSenzoriNaCrnom(ocitanjaGlavnihSenzora)) {
        zaustavi();
        Serial.println("\nZADATAK ZAVRSEN!");
        trenutnoStanje = ZADATAK_ZAVRSEN;
        break;
      }
      // Logika je ista kao u prvom prolazu: prati liniju dok ne dođeš na raskrižje.
      if (jelNaRaskrscu(ocitanjaGlavnihSenzora, lijeviBocniVidiLiniju, desniBocniVidiLiniju)) {
        naprijed(osnovnaBrzina); delay(150);
        zaustavi(); delay(200);
        trenutnoStanje = DRUGI_PROLAZ_MANEVAR;
      } else {
        pratiLiniju(ocitanjaGlavnihSenzora);
      }
      break;

    case DRUGI_PROLAZ_MANEVAR:
      // U drugom prolazu, ne "razmišljamo", samo izvršavamo naredbe iz memorije.
      if (korakNaPutu < duzinaPutanje) {
        char sljedeciKorak = putanja[korakNaPutu];
        korakNaPutu++; // Pripremi se za idući korak na idućem raskrižju
        
        // Izvrši manevar koji je zapisan u memoriji
        if (sljedeciKorak == 'L') { izvrsiSkretanjeLijevo(); } 
        else if (sljedeciKorak == 'R') { prijediRavnoPrekoRaskrsca(); } 
        else if (sljedeciKorak == 'D') { izvrsiSkretanjeDesno(); }
      }
      // Vrati se na praćenje linije do idućeg raskrižja
      trenutnoStanje = DRUGI_PROLAZ_PRACENJE;
      break;

    case ZADATAK_ZAVRSEN:
      // Robot je gotov, ne radi ništa.
      break;
  }
}

//==============================================================================
// 5. ALGORITAM - "Mozak" za Pamćenje i Optimizaciju
//==============================================================================

/**
 * @brief Bilježi korak i odmah pokušava optimizirati putanju.
 * Ovo je najkompleksniji dio koda i "mozak" robota za mapiranje.
 * Koristi rekurziju: funkcija poziva samu sebe kako bi osigurala
 * da se putanja optimizira do kraja nakon svake promjene.
 * @param korak Odluka donesena na raskrižju ('L', 'R', 'N').
 */
void zabiljeziIOptimizirajKorak(char korak) {
  if (duzinaPutanje >= 99) return; // Osiguranje od preljeva memorije
  
  // 1. Uvijek prvo dodaj novi korak na kraj putanje.
  putanja[duzinaPutanje] = korak;
  duzinaPutanje++;

  // 2. Ako nemamo barem 3 koraka u povijesti, nema smisla provjeravati.
  if (duzinaPutanje < 3) return;

  // --- Pravila Zamjene ---
  // Analiziramo zadnja 3 koraka. 'p' je pokazivač na početak te sekvence.
  char* p = &putanja[duzinaPutanje - 3];

  // Pravilo 1: L-N-L -> R
  // (Skrenuo lijevo, slijepa ulica, vratio se, iduća opcija je bila 'opet lijevo' (tj. ravno))
  if (p[0] == 'L' && p[1] == 'N' && p[2] == 'L') {
    duzinaPutanje -= 3; // Logički obriši zadnja 3 koraka
    zabiljeziIOptimizirajKorak('R'); // Dodaj ispravan korak 'R' i ponovno pokreni optimizaciju
  }
  // Pravilo 2: L-N-R -> D
  // (Skrenuo lijevo, slijepa ulica, vratio se, iduća opcija je bila 'ravno' (tj. desno))
  else if (p[0] == 'L' && p[1] == 'N' && p[2] == 'R') {
    duzinaPutanje -= 3;
    zabiljeziIOptimizirajKorak('D');
  }
  // Pravilo 3: R-N-L -> D
  // (Išao ravno, slijepa ulica, vratio se, iduća opcija je bila 'lijevo' (tj. desno))
  else if (p[0] == 'R' && p[1] == 'N' && p[2] == 'L') {
    duzinaPutanje -= 3;
    zabiljeziIOptimizirajKorak('D');
  }
}

// Pomoćna funkcija za ispis memorije robota
void ispisiPutanju(String poruka, char* p, int duzina) {
  Serial.print(poruka); Serial.print(": ");
  for (int i = 0; i < duzina; i++) { Serial.print(p[i]); }
  Serial.println();
}

//==============================================================================
// 6. LOGIKA - Pomoćne Funkcije za Senzore i Praćenje
//==============================================================================

// Očitava svih 7 senzora i sprema ih u jedno polje radi lakšeg prosljeđivanja
void ocitajSveSenzore(int* ocitanja) {
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    ocitanja[i] = digitalRead(pinoviGlavnihSenzora[i]);
  }
  // Bočne senzore spremamo na kraj istog polja
  ocitanja[BROJ_GLAVNIH_SENZORA] = digitalRead(PIN_SENZOR_LIJEVI);
  ocitanja[BROJ_GLAVNIH_SENZORA+1] = digitalRead(PIN_SENZOR_DESNI);
}

// Provjerava da li se robot nalazi na raskrižju gdje treba donijeti odluku
bool jelNaRaskrscu(int* ocitanja, bool lijeviVidi, bool desniVidi) {
  bool sredinaVidiLiniju = (ocitanja[2] == 0);
  if (sredinaVidiLiniju && (lijeviVidi || desniVidi)) {
    return true; // T ili + raskrižje
  }
  bool sviGlavniSenzoriBijelo = (ocitanja[0]==1&&ocitanja[1]==1&&ocitanja[2]==1&&ocitanja[3]==1&&ocitanja[4]==1);
  if (sviGlavniSenzoriBijelo && !lijeviVidi && !desniVidi) {
    return true; // Slijepa ulica
  }
  return false; 
}

// Provjerava jesu li svi glavni senzori na crnom (kraj staze)
bool jelSviSenzoriNaCrnom(int* ocitanja) {
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    if (ocitanja[i] != 0) return false;
  }
  return true;
}

// Upravlja robotom da prati liniju koristeći P-regulator
void pratiLiniju(int* ocitanja) {
  float odstupanje = izracunajOdstupanje(ocitanja);
  float skretanje = Kp * odstupanje;
  
  // Izračunaj brzine za lijevu i desnu stranu
  int brzinaLijevo = osnovnaBrzina + skretanje;
  int brzinaDesno  = osnovnaBrzina - skretanje;
  
  // Pošalji komande motorima. Zbog načina montaže, desni motori se moraju vrtiti "unazad"
  // da bi robot išao naprijed.
  postaviBrzinuMotora(MOTOR_LP, brzinaLijevo);
  postaviBrzinuMotora(MOTOR_LS, brzinaLijevo);
  postaviBrzinuMotora(MOTOR_DP, -brzinaDesno);
  postaviBrzinuMotora(MOTOR_DS, -brzinaDesno);
}

// Izračunava odstupanje od linije. Pamti zadnju poziciju za oštre zavoje
float izracunajOdstupanje(int* ocitanja) {
  float tezinskaSuma = 0; int senzoraNaLiniji = 0;
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    if (ocitanja[i] == 0) { // Ako je senzor na crnom
      tezinskaSuma += tezine[i]; senzoraNaLiniji++;
    }
  }
  if (senzoraNaLiniji > 0) {
    float odstupanje = tezinskaSuma / senzoraNaLiniji;
    zadnjePoznatoOdstupanje = odstupanje; // Spremi za slučaj da izgubimo liniju
    return odstupanje;
  } else {
    // Ako smo izgubili liniju, vrati ekstremnu vrijednost na temelju zadnje
    // poznate pozicije kako bi se robot oštro okrenuo i pronašao je.
    return (zadnjePoznatoOdstupanje > 0) ? 5 : -5;
  }
}

//==============================================================================
// 7. NISKA RAZINA - Upravljanje Motorima i Manevri
//==============================================================================
// Ove funkcije su "mišići" robota. One direktno komuniciraju sa servo motorima.

/**
 * @brief Osnovna funkcija koja postavlja brzinu i smjer jednog servo motora.
 * @param indeksMotora Koji motor kontroliramo (0-3).
 * @param brzina Željena brzina od -100 (puna nazad) do 100 (puna naprijed).
 */
void postaviBrzinuMotora(int indeksMotora, int brzina) {
  brzina = constrain(brzina, -100, 100);
  // Mapiraj našu skalu (-100 do 100) na servo skalu (0 do 180).
  // 90 je točka mirovanja za kontinuirane servo motore.
  int servoVrijednost = map(brzina, -100, 100, 0, 180);
  motori[indeksMotora].write(servoVrijednost);
}

void zaustavi() {
  for (int i = 0; i < BROJ_MOTORA; i++) { postaviBrzinuMotora(i, 0); }
}

void naprijed(int brzina) {
  // Lijevi idu naprijed, desni "nazad" (zbog inverzne montaže)
  postaviBrzinuMotora(MOTOR_LP, brzina); postaviBrzinuMotora(MOTOR_LS, brzina);
  postaviBrzinuMotora(MOTOR_DP, -brzina); postaviBrzinuMotora(MOTOR_DS, -brzina);
}

void okreniDesno(int brzina) {
  // Svi kotači se vrte u istom smjeru da se robot rotira
  postaviBrzinuMotora(MOTOR_LP, brzina); postaviBrzinuMotora(MOTOR_LS, brzina);
  postaviBrzinuMotora(MOTOR_DP, brzina); postaviBrzinuMotora(MOTOR_DS, brzina);
}

void okreniLijevo(int brzina) {
  // Svi kotači se vrte u suprotnom smjeru
  postaviBrzinuMotora(MOTOR_LP, -brzina); postaviBrzinuMotora(MOTOR_LS, -brzina);
  postaviBrzinuMotora(MOTOR_DP, -brzina); postaviBrzinuMotora(MOTOR_DS, -brzina);
}

// Funkcije za manevre koje poziva naš algoritam.
void izvrsiSkretanjeLijevo() { Serial.println("Manevar: Skretanje LIJEVO"); okreniLijevo(BRZINA_OKRETA); delay(VRIJEME_OKRETA_90_STUPNJEVA); zaustavi(); delay(200); }
void izvrsiSkretanjeDesno() { Serial.println("Manevar: Skretanje DESNO"); okreniDesno(BRZINA_OKRETA); delay(VRIJEME_OKRETA_90_STUPNJEVA); zaustavi(); delay(200); }
void izvrsiOkret180() { Serial.println("Manevar: Okret 180"); okreniDesno(BRZINA_OKRETA); delay(VRIJEME_OKRETA_180_STUPNJEVA); zaustavi(); delay(200); }
void prijediRavnoPrekoRaskrsca() { Serial.println("Manevar: Prelazak RAVNO"); naprijed(osnovnaBrzina); delay(VRIJEME_PRELASKA_RASKRSCA); }