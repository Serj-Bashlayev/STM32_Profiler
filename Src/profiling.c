/***********************************************************************
 File Name    : 'profiling.c'
 Title        : PROFILER
 Description  : Code time profiler with output to ITM Stimulus Port 0
                Debug (printf) Viewer
                Time accuracy 1µS

                Examle output:
                Profiling "Start" sequence:
                --Event-----------------------|--timestamp--|---delta_t----
                GLCD_Init                     :       41 us | +       41 us
                OLED_Init                     :     5288 us | +     5247 us
                u8g_SetFont                   :     5292 us | +        4 us
                HAL_Delay(10)                 : 10004967 us | +  9999675 us

 Author       : Serj Bashlayev
                https://github.com/Serj-Bashlayev
                email: phreak_ua@yahoo.com
 Created      : 16/08/2016
 Revised      : 04/10/2018
 Version      : 3.0
 Target MCU   : STM32
 Compiler     : ARM Compiler v5.04 for µVision armcc
 Editor Tabs  : 2
***********************************************************************/

/* Includes ----------------------------------------------------------*/
#include "profiling.h"

/* Private Definitions -----------------------------------------------*/
#define DEBUG_PRINTF printf
#define __PROF_STOPED 0xFF

/* External variables ------------------------------------------------*/
/* Private variables -------------------------------------------------*/
static uint32_t   time_start; // profiler start time
static const char *prof_name; // profiler name
static uint32_t   time_event[MAX_EVENT_COUNT]; // events time
static const char *event_name[MAX_EVENT_COUNT]; // events name
static uint8_t    event_count = __PROF_STOPED; // events counter

/* Private function prototypes ---------------------------------------*/
/* -------------------------------------------------------------------*/

/**
 * redefinition fputc() for output printf(..) to ITM Stimulus Port 0
 */
struct __FILE { int handle; /* Add whatever needed */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f)
{
  ITM_SendChar(ch);
  return(ch);
}


/**
 * @brief Start profiler, save profiler name and start time
 *
 * @param profile_name Profiler name
 */
void PROFILING_START(const char *profile_name)
{
  prof_name = profile_name;
  event_count = 0;

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // enable counter
  //DWT->CYCCNT  = time_start = 0;
  time_start = DWT->CYCCNT;
}


/**
 * @brief  Event. Save events name and time
 *
 * @param event Event name
 */
void PROFILING_EVENT(const char *event)
{
  if (event_count == __PROF_STOPED)
    return;

  if (event_count < MAX_EVENT_COUNT)
  {
    time_event[event_count] = DWT->CYCCNT;
    event_name[event_count] = event;
    event_count++;
  }
}


/**
 * @brief Stop profiler. Print event table to ITM Stimulus Port 0
 */
void PROFILING_STOP(void)
{
  int32_t tick_per_1us;
  int32_t time_prev;
  int32_t timestamp;
  int32_t delta_t;

  tick_per_1us = SystemCoreClock / 1000000;

  if (event_count == __PROF_STOPED)
  {
    DEBUG_PRINTF("\r\nWarning: PROFILING_STOP WITHOUT START.\r\n");
    return;
  }

  DEBUG_PRINTF("Profiling \"%s\" sequence: \r\n"
               "--Event-----------------------|--timestamp--|----delta_t---\r\n", prof_name);
  time_prev = 0;

  for (int i = 0; i < event_count; i++)
  {
    timestamp = (time_event[i] - time_start) / tick_per_1us;
    delta_t = timestamp - time_prev;
    time_prev = timestamp;
    DEBUG_PRINTF("%-30s:%9d µs | +%9d µs\r\n", event_name[i], timestamp, delta_t);
  }
  DEBUG_PRINTF("\r\n");
  event_count = __PROF_STOPED;
}

