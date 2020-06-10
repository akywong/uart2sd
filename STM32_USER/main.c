#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "string.h"
#include "beep.h"
#include "ff.h"
#include "sdio_sdcard.h"
#include "UartDMA.h"

FATFS fs; 
FIL fsrc;
int frame=0;

const char f_path[]={"test.txt"};

u8  Uart2_RxBuf[1024]={0};//串口接收缓存
u32 Uart2_RxCnt=0;//接收计数
u8  Uart2_RxOK=0;//接收到一帧数据
//
int main(void)
{
	u8 t=0,r=0;
	u32 count=0;
	delay_init();	    //延时函数初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	LED_Init();
	Beep_Init();
	USART1_Init(115200);//串口1初始化
	USART2_Init(1200);
	USART_DMA_Rx_Init(Uart2_RxBuf,1024);
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);// 打开串口2空闲中断
	delay_ms(1000);
	
	//printf("\r\n\r\n太极M3 STM32开发板串口控制LED及蜂鸣器测试程序。\r\n");
	//USART_SendString(USART1,"\r\n通过串口发送指令可控制LED和蜂鸣器!您可尝试下指令:\r\n\r\n");
	//USART_SendString(USART1,"\"开灯\"   \"关灯\"  \"打开蜂鸣器\"   \"关闭蜂鸣器\"\r\n");
	
	Start:
	while( SD_OK != SD_Init() ){
		delay_ms(30);
		t++;
		if(t%10==0){
			//BEEP_TOGGLE();
			LED_TOGGLE(LED1);
		}
		if(t>30){
			t=0;
			printf("SD卡初始化失败!\r\n");
		}
	}
	//BEEP_OFF();
	printf("\r\nSD卡初始化成功!\r\n");
	printf("SD卡容量: %dM\r\n",(u32)SDCardInfo.CardCapacity/1024/1024);
	
	delay_ms(1000);
	
	//
	while(FR_OK != f_mount(&fs,"0:",1)){
		delay_ms(30);
		t++;
		if(t%10==0){
			//BEEP_TOGGLE();
			LED_TOGGLE(LED1);
		}
		if(t>30){
			t=0;
			printf("FatFs 挂载失败 !\r\n");
		}
	}
	//BEEP_OFF();
	printf("\r\nFatFs 挂载成功 !\r\n");
	
	delay_ms(1000);
	
	//
	r=f_open(&fsrc,f_path,FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
	if(FR_OK != r){
		printf("\r\n创建文件失败 !\r\n");
		goto Start;
	}
	printf("\r\n 创建文件成功 !\r\n");
	
	delay_ms(1000);
	
	//
	//r=f_write(&fsrc,test_txt,strlen((const char*)test_txt),&count);
	//
	f_close(&fsrc);
	if(FR_OK != r){
		printf("关闭文件失败 !\r\n");
		goto Start;
	}
	
	delay_ms(1000);
	
	while(1)
	{
		
		if(Uart2_RxOK!=0){
			__disable_irq();
			USART_SendString(USART1,"\r\n串口1接收到1帧数据: ");
			USART_SendBuf(USART1,Uart2_RxBuf,Uart2_RxCnt);
			USART_SendString(USART1,"\r\n");
			
			r=f_write(&fsrc,Uart2_RxBuf,Uart2_RxCnt,&count);
			if(FR_OK != r) {
				USART_SendString(USART1,"\r\n写入数据失败！\r\n ");
			} else {
				frame++;
				f_sync(&fsrc);
			}
			memset(Uart2_RxBuf,0,Uart2_RxCnt);
			
			
			USART_DMA_Rx_Init(Uart2_RxBuf,1024);//注意,到这里一次数据接收并且处理就完成了,重新开始下一次接收
			
			
			Uart2_RxOK=0;
			Uart2_RxCnt=0;
			count=0;
			__enable_irq();
		}
		
		delay_ms(1);
	}
}
//串口1中断函数
void USART2_IRQHandler(void)
{
	uint8_t Clear=Clear;//这种定义方法，用来消除编译器的"没有用到"提醒
	
	
	//DMA会自动将数据放入缓存,因此这里不需要CPU处理
////	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){ //如果接收到1个字节
////		if(Uart1_RxCnt<1024){
////			Uart1_RxBuf[Uart1_RxCnt++] = USART1->DR;// 把接收到的字节保存，数组地址加1
////		}
////	}
	
	
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET){// 如果接收到1帧数据
		Clear=USART2->SR;// 读SR寄存器
		Clear=USART2->DR;// 读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Uart2_RxCnt=DMA_GetCurrDataCounter(DMA1_Channel5);//获取接收到的长度
		Uart2_RxOK=1;// 标记接收到了1帧数据
	}
}























