/*
 * IMU_Yaw_Reset_Demo.ino
 * * Demonstracija očitavanja Yaw (Heading) kuta s LSM9DS1 senzora
 * s mogućnošću resetiranja "nulte točke" pritiskom na tipkalo.
 *
 * HARDVER:
 * - Arduino Mega (ili bilo koji Arduino)
 * - SparkFun LSM9DS1 IMU (spojen na SDA/SCL)
 * - Tipkalo spojeno na PIN 2 i GND
 */

#include <Wire.h>
#include <SparkFunLSM9DS1.h>

// --- POSTAVKE IMU SENZORA ---
LSM9DS1 imu;

// [VAŽNO] Ovdje zalijepite rezultate vaše kalibracije magnetometra!
// (Ovo su primjeri, vaši će biti drugačiji)
float magBias[3] = { 305.0f, 185.5f, 1095.0f }; 

// Deklinacija za vašu lokaciju (Pazin, Istra = cca 4.8 stupnjeva)
// Ovo služi da "Sjever" bude pravi geografski sjever, a ne magnetski.
#define DECLINATION 4.8 

// --- POSTAVKE TIPKALA I LOGIKE ---
#define PIN_TIPKALO 2   // Tipkalo spojeno na Pin 2 i GND
float headingOffset = 0; // Varijabla koja pamti "pomak" za resetiranje

// Postavke ispisa (da ne spamamo Serial Monitor prebrzo)
unsigned long lastPrint = 0;
#define PRINT_SPEED 100 // Ispis svakih 100ms

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Postavi pin za tipkalo (INPUT_PULLUP znači da ne trebamo vanjski otpornik)
  pinMode(PIN_TIPKALO, INPUT_PULLUP);

  Serial.println("Inicijalizacija IMU senzora...");
  
  if (imu.begin() == false) {
    Serial.println("GRESKA: Nisam pronasao LSM9DS1 senzor. Provjeri zice!");
    while (1); // Stani zauvijek
  }

  Serial.println("IMU spreman. Kalibracija magnetometra primijenjena.");
  Serial.println("Pritisni tipku za postavljanje kuta na 0.");
}

void loop() {
  // 1. Ažuriraj podatke sa senzora (ako su dostupni)
  // Magnetometar je najvažniji za Heading (Smjer)
  if (imu.magAvailable()) {
    imu.readMag();
  }
  if (imu.accelAvailable()) {
    imu.readAccel();
  }
  if (imu.gyroAvailable()) {
    imu.readGyro();
  }

  // 2. Primijeni kalibraciju magnetometra (ručno oduzimanje biasa)
  // Moramo vratiti bias u 'sirovi' format (int16_t) jer biblioteka tako radi
  imu.mx -= (int16_t)magBias[0];
  imu.my -= (int16_t)magBias[1];
  imu.mz -= (int16_t)magBias[2];

  // 3. Izračunaj SIROVI Heading (stvarni kut prema sjeveru)
  float rawHeading = calculateHeading(imu.mx, imu.my, imu.mz);

  // 4. Provjeri je li pritisnuto tipkalo za resetiranje
  // (LOW znači pritisnuto jer koristimo INPUT_PULLUP)
  if (digitalRead(PIN_TIPKALO) == LOW) {
    // Spremi trenutni stvarni kut kao "pomak"
    headingOffset = rawHeading;
    Serial.println("--- KUT RESETIRAN NA 0! ---");
    delay(500); // Mali delay da ne detektira višestruke pritiske (debounce)
  }

  // 5. Izračunaj RELATIVNI Heading (Korisnički kut)
  // Ovo je ono što nas zanima: Kut u odnosu na trenutak pritiska tipke
  float relativeHeading = rawHeading - headingOffset;

  // Normalizacija kuta (da bude uvijek između -180 i +180 stupnjeva)
  if (relativeHeading > 180.0) relativeHeading -= 360.0;
  if (relativeHeading < -180.0) relativeHeading += 360.0;


  // 6. Ispis rezultata
  if (millis() - lastPrint > PRINT_SPEED) {
    Serial.print("Stvarni Sjever: ");
    Serial.print(rawHeading, 1);
    Serial.print(" deg \t | \t MOJ KUT: ");
    Serial.print(relativeHeading, 1);
    Serial.println(" deg");
    
    lastPrint = millis();
  }
}

// --- Funkcija za izračun Headinga (Matematika) ---
// Ovo je standardna trigonometrija za kompas s tilt-kompenzacijom (ako bi koristili i akcelerometar)
// Ovdje koristimo jednostavniju verziju (bez tilt-kompenzacije) koja radi ako je robot ravan na podu.
float calculateHeading(float mx, float my, float mz) {
  float heading;

  // Osnovna trigonometrija: kut je arctan(y/x)
  if (my == 0)
    heading = (mx < 0) ? 180.0 : 0;
  else
    heading = atan2(mx, my);

  // Pretvori radijane u stupnjeve
  heading *= 180.0 / PI;

  // Dodaj magnetsku deklinaciju (korekcija za pravi sjever)
  heading -= DECLINATION;

  // Normaliziraj na 0-360
  if (heading < 0) heading += 360.0;
  if (heading > 360) heading -= 360.0;

  return heading;
}
