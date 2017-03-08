#ifndef BOARD_H
#define BOARD_H

#include "stm32l0xx_hal.h"


//LED

#define LED1_PORT GPIOA
#define LED1 GPIO_PIN_11

#define LED2_PORT GPIOA
#define LED2 GPIO_PIN_8

#define LED3_PORT GPIOB
#define LED3 GPIO_PIN_12


//IR Control
#define IR_EN2_PORT GPIOA
#define IR_EN2 GPIO_PIN_2

#define IR_EN1_PORT GPIOB
#define IR_EN1 GPIO_PIN_0
//PIR Select
#define PIR_S1_PORT GPIOB
#define PIR_S1 GPIO_PIN_13

#define PIR_S2_PORT GPIOB
#define PIR_S2 GPIO_PIN_14

//4G Port
#define LTE_CLOSE_PORT GPIOB
#define LTE_CLOSE GPIO_PIN_4

#define LTE_RESET_PORT GPIOB
#define LTE_RESET GPIO_PIN_6

#define LTE_WAKEUP_PORT GPIOA
#define LTE_WAKEUP_PIN GPIO_PIN_12
//**************

//GPS Enable
#define GPS_EN_PORT GPIOB
#define GPS_EN GPIO_PIN_15

//MV Enable
#define MV_EN_PORT GPIOB
#define MV_EN GPIO_PIN_8

//S2L On_off
#define ON_OFF_PORT GPIOA
#define ON_OFF GPIO_PIN_15

//SPI_CS
#define SPI_CS_PORT GPIOA
#define SPI_CS GPIO_PIN_4




/***************************input exti*****************************/
//GPIO_PIN_13
#define POWER_PORT GPIOC
//GPIO_PIN_1
#define AC_IT1 GPIOB
//GPIO_PIN_2
#define AC_IT2 GPIOB
//GPIO_PIN_5
#define LTE_IT GPIOB
//GPIO_PIN_9
#define MW_IT GPIOB

//










#endif