/***********************************************************************
 File Name    : 'profiling.c'
 Title        : PROFILER
 Description  : Точность отсчета времени 1µS (1E-6)

                Profiling "Start" sequence:
                --Event-----------------------|--timestamp--|---delta_t----
                GLCD_Init                     :       41 us | +       41 us
                OLED_Init                     :     5288 us | +     5247 us
                u8g_SetFont                   :     5292 us | +        4 us
                HAL_Delay(10)                 : 10004967 us | +  9999675 us

                note: требует коррекции констант в _profiling_sub_time при Fcpu != 72000000 Hz

 Author       : Serj Bashlayev (phreak_ua@yahoo.com)
 Created      : 16/08/2016
 Revised      :
 Version      : 2.0
 Target MCU   : STM32F303VCT6
 Compiler     : ARM Compiler
 Editor Tabs  : 2
***********************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "profiling.h"

extern __IO int32_t Tick;
#define GET_TICK() Tick
//#define GET_TICK() HAL_GetTick()

#define DEBUG_PRINTF printf

/* Private variables ---------------------------------------------------------*/

// Структура для быстрого сохранения метки времени.
struct s_time {
  uint32_t SysTickVal; // <-- SysTick->VAL (dec every CORE clock (HCLK) or HCLK/8)
                       //        see SysTick control and status register (STK_CTRL): CLKSOURCE bit
  uint32_t HalTickVal; // <-- HAL_GetTick()(inc every 1 µs in SysTick_Handler
                       //                   for example HAL_GetTick()
};

struct s_time start; // время старта профайлера
struct s_time times[MAX_PROFILING_EVENT_COUNT]; // время события
const char *name; // имя профайлера
const char *names[MAX_PROFILING_EVENT_COUNT]; // имя события
char event_count = 0xFF; // счетчик событий. ограничен MAX_PROFILING_EVENT_COUNT.
                         // Последующие события игнорируются.
                         // 0xFF - счет не запущен

/* Private function prototypes -----------------------------------------------*/
static void _profiling_get_time(struct s_time *p_time);
static int64_t _profiling_sub_time(const struct s_time *const pA, const struct s_time *const pB);


/**
 * Переопределение fputc() для вывода printf(..) в ITM Stimulus Port 0
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
 * @brief Запускает отсчёт времени, сохраняем имя процесса
 *
 * @param profile_name Имя процесса
 */
void PROFILING_START(const char *profile_name)
{
  name = profile_name;
  _profiling_get_time(&start);
  event_count = 0;
}


/**
 * @brief  Отмечаем событие, запоминаем его название и текущее время
 *
 * @param event Название собятия
 */
void PROFILING_EVENT(const char *event)
{
  if (event_count == 0xFF)
    return;

  if (event_count < MAX_PROFILING_EVENT_COUNT)
  {
    _profiling_get_time(&times[event_count]);
    names[event_count] = event;
    event_count++;
  }
}


/**
 * @brief Заканчиваем отсчёт, выводим в отладочную консоль таблицу стадий
 * и их длительности (время с момента profiling_start(...) и разница между событиями)
 */
void PROFILING_STOP(void)
{
  int64_t deltaTime;
  int64_t timestamp;
	struct s_time time_prev;

  if (event_count == 0xFF)
  {
    DEBUG_PRINTF("\r\nWarning: PROFILING_STOP WITHOUT START.\r\n");
    return;
  }

  DEBUG_PRINTF("\r\nProfiling \"%s\" sequence: \r\n--Event-----------------------|--timestamp--|----delta_t---\r\n", name);

  time_prev.HalTickVal = start.HalTickVal;
  time_prev.SysTickVal = start.SysTickVal;

  for(int i = 0; i < event_count; i++)
  {
    timestamp = _profiling_sub_time(&times[i], &start);
    deltaTime = _profiling_sub_time(&times[i], &time_prev);

    DEBUG_PRINTF("%-30s:%9lld µs | +%9lld µs\r\n", names[i], timestamp, deltaTime);

    time_prev.HalTickVal = times[i].HalTickVal;
    time_prev.SysTickVal = times[i].SysTickVal;
  }

  event_count = 0xFF;
}


/**
 * @brief Заполнение структуры *p_time меткой текущего времени.
 * struct s_time  {
 *   uint32_t SysTickVal; // <-- SysTick->VAL
 *   uint32_t HalTickVal; // <-- HAL_GetTick()
 * };
 * Отсчет с момента запуска SysTick_Config()
 * Время на отладку ограничено HAL_GetTick() <= INT32_MAX
 * 2147483647 mS; /1000 = 2147483 S; /60 = 35791 min; /60 = 596 h; /24 = 24 day
 *
 * @param p_time указатель на структуру для сохранения времени
 */
static void _profiling_get_time(struct s_time *p_time)
{
  uint32_t SysTick_2;
  volatile uint32_t *pSysTick_VAL = &(SysTick->VAL);

	// Double reading SysTick->VAL.
  do
  {
    p_time->SysTickVal = *pSysTick_VAL;
    p_time->HalTickVal = GET_TICK();
    SysTick_2 = *pSysTick_VAL;
  }
  while (SysTick_2 > p_time->SysTickVal);
}


/**
 * @brief Вычисление интервала между событиями (pA-pB)
 * @param pA,pB :указатели на структуру s_time
 *
 * @return int64_t разница вермени в µs (10e-6 s) между pA и pB. Результат может быть отрицательным
 */
static int64_t _profiling_sub_time(const struct s_time *const pA, const struct s_time *const pB)
{
  int64_t result;
  int32_t t_sys;
  int32_t t_hal;

  t_hal = pA->HalTickVal - pB->HalTickVal;

  if (pB->SysTickVal >= pA->SysTickVal)
    t_sys = pB->SysTickVal - pA->SysTickVal;
  else
  {
    t_sys = pB->SysTickVal + 72000 - pA->SysTickVal;
    t_hal -= 1;
  }

  result = t_hal * 1000LL + t_sys / 72;

  return result;
}


/*
void profiling_debug()
{
  name = "Name test 1";
  start.HalTickVal = 10;
  start.SysTickVal = 71999;

  names[0] = "name_0";
  times[0].HalTickVal = 10;
  times[0].SysTickVal = 72000/2-1;

  event_count = 1;
  profiling_stop();

  name = "Name test 3";
  names[1] = "name_1";
  times[1].HalTickVal = 11;
  times[1].SysTickVal = 71999;

  names[2] = "name_2";
  times[2].HalTickVal = 12;
  times[2].SysTickVal = 0;

  event_count = 3;
  profiling_stop();
}
*/

