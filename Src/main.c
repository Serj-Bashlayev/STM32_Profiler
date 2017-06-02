/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.1.2
  * @date    14-August-2015
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f30x.h"
#include "profiling.h"
#include <stdbool.h>

extern __IO int32_t Tick;
/* Private variables ---------------------------------------------------------*/
static int32_t delay_tick;

/* Private function prototypes -----------------------------------------------*/
static void Init_TIM6(void);
static void Init_IO(void);


/**
 * @brief Timer6 interrupt handler (1 kHz)
 */
void TIM6_DAC_IRQHandler(void)
{
  static uint16_t delayT6_sec = 0;
  static bool trigger = false;

  if (TIM_GetITStatus(TIM6, TIM_IT_Update) == SET)
  {
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    // Прерывание по переполнению TIM6
    // Моргаем светодиодом 1Гц
    if (++delayT6_sec >= 500)
    {
      delayT6_sec = 0;
      trigger = !trigger;
      GPIO_WriteBit(GPIOE, GPIO_Pin_8, (trigger ? Bit_SET : Bit_RESET));
    }
  }
}


/**
 * @brief MAIN
 */
int main(void)
{
  SysTick_Config(SystemCoreClock / 1000);

  PROFILING_START("MAIN startup timing");

  Init_IO();
  PROFILING_EVENT("IO_Init()");

  Init_TIM6();
  PROFILING_EVENT("TIM6_Init()");
  PROFILING_STOP();

  while (1)
  {
    PROFILING_START("MAIN loop timing");

    GPIO_WriteBit(GPIOE, GPIO_Pin_9, (((Tick % 1000) > 500) ? Bit_SET : Bit_RESET));
    PROFILING_EVENT("GPIO_WriteBit(...)");

    // Wait for update Tick
    delay_tick = Tick;
    while (delay_tick == Tick);
    PROFILING_EVENT("Wait for update Tick");

    // Delay 1000 ms
    delay_tick = Tick + 1000;
    while (delay_tick > Tick);
    PROFILING_EVENT("DELAY 1 s");

    // Stop profiling and print
    PROFILING_STOP();
  }
}


/**
 * @brief Configure LEDs CPIO PE8, PE9, PE10
 */
static void Init_IO(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* GPIOE clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);

  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
}


/**
 * @brief TIM6 init function. 1kHz interrupt
 */
static void Init_TIM6(void)
{
  NVIC_InitTypeDef        NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

  /* TIM6 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

  /* Enable the TIM6 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1;
  TIM_TimeBaseStructure.TIM_Prescaler = 35999;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

  /* TIM enable counter */
  TIM_Cmd(TIM6, ENABLE);

  /* TIM Interrupts enable */
  TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
}

