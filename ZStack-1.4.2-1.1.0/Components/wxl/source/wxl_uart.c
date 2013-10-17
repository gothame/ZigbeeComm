//cd wxl      ����0������
#include <ioCC2430.h>
#include <wxl_uart.h>
//#include <string.h>

/****************************************************************
*�������� ����ʼ������1										
*��ڲ��� ����												
*�� �� ֵ ����							
*˵    �� ��57600-8-n-1						
****************************************************************/
void initUARTtest(void)
{

    CLKCON &= ~0x40;              //����
    while(!(SLEEP & 0x40));      //�ȴ������ȶ�
    CLKCON &= ~0x47;             //TICHSPD128��Ƶ��CLKSPD����Ƶ
    SLEEP |= 0x04; 		 //�رղ��õ�RC����
    PERCFG = 0x00;				//λ��1 P0��
    P0SEL |= 0x0C;				//P0��������
    P2DIR &= ~0xC0;                             //P0������Ϊ����0
    U0CSR |= 0x80;				//UART��ʽ
    UTX0IF = 0;

}
/****************************************************************
*�������� �����ڷ����ַ�������					
*��ڲ��� : data:����									
*			len :���ݳ���							
*�� �� ֵ ����											
*˵    �� ��				
****************************************************************/
void UartTX_Send_String(char *Data,int len)
{
  int j;
  for(j=0;j<len;j++)
  {
    U0DBUF = *Data++;
    while(UTX0IF == 0);
    UTX0IF = 0;
  }
}
void UartTX_Send_Single(char single_Data)
{
    U0DBUF = single_Data;
    while(UTX0IF == 0);
    UTX0IF = 0;
}
/*******************************************************************************
������
    ���ڽ���һ���ַ�
��������char UartRX_Receive_Char (void)
*******************************************************************************/
char UartRX_Receive_Char (void)
{
   char c;
   unsigned char status;
   status = U0CSR;
   U0CSR |= UART_ENABLE_RECEIVE;
   while (!URX0IF);
   c = U0DBUF;
   URX0IF = 0;
   U0CSR = status;
   return c;
}
/*******************************************************************************
������
    �����ʵ�����
��������void Uart_Baud_rate(int Baud_rate)
*******************************************************************************/
void Uart_Baud_rate(int Baud_rate)
{
  switch (Baud_rate)
  {
    case 24:
      U0GCR |= 6;				
      U0BAUD |= 59;				//����������
    break;
    case 48:
      U0GCR |= 7;				
      U0BAUD |= 59;				//����������
    break;
    case 96:
      U0GCR |= 8;				
      U0BAUD |= 59;				//����������
    break;
    case 144:
      U0GCR |= 8;				
      U0BAUD |= 216;				//����������
    break;
    case 192:
      U0GCR |= 9;				
      U0BAUD |= 59;				//����������
    break;
    case 288:
      U0GCR |= 9;				
      U0BAUD |= 216;				//����������
    break;
    case 384:
      U0GCR |= 10;				
      U0BAUD |= 59;				//����������
    break;
    case 576:
      U0GCR |= 10;				
      U0BAUD |= 216;				//����������
    break;
    case 768:
      U0GCR |= 11;				
      U0BAUD |= 59;				//����������
    break;
    case 1152:
      U0GCR |= 11;				
      U0BAUD |= 216;				//����������
    break;
    case 2304:
      U0GCR |= 12;				
      U0BAUD |= 216;				//����������
    break;
    default:
    break;
  }

}

