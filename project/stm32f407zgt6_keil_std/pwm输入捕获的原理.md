好，我们先不管时钟、不管频率错误，**先把这个官方例程的核心机制搞懂**。

你现在看的这个：

```c
IC2Value = TIM_GetCapture2(TIM4);
```

和：

```c
TIM_GetCapture1(TIM4)
```

其实就是读取：

* **TIM4->CCR2寄存器**
* **TIM4->CCR1寄存器**

也就是两个输入捕获通道记录的时间值。

---

# 1. 什么叫 Input Capture（输入捕获）？

简单理解：

STM32内部有一个计数器：

```
          TIM4
          
     时钟84MHz
          |
          ↓
        CNT计数器

0 1 2 3 4 5 6 7 8 .....
```

这个CNT一直增加。

比如：

```
CNT:
0
1
2
3
4
5
...
```

当外部PWM边沿来了：

例如：

```
PWM:

____|‾‾‾‾‾|____|‾‾‾‾‾|____

    ↑     ↑
    |     |
   上升  下降
```

Timer硬件会自动：

```
当前CNT值
     |
     ↓
复制到CCR寄存器
```

这个动作叫：

> 输入捕获(Input Capture)

---

# 2. PWM Input模式干了什么？

PWM Input模式本质：

**用两个捕获通道，同时测周期和高电平时间。**

STM32官方例程：

使用：

```
TIM4_CH2 输入
PB7
```

但是内部同时使用：

```
CH1
CH2
```

两个捕获。

---

## CH2干什么？

你的配置：

```c
TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;

TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;

TIM_PWMIConfig(TIM4, &TIM_ICInitStructure);
```

表示：

CH2捕获：

### 上升沿

例如：

```
PWM:

      周期T

      ↓
______|‾‾‾‾‾‾|________|‾‾‾‾‾
      ↑
      |
    Rising
```

每次上升沿：

Timer做：

```
CCR2 = 当前CNT
```

但是注意：

后面还有：

```c
TIM_SelectSlaveMode(TIM4, TIM_SlaveMode_Reset);
```

这个非常关键。

---

# 3. Reset Mode是什么？

这里：

```c
TIM_SelectInputTrigger(TIM4, TIM_TS_TI2FP2);

TIM_SelectSlaveMode(TIM4, TIM_SlaveMode_Reset);
```

意思：

用CH2的上升沿作为复位触发。

也就是：

每次PWM上升沿：

执行：

```
1. 保存CCR2
2. CNT清零
```

类似：

```
第一次上升沿

CNT=500

保存:
CCR2=500

然后:

CNT=0


下一周期:

CNT重新计数
```

所以：

## CCR2就是：

> 两个PWM上升沿之间，Timer计数了多少次

也就是：

# PWM周期计数值

---

所以：

```c
IC2Value = TIM_GetCapture2(TIM4);
```

得到：

```
PWM周期
```

单位：

Timer tick

例如：

假设：

TIM时钟：

84MHz

10kHz PWM:

周期:

100us

那么：

```
CCR2=84MHz×100us

=8400
```

---

# 4. 那CCR1是什么？

看：

```c
TIM_GetCapture1(TIM4)
```

为什么有CH1？

PWM Input模式内部自动配置：

CH1:

* 同一个输入
* 但是下降沿捕获

也就是：

```
PWM:

       高电平
     <------>

_____|‾‾‾‾‾‾|_____

     ↑      ↑
     |      |
   Rising Falling


CH2:
抓 ↑

CH1:
抓 ↑
```

---

过程：

假设：

PWM:

```
周期100us

高电平40us


       40us
<-------------->

_____|‾‾‾‾‾|________
```

Timer:

上升沿：

```
CNT=0
```

开始计数。

下降沿：

```
CNT=40us对应计数
```

此时：

```
CCR1 = 40us对应tick
```

所以：

## CCR1：

> 高电平持续时间

---

# 5. 所以这两个值关系：

假设：

PWM:

```
周期:

100us


高电平:

30us
```

Timer：

84MHz

那么：

```
CCR2:

100us×84MHz

=8400


CCR1:

30us×84MHz

=2520
```

计算：

```c
DutyCycle =
(CCR1*100)/CCR2
```

得到：

```
2520/8400*100

=30%
```

---

# 6. 回到你的代码

```c
IC2Value = TIM_GetCapture2(TIM4);
```

实际：

读取：

```
TIM4->CCR2
```

表示：

```
PWM周期tick数
```

---

```c
TIM_GetCapture1(TIM4)
```

实际：

读取：

```
TIM4->CCR1
```

表示：

```
PWM高电平tick数
```

---

所以官方代码：

```c
DutyCycle =
(TIM_GetCapture1(TIM4)*100)
/IC2Value;
```

等价：

```
占空比 =
高电平时间 / 周期时间
```

---

# 7. 你之前10kHz测试：

你的：

```
CCR1=0x53D=1341

CCR2=0xA7D=2685
```

所以：

```
Duty=1341/2685

=49.94%
```

这证明：

整个捕获链路是正确的。

---

你后面那个频率不对的问题，和这个机制没关系，属于：

**“CCR2这个周期tick换算成真实时间时，TIM计数频率取错或者Timer时钟问题”**

但是第一步你现在已经搞懂：

```
CCR2 = 周期
CCR1 = 高电平时间
```

这个就是 STM32 PWM Input 模式的核心。你后面做 DShot600 接收，本质也是用这个思想，只不过一般不用中断，而是：

```
TIM Input Capture + DMA
```

连续抓边沿时间。
