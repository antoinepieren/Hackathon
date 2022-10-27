#pragma once

#include <math.h>

#define KRAKEN
#define PI 3.1415926535897932384626433832795

#define DELTA_T_ASSER_us 10000.0
#define MAXOSC 200

// la datasheet donne :
//333.3 TOP / Tour
#define DIAMETRE_ROUES 58.533724340176 //POUR LA PRECISION DU DEPLACEMENT EN DISTANCE
#define COEFFDIFGD 1.0
#define DISTANCE_PAR_TICK_G (PI*DIAMETRE_ROUES*3)/1000 // mm
#define DISTANCE_PAR_TICK_D (PI*COEFFDIFGD*DIAMETRE_ROUES*3)/1000  // mm

// L'écart entre les deux roues codeuses
// (attention pas les roues motrices)
#define ECART_ROUES 369.609375//366.875//373.75///250.0 // mm //TROP LOIN = DIMINUER

#define ECART_ROUES_MOTEUR 145.0 // mm



// L'erreur de distance à l'objectif min pour considérer une ligne droite terminée
#define ERROR_MIN_DIST 4 // mm
// L'erreur d'angle à l'objectif min pour considérer une rotation terminée
#define ERROR_MIN_ANGLE 0.01 // rad

// L'angle max du robot vers son objectif à partir duquel le robot décide de rotationner sur place avant de commencer le déplacement
// hysteresis pour éviter les oscillations bizarre autour du seuil
#define ANGLE_MAX_ROTATION_SEULE PI / 12.0
#define ANGLE_MIN_ROTATION_SEULE PI / 120.0


// Consigne de vitesse max
#define MAX_VITESSE 64000

#define ACCEL_MAX 40.0 // en mm/s²

// Les valeurs brutes correspondent à un départ du côté jaune, elles sont transformées en INIT_X/Y/THETA en fonction du coté
#define RAW_INIT_X 0
#define RAW_INIT_Y 0
#define RAW_INIT_THETA 0
#define INIT_X RAW_INIT_X
#define INIT_Y RAW_INIT_Y
#define INIT_THETA RAW_INIT_THETA


// Des coefficients calculé pour faire environ une ligne droite sans asser + non linéarité frottements
#define COEF_G 0//0.155054//0.96
#define COEF_D 0//0.17341


// Coef pour calibrer les tofs utilisés pour la calibration


#define MAX_ERREUR_DIST 200.0
#define MAX_ERREUR_I 100.0

#define MAX_CALI_TOFS_ANGLE 20.0
#define MAX_CALI_TOFS_DIST 100.0

extern int timer_start_tirette;

