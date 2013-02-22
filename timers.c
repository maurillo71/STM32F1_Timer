/**
 * @file timers.c
 *
 * @brief Timers driver.
 *
 * It implements a timers handling with TIM2 STM32 hardware device.
 * The timers resolution is 10 msec (look timerINTERRUPT_FREQUENCY).
 *
 * @version	0.1
 * @date	28 January 2009
 * @author	Mauro Gamba
 * 
 */

#include <stddef.h>

/* Library includes. */
#include "stm32f10x_it.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "digital_output.h" /* DEBUG */

#include "timers.h"
extern u8 toggle;
/*===========================================================================*
                        Private Definitions
 *===========================================================================*/
#define MAX_TIMER       8

#define timerINTERRUPT_FREQUENCY		100    /* 10 msec timer */
//#define timerINTERRUPT_FREQUENCY		1000   /* 1 msec timer */
#define timerPRESCALER					15
#define timerFREQUENCY					((configCPU_CLOCK_HZ) / (timerPRESCALER+1))
//#define timerFREQUENCY					configCPU_CLOCK_HZ / (timerPRESCALER + 1)

/* The highest available interrupt priority. */
#define timerHIGHEST_PRIORITY			( 0 )

/*===========================================================================*
                                 Private Data
 *===========================================================================*/
static Timers_t *Timers_List[MAX_TIMER];
static u32 TickCounter; /* Timer Counter */

/*===========================================================================*
                       Public Functions Implementation
 *===========================================================================*/

/**
 * @fn		Timer_Init(void)
 *
 * @brief 	Initialize Timers driver and related data.
 *
 * @return Nothing.
 */
void Timer_Init(void)
{
  u8 idx;
  u32 ulFrequency;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  for (idx = 0;idx < MAX_TIMER;idx++)
  {
    Timers_List[idx] = NULL;
  }

  /*** Init timer and tick counter ***/
  TickCounter = 0;

  /* Enable timer clocks */
  RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );

  /* Initialise data. */
  TIM_DeInit( TIM2 );

  TIM_TimeBaseStructInit( &TIM_TimeBaseStructure );

  /* Time base configuration for timer 2 - which generates the interrupts. */
  ulFrequency = timerFREQUENCY / timerINTERRUPT_FREQUENCY;
  TIM_TimeBaseStructure.TIM_Period = ( unsigned portSHORT ) ( ulFrequency & 0xffffUL );
  TIM_TimeBaseStructure.TIM_Prescaler = timerPRESCALER;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit( TIM2, &TIM_TimeBaseStructure );
  TIM_ARRPreloadConfig( TIM2, ENABLE );

  /* Enable TIM2 interrupt. */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQChannel;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = timerHIGHEST_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init( &NVIC_InitStructure );

  TIM_ITConfig( TIM2, TIM_IT_Update, ENABLE );

  /* Finally, enable the timer. */
  TIM_Cmd( TIM2, ENABLE );
}

/**
 *  @fn Timer_Start(Timers_t *timer, pFunction FunctionPnt, u16 millisec)
 *
 *  Start a Timer.
 *
 *  @param timer 		Pointer to the timer structure.
 *  @param FunctionPnt 	Timeout function handler.
 *  @param millisec		Timer elapse time ( in 10msec ).
 *
 *  @return tmOK if timer is started correctly.
 *  @return tmERROR if timer can't start.
 */
u8 Timer_Start(Timers_t *timer, pFunction FunctionPnt, u16 millisec)
{
  u32 FinalTickCnt,ActualTickCounter;
  u8 ret_code,idx;

  ret_code = tmERROR;

  /**
   *  First of all search if timer is in the list or
   *  if we have to allocate another one.
   **/
  if (timer->TimerArmed == TRUE)
  {
    /*** Timer is already armed: Search it in the list ***/
    for (idx = 0;idx < MAX_TIMER; idx++)
    {
      if (Timers_List[idx] == timer)
      {
        break;
      }
    }
  }
  else
  {
    /*** Search unused timer ***/
    for (idx = 0;idx < MAX_TIMER; idx++)
    {
      if (Timers_List[idx] == NULL)
      {
        break;
      }
    }
  }

  if (idx<MAX_TIMER)
  {
	/*** Disable interrupt ***/
	vPortEnterCritical();

    /* Save Actual Tick Counter */
    ActualTickCounter = TickCounter;

    /* Calculate final Tick counter */
    /* We have a timer every 500usec: So millisec has to be multiplied by 2 */
//    FinalTickCnt = ActualTickCounter + ((u32)(millisec)<<1);
    /* We have a timer every 10 msec */
    FinalTickCnt = ActualTickCounter + (u32)(millisec);

    Timers_List[idx] = timer;
    Timers_List[idx]->TimeoutTick = FinalTickCnt;
    Timers_List[idx]->TimerHandler = FunctionPnt;

    Timers_List[idx]->TimerArmed = TRUE;

    /*** Enable interrupt ***/
    vPortExitCritical();

    ret_code = tmOK;
  }

  return (ret_code);
}

/**
 *  @fn Timer_Stop(Timers_t *timer)
 *
 *  Stop a Timer.
 *
 *  @param timer 		Pointer to the timer structure.
 *
 *  @return Nothing.
 */
void Timer_Stop(Timers_t *timer)
{
	u8 idx;

    /*** Search it in the list ***/
    for (idx = 0;idx < MAX_TIMER; idx++)
    {
      if (Timers_List[idx] == timer)
      {
    	  /*** Disable interrupt ***/
    	  vPortEnterCritical();

    	  Timers_List[idx]->TimerArmed = FALSE;
    	  Timers_List[idx] = NULL;

    	  /*** Enable interrupt ***/
    	  vPortExitCritical();
      }
    }

}

/**
 *  @fn Timer_Check(void)
 *
 *  Check if there's timers elapsed and launch timeout function if necessary.
 *
 *  @return Nothing.
 */
void Timer_Check(void)
{
  u32 ActualTickCounter;
  u8 idx;
  Timers_t *TempTimer;

  /*** save actual tick counter ***/
  ActualTickCounter = TickCounter;

  for (idx = 0; idx < MAX_TIMER; idx++)
  {
    if (Timers_List[idx] != NULL)
    {
    	if ( (Timers_List[idx]->TimerArmed == TRUE) &&
             (Timers_List[idx]->TimeoutTick < ActualTickCounter) )
    	{
        	/*** Disable interrupt ***/
    		vPortEnterCritical();
			TempTimer = Timers_List[idx];
    		Timers_List[idx]->TimerArmed = FALSE;
			Timers_List[idx] = NULL;
	    	/*** Enable interrupt ***/
			vPortExitCritical();

			TempTimer->TimerHandler();
    	}
    }
  }
}

/**
 *  @fn 	Timer_IsActive(void)
 *
 *  @brief	Check if there's timers active.
 *
 *  @return TRUE if there's timers active, FALSE otherwise.
 */
bool Timer_IsActive(void)
{
	  u8 idx;
	  bool RetCode;

	  RetCode = FALSE;
	  for (idx = 0; idx < MAX_TIMER; idx++)
	  {
	    if (Timers_List[idx] != NULL)
	    {
	    	RetCode = TRUE;
	    	break;
	    }
	  }
	  return (RetCode);
}

/**
 * @fn 		vTIM2InterruptHandler(void)
 *
 * @brief	TIM2 interrupt handler routine.
 * 			It generate a 1 msec interrupt.
 *
 * @return 	Nothing.
 */
void vTIM2InterruptHandler(void)
{
	TickCounter++;

//	/* DEBUG CODE */
//	if (TickCounter % 2)
//	{
//		EV_OPEN_LOW();
//	}
//	else
//	{
//		EV_OPEN_HIGH();
//	}
	TIM_ClearITPendingBit( TIM2, TIM_IT_Update );
}
