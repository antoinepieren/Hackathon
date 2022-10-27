#include "sonar.h"


#define ANGLE_ZERO 2100
#define ANGLE_90  4000
#define ANGLE90  750
#define TimeToServo 1500 //1.5s
extern int Time;
extern float sonarmeasure;


Sonar::Sonar()
{

	leftdist=0;
	frontdist=0;
	rightdist=0;
	timestamp=0;
	HAL_GPIO_WritePin(GPIOB,Trig_sonar_Pin,GPIO_PIN_SET); //TRIG sonar
}

Sonar::~Sonar()
{
	HAL_GPIO_WritePin(GPIOB,Trig_sonar_Pin,GPIO_PIN_RESET); //STOP sonar

}

void Sonar::lookDir(int dirSonar)
{
	timestamp=Time;
	switch(dirSonar)
	{
		case left:
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint16_t) ANGLE_90);
			while( Time < timestamp + TimeToServo)
			leftdist = sonarmeasure;
		case front:
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint16_t) ANGLE_ZERO);
			while( Time < timestamp + TimeToServo);
			frontdist = sonarmeasure;

		case right:
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint16_t) ANGLE90);
			while( Time < timestamp + TimeToServo);
			rightdist = sonarmeasure;
		default :
			break;
	}

}

float Sonar::getValue(int dirSonar)
{
	switch(dirSonar)
	{
		case left:
			return leftdist;
		case front:
			return frontdist;
		case right :
			return rightdist;
		default :
			return 0;
	}
}

void Sonar::update(){
	for(int i=0; i<3; i++)
	{
		lookDir(i);

	}
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint16_t) ANGLE_ZERO);

}
