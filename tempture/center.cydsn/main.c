/* ========================================
 *
 * Copyright CQUPT, 2015
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 *    汇集点程序
 *    向节点发送指令soe sce shxxxe slxxxe  spxxxe
 *    向PC发送数据 三个节点的温度以及各个节点的错误信息
 *    接收来自PC的指令 scoae sccae ssha200e ssla200e sppa10e
 *  
 * ========================================
 */

#include <project.h>

#define BUFFER_SIZE  20 

char RxBuffer[BUFFER_SIZE]; //Rx指令数组
uint8 RxSize = 0;           //Rx指令数组大小
char Rx_1_Buffer[BUFFER_SIZE]; //Rx1指令数组
uint8 Rx_1_Size = 0;           //Rx1指令数组大小
char Rx_2_Buffer[BUFFER_SIZE]; //Rx2指令数组
uint8 Rx_2_Size = 0;           //Rx2指令数组大小
uint16 tempture = 0; //温度值
uint16 period = 10; //周期
uint16 timerCount=0; //定时器溢出次数
uint16 highTempture = 500;
uint16 lowTempture = 100;

/* 函数声明 */
void updateDisplay(uint16 tempture);
void errorDisplay();
void getTempture();
void init();
void enQueue(char ch, uint8 node);
void deQueue(char *arr, uint8 *size, uint8 num);
void configNode(uint8 config);
void setNode(uint8 config);
void setGate(uint16 *temp);
void setUartSend(uint8 NodeID);
void setPeriod(uint16 periodSet);


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
        //UART_1_PutStringConst("soett");
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
        enQueue(ch,0);
    }
}

/*******************************************************************************
* Uart Tx 中断
********************************************************************************/
CY_ISR(TxInterruptHandler)
{
    
}


/*******************************************************************************
* Uart1 Rx 中断  接收到1字节产生的中断
********************************************************************************/
CY_ISR(Rx_1_InterruptHandler)
{
    char8 ch; 
    ch = UART_1_GetChar();
    if(ch > 0u)
    {
        enQueue(ch,1);
    }

}

/*******************************************************************************
* Uart1 Tx 中断
********************************************************************************/
CY_ISR(Tx_1_InterruptHandler)
{

}


/*******************************************************************************
* Uart2 Rx 中断  接收到1字节产生的中断
********************************************************************************/
CY_ISR(Rx_2_InterruptHandler)
{
    char8 ch; 
    ch = UART_2_GetChar();
    if(ch > 0u)
    {
        enQueue(ch,2);
    }
}

/*******************************************************************************
* Uart2 Tx 中断
********************************************************************************/
CY_ISR(Tx_2_InterruptHandler)
{

}

int main()
{
    init();
    while(1)
    {   
        //LCD_Char_1_PrintNumber(RxSize);
        if(RxSize>4)//来自PC的数据，应该进行解析
        {       
            int i = 0;
            //LCD_Char_1_PrintNumber(RxSize);
            if(RxBuffer[RxSize-1] == 'e' && RxBuffer[0] == 's')
            {
                uint8 config = 0;
                switch(RxBuffer[3])//获取针对节点的信息
                {
                    case 'a' :
                    {
                        config += 3;
                        break;
                    }
                    case '0' :
                    {
                        config += 0;
                        break;
                    }
                    case '1' :
                    {
                        config += 1;
                        break;
                    }
                    case '2' :
                    {
                        config += 2;
                        break;
                    }
                    default :
                    {
                        config = 20;
                        break;
                    }
                }
                switch(RxBuffer[1])
                {                    
                    case 'c' : //设置节点
                    {  
                        if(RxBuffer[2] == 'o')//打开
                        {
                            ;                           
                        }
                        else if(RxBuffer[2] == 'c')//关闭
                        {
                            config += 10; 
                        }
                        
                        configNode(config);
                        break;
                    }
                    case 's' :
                    {
                        if(RxBuffer[2] == 'h')//打开
                        {
                            ;                           
                        }
                        else if(RxBuffer[2] == 'l')//关闭
                        {
                            config += 10; 
                        }                        
                        setNode(config);
                        break;
                    }
                    case 'p' :
                    {
                        setPeriod(config);
                        break;
                    }
                    default :
                    {
                        break;
                    }
                }
                for(i = 0; i < RxSize; ++i)//处理完 清指令队列
                {
                    RxBuffer[RxSize] = '\0';
                }
                RxSize = 0;
                
            }
            else if(RxBuffer[RxSize-1] == 'e' && RxBuffer[0] != 's')//指令出错
            {                
                LCD_Char_1_PrintString("e0");
                for(i = 0; i < RxSize; ++i)//错误指令 清指令队列
                {
                    RxBuffer[RxSize] = '\0';
                }
                RxSize = 0;
            }
            else if(RxSize > 10)
            {
                LCD_Char_1_PrintString("c0");
                for(i = 0; i < RxSize; ++i)//错误指令 清指令队列
                {
                    RxBuffer[i] = '\0';
                }
                RxSize = 0;
            }
            
        }
        
        if(Rx_1_Size > 2)//来自结点1的数据，应该转发给电脑
        {
            uint8 i = 0;                 
            if(Rx_1_Buffer[Rx_1_Size-1] == 'e' && Rx_1_Buffer[0] == 's')
            {
                LCD_Char_1_PrintString(Rx_1_Buffer);
                if(Rx_1_Buffer[1] == 't') //转发温度，加上节点号
                {
                    char tem[9]; 
                    tem[0] = 's';
                    tem[1] = 't';
                    tem[2] = '1';//节点号
                    tem[3] = Rx_1_Buffer[2];
                    tem[4] = Rx_1_Buffer[3];
                    tem[5] = Rx_1_Buffer[4];
                    tem[6] = 'e';
                    tem[7] = '\0';
                    UART_PutString(tem);
                }
                else if(Rx_1_Buffer[1] == 'h') //转发温度过高
                {
                    LCD_Char_1_PutChar('l');
                    UART_PutString("sh1he");
                }
                else if(Rx_1_Buffer[1] == 'l') //转发温度过低
                {
                    UART_PutString("sh1le");
                }
                for(i = 0; i < Rx_1_Size; ++i)//处理完 清指令队列
                {
                    Rx_1_Buffer[i] = '\0';
                }
                Rx_1_Size = 0;
            }
            
            else if(Rx_1_Buffer[Rx_1_Size-1] == 'e' && Rx_1_Buffer[0] != 's')//指令出错
            {                
                LCD_Char_1_PrintString("e");
                for(i = 0; i < Rx_1_Size; ++i)//错误指令 清指令队列
                {
                    Rx_1_Buffer[i] = '\0';
                }
                Rx_1_Size = 0;
            }
            else if(Rx_1_Size > 10)
            {
                LCD_Char_1_PrintString("c");
                for(i = 0; i < Rx_1_Size; ++i)//错误指令 清指令队列
                {
                    Rx_1_Buffer[i] = '\0';
                }
                Rx_1_Size = 0;
            }
        }
        
        if(Rx_2_Size > 4)//来自结点2的数据，应该转发给电脑
        {
            uint8 i = 0;                 
            if(Rx_2_Buffer[Rx_2_Size-1] == 'e' && Rx_2_Buffer[0] == 's')
            {
                if(Rx_2_Buffer[1] == 't') //转发温度，加上节点号
                {
                    char tem[9]; 
                    tem[0] = 's';
                    tem[1] = 't';
                    tem[2] = '2';//节点号
                    tem[3] = Rx_2_Buffer[2];
                    tem[4] = Rx_2_Buffer[3];
                    tem[5] = Rx_2_Buffer[4];
                    tem[6] = 'e';
                    tem[7] = '\0';
                    UART_PutString(tem);
                }
                else if(Rx_2_Buffer[1] == 'h') //转发温度过高
                {
                    UART_PutString("sh2he");
                }
                else if(Rx_2_Buffer[1] == 'l') //转发温度过低
                {
                    UART_PutString("sh2le");
                }
                for(i = 0; i < Rx_2_Size; ++i)//处理完 清指令队列
                {
                    Rx_2_Buffer[i] = '\0';
                }
                Rx_2_Size = 0;
            }
            
            else if(Rx_2_Buffer[Rx_2_Size-1] == 'e' && Rx_2_Buffer[0] != 's')//指令出错
            {                
                LCD_Char_1_PrintString("e");
                for(i = 0; i < Rx_2_Size; ++i)//错误指令 清指令队列
                {
                    Rx_2_Buffer[i] = '\0';
                }
                Rx_2_Size = 0;
            }
            else if(Rx_2_Size > 10)
            {
                LCD_Char_1_PrintString("c");
                for(i = 0; i < Rx_2_Size; ++i)//错误指令 清指令队列
                {
                    Rx_2_Buffer[i] = '\0';
                }
                Rx_2_Size = 0;
            }
        }
        
    }
}

/*******************************************************************************
* 设置节点的上报数据的周期
********************************************************************************/
void setPeriod(uint16 periodSet)
{
    LCD_Char_1_PrintNumber(periodSet);
    if(periodSet < 5)
    switch(periodSet)
    {
        case 0 ://设置0节点上报数据周期
        {
            setGate(&period);
            LCD_Char_1_PrintNumber(period);
            break;
        }
        case 1 :
        {
            setUartSend(11);
            break;
        }
        case 2 :
        {
            setUartSend(12);
            break;
        }
        case 3 ://设置所有节点上报数据周期
        {
            setGate(&period);
            setUartSend(11);
            setUartSend(12);
            break;
        }
        default :
        {
            break;
        }
    }
}

/*******************************************************************************
* 串口发送温度上下限
********************************************************************************/
void setUartSend(uint8 NodeID)
{
    int i;
    switch(NodeID)
    {
        case 1 :
        {
            for(i = 1;i < RxSize ; ++i)
            {
                if(i != 3)
                {
                    UART_1_WriteTxData(RxBuffer[i]);
                    //LCD_Char_1_PutChar(RxBuffer[i]);
                }                
            }
            break;
        }
        case 2 :
        {
            for(i = 1;i < RxSize ; ++i)
            {
                if(i != 3)
                {
                    UART_2_WriteTxData(RxBuffer[i]);
                    //LCD_Char_1_PutChar(RxBuffer[i]);
                }                
            }
            break;
        }
        case 11 :
        {
            for(i = 0;i < RxSize ; ++i)
            {
                if(i != 3 && i!= 2)
                {
                    UART_1_WriteTxData(RxBuffer[i]);
                    //LCD_Char_1_PutChar(RxBuffer[i]);
                }                
            }
            break;
        }
        case 12 :
        {
            for(i = 0;i < RxSize ; ++i)
            {
                if(i != 3 && i!= 2)
                {
                    UART_2_WriteTxData(RxBuffer[i]);
                    //LCD_Char_1_PutChar(RxBuffer[i]);
                }                
            }
            break;
        }
        default :
        {
            break;
        }
    }
}

/*******************************************************************************
* 设置温度上下限
********************************************************************************/
void setGate(uint16 *temp)
{
    if(RxSize == 8)
    {
        *temp = ( (RxBuffer[4]-48)*1000+(RxBuffer[5]-48)*100+(RxBuffer[6]-48)*10 )/10;
    }
    else if(RxSize == 7)
    {
        *temp = (RxBuffer[4]-48)*10+(RxBuffer[5]-48);
    }
    else 
    {
        *temp = (RxBuffer[4]-48);
    }
}

/*******************************************************************************
* 设置节点温度上下限
********************************************************************************/
void setNode(uint8 config)
{
    //LCD_Char_1_PrintNumber(config);
    if(config < 19)
    switch(config)
    {
        case 0 ://设置0节点上限
        {
            setGate(&highTempture);
            LCD_Char_1_PrintNumber(highTempture);
            break;
        }
        case 1 :
        {
            setUartSend(1);
            break;
        }
        case 2 :
        {
            setUartSend(2);
            break;
        }
        case 3 ://设置所有节点上限
        {
            setGate(&highTempture);
            setUartSend(1);
            setUartSend(2);
            break;
        }
        case 10 ://设置0节点下限
        {
            setGate(&lowTempture);
            //LCD_Char_1_PrintNumber(lowTempture);
            break;
        }
        case 11 :
        {
            setUartSend(1);
            break;
        }
        case 12 :
        {
            setUartSend(2);
            break;
        }
        case 13 ://设置所有节点下限
        {
            setGate(&lowTempture);
            setUartSend(1);
            setUartSend(2);
            break;
        }
        default :
        {
            break;
        }
    }
}

/*******************************************************************************
* 打开关闭结点
********************************************************************************/
void configNode(uint8 config)
{
    //LCD_Char_1_PrintNumber(config);
    if(config < 19)
    switch(config)
    {
        case 0 :
        {
            Timer_Start();          /* 开启定时器 */
            break;
        }
        case 1 :
        {
            UART_1_PutStringConst("soe");
            break;
        }
        case 2 :
        {
            UART_2_PutStringConst("soe");
            break;
        }
        case 3 :
        {
            Timer_Start();
            UART_1_PutStringConst("soe");
            UART_2_PutStringConst("soe");
            break;
        }
        case 10 :
        {
            Timer_Stop();          /* 关闭定时器 */
            break;
        }
        case 11 :
        {
            UART_1_PutStringConst("sce");
            break;
        }
        case 12 :
        {
            UART_2_PutStringConst("sce");
            break;
        }
        case 13 :
        {
            Timer_Stop();
            UART_1_PutStringConst("sce");
            UART_2_PutStringConst("sce");
            break;
        }
        default :
        {
            break;
        }
    }
}


/*******************************************************************************
* 将指定个数的数据移出队列
********************************************************************************/
void deQueue(char *arr, uint8 *size, uint8 num)
{
    int i;
    for(i = 0; i <= num; ++i)
    {
        arr[i] = arr[i+num];
    }
    *size = *size - num;
}

/*******************************************************************************
* 将接受到的数据压入指令队列
********************************************************************************/
void enQueue(char ch, uint8 node)
{
    //LCD_Char_1_PrintNumber(node);
    switch(node)
    {
        
        case 0 :
        { 
            RxBuffer[RxSize++] = ch;
            break;
        }
        case 1 :
        { 
            Rx_1_Buffer[Rx_1_Size++] = ch;
            break;
        }
        case 2 :
        { 
            Rx_2_Buffer[Rx_2_Size++] = ch;
            break;
        }
        default :
        { 
            break;
        }
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
    
    Uart_Rx_ISR_1_StartEx(Rx_1_InterruptHandler);      /* 开启 Uart Rx 中断 并连接到 RxInterruptHandler */
    Uart_Tx_ISR_1_StartEx(Tx_1_InterruptHandler);      /* 开启 Uart Tx 并连接到 TxInterruptHandler */
    UART_1_Start();     /* 开启 UART1 */
    
    Uart_Rx_ISR_2_StartEx(Rx_1_InterruptHandler);      /* 开启 Uart Rx 中断 并连接到 RxInterruptHandler */
    Uart_Tx_ISR_2_StartEx(Tx_1_InterruptHandler);      /* 开启 Uart Tx 并连接到 TxInterruptHandler */
    UART_2_Start();     /* 开启 UART2 */
    
    
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
		errorDisplay();/* 处理错误数据或是零下的温度 */
        UART_PutString("sl0e");
	}
    else if(tempture > highTempture )
    {
        char tem[6]; 
        tem[0] = 's';
        tem[1] = 'h';
        tem[2] = '0';
        tem[3] = 'e';
        tem[4] = '\0';
        UART_PutString(tem);
        LCD_Char_1_PrintNumber(highTempture);
    }
    else if(tempture < lowTempture)//防止温度值溢出
    {
        char tem[6]; 
        tem[0] = 's';
        tem[1] = 'l';
        tem[2] = '0';
        tem[3] = 'e';
        tem[4] = '\0';
        UART_PutString(tem);
        LCD_Char_1_PrintNumber(lowTempture);
    }
	else 
	{	
        char tem[9]; 
        tem[0] = 's';
        tem[1] = 't';
        tem[2] = '0';
        tem[3] = tempture/100 + 48;
        tem[4] = (tempture%100)/10 + 48;
        tem[5] = tempture%10 + 48;
        tem[6] = 'e';
        tem[7] = '\0';
        UART_PutString(tem);
    }
        
        
        //LCD_Char_1_PrintString(" tempture ");
        //LCD_Char_1_PrintNumber(tempture/10); /* 打印小数点前面的数据 */
        //LCD_Char_1_PrintNumber(tempture%10); /* 打印小数点后面的数据 */
        
        //LCD_Char_1_PrintNumber(tempture);
        //updateDisplay(tempture); /* 打印数据到LCD */
}

/*******************************************************************************
* 错误显示
********************************************************************************/

void errorDisplay()
{
    LCD_Char_1_ClearDisplay();    
    LCD_Char_1_PrintString("DATA ERROR");/* 打印错误提示信息 */
}

/*******************************************************************************
* 显示数据到显示屏
********************************************************************************/
void updateDisplay (uint16 tempture)
{
    //LCD_Char_1_ClearDisplay();//清除显示    
    LCD_Char_1_PrintString("Tempture:");/* 打印提示信息 */
    
	LCD_Char_1_Position(0,9); /* 把光标移动到1行，10列 */
	LCD_Char_1_PrintNumber(tempture/10); /* 打印小数点前面的数据 */
    LCD_Char_1_PutChar('.');
    LCD_Char_1_PrintNumber(tempture%10); /* 打印小数点后面的数据 */
    
    LCD_Char_1_PrintString(" ");
    //LCD_Char_1_PutChar(LCD_Char_1_CUSTOM_7);
    LCD_Char_1_PrintString("C");
    
}



/* [] END OF FILE */
