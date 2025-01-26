# PontTournant

## Description

Ce projet permet de gérer un pont *tournant ferroviaire* avec un **Arduino**.

Il utilise :    
  - un afficheur LCD 4 x 20 I2C avec SCL sur les broches A5 et SDA sur A4
  - PAD 4 * 4 touches sur les broches D2 a D9
  - un moteur à pas NEMA 14 à 200 pas/rotation avec reduction 8:1 via A4988 sur les broches D11 (DIR) et D12 (STEP) et mise en oeuve avec la librairie AccelStepper