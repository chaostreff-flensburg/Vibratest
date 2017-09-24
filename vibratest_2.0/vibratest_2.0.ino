/*
 *  Mit diesem Sektch können Vibrationen im niedrigen Hz-Bereich visualisiert werden.
 *  Mit Hilfe eines drei-Achsen-Beschleunigungssensors wird die Gesamtbeschleunigung ermittelt.
 *  Zunächst wird eine Messreihe aufgenommen #MEASURMENTS,
 *  anschließend wird die Messreihe der Größe nach sortiert #sort
 *  und aus die höchsten und niedrigsten Werte entfernt #CUT.
 *  Aus den verbleibenden Werten wird das arithmetische Mittel bestimmt.
 *  Eine Glättung Ausgabewerte erfolgt zum Schluss durch eine Tiefpassfilterung,
 *  welche die Antuellen Messwerte mit den vorangegangenen Messwerten ins Verhältnis setzt #WEIGHT
 */

// Pinbelegung des XYZ-Beschleunigungssensors
// sw - gnd
// rt - 5v
// gr - A4  I2C
// ws - A5  I2C

//*******************************************************************************************************************

// Benötigte Bibliotheken
#include <Wire.h>
#include <L3G.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

//*******************************************************************************************************************

// LED-Matrix Config
#ifndef PSTR
  #define PSTR // Make Arduino Due happy
#endif

#define LEDPIN 4

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(16, 8, LEDPIN, NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);

//*******************************************************************************************************************

// Umfang der Messreihe
#define MEASURMENTS 50

// Tiefpassfilter
#define CUT 5         // schneidet die höchten und tiefsten Messwerte aus
#define WEIGHT 35     // Anteil der neuen Sensorwerte in Prozent

int scale = 80;       // Stauchung der Y-Achse ************** ggf über Poti einstellen??? *************************
int overload = 8;   // Begrenzung der Y-Ache. Ohne Begrenzung werden sehr große Werte auf 0 gesetzt                          

//*******************************************************************************************************************

L3G gyro;

// Berechnung des geometrischen Mittels
long aa;
long bb;
int tt = MEASURMENTS - 1;

int X[MEASURMENTS];
int Y[MEASURMENTS];
int Z[MEASURMENTS];

float dx;
float dy;
float dz;

//Ergebnisse
float durchschnitt_alt;
float durchschnitt_neu;

//*******************************************************************************************************************

void setup() {
  Serial.begin(19200);
  Wire.begin();
  
  if (!gyro.init()) {
    Serial.println("Failed to autodetect gyro type!");
    while (1);
  }
  gyro.enableDefault();

  matrix.begin();
  // matrix.setRotation(2);
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(matrix.Color(255, 0, 0));
}

//*******************************************************************************************************************

// "Bubblesort Algorithmus" zur aufsteigenden Sortierung der Messwerte
void sort(int a[], int size) {
  for (int i = 0; i < (size - 1); i++) {
    for (int o = 0; o < (size - (i + 1)); o++) {
      if (a[o] > a[o + 1]) {
        int t = a[o];
        a[o] = a[o + 1];
        a[o + 1] = t;
      }
    }
  }
}

//*******************************************************************************************************************

// Matrix-Pixel-Buffer Logic
int pixelBuffer[] = {0,1,2,3,4,5,6,7,8,0,1,2,3,4,5,6};

void pushValue(int value) {
  for (int i = 15; i > 0; i--) {
    pixelBuffer[i] = pixelBuffer[i-1];
  }
  pixelBuffer[0] = value;
}

void graphData(int value) {
  pushValue(value);
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 8; j++) {
      uint16_t color;

      if ( pixelBuffer[i] >= (j+1) ) {
        if (j < 4) { color = matrix.Color(0, 255, 60); }
        else if (j > 5) { color = matrix.Color(255, 0, 0); }
        else { color = matrix.Color(255, 240, 0); }
      }
      else {
        color = matrix.Color(0, 0, 0);
      }

      matrix.drawPixel(i, j, color);
    }
  }
  matrix.show();
}

//*******************************************************************************************************************

void loop() {
  
// Messwerterfassung
  for (int i = 0; i < tt; i++) {
    gyro.read();
    aa = (int)gyro.g.x;
    if (aa < 0) {
      bb = -aa;
    } else {
      bb = aa;
    }
    X[i] = bb;
    aa = (int)gyro.g.y;
    if (aa < 0) {
      bb = -aa;
    } else {
      bb = aa;
    }
    Y[i] = bb;
    aa = (int)gyro.g.z;
    if (aa < 0) {
      bb = -aa;
    } else {
      bb = aa;
    }
    Z[i] = bb;
    delayMicroseconds(5);
  }

// Sortieren der Messwerte 
  sort(X, MEASURMENTS);
  sort(Y, MEASURMENTS);
  sort(Z, MEASURMENTS);

// Berechnung der Durchnittswerte der einzelnen Achsen
  dx = 0;
  dy = 0;
  dz = 0;

  for (int i = (CUT - 1); i < (MEASURMENTS - CUT); i++) {
    dx = dx + X[i] / (MEASURMENTS - 2 * CUT);
    dy = dy + Y[i] / (MEASURMENTS - 2 * CUT);
    dz = dz + Z[i] / (MEASURMENTS - 2 * CUT);
  }

// Tiefpassfilterung
  durchschnitt_neu = ((100 - WEIGHT)  * durchschnitt_alt / 100 + WEIGHT  * (dx + dy + dz) / 300)/scale;
  durchschnitt_alt = durchschnitt_neu;

// Ausgabe des Durchschnitts
  if (durchschnitt_neu < 0) {
    durchschnitt_neu = 0;
  }
  if (durchschnitt_neu > overload) {
    durchschnitt_neu = overload;
  }
  Serial.println(durchschnitt_neu);

  graphData(durchschnitt_neu);

  delay(200);
}
