# PontTournant

## Description

Ce projet permet de gérer un pont *tournant ferroviaire* avec un **Arduino**.

Le pont tournant permet de faire pivoter une voie pour aligner une locomotive avec une voie (entrée et/ou sortie) et une voie de garage).

## Contrainte

L'entrée de la locomotive sur le pont et la sortie peut se faire sur la même voie ou sur deux voies différentes.

Le pont n’est pas symétrique du fait de la présence de la cabine de pilotage sur le pont. 
L’entrée de la locomotive sur le pont se fait toujours à l’opposé de la cabine.
De ce fait, il doit être possible de retourner la locomotive lors d'une manoeuvre.

Pour atteindre la voie choisie, le trajet du pont doit être optimisé en choisisant le chemin le plus court dans le sens des aiguilles d'une montre ou le sens inverse.

Une locomotive à vapeur est toujours garée, dans une voie couverte, cheminée vers le pont tournant (garée *tender en arrière* sur sa voie de garage couverte).

La voie de référence pour le point zéro du moteur PAP est la voie d’entrée (voie 1). Une fonction de *homing* doit permet de recaler le pont sur la voie d'entrée.
Cette fonction est appelée systématiquement à l'initialisation du pont.

## Matériel

Il utilise :    
  - un pont tournant JOUEF, 40 voies max, angle de 9° entre chaque voie ;
  - un arduino Uno / Nano ou Mega ;
  - un afficheur LCD 4 x 20 I2C avec SCL sur les broches A5 et SDA sur A4 ;
  - PAD 4 x 4 touches sur les broches D2 a D9 ;
  - un capteur Hall et un aimant pour le *homing* ; 
  - un moteur à pas NEMA 14 à 200 pas/rotation avec reduction 8:1 via A4988 sur les broches D11 (DIR) et D12 (STEP) et mise en oeuve avec la librairie AccelStepper