/******************************************************************************
 *
 * PROJEKT: STABILNA ORIJENTACIJA ROBOTA (HEADNING)
 *
 * @file        LSM9DS1_Orijentacija_Robota.ino
 * @brief       Edukativni kod za Arduino Mega pločicu.
 *
 * @description
 * Ovaj program koristi SVA TRI senzora unutar LSM9DS1 čipa (akcelerometar,
 * giroskop i magnetometar) kako bi izračunao stabilan kut orijentacije
 * oko Z-osi (yaw / heading). Koristi algoritam senzorske fuzije kako bi
 * eliminirao drift giroskopa. Rezultat je pouzdan "kompas" za robota.
 *
 * SVE UPUTE ZA RAD NALAZE SE U OVOM KODU KAO KOMENTARI.
 *
 ******************************************************************************
 *
 * UPUTE ZA UČENIKE - KAKO POKRENUTI PROJEKT
 *
 ******************************************************************************
 *
 * KORAK 1: POTREBNE KOMPONENTE
 * 1. Arduino Mega 2560 pločica
 * 2. LSM9DS1 9-osni senzor (na modulu/pločici)
 * 3. Spojne žice (4 komada)
 *
 ******************************************************************************
 *
 * KORAK 2: SPAJANJE ŽICA (Isto kao i prije)
 *
 * Senzor se spaja na I2C pinove Arduino Mega ploče.
 *
 * +----------------+--------------------+
 * | PIN NA SENZORU | ARDUINO MEGA PIN   |
 * +----------------+--------------------+
 * | VCC (ili 5V)   | 5V                 |
 * | GND            | GND                |
 * | SDA            | 20                 |
 * | SCL            | 21                 |
 * +----------------+--------------------+
 *
 ******************************************************************************
 *
 * KORAK 3: INSTALACIJA SVIH POTREBNIH BIBLIOTEKA
 *
 * Za ovaj napredni pristup, trebamo nekoliko biblioteka koje rade zajedno.
 * Sve se instaliraju na isti način:
 * Sketch > Include Library > Manage Libraries...
 *
 * Potrebno je instalirati SVE ČETIRI biblioteke:
 * 1. Adafruit LSM9DS1 Library (od Adafruit)
 * 2. Adafruit Sensor Fusion (od Adafruit)
 * 3. Adafruit Unified Sensor (od Adafruit) -> ovo je ovisnost (dependency)
 * 4. Adafruit BusIO (od Adafruit) -> ovo je ovisnost (dependency)
 *
 ******************************************************************************
 *
 * KORAK 4: KALIBRACIJA MAGNETOMETRA - NAJVAŽNIJI KORAK!
 *
 * Magnetometar (kompas) je osjetljiv na metal i magnetska polja u okolini.
 * Da bi davao točne rezultate, MORAMO ga kalibrirati.
 *
 * KAKO KALIBRIRATI:
 * 1. U Arduino IDE, idite na: File > Examples > Adafruit LSM9DS1 Library > calib
 * 2. Učitajte taj "calib" primjer na vaš Arduino.
 * 3. Otvorite Serial Plotter (Tools > Serial Plotter).
 * 4. Polako okrećite senzor u svim smjerovima, iscrtavajući "osmice" u zraku.
 * Činite to oko minutu dok se vrijednosti na grafu ne stabiliziraju.
 * 5. Prebacite se na Serial Monitor. Tamo će biti ispisane minimalne i
 * maksimalne vrijednosti za X, Y i Z os magnetometra.
 * 6. ZAPIŠITE te vrijednosti. Trebat će vam za korak 5.
 * Izgledat će otprilike ovako:
 * Mag Min: -60.89, -79.94, -66.00
 * Mag Max: 49.92, 34.41, 41.54
 *
 ******************************************************************************
 *
 * KORAK 5: UČITAVANJE GLAVNOG KODA (ovog koda)
 *
 * 1. U kodu ispod, pronađite sekciju "--- UNESITE VRIJEDNOSTI KALIBRACIJE OVDJE ---".
 * 2. Unesite minimalne i maksimalne vrijednosti koje ste zapisali u koraku 4.
 * 3. Učitajte ovaj kod na vašu Arduino Mega pločicu.
 * 4. Otvorite Serial Monitor (brzina 9600).
 * 5. Vidjet ćete stabilan kut od 0 do 360 stupnjeva koji se ne mijenja
 * kada senzor miruje!
 *
 ******************************************************************************/


// ------- POČETAK KODA -------

// UKLJUČIVANJE SVIH POTREBNIH BIBLIOTEKA
#include <Wire.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor_Fusion.h>
#include <Adafruit_Sensor.h>

// --- UNESITE VRIJEDNOSTI KALIBRACIJE OVDJE ---
// Zamijenite ove primjere s vrijednostima koje ste dobili u KORAKU 4.
// Format je: {X_MIN, Y_MIN, Z_MIN, X_MAX, Y_MAX, Z_MAX}
float mag_min[3] = {-60.89, -79.94, -66.00};
float mag_max[3] = {49.92,  34.41,  41.54};

// Stvaranje objekta za senzor.
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();

// Pomoćni objekti za pristup pojedinačnim senzorima unutar čipa.
Adafruit_Sensor *accelerometer, *magnetometer, *gyroscope;

// Filter za senzorsku fuziju. On će raditi svu tešku matematiku.
// Koristimo Mahony filter, koji je brz i dobar za Arduino.
Adafruit_Sensor_Fusion filter;

// SETUP FUNKCIJA - Izvršava se samo jednom.
void setup() {
    Serial.begin(9600);
    while (!Serial);

    // Inicijalizacija senzora
    if (!lsm.begin()) {
        Serial.println("Greska pri inicijalizaciji LSM9DS1 senzora!");
        while (1);
    }
    Serial.println("LSM9DS1 senzor pronadjen!");

    // "Povezivanje" naših pomoćnih objekata sa stvarnim senzorima.
    accelerometer = lsm.getAccel();
    magnetometer  = lsm.getMag();
    gyroscope     = lsm.getGyro();
    
    // Pokretanje filtera za fuziju. Argument je brzina ažuriranja u Hz.
    filter.begin(100); // 100 Hz je dobra brzina.
}

// LOOP FUNKCIJA - Ponavlja se u krug.
void loop() {
    // Varijable za spremanje podataka sa senzora.
    sensors_event_t accel_event, mag_event, gyro_event;

    // Očitavanje podataka sa sva tri senzora.
    accelerometer->getEvent(&accel_event);
    magnetometer->getEvent(&mag_event);
    gyroscope->getEvent(&gyro_event);

    // --- Primjena kalibracije na podatke magnetometra ---
    // Ovo je ključan korak za točan smjer!
    float mag_x = mag_event.magnetic.x - (mag_min[0] + mag_max[0]) / 2;
    float mag_y = mag_event.magnetic.y - (mag_min[1] + mag_max[1]) / 2;
    float mag_z = mag_event.magnetic.z - (mag_min[2] + mag_max[2]) / 2;
    
    // "Nahranimo" filter svježim podacima sa senzora.
    // Jedinice moraju biti rad/s za giroskop i m/s^2 za akcelerometar,
    // što su standardne jedinice koje Adafruit biblioteka daje.
    filter.update(gyro_event.gyro.x, gyro_event.gyro.y, gyro_event.gyro.z,
                  accel_event.acceleration.x, accel_event.acceleration.y, accel_event.acceleration.z,
                  mag_x, mag_y, mag_z);

    // Dohvaćanje rezultata iz filtera
    float roll, pitch, heading;
    roll    = filter.getRoll();
    pitch   = filter.getPitch();
    heading = filter.getYaw(); // Yaw je ono što nas zanima - smjer!

    // Ispis rezultata
    Serial.print("Orijentacija (Heading/Smjer): ");
    Serial.print(heading);
    Serial.println(" stupnjeva");

    // Pauza kako bi se uskladili s brzinom filtera (100 Hz -> 10ms)
    // i da ne pretrpamo Serial Monitor.
    delay(100);
}
