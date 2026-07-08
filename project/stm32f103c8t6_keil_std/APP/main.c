/**
 ******************************************************************************
 * @file    TIM/ComplementarySignals/main.c
 * @author  MCD Application Team
 * @version V3.6.0
 * @date    20-September-2021
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2011 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/** @addtogroup STM32F10x_StdPeriph_Examples
 * @{
 */

/** @addtogroup TIM_ComplementarySignals
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;// 时基结构体：分频、ARR计数上限
TIM_OCInitTypeDef TIM_OCInitStructure;  // 输出比较结构体：PWM模式、极性、互补通道使能
TIM_BDTRInitTypeDef TIM_BDTRInitStructure;// BDTR刹车死区结构体：死区、BKIN、故障模式
uint16_t TimerPeriod = 0;  // ARR自动重装值，决定PWM频率
uint16_t Channel1Pulse = 0, Channel2Pulse = 0, Channel3Pulse = 0; // CCR1/2/3比较值，决定占空比
NVIC_InitTypeDef NVIC_InitStructure;

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void)
{
    /*!< At this stage the microcontroller clock setting is already configured,
         this is done through SystemInit() function which is called from startup
         file (startup_stm32f10x_xx.s) before to branch to application main.
         To reconfigure the default setting of SystemInit() function, refer to
         system_stm32f10x.c file
       */

    /* System Clocks Configuration */
    RCC_Configuration();

    /* GPIO Configuration */
    GPIO_Configuration();

    /* -----------------------------------------------------------------------
    TIM1 Configuration to:

    1/ Generate 3 complementary PWM signals with 3 different duty cycles:
      TIM1CLK is fixed to SystemCoreClock, the TIM1 Prescaler is equal to 0 so the
      TIM1 counter clock used is SystemCoreClock.
      * SystemCoreClock is set to 72 MHz for Low-density, Medium-density, High-density
      and Connectivity line devices. For Low-Density Value line and Medium-Density
      Value line devices, SystemCoreClock is set to 24 MHz.

      The objective is to generate PWM signal at 17.57 KHz:
      - TIM1_Period = (SystemCoreClock / 17570) - 1

      The Three Duty cycles are computed as the following description:

      The channel 1 duty cycle is set to 50% so channel 1N is set to 50%.
      The channel 2 duty cycle is set to 25% so channel 2N is set to 75%.
      The channel 3 duty cycle is set to 12.5% so channel 3N is set to 87.5%.
      The Timer pulse is calculated as follows:
        - ChannelxPulse = DutyCycle * (TIM1_Period - 1) / 100

    2/ Insert a dead time equal to 11/SystemCoreClock ns
    3/ Configure the break feature, active at High level, and using the automatic
       output enable feature
    4/ Use the Locking parameters level1.
    ----------------------------------------------------------------------- */

    /* Compute the value to be set in ARR register to generate signal frequency at 17.57 Khz */
    TimerPeriod = (SystemCoreClock / 17570) - 1;// ARR值
    /* Compute CCR1 value to generate a duty cycle at 50% for channel 1 */
    Channel1Pulse = (uint16_t)(((uint32_t)5 * (TimerPeriod - 1)) / 10);
    /* Compute CCR2 value to generate a duty cycle at 25%  for channel 2 */
    Channel2Pulse = (uint16_t)(((uint32_t)25 * (TimerPeriod - 1)) / 100);
    /* Compute CCR3 value to generate a duty cycle at 12.5%  for channel 3 */
    Channel3Pulse = (uint16_t)(((uint32_t)125 * (TimerPeriod - 1)) / 1000);

    /* Time Base configuration */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;// 预分频不分频，定时器时钟72M
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;// 向上计数 0→ARR
    TIM_TimeBaseStructure.TIM_Period = TimerPeriod;// ARR上限
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; // 高级定时器重复计数器不用

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* Channel 1, 2 and 3 Configuration in PWM mode */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 开启主通道CHx
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;// 开启互补通道CHxN
    TIM_OCInitStructure.TIM_Pulse = Channel1Pulse; // CCR比较值
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 主通道：低电平有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High; // 互补通道：低电平有效

    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set; // PWM关闭时，主通道置高
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;// PWM关闭时，互补通道拉低

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_OCInitStructure.TIM_Pulse = Channel2Pulse;
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);

    TIM_OCInitStructure.TIM_Pulse = Channel3Pulse;
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);

    /* Automatic Output enable, Break, dead time and lock configuration*/
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;// 运行时故障安全
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;// 闲置时故障安全
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;// 寄存器锁，防止干扰篡改保护参数
    TIM_BDTRInitStructure.TIM_DeadTime = 50;// 硬件死区时间

    /*硬件刹车功能，可以结合着互补PWM在电机MOS驱动，如果上下两MOS同时导通，把此IO接到运放比较器上
    ，过流时候硬件自动拉高此IO，然后直接从硬件层面切断PWM输出，防止上下导通烧毁板子烧MOS*/
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable; // 开启BKIN硬件刹车
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;// 高电平触发刹车
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;/*数器运行时自动允许 PWM 输出，配合 MOE 主输出位。*/


    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);


    // TIM1中断通道
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    // 抢占优先级、子优先级，根据工程需求设置
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    /* TIM1 counter enable */
    TIM_Cmd(TIM1, ENABLE); // 1. 开启定时器计数器开始计数

    /* Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE); // 2. 主输出使能 MOE=1


    // 开启TIM1更新中断标志
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

    while (1)
    {
        // TIM_SetCompare1(TIM1, 888);
    }
}

/**
 * @brief  Configures the different system clocks.
 * @param  None
 * @retval None
 */
void RCC_Configuration(void)
{
    /* TIM1, GPIOA, GPIOB, GPIOE and AFIO clocks enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOE |
                               RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
                           ENABLE);
}

/**
 * @brief  Configure the TIM1 Pins.
 * @param  None
 * @retval None
 */
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

#ifdef STM32F10X_CL
    /* GPIOE Configuration: Channel 1/1N, 2/2N, 3/3N as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13 |
                                  GPIO_Pin_8 | GPIO_Pin_10 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* GPIOE Configuration: BKIN pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* TIM1 Full remapping pins */
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);

#else
    /* GPIOA Configuration: Channel 1, 2 and 3 as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* GPIOB Configuration: Channel 1N, 2N and 3N as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* GPIOB Configuration: BKIN pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    while (1)
    {
    }
}

#endif

/**
 * @}
 */

/**
 * @}
 */
