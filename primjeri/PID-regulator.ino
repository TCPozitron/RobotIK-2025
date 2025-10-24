/********************************************************************************
 * @file    Zadatak 1 (Praćenje Linije - DC Motori)
 * @author  Ekipa 'Juraj Dobrila II'
 * @version 2.1 (PID Verzija)
 * @date    07.10.2025.
 *
 * @brief   Upravljački softver za autonomnog robota...
 * (Originalni opis...)
 * Ova verzija nadograđuje P-regulator na puni PID regulator
 * za stabilniju vožnju pri većim brzinama.
 *
 * @hardware
 * (Isti kao prije)
 ********************************************************************************/

//==============================================================================
// DEFINICIJE PINOVA (Hardverska postava)
//==============================================================================
// (Svi pinovi ostaju isti kao u vašem kodu)
// --- Pinovi za prednje motore ---
#define speedPinR 9
#define RightMotorDirPin1 24
#define RightMotorDirPin2 22
#define speedPinL 10
#define LeftMotorDirPin1 28
#define LeftMotorDirPin2 26

// --- Pinovi za stražnje motore ---
#define speedPinRB 11
#define RightMotorDirPin1B 6
#define RightMotorDirPin2B 5
#define speedPinLB 12
#define LeftMotorDirPin1B 8
#define LeftMotorDirPin2B 7

// --- Pinovi za 5-kanalni IR senzor ---
#define sensor1 A4
#define sensor2 A3
#define sensor3 A2
#define sensor4 A1
#define sensor5 A0

//==============================================================================
// GLAVNE POSTAVKE I PARAMETRI
//==============================================================================

const int BROJ_SENZORA = 5;
const int pinoviSenzora[BROJ_SENZORA] = { sensor1, sensor2, sensor3, sensor4, sensor5 };
const int tezine[BROJ_SENZORA] = { -6, -3, 0, 3, 6 };
const int PIN_TIPKALO = 15;

// --- Parametri za P-regulator (PID kontroler) ---
// OVE VRIJEDNOSTI JE POTREBNO EKSPERIMENTALNO PODESITI ZA OPTIMALAN RAD!
float Kp = 9;            // Proporcionalni koeficijent (Počnite tjunirati s ovim)
float Ki = 0.0;          // NOVO: Integralni koeficijent (Tjunirajte zadnje, s malim vrijednostima)
float Kd = 0.0;          // NOVO: Derivativni koeficijent (Tjunirajte drugo, za smirivanje oscilacija)

int osnovnaBrzina = 100; // Osnovna brzina kretanja (0-255).

// --- Parametri za posebne manevre ---
// (Isti kao prije)
const int VRIJEME_PROVJERE_KRAJA_MS = 300;
const int BRZINA_PROVJERE_KRAJA = 80;
const int BRZINA_PORAVNANJA_NAPRIJED = 80;
const int BRZINA_PORAVNANJA_OKRET = 70;

//==============================================================================
// DEFINICIJA STANJA ROBOTA
//==============================================================================
// (Isto kao prije)
enum StanjeRobota {
  PRACENJE_LINIJE,
  PROVJERA_KRAJA,
  CEKANJE_NA_TIPKU,
  PORAVNANJE_KRETANJE,
  PORAVNANJE_OKRETANJE,
  ZADATAK_ZAVRSEN
};

//==============================================================================
// GLOBALNE VARIJABLE
//==============================================================================
int ocitanjaSenzora[BROJ_SENZORA];
StanjeRobota trenutnoStanje = PRACENJE_LINIJE;
unsigned long vrijemePocetkaStanja = 0;
float zadnjePoznatoOdstupanje = 0;

// --- NOVE VARIJABLE ZA PID ---
float integral = 0;           // Zbroj grešaka za I-član
float prethodnoOdstupanje = 0; // Greška iz prošle petlje za D-član

//==============================================================================
// FUNKCIJE NISKE RAZINE ZA UPRAVLJANJE DC MOTORIMA
//==============================================================================
// (Sve funkcije FR_fwd, FL_fwd, RR_fwd, RL_fwd... ostaju ISTE)
void FR_fwd(int speed) {
  digitalWrite(RightMotorDirPin1, LOW);
  digitalWrite(RightMotorDirPin2, HIGH);
  analogWrite(speedPinR, speed);
}
void FR_bck(int speed) {
  digitalWrite(RightMotorDirPin1, HIGH);
  digitalWrite(RightMotorDirPin2, LOW);
  analogWrite(speedPinR, speed);
}
void FL_fwd(int speed) {
  digitalWrite(LeftMotorDirPin1, LOW);
  digitalWrite(LeftMotorDirPin2, HIGH);
  analogWrite(speedPinL, speed);
}
void FL_bck(int speed) {
  digitalWrite(LeftMotorDirPin1, HIGH);
  digitalWrite(LeftMotorDirPin2, LOW);
  analogWrite(speedPinL, speed);
}
void RR_fwd(int speed) {
  digitalWrite(RightMotorDirPin1B, LOW);
  digitalWrite(RightMotorDirPin2B, HIGH);
  analogWrite(speedPinRB, speed);
}
void RR_bck(int speed) {
  digitalWrite(RightMotorDirPin1B, HIGH);
  digitalWrite(RightMotorDirPin2B, LOW);
  analogWrite(speedPinRB, speed);
}
void RL_fwd(int speed) {
  digitalWrite(LeftMotorDirPin1B, LOW);
  digitalWrite(LeftMotorDirPin2B, HIGH);
  analogWrite(speedPinLB, speed);
}
void RL_bck(int speed) {
  digitalWrite(LeftMotorDirPin1B, HIGH);
  digitalWrite(LeftMotorDirPin2B, LOW);
  analogWrite(speedPinLB, speed);
}

//==============================================================================
// FUNKCIJE VISOKE RAZINE ZA KRETANJE
//==============================================================================
// (Sve funkcije 'zaustavi', 'naprijed', 'okreniDesno', 'okreniLijevo' ostaju ISTE)

void zaustavi() {
  analogWrite(speedPinLB, 0);
  analogWrite(speedPinRB, 0);
  analogWrite(speedPinL, 0);
  analogWrite(speedPinR, 0);
}

void naprijed(int brzina) {
  brzina = abs(brzina);
  RL_fwd(brzina);
  RR_fwd(brzina);
  FR_fwd(brzina);
  FL_fwd(brzina);
}

void okreniDesno(int brzina) {
  brzina = abs(brzina);
  RL_fwd(brzina);
  RR_bck(brzina);
  FR_bck(brzina);
  FL_fwd(brzina);
}

void okreniLijevo(int brzina) {
  brzina = abs(brzina);
  RL_bck(brzina);
  RR_fwd(brzina);
  FR_fwd(brzina);
  FL_bck(brzina);
}

// Funkcija 'pokreniMotore' ostaje ISTA
void pokreniMotore(int brzinaLijevo, int brzinaDesno) {
  // Serial.print("BrzinaD: "); Serial.println(brzinaDesno);
  // Serial.print("BrzinaL: "); Serial.println(brzinaLijevo);

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

// Funkcije 'jelSviSenzoriNaCrnom' i 'ocitajSenzore' ostaju ISTE
bool jelSviSenzoriNaCrnom() {
  for (int i = 0; i < BROJ_SENZORA; i++) {
    if (ocitanjaSenzora[i] != 0) return false;
  }
  return true;
}

void ocitajSenzore() {
  for (int i = 0; i < BROJ_SENZORA; i++) {
    ocitanjaSenzora[i] = digitalRead(pinoviSenzora[i]);
    // Serial.print(ocitanjaSenzora[i]); // Smanjiti ispis za brži loop
  }
  // Serial.println();
}

// Funkcija 'izracunajOdstupanje' ostaje ISTA
float izracunajOdstupanje() {
  float tezinskaSuma = 0;
  int senzoraNaLiniji = 0;
  for (int i = 0; i < BROJ_SENZORA; i++) {
    if (ocitanjaSenzora[i] == 0) {
      tezinskaSuma += tezine[i];
      senzoraNaLiniji++;
    }
  }

  if (senzoraNaLiniji > 0) {
    float odstupanje = tezinskaSuma / senzoraNaLiniji;
    zadnjePoznatoOdstupanje = odstupanje;
    return odstupanje;
  } else {
    if (zadnjePoznatoOdstupanje > 0) {
      return 30;
    } else {
      return -30;
    }
  }
}

//==============================================================================
// NADOGRAĐENA FUNKCIJA 'pratiLiniju' (Sada koristi PID)
//==============================================================================
void pratiLiniju() {
  // 1. P-ČLAN (Proporcionalni)
  // Izračunaj trenutnu grešku (koliko smo daleko od centra)
  float odstupanje = izracunajOdstupanje();

  // 2. I-ČLAN (Integralni)
  // Zbroji grešku tijekom vremena
  integral = integral + odstupanje;

  // --- RJEŠENJE ZA "WINDUP" (da integral ne "pobjesni") ---
  // Ako smo prešli preko centra (promjena predznaka), resetiraj integral
  if ((odstupanje > 0 && prethodnoOdstupanje < 0) || (odstupanje < 0 && prethodnoOdstupanje > 0)) {
    integral = 0;
  }
  // Također, ograniči ga na neki maksimum ("clamping")
  integral = constrain(integral, -500, 500); // Podesite ove vrijednosti po potrebi!

  // 3. D-ČLAN (Derivativni)
  // Izračunaj brzinu promjene greške (koliko brzo se približavamo/udaljavamo)
  float derivativ = odstupanje - prethodnoOdstupanje;

  // 4. SPREMI TRENUTNU GREŠKU ZA IDUĆI KRUG
  // Ono što je sad "odstupanje", u idućoj petlji bit će "prethodnoOdstupanje"
  prethodnoOdstupanje = odstupanje;

  // 5. KONAČNI IZRAČUN SKRETANJA (PUNI PID)
  // Zbroji sve tri komponente pomnožene s njihovim koeficijentima
  float skretanje = (Kp * odstupanje) + (Ki * integral) + (Kd * derivativ);

  // 6. PRIMIJENI NA MOTORE (isto kao prije)
  int brzinaLijevo = osnovnaBrzina + skretanje;
  int brzinaDesno = osnovnaBrzina - skretanje;

  pokreniMotore(brzinaLijevo, brzinaDesno);

  // --- DEBUG ISPIS (otkomentirajte po potrebi za tjuniranje) ---
  /*
  Serial.print("Odstup: "); Serial.print(odstupanje);
  Serial.print(" P: "); Serial.print(Kp * odstupanje);
  Serial.print(" I: "); Serial.print(Ki * integral);
  Serial.print(" D: "); Serial.print(Kd * derivativ);
  Serial.print(" Skret: "); Serial.println(skretanje);
  */
}

//==============================================================================
// SETUP FUNCIJA
//==============================================================================
// (Setup funkcija ostaje POTPUNO ISTA)
void setup() {
  Serial.begin(9600);
  Serial.println("Inicijalizacija: Zadatak 1, DC Robot (PID Verzija)...");

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

  for (int i = 0; i < BROJ_SENZORA; i++) {
    pinMode(pinoviSenzora[i], INPUT);
  }
  pinMode(PIN_TIPKALO, INPUT_PULLUP);

  zaustavi();
  delay(500);
  Serial.println("Sustav spreman. Započinjem praćenje linije.");
}

//==============================================================================
// GLAVNA PETLJA (loop)
//==============================================================================
// (Loop funkcija ostaje POTPUNO ISTA, jer smo svu logiku
//  promijenili unutar 'pratiLiniju()')
void loop() {
  ocitajSenzore();
  
  switch (trenutnoStanje) {
    case PRACENJE_LINIJE:
      pratiLiniju(); // Ova funkcija sada interno koristi PID
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
      if (digitalRead(PIN_TIPKALO) == LOW) {
        delay(50);
        Serial.println("-> Stanje: PORAVNANJE (KRETANJE)");
        trenutnoStanje = PORAVNANJE_KRETANJE;
      }
      break;
    case PORAVNANJE_KRETANJE:
      naprijed(BRZINA_PORAVNANJA_NAPRIJED);
      if (ocitanjaSenzora[2] == 0) {
        Serial.println("-> Stanje: PORAVNANJE (OKRETANJE)");
        zaustavi();
        trenutnoStanje = PORAVNANJE_OKRETANJE;
      }
      break;
    case PORAVNANJE_OKRETANJE:
      okreniDesno(BRZINA_PORAVNANJA_OKRET);
      if (ocitanjaSenzora[0] == 0 && ocitanjaSenzora[4] == 0) {
        Serial.println("-> Stanje: ZADATAK ZAVRSEN");
        zaustavi();
        trenutnoStanje = ZADATAK_ZAVRSEN;
      }
      break;
    case ZADATAK_ZAVRSEN:
      zaustavi();
      break;
  }
}