/********************************************************************************
 * @file    Zadatak 2 (labirint - linije) - Priručnik u Kodu
 * @author  Tim Juraj Dobrila II
 * @version 4.5
 * @date    16.10.2025.
 *
 * @brief   Ovaj kod je istovremeno i program za rješavanje labirinta i detaljan
 * priručnik za učenike. Svaki dio koda je opširno komentiran kako bi
 * objasnio strategije i tehnike programiranja autonomnih robota.
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
 * - Ovo je "mozak" našeg robota. Dok istražuje, robot pamti svaku odluku
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
 * - Motori: 4x DC motor s motornim driverom
 * - Senzori: 5x IR senzor u sredini (pinovi A0-A4)
 * - Senzori: 2x IR senzor sa strane za raskršća (pinovi 33, 34)
 * - Ulaz: 1x Tipkalo za start drugog prolaza (pin 15)
 ********************************************************************************/

//==============================================================================
// 1. HARDVERSKE POSTAVKE (Definicija Pinova i Parametara)
//==============================================================================
// Ovdje definiramo sve "fizičke" karakteristike našeg robota.
// Mijenjanje ovih vrijednosti direktno utječe na ponašanje hardvera.

// --- Postavke pinova za MOTORE ---
// Svaka strana robota (lijeva i desna) zahtijeva 3 pina za kontrolu:
//   - Dva za smjer (npr. IN1, IN2)
//   - Jedan za brzinu (PWM pin, npr. ENA)
#define speedPinR 9        // PWM pin za brzinu DESNE strane
#define RightMotorDirPin1 22 // Pin za smjer 1 DESNE strane
#define RightMotorDirPin2 24 // Pin za smjer 2 DESNE strane
#define LeftMotorDirPin1 26  // Pin za smjer 1 LIJEVE strane
#define LeftMotorDirPin2 28  // Pin za smjer 2 LIJEVE strane
#define speedPinL 11       // PWM pin za brzinu LIJEVE strane

// --- Postavke pinova za SENZORE ---
const int pinoviGlavnihSenzora[] = {A0, A1, A2, A3, A4}; // Glavni niz od 5 senzora
const int BROJ_GLAVNIH_SENZORA = 5;
// PRAG_CRNE_BOJE je ključna vrijednost za kalibraciju.
// Očitavamo analogne vrijednosti sa senzora (0-1023). Moramo eksperimentalno
// pronaći vrijednost koja pouzdano razdvaja crnu od bijele podloge.
const int PRAG_CRNE_BOJE = 400; // Primjer: sve iznad 400 smatramo crnom bojom.

// Bočni senzori nam trebaju samo za digitalnu informaciju (vidi/ne vidi),
// pa ih spajamo na digitalne pinove.
const int PIN_SENZOR_LIJEVI = 33;
const int PIN_SENZOR_DESNI = 34;

// Tipkalo za interakciju s korisnikom
const int PIN_TIPKALO = 15;

// --- Parametri za P-regulator (Fino Podešavanje Praćenja Linije) ---
// Kp (Proporcionalni koeficijent) određuje "agresivnost" robota.
// Veći Kp = oštrije i brže reakcije (dobro za oštre zavoje, loše za ravnicu).
// Manji Kp = glađe i sporije reakcije.
float Kp = 40;
// Osnovna brzina je brzina motora (0-255) kada robot ide savršeno ravno.
int osnovnaBrzina = 100;
// Težine određuju "važnost" svakog senzora. Krajnji senzori imaju najveću
// težinu jer signaliziraju najveće odstupanje od linije.
const int tezine[BROJ_GLAVNIH_SENZORA] = {-6, -3, 0, 3, 6};

// --- Parametri za manevre (Potrebno Kalibrirati!) ---
// Vremena za okrete ovise o brzini okretanja, podlozi i stanju baterije.
// Ove vrijednosti morate pronaći eksperimentalno!
const int BRZINA_OKRETA = 120;
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
  Serial.println("Inicijalizacija: Zadatak 2, v4.5 - Tim Juraj Dobrila I (Edukacijska Verzija)");

  // Postavljanje svih pinova za motore kao IZLAZNE
  pinMode(RightMotorDirPin1, OUTPUT);
  pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(speedPinR, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT);
  pinMode(LeftMotorDirPin2, OUTPUT);
  pinMode(speedPinL, OUTPUT);

  // Postavljanje pinova za bočne senzore i tipkalo kao ULAZNE
  pinMode(PIN_SENZOR_LIJEVI, INPUT);
  pinMode(PIN_SENZOR_DESNI, INPUT);
  // INPUT_PULLUP koristi interni otpornik na Arduinu, tako da nam ne treba vanjski.
  // Pin će biti HIGH kada tipkalo nije pritisnuto, a LOW kada jest.
  pinMode(PIN_TIPKALO, INPUT_PULLUP);
  
  stop_bot(); // Osiguraj da motori stoje na početku
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
  bool lijeviBocniVidiLiniju = (digitalRead(PIN_SENZOR_LIJEVI) == 0);
  bool desniBocniVidiLiniju = (digitalRead(PIN_SENZOR_DESNI) == 0);
  int ocitanjaGlavnihSenzora[BROJ_GLAVNIH_SENZORA];
  ocitajGlavneSenzore(ocitanjaGlavnihSenzora);

  // 'switch' naredba je idealna za implementaciju stroja stanja.
  switch (trenutnoStanje) {
    //-----------------------------------------------------
    // LOGIKA ZA PRVI PROLAZ (ISTRAŽIVANJE)
    //-----------------------------------------------------
    case PRVI_PROLAZ_ISTRAZIVANJE:
      // Uvjet za kraj: ako svi glavni senzori vide crno, stigli smo na cilj.
      if (jelSviSenzoriNaCrnom(ocitanjaGlavnihSenzora)) {
        stop_bot();
        Serial.println("\nCILJ DOSEGNUT! PRVI PROLAZAK GOTOV.");
        ispisiPutanju("Finalna optimizirana putanja", putanja, duzinaPutanje);
        Serial.println("\nPritisni tipkalo za drugi, brzi prolazak.");
        trenutnoStanje = CEKANJE_NA_DRUGI_PROLAZ; // Prelazak u stanje čekanja
        break; // Odmah izađi iz switch-a
      }
      
      // Provjeri jesmo li na raskrižju.
      if (jelNaRaskrscu(ocitanjaGlavnihSenzora, lijeviBocniVidiLiniju, desniBocniVidiLiniju)) {
        // Centriranje: Pomakni se malo naprijed da centar robota bude na raskrižju.
        forward(osnovnaBrzina, osnovnaBrzina); delay(150);
        stop_bot(); delay(200);
        trenutnoStanje = PRVI_PROLAZ_ODLUKA; // Prelazak u stanje donošenja odluke
      } else {
        // Ako nismo na raskrižju, samo nastavi pratiti liniju.
        pratiLiniju(ocitanjaGlavnihSenzora);
      }
      break;

    case PRVI_PROLAZ_ODLUKA:
      // Očitaj senzore ponovno dok stojimo, za maksimalnu preciznost.
      ocitajGlavneSenzore(ocitanjaGlavnihSenzora);
      lijeviBocniVidiLiniju = (digitalRead(PIN_SENZOR_LIJEVI) == 0);

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
        stop_bot();
        Serial.println("\nZADATAK ZAVRSEN!");
        trenutnoStanje = ZADATAK_ZAVRSEN;
        break;
      }
      // Logika je ista kao u prvom prolazu: prati liniju dok ne dođeš na raskrižje.
      if (jelNaRaskrscu(ocitanjaGlavnihSenzora, lijeviBocniVidiLiniju, desniBocniVidiLiniju)) {
        forward(osnovnaBrzina, osnovnaBrzina); delay(150);
        stop_bot(); delay(200);
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
    zabiljeziIOptimizirajKorak('R'); // Dodaj ispravan korak 'R' i ponovno provjeri optimizaciju
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

// Očitava 5 glavnih senzora s analognih pinova
void ocitajGlavneSenzore(int* ocitanja) {
  for (int i = 0; i < BROJ_GLAVNIH_SENZORA; i++) {
    ocitanja[i] = (analogRead(pinoviGlavnihSenzora[i]) > PRAG_CRNE_BOJE) ? 0 : 1;
  }
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
  
  // Diferencijalno upravljanje: brzina lijevog i desnog kotača se mijenja
  // ovisno o 'skretanju' kako bi se robot ispravio.
  int brzinaLijevo = constrain(osnovnaBrzina + skretanje, 0, 255);
  int brzinaDesno  = constrain(osnovnaBrzina - skretanje, 0, 255);
  
  forward(brzinaLijevo, brzinaDesno);
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
// Ove funkcije su "mišići" robota. One direktno komuniciraju s motornim
// driverom i govore mu kako da se vrti.

void stop_bot() {
  analogWrite(speedPinL, 0);
  analogWrite(speedPinR, 0);
}

void forward(int left_speed, int right_speed) {
  digitalWrite(RightMotorDirPin1, HIGH);
  digitalWrite(RightMotorDirPin2, LOW);
  digitalWrite(LeftMotorDirPin1, HIGH);
  digitalWrite(LeftMotorDirPin2, LOW);
  analogWrite(speedPinL, left_speed);
  analogWrite(speedPinR, right_speed);
}

void right_turn(int speed) {
  digitalWrite(RightMotorDirPin1, LOW); // Desni kotači idu nazad
  digitalWrite(RightMotorDirPin2, HIGH);
  digitalWrite(LeftMotorDirPin1, HIGH); // Lijevi kotači idu naprijed
  digitalWrite(LeftMotorDirPin2, LOW);
  analogWrite(speedPinL, speed);
  analogWrite(speedPinR, speed);
}

void left_turn(int speed) {
  digitalWrite(RightMotorDirPin1, HIGH); // Desni kotači idu naprijed
  digitalWrite(RightMotorDirPin2, LOW);
  digitalWrite(LeftMotorDirPin1, LOW);  // Lijevi kotači idu nazad
  digitalWrite(LeftMotorDirPin2, HIGH);
  analogWrite(speedPinL, speed);
  analogWrite(speedPinR, speed);
}

// Funkcije za manevre koje poziva naš algoritam.
// One kombiniraju osnovne pokrete i 'delay' kako bi izvršile složeniji manevar.
void izvrsiSkretanjeLijevo() { Serial.println("Manevar: Skretanje LIJEVO"); left_turn(BRZINA_OKRETA); delay(VRIJEME_OKRETA_90_STUPNJEVA); stop_bot(); delay(200); }
void izvrsiSkretanjeDesno() { Serial.println("Manevar: Skretanje DESNO"); right_turn(BRZINA_OKRETA); delay(VRIJEME_OKRETA_90_STUPNJEVA); stop_bot(); delay(200); }
void izvrsiOkret180() { Serial.println("Manevar: Okret 180"); right_turn(BRZINA_OKRETA); delay(VRIJEME_OKRETA_180_STUPNJEVA); stop_bot(); delay(200); }
void prijediRavnoPrekoRaskrsca() { Serial.println("Manevar: Prelazak RAVNO"); forward(osnovnaBrzina, osnovnaBrzina); delay(VRIJEME_PRELASKA_RASKRSCA); }