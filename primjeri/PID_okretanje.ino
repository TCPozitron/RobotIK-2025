/*
 * PID_okretanje.ino
 *
 * SVRHA PROGRAMA:
 * Ovaj program demonstrira kako robot "zna" okrenuti se za točno određeni kut (npr. 90 stupnjeva).
 * Koristi IMU senzor (kompas) da sazna gdje gleda i PID regulator da odluči kako upravljati motorima.
 *
 * HARDVER:
 * 1. Arduino Mega (Mozak)
 * 2. SparkFun LSM9DS1 (Oči/Centar za ravnotežu) - spojen na SDA/SCL (I2C)
 * 3. Motor Driver (Mišići) - spojen na PWM pinove
 * 4. Tipkalo na pinu 2 (Okidač za start)
 *
 * KONCEPT (KAKO OVO RADI?):
 * Zamislite da ste vi robot. Žmirite. Netko vam kaže "Okreni se desno".
 * 1. Očitate kompas (IMU) -> Znate trenutni smjer.
 * 2. Izračunate CILJ -> Trenutni smjer + 90 stupnjeva.
 * 3. Izračunate GREŠKU -> Koliko sam daleko od cilja?
 * 4. Ako je greška velika -> Okreći se brzo.
 * 5. Kako se približavate cilju -> Usporite (da ne preletite).
 * 6. Kad je greška 0 -> Stani.
 */

#include <Wire.h>
#include <SparkFunLSM9DS1.h>

// =============================================================
// 1. POSTAVKE SENZORA (IMU)
// =============================================================
LSM9DS1 imu;

// [KALIBRACIJA] - Ovi brojevi ispravljaju tvorničke greške vašeg senzora.
// Učenici moraju ovdje upisati vrijednosti iz programa za kalibraciju!
float magBias[3] = { 305.0f, 185.5f, 1095.0f };

// Magnetska deklinacija (razlika između magnetskog i pravog sjevera).
// Za Hrvatsku je to cca 4.8 stupnjeva.
#define DECLINATION 4.8 

// =============================================================
// 2. POSTAVKE MOTORA (L298N ili sličan driver) - upisati ispravne pinove!!!
// =============================================================
// Motor A (Lijevi)
#define ENA 10  // Pin za brzinu (PWM - 0 do 255)
#define IN1 9   // Smjer 1
#define IN2 8   // Smjer 2
// Motor B (Desni)
#define ENB 5   // Pin za brzinu (PWM - 0 do 255)
#define IN3 7   // Smjer 1
#define IN4 6   // Smjer 2

// Minimalna brzina (PWM) potrebna da se motori uopće počnu vrtjeti.
// Ispod ove vrijednosti motori samo "cvile" ali nemaju snage.
#define MIN_BRZINA_MOTORA 45

// =============================================================
// 3. PID PARAMETRI (FINO PODEŠAVANJE)
// =============================================================
// Kp (Snaga): Koliko agresivno reagiramo na grešku?
// Veći broj = brži okret, ali rizik od "cimanja".
float Kp = 2.5;  

// Ki (Upornost): Ispravlja sitne greške na kraju.
// Ostaviti na 0 osim ako robot stalno staje na 89 stupnjeva.
float Ki = 0.0;  

// Kd (Kočnica): Sprječava prelijetanje cilja.
// Ako robot preleti 90 pa se mora vraćati, povećajte ovo.
float Kd = 0.5;  

// =============================================================
// 4. GLOBALNE VARIJABLE (MEMORIJA ROBOTA)
// =============================================================
float ciljaniKut = 0;      // Kamo želimo gledati?
float trenutniKut = 0;     // Kamo sada gledamo?
float nultaTocka = 0;      // Kut koji smo proglasili "nulom" na početku

// Varijable koje PID regulator mora pamtiti između ciklusa
float proslaGreska = 0;
float integral = 0;
unsigned long prosloVrijeme = 0;

// Tipkalo za pokretanje testa
#define PIN_TIPKALO 2

// =============================================================
// SETUP - POKREĆE SE SAMO JEDNOM
// =============================================================
void setup() {
  Serial.begin(115200); // Pokreni komunikaciju s računalom
  Wire.begin();         // Pokreni I2C komunikaciju sa senzorom

  // Postavi pinove motora kao izlaze (jer mi šaljemo struju njima)
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
  // Postavi pin tipkala kao ulaz (jer mi čitamo stanje)
  pinMode(PIN_TIPKALO, INPUT_PULLUP);

  Serial.println("Inicijalizacija IMU senzora...");
  if (imu.begin() == false) {
    Serial.println("GRESKA: IMU senzor nije pronaden! Provjeri zice.");
    while (1); // Blokiraj program zauvijek ako nema senzora
  }
  
  Serial.println("IMU spreman.");
  Serial.println("Postavi robota na pod i pritisni tipku za okret.");

  // Napravi prvo očitavanje da znamo gdje je "naprijed" u trenutku paljenja
  azurirajIMU();
  nultaTocka = izracunajSiroviKut(); 
}

// =============================================================
// LOOP - GLAVNI KRUG RAZMIŠLJANJA ROBOTA
// =============================================================
void loop() {
  // KORAK 1: Saznaj istinu o svijetu (Gdje sam?)
  azurirajIMU();
  trenutniKut = dohvatiRelativniKut(); 

  // KORAK 2: Slušaj naredbe (Je li pritisnuta tipka?)
  if (digitalRead(PIN_TIPKALO) == LOW) {
    // Postavi novi cilj: Tamo gdje sad gledam + 90 stupnjeva
    ciljaniKut = trenutniKut + 90.0;
    
    // Matematička korekcija: Ako je cilj 200 stupnjeva, to je zapravo -160
    // Držimo kutove u rasponu -180 do +180 radi lakše matematike.
    if (ciljaniKut > 180) ciljaniKut -= 360;
    
    Serial.print("NOVA NAREDBA: Okreni se na "); 
    Serial.println(ciljaniKut);
    
    // Resetiraj PID memoriju jer krećemo u novi pokret
    integral = 0;
    proslaGreska = 0; 
    delay(1000); // Pričekaj sekundu da maknemo prst s tipke
  }

  // KORAK 3: PID Mozak (Što trebam napraviti?)
  // Ova funkcija vraća broj od -255 do 255.
  // Pozitivan broj = Treba skrenuti desno.
  // Negativan broj = Treba skrenuti lijevo.
  // Veličina broja = Snaga motora (PWM).
  int snagaOkretanja = izracunajPID(ciljaniKut, trenutniKut);

  // KORAK 4: Akcija (Pokreni motore)
  // Ali samo ako nismo već stigli na cilj!
  
  // Provjera: Jesmo li dovoljno blizu? (unutar 2 stupnja greške)
  if (abs(ciljaniKut - trenutniKut) < 2.0) {
    zaustaviMotore(); // Stigli smo! Odmaraj.
    integral = 0;     // Zaboravi nakupljenu grešku
  } else {
    // Nismo stigli, primijeni silu koju je PID izračunao
    pokreniMotoreZaOkret(snagaOkretanja);
  }

  // Kratka pauza da procesor "prodiše" (stabilnost petlje)
  delay(10); 
}

// =============================================================
// FUNKCIJE - ALATI KOJE KORISTIMO
// =============================================================

// --- PID REGULATOR (Matematika upravljanja) ---
int izracunajPID(float cilj, float trenutno) {
  unsigned long sadasnjeVrijeme = millis();
  float protekloVrijeme = (sadasnjeVrijeme - prosloVrijeme) / 1000.0; // u sekundama
  prosloVrijeme = sadasnjeVrijeme;

  // 1. Izračunaj GREŠKU (Koliko sam daleko?)
  float greska = cilj - trenutno;

  // [Jako važno] Rješavanje problema kruga:
  // Ako sam na -170, a cilj je +170, neću se vrtjeti 340 stupnjeva uokolo!
  // Najkraći put je preko granice 180 (samo 20 stupnjeva).
  if (greska > 180) greska -= 360;
  if (greska < -180) greska += 360;

  // 2. P-član (Proporcionalni) - Glavna snaga
  float P = Kp * greska;

  // 3. I-član (Integralni) - Popravlja sitne greške na kraju
  // Zbrajamo grešku samo ako smo blizu cilja (npr. unutar 10 stupnjeva)
  if (abs(greska) < 10) {
      integral += greska * protekloVrijeme;
  }
  float I = Ki * integral;

  // 4. D-član (Derivativni) - Kočnica
  // Gleda kojom brzinom se greška smanjuje
  float brzinaPromjene = (greska - proslaGreska) / protekloVrijeme;
  float D = Kd * brzinaPromjene;
  
  proslaGreska = greska; // Zapamti za idući put

  // 5. Zbroji sve zajedno
  float izlaz = P + I + D;

  // Ograniči izlaz na ono što motori mogu (PWM je max 255)
  izlaz = constrain(izlaz, -255, 255);

  return (int)izlaz;
}

// --- UPRAVLJANJE MOTORIMA ---
void pokreniMotoreZaOkret(int pwm) {
  // pwm > 0 znači okret UDESNO (Lijevi naprijed, Desni nazad)
  // pwm < 0 znači okret ULIJEVO (Lijevi nazad, Desni naprijed)

  // Deadband (Mrtva zona): Ako PID kaže "snaga 5", motor se neće pomaknuti
  // zbog trenja i težine. Moramo ga "pogurati" na minimalnu brzinu.
  if (abs(pwm) < MIN_BRZINA_MOTORA && pwm != 0) {
      if (pwm > 0) {
          pwm = MIN_BRZINA_MOTORA;
      } else {
          pwm = -MIN_BRZINA_MOTORA;
      }
  }

  if (pwm > 0) { 
    // --- OKRET UDESNO ---
    // Lijevi motor ide NAPRIJED
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); 
    // Desni motor ide NAZAD
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); 
  } else {       
    // --- OKRET ULIJEVO ---
    // Lijevi motor ide NAZAD
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); 
    // Desni motor ide NAPRIJED
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); 
  }
  
  // Pošalji snagu na pinove (uvijek pozitivna vrijednost za analogWrite)
  analogWrite(ENA, abs(pwm));
  analogWrite(ENB, abs(pwm));
}

void zaustaviMotore() {
  // Ugasi sve
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// --- OČITAVANJE IMU SENZORA (Matematika kuta) ---
void azurirajIMU() {
  if (imu.magAvailable()) imu.readMag();
}

// Ova funkcija računa "apsolutni" kut prema Sjeveru
float izracunajSiroviKut() {
  // 1. Oduzmi grešku (bias) koju smo našli kalibracijom
  float mx = imu.mx - (int16_t)magBias[0];
  float my = imu.my - (int16_t)magBias[1];
  
  // 2. Trigonometrija (atan2) pretvara X i Y vektor u kut
  float heading;
  if (my == 0) heading = (mx < 0) ? 180.0 : 0;
  else heading = atan2(mx, my);
  
  // 3. Pretvori radijane u stupnjeve
  heading *= 180.0 / PI;
  
  // 4. Ispravi za geografski sjever
  heading -= DECLINATION;
  
  // 5. Osiguraj da je kut između 0 i 360
  if (heading < 0) heading += 360;
  if (heading > 360) heading -= 360;
  
  return heading;
}

// Ova funkcija vraća kut RELATIVNO na naš početni položaj
// To je ono što PID koristi. 0 = ravno, +90 = desno, -90 = lijevo
float dohvatiRelativniKut() {
  float sirovi = izracunajSiroviKut();
  float relativni = sirovi - nultaTocka; // Oduzmi početni offset
  
  // Normalizacija na -180 do 180 (da nemamo kuteve tipa 350, nego -10)
  if (relativni > 180) relativni -= 360;
  if (relativni < -180) relativni += 360;
  
  return relativni;
}
