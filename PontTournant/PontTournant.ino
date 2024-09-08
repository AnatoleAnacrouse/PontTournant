// -----------------------------------------------------------------------------
//
// TITRE       : Pont tournant
// AUTEUR      : M. EPARDEAU et F. FRANKE
// DATE        : 8/09/2024
//
#define VERSION "   VERSION 0.5"
//
// ----------------------------- DESCRIPTION ----------------------------------
//
//
// ----------------------------- MATERIELS ------------------------------------
//
//    - AFFICHEUR 4 x 20 I2C
//       . SCL sur A5
//       . SDA sur A4
//
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C LCD(0x27, 20, 4);
//
//
//    - PAD 4 * 4 touches
//      . broches D2 a D9
//
#include <Keypad.h>
#define ROWS 4
#define COLS 4
const char kpKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowKpPin [4] = {9, 8, 7, 6};
byte colKpPin [4] = {5, 4, 3, 2};
Keypad kp = Keypad(makeKeymap(kpKeys), rowKpPin, colKpPin, ROWS, COLS);
//
//
//  - Moteur à pas NEMA 14 200 pas/rotation avec reduction 8:1 ()
//       via A4988 sur les broches D11 (DIR) et D12 (STEP)
//
//#include <Servo.h>
#include <AccelStepper.h>
#define PIN_MOT_STEP 12
#define PIN_MOT_DIR 11
const int stepsPerRevolution = 1600;
AccelStepper pontTournant(1, PIN_MOT_STEP, PIN_MOT_DIR);
//
// -----------------------------------------------------------------------------
//    - Buzzer
//#include <Tone.h>
#define PIN_BUZZER 13

// --------------------------- CONSTANTES --------------------------------------

// Constantes globales
#define OK 0
#define ERROR -1
#define SECOND 1000

// ------------------------CONFIGURATION DU PONT -------------------------------

#define NB_MAX_VOIE 40
// Pour une reduction de 1/8 d'un moteur de 200 pas / rev. il y a 1600 pas/rev.
// pour 40 voies, chaque voie necessite un déplacement de 40 * n
const int tabVoie[NB_MAX_VOIE + 1] = {
  0, 40, 80, 120, 160, 200, 240, 280, 320, 360,
  400, 440, 480, 520, 560, 600, 640, 680, 720, 760,
  800, 840, 880, 920, 960, 1000, 1040, 1080, 1120, 1160,
  1200, 1240, 1280, 1320, 1360, 1400, 1440, 1480, 1520, 1560, 1600
};

const byte voieDEntree = 0;
int voieCourante = voieDEntree;
int voieSelectionnee = 0;
bool manoeuvreDEntre = false;
bool retournementChoisi = false;

/* ============================================================================
   Procedures d'affichage sur le LCDup
   ========================================================================== */
void afficherLCD(const String texte) {
  LCD.clear();
  LCD.print(texte);
}

void afficherLCD2(const String texte) {
  LCD.setCursor(0, 1);
  LCD.print(texte);
}

/* ============================================================================
   Setup
   ========================================================================== */
void setup() {

  Serial.begin(115200);

  // Nombre de lignes et colonnes du LCD
  LCD.begin(0x27, 16, 2);
  LCD.backlight();
  LCD.setCursor(0, 0);

  pinMode(PIN_BUZZER, OUTPUT);

  // Broches moteur
  //pinMode(PIN_MOT_DIR,OUTPUT);
  //pinMode(PIN_MOT_STEP,OUTPUT);
  pontTournant.setMaxSpeed(1000);
  pontTournant.setAcceleration(100);

  // servo commandée par la pin D9
  //pontTournant.attach(pinPontTournant);
  //pontTournant.write(tabVoie[voieCourante]);

  afficherLCD(VERSION);
  delay(1*SECOND);
}

/* ============================================================================
   Saisie sur le PAD de la manoeuvre
   ========================================================================== */
int saisirManoeuvre (bool* entree, int* voie, bool* retournement) {

  int v = 0;
  bool e = false;
  bool r = false;
  char touche = '\0';
  bool entreeValide = false;

  // Saisir le type de manoeuvre Entree=A ou Sortie=B
  // ------------------------------------------------
  afficherLCD("E ou S (A/B)? ");
  do {
    touche = kp.getKey();
    entreeValide = (touche == 'A' || touche == 'B' || touche == '#');
    if (!entreeValide)
      tone(PIN_BUZZER, 220, 1/2*SECOND);
  } while (!entreeValide);

  if (touche == '#') {
    afficherLCD("Abandon");
    delay(1*SECOND);
    return ERROR;
  }

  if (touche == 'A') {
    *entree = false;
    *voie = voieDEntree;
    *retournement = false;
    afficherLCD("Entree => retour");
    afficherLCD2("sur voie de garage");
    delay(SECOND);
    return OK;
  }
  else { // touche == 'B'
    // e est deja a False
    afficherLCD("Sortie");
    delay(1/2*SECOND);
  }

  // Saisir la voie
  // --------------
  afficherLCD("Voie (1 - 9) ? ");
  entreeValide = false;
  do {
    touche = kp.getKey();
    entreeValide = (((touche >= '1') && (touche <= '9')) || (touche == '#'));
  } while (!entreeValide);

  if (touche == '#') {
    afficherLCD("Abandon");
    delay(1*SECOND);
    return ERROR;
  }

  else {
    v = touche - '0';
    LCD.print(v);
    delay(1/2*SECOND);
  }

  // Saisir la voie (suite) ou le retournement Rnt=C/~Rnt=D
  // ------------------------------------------------------
  if (v < 5)  // (NB_MAX_VOIE / 10) +1
    afficherLCD("Voie ou Ret. (C/D) ? ");
  else       
    afficherLCD("Ret. (C/D) ? ");

  afficherLCD2("Voie "  + (String) v);

  entreeValide = false;
  do {
    touche = kp.getKey();
    entreeValide = ((touche >= '0') && (touche <= '9'))
                   || (touche == 'C') || (touche == 'D')
                   || (touche == '#');
    if ((touche >= '0') && (touche <= '9')) {
      int vp = (v * 10) + touche - '0';
      entreeValide = (vp <= NB_MAX_VOIE);
    }
    delay(1/2*SECOND);
  } while (!entreeValide);

  if (touche == '#') {
    afficherLCD("Abandon");
    delay(1*SECOND);
    return ERROR;
  }

  else if ((touche >= '0') && (touche <= '9')) {
    v = (v * 10) + touche - '0';
    afficherLCD2("Voie "  + (String) v);
    delay(1*SECOND);


    // Saisir la le retournement
    // --------------------------
    afficherLCD("Ret.(C/D) ? ");
    entreeValide = false;
    do {
      touche = kp.getKey();
      entreeValide = (touche == 'C' || touche == 'D' || touche == '#');
    } while (!entreeValide);

    if (touche == '#') {
      afficherLCD("Abandon");
      delay(1*SECOND);
      return ERROR;
    }
    else {
      r = (touche == 'C');
      delay(1/2*SECOND);
    }
  } // sinon si (touche entre '0'et '9')

  else
    r = (touche == 'C');

  // --------------------------------
  if (r) 
    afficherLCD2("Retournement");
  else 
    afficherLCD2("Pas de retournement");

  delay(1*SECOND);

  // Mise à jour des parametres en sortie
  // ------------------------------------
  *entree = e;
  *voie = v;
  *retournement = r;

  return OK;
}

/* ============================================================================
  fonction d'pptimisation du trajet du pont roulan

  int calculateShortestPath(int currentPos, int targetPos) {

     int distance = targetPos - currentPos;

     if (abs(distance) > stepsPerRevolution / 2) {
        if (distance > 0) {
          distance -= stepsPerRevolution;
        } else {
          distance += stepsPerRevolution;
        }
     }

     return currentPos + distance;
  }
   ========================================================================== */
int calculerPlusCourtChemin(int currentPos, int targetPos) {

  // Calculer la distance entre la position actuelle et la position cible
  int distance = targetPos - currentPos;

  // Si la distance absolue est supérieure à la moitié d'un tour complet
  if (abs(distance) > stepsPerRevolution / 2) {
    // Ajuster la distance pour prendre le chemin le plus court
    if (distance > 0) {
      distance -= stepsPerRevolution;
    } else {
      distance += stepsPerRevolution;
    }
  }

  //return currentPos + distance;
  return distance;
}

/* ============================================================================
   Boucle principale
   ========================================================================== */
void loop() {

  //voieSelectionnee = -1;
  //manoeuvreDEntre = false;
  //retournementChoisi = false;

  LCD.clear();

  // Saisir les donnees de la manoeuvre
  int codeRetour = saisirManoeuvre(&manoeuvreDEntre, &voieSelectionnee, &retournementChoisi);

  // Si la saisie de la manouvre est complete
  if (codeRetour == OK) {

    // Si c'est un retournement alors choisir la voie opposée
    if (retournementChoisi) {
      voieSelectionnee = (NB_MAX_VOIE / 2 + voieSelectionnee) % NB_MAX_VOIE;
    }

    // Si on est deja sur la voie selectionnee alors ne rien faire
    if (voieSelectionnee == voieCourante)
      return;

    afficherLCD("Manoeuvre en cours");

    // Si c'est une manoeuvre d'entree alors choisr la voie de garage
    if (manoeuvreDEntre)  {
      voieSelectionnee = voieDEntree;
    }

    Serial.print("Voie selectionnee = "); Serial.print(voieSelectionnee);   Serial.println("  ");

    // Normaliser si necessaire
    if (abs(pontTournant.currentPosition()) > stepsPerRevolution) {
      int position = pontTournant.currentPosition() % stepsPerRevolution;
      pontTournant.setCurrentPosition(position);
    }

    if (voieSelectionnee == voieDEntree)
      afficherLCD2("vers voie garage");
    else
      afficherLCD2("vers voie "  + (String) voieSelectionnee);

    // Enables the motor to move in a particular direction (LOW)
    // digitalWrite(PIN_MOT_DIR,HIGH);

    // Calculer la maneuvre optimale
    int distance = calculerPlusCourtChemin(tabVoie[voieCourante], tabVoie[voieSelectionnee]);

    Serial.print("Position = "); Serial.print(pontTournant.currentPosition());
    Serial.print("  Cible = "); Serial.print(tabVoie[voieSelectionnee]);
    Serial.print("  Distance = "); Serial.print(distance); Serial.println("  ");

    // Réaliser la manoeuvre
    pontTournant.moveTo(pontTournant.currentPosition() + distance);
    pontTournant.runToPosition();

    // pontTournant.runToNewPosition(tabVoie[voieSelectionnee]);
    voieCourante = voieSelectionnee;

    afficherLCD("Fin de manoeuvre");
  } // if codeRetour est OK

  delay(2 * SECOND);

}