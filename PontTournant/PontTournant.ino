// --------------------------------------------------------------------
//
// TITRE       : Pont tournant
// AUTEUR      : M. EPARDEAU et F. FRANKE
// DATE        : 09/02/2025
//
#define VERSION "  VERSION 1.0"
//
//DESCRIPTION :
//
// --------------------------------------------------------------------
//
// Configuration de l'afficheur LCD
//    - AFFICHEUR 4 x 20 I2C
//       . SCL sur A5
//       . SDA sur A4
//
// Bibliothèque pour l'écran LCD I2C
#include <LiquidCrystal_I2C.h>
// Initialisation de l'écran LCD : adresse 0x27, 20 colonnes, 4 lignes
LiquidCrystal_I2C LCD(0x27, 20, 4);
//
//    - PAD 4 * 4 touches
//      . broches D2 a D9
//
// --------------------------------------------------------------------
// Configuration du pavé numérique
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

// Broches Arduino connectées aux lignes du clavier (9, 8, 7, 6)
const byte rowKpPin [4] = {9, 8, 7, 6};
// Broches Arduino connectées aux colonnes du clavier (5, 4, 3, 2)
const byte colKpPin [4] = {5, 4, 3, 2};
// Initialisation du clavier avec la configuration définie
Keypad kp = Keypad(makeKeymap(kpKeys), rowKpPin, colKpPin, ROWS, COLS);

// --------------------------------------------------------------------
// Configuration du moteur pas à pas
//
//  - Moteur à pas NEMA 14 200 pas/rotation avec reduction 2:1 ()
//       via A4988 sur les broches D11 (DIR) et D12 (STEP)
#include <AccelStepper.h>
// Broche Arduino pour le signal STEP (impulsions)
#define PIN_MOT_STEP 12
// Broche Arduino pour le signal DIR (direction)
#define PIN_MOT_DIR 11
// Nombre de pas pour une rotation complète (200 pas * réduction 2:1)
const int stepsPerRevolution = 400;
//
// Instanciation du pont tournant avec la librairie AccelStepper
// Cette librairie  elle permet de définir une vitesse maximale (setMaxSpeed) 
// et une accélération (setAcceleration), ce qui assure des démarrages 
// et des arrêts progressifs pour le pont tournant
AccelStepper pontTournant(1, PIN_MOT_STEP, PIN_MOT_DIR);

// --------------------------------------------------------------------
// Constantes globales
//
// Une seconde = 1000 millisecondes
#define SECOND 1000

/* enum {
  ERREUR = -1,   OK = 0,   ABANDON = 1,   
  ENTREE = 10,   SORTIE = 11, 
  RETOURNEMENT = 20,   SANSRETOURNEMENT = 21
}; 

enum ActionType { ERREUR = -1, OK = 0, ABANDON = 1, 
                  ENTREE = 10, SORTIE = 11, 
                  RETOURNEMENT = 20, SANSRETOURNEMENT = 21 };
*/

// Constante utiles à la saisie des commandes
#define ERREUR -1
#define OK 0      // et pour presence engin sur PT
#define ABANDON 1
#define ENTREE 10
#define SORTIE 11
#define RETOURNEMENT 20
#define SANSRETOURNEMENT 21

// --------------------------------------------------------------------
// Configuration du pont tournant
//
// Nombre maximum de voies disponibles
/* const int NB_MAX_VOIE = 40;*/
#define NB_MAX_VOIE 40

// Tableau des positions en pas pour chaque voie (0 à 40)
// Pour un moteur à 200 pas/rev. et une reduction de 1/2, il y a au total 400 pas/rev.
// Avec 400 pas/révolution et 40 voies : chaque voie = 10 pas
const int tabVoie[NB_MAX_VOIE + 1] = {
  0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
  110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
  210, 220, 230, 240, 250, 260, 270, 280, 290, 300,
  310, 320, 330, 340, 350, 360, 370, 380, 390, 400 };

// Definition des voies principales
const byte voieEntree = 0;
const byte voieSortie = voieEntree;

// Au démarrage, la voie courante est la voie d'entree
int voieCourante = voieEntree;

/* ==========================================
   Procedures d'effacement d'une ligne du LCD
   ========================================== */
void effacerLCD(const byte ligne) {
  LCD.setCursor(0, ligne);
  LCD.print(F("                    "));
}

/* ==========================================
   Procedures d'affichage sur le LCD
   ========================================== */
void afficherLCD(const String &texte, const byte ligne, const bool effacement) {
  
  if (effacement) {
  LCD.clear();
  }

  effacerLCD(ligne);
  LCD.setCursor(0, ligne);
  LCD.print(texte);
}

/* ==========================================
   Setup
   ========================================== */
void setup() {

  Serial.begin(115200);

  // Nombre de lignes et colonnes du LCD
  LCD.begin(0x27, 20, 4);
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  
  afficherLCD(VERSION, 0, true);

  // Broches moteur
  //pinMode(PIN_MOT_DIR,OUTPUT);
  //pinMode(PIN_MOT_STEP,OUTPUT);
  pontTournant.setMaxSpeed(1000);
  pontTournant.setAcceleration(100);

  // --------------------------------------------------
  // INSERER ICI LA PROCEDURE DE CALIBRATION => se positionner sur la voie d'entrée
  // --------------------------------------------------

  delay(1*SECOND);
}

/* ===============================================================
   Saisie sur le PAD du type de manoeuvre ENTREE (A) ou SORTIE (B)
   
   Afficher "E (A) ou S (B)?"
   
   Répéter jusqu’à ce qu’une touche valide soit pressee ('A', 'B', ou '#')
      Si '#' alors retourner ABANDON
      Si 'A' alors retourner ENTREE
      Si 'B' alors retourner SORTIE
      
   Afficher le choix sur l’écran
   Attendre 1 seconde
   Effacer la ligne 4
   
   =============================================================== */
int saisirTypeManoeuvre() {

  int typeManoeuvre = ABANDON;
  char touche = '\0'; // initialise avec le caractere ASCCI NULL
  bool entreeValide = false;

  // Saisir le type de manoeuvre Entree=A ou Sortie=B
  afficherLCD("E (A) ou S (B)? ", 0, true);

  do {
    touche = kp.getKey();
    entreeValide = (touche == 'A' || touche == 'B' || touche == '#');
    // if (!entreeValide) tone(PIN_BUZZER, 220, 1/2*SECOND);
  } while (!entreeValide);

  if (touche == '#') {
    typeManoeuvre = ABANDON;
    afficherLCD("ABANDON", 3, false);
  }
  else if (touche == 'A') {
    typeManoeuvre = ENTREE;
    afficherLCD("Entree", 0, true);
  }
  else { // typeManoeuvre == SORTIE
    typeManoeuvre = SORTIE;
    afficherLCD("Sortie", 0, true);
  }

  delay(1*SECOND);
  effacerLCD(3);

  return typeManoeuvre;
}

/* ==============================================
   Saisir sur le PAD de la voie (1 à NB_MAX_VOIE)
   
   Afficher "Voie (1-40) ou '*'"
   
   Lire la première touche (1-9 ou #)
      Si '#' alors retourner ABANDON
      Sinon convertir le char en int dans 'voie'
      
   Lire la deuxième touche (0-9, '*', ou #)
      Si '#' alors retourner ABANDON
      Si '*' alors retourner la voie actuelle
      Si chiffre alors calculer voie = voie * 10 + chiffre
         Si voie > 40 alors retourner ERREUR
         Sinon afficher la voie et retourner la valeur
   
   ============================================== */
int saisirVoie() {

  int voie = 0;
  char touche = '\0'; // initialise avec le caractere ASCCI NULL
  bool entreeValide = false;

  afficherLCD("Voie (1-40) ou '*'", 1, false);
  do {
    touche = kp.getKey();
    entreeValide = ((touche >= '1') && (touche <= '9') || (touche == '#'));
  } while (!entreeValide);

  if (touche == '#') {
    afficherLCD("ABANDON", 3, false);
    delay(1*SECOND);
    return ABANDON;
  }

  if ((touche >= '1') && (touche <= '9')) {
    voie = touche - '0';
    afficherLCD("Voie " + (String) voie, 3, false);
  }

  do {
    touche = kp.getKey();
    entreeValide = ((touche >= '0') && (touche <= '9') || (touche == '*') || (touche == '#'));
  } while (!entreeValide);

  if (touche == '#') {
    afficherLCD("ABANDON", 3, false);
    delay(1*SECOND);
    return ABANDON;
  }

  if (touche == '*') {
    afficherLCD("Voie " + (String) voie, 1, false);
    delay(1*SECOND);
    effacerLCD(3);
    return voie;
  }

  // il ne peut pas y avoir plus de 40 voies
  else if (voie < 5)
   {
    if ((touche >= '0') && (touche <= '9')) {
      voie = (voie * 10) + touche - '0';
      entreeValide = (voie <= NB_MAX_VOIE);
      if (!entreeValide) {
        afficherLCD("Au dessus de 40", 3, false);
        delay(1*SECOND);
        return ERREUR;
      }
      else {
        afficherLCD("Voie (1-40) ou '*'", 3, false);
        afficherLCD("Voie " + (String) voie, 1, false);
        delay(2*SECOND);
        effacerLCD(3);
        return voie;
      }
    }
  }
  else {
    afficherLCD("ERREUR", 3, false);
    delay(1*SECOND);
    return ERREUR;
  }
}

/* ======================
   Saisir le retournement
   
   Afficher "Ret. O(C) / N(D)?"
   
   Répéter jusqu’à ce qu’une touche valide soit pressée ('C', 'D', ou '#')
     Si '#' alors retourner ABANDON
     Si 'C' alors retourner RETOURNEMENT
     Si 'D' alors retourner SANSRETOURNEMENT
     
  Afficher le choix sur l’écran

   ====================== */
int saisirRetournement() {

  bool retournement = false;
  char touche = '\0'; // initialise avec le caractere ASCCI NULL
  bool entreeValide = false;

  afficherLCD("Ret. O(C) / N(D)? ", 2, false);
  
  do {
    touche = kp.getKey();
    entreeValide = (touche == 'C' || touche == 'D' || touche == '#');
  } while (!entreeValide);

  if (touche == '#') {
    afficherLCD("ABANDON", 3, false);
    delay(1*SECOND);
    return ABANDON;
  }
    
  retournement = (touche == 'C');

  if (retournement)
  {
    afficherLCD("Retournement", 2, false);
    effacerLCD(3);
    return RETOURNEMENT;
  }
  else
  {
    afficherLCD("Pas de retournement", 2, false);
    effacerLCD(3);
    return SANSRETOURNEMENT;
  }

  delay(1*SECOND);
}

/* ==========================================================
   Saisir sur le PAD entree/sortie loco sur PT par touche '*'
   
   Afficher "Loco deplacee? oui=*"
   Attendre que l’utilisateur appuie sur '*'
   
   Effacer la ligne d’affichage
   Retourner OK

   ========================================================== */
int attendreDeplacementEngin() {

  char touche = '\0';     // donne automatiquement la valeur ASCII
  bool entreeValide = false;

  // Saisir le type de manoeuvre Entree=A ou Sortie=B
  afficherLCD("Loco deplacee? oui=*", 3, false);

  do {
    touche = kp.getKey();
    entreeValide = (touche == '*' );
    if (!entreeValide) ; // tone(PIN_BUZZER, 220, 1/2*SECOND);
  } while (!entreeValide);

  effacerLCD(3);
  return OK;
}

/* ==================================
  Optimiser le trajet du pont roulant
   ================================== */
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

  // return currentPos + distance;
  return distance;
}

/* ==================================================
   Deplacer le PT de la voie actuelle à la voie cible
   
   Afficher "En rotation"
   Si retournement demandé alors calculer la voie opposée : voie = (20 + voie) % 40
   Si déjà sur la voie alors retourner OK
   
   Normaliser la position actuelle si > 400 pas
   
   Calculer la distance optimale avec calculerPlusCourtChemin()
   Déplacer le moteur à la position cible
   Mettre à jour la voie courante
   
   Effacer le message "En rotation"
   Retourner OK
   
   ================================================== */

int deplacerPT(const int voieCible, const int retournement) {
  int voie = voieCible;

  afficherLCD("En rotation", 3, false);

  // Si c'est un retournement alors choisir la voie opposée (pivot de 180 deg.)
  if (retournement == RETOURNEMENT) {
    voie = (NB_MAX_VOIE / 2 + voie) % NB_MAX_VOIE;
  }

  // Si on est deja sur la voie  alors ne rien faire
  if (voie == voieCourante) {
    effacerLCD(3);
    return OK;
  }

  // Normaliser si necessaire
  if (abs(pontTournant.currentPosition()) > stepsPerRevolution) {
    int position = pontTournant.currentPosition() % stepsPerRevolution;
    pontTournant.setCurrentPosition(position);
  }

  // Calculer la maneuvre optimale
  int distance = calculerPlusCourtChemin(tabVoie[voieCourante], tabVoie[voie]);

  // Réaliser la manoeuvre
  pontTournant.moveTo(pontTournant.currentPosition() + distance);
  pontTournant.runToPosition();

  effacerLCD(3);
  voieCourante = voie;
  return OK;
}

/* =================
   Boucle principale
   ================= */
void loop() {

  int voieSelectionnee = 0;
  int retournementChoisi = SANSRETOURNEMENT;

  LCD.clear();

  // Saisir le type de manoeuvre Entree ou Sortie
  int typeManoeuvre = saisirTypeManoeuvre();

  switch (typeManoeuvre) { 

    case ABANDON: 
      return;
    break;

    case ENTREE:
      afficherLCD("Entree", 0, true);
      if (voieCourante != voieEntree) {
        deplacerPT(voieEntree, false);
        attendreDeplacementEngin();
      }

      voieSelectionnee = saisirVoie();
      if (voieSelectionnee == ABANDON) {
        return;
      }
      else if (voieSelectionnee == ERREUR) {
        afficherLCD("Saisie erronnee", 3, false);
        delay (1*SECOND);
        return;
      }

      retournementChoisi = saisirRetournement();
      if (retournementChoisi == ABANDON) {
        return;
      }

      deplacerPT(voieSelectionnee, retournementChoisi);
    break;

    case SORTIE:
      afficherLCD("Sortie", 0, true);
      voieSelectionnee = saisirVoie();
      if (voieSelectionnee == ABANDON) {
        return;
      }
      else if (voieSelectionnee == ERREUR) {
        afficherLCD("Saisie erronnee", 3, false);
        delay (1*SECOND);
        return;
      }

      deplacerPT(voieSelectionnee, false);
      attendreDeplacementEngin();

      retournementChoisi = saisirRetournement();
      if (retournementChoisi == ABANDON) {
        return;
      }     
      deplacerPT(voieEntree, retournementChoisi);
    break;

    default:
      // Cas non prévu 
      afficherLCD("ERREUR DE TRAITEMENT", 3, false);
      delay (1*SECOND);
      break;
  }

  // Liberer le pont avant la prochaine manoeuvre
    attendreDeplacementEngin();
}