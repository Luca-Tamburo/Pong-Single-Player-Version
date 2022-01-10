/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_timer.c
** Last modified Date:  09-01-2022
** Last Version:        V1.00
** Descriptions:        functions to manage T0, T1 and T2 interrupts
** Correlated files:    timer.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include <stdio.h>
#include "lpc17xx.h"
#include "timer.h"
#include "../GLCD/GLCD.h"
#include "../ADC/adc.h"
#include "../RIT/RIT.h"

/******************************************************************************
** Function name:			Timer0_IRQHandler
**
** Descriptions:			Timer/Counter 0 interrupt handler
**
** parameters:				None
** Returned value:		None
**
******************************************************************************/


static double valore_tan_15 = 0.2679;						/* Tangent value at 15 degrees. */
static double valore_tan_45 = 1;								/* Tangent value at 45 degrees */
int primo_tocco = 1;														
static double tan = -1;													

/* Ball variables. */

static uint16_t ball_side = 158;								/* Bottom of the ball */
static uint16_t ball_side_left = 229;						/* Left side of the ball */
static uint16_t ball_side_right = 233;					/* Right side of the ball. */
static int ball = 1;														/* Ball = 1 goes downwards, otherwise it goes upwards. */
static int move_ball = 0;												/* move_ball = 0 the ball goes from right to left, otherwise the opposite. */

/* Paddle variables. */

extern uint16_t start_paddle;		  							/* Paddle start. */
extern uint16_t stop_paddle;										/* End of paddle. */
extern uint16_t paddle_base;										/* Paddle base. */

/* Border variables. */

uint16_t x_right = 234;													/* Right border. */
uint16_t x_left = 6;														/* Left border. */			
uint16_t x_top = 6;															/* Top border. */
uint16_t x_down = 278; 													/* Bottom border. */

/* Score variables. */

extern int game_status;																		
extern int current_score;																			 
extern int record_max;													
extern char record[];
extern char score[];

static int score_left = 10;										
static int score_right = 25;
static int score_up = 145;
static int score_down = 170;

/* Record variables. */

static int record_left = 180;
static int record_right = 190;
static int record_up = 0;
static int record_down = 25;

static int record_text_left = 100;
static int record_text_right = 180;
static int record_text_up = 0;
static int record_text_down = 25;

static int update_element = 0;								
static int choose_element = 0;								

/* Notes variables. */

int note_lower = 106;															/* E8 - Bouncing on border. */
int note_higher = 133;														/* C8 - Bouncing on paddle. */

/******************************************************************************
** Function name:			Timer0_IRQHandler
**
** Descriptions:			Timer/Counter 0 interrupt handler
**
** parameters: 				None
** Returned value:		None
******************************************************************************/

void TIMER0_IRQHandler (void)
{
	ADC_start_conversion();
	
  LPC_TIM0->IR = 1;			/* clear interrupt flag */
  return;
}

/******************************************************************************
** Function name:			Timer1_IRQHandler
**
** Descriptions:			Timer/Counter 1 interrupt handler
**
** parameters: 				None
** Returned value:		None
******************************************************************************/

void TIMER1_IRQHandler (void)	
{  
	int i = 0;
	
	/* Variables used to calculate the angle after the ball hits the paddle. */

	double relative_intersect_x;
	double normalized_relative_intersect_x;
	
	/* New ball value variables. */
	
	uint16_t new_ball_side_left;																		/* New value of the left side of the ball. */
	uint16_t new_ball_side_right;																		/* New value of the right side of the ball. */
	uint16_t new_ball_side;																					/* New ball base value. */
	
	/* Initial case, i.e. at the beginning of the game. */
	
	if(primo_tocco == 1){
		ball = 1;
		move_ball = 0;
		tan = -valore_tan_45;
		
		ball_side = 158;
		ball_side_left = 228;
		ball_side_right = 233;
	}
	
	/* Post game start case. */
	
	if(!primo_tocco){
		/* Ball touches the border. */
		
		/* Ball goes down and touches the left wall. */
		if(ball && ball_side_left <= (x_left + 2)){														
			move_ball = 1;
			if(tan < 0){																									
				tan = -tan;
			}
			init_timer(2, note_lower);
			enable_timer(2);
		/* Ball goes down and touches the right wall. */
		} else if (ball && ball_side_right >= (x_right - 2)){ 								
			move_ball = 0;
			if(tan > 0){
				tan = -tan;
			}
			init_timer(2, note_lower);
			enable_timer(2);
		/* Ball goes up, touches the top of the board and the ball moves from left to right. */
		} else if (!ball && ball_side <= (x_top + 1) && move_ball == 1){ 	
			ball = 1;
			if(tan < 0){
				tan = -tan;
			}
			init_timer(2, note_lower);
			enable_timer(2);
		/* Ball goes up, touches the top of the board and the ball moves from right to left. */
		} else if (!ball && ball_side <= (x_top + 1) && move_ball == 0){ 
			ball = 1;
			if(tan > 0){
				tan = -tan;
			}
			init_timer(2, note_lower);
			enable_timer(2);
		/* Ball goes up and touches the right side of the board. */
		} else if (!ball && ball_side_right >= (x_right - 2)){ 			
			move_ball = 0;
			if(tan < 0){
				tan = -tan;
			}
			init_timer(2, note_lower);
			enable_timer(2);
		/* Ball goes up and touches the left side of the board. */
		} else if (!ball && ball_side_left <= (x_left + 2)){ 					
			move_ball = 1;
			if(tan > 0){
				tan = -tan;
			}
			init_timer(2, note_lower);
			enable_timer(2);
			
		/* Ball touches paddle. */
			
		} else if (ball && (ball_side + 5) == paddle_base && ball_side_left >= start_paddle && ball_side_right <= stop_paddle) {
			/* The direction of the ball is decided when the ball hits the paddle. */
			ball = 0;
			/* Relative position of the ball. Indicates the point where the paddle and ball meet.*/
			relative_intersect_x = (start_paddle + (stop_paddle - start_paddle)/2) - (ball_side_left);	
			normalized_relative_intersect_x = (relative_intersect_x/(stop_paddle - start_paddle)/2);
			/* Bounce angle takes values from -45 to 45 degrees. */
			tan = normalized_relative_intersect_x * valore_tan_45;
			/* If the bounce angle is less than zero, the direction tends to be horizontal, otherwise it will be 15 degrees. */
			if(tan > -0.25 && tan <= 0){ 																	
				tan = - valore_tan_15;
			} else if(tan < 0.25 && tan >= 0){ 
				tan = valore_tan_15;
			}	
			if(tan < 0){
				move_ball = 1;
			} else {
				move_ball = 0;
			}
			
			if(current_score >= 100){	
				current_score += 10;
			} else {
				current_score += 5;
			}
			
			if(current_score > record_max){
				record_max = current_score;
				sprintf(record, "%d", record_max);
				GUI_Text(100, 10, (uint8_t *) " Record: ", White, Black);
				GUI_Text(190, 10, (uint8_t *) record, White, Black);
			} 
				sprintf(score, "%d", current_score);
				GUI_Text(10, 160, (uint8_t *) score, White, Black);
				
				init_timer(2, note_higher);
				enable_timer(2);
		}
	}

	/*If the ball drops increase the side value, otherwise decrease it. */
	if(ball == 1){																							
		new_ball_side = ball_side + 1;
	} else {
		new_ball_side = ball_side - 1;
	}
	
	/* Angular coefficient */
	new_ball_side_left = (new_ball_side + (tan * ball_side_left) - ball_side) / tan;
	new_ball_side_right = new_ball_side_left + 4;

	/* GAME OVER, meaning the ball goes under the paddle and you lose. */
	if(ball && ((ball_side + 5) > paddle_base + 20)){
		game_status = 3;
		GUI_Build(3);
		NVIC_DisableIRQ(TIMER0_IRQn);
		NVIC_DisableIRQ(TIMER1_IRQn);
		return;
	} 
	
	/* Case where the ball passes over the record or score and clears it. */
	if(!update_element && new_ball_side_left >= record_left && new_ball_side_right <= record_right && new_ball_side <= record_down && (new_ball_side - 5) >= record_up){
		choose_element = 1;
		update_element = 1;
	} else if (choose_element == 1 && update_element == 2 && (new_ball_side_left < record_left || (new_ball_side > record_down))){
		update_element = 3;
	}
	
	if(!update_element && new_ball_side_left >= record_text_left && new_ball_side_right <= record_text_right && new_ball_side <= record_text_down && (new_ball_side - 5) >= record_text_up){
		choose_element = 2;
		update_element = 1;
	} else if (choose_element == 2 && update_element == 2 && ((new_ball_side_left < record_text_left || new_ball_side_right > record_text_right) || new_ball_side > record_text_down)){
		update_element = 3;
	}
	
	if(!update_element && new_ball_side_left >= score_left && new_ball_side_right <= score_right && new_ball_side <= score_down && (new_ball_side - 5) >= score_up){
		choose_element = 3;
		update_element = 1;
	} else if (choose_element == 3 && update_element == 2 && (new_ball_side_right > score_right || (new_ball_side > score_down || (new_ball_side - 5) < score_up))){
		update_element = 3;
	}
	
	if(update_element == 1){
		switch(choose_element){
			case 1:
				GUI_Text(190, 10, (uint8_t *) record, Black, Black);
				break;
			case 2:
				GUI_Text(100, 10, (uint8_t *) " Record: ", White, Black);
				break;
			case 3:
				GUI_Text(10, 160, (uint8_t *) score, Black, Black);
				break;
			default:
				break;
		}
		update_element = 2;
	} else if(update_element == 3){
			switch(choose_element){
				case 1:
					GUI_Text(190, 10, (uint8_t *) record, White, Black);
					break;
				case 2:
					GUI_Text(100, 10, (uint8_t *) " Record: ", White, Black);
					break;
				case 3:
					GUI_Text(10, 160, (uint8_t *) score, White, Black);
					break;
				default:
					break;
			}
		update_element = 0;
	}
	
	for(i = 0; i < 5; i++){
		LCD_DrawLine(ball_side_left, ball_side, ball_side_right, ball_side, Black);
		ball_side++;
	}
	
	for(i = 0; i < 5; i++){
		LCD_DrawLine(new_ball_side_left, new_ball_side, new_ball_side_right, new_ball_side, Green);
		new_ball_side++;
	}
	
	ball_side_left = new_ball_side_left;
	ball_side_right = new_ball_side_right;
	ball_side = new_ball_side - 5;

	if(primo_tocco){
		primo_tocco = 0;
	}
		
	LPC_TIM1->IR = 1;			/* clear interrupt flag */
  return;
}


/******************************************************************************
** Function name:			Timer2_IRQHandler
**
** Descriptions:			Timer/Counter 2 interrupt handler
**
** parameters: 				None
** Returned value:		None
******************************************************************************/

/* k=1/f'*f/n  k=f/(f'*n) k=25MHz/(f'*45) */


uint16_t SinTable[45] =                                       /* ÕýÏÒ±í                       */
{
    410, 467, 523, 576, 627, 673, 714, 749, 778,
    799, 813, 819, 817, 807, 789, 764, 732, 694, 
    650, 602, 550, 495, 438, 381, 324, 270, 217,
    169, 125, 87 , 55 , 30 , 12 , 2  , 0  , 6  ,   
    20 , 41 , 70 , 105, 146, 193, 243, 297, 353
};

void TIMER2_IRQHandler (void)
{
	static int ticks = 0;
	
	/* DAC management */	
	LPC_DAC->DACR = SinTable[ticks] << 6;
	ticks++;
	
	if(ticks == 45){
		ticks = 0;
		disable_timer(2);
		reset_timer(2);
	}
	
	LPC_TIM2->IR = 1;			/* clear interrupt flag */
  return;
}

/******************************************************************************
** Function name:			Timer3_IRQHandler
**
** Descriptions:			Timer/Counter 3 interrupt handler
**
** parameters: 				None
** Returned value:		None
******************************************************************************/

void TIMER3_IRQHandler (void)
{	
	LPC_TIM3->IR = 1;			/* clear interrupt flag */
  return;
}


/******************************************************************************
**                            End Of File
******************************************************************************/
