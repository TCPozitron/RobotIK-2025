// 1. Uključujemo NOVU SparkFun biblioteku
#include <SparkFunLSM9DS1.h>
#include <Wire.h>

// 2. Kreiramo objekt iz SparkFun klase
LSM9DS1 imu;

// Postavke kalibracije
#define CALIBRATION_TIME 15000 // Skupljaj podatke 15 sekundi

// Varijable za praćenje min/max vrijednosti
int16_t magMin[3] = {32767, 32767, 32767};
int16_t magMax[3] = {-32767, -32767, -32767};

// Varijable za spremanje izračunatih pogrešaka (bias)
float magBias[3];

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // 3. Koristimo .begin() iz SparkFun biblioteke
  if (imu.begin() == false) {
    Serial.println("Greska u komunikaciji s LSM9DS1. Provjerite zice.");
    while (1);
  }
  
  // Ovo je važno: postavi skalu magnetometra
  // (mora odgovarati onoj u glavnom kodu)
  imu.setMagScale(4); // Postavljamo na +/- 4 Gaussa

  Serial.println("Krecem s kalibracijom magnetometra za 3 sekunde...");
  delay(3000);
  
  Serial.println("====================================================");
  Serial.println("KRENITE! Polako rotirajte senzor u svim smjerovima (osmica).");
  Serial.println("Imate 15 sekundi...");
  Serial.println("====================================================");

  unsigned long startTime = millis();
  while ((millis() - startTime) < CALIBRATION_TIME) {
    if (imu.magAvailable()) {
      imu.readMag(); // Očitaj podatke

      // Ažuriraj minimalne vrijednosti
      if (imu.mx < magMin[0]) magMin[0] = imu.mx;
      if (imu.my < magMin[1]) magMin[1] = imu.my;
      if (imu.mz < magMin[2]) magMin[2] = imu.mz;

      // Ažuriraj maksimalne vrijednosti
      if (imu.mx > magMax[0]) magMax[0] = imu.mx;
      if (imu.my > magMax[1]) magMax[1] = imu.my;
      if (imu.mz > magMax[2]) magMax[2] = imu.mz;
    }
  }

  Serial.println("...Kalibracija GOTOVA!");

  // Izračunaj bias (pogrešku)
  // Bias je točno na pola puta između minimuma i maksimuma
  magBias[0] = (float)(magMax[0] + magMin[0]) / 2.0;
  magBias[1] = (float)(magMax[1] + magMin[1]) / 2.0;
  magBias[2] = (float)(magMax[2] + magMin[2]) / 2.0;

  Serial.println("====================================================");
  Serial.println("Kopirajte sljedecu liniju koda u vas glavni sketch:");
  Serial.println();
  
  // Ispis u formatu koji se lako kopira
  // Napomena: SparkFun funkcija `calcMag` interno koristi ove sirove (RAW)
  // vrijednosti, tako da ih ne moramo pretvarati u Gausse.
  Serial.print("float magBias[3] = { ");
  Serial.print(magBias[0], 6);
  Serial.print("f, ");
  Serial.print(magBias[1], 6);
  Serial.print("f, ");
  Serial.print(magBias[2], 6);
  Serial.println("f };");
  Serial.println("====================================================");
}

void loop() {
  // Ništa
}
