/********************************************************************************
 * @file    Zadatak 1 (Praćenje Linije - DC Motori)
 * @author  Ekipa 'Juraj Dobrila II'
 * @version 2.0 (Prezentacijska verzija)
 * @date    07.10.2025.
 *
 * @brief   Upravljački softver za autonomnog robota dizajniranog za natjecateljski
 * zadatak praćenja linije. Kod implementira naprednu logiku pomoću
 * "stroja stanja" (State Machine) za upravljanje različitim fazama
 * zadatka, te P-regulator za glatko i precizno praćenje linije.
 *
 * @hardware
 * - Mikrokontroler: Arduino Mega
 * - Pogonski sustav: Robot s 4 DC motora i pripadajućim motornim driverom
 * - Senzori: 5-kanalni infracrveni (IR) senzor za detekciju linije
 * - Ulaz: 1x Tipkalo za interakciju korisnika
 ********************************************************************************/

//==============================================================================
// DEFINICIJE PINOVA (Hardverska postava)
//==============================================================================
// Komentari objašnjavaju koji pin na Arduinu kontrolira koju funkciju na motornom driveru.

// --- Pinovi za prednje motore ---
#define speedPinR 9           // PWM pin za brzinu prednjeg desnog motora
#define RightMotorDirPin1 24  // Pin za smjer 1 prednjeg desnog motora
#define RightMotorDirPin2 22  // Pin za smjer 2 prednjeg desnog motora
#define speedPinL 10          // PWM pin za brzinu prednjeg lijevog motora
#define LeftMotorDirPin1 28   // Pin za smjer 1 prednjeg lijevog motora
#define LeftMotorDirPin2 26   // Pin za smjer 2 prednjeg lijevog motora

// --- Pinovi za stražnje motore ---
#define speedPinRB 11         // PWM pin za brzinu stražnjeg desnog motora
#define RightMotorDirPin1B 6  // Pin za smjer 1 stražnjeg desnog motora
#define RightMotorDirPin2B 5  // Pin za smjer 2 stražnjeg desnog motora
#define speedPinLB 12         // PWM pin za brzinu stražnjeg lijevog motora
#define LeftMotorDirPin1B 8   // Pin za smjer 1 stražnjeg lijevog motora
#define LeftMotorDirPin2B 7   // Pin za smjer 2 stražnjeg lijevog motora

// --- Pinovi za 5-kanalni IR senzor ---
// Spojeni na analogne pinove, ali se koriste kao digitalni ulazi
#define sensor1 A4  // Krajnji lijevi senzor
#define sensor2 A3
#define sensor3 A2  // Središnji senzor
#define sensor4 A1
#define sensor5 A0  // Krajnji desni senzor

//==============================================================================
// GLAVNE POSTAVKE I PARAMETRI
//==============================================================================

const int BROJ_SENZORA = 5;
const int pinoviSenzora[BROJ_SENZORA] = { sensor1, sensor2, sensor3, sensor4, sensor5 };
// "Težine" za svaki senzor. Ključne za izračun pozicije linije.
// Negativne vrijednosti znače da je linija lijevo, pozitivne da je desno.
const int tezine[BROJ_SENZORA] = { -6, -3, 0, 3, 6 };
const int PIN_TIPKALO = 15;  // Pin za gumb kojim pokrećemo drugu fazu zadatka.

// --- Parametri za P-regulator (PID kontroler) ---
// OVE VRIJEDNOSTI JE POTREBNO EKSPERIMENTALNO PODESITI ZA OPTIMALAN RAD!
float Kp = 9;            // Proporcionalni koeficijent. Određuje "agresivnost" reakcije robota na grešku.
int osnovnaBrzina = 100;  // Osnovna brzina kretanja (0-255). Određuje opću brzinu robota na stazi.

// --- Parametri za posebne manevre ---
const int VRIJEME_PROVJERE_KRAJA_MS = 300;  // Koliko dugo (u ms) robot ide naprijed da provjeri je li na crnom kvadratu.
const int BRZINA_PROVJERE_KRAJA = 80;       // Brzina kretanja tijekom te provjere.
const int BRZINA_PORAVNANJA_NAPRIJED = 80;  // Brzina kretanja prema kosoj liniji.
const int BRZINA_PORAVNANJA_OKRET = 70;     // Brzina rotacije prilikom poravnanja.

//==============================================================================
// DEFINICIJA STANJA ROBOTA (Logika "Stroja Stanja")
//==============================================================================
// "Stroj stanja" omogućuje robotu da točno zna koji dio zadatka trenutno izvršava.
enum StanjeRobota {
  PRACENJE_LINIJE,       // Glavno stanje: robot prati liniju.
  PROVJERA_KRAJA,        // Stanje provjere: robot misli da je na cilju i vozi naprijed da potvrdi.
  CEKANJE_NA_TIPKU,      // Stanje čekanja: robot je na cilju i čeka gumb za idući korak.
  PORAVNANJE_KRETANJE,   // Stanje poravnanja 1: robot se kreće naprijed do kose linije.
  PORAVNANJE_OKRETANJE,  // Stanje poravnanja 2: robot se rotira da se poravna s kosom linijom.
  ZADATAK_ZAVRSEN        // Konačno stanje: zadatak je gotov.
};

//==============================================================================
// GLOBALNE VARIJABLE
//==============================================================================
int ocitanjaSenzora[BROJ_SENZORA];              // Sprema zadnje očitane vrijednosti senzora.
StanjeRobota trenutnoStanje = PRACENJE_LINIJE;  // Postavljamo početno stanje robota.
unsigned long vrijemePocetkaStanja = 0;         // Pomoćna varijabla za mjerenje vremena u stanjima.
float zadnjePoznatoOdstupanje = 0;              // "Memorija" robota za oštre zavoje.

//==============================================================================
// FUNKCIJE NISKE RAZINE ZA UPRAVLJANJE DC MOTORIMA
//==============================================================================
// Ove funkcije direktno šalju signale motornom driveru za pokretanje pojedinog kotača.
void FR_fwd(int speed) {
  digitalWrite(RightMotorDirPin1, LOW);
  digitalWrite(RightMotorDirPin2, HIGH);
  analogWrite(speedPinR, speed);
}  // Prednji desni naprijed
void FR_bck(int speed) {
  digitalWrite(RightMotorDirPin1, HIGH);
  digitalWrite(RightMotorDirPin2, LOW);
  analogWrite(speedPinR, speed);
}  // Prednji desni nazad
void FL_fwd(int speed) {
  digitalWrite(LeftMotorDirPin1, LOW);
  digitalWrite(LeftMotorDirPin2, HIGH);
  analogWrite(speedPinL, speed);
}  // Prednji lijevi naprijed
void FL_bck(int speed) {
  digitalWrite(LeftMotorDirPin1, HIGH);
  digitalWrite(LeftMotorDirPin2, LOW);
  analogWrite(speedPinL, speed);
}  // Prednji lijevi nazad
void RR_fwd(int speed) {
  digitalWrite(RightMotorDirPin1B, LOW);
  digitalWrite(RightMotorDirPin2B, HIGH);
  analogWrite(speedPinRB, speed);
}  // Stražnji desni naprijed
void RR_bck(int speed) {
  digitalWrite(RightMotorDirPin1B, HIGH);
  digitalWrite(RightMotorDirPin2B, LOW);
  analogWrite(speedPinRB, speed);
}  // Stražnji desni nazad
void RL_fwd(int speed) {
  digitalWrite(LeftMotorDirPin1B, LOW);
  digitalWrite(LeftMotorDirPin2B, HIGH);
  analogWrite(speedPinLB, speed);
}  // Stražnji lijevi naprijed
void RL_bck(int speed) {
  digitalWrite(LeftMotorDirPin1B, HIGH);
  digitalWrite(LeftMotorDirPin2B, LOW);
  analogWrite(speedPinLB, speed);
}  // Stražnji lijevi nazad

//==============================================================================
// FUNKCIJE VISOKE RAZINE ZA KRETANJE
//==============================================================================
// Ove funkcije kombiniraju pokrete pojedinih kotača kako bi se dobili složeni pokreti cijelog robota.
// One su "sloj apstrakcije" - olakšavaju nam upravljanje robotom.

/** @brief Zaustavlja sve motore. */
void zaustavi() {
  analogWrite(speedPinLB, 0);
  analogWrite(speedPinRB, 0);
  analogWrite(speedPinL, 0);
  analogWrite(speedPinR, 0);
}

/** @brief Pokreće sve motore za kretanje ravno naprijed. */
void naprijed(int brzina) {
  brzina = abs(brzina);
  RL_fwd(brzina);
  RR_fwd(brzina);
  FR_fwd(brzina);
  FL_fwd(brzina);
}

/** @brief Rotira robota u mjestu udesno (u smjeru kazaljke na satu). */
void okreniDesno(int brzina) {
  brzina = abs(brzina);
  RL_fwd(brzina);
  RR_bck(brzina);
  FR_bck(brzina);
  FL_fwd(brzina);
}

/** @brief Rotira robota u mjestu ulijevo (suprotno od kazaljke na satu). */
void okreniLijevo(int brzina) {
  brzina = abs(brzina);
  RL_bck(brzina);
  RR_fwd(brzina);
  FR_fwd(brzina);
  FL_bck(brzina);
}

/** * @brief Glavna funkcija za diferencijalno upravljanje. 
 * Postavlja različite brzine za lijevu i desnu stranu robota, 
 * što je ključno za skretanje tijekom praćenja linije.
 */
void pokreniMotore(int brzinaLijevo, int brzinaDesno) {
Serial.print("BrzinaD: "); Serial.println(brzinaDesno);
Serial.print("BrzinaL: "); Serial.println(brzinaLijevo);

  if (brzinaDesno < 0) {
    RR_bck(abs(brzinaDesno));
    FR_bck(abs(brzinaDesno));
  } else {
    RR_fwd(brzinaDesno);
    FR_fwd(brzinaDesno);
  }
    if (brzinaLijevo < 0) {
    RL_bck(abs(brzinaLijevo));
    FL_bck(abs(brzinaLijevo));
  } else {
    RL_fwd(brzinaLijevo);
    FL_fwd(brzinaLijevo);
  }

}

//==============================================================================
// POMOĆNE I GLAVNE FUNKCIJE
//==============================================================================

/* @brief Provjerava jesu li svih 5 senzora na crnoj podlozi. */
bool jelSviSenzoriNaCrnom() {
  for (int i = 0; i < BROJ_SENZORA; i++) {
    if (ocitanjaSenzora[i] != 0) return false;
  }
  return true;
}



/** @brief Očitava stanje svih 5 IR senzora. */
void ocitajSenzore() {
  for (int i = 0; i < BROJ_SENZORA; i++) {
    // Korištena senzorska pločica daje HIGH(1) za crnu, a LOW(0) za bijelu.
    // Naša logika radi s 0=crno, 1=bijelo, pa moramo invertirati očitani signal s '!'.
    ocitanjaSenzora[i] = digitalRead(pinoviSenzora[i]);
    Serial.print(ocitanjaSenzora[i]);
  }
  Serial.println();
  delay(0);
}

/** * @brief "Mozak" P-regulatora. Izračunava ponderiranu poziciju linije.
 * Također sadrži logiku za oštre zavoje (pamćenje zadnje pozicije).
 */
float izracunajOdstupanje() {
  float tezinskaSuma = 0;
  int senzoraNaLiniji = 0;
  for (int i = 0; i < BROJ_SENZORA; i++) {
    if (ocitanjaSenzora[i] == 0) {  // Ako je senzor na crnoj liniji...
      tezinskaSuma += tezine[i];    // ...dodaj njegovu "težinu" u sumu.
      senzoraNaLiniji++;
    }
  }

  if (senzoraNaLiniji > 0) {  // Ako barem jedan senzor vidi liniju...
    float odstupanje = tezinskaSuma / senzoraNaLiniji;
    zadnjePoznatoOdstupanje = odstupanje;  // ...spremi trenutnu poziciju u "memoriju".
    return odstupanje;
  } else {  // Ako je robot izgubio liniju (svi senzori na bijelom)...
    // ...vrati ekstremnu vrijednost na temelju zadnje poznate pozicije
    // kako bi se robot oštro okrenuo i potražio liniju.
    if (zadnjePoznatoOdstupanje > 0) {
      return 30;
    } else {
      return -30;
    }
  }
}

/** * @brief Glavna funkcija za praćenje linije.
 * Poziva izračun odstupanja i na temelju njega upravlja motorima.
 */
void pratiLiniju() {
  float odstupanje = izracunajOdstupanje();
  Serial.print("Odstupanje: ");
  Serial.println(odstupanje);
  // 'skretanje' je vrijednost korekcije. Ako je pozitivna, treba skrenuti desno.
  float skretanje = Kp * odstupanje;
  Serial.print("Skretanje: ");
  Serial.println(skretanje);
  // Diferencijalno upravljanje: da bi skrenuo desno, uspori lijevu stranu i ubrzaj desnu.
  int brzinaLijevo = osnovnaBrzina + skretanje;
  int brzinaDesno = osnovnaBrzina - skretanje;
  Serial.print("Brzina L: ");
  Serial.println(brzinaLijevo);
  Serial.print("Brzina D: ");
  Serial.println(brzinaDesno);
  pokreniMotore(brzinaLijevo, brzinaDesno);
}
//==============================================================================
// SETUP FUNCIJA - Izvršava se jednom na početku programa
//==============================================================================
void setup() {
  Serial.begin(9600);  // Pokretanje serijske komunikacije za ispis poruka na računalo.
  Serial.println("Inicijalizacija: Zadatak 1, DC Robot...");

  // Postavljanje svih pinova za motore kao izlazne (OUTPUT).
  pinMode(RightMotorDirPin1, OUTPUT);
  pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT);
  pinMode(LeftMotorDirPin2, OUTPUT);
  pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT);
  pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT);
  pinMode(LeftMotorDirPin2B, OUTPUT);
  pinMode(speedPinRB, OUTPUT);

  // Postavljanje pinova IR senzora kao ulaznih (INPUT).
  for (int i = 0; i < BROJ_SENZORA; i++) {
    pinMode(pinoviSenzora[i], INPUT);
  }
  // Pin za tipkalo postavlja se kao ulaz s internim PULLUP otpornikom.
  pinMode(PIN_TIPKALO, INPUT_PULLUP);

  zaustavi();
  delay(500);  // Kratka pauza za stabilizaciju.
  Serial.println("Sustav spreman. Započinjem praćenje linije.");
}

//==============================================================================
// GLAVNA PETLJA (loop) - Srce programa, izvršava se neprestano
//==============================================================================
void loop() {
  ocitajSenzore();  // Na početku svakog ciklusa, provjeri stanje senzora.
  
  // Glavna "switch" petlja koja upravlja strojem stanja.
  // Ovisno o vrijednosti varijable 'trenutnoStanje', izvršava se odgovarajući dio koda.
  switch (trenutnoStanje) {
    case PRACENJE_LINIJE:
      pratiLiniju();
      // Provjera uvjeta za prijelaz u iduće stanje.
      if (jelSviSenzoriNaCrnom()) {
        Serial.println("-> Stanje: PROVJERA KRAJA");
        trenutnoStanje = PROVJERA_KRAJA;
        vrijemePocetkaStanja = millis();
      }
      break;
    case PROVJERA_KRAJA:
      naprijed(BRZINA_PROVJERE_KRAJA);
      if (millis() - vrijemePocetkaStanja > VRIJEME_PROVJERE_KRAJA_MS) {
        if (jelSviSenzoriNaCrnom()) {
          Serial.println("-> Stanje: CEKANJE NA TIPKU");
          zaustavi();
          trenutnoStanje = CEKANJE_NA_TIPKU;
        } else {
          Serial.println("Lažni alarm, vraćam se na praćenje.");
          trenutnoStanje = PRACENJE_LINIJE;
        }
      }
      break;
    case CEKANJE_NA_TIPKU:
      if (digitalRead(PIN_TIPKALO) == LOW) {  // Čeka se pritisak gumba (signal LOW).
        delay(50);                            // Debounce - kratka pauza protiv smetnji.
        Serial.println("-> Stanje: PORAVNANJE (KRETANJE)");
        trenutnoStanje = PORAVNANJE_KRETANJE;
      }
      break;
    case PORAVNANJE_KRETANJE:
      naprijed(BRZINA_PORAVNANJA_NAPRIJED);
      if (ocitanjaSenzora[2] == 0) {  // Ako središnji senzor vidi kosu liniju...
        Serial.println("-> Stanje: PORAVNANJE (OKRETANJE)");
        zaustavi();
        trenutnoStanje = PORAVNANJE_OKRETANJE;
      }
      break;
    case PORAVNANJE_OKRETANJE:
      okreniDesno(BRZINA_PORAVNANJA_OKRET);
      // Rotiraj se dok krajnji lijevi i krajnji desni senzor nisu istovremeno na liniji.
      if (ocitanjaSenzora[0] == 0 && ocitanjaSenzora[4] == 0) {
        Serial.println("-> Stanje: ZADATAK ZAVRSEN");
        zaustavi();
        trenutnoStanje = ZADATAK_ZAVRSEN;
      }
      break;
    case ZADATAK_ZAVRSEN:
      zaustavi();  // Za svaki slučaj, osiguraj da robot stoji.
      break;
  }
}
/*
void loop() {
  ocitajSenzore();
  float odstupanje = izracunajOdstupanje();
  Serial.print("Odstupanje ");
  Serial.println(odstupanje);
  delay(500);
}
*/
