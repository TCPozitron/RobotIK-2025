/********************************************************************************
 * Kompletan Arduino kod za upravljanje robotom s Mecanum kotačima
 * * Hardverske postavke:
 * - 4 continuous servo motora
 * - Motor 0 spojen na pin 2
 * - Motor 1 spojen na pin 3
 * - Motor 2 spojen na pin 4
 * - Motor 3 spojen na pin 5
 * * Specifikacije robota (prema korisniku):
 * - Raspored motora:
 * - Lijevi-Stražnji: motor[0]
 * - Desni-Stražnji: motor[1]
 * - Lijevi-Prednji:  motor[2]
 * - Desni-Prednji:  motor[3]
 * - Smjer okretanja:
 * - Lijevi motori: pozitivna brzina = naprijed, negativna = nazad
 * - Desni motori: pozitivna brzina = nazad, negativna = naprijed (invertirano)
 * ********************************************************************************/

// Uključivanje biblioteke za upravljanje servo motorima
#include <Servo.h>

// Definicija broja motora i pinova na koje su spojeni
const int NUM_MOTORS = 4;
const int motorPins[NUM_MOTORS] = {2, 3, 4, 5}; // motor[0]->pin 2, motor[1]->pin 3, itd.

// Kreiranje niza Servo objekata
Servo motors[NUM_MOTORS];


// ===================================================================
// RASPORED MOTORA PREMA VAŠIM SPECIFIKACIJAMA
// Mijenjajte samo brojeve ako je potrebno, ne i nazive.
const int MOTOR_BL = 0; // Lijevi-Stražnji je spojen kao motor[0]
const int MOTOR_FL = 2; // Lijevi-Prednji je spojen kao motor[2]
const int MOTOR_BR = 1; // Desni-Stražnji je spojen kao motor[1]
const int MOTOR_FR = 3; // Desni-Prednji je spojen kao motor[3]
// ===================================================================


void setup() {
  // Pokretanje serijske komunikacije za ispis poruka
  Serial.begin(9600);
  Serial.println("Inicijalizacija Mecanum pogona...");

  // Spajanje (attach) svakog motora na njegov pin pomoću petlje
  for (int i = 0; i < NUM_MOTORS; i++) {
    motors[i].attach(motorPins[i]);
  }
  
  // Osiguraj da svi motori stoje na početku
  stop(); 
  delay(500);
  
  Serial.println("Sustav je spreman za testiranje.");
}


/**
 * @brief Osnovna funkcija koja postavlja brzinu pojedinog motora.
 * @param motorIndex Indeks motora (0-3).
 * @param speed Željena brzina od -100 (puna brzina nazad) do 100 (puna brzina naprijed). 0 je stop.
 */
void setMotorSpeed(int motorIndex, int speed) {
  // Ograničavanje vrijednosti brzine na raspon od -100 do 100
  speed = constrain(speed, -100, 100);

  // Mapiranje vrijednosti brzine [-100, 100] na servo vrijednost [0, 180]
  int servoValue = map(speed, -100, 100, 0, 180);

  // Slanje komande motoru
  motors[motorIndex].write(servoValue);
}


// ===================================================================
// FUNKCIJE ZA KRETANJE (PRILAGOĐENE VAŠEM ROBOTU)
// ===================================================================

/**
 * @brief Zaustavlja sve motore.
 */
void stop() {
  setMotorSpeed(MOTOR_FL, 0);
  setMotorSpeed(MOTOR_FR, 0);
  setMotorSpeed(MOTOR_BL, 0);
  setMotorSpeed(MOTOR_BR, 0);
}

/**
 * @brief Kretanje naprijed.
 * @param speed Brzina od 0 do 100.
 */
void naprijed(int speed) {
  speed = abs(speed);
  // Lijevi idu naprijed s pozitivnom brzinom
  setMotorSpeed(MOTOR_FL, speed);
  setMotorSpeed(MOTOR_BL, speed);
  // Desni idu naprijed s negativnom brzinom (jer su invertirani)
  setMotorSpeed(MOTOR_FR, -speed);
  setMotorSpeed(MOTOR_BR, -speed);
}

/**
 * @brief Kretanje nazad.
 * @param speed Brzina od 0 do 100.
 */
void nazad(int speed) {
  speed = abs(speed);
  // Lijevi idu nazad s negativnom brzinom
  setMotorSpeed(MOTOR_FL, -speed);
  setMotorSpeed(MOTOR_BL, -speed);
  // Desni idu nazad s pozitivnom brzinom (jer su invertirani)
  setMotorSpeed(MOTOR_FR, speed);
  setMotorSpeed(MOTOR_BR, speed);
}

/**
 * @brief Klizanje (strafe) udesno.
 * @param speed Brzina od 0 do 100.
 */
void desno(int speed) {
  speed = abs(speed);
  setMotorSpeed(MOTOR_FL, speed);    // Lijevi prednji: naprijed
  setMotorSpeed(MOTOR_BL, -speed);   // Lijevi stražnji: nazad
  setMotorSpeed(MOTOR_FR, speed);    // Desni prednji: nazad (invertirano)
  setMotorSpeed(MOTOR_BR, -speed);   // Desni stražnji: naprijed (invertirano)
}

/**
 * @brief Klizanje (strafe) ulijevo.
 * @param speed Brzina od 0 do 100.
 */
void lijevo(int speed) {
  speed = abs(speed);
  setMotorSpeed(MOTOR_FL, -speed);   // Lijevi prednji: nazad
  setMotorSpeed(MOTOR_BL, speed);    // Lijevi stražnji: naprijed
  setMotorSpeed(MOTOR_FR, -speed);   // Desni prednji: naprijed (invertirano)
  setMotorSpeed(MOTOR_BR, speed);    // Desni stražnji: nazad (invertirano)
}


// ===================================================================
// GLAVNA PETLJA ZA TESTIRANJE
// ===================================================================
void loop() {
  int brzina = 80;    // Brzina kretanja za test (0-100)
  int pauza = 2000;   // Trajanje svakog pokreta u milisekundama
  int stop_pauza = 1000; // Pauza između pokreta

  Serial.println("\nTest: NAPRIJED");
  naprijed(brzina);
  delay(pauza);
  stop();
  delay(stop_pauza);

  Serial.println("Test: NAZAD");
  nazad(brzina);
  delay(pauza);
  stop();
  delay(stop_pauza);

  Serial.println("Test: DESNO");
  desno(brzina);
  delay(pauza);
  stop();
  delay(stop_pauza);

  Serial.println("Test: LIJEVO");
  lijevo(brzina);
  delay(pauza);
  stop();
  delay(stop_pauza);
  
  Serial.println("--- Kraj sekvence, ponavljam za 3 sekunde. ---");
  delay(3000);
}