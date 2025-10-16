/********************************************************************************
 * @file    Mecanum_Robot_Prilagodjene_Naredbe.ino
 * @author  Tim Juraj Dobrila I (prilagodio AI Assistant)
 * @version 1.0
 * @date    16.10.2025.
 *
 * @brief   Ovaj kod upravlja Mecanum robotom s DC motorima putem Bluetootha.
 * Originalni kod proizvođača je prilagođen da koristi jednostavne,
 * jednoznakovne naredbe, identične onima iz primjera za ESP32.
 * Kompleksno parsiranje podataka je uklonjeno radi jednostavnosti.
 *
 * @upute
 * 1. Spojite HC-02 (ili sličan) Bluetooth modul na Serial1 (TX->18, RX->19).
 * 2. Učitajte ovaj kod na vašu Arduino Mega ploču.
 * 3. Spojite se na Bluetooth modul pomoću "Serial Bluetooth Terminal" aplikacije.
 * 4. Šaljite jednoznakovne naredbe za upravljanje.
 *
 * ### MAPA NAREDBI ###
 * 'F': Naprijed
 * 'B': Nazad
 * 'L': Okret u mjestu ulijevo
 * 'R': Okret u mjestu udesno
 * 'S': Stani
 * 'G': Dijagonalno naprijed-lijevo
 * 'I': Dijagonalno naprijed-desno
 * 'H': Dijagonalno nazad-lijevo
 * 'J': Dijagonalno nazad-desno
 *
 * Brzina (0-9, q):
 * '0'-'9': Postavlja brzinu od 0% do 90%
 * 'q': Postavlja maksimalnu brzinu (100%)
 ********************************************************************************/


//==============================================================================
// 1. HARDVERSKE POSTAVKE (preuzeto od proizvođača)
//==============================================================================
// Ovdje su sačuvane sve originalne definicije pinova i konstanti za brzine.

#define MAX_SPEED  150
#define MIN_SPEED  70
#define TURN_SPEED  120
#define SLOW_TURN_SPEED  80
#define BACK_SPEED  90

// Pinovi za prednje motore (spojeni na driver B)
#define speedPinR 9   //  PWM za prednji desni motor
#define RightMotorDirPin1  22
#define RightMotorDirPin2  24
#define LeftMotorDirPin1  26
#define LeftMotorDirPin2  28
#define speedPinL 10  //  PWM za prednji lijevi motor

// Pinovi za stražnje motore (spojeni na driver A)
#define speedPinRB 11  // PWM za stražnji desni motor
#define RightMotorDirPin1B  5
#define RightMotorDirPin2B 6
#define LeftMotorDirPin1B 7
#define LeftMotorDirPin2B 8
#define speedPinLB 12   // PWM za stražnji lijevi motor

// Globalna varijabla za brzinu kretanja
int move_speed = 150; // Početna brzina (0-255)


//==============================================================================
// 2. SETUP - Inicijalizacija Robota
//==============================================================================
void setup() {
  // Inicijalizacija svih pinova motora kao izlaznih
  init_GPIO();
  
  // Pokretanje serijske komunikacije preko USB-a za debugging
  Serial.begin(9600);
  
  // Pokretanje serijske komunikacije za Bluetooth modul na portu Serial1
  Serial1.begin(9600);
 
  Serial.println("--- Mecanum Robot Spreman ---");
  Serial.println("Ceka na naredbe preko Bluetootha...");

  // Osiguraj da svi motori stoje na početku
  stop_stop();
}

//==============================================================================
// 3. GLAVNA PETLJA (LOOP) - Novi, jednostavniji "Mozak"
//==============================================================================
void loop() {
  // Provjeri je li stigao podatak preko Bluetootha (Serial1)
  if (Serial1.available()) {
    // Pročitaj primljeni znak
    char naredba = Serial1.read();
    
    // Ispiši primljenu naredbu na USB Serial Monitor radi provjere
    Serial.print("Primljena naredba: ");
    Serial.println(naredba);

    // SWITCH NAREDBA - Odabir akcije na temelju primljenog znaka
    switch (naredba) {
      // Naredbe za kretanje (prema ESP32 kodu)
      case 'F': go_advance(move_speed, move_speed); break;
      case 'B': go_back(move_speed, move_speed); break;
      case 'L': left_turn(TURN_SPEED); break;
      case 'R': right_turn(TURN_SPEED); break;
      case 'S': stop_stop(); break;
      
      // Dijagonalne kretnje (prilagođeno mapiranje)
      // ESP32 'G' (lijevo naprijed) -> Mecanum dijagonala lijevo naprijed
      case 'G': left_shift(move_speed, 0, move_speed, 0); break; 
      // ESP32 'I' (desno naprijed) -> Mecanum dijagonala desno naprijed
      case 'I': right_shift(0, move_speed, 0, move_speed); break;
      // ESP32 'H' (lijevo natrag) -> Mecanum dijagonala lijevo natrag
      case 'H': left_shift(0, move_speed, 0, move_speed); break;
      // ESP32 'J' (desno natrag) -> Mecanum dijagonala desno natrag
      case 'J': right_shift(move_speed, 0, move_speed, 0); break;

      // Naredbe za postavljanje brzine (0-255)
      case '0': move_speed = 0; break;
      case '1': move_speed = 50; break;
      case '2': move_speed = 75; break;
      case '3': move_speed = 100; break;
      case '4': move_speed = 125; break;
      case '5': move_speed = 150; break;
      case '6': move_speed = 175; break;
      case '7': move_speed = 200; break;
      case '8': move_speed = 225; break;
      case '9': move_speed = 250; break;
      case 'q': move_speed = 255; break;
    }
  }
}


//==============================================================================
// 4. NISKA RAZINA - Upravljanje Motorima (sačuvano od proizvođača)
//==============================================================================
// Sve funkcije ispod su originalne funkcije proizvođača za kontrolu
// Mecanum kotača. One ostaju nepromijenjene.

/* Inicijalizacija pinova */
void init_GPIO() {
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
}

/* Funkcije za složene pokrete */
void right_shift(int speed_fl_fwd, int speed_rl_bck, int speed_rr_fwd, int speed_fr_bck) {
  FL_fwd(speed_fl_fwd); 
  RL_bck(speed_rl_bck); 
  RR_fwd(speed_rr_fwd);
  FR_bck(speed_fr_bck);
}
void left_shift(int speed_fl_bck, int speed_rl_fwd, int speed_rr_bck, int speed_fr_fwd){
   FL_bck(speed_fl_bck);
   RL_fwd(speed_rl_fwd);
   RR_bck(speed_rr_bck);
   FR_fwd(speed_fr_fwd);
}
void go_advance(int left_speed, int right_speed){
   RL_fwd(left_speed);
   RR_fwd(right_speed);
   FR_fwd(right_speed);
   FL_fwd(left_speed); 
}
void go_back(int left_speed, int right_speed){
   RL_bck(left_speed);
   RR_bck(right_speed);
   FR_bck(right_speed);
   FL_bck(left_speed); 
}
void left_turn(int speed){
   RL_bck(speed);
   RR_fwd(speed);
   FR_fwd(speed);
   FL_bck(speed); 
}
void right_turn(int speed){
   RL_fwd(speed);
   RR_bck(speed);
   FR_bck(speed);
   FL_fwd(speed); 
}
void clockwise(int speed){
   RL_fwd(speed);
   RR_bck(speed);
   FR_bck(speed);
   FL_fwd(speed); 
}
void countclockwise(int speed){
   RL_bck(speed);
   RR_fwd(speed);
   FR_fwd(speed);
   FL_bck(speed); 
}

/* Funkcije za kontrolu pojedinih kotača */
void FR_fwd(int speed) { //front-right wheel forward turn
  digitalWrite(RightMotorDirPin1,LOW);
  digitalWrite(RightMotorDirPin2,HIGH); 
  analogWrite(speedPinR,speed);
}
void FR_bck(int speed) { // front-right wheel backward turn
  digitalWrite(RightMotorDirPin1,HIGH);
  digitalWrite(RightMotorDirPin2,LOW); 
  analogWrite(speedPinR,speed);
}
void FL_fwd(int speed) { // front-left wheel forward turn
  digitalWrite(LeftMotorDirPin1,LOW);
  digitalWrite(LeftMotorDirPin2,HIGH);
  analogWrite(speedPinL,speed);
}
void FL_bck(int speed) { // front-left wheel backward turn
  digitalWrite(LeftMotorDirPin1,HIGH);
  digitalWrite(LeftMotorDirPin2,LOW);
  analogWrite(speedPinL,speed);
}
void RR_fwd(int speed) { //rear-right wheel forward turn
  digitalWrite(RightMotorDirPin1B, LOW);
  digitalWrite(RightMotorDirPin2B,HIGH); 
  analogWrite(speedPinRB,speed);
}
void RR_bck(int speed) { //rear-right wheel backward turn
  digitalWrite(RightMotorDirPin1B, HIGH);
  digitalWrite(RightMotorDirPin2B,LOW); 
  analogWrite(speedPinRB,speed);
}
void RL_fwd(int speed) { //rear-left wheel forward turn
  digitalWrite(LeftMotorDirPin1B,LOW);
  digitalWrite(LeftMotorDirPin2B,HIGH);
  analogWrite(speedPinLB,speed);
}
void RL_bck(int speed) { //rear-left wheel backward turn
  digitalWrite(LeftMotorDirPin1B,HIGH);
  digitalWrite(LeftMotorDirPin2B,LOW);
  analogWrite(speedPinLB,speed);
}
void stop_stop() { //Stop
 analogWrite(speedPinLB,0);
 analogWrite(speedPinRB,0);
 analogWrite(speedPinL,0);
 analogWrite(speedPinR,0);
}