/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "gpio.h"
#include "BlocMoteurs.h"
#include "HardEncodeurs.h"
#include "Odometrie.h"
#include "Reglages.h"
#include "FileTaches.h"
#include "DefileurTaches.h"
#include "const.h"
#include "sonar.h"
#include <stdlib.h>
#include <stdio.h>

#define AVANCE 	GPIO_PIN_SET
#define RECULE  GPIO_PIN_RESET
#define POURCENT 640
#define Seuil_Dist_4 1600 // correspond à 10 cm.
#define Seuil_Dist_3 1600
#define Seuil_Dist_1 1600
#define Seuil_Dist_2 1600
#define V1 38
#define V2 56
#define V3 76
#define Vmax 95
#define T_10_MS 5
#define T_2_S 1000
#define T_200_MS 100
#define CKp_D 100  //80 Robot1
#define CKp_G 100  //80 Robot1
#define CKi_D 80  //50 Robot1
#define CKi_G 80  //50 Robot1
#define CKd_D 0
#define CKd_G 0
#define DELTA 0x50
#define AVANCER 420
// Positions (angles) du servomoteur du sonar
#define ANGLE_ZERO 2100
#define ANGLE_90  4000
#define ANGLE90  750

#define ITM_Port32(n) (*((volatile unsigned long *(0xE0000000+4*n)))
int CMDE;
BlocMoteurs *moteurs;
ControleurPID *pid;
FileTaches *taches;
DefileurTaches *defileur;
Sonar* sonar;
Odometrie *odometrie;
int _min_update_period = 10000;
double x;
double y;
double theta;
extern float xglob;
extern float yglob;
extern float thetaglob;
extern float targetdistglob;
extern float targetangleglob;
extern float distglob;
extern float angleglob;
extern float CG;
extern float CD;
extern float COEF2D;
extern float COEF2G;
extern float tempmult;
extern float sonarmeasure;
double leftsonar;
double rightsonar;
double frontsonar;
uint16_t adc_buffer[10]; // Nouvelle borne supérieure pour intégrer le monitoring de la batterie
uint16_t Buff_Dist[8];
uint16_t UartBuff[30];
uint16_t countG = 0;
uint16_t countD = 0;
extern volatile unsigned int Time;
extern volatile unsigned int TLoop;
volatile uint32_t Dist_Obst;
uint32_t Dist_Obst_;
double Dist_Obst_cm;
int sonarcount = 0;
GPIO_PinState pinstate= GPIO_PIN_RESET;
volatile double xpos;
volatile double ypos ;
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
void Gestion_Commandes(void);
void regulateur(void);
void controle(void);
void Calcul_Vit(void);
void ACS(void);
void trig_sonar(void);
void stop_sonar(void);

/* HACKATHON : FONCTION MAIN */
int main(void)
{
  // IGNORER : INITIALISATIONS
  HAL_Init();
  Dist_Obst = 0;
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
  HAL_SuspendTick(); // suppression des Ticks interrupt pour le mode sleep.
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4); // Start PWM motor
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);	// Start PWM motor
  HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);//Start Encoder
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);//Start Encoder
  HAL_TIM_Base_Start_IT(&htim2);  // Start IT sur font montant PWM
  HAL_ADC_Start_IT( &hadc1); // Active l'interruption de l'ADC (pour l'analog Watchdog)
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2);
  HAL_TIM_Base_Start(&htim1);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint16_t) ANGLE_ZERO);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  stop_sonar();
  moteurs = new BlocMoteurs();
  moteurs->motors_on();
  odometrie = new Odometrie(_min_update_period / 100);
  pid = new ControleurPID(moteurs,odometrie);
  sonar = new Sonar();
  //INIT DEFILEUR
  taches = new FileTaches();
  TLoop=0;
  while(CMDE!=1)
  {
	  HAL_ADC_Start_IT( &hadc1);
	  if(TLoop>=T_2_S)
	  {
		  if(pinstate== GPIO_PIN_RESET)
		  {
			  pinstate = GPIO_PIN_SET;
		  }
		  else
		  {
			  pinstate= GPIO_PIN_RESET;
		  }

		  TLoop=0;
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, pinstate);
	  }
  }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  TLoop=0;

  /* HACKATHON : Liste d'instructions possibles :

     taches->addLigne(X,Y);
     taches->addRotation(Theta);
     taches->addWait(number);
     taches->sonarscan();
     sonar->getValue(left); //dernières valeures mesurées du sonar //retourne la distance en mm (double)
     sonar->getValue(right)
     sonar->getValue(front);
     odometrie->getX();
     odometrie->getY();
     odometrie->getThetaRadian();
    */


  /* HACKATHON : Routine INITIALE */
  defileur = new DefileurTaches(taches, pid,sonar);
  TLoop=0;

  //taches->sonarscan();


  /* HACKATHON : Boucle principale */
  while (1)
  {
	  HAL_ADC_Start_IT( &hadc1); //sert à la surveillance batterie
	  if(TLoop++>=T_10_MS) // On actualise la file de tache toute les 10ms
	  {
		  TLoop=0;
		  if(defileur->update()) //on updtate et  si on arrive à la fin de la file de tache, on agit.
		  {
			  leftsonar = sonar->getValue(left); //dernières valeures mesurées du sonar
			  rightsonar = sonar->getValue(right);
			  frontsonar = sonar->getValue(front);
			  xpos = odometrie->getX();
			  ypos = odometrie->getY();
			  theta = odometrie->getThetaRadian();
		  }
	  }
  }
}
// Fonctions annexes : ignorer
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_NVIC_Init(void)
{
  /* EXTI15_10_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  /* USART3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
}

/* Commandes Start et Stop via l’IT EXTI13 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

	static unsigned char TOGGLE = 0;

	if (TOGGLE)
		CMDE = 0;
	else
		CMDE = 1;
	TOGGLE = ~TOGGLE;

}

/* Trigger sonar */
void trig_sonar(void) {

	HAL_GPIO_WritePin(GPIOB,Trig_sonar_Pin,GPIO_PIN_SET);
}

/* Stop sonar */
void stop_sonar(void) {

	HAL_GPIO_WritePin(GPIOB,Trig_sonar_Pin,GPIO_PIN_RESET);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	if(htim->Instance == TIM2 )
	{
		Time++;
		TLoop++;
	}
}

/* Mesure de la distance avec le sonar */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {


	if ( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2 ){
		Dist_Obst = (uint32_t) HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_2);
		Dist_Obst_cm += (double) ( 650*Dist_Obst) / 65535; // en cm
		HAL_GPIO_WritePin(GPIOB,Trig_sonar_Pin,GPIO_PIN_SET);
		if(sonarcount++>9)
		{
			sonarmeasure = (10*Dist_Obst_cm)/10;
			sonarcount=0;
			Dist_Obst_cm = 0;
			if( sonarmeasure < 450 )
			{
				HAL_GPIO_WritePin(ledFR_GPIO_Port, ledFR_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(ledFL_GPIO_Port, ledFL_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(ledBR_GPIO_Port, ledBR_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(ledBL_GPIO_Port, ledBL_Pin, GPIO_PIN_SET);
			}
			else
			{
				HAL_GPIO_WritePin(ledFR_GPIO_Port, ledFR_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(ledFL_GPIO_Port, ledFL_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(ledBR_GPIO_Port, ledBR_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(ledBL_GPIO_Port, ledBL_Pin, GPIO_PIN_RESET);
			}
		}
	}

}

/* Interrupt de l'ADC pour l'Analog Watchdog. Allume Led 2 si la batterie est faible, désactive l'interruption pour la suite */
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
}

void Error_Handler(void)
{
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */


// HACKATHON : Exemples de code

//--------------------//
//CALIBRATION ROTATION//
//MODIFIER ECART ROUE //
//ET COEFFDIFGD JUSQUE//
//ATTEINDRE UN MOUVEMENT//
//SUFFISAMMENT PRECIS//
//--------------------//


/*
for(int i = 0; i<4; i ++){

	  taches->addLigne(400,0);
	  taches->addRotation(-PI/2);
	  taches->addWait(10000);
	  taches->addRotation(-PI);
	  taches->addWait(10000);
	  taches->addRotation(PI/2);
	  taches->addWait(10000);
	  taches->addRotation(0);
	  taches->addWait(10000);
	  taches->addRotation(-PI/2);
	  taches->addWait(10000);
	  taches->addLigne(400,-500);
	  taches->addRotation(PI);
	  taches->addWait(10000);
	  taches->addRotation(PI/2);
	  taches->addWait(10000);
	  taches->addRotation(0);
	  taches->addWait(10000);
	  taches->addRotation(-PI/2);
	  taches->addWait(10000);
	  taches->addRotation(PI);
	  taches->addWait(10000);
	  taches->addLigne(0,-500);
	  taches->addWait(10000);
	  taches->addRotation(PI/2);
	  taches->addWait(10000);
	  taches->addRotation(0);
	  taches->addWait(10000);
	  taches->addRotation(-PI/2);
	  taches->addWait(10000);
	  taches->addRotation(PI);
	  taches->addWait(10000);
	  taches->addRotation(PI/2);
	  taches->addWait(10000);
	  taches->addLigne(0,0);
	  taches->addRotation(0);
	  taches->addWait(10000);
	  taches->addRotation(-PI/2);
	  taches->addWait(10000);
	  taches->addRotation(PI);
	  taches->addWait(10000);
	  taches->addRotation(PI/2);
	  taches->addWait(10000);
	  taches->addRotation(0);
	  taches->addWait(10000);

}
*/




/*
for(int i = 0; i< 8; i ++)
{

	  for(int k=0; k<4; k++)
	  {
		  taches->addRotation(k*PI/2);
		  if(k==0)
		  {
			  taches->addWait(50000);
		  }
		  else
		  {
			  taches->addWait(20000);
		  }

	  }
}
*/

/*
for(int i = 0; i< 8; i ++)
{

	  for(int k=0; k<4; k++)
	  {
		  taches->addRotation(k*PI/2);
		  if(k==0)
		  {
			  taches->addWait(50000);
		  }
		  else
		  {
			  taches->addWait(20000);
		  }

	  }
}

*/



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
