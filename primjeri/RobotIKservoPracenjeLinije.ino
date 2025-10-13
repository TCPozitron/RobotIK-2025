/********************************************************************************

 * @file    Zadatak 1 (Praćenje Linije - DC Motori)

 * @author  Ekipa 'Juraj Dobrila I'

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

 * - Pogonski sustav: Robot s 4 continous servo motora

 * - Senzori: 5-kanalni infracrveni (IR) senzor za detekciju linije

 * - Ulaz: 1x Tipkalo za interakciju korisnika

 ********************************************************************************/



//==============================================================================

// UKLJUČIVANJE BIBLIOTEKA

//==============================================================================

#include <Servo.h>


//==============================================================================

// GLOBALNE KONSTANTE I POSTAVKE

//==============================================================================


// --- Postavke motora ---

const int BROJ_MOTORA = 4;

const int pinoviMotora[BROJ_MOTORA] = {2, 3, 4, 5};

const int MOTOR_LS = 0, MOTOR_LP = 2, MOTOR_DS = 1, MOTOR_DP = 3;


// --- Postavke senzora i tipkala ---

const int BROJ_SENZORA = 5;

const int pinoviSenzora[BROJ_SENZORA] = {28, 29, 30, 31, 32}; 

const int tezine[BROJ_SENZORA] = {-12, -4, 0, 4, 12}; 

const int PIN_TIPKALO = 15;


// --- Parametri za P-regulator ---

float Kp = 35;

int osnovnaBrzina = 80;


// --- Parametri za specifične akcije ---

const int VRIJEME_PROVJERE_KRAJA_MS = 300;

const int BRZINA_PROVJERE_KRAJA = 40;

const int BRZINA_PORAVNANJA_NAPRIJED = 40;

const int BRZINA_PORAVNANJA_OKRET = 30;


//==============================================================================

// DEFINICIJA STANJA ROBOTA (STATE MACHINE)

//==============================================================================

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

Servo motori[BROJ_MOTORA];

int ocitanjaSenzora[BROJ_SENZORA];

StanjeRobota trenutnoStanje = PRACENJE_LINIJE;

unsigned long vrijemePocetkaStanja = 0;

float zadnjePoznatoOdstupanje = 0;


//==============================================================================

// SETUP FUNCIJA

//==============================================================================

void setup() {

  Serial.begin(9600);

  Serial.println("Inicijalizacija");


  for (int i = 0; i < BROJ_MOTORA; i++) {

    motori[i].attach(pinoviMotora[i]);

  }


  for (int i = 0; i < BROJ_SENZORA; i++) {

    pinMode(pinoviSenzora[i], INPUT);

  }

  

  pinMode(PIN_TIPKALO, INPUT_PULLUP);

  

  zaustavi(); 
  delay(100);
  naprijed(60);
  delay(1000);
  zaustavi();
  delay(500);

  

  Serial.println("Sustav spreman. Započinjem praćenje linije.");

}



//==============================================================================

// GLAVNA PETLJA - Upravlja strojem stanja

//==============================================================================

void loop() {

  ocitajSenzore();


  switch (trenutnoStanje) {

    

    case PRACENJE_LINIJE:

      pratiLiniju();

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

      okreniLijevo(BRZINA_PORAVNANJA_OKRET);

      if (ocitanjaSenzora[0] == 0 && ocitanjaSenzora[4] == 0) {

        Serial.println("-> Stanje: ZADATAK ZAVRSEN");

        zaustavi();

        trenutnoStanje = ZADATAK_ZAVRSEN;

      }

      break;


    case ZADATAK_ZAVRSEN:

      // Ne radi ništa.

      break;

  }

}



//==============================================================================

// POMOĆNE I GLAVNE FUNKCIJE

//==============================================================================


bool jelSviSenzoriNaCrnom() {

  for (int i = 0; i < BROJ_SENZORA; i++) {

    if (ocitanjaSenzora[i] != 0) {

      return false;

    }

  }

  return true;

}


void pratiLiniju() {

  float odstupanje = izracunajOdstupanje();

  float skretanje = Kp * odstupanje;


  int brzinaLP = osnovnaBrzina + skretanje;

  int brzinaLS = osnovnaBrzina + skretanje;

  int brzinaDP = -osnovnaBrzina + skretanje;

  int brzinaDS = -osnovnaBrzina + skretanje;


  postaviBrzinuMotora(MOTOR_LP, brzinaLP);

  postaviBrzinuMotora(MOTOR_LS, brzinaLS);

  postaviBrzinuMotora(MOTOR_DP, brzinaDP);

  postaviBrzinuMotora(MOTOR_DS, brzinaDS);

}


void ocitajSenzore() {

  for (int i = 0; i < BROJ_SENZORA; i++) {

    ocitanjaSenzora[i] = digitalRead(pinoviSenzora[i]);

  }

}


float izracunajOdstupanje() {

  float tezinskaSuma = 0;

  int senzoraNaLiniji = 0;


  for (int i = 0; i < BROJ_SENZORA; i++) {

    if (ocitanjaSenzora[i] == 0) { // Ako je senzor na crnoj liniji

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

    if (zadnjePoznatoOdstupanje > 0) { // Linija je zadnje bila DESNO

      return 5;  // Vrati veliku pozitivnu vrijednost za oštro skretanje UDESNO

    } else { // Linija je zadnje bila LIJEVO (ili točno u centru)

      return -5; // Vrati veliku negativnu vrijednost za oštro skretanje ULIJEVO

    }

  }

}



//==============================================================================

// OSNOVNE FUNKCIJE ZA UPRAVLJANJE MOTORIMA I KRETANJE

//==============================================================================


void postaviBrzinuMotora(int indeksMotora, int brzina) {

  brzina = constrain(brzina, -100, 100);

  int servoVrijednost = map(brzina, -100, 100, 0, 180);

  motori[indeksMotora].write(servoVrijednost);

}


void zaustavi() {

  for (int i = 0; i < BROJ_MOTORA; i++) { postaviBrzinuMotora(i, 0); }

}


void naprijed(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, brzina); postaviBrzinuMotora(MOTOR_LS, brzina);

  postaviBrzinuMotora(MOTOR_DP, -brzina); postaviBrzinuMotora(MOTOR_DS, -brzina);

}


void nazad(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, -brzina); postaviBrzinuMotora(MOTOR_LS, -brzina);

  postaviBrzinuMotora(MOTOR_DP, brzina); postaviBrzinuMotora(MOTOR_DS, brzina);

}


void desno(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, brzina); postaviBrzinuMotora(MOTOR_LS, -brzina);

  postaviBrzinuMotora(MOTOR_DP, brzina); postaviBrzinuMotora(MOTOR_DS, -brzina);

}


void lijevo(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, -brzina); postaviBrzinuMotora(MOTOR_LS, brzina);

  postaviBrzinuMotora(MOTOR_DP, -brzina); postaviBrzinuMotora(MOTOR_DS, brzina);

}


void okreniDesno(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, brzina); postaviBrzinuMotora(MOTOR_LS, brzina);

  postaviBrzinuMotora(MOTOR_DP, brzina); postaviBrzinuMotora(MOTOR_DS, brzina);

}


void okreniLijevo(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, -brzina); postaviBrzinuMotora(MOTOR_LS, -brzina);

  postaviBrzinuMotora(MOTOR_DP, -brzina); postaviBrzinuMotora(MOTOR_DS, -brzina);

}


void naprijedDesno(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, brzina); postaviBrzinuMotora(MOTOR_DS, -brzina);

  postaviBrzinuMotora(MOTOR_DP, 0); postaviBrzinuMotora(MOTOR_LS, 0);

}


void naprijedLijevo(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_DP, -brzina); postaviBrzinuMotora(MOTOR_LS, brzina);

  postaviBrzinuMotora(MOTOR_LP, 0); postaviBrzinuMotora(MOTOR_DS, 0);

}


void nazadDesno(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_DP, brzina); postaviBrzinuMotora(MOTOR_LS, -brzina);

  postaviBrzinuMotora(MOTOR_LP, 0); postaviBrzinuMotora(MOTOR_DS, 0);

}


void nazadLijevo(int brzina) {

  brzina = abs(brzina);

  postaviBrzinuMotora(MOTOR_LP, -brzina); postaviBrzinuMotora(MOTOR_DS, brzina);

  postaviBrzinuMotora(MOTOR_DP, 0); postaviBrzinuMotora(MOTOR_LS, 0);

}
