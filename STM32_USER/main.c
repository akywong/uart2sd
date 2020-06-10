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

u8  Uart2_RxBuf[1024]={0};//���ڽ��ջ���
u32 Uart2_RxCnt=0;//���ռ���
u8  Uart2_RxOK=0;//���յ�һ֡����
//
int main(void)
{
	u8 t=0,r=0;
	u32 count=0;
	delay_init();	    //��ʱ������ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	LED_Init();
	Beep_Init();
	USART1_Init(115200);//����1��ʼ��
	USART2_Init(1200);
	USART_DMA_Rx_Init(Uart2_RxBuf,1024);
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);// �򿪴���2�����ж�
	delay_ms(1000);
	
	//printf("\r\n\r\n̫��M3 STM32�����崮�ڿ���LED�����������Գ���\r\n");
	//USART_SendString(USART1,"\r\nͨ�����ڷ���ָ��ɿ���LED�ͷ�����!���ɳ�����ָ��:\r\n\r\n");
	//USART_SendString(USART1,"\"����\"   \"�ص�\"  \"�򿪷�����\"   \"�رշ�����\"\r\n");
	
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
			printf("SD����ʼ��ʧ��!\r\n");
		}
	}
	//BEEP_OFF();
	printf("\r\nSD����ʼ���ɹ�!\r\n");
	printf("SD������: %dM\r\n",(u32)SDCardInfo.CardCapacity/1024/1024);
	
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
			printf("FatFs ����ʧ�� !\r\n");
		}
	}
	//BEEP_OFF();
	printf("\r\nFatFs ���سɹ� !\r\n");
	
	delay_ms(1000);
	
	//
	r=f_open(&fsrc,f_path,FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
	if(FR_OK != r){
		printf("\r\n�����ļ�ʧ�� !\r\n");
		goto Start;
	}
	printf("\r\n �����ļ��ɹ� !\r\n");
	
	delay_ms(1000);
	
	//
	//r=f_write(&fsrc,test_txt,strlen((const char*)test_txt),&count);
	//
	f_close(&fsrc);
	if(FR_OK != r){
		printf("�ر��ļ�ʧ�� !\r\n");
		goto Start;
	}
	
	delay_ms(1000);
	
	while(1)
	{
		
		if(Uart2_RxOK!=0){
			__disable_irq();
			USART_SendString(USART1,"\r\n����1���յ�1֡����: ");
			USART_SendBuf(USART1,Uart2_RxBuf,Uart2_RxCnt);
			USART_SendString(USART1,"\r\n");
			
			r=f_write(&fsrc,Uart2_RxBuf,Uart2_RxCnt,&count);
			if(FR_OK != r) {
				USART_SendString(USART1,"\r\nд������ʧ�ܣ�\r\n ");
			} else {
				frame++;
				f_sync(&fsrc);
			}
			memset(Uart2_RxBuf,0,Uart2_RxCnt);
			
			
			USART_DMA_Rx_Init(Uart2_RxBuf,1024);//ע��,������һ�����ݽ��ղ��Ҵ���������,���¿�ʼ��һ�ν���
			
			
			Uart2_RxOK=0;
			Uart2_RxCnt=0;
			count=0;
			__enable_irq();
		}
		
		delay_ms(1);
	}
}
//����1�жϺ���
void USART2_IRQHandler(void)
{
	uint8_t Clear=Clear;//���ֶ��巽��������������������"û���õ�"����
	
	
	//DMA���Զ������ݷ��뻺��,������ﲻ��ҪCPU����
////	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){ //������յ�1���ֽ�
////		if(Uart1_RxCnt<1024){
////			Uart1_RxBuf[Uart1_RxCnt++] = USART1->DR;// �ѽ��յ����ֽڱ��棬�����ַ��1
////		}
////	}
	
	
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET){// ������յ�1֡����
		Clear=USART2->SR;// ��SR�Ĵ���
		Clear=USART2->DR;// ��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Uart2_RxCnt=DMA_GetCurrDataCounter(DMA1_Channel5);//��ȡ���յ��ĳ���
		Uart2_RxOK=1;// ��ǽ��յ���1֡����
	}
}























