/* ========================================
 *
 * Copyright CQUPT, 2015
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 *    节点程序
 * 接收指令： soe sce shxxxe slxxxe  spxxxe
 *   
 *
 * ========================================
*/
#include <project.h>

#define BUFFER_SIZE  20 

char RxBuffer[BUFFER_SIZE]; //Rx指令数组
uint8 RxSize = 0;           //Rx指令数组大小
uint16 tempture = 0;    //温度值
uint16 period = 10;  //周期
uint16 timerCount=0; //定时器溢出次数
uint16 highTempture = 500;
uint16 lowTempture = 100;

/* 函数声明 */
void getTempture();
void init();
void updateDisplay (uint16 tempture);
void setGate(uint16 *temp);


/*******************************************************************************
* Timer 中断
********************************************************************************/
CY_ISR(TimerInterruptHandler)
{
    Timer_STATUS;
    ++timerCount;
    if(timerCount >= period/5)
    {
        getTempture();
        timerCount = 0;
        //LCD_Char_1_PrintString("timer");
    }
}

/*******************************************************************************
* Uart Rx 中断  接收到1字节产生的中断
********************************************************************************/
CY_ISR(RxInterruptHandler)
{
    char8 ch; 
    ch = UART_GetChar();
    if(ch > 0u)
    {
         RxBuffer[RxSize++] = ch;
    }
}

/*******************************************************************************
* 发送队列为空时触发中断
********************************************************************************/
CY_ISR(TxInterruptHandler)
{
    //UART_1_PutArray("send over",9);  
    //LCD_Char_1_PrintString("send end ");
}

int main()
{
    init();
    while(1)
    {
        //LCD_Char_1_PrintNumber(RxSize);
        if(RxSize > 2)//来自中心节点的数据，应该进行解析
        {
            int i = 0;           
            if(RxBuffer[RxSize-1] == 'e' && RxBuffer[0] == 's')
            {
                switch(RxBuffer[1])
                {                    
                    case 'o' : //设置节点
                    {  
                        Timer_Start();
                        break;
                    }
                    case 'c' :
                    {
                        Timer_Stop();
                        break;
                    }
                    case 'h' :
                    {
                        setGate(&highTempture);
                        //LCD_Char_1_PrintString(RxBuffer);
                        break;
                    }
                    case 'l' :
                    {
                        setGate(&lowTempture);
                        //LCD_Char_1_PrintString(RxBuffer);
                        break;
                    }
                    case 'p' :
                    {
                        setGate(&period);
                        break;
                    }
                    default :
                    {
                        break;
                    }
                }
                /*for(i = 0; i < RxSize; ++i)
                {
                    LCD_Char_1_PutChar(RxBuffer[i]);
                }*/
                
                for(i = 0; i < RxSize; ++i)//处理完 清指令队列
                {
                    RxBuffer[RxSize] = '\0';
                }
                RxSize = 0;
            }
            else if(RxBuffer[RxSize-1] == 'e' && RxBuffer[0] != 's')//指令出错
            {                
                LCD_Char_1_PrintString("e");
                for(i = 0; i < RxSize; ++i)//错误指令 清指令队列
                {
                    RxBuffer[i] = '\0';
                }
                RxSize = 0;
            }
        }
        
    }
}


/*******************************************************************************
* 设置温度上下限
********************************************************************************/
void setGate(uint16 *temp)
{
    if(RxSize == 6)
    {
        *temp = ( (RxBuffer[2]-48)*1000+(RxBuffer[3]-48)*100+(RxBuffer[4]-48)*10 )/10;
    }
    else if(RxSize == 5)
    {
        *temp = (RxBuffer[2]-48)*10+(RxBuffer[3]-48);
    }
    else if(RxSize == 4)
    {
        *temp = (RxBuffer[2]-48);
    }
}

/*******************************************************************************
* 初始化函数
********************************************************************************/
void init()
{
    CyGlobalIntEnable; //全局中断开启
    
    ADC_DelSig_1_Start();  /* 配置并开启ADC */
    ADC_DelSig_1_StartConvert(); /* 开始进行转换 */
    
    Uart_Rx_ISR_StartEx(RxInterruptHandler);      /* 开启 Uart Rx 中断 并连接到 RxInterruptHandler */
    Uart_Tx_ISR_StartEx(TxInterruptHandler);      /* 开启 Uart Tx 并连接到 TxInterruptHandler */
    UART_Start();     /* 开启 UART */   
    
    Timer_ISR_StartEx(TimerInterruptHandler); /* 开启 Timer 中断并连接到 TimerInterruptHandler */
    Timer_Start();          /* 开启定时器 */
    
    LCD_Char_1_Start(); /* 初始化并清除LCD */
    //LCD_Char_1_PrintString("init");
}

/*******************************************************************************
* 获取温度
********************************************************************************/
void getTempture()
{
    uint16 voltageRawCount;//电平值得统计
    ADC_DelSig_1_IsEndConversion(ADC_DelSig_1_WAIT_FOR_RESULT); /* 等待温度转换完成 */        
    voltageRawCount = ADC_DelSig_1_GetResult32(); /* 得到转换后的值 GetResult16 最大值是32767  */
    tempture = 1.024*voltageRawCount/65536*1000; /* 根据公式进行转换 */ 
	if (voltageRawCount <= 0)
	{
		//errorDisplay();/* 处理错误数据或是零下的温度 */
        UART_PutString("sle");
	}
    else if(tempture > highTempture )
    {
        char tem[5]; 
        tem[0] = 's';
        tem[1] = 'h';
        tem[2] = 'e';
        tem[3] = '\0';
        UART_PutString(tem);
        //LCD_Char_1_PrintNumber(highTempture);
    }
    else if(tempture < lowTempture)
    {
        char tem[5]; 
        tem[0] = 's';
        tem[1] = 'l';
        tem[2] = 'e';
        tem[3] = '\0';
        UART_PutString(tem);
        //LCD_Char_1_PrintNumber(lowTempture);
    }
	else 
	{	
        char tem[8]; 
        tem[0] = 's';
        tem[1] = 't';
        tem[2] = tempture/100 + 48;
        tem[3] = (tempture%100)/10 + 48;
        tem[4] = tempture%10 + 48;
        tem[5] = 'e';
        tem[6] = '\0';
        UART_PutString(tem);
    }
}


/* [] END OF FILE */
