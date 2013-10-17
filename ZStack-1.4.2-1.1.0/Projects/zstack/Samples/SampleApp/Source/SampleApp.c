/*********************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2007-05-31 15:56:04 -0700 (Thu, 31 May 2007) $
  Revision:       $Revision: 14490 $

  Description:
				  - Sample Application (no Profile).
				
          This application isn't intended to do anything useful,
          it is intended to be a simple example of an application's
          structure.

          This application sends it's messages either as broadcast or
          broadcast filtered group messages.  The other (more normal)
          message addressing is unicast.  Most of the other
          sample applications are written to support the unicast
          message model.

          Key control:
            SW1:  Sends a flash command to all devices in Group 1.
            SW2:  Adds/Removes (toggles) this device in and out
                  of Group 1.  This will enable and disable the
                  reception of the flash command.

  Notes:

  Copyright (c) 2007 by Texas Instruments, Inc.
  All Rights Reserved.  Permission to use, reproduce, copy, prepare
  derivative works, modify, distribute, perform, display or sell this
  software and/or its documentation for any purpose is prohibited
  without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"
#include "stdio.h"

#include "NLMEDE.h"
#include "string.h"
/* HAL */
#include "lcd128_64.h"
#include "hal_led.h"
#include "hal_key.h"

#include "Menu.h"
#include "hal.h"
#include "wxl_uart.h"

INT8U LanguageSel = 1;
//uint8 16 = 16;   //ע�⻺�����ĳ�����õ���4�ı���
//uint8 29 = 16 + 13;
//01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

//-----------------------------------------------------------------------------------------
//��������
//-----------------------------------------------------------------------------------------
union h{
  uint8 RxBuf[33];                             //�޸Ļ��壡
  struct RFRXBUF
  {
    uint8 HeadCom[3]; //����ͷ
    uint8 Laddr[8];   //�����ַ
    uint8 Saddr[2];   //�����ַ
    uint8 DataBuf[20];  //���ݻ�����       //�޸Ļ��壡   ע��һ��Ҫ�޸�����ط��ý��պͷ��͵Ļ������Щ����
  }RXDATA;
}RfRx;//���߽��ջ�����

union j{
  uint8 TxBuf[33];                             //�޸Ļ��壡
  struct RFTXBUF
  {
    uint8 HeadCom[3]; //����ͷ
    uint8 Laddr[8];
    uint16 Saddr;
    uint8 DataBuf[20];  //���ݻ�����       //�޸Ļ��壡  ע��һ��Ҫ�޸�����ط��ý��պͷ��͵Ļ������Щ����
  }TXDATA;
}RfTx;//���߷��ͻ�����

// ����б��г���Ӧ�ó�������� Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

struct join
{
  uint8 RfdCount;		//RFD������
  uint8 RouterCount;	//·����������
  uint8 RfdAddr[20][10];	//�洢RFD�ڵ��ַ�ã�����λΪ�����ַ����8λΪ�����ַ
  uint8 RouterAddr[20][10];//�洢RFD�ڵ��ַ�ã�����λΪ�����ַ����8λΪ�����ַ
}JoinNode;

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.

endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.

uint8 Menu_Key;
devStates_t SampleApp_NwkState;
uint8 SampleApp_TransID;  // ���Ǹ�Ψһ����ϢID(counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;

aps_Group_t SampleApp_Group;

NLME_LeaveCnf_t* Remove;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;
uint8 state2 = 0;
uint8 arr[20];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime );
uint8 SendData(uint8 *buf, uint16 addr, uint8 Leng);
uint16 get_short_addr(void);

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Ӧ�ó�ʼ������������Ӳ����ʼ����ƽ̨��ʼ�������ĳ�ʼ����
 *
 * @param   task_id - ����ϵͳ���������ID�����ID����������Ϣ������ʱ��
 *
 * @return  none
 */
void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

 #if defined ( SOFT_START )
  // The "Demo" target is setup to have SOFT_START and HOLD_AUTO_START
  // SOFT_START is a compile option that allows the device to start
  //  as a coordinator if one isn't found.
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
	//ѡ��Zigbee�豸Э����
	#ifdef ZG_Coord
		zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
	#endif
	//ѡ��Zigbee�豸Ϊ·����
	#ifdef ZG_Router
		zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
	#endif
	//ѡ��Zigbee�豸Ϊ�ն˽ڵ�
	#ifdef ZG_ENDDEVICE
		zgDeviceLogicalType = ZG_DEVICETYPE_ENDDEVICE;
	#endif
#endif //SOFT_START

#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // ����ͨѶ��Ŀ�ĵ�ַ
  // �㲥���нڵ�
/*  SampleApp_All_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_All_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_All_DstAddr.addr.shortAddr = 0xFFFF;
*/
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

  //����Ϊ�����ķ��ͷ�ʽ
/*  SampleApp_Single_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  SampleApp_Single_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
*/


  // Setup for the flash command's destination address - Group 1
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;

  // ����ն�����
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID );

  // By default, all devices start out in Group 1
  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );

  //��ʼ������
  initUARTtest();
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - ����ϵͳ���������ID.
 * @param   events -   �¼�����
 *
 * @return  none
 */
extern void halWait(BYTE wait);
extern int Send_Flag;
int Send_Data=0;
extern INT16U SrcSaddr;//�������ݵ�ַ
extern int Send_Flag_Consecution;
void Frist_Data(void);
int Pingpong_Data = 0;
int Pingpong_Flag = 0;
int Sensor_Flag = 0;
int count = 0;
extern uint16 ReadBattery(void);
extern uint16 Read_Temp(void);
extern uint8 ReadVoltage0(int portnumber);
uint16 Power_value;
uint16 Temp_value;
uint8 Volt_value;

uint8 rxBuf[16]={0x68,0x00,0x00,0x00,0x00,0x00,0x00,0x68,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x16};//ͬ��ָ��
uint8 outputnum = 0;

unsigned char status;

uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  //uint8 buffer[4];
  //uint8 buf[5];

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // �����¼�
        case KEY_CHANGE:
        //SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
        //MenuMenuDisp(((keyChange_t *)MSGpkt)->keys);
        //���β˵�Ŀ¼
        Menu_all(((keyChange_t *)MSGpkt)->keys);
        break;

        // �յ���Ϣ�¼������յ���Ƶ��Ϣ��������ת��CC2430���������
        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          MSGpkt->hdr.event ^= AF_INCOMING_MSG_CMD;
          break;

        //�豸��������
        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (SampleApp_NwkState == DEV_ZB_COORD)
              || (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            // ��һ����Χ�ڷ���һ����Ϣ
            Frist_Data();
            state2=10;
            for(int i=0;i<10;i++)
            halWait(200);
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
          }
          else
          {
            // �豸����������
          }
          break;

        default:
          break;
      }

      // �ͷŴ洢��
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // ����δ�����¼�
    return (events ^ SYS_EVENT_MSG);
  }


  // ����һ����Ϣ - ����¼�ʱ����һ��ʱ��
  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    #if(defined(ZG_Coord))//��ΪSinkʱ�������Է���ͬ��ָ��
       SendData(rxBuf, 0xFFFF, 16);//�㲥����

       //���Գ��򣺴����Է�����
 /*      outputnum = 0;
       while(outputnum<16)
       {
         //����PC����
         RfRx.RXDATA.DataBuf[outputnum] = UartRX_Receive_Char();
         outputnum = outputnum + 1;
        }
       URX0IF = 0;//��մ��ڽ��ջ���
        //��ARM������Ϣ������PC
        UartTX_Send_String(RfRx.RXDATA.DataBuf,16);
*/
    #endif

    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                       2000);
    // return unprocessed events
    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }

  // ����δ֪�¼�
  return 0;
}

uint16 get_short_addr(void)
{
  uint16 Short_addr;
  Short_addr = RfRx.RXDATA.Saddr[0];
  Short_addr <<= 8;
  Short_addr = RfRx.RXDATA.Saddr[1];
  return Short_addr;
}

void Frist_Data(void)
{

#ifdef ZG_Coord
            HalLedBlink( HAL_LED_4, 2, 50, (1000 / 4) );
            ClearScreen();
            Print(3,0,"Found network OK",1);
            halWait(200);
            RfTx.TXDATA.HeadCom[0] = 'l';
            RfTx.TXDATA.HeadCom[1] = 'o';
            RfTx.TXDATA.HeadCom[2] = 'k';
            SendData(RfTx.TxBuf, Send_Mode_Broadcast, 20);            //�޸Ļ��壡��
#endif
#ifdef ZG_Router //·����
            uint8 *ieeeAddr;
            HalLedBlink( HAL_LED_4, 4, 50, (1000 / 4) );
            ClearScreen();
            Print(3,0,"Join network OK",1);
            Print(0,0,"-----ROUTER-----",1);						
            RfTx.TXDATA.HeadCom[0] = 'J';
            RfTx.TXDATA.HeadCom[1] = 'O';
            RfTx.TXDATA.HeadCom[2] = 'N';
				
            ieeeAddr = NLME_GetExtAddr();
            memcpy(RfTx.TXDATA.Laddr,ieeeAddr,8);
            RfTx.TXDATA.Saddr = NLME_GetShortAddr();
            RfTx.TXDATA.DataBuf[0] = 'R';
            RfTx.TXDATA.DataBuf[1] = 'O';
            RfTx.TXDATA.DataBuf[2] = 'U';
            SendData(RfTx.TxBuf, 0x0000, 33);                         //�޸Ļ��壡��
#endif
#ifdef ZG_ENDDEVICE //�ն��豸
            uint8 *ieeeAddr;
            HalLedBlink( HAL_LED_4, 3, 50, (1000 / 4) );

            //ClearScreen();
            //Print(3,0,"Join network OK",1);
            //Print(0,4,"---ENDDEVICE----",1);
			
            RfTx.TXDATA.HeadCom[0] = 'J';
            RfTx.TXDATA.HeadCom[1] = 'O';
            RfTx.TXDATA.HeadCom[2] = 'N';
						
            ieeeAddr = NLME_GetExtAddr();
            memcpy(RfTx.TXDATA.Laddr,ieeeAddr,8);
            RfTx.TXDATA.Saddr = NLME_GetShortAddr();

            RfTx.TXDATA.DataBuf[0] = 'R';
            RfTx.TXDATA.DataBuf[1] = 'F';
            RfTx.TXDATA.DataBuf[2] = 'D';
            SendData(RfTx.TxBuf, 0x0000, 33);                        //�޸Ļ��壡��
#endif
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   �ֱ��Ͱ�������������
 *
 * @param   shift -  if �� shift/alt��ʾΪtrue
 * @param   keys - λɨ�谴���¼�. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
endPointDesc_t SampleApp_epDesc;
void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
     //MenuMenuDisp(keys);
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   �������ݴ�����.  �����������������������ݣ����ݿ��������������豸.
 *          ���Խ����ڲ�ͬ��ID֮�ϣ���ɲ�ͬ������.
 *
 * @param   none
 *
 * @return  none
 */
extern void ClearScreenLcd(void);
//uint8 23 = 16 + 7;  ��ַλ�����3λ
//uint8 UartStrSub[5];
uint8 UartStr[27];//�޸Ļ��壡��

void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  ///HalLedBlink( HAL_LED_4, 1, 50, (1000 / 4) );
  P1DIR |= 0x02;
  P1_1 = 0;
  halWait(200);
  P1_1 = 1;
#ifdef	ZG_Coord
	uint8 i, j;
        uint8 HaveFlag;
	memcpy(RfRx.RxBuf,pkt->cmd.Data,33);                   //�޸Ļ��壡��
	if((RfRx.RXDATA.HeadCom[0] == 'J') && (RfRx.RXDATA.HeadCom[1] == 'O') && (RfRx.RXDATA.HeadCom[2] == 'N'))//���½ڵ��������
	{
		if((RfRx.RXDATA.DataBuf[0] == 'R') && (RfRx.RXDATA.DataBuf[1] == 'F') && (RfRx.RXDATA.DataBuf[2] == 'D'))//RFD�ڵ�
		{
			for(i=0; i<8; i++)
			{
				JoinNode.RfdAddr[JoinNode.RfdCount][i] = RfRx.RXDATA.Laddr[i];
			}
			for(i=0; i<2; i++)
			{
				JoinNode.RfdAddr[JoinNode.RfdCount][8+i] = RfRx.RXDATA.Saddr[1-i];
			}
				
			for(j=0; j<JoinNode.RfdCount; j++)//�ж������ظ�����Ľڵ�
			{
				HaveFlag = 1;
				for(i=0; i<8; i++)
				{
					if(JoinNode.RfdAddr[JoinNode.RfdCount][i] != JoinNode.RfdAddr[j][i])
					{
						HaveFlag = 0;
						break;//����
					}
				}
				if(HaveFlag == 0)continue;
				JoinNode.RfdCount--;//��
				JoinNode.RfdAddr[j][8] = RfRx.RXDATA.Saddr[1];
				JoinNode.RfdAddr[j][9] = RfRx.RXDATA.Saddr[0];	//�޸����������ַ
				break;
			}
			JoinNode.RfdCount++;
		}
		else if((RfRx.RXDATA.DataBuf[0] == 'R') && (RfRx.RXDATA.DataBuf[1] == 'O') && (RfRx.RXDATA.DataBuf[2] == 'U'))//·�ɽڵ�
		{
			for(i=0; i<8; i++)
			{
				JoinNode.RouterAddr[JoinNode.RouterCount][i] = RfRx.RXDATA.Laddr[i];
			}
			for(i=0; i<2; i++)
			{
				JoinNode.RouterAddr[JoinNode.RouterCount][8+i] = RfRx.RXDATA.Saddr[1-i];
			}
				
			for(j=0; j<JoinNode.RouterCount; j++)//�ж������ظ�����Ľڵ�
			{
				HaveFlag = 1;
				for(i=0; i<8; i++)
				{
					if(JoinNode.RouterAddr[JoinNode.RouterCount][i] != JoinNode.RouterAddr[j][i])
					{
						HaveFlag = 0;
						break;//����
					}
				}
				if(HaveFlag == 0)continue;
				JoinNode.RouterCount--;//��
				JoinNode.RouterAddr[j][8] = RfRx.RXDATA.Saddr[1];
				JoinNode.RouterAddr[j][9] = RfRx.RXDATA.Saddr[0];	//�޸����������ַ
				break;
			}
			JoinNode.RouterCount++;
                }
                UartTX_Send_String(RfRx.RxBuf,20);                                     //�޸Ļ��壡��(UartStr,27)
	}
    else
   {
        if (RfRx.RXDATA.HeadCom[0] == 'D')
        {
            if (RfRx.RXDATA.HeadCom[1] == 'R')
            {
              sprintf(UartStr,(char *)"%c%c%c%c%c%c%s",RfRx.RXDATA.HeadCom[1],RfRx.RXDATA.Laddr[0],RfRx.RXDATA.Laddr[1],RfRx.RXDATA.Laddr[2],RfRx.RXDATA.Saddr[0],RfRx.RXDATA.Saddr[1],RfRx.RXDATA.DataBuf);
            }
            else if (RfRx.RXDATA.HeadCom[1] == 'E')
            {
              sprintf(UartStr,(char *)"%c%c%c%c%c%c%s",RfRx.RXDATA.HeadCom[2],RfRx.RXDATA.Laddr[0],RfRx.RXDATA.Laddr[1],RfRx.RXDATA.Laddr[2],RfRx.RXDATA.Saddr[0],RfRx.RXDATA.Saddr[1],RfRx.RXDATA.DataBuf);
            }
        }
        UartStr[26] = '\n';//�޸Ļ��壡��
        UartTX_Send_String(RfRx.RxBuf,20);                                     //�޸Ļ��壡��(UartStr,27)
    }
#endif
#if (defined(ZG_Router) || defined(ZG_ENDDEVICE))
/*
    SendData(RfRx.RxBuf, 0x0000, 16);//������Sink
    memcpy(RfRx.RxBuf,pkt->cmd.Data,20);                                          //�޸Ļ��壡��
    if((RfRx.RXDATA.HeadCom[0] == 'l')&&(RfRx.RXDATA.HeadCom[1] == 'o')&&(RfRx.RXDATA.HeadCom[2] == 'k'))
    {
        halWait(200);
        Frist_Data();
    }

    //�ն˽��յ���Ƶ��Ϣת����ARM
    UartTX_Send_String(RfRx.RxBuf,16);
    //��ȡARM������Ϣ��"16"Ϊ��ֹ��
    //uint8 temp = 1;
    //uint8 counter = 0;
    outputnum = 0;
    while((outputnum < 20) && (RfRx.RXDATA.DataBuf[outputnum-1] != 0x16))
    {
        RfRx.RXDATA.DataBuf[outputnum] = UartRX_Receive_Char();
        outputnum = outputnum + 1;
    }
    URX0IF = 0;//��մ��ڽ��ջ���
    //��ARM������Ϣ������Sink
    SendData(RfRx.RxBuf, 0x0000, 33);//������Sink
*/

    //���Գ���-2���ڵ��Է�����
    memcpy(RfRx.RxBuf,pkt->cmd.Data,pkt->cmd.DataLength);
    SendData(RfRx.RxBuf, 0x0000, 16);//������Sink

    UartTX_Send_String(RfRx.RxBuf,16);
    outputnum = 0;
    while((outputnum < 20) && (RfRx.RXDATA.DataBuf[outputnum-1] != 0x16))
    {
        RfRx.RXDATA.DataBuf[outputnum] = UartRX_Receive_Char();
        outputnum = outputnum + 1;
    }
    URX0IF = 0;//��մ��ڽ��ջ���
    //��ARM������Ϣ������Sink
    SendData(RfRx.RXDATA.DataBuf, 0x0000, 20);//������Sink
#endif	

}

/*********************************************************************
 * @fn      SampleApp_SendPeriodicMessage
 *
 * @brief   ����һ����������
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_SendPeriodicMessage( void )
{
  if ( AF_DataRequest( &SampleApp_Periodic_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_PERIODIC_CLUSTERID,
                       1,
                       (uint8*)&SampleAppPeriodicCounter,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
 * @fn      SampleApp_SendFlashMessage
 *
 * @brief   ����һ����˸��С����˸�����ڣ�����.
 *
 * @param   flashTime - ����
 *
 * @return  none
 */
void SampleApp_SendFlashMessage( uint16 flashTime )
{
  uint8 buffer[3];
  buffer[0] = (uint8)(SampleAppFlashCounter++);
  buffer[1] = LO_UINT16( flashTime );
  buffer[2] = HI_UINT16( flashTime );

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       3,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

//**********************************************************************
//**�Զ̵�ַ��ʽ��������
//buf ::���͵�����
//addr::Ŀ�ĵ�ַ
//Leng::���ݳ���
//********************************************************************
uint8 SendData(uint8 *buf, uint16 addr, uint8 Leng)
{
	afAddrType_t SendDataAddr;
	
	SendDataAddr.addrMode = (afAddrMode_t)Addr16Bit;         //�̵�ַ����
	SendDataAddr.endPoint = SAMPLEAPP_ENDPOINT;
	SendDataAddr.addr.shortAddr = addr;
        if ( AF_DataRequest( &SendDataAddr, //���͵ĵ�ַ��ģʽ
                       &SampleApp_epDesc,   //�նˣ��������ϵͳ������ID�ȣ�
                       2,//���ʹ�ID
                       Leng,
                       buf,
                       &SampleApp_TransID,  //��ϢID������ϵͳ������
                       AF_DISCV_ROUTE,
                     //  AF_ACK_REQUEST,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
	{
		return 1;
	}
	else
	{
		return 0;// Error occurred in request to send.
	}
}


//**********************************************************************
//**����չ��������ַ��ʽ��������
//buf ::���͵�����
//addr::Ŀ�ĵ�ַ
//Leng::���ݳ���
/********************************************************************
uint8 SendData(uint8 *buf, uint16 *addr, uint8 Leng)
{
	afAddrType_t SendDataAddr;
	
	SendDataAddr.addrMode = (afAddrMode_t)Addr64Bit;         //����ַ����
	SendDataAddr.endPoint = SAMPLEAPP_ENDPOINT;
	SendDataAddr.addr.shortAddr = addr;
        if ( AF_DataRequest( &SendDataAddr, //���͵ĵ�ַ��ģʽ
                       &SampleApp_epDesc,   //�նˣ��������ϵͳ������ID�ȣ�
                       2,//���ʹ�ID
                       Leng,
                       buf,
                       &SampleApp_TransID,  //��ϢID������ϵͳ������
                       AF_DISCV_ROUTE,
                     //  AF_ACK_REQUEST,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
	{
		return 1;
	}
	else
	{
		return 0;// Error occurred in request to send.
	}
}

********************************************************************
*********************************************************************/


//Graduation Test Gao Lao Zhuang Tested Wadnd Wanted Helld