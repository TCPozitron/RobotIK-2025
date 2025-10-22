// 1. UKLJUČI NOVU BIBLIOTEKU
#include <SparkFunLSM9DS1.h>
#include "Wire.h"

/* ... Dijagram spajanja ... (isti je) */

// 2. KREIRAJ OBJEKT IZ NOVE KLASE
LSM9DS1 imu;

// 3. ZALIJEPITE SVOJE VRIJEDNOSTI OVDJE
// (Ovdje treba zalijepiti rezultat kalibracije magnetonetra)
float magBias[3] = { 305.000000f, 185.500000f, 1095.000000f };

// Postavke ispisa...
#define PRINT_CALCULATED
#define PRINT_SPEED 250 
static unsigned long lastPrint = 0; 

// 4. ISPRAVAK DEKLINACIJE
#define DECLINATION 4.8 // Deklinacija za Pazin, Istra

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    // 5. KORISTI NOVI .begin()
    if (imu.begin() == false) 
    {
        Serial.println("Failed to communicate with LSM9DS1.");
        Serial.println("Double-check the wiring!");
        while (1) { delay(100); }
    }
    
    // 6. MAKNULI SMO 'setMagBiases()' JER NE POSTOJI
    
    Serial.println("Senzor inicijaliran. Koristim rucnu kalibraciju."); 
}


void loop()
{
    // Ažuriraj senzore
    if (imu.gyroAvailable()) { imu.readGyro(); }
    if (imu.accelAvailable()) { imu.readAccel(); }
    
    // --- OVDJE JE KLJUČNA PROMJENA ---
    if (imu.magAvailable()) 
    { 
        // 1. Očitaj sirove (RAW) podatke
        imu.readMag(); 
        
        // 2. Ručno oduzmi pogrešku (bias) od sirovih vrijednosti
        // Pretvaramo float bias natrag u int16_t za oduzimanje
        imu.mx = imu.mx - (int16_t)magBias[0];
        imu.my = imu.my - (int16_t)magBias[1];
        imu.mz = imu.mz - (int16_t)magBias[2];
    }
    // --- KRAJ PROMJENE ---


    // Ispis (potpuno isto)
    if ((lastPrint + PRINT_SPEED) < millis())
    {
        // Sada će printMag() i printAttitude() automatski koristiti
        // ISPRAVLJENE 'imu.mx', 'imu.my' i 'imu.mz' vrijednosti.
        
        printGyro(); 
        printAccel();
        printMag();
        printAttitude(imu.ax, imu.ay, imu.az, -imu.my, -imu.mx, imu.mz);
        Serial.println();
        lastPrint = millis(); 
    }
}

//
// SVE FUNKCIJE ISPOD OSTAJU POTPUNO ISTE
//

void printGyro()
{
    Serial.print("G: ");
#ifdef PRINT_CALCULATED
    Serial.print(imu.calcGyro(imu.gx), 2);
    Serial.print(", ");
    Serial.print(imu.calcGyro(imu.gy), 2);
    Serial.print(", ");
    Serial.print(imu.calcGyro(imu.gz), 2);
    Serial.println(" deg/s");
#elif defined PRINT_RAW
    Serial.print(imu.gx);
    Serial.print(", ");
    Serial.print(imu.gy);
    Serial.print(", ");
    Serial.println(imu.gz);
#endif
}

void printAccel()
{
    Serial.print("A: ");
#ifdef PRINT_CALCULATED
    Serial.print(imu.calcAccel(imu.ax), 2);
    Serial.print(", ");
    Serial.print(imu.calcAccel(imu.ay), 2);
    Serial.print(", ");
    Serial.print(imu.calcAccel(imu.az), 2);
    Serial.println(" g");
#elif defined PRINT_RAW
    Serial.print(imu.ax);
    Serial.print(", ");
    Serial.print(imu.ay);
    Serial.print(", ");
    Serial.println(imu.az);
#endif
}

void printMag()
{
    Serial.print("M: ");
#ifdef PRINT_CALCULATED
    // Ova funkcija sada koristi ispravljene (kalibrirane)
    // vrijednosti imu.mx, imu.my, imu.mz
    Serial.print(imu.calcMag(imu.mx), 2);
    Serial.print(", ");
    Serial.print(imu.calcMag(imu.my), 2);
    Serial.print(", ");
    Serial.print(imu.calcMag(imu.mz), 2);
    Serial.println(" gauss");
#elif defined PRINT_RAW
    Serial.print(imu.mx);
    Serial.print(", ");
    Serial.print(imu.my);
    Serial.print(", ");
    Serial.println(imu.mz);
#endif
}

void printAttitude(float ax, float ay, float az, float mx, float my, float mz)
{
    float roll = atan2(ay, az);
    float pitch = atan2(-ax, sqrt(ay * ay + az * az));

    float heading;
    if (my == 0)
        heading = (mx < 0) ? PI : 0;
    else
        // Ova funkcija sada također koristi kalibrirane vrijednosti
        heading = atan2(mx, my);

    heading -= DECLINATION * PI / 180;

    if (heading > PI)
        heading -= (2 * PI);
    else if (heading < -PI)
        heading += (2 * PI);

    heading *= 180.0 / PI;
    pitch *= 180.0 / PI;
    roll *= 180.0 / PI;

    Serial.print("Pitch, Roll: ");
    Serial.print(pitch, 2);
    Serial.print(", ");
    Serial.println(roll, 2);
    Serial.print("Heading: ");
    Serial.println(heading, 2);
}
