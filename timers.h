/**
 * @file timers.h
 *
 * @brief Timers driver interface file.
 *
 * It implements a timers handling with TIM2 STM32 hardware device.
 * The timers resolution is 10 msec (look timerINTERRUPT_FREQUENCY).
 *
 * @version	0.1
 * @date	28 January 2009
 * @author	Mauro Gamba
 * 
 */

#ifndef TIMERS_H_
#define TIMERS_H_

/* Error codes definitions */
#define tmOK      0
#define tmERROR   1

/* Timeout definitions */
#define TIMEOUT_10_MSEC		1
#define TIMEOUT_70_MSEC		7
#define TIMEOUT_100_MSEC	10

///* Timeout definitions */
//#define TIMEOUT_10_MSEC		10
//#define TIMEOUT_70_MSEC		70
//#define TIMEOUT_100_MSEC	100

/* Typedef definitions */
typedef struct
{
  bool TimerArmed;  /* TRUE if timer is  armed */
  u32 TimeoutTick; /* Timeout tick counter */
  pFunction TimerHandler; /* Timeout function handler */
} Timers_t;

/**
 * @fn		Timer_Init(void)
 *
 * @brief 	Initialize Timers driver and related data.
 *
 * @return Nothing.
 */
void Timer_Init(void);

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
u8 Timer_Start(Timers_t *timer, pFunction FunctionPnt, u16 millisec);

/**
 *  @fn Timer_Stop(Timers_t *timer)
 *
 *  Stop a Timer.
 *
 *  @param timer 		Pointer to the timer structure.
 *
 *  @return Nothing.
 */
void Timer_Stop(Timers_t *timer);

/**
 *  @fn Timer_Check(void)
 *
 *  Check if there's timers elapsed and launch timeout function if necessary.
 *
 *  @return Nothing.
 */
void Timer_Check(void);

/**
 *  @fn 	Timer_IsActive(void)
 *
 *  @brief	Check if there's timers active.
 *
 *  @return TRUE if there's timers active, FALSE otherwise.
 */
bool Timer_IsActive(void);

#endif /* TIMERS_H_ */
