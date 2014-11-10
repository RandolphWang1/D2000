///////////////////////////////////////////////////////////////////////////////
//    Copyright (c), Philips Semiconductors Gratkorn
//
//                  (C)PHILIPS Electronics N.V.2000
//       All rights are reserved. Reproduction in whole or in part is 
//      prohibited without the written consent of the copyright owner.
//  Philips reserves the right to make changes without notice at any time.
// Philips makes no warranty, expressed, implied or statutory, including but
// not limited to any implied warranty of merchantibility or fitness for any
//particular purpose, or that the use will not infringe any third party patent,
// copyright or trademark. Philips must not be liable for any loss or damage
//                          arising from its use.
///////////////////////////////////////////////////////////////////////////////
#include "Mfreg500.h"
#include "M500A.h"
#include "rdio.h"
#include "rc531.h"
//#include "uart_defs.h"


////////////////////////////////////////////////////////////////////////////////
//                M O D U L E   D E F I N I T I O N
////////////////////////////////////////////////////////////////////////////////
// COMMENT: This library module is modified from the original source code for a
//	    microcontroller C164 CI, to suit the general purpose 8051 mcu.
//          The source can be ported to other platforms very easily. 
//          The communication channel to the RC500 reader IC is assumed to be 
//          unknown. All data is written with the generic IO functions 
//          of the module ReaderIO.h. In our case the reader module is 
//          connected via memory mapped io at base address 0x7f00.
//          The interrupt pin of the reader IC is assumed to be connected to 
//          the fast external interrupt pin INT0# (active low) and the reset
//          pin of the reader IC should be connected to a dedicated port pin
//          (Port3: Pin: 3).
//          In this configuration, a reset of the reader module is independend
//          from the reset of the microcontroller.
//          In order to generate communication timeouts, 
//          general purpose timer 2 of the microcontroller is used. This 
//          timer need not to be initialised in advance. Before every usage 
//          the timer is completely initialised in each function. 
//          Non of the timers is essential for the functionality of the reader
//          module, but are helpful furing software development. All protocoll 
//          relevant timing constraints are generated
//          by the internal timer of the reader module.
//
//          Some explanations to the programming method of this library.
//          There are three kind of functions coded in this module.
//            a) internal functions, which have no prototypes in a header
//               file and are not intended to be used outside of this file
//            b) commands, which are intended for the reader module itself
//            c) commands, which are intended for any tag in the rf field.
//               These commands are send to the reader and the reader module
//               transmitts the data to the rf interface.
//          Commands for the reader and for the tag have the appropriate 
//          prefix (PCD for proximity coupled device or reader module
//                  PICC for proximity integrated circuit card or tag)
//          and their protypes are defined in the header file.
//          Each command for a PICC consists of a PCD command. Therefore
//          the function M500PcdCmd is very important for the understanding
//          of the communication.
//
//          The basic functionality is provided by the interrupt service
//          routine (ISR), which closely works together with the function
//          M500PcdCmd. All kinds of interrupts are serviced by the 
//          same ISR. 


// inline structure in order to reset the communication channel between 
// function and ISR
#define ResetInfo(info)    \
            info.cmd            = 0; \
            info.status         = MI_OK;\
            info.irqSource      = 0;   \
            info.nBytesSent     = 0;   \
            info.nBytesToSend   = 0;  \
            info.nBytesReceived = 0;  \
            info.nBitsReceived  = 0;  \
            info.collPos        = 0;


// modul variables 
unsigned char  *GpBase;

static   unsigned char  MFIFOLength = DEF_FIFO_LENGTH; // actual FIFO length

static   unsigned char  MKeys[16][12]; 				// storage for authentication keys
                                      						// in order to provide a calling 
                                      						// compatible interface to old libraries
                             								// Other reader modules keep several sets
                             								// of keys in an E2PROM. In this case,
                             								// these keys are stored in the uC and
                             								// transfered to the reader module 
                             								// before authentication

// Infomation concerning data processing
         // send buffer for general use
static   volatile unsigned char  MSndBuffer[SND_BUF_LEN];
         // receive buffer for general use
static   unsigned char  MRcvBuffer[RCV_BUF_LEN];
         // info struct for general use
static   volatile MfCmdInfo     MInfo;                  

// Interrupt service routine
// Variable in order to exchange data between function and ISR
static   volatile MfCmdInfo     *MpIsrInfo = 0; 
        // ISR send buffer
static   volatile unsigned char *MpIsrOut  = 0; 
         // ISR receive buffer
static   volatile unsigned char *MpIsrIn   = 0;     

// storage of the last selected serial number including check byte.
//For multi level serial numbers, only the first 4 bytes are stored.
unsigned char MLastSelectedSnr[5];

// Timer

int  LimitTimer_Flag = 0;	

//sbit    RC500RST        	= P3^5;
//sbit    LED	        	= P3^4;



///////////////////////////////////////////////////////////////////////////////
//                  Interrupt Handler RC500
///////////////////////////////////////////////////////////////////////////////
void ENT_RC531_HISR (void)     //Ext0 interrupt
{
   static unsigned char  irqBits;
   static unsigned char  irqMask;            
   static unsigned char  nbytes;
   static unsigned char  cnt;
   U8 data=0x11,  data1=0x22, data2=0x0, data3;
   //IE0 = 0; 	// Clear interrupt request flag
   
   mask_irq(INTSRC_EXINT6);

   if (MpIsrInfo && MpIsrOut && MpIsrIn)  // transfer pointers have to be set
                                          // correctly
   {    
     while((ReadIO(RegPrimaryStatus))& 0x08) // loop while IRQ pending
                                                // Attention: IRQ bit is inverted when used with low activ IRQ
      {
         irqMask = ReadIO(RegInterruptEn); // read enabled interrupts
         irqBits = ReadIO(RegInterruptRq) & irqMask;    // read pending interrupts
         MpIsrInfo->irqSource |= irqBits; // save pending interrupts	所产生中断的真正中断源
         
         //************ LoAlertIRQ ******************
        if (irqBits & 0x01)    // LoAlert		FIFO缓冲区变空
         {  
            nbytes = MFIFOLength - ReadIO(RegFIFOLength);
            // less bytes to send, than space in FIFO
            if ((MpIsrInfo->nBytesToSend - MpIsrInfo->nBytesSent) <= nbytes)
            {
               nbytes = MpIsrInfo->nBytesToSend - MpIsrInfo->nBytesSent;
               WriteIO(RegInterruptEn,0x01); // disable LoAlert IRQ
            }
            // write remaining data to the FIFO
            for ( cnt = 0;cnt < nbytes;cnt++)
            {
               WriteIO(RegFIFOData,MpIsrOut[MpIsrInfo->nBytesSent]);
               MpIsrInfo->nBytesSent++;
            }
           WriteIO(RegInterruptRq,0x01);  // reset IRQ bit
           
          // printf("11\n");
         }
       
         //************* TxIRQ Handling **************
         if (irqBits & 0x10)       // TxIRQ		Transceive 命令所有数据都已发送		Auth1 和Auth2 命令所有数据都已发送
         						   //			WriteE2 命令所有数据都已编程		CalcCRC 命令所有数据都已处理
         {
           WriteIO(RegInterruptRq,0x10);    // reset IRQ bit 
           WriteIO(RegInterruptEn,0x82);    // enable HiAlert Irq for response
            if (MpIsrInfo->cmd == PICC_ANTICOLL1) 	// if cmd is anticollision
	    	{                                           // switch off parity generation
               WriteIO(RegChannelRedundancy,0x02);	// RXCRC and TXCRC disable, parity disable
	    	}
	    	
	    	//printf("22\n");
         }
		  // ================ RxIRQ Handling ===============================
		/* 
		if (irqBits & 0x08) 					// RxIRQ - possible End of response processing
		 {
			// no error or collision during 
		 	//if (MpIsrInfo->DisableDF || (errorFlags == 0x00)) 
			//{
				 WriteIO(RegCommand,0x00); // cancel current command
				 irqBits |= 0x04; // set idle flag in order to signal the end of
				 // processing. For single reponse processing, this
				 // flag is already set.
				 // } 
			  else 
			  {// error occured - flush data and continue receiving
				  MpIsrInfo->saveErrorState = errorFlags; // save error state
				  WriteRC(RegControl,0x01);
				  MpIsrInfo->nBytesReceived = 0x00;
				  irqBits &= ~0x08; // clear interrupt request
				  WriteRawRC(RegInterruptRq,0x08);
			  }
		 }
		 */
			
         //************* HiAlertIRQ or RxIRQ Handling ******************
         if (irqBits & 0x0E) // HiAlert, Idle or RxIRQ		数据流从卡接收结束RxIRQ
         {
            // read some bytes ( length of FIFO queue)              
            // into the receive buffer
            nbytes = ReadIO(RegFIFOLength);
            // read date from the FIFO and store them in the receive buffer
            for ( cnt = 0; cnt < nbytes; cnt++)               
            {
               MpIsrIn[MpIsrInfo->nBytesReceived] = ReadIO(RegFIFOData);
            
               MpIsrInfo->nBytesReceived++;
            }
            WriteIO(RegInterruptRq,0x0A & irqBits);  //0000 1010
            
            //printf("33\n");
         }   
   
         //************** IdleIRQ Handling ***********
         if (irqBits & 0x04)     // Idle IRQ	命令执行完成
         {
            WriteIO(RegInterruptEn,0x20); 		// disable Timer IRQ
            WriteIO(RegInterruptRq,0x20); 		// disable Timer IRQ request
            irqBits &= ~0x20;   				// clear Timer IRQ in local var
            MpIsrInfo->irqSource &= ~0x20; 		// clear Timer IRQ in info var when idle received, then cancel timeout
            WriteIO(RegInterruptRq,0x04);  		// reset IRQ bit 
            // status should still be MI_OK
            // no error - only used for wake up
            
            //printf("44\n");
         }
       
         //************* TimerIRQ Handling ***********
         if (irqBits & 0x20)       // timer IRQ		定时器TimerValue 寄存器值减为0 时置位
         {
            WriteIO(RegInterruptRq,0x20); 		// reset IRQ bit 
            MpIsrInfo->status = MI_NOTAGERR; 	// timeout error, therwise ignore the interrupt
         }
      }
   
   }
   *(RP)GPIO_PORTA_INTRCLR |= 0x40;
   
    unmask_irq(INTSRC_EXINT6);

   return;
}



///////////////////////////////////////////////////////////////////////
//         S e t   T i m e o u t   L E N G T H
///////////////////////////////////////////////////////////////////////

void M500PcdSetTmo(unsigned char tmoLength)
{
 switch(tmoLength)
   {  // timer clock frequency 13,56 MHz
      case 0:                         // (0.302 ms) FWI=0
         WriteIO(RegTimerClock,0x07); // TAutoRestart=0,TPrescale=128
         WriteIO(RegTimerReload,0x21);// TReloadVal = 'h21 =33(dec) 
         break;
      case 1:                         // (0.604 ms) FWI=1
         WriteIO(RegTimerClock,0x07); // TAutoRestart=0,TPrescale=128
         WriteIO(RegTimerReload,0x41);// TReloadVal = 'h41 =65(dec) 
         break;
      case 2:                         // (1.208 ms) FWI=2
         WriteIO(RegTimerClock,0x07); // TAutoRestart=0,TPrescale=128
         WriteIO(RegTimerReload,0x81);// TReloadVal = 'h81 =129(dec) 
         break;
      case 3:                         // (2.416 ms) FWI=3
         WriteIO(RegTimerClock,0x09); // TAutoRestart=0,TPrescale=4*128
         WriteIO(RegTimerReload,0x41);// TReloadVal = 'h41 =65(dec) 
         break;
      case 4:                         // (4.833 ms) FWI=4
         WriteIO(RegTimerClock,0x09); // TAutoRestart=0,TPrescale=4*128
         WriteIO(RegTimerReload,0x81);// TReloadVal = 'h81 =129(dec) 
         break;
      case 5:                         // (9.666 ms) FWI=5
         WriteIO(RegTimerClock,0x0B); // TAutoRestart=0,TPrescale=16*128
         WriteIO(RegTimerReload,0x41);// TReloadVal = 'h41 =65(dec) 
         break;
      case 6:                         // (19.33 ms) FWI=6
         WriteIO(RegTimerClock,0x0B); // TAutoRestart=0,TPrescale=16*128
         WriteIO(RegTimerReload,0x81);// TReloadVal = 'h81 =129(dec) 
         break;
      case 7:                         // (38.66 ms) FWI=7
         WriteIO(RegTimerClock,0x0D); // TAutoRestart=0,TPrescale=64*128
         WriteIO(RegTimerReload,0x41);// TReloadVal = 'h41 =65(dec) 
         break;
      case 8:                         // (77.32 ms) FWI=8
         WriteIO(RegTimerClock,0x0D); // TAutoRestart=0,TPrescale=64*128
         WriteIO(RegTimerReload,0x81);// TReloadVal = 'h81 =129(dec) 
         break;
      case 9:                         // (154.6 ms) FWI=9
         WriteIO(RegTimerClock,0x0F); // TAutoRestart=0,TPrescale=256*128
         WriteIO(RegTimerReload,0x41);// TReloadVal = 'h41 =65(dec) 
         break;
      case 10:                        // (309.3 ms) FWI=10
         WriteIO(RegTimerClock,0x0F); // TAutoRestart=0,TPrescale=256*128
         WriteIO(RegTimerReload,0x81);// TReloadVal = 'h81 =129(dec) 
         break;
      case 11:                        // (618.6 ms) FWI=11
         WriteIO(RegTimerClock,0x13); // TAutoRestart=0,TPrescale=4096*128
         WriteIO(RegTimerReload,0x11);// TReloadVal = 'h21 =17(dec) 
         break;
      case 12:                        // (1.2371 s) FWI=12
         WriteIO(RegTimerClock,0x13); // TAutoRestart=0,TPrescale=4096*128
         WriteIO(RegTimerReload,0x21);// TReloadVal = 'h41 =33(dec) 
         break;
      case 13:                        // (2.4742 s) FWI=13
         WriteIO(RegTimerClock,0x13); // TAutoRestart=0,TPrescale=4096*128
         WriteIO(RegTimerReload,0x41);// TReloadVal = 'h81 =65(dec) 
         break;
      case 14:                        // (4.9485 s) FWI=14
         WriteIO(RegTimerClock,0x13); // TAutoRestart=0,TPrescale=4096*128
         WriteIO(RegTimerReload,0x81);// TReloadVal = 'h81 =129(dec) 
         break;
      default:                       // 
         WriteIO(RegTimerClock,0x07); // TAutoRestart=0,TPrescale=128
         WriteIO(RegTimerReload,tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
         break;
   }     
}


//////////////////////////////////////////////////////////////////////
//       W R I T E   A   P C D   C O M M A N D 
///////////////////////////////////////////////////////////////////////
char  M500PcdCmd(unsigned char cmd,
               volatile unsigned char* send, 
               volatile unsigned char* rcv,
               volatile MfCmdInfo *info)
{     
   char           status    = MI_OK;
   char           tmpStatus, data ;
   unsigned char  lastBits;
   static unsigned char  nbytes;
   static unsigned char  cnt;
   U32  timecnt,i;
   U8 data1;
    
   unsigned char  irqEn     = 0x00;
   unsigned char  waitFor   = 0x00;
   unsigned char  timerCtl  = 0x00;
   
   WriteIO(RegInterruptEn,0x7F); // disable all interrupts
   WriteIO(RegInterruptRq,0x7F); // reset interrupt requests
   WriteIO(RegCommand,PCD_IDLE); // terminate probably running command
   
   
   FlushFIFO();            // flush FIFO buffer

   // save info structures to module pointers
   MpIsrInfo = info;  
   MpIsrOut  = send;
   MpIsrIn   = rcv;

   info->irqSource = 0x0; // reset interrupt flags
   // depending on the command code, appropriate interrupts are enabled (irqEn)
   // and the commit interrupt is choosen (waitFor).
   switch(cmd)
   {
      case PCD_IDLE:                   // nothing else required
         irqEn = 0x00;
         waitFor = 0x00;
         break;
      case PCD_WRITEE2:                // LoAlert and TxIRq
         irqEn = 0x11;				   // 激活LoAlert中断,以便刚开始将数据写入FIFOData寄存器中
         waitFor = 0x10;
         break;
      case PCD_READE2:                 // HiAlert, LoAlert and IdleIRq
         irqEn = 0x07;				   // 激活LoAlert中断,以便刚开始将前三个字节(读取地址和读取字节数)
         							   // 写入FIFOData寄存器中
         waitFor = 0x04;
         break;
      case PCD_LOADCONFIG:             // IdleIRq
      case PCD_LOADKEYE2:              // IdleIRq
      case PCD_AUTHENT1:               // IdleIRq
         irqEn = 0x05;
         waitFor = 0x04;
         break;
      case PCD_CALCCRC:                // LoAlert and TxIRq
         irqEn = 0x11;
         waitFor = 0x10;
         break;
      case PCD_AUTHENT2:               // IdleIRq 
         irqEn = 0x04;
         waitFor = 0x04;
         break;
      case PCD_RECEIVE:                // HiAlert and IdleIRq
         info->nBitsReceived = -(ReadIO(RegBitFraming) >> 4);
         irqEn = 0x06;
         waitFor = 0x04;
         break;
      case PCD_LOADKEY:                // IdleIRq and LoAlert,需先将密钥内容装入FIFO中
         irqEn = 0x05;
         waitFor = 0x04;
         break;
      case PCD_TRANSMIT:               // LoAlert and IdleIRq
         irqEn = 0x05;
         waitFor = 0x04;
         break;
      case PCD_TRANSCEIVE:             // TxIrq, RxIrq, IdleIRq and LoAlert
	 	 info->nBitsReceived = -(ReadIO(RegBitFraming) >> 4);
         irqEn = 0x3D;
         waitFor = 0x04;
         break;
      default:
         status = MI_UNKNOWN_COMMAND;
   }        
   if (status == MI_OK)
   {
      // Initialize uC Timer for global Timeout management
     irqEn |= 0x20;                        	// always enable timout irq
     waitFor |= 0x20;                      	// always wait for timeout 

     timecnt = 2000;         				// initialise and start guard timer for reader 200ms
      

    
     WriteIO(RegInterruptEn,irqEn | 0x80);  			//necessary interrupts are enabled

     WriteIO(RegCommand,cmd); 
    
  
      while (!(MpIsrInfo->irqSource & waitFor))   // // wait for cmd completion or timeout
     {
     	if(timecnt <=0 )
     		break;
     	else timecnt--;
     }


      WriteIO(RegInterruptEn,0x7F);          	// disable all interrupts
      WriteIO(RegInterruptRq,0x7F);          	// clear all interrupt requests
      SetBitMask(RegControl,0x04);           	// stop timer now

      							
      LimitTimer_Flag = 0;
      WriteIO(RegCommand,PCD_IDLE);         	// reset command register


      if (!(MpIsrInfo->irqSource & waitFor))   	// reader has not terminated timer 2 expired
      {                                			// 在对PICC卡发送一个命令后,有时要执行好几种中断,只有符合waitFor要求的中断才能退出等待．　一般waitFor中设定了两个种情况退出源：1.TIMEROUT ERROR,2.命令执行完了.   waitFor设定了能够退出while循环的中断源
         status = MI_ACCESSTIMEOUT;
      }
      else
         status = MpIsrInfo->status;           // set status

      if (status == MI_OK)                     // no timeout error occured
      {
         if (tmpStatus = (ReadIO(RegErrorFlag) & 0x17)) // error occured
         {
            if (tmpStatus & 0x01)   // collision detected
            {
               info->collPos = ReadIO(RegCollpos); // read collision position
               status = MI_COLLERR;
            }
            else
            {
               info->collPos = 0;
               if (tmpStatus & 0x02)   // parity error
               {
                  status = MI_PARITYERR;
               }
            }
            if (tmpStatus & 0x04)   // framing error
            {
               status = MI_FRAMINGERR;
            }
            if (tmpStatus & 0x10)   // FIFO overflow
            {
               FlushFIFO();
               status = MI_OVFLERR;
            }
	 	    if (tmpStatus & 0x08) //CRC error
		    {
	               status = MI_CRCERR;
		    }	
            if (status == MI_OK)
               status = MI_NY_IMPLEMENTED;
            // key error occures always, because of 
            // missing crypto 1 keys loaded
         }
         // if the last command was TRANSCEIVE, the number of 
         // received bits must be calculated - even if an error occured
         if (cmd == PCD_TRANSCEIVE)
         {
            // number of bits in the last byte
            lastBits = ReadIO(RegSecondaryStatus) & 0x07;
            if (lastBits)
               info->nBitsReceived += (info->nBytesReceived-1) * 8 + lastBits;
            else
               info->nBitsReceived += info->nBytesReceived * 8;
         }
      }
      else
      {
         info->collPos = 0x00;
      }
   }
   MpIsrInfo = 0;         // reset interface variables for ISR
   MpIsrOut  = 0;
   MpIsrIn   = 0; 
   return status;
}   

//////////////////////////////////////////////////////////////////////
//   S E T   A   B I T   M A S K 
///////////////////////////////////////////////////////////////////////
char SetBitMask(unsigned char reg,unsigned char mask) // 
{
   char  tmp = 0x0;
   U8  data;

   tmp = ReadIO(reg);
   data = tmp|mask;
   WriteIO(reg,tmp | mask);  // set bit mask
   tmp = ReadIO(reg);
   return 0x0;
}

//////////////////////////////////////////////////////////////////////
//   C L E A R   A   B I T   M A S K 
///////////////////////////////////////////////////////////////////////
char ClearBitMask(unsigned char reg,unsigned char mask) // 
{
   char  tmp = 0x0;
   U8 data;
   tmp = ReadIO(reg);
   data = tmp & ~mask;
   WriteIO(reg,tmp & ~mask);  // clear bit mask
   tmp = ReadIO(reg);
   return 0x0;
}

///////////////////////////////////////////////////////////////////////
//                  F L U S H    F I F O
///////////////////////////////////////////////////////////////////////
void FlushFIFO(void)
{  
   SetBitMask(RegControl,0x01);
}

///////////////////////////////////////////////////////////////////////
//      M I F A R E   M O D U L E   R E S E T 
///////////////////////////////////////////////////////////////////////
char M500PcdReset(void)
{
   S32  status = MI_OK;
   //U16 timecnt = 2500;
   //U8 data;
//   INT out_data1,out_data2;
   //////产生复位时序MF_RST(PG6)//////
   RC531RST_L;  		// clear reset pin
   delay_1ms(25);  		// wait for 25ms    
   RC531RST_H;   		// reset RC500
   delay_50us(50);  	// wait for 2.5ms
   RC531RST_L;  		// clear reset pin
   
    	// count down with a period of 20 ns
   			        		// 2100 * 1 ms = 2.1 s
   // out_data1 = ReadIO(RegCommand);	在此读出值为0xff,不知道为什么???   		
  // wait until reset command recognized
  // 在整个启动阶段中Command 值读出为3FH 在初始化阶段的结束MF RC500 自动输入Idle 命令
  // 结果Command 值变为00H
	while(ReadIO(RegCommand)!= 0x00)

/*
  while (((ReadIO(RegCommand) & 0x3F) != 0x3F) && (!LimitTimer_Flag));
  // while reset sequence in progress
  while ((ReadIO(RegCommand) & 0x3F) && (!LimitTimer_Flag)); 
  */ 
    		

  // if (LimitTimer_Flag) 				// If reader timeout occurs
  // {
  //    status = MI_RESETERR; 			// respose of reader IC is not correct
  //    LimitTimer_Flag = 0;				// 清除超时标志
 //  }
 //  else
 //  {
      WriteIO(RegPage,0x80); // Dummy access in order to determine the bus configuration
      //data = ReadIO(RegPage);
      // necessary read access 
      // after first write access, the returned value
      // should be zero ==> interface recognized
     if ((ReadIO(RegCommand)) != 0x00)
      {                           
          status = MI_INTERFACEERR;
      }
      WriteIO(RegPage,0x00); // configure to linear address mode
     // data = ReadIO(RegPage);
  // }

   return status;
}

///////////////////////////////////////////////////////////////////////
//      M I F A R E   M O D U L E   C O N F I G U R A T I O N
///////////////////////////////////////////////////////////////////////
char M500PcdConfig(void)
{
   char  status;
   char  i;
   char  j;

   if ((status = M500PcdReset()) == MI_OK)
   {
     // test clock Q calibration - value in the range of 0x46 expected
     WriteIO(RegClockQControl,0x0);			//时钟Q校准
         
     WriteIO(RegClockQControl,0x40);		//0x1F---Q-时钟在复位后和从卡接收数据后自动校准

     delay_50us(2);  						// wait approximately 100 us - calibration in progress
     ClearBitMask(RegClockQControl,0x40); 	// clear bit ClkQCalib for further calibration
     
                                               // 清除CLkQCalib，允许自动校准 

     WriteIO(RegBitPhase,0xAD);      		//0x1B---定义发送器和接收器时钟之间的位相位
	// status = ReadIO(RegBitPhase);

	// printf("0x%c\n",status);										
     // initialize minlevel
     
     WriteIO(RegRxThreshold,0xFF);			//0x1C---选择位解码器的阀值   
    // printf("0x%x\n",ReadIO(RegRxThreshold));
     // disable auto power down
    
     WriteIO(RegRxControl2,0x01);			//0x1E---bit7:为0 表示使用Q-时钟; bit6:为0 接收器始终有效
   //  printf("0x%x\n",ReadIO(RegRxControl2));
     // Depending on the processing speed of the operation environment, the waterlevel 
     // can be adapted. (not very critical for mifare applications)
     // initialize waterlevel to value 4
     WriteIO(RegFIFOLevel,0x04);   			//0x29---定义FIFO 上溢和下溢警告界限
   //  printf("0x%x\n",ReadIO(RegFIFOLevel));
     //Timer configuration
     WriteIO(RegTimerControl,0x08);  		// TStopRxEnd=0,TStopRxBeg=0, TStartTxEnd=1,TStartTxBeg=0  
   //  printf("0x%x\n",ReadIO(RegTimerControl));                     					// timer must be stopped manually发送结束开定时器，接收开始关定时器
     M500PcdSetTmo(106);               		// short timeout
											// 设定时器为10ms 原程序中参数为1
     WriteIO(RegIRqPinConfig, 0x02);			// 0x2D---interrupt active low enable
   //  printf("0x%x\n",ReadIO(RegIRqPinConfig));  
     M500PcdRfReset(1);            			// Rf - reset and enable output driver   
	 M500PcdConfigISOType(TYPEA);


   }
   return status;
}



///////////////////////////////////////////////////////////////////////
//          C O N F I G   I S O 1 4 4 4 3   T Y P E 
///////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(unsigned char type)
{
   U8 data;
   if(type==TYPEA)
   {
     WriteIO(RegTxControl,0x5b); 		// Force100ASK, TX1 & TX2 enable
     WriteIO(RegCoderControl,0x19);     // Miller coding, 106kbps
     WriteIO(RegRxControl1,0x73);
     WriteIO(RegDecoderControl,0x08);   // Manchester Coding
     WriteIO(RegCRCPresetLSB,0x63);     // set CRC preset to 0x6363
     WriteIO(RegCRCPresetMSB,0x63);
     WriteIO(RegRxThreshold,0xff);  	// set max MinLevel & ColLevel.
//     TYPE = TYPEA;	
   }
   else
   {
     WriteIO(RegTxControl,0x4b);      	// disable Force100ASk
     WriteIO(RegCoderControl,0x20);     // NRZ-L, TypeB baud 106kbps
     WriteIO(RegRxControl1,0x73);       //
     WriteIO(RegDecoderControl,0x19);   // BPSK coding
     WriteIO(RegCRCPresetLSB,0xff);     // set CRC preset to 0xffff
     WriteIO(RegCRCPresetMSB,0xff);
     WriteIO(RegTypeBFraming,0x23);     // EGT=0
     WriteIO(RegBPSKDemControl,0x3e);   // ignore EOF, on amp. detect
     WriteIO(RegModConductance,0x06);	// set modulation index at 12%
     WriteIO(RegRxThreshold,0x44);  // Reduce MinLevel & ColLevel.
				    // Increase higher nibble if carrier
				    // present but not detect
     //TYPE = TYPEB;
   }
   return MI_OK;
}

  
///////////////////////////////////////////////////////////////////////
//          M I F A R E    R E Q U E S T 
///////////////////////////////////////////////////////////////////////
char  M500PiccRequest(unsigned char req_code, 	// request code ALL = 0x52 
                                             	// or IDLE = 0x26 
                   unsigned char *atq)     		// answer to request
{
   return M500PiccCommonRequest(req_code, atq);
}

///////////////////////////////////////////////////////////////////////
//          M I F A R E   C O M M O N   R E Q U E S T	放入操作范围之内的卡片进行Request操作
// 控制单元->射频卡
// Command: 0x26 or 0x52
// 0x26: IDLE 模式，只选择天线范围内IDLE 模式的卡片
// 0x52: ALL 模式，选择天线范围内所有卡片
// Len: 0
// 射频卡->控制单元
// Len： 2
// Data[0]： _TagType（低字节）0x04
// Data[1]： _TagType（高字节）0x00
// 在重新选择卡片时必须执行request 操作
///////////////////////////////////////////////////////////////////////
char M500PiccCommonRequest(unsigned char req_code, unsigned char *atq)                         
{
   char  status = MI_OK;
   //int iodata=0;
    //************* initialize ******************************
   
   WriteIO(RegChannelRedundancy,0x03); 	// RxCRC and TxCRC disable, parity enable	关闭CRC
   ClearBitMask(RegControl,0x08);    	// disable crypto 1 unit   屏蔽CRYPTO1 位
   WriteIO(RegBitFraming,0x07);        	// set TxLastBits to 7 发送7bit
   SetBitMask(RegTxControl,0x03);    	// Tx2RF-En, Tx1RF-En enable	开启TX1、TX2
   
   M500PcdSetTmo(106);
   
   ResetInfo(MInfo);   
   MSndBuffer[0] = req_code;
   MInfo.nBytesToSend   = 1;   
   status = M500PcdCmd(PCD_TRANSCEIVE,
                      MSndBuffer,
                      MRcvBuffer,
                      &MInfo);
  
   if (status)      // error occured
   {
      *atq = 0;
   } 
   else 
   {
      if (MInfo.nBitsReceived != 16) 	// 2 bytes expected
      {
         *atq = 0;
         status = MI_BITCOUNTERR;
      } 
      else 
      {
         status = MI_OK;
         memcpy(atq,MRcvBuffer,2);		//返回卡的类型	temp1 = 03; 上海标准TOKEN 卡
										//				temp1 = 04; MIFARE 标准8K
										//				temp1 = 05; MIFARE 标准TOKEN 卡
										//				temp1 = 53; 上海标准8K 卡
      }									//MIRARE 4k(0X0002)		MIRARE DESFire(0x0344)
   }
   return status; 
}

///////////////////////////////////////////////////////////////////////
//          M I F A R E    A N T I C O L L I S I O N	对放入操作范围之内的卡片的防冲突检测
// for standard select
///////////////////////////////////////////////////////////////////////
char M500PiccAnticoll (unsigned char code, unsigned char bcnt, unsigned char *snr)                    
{
   return M500PiccCascAnticoll(code, bcnt, snr); // first cascade level
}

///////////////////////////////////////////////////////////////////////
//          M I F A R E    A N T I C O L L I S I O N
// for extended serial numbers
// 控制单元->射频卡
// Command: 0x93(指令代码后的数值!=0x70)
// Len: 1
// Data[0]: 0x20 NVB
// 射频卡->控制单元
// Len: 5(前四字节为真正序列号,后一字节为校验码)
// Data[0]: _Snr(LL)
// Data[1]: _Snr(LH)
// Data[2]: _Snr(HL) 卡片系列号
// Data[3]: _Snr(HH)
// Data[4]: BCC
// 此操作必须紧随在request 操作后执行.如果被选的卡片的系列号已知，可以不用执行此操作
///////////////////////////////////////////////////////////////////////
char M500PiccCascAnticoll (unsigned char select_code,
                           unsigned char bcnt,       
                           unsigned char *snr)       
{
   char  status = MI_OK;
   char  snr_in[4];     			// copy of the input parameter snr
   char  nbytes = 0;
   char  nbits = 0;
   char  complete = 0;
   char  i        = 0;
   char  byteOffset = 0;
   unsigned char  snr_crc;
   unsigned char  snr_check;
   unsigned char dummyShift1;       // dummy byte for snr shift
   unsigned char dummyShift2;       // dummy byte for snr shift   
 
   //************* Initialisation ******************************
   M500PcdSetTmo(106);
   memcpy(snr_in,snr,4);
   
   WriteIO(RegDecoderControl,0x28); 	// ZeroAfterColl aktivieren  控制解码器状态 
   ClearBitMask(RegControl,0x08);    	// disable crypto 1 unit

   //************** Anticollision Loop ***************************
   // 采用动态二进制搜索法
   complete = 0;
//   bcnt = 0;   // no part of the snr is known
   while (!complete && (status == MI_OK) )
   {
      ResetInfo(MInfo);           
      WriteIO(RegChannelRedundancy,0x03); 	// RxCRC and TxCRC disable, parity enable
      nbits = bcnt % 8;   					// remaining number of bits
      if (nbits)
      {
         WriteIO(RegBitFraming,nbits << 4 | nbits); // TxLastBits/RxAlign auf nb_bi
         nbytes = bcnt / 8 + 1;   
         // number of bytes known

         // in order to solve an inconsistancy in the anticollision sequence
         // (will be solved soon), the case of 7 bits has to be treated in a
         // separate way - please note the errata sheet
         if (nbits == 7)
         {
            MInfo.cmd = PICC_ANTICOLL1;   // pass command flag to ISR        
            WriteIO(RegBitFraming,nbits); // reset RxAlign to zero
         }
      } 
      else
      {
         nbytes = bcnt / 8;
      }

      MSndBuffer[0] = select_code;
      MSndBuffer[1] = 0x20 + ((bcnt/8) << 4) + nbits; //number of bytes send
               
      for (i = 0; i < nbytes; i++)  // Sende Buffer beschreiben
      {
         MSndBuffer[i + 2] = snr_in[i];
      }
      MInfo.nBytesToSend   = 2 + nbytes;   

      status = M500PcdCmd(PCD_TRANSCEIVE,
                         MSndBuffer,
                         MRcvBuffer,
                         &MInfo);
    
      // in order to solve an inconsistancy in the anticollision sequence
      // (will be solved soon), the case of 7 bits has to be treated in a
      // separate way 
      if (nbits == 7)
      {
         // reorder received bits
         dummyShift1 = 0x00;
         for (i = 0; i < MInfo.nBytesReceived; i++)
         {
            dummyShift2 = MRcvBuffer[i];
            MRcvBuffer[i] = (dummyShift1 >> (i+1)) | (MRcvBuffer[i] << (7-i));
            dummyShift1 = dummyShift2;
         }
         MInfo.nBitsReceived -= MInfo.nBytesReceived; // subtract received parity bits
         // recalculation of collision position
         if ( MInfo.collPos ) 
         	MInfo.collPos += 7 - (MInfo.collPos + 6) / 9;
      }
         
      if ( status == MI_OK || status == MI_COLLERR)    // no other occured
      {
         // R e s p o n s e   P r o c e s s i n g   
         if ( MInfo.nBitsReceived != (40 - bcnt) ) 	// not 5 bytes answered
         {
            status = MI_BITCOUNTERR; 				// Exit with error
         } 
         else 
         {
            byteOffset = 0;
            if( nbits != 0 ) 						// last byte was not complete
            {
                snr_in[nbytes - 1] = snr_in[nbytes - 1] | MRcvBuffer[0];
                byteOffset = 1;
            }

            for ( i =0; i < (7 - nbytes); i++)     
            {
               snr_in[nbytes + i] = MRcvBuffer[i + byteOffset];
            }
  
            if (status != MI_COLLERR ) 				// no error and no collision
            {
               // SerCh check
               snr_crc = snr_in[0] ^ snr_in[1] ^ snr_in[2] ^ snr_in[3];
               snr_check = MRcvBuffer[MInfo.nBytesReceived - 1];
               if (snr_crc != snr_check)
               {
                  status = MI_SERNRERR;
               } 
               else   
               {
                  complete = 1;
               }
            }
            else                   // collision occured
            {
               bcnt = bcnt + MInfo.collPos - nbits;
               status = MI_OK;
            }
         }
      }
   }
   if (status == MI_OK)
   {
      // transfer snr_in to snr
      memcpy(snr,snr_in,4);
   }
   else
   {
      memcpy(snr,"0000",4);
   }

   //----------------------Einstellungen aus Initialisierung ruecksetzen 
   ClearBitMask(RegDecoderControl,0x20); // ZeroAfterColl disable
   
   return status;  
}


///////////////////////////////////////////////////////////////////////
//          M I F A R E    S E L E C T 			选择卡片
// for std. select
///////////////////////////////////////////////////////////////////////
char M500PiccSelect(unsigned char code, unsigned char *snr, 
                  unsigned char *sak)
{
   return M500PiccCascSelect(code,snr,sak); // first cascade level
}

///////////////////////////////////////////////////////////////////////
//          M I F A R E    C A S C A D E D   S E L E C T 
//  for extended serial number
// 控制单元->射频卡
// Command: 0x93
// Len: 6
// Data[0]: 0x70
// Data[1]: _Snr(LL)
// Data[2]: _Snr(LH)
// Data[3]: _Snn(HL)
// Data[4]: _Snr(HH) 卡片系列号(UID)
// Data[5]: BCC
// 射频卡->控制单元
// Len: 1(卡片发送置于其0扇区块0的1字节卡片容量信息Size作为应答)
// Data[0]: _Size (卡片容量值：0x08 或0x88)
///////////////////////////////////////////////////////////////////////
char M500PiccCascSelect(unsigned char select_code, 
                        unsigned char *snr,
                        unsigned char *sak)
{
   char  status = MI_OK; 
 
   M500PcdSetTmo(106);
	
   WriteIO(RegChannelRedundancy,0x0F); // RxCRC,TxCRC, Parity enable
   ClearBitMask(RegControl,0x08);    // disable crypto 1 unit

   //************* Cmd Sequence ********************************** 
   ResetInfo(MInfo);   
   MSndBuffer[0] = select_code;
   MSndBuffer[1] = 0x70;         // number of bytes send
   
   memcpy(MSndBuffer + 2,snr,4);
   MSndBuffer[6] = MSndBuffer[2] 
                   ^ MSndBuffer[3] 
                   ^ MSndBuffer[4] 
                   ^ MSndBuffer[5];
   MInfo.nBytesToSend   = 7;
   status = M500PcdCmd(PCD_TRANSCEIVE,
                       MSndBuffer,
                       MRcvBuffer,
                       &MInfo);

   *sak = 0;   
   if (status == MI_OK)    // no timeout occured
   {
      if (MInfo.nBitsReceived != 8)    // last byte is not complete
      {
         status = MI_BITCOUNTERR;
      }
      else
      {
	 	 *sak = MRcvBuffer[0];
         memcpy(MLastSelectedSnr,snr,4);            
      }	
   }
  
   return status;
}


///////////////////////////////////////////////////////////////////////
//          M I F A R E      A U T H E N T I C A T I O N
//   calling compatible version    
// 调用些函数进行AUTHENTION过程时,密钥由程序自己提供给FIFO,再放入密匙缓冲区
///////////////////////////////////////////////////////////////////////
char M500PiccAuth(unsigned char keyAB,      // KEYA or KEYB
               unsigned char *snr,        	// 4 bytes card serial number		卡片序列号4字节
               unsigned char *key_addr,    	// key address in reader storage	密钥的地址
               unsigned char block)       	// block number which should be 	需要认证的块号
                                         	// authenticated
{
   char    status = MI_OK;
 //  unsigned char  * key = 0;
   unsigned char   keycoded[12];
  // unsigned char   offset = (keyAB == PICC_AUTHENT1A) ? 0 : 6;   

   // 准备密钥过程 
  // key = MKeys[key_addr] + offset;		
   M500HostCodeKey(key_addr,keycoded);			// KeyA,KeyB均为6字节,转换后为12字节

  // M500HostCodeKey(key,keycoded);			// KeyA,KeyB均为6字节,转换后为12字节
   status = M500PiccAuthKey(keyAB,
                            snr,
                            keycoded,
                            block);  
  return status;
}


///////////////////////////////////////////////////////////////////////
//                  A U T H E N T I C A T I O N   
//             W I T H   P R O V I D E D   K E Y S
///////////////////////////////////////////////////////////////////////
char M500PiccAuthKey(unsigned char auth_mode,			// KEYA or KEYB
                     unsigned char *snr,       			// 卡片序列号4字节
                     unsigned char *keys,  				// 转换好的密钥    
                     unsigned char block)      			// 需要认证的块号
{
   char  status = MI_OK;
   unsigned char  i = 0;
   
   FlushFIFO();    						// empty FIFO
   ResetInfo(MInfo);
   memcpy(MSndBuffer,keys,12);          // write 12 bytes of the key
   MInfo.nBytesToSend = 12;
   // write load command
   // 从FIFO 缓冲区读出密匙字节并将其放入密匙缓冲区
   // 需先将12字节密钥放入FIFO中
   if ((status=M500PcdCmd(PCD_LOADKEY,MSndBuffer,MRcvBuffer,&MInfo)) == MI_OK)
   {      
      // execute authentication
      status = M500PiccAuthState(auth_mode,snr,block); 
   }
   return status;
}


///////////////////////////////////////////////////////////////////////
//                      C O D E   K E Y S  				
// 密钥转换,转换成符合规范的格式
///////////////////////////////////////////////////////////////////////
char M500HostCodeKey(  unsigned char *uncoded, // 6 bytes key value uncoded
                     unsigned char *coded)   // 12 bytes key value coded
{
   char  status = MI_OK;
   unsigned char  cnt = 0;
   unsigned char  ln  = 0;     // low nibble
   unsigned char  hn  = 0;     // high nibble
   
   for (cnt = 0; cnt < 6; cnt++)
   {
      ln = uncoded[cnt] & 0x0F;
      hn = uncoded[cnt] >> 4;
      coded[cnt * 2 + 1] = (~ln << 4) | ln;
      coded[cnt * 2 ] = (~hn << 4) | hn;

   }
   return status;
}


///////////////////////////////////////////////////////////////////////
//                  A U T H E N T I C A T I O N   
//             W I T H   K E Y S   F R O M   E 2 P R O M
///////////////////////////////////////////////////////////////////////
char M500PiccAuthE2( unsigned char auth_mode,   // KEYA, KEYB
                     unsigned char *snr,        // 4 bytes card serial number
                     unsigned char key_sector,  // key address in reader storage, 
						// 0 <= key_sector <= 15                     
                     unsigned char block)      // block number which should be 
                                               // authenticated
					       // 0 <= block <= 256
{
   char  status = MI_OK;
   // eeprom address calculation
   // 0x80 ... offset
   // key_sector ... sector
   // 0x18 ... 2 * 12 = 24 = 0x18
   unsigned short e2addr = 0x80 + key_sector * 0x18;
   unsigned char *e2addrbuf = (unsigned char*)&e2addr;
   

   if (auth_mode == PICC_AUTHENT1B)
      e2addr += 12; // key B offset   
   FlushFIFO();    // empty FIFO
   ResetInfo(MInfo);

   memcpy(MSndBuffer,e2addrbuf,2); // write low and high byte of address
   MSndBuffer[2] = MSndBuffer[0];  // Move the LSB of the 2-bytes
   MSndBuffer[0] = MSndBuffer[1];  // address to the first byte
   MSndBuffer[1] = MSndBuffer[2];  // 交换两个地址高低字节(起始地址低字节---起始地址高字节)
   MInfo.nBytesToSend   = 2;
    // write load command
   // 将一个密匙从E2PROM 复制到密匙缓冲区
   if ((status=M500PcdCmd(PCD_LOADKEYE2,MSndBuffer,MRcvBuffer,&MInfo)) == MI_OK)
   {      
      // execute authentication
      status = M500PiccAuthState(auth_mode,snr,block);  
   }
   return status;
}                        


///////////////////////////////////////////////////////////////////////
//        A U T H E N T I C A T I O N   S T A T E S
// 控制单元->射频卡
// Command: 0x60 or 0x61
// Len: 2
// Data[0]: 0x60 or 0x61 (0x60 使用KEYA 作验证，0x61 使KEYB 作验证)
// Data[1]: _SecNr （扇区号）*4(即每个扇区的块0 的块地址)
// 射频卡->控制单元
// Len: 0
// 如果读写模块中的密码与卡片中的密码相匹配，则可以进行读、写等操作。
///////////////////////////////////////////////////////////////////////
char M500PiccAuthState( unsigned char auth_mode,
                        unsigned char *snr,
                        unsigned char block)
{
   char  status = MI_OK;
   unsigned char  i = 0;
   
   status = ReadIO(RegErrorFlag);   // read error flags of the previous
                                    // key load
   if (status != MI_OK)
   {
      if (status & 0x40)            // key error flag set
         status = MI_KEYERR;
      else
         status = MI_AUTHERR;       // generic authentication error 
   }
   else
   {
      MSndBuffer[0] = auth_mode;        // write authentication command

      MSndBuffer[1] = block;    // write block number for authentication
      memcpy(MSndBuffer + 2,snr,4); // write 4 bytes card serial number 
      ResetInfo(MInfo);
      MInfo.nBytesToSend = 6;
      if ((status = M500PcdCmd(PCD_AUTHENT1,
                               MSndBuffer,
                               MRcvBuffer,
                               &MInfo)) == MI_OK)
      {
         if (ReadIO(RegSecondaryStatus) & 0x07) // Check RxLastBits for error
         {
            status = MI_BITCOUNTERR;
         }
         else
         {
            ResetInfo(MInfo);
            MInfo.nBytesToSend = 0;
            if ((status = M500PcdCmd(PCD_AUTHENT2,
                                     MSndBuffer,
                                     MRcvBuffer,
                                     &MInfo)) == MI_OK) 
            {
               if ( ReadIO(RegControl) & 0x08 ) // Crypto1 activated
               {
                   status = MI_OK;
               }
               else
               {
                   status = MI_AUTHERR;
               }
            }
         }
      }
   }
   return status;
}


///////////////////////////////////////////////////////////////////////
//          M I F A R E   R E A D   
// 控制单元->射频卡
// Command: 0x30
// Len: 1
// Data[0]: _Adr 块地址（0～63）
// 射频卡->控制单元
// Len: 16
// Data[0]: 数据块的第一字节
//  :
// Data[15]：数据块的最后一个字节
///////////////////////////////////////////////////////////////////////
char M500PiccRead(  unsigned char addr,
                  unsigned char *_data)
{
   char  status = MI_OK;
   char  tmp    = 0;

   FlushFIFO();    						// empty FIFO

   M500PcdSetTmo(3);     				// long timeout 

   WriteIO(RegChannelRedundancy,0x0F); 	// RxCRC, TxCRC, Parity enable
   
   // ************* Cmd Sequence ********************************** 
   ResetInfo(MInfo);   
   MSndBuffer[0] = PICC_READ;   // read command code
   MSndBuffer[1] = addr;
   MInfo.nBytesToSend   = 2;   
   status = M500PcdCmd(PCD_TRANSCEIVE,
                       MSndBuffer,
                       MRcvBuffer,
                       &MInfo);

   if (status != MI_OK)
   {
      if (status != MI_NOTAGERR ) // no timeout occured
      {
         if (MInfo.nBitsReceived == 4)  // NACK
         {
             MRcvBuffer[0] &= 0x0f;  // mask out upper nibble
             if ((MRcvBuffer[0] & 0x0a) == 0)
             {
                status = MI_NOTAUTHERR;
             }
             else
             {
                status = MI_CODEERR;
             }
          }
      }
      memcpy(_data,"0000000000000000",16); // in case of an error initialise 
                                          // data
   }
   else   // Response Processing
   {
      if (MInfo.nBytesReceived != 16)
      {
         status = MI_BYTECOUNTERR;
         memcpy(_data,"0000000000000000",16);
      }
      else
      {
         memcpy(_data,MRcvBuffer,16);
      }
   }
   M500PcdSetTmo(1);               // short timeout   
   return status; 
}


///////////////////////////////////////////////////////////////////////
//          M I F A R E   W R I T E     
// 控制单元->射频卡
// Command: 0xA0
// Len: 1
// Data[0]: _Adr 要写入数据的块地址（1～63）
// 射频卡->控制单元
// Len：17
// DATA[0]: 0x0A(ACK)
// 控制单元->射频卡
// Data[1]: 要写入卡片中的第一个数据
//  :
// Data[16]: 要写入卡片中的最后一个数据
// 射频卡->控制单元
// Len: 4Bit
// DATA[0]: 0x0A(ACK)
///////////////////////////////////////////////////////////////////////
char M500PiccWrite( unsigned char addr,
                  unsigned char *_data)
{
   char  status = MI_OK;
   U8 data2;
     // ************* Cmd Sequence ********************************** 
     ResetInfo(MInfo);   
     MSndBuffer[0] = PICC_WRITE;        // Write command code
     MSndBuffer[1] = addr;
     MInfo.nBytesToSend   = 2;
    
     status = M500PcdCmd(PCD_TRANSCEIVE,
                         MSndBuffer,
                         MRcvBuffer,
                         &MInfo);

     if (status != MI_NOTAGERR)   // no timeout error
     {
        if (MInfo.nBitsReceived != 4)   // 4 bits are necessary
        {
           status = MI_BITCOUNTERR;
        }
        else                     // 4 bit received
        {
           MRcvBuffer[0] &= 0x0f; // mask out upper nibble
           if ((MRcvBuffer[0] & 0x0a) == 0)
           {
              status = MI_NOTAUTHERR;                   
           }
           else
           {
              if (MRcvBuffer[0] == 0x0a)
              {
                 status = MI_OK;
              }
              else 
              {
                 status = MI_CODEERR;
              }
           }
        }
     }
    
     if ( status == MI_OK)
     {
        M500PcdSetTmo(3);     // long timeout 
       
        ResetInfo(MInfo);   
        memcpy(MSndBuffer,_data,16);
        MInfo.nBytesToSend   = 16;
       
        status = M500PcdCmd(PCD_TRANSCEIVE,
                            MSndBuffer,
                            MRcvBuffer,
                            &MInfo);
       
        if (status & 0x80)    // timeout occured
        {
           status = MI_NOTAGERR;
        }
        else
        {
         
           if (MInfo.nBitsReceived != 4)  // 4 bits are necessary
           {
              status = MI_BITCOUNTERR;
           }
           else                     // 4 bit received
           {
              MRcvBuffer[0] &= 0x0f; // mask out upper nibble
              if ((MRcvBuffer[0] & 0x0a) == 0)
              {
                 status = MI_WRITEERR;
              }
              else
              {
                 if (MRcvBuffer[0] == 0x0a)
                 {
                    status = MI_OK;
                 }
                 else 
                 {
                    status = MI_CODEERR;
                 }
              }     
           }
        }        
        M500PcdSetTmo(1);    // short timeout
     }
  return status;
}


///////////////////////////////////////////////////////////////////////
//                      C O D E   K E Y S  				
// 数值分组的数据,转换成符合规范的格式
///////////////////////////////////////////////////////////////////////
/*
char M500HostCodeValue(  unsigned char *value,	// 4 bytes  data value uncoded
						 unsigned char addr,    // 1 byte address value uncoded
                     	 unsigned char *coded)   // 16 bytes data value coded
{
   char  status = MI_OK;
   unsigned char  cnt = 0;
   unsigned char temp;
   
   memcpy(coded + cnt,value,4);
   cnt += 4;
   for (; cnt < 8; cnt++)
   {
      coded[cnt] = ~value[cnt - 4];
   }
   memcpy(coded + cnt,value,4);
   cnt += 4;
   *(coded + cnt) = addr;
   cnt ++;
   *(coded + cnt) = ~addr;
   cnt ++;
   *(coded + cnt) = addr;
   cnt ++;
   *(coded + cnt) = ~addr;
   cnt ++;

   return status;
}
*/
///////////////////////////////////////////////////////////////////////
//                V A L U E   M A N I P U L A T I O N 
// 控制单元->射频卡
// Command: 0xC1
// Len: 5
// Data[0]: _Adr 数值块的地址
// 射频卡->控制单元
// Len: 4Bit
// DATA[0]: 0x0A(ACK)
// 控制单元->射频卡
// Data[1]: _Value(LL)
// Data[2]: _Value(LH)
// Data[3]: _Value(HL)
// Data[4]: _Value(HH) 要增加的数值
// 射频卡->控制单元
// Len: 0
///////////////////////////////////////////////////////////////////////
char M500PiccValue(unsigned char dd_mode, 
                    unsigned char addr, 
                    unsigned char *value,
                    unsigned char trans_addr)
{
   char status = MI_OK;

   M500PcdSetTmo(1);    
   // ************* Cmd Sequence ********************************** 
   ResetInfo(MInfo);   
   MSndBuffer[0] = dd_mode;        // Inc,Dec,restroe command code 增值/减值/重储指令
   MSndBuffer[1] = addr;
   MInfo.nBytesToSend   = 2;
   status = M500PcdCmd(PCD_TRANSCEIVE,
                       MSndBuffer,
                       MRcvBuffer,
                       &MInfo);

   if (status != MI_NOTAGERR)   // no timeout error
   {
        if (MInfo.nBitsReceived != 4)   // 4 bits are necessary
        {
           status = MI_BITCOUNTERR;
        }
        else                     // 4 bit received
        {
           MRcvBuffer[0] &= 0x0f; // mask out upper nibble
           switch(MRcvBuffer[0])
           {
              case 0x00: 
                 status = MI_NOTAUTHERR;
                 break;
              case 0x0a:
                 status = MI_OK;
                 break;
              case 0x01:
                 status = MI_VALERR;
                 break;
              default:
                 status = MI_CODEERR;
                 break;
           }
        }
     }

     if ( status == MI_OK)
     {
        M500PcdSetTmo(3);     // long timeout 

        ResetInfo(MInfo);   
        memcpy(MSndBuffer,value,4);
        MInfo.nBytesToSend   = 4;
        status = M500PcdCmd(PCD_TRANSMIT,                     //此处源码为PCD_TRANCEIVE,但是此处根本没有返回值  wyy
                            MSndBuffer,
                            MRcvBuffer,
                            &MInfo);
        
      /* if (status == MI_OK)    // no timeout occured                 //没有返回值 因此此处不需要判断    by  wyy
        {
            if (MInfo.nBitsReceived != 4)   // 4 bits are necessary
            {
               status = MI_BITCOUNTERR;
            }
            else                     // 4 bit received
            {
               MRcvBuffer[0] &= 0x0f; // mask out upper nibble
               switch(MRcvBuffer[0])
               {
                  case 0x00: 
                     status = MI_NOTAUTHERR;
                     break;
                  case 0x01:
                     status = MI_VALERR;
                     break;
                  default:
                     status = MI_CODEERR;
                     break;
               }
            }
         }        
         else
         if (status == MI_NOTAGERR )
            status = MI_OK;  // no response after 4 byte value - 
                             // transfer command has to follow
       */                      
     }
     
     // 增值/减值/重储指令的操作结果均置于DATA寄存器,需要用Transfer指令(0xB0)
     // 将操作结果最终传送到E2PROM中
     if ( status == MI_OK)
     {
        ResetInfo(MInfo);   
        MSndBuffer[0] = PICC_TRANSFER;        // transfer command code
        MSndBuffer[1] = trans_addr;
        MInfo.nBytesToSend   = 2;
        status = M500PcdCmd(PCD_TRANSCEIVE,
                            MSndBuffer,
                            MRcvBuffer,
                            &MInfo);
        
        if (status != MI_NOTAGERR)    // timeout occured
        {
            if (MInfo.nBitsReceived != 4)   // 4 bits are necessary
            {
               status = MI_BITCOUNTERR;
            }
            else                     // 4 bit received
            {
               MRcvBuffer[0] &= 0x0f; // mask out upper nibble
               switch(MRcvBuffer[0])
               {
                  case 0x00: 
                     status = MI_NOTAUTHERR;
                     break;
                  case 0x0a:
                     status = MI_OK;
                     break;
                  case 0x01:
                     status = MI_VALERR;
                     break;
                  default:
                     status = MI_CODEERR;
                     break;
               }
            }
        }        
     }
   return status;
}



///////////////////////////////////////////////////////////////////////
//   V A L U E   M A N I P U L A T I O N   W I T H   B A C K U P
///////////////////////////////////////////////////////////////////////
char M500PiccValueDebit(unsigned char dd_mode,    
                         unsigned char addr,    
                         unsigned char *value)   
{   
   char status = MI_OK;   
      
   M500PcdSetTmo(1);    // short timeout    
   ResetInfo(MInfo);      
   MSndBuffer[0] = dd_mode;        // Inc,Dec command code    
   MSndBuffer[1] = addr;   
   MInfo.nBytesToSend   = 2;   
   status = M500PcdCmd(PCD_TRANSCEIVE,   
                       MSndBuffer,   
                       MRcvBuffer,   
                       &MInfo);   
   
   if (status != MI_NOTAGERR)   // no timeout error    
   {   
        if (MInfo.nBitsReceived != 4)   // 4 bits are necessary    
        {   
           status = MI_BITCOUNTERR;   
        }   
        else                     // 4 bit received    
        {   
           MRcvBuffer[0] &= 0x0f; // mask out upper nibble    
           switch(MRcvBuffer[0])   
           {   
              case 0x00:    
                 status = MI_NOTAUTHERR;   
                 break;   
              case 0x0A:   
                 status = MI_OK;   
                 break;   
              case 0x01:   
                 status = MI_VALERR;   
                 break;   
              default:   
                 status = MI_CODEERR;   
                 break;   
           }   
        }   
     }   
   
     if ( status == MI_OK)   
     {   
        M500PcdSetTmo(3);     // long timeout     
   
        ResetInfo(MInfo);      
        memcpy(MSndBuffer,value,4);   
        MInfo.nBytesToSend   = 4;   
        status = M500PcdCmd(PCD_TRANSMIT,                           //此处源码为PCD_TRANCEIVE,但是此处根本没有返回值  by wyy 
                            MSndBuffer,   
                            MRcvBuffer,   
                            &MInfo);   
           
       /*   
       if (status == MI_OK)    // no timeout occured                      //没有返回值 因此此处不需要判断    by  wyy
        {   
            if (MInfo.nBitsReceived != 4)   // 4 bits are necessary    
            {   
               status = MI_BITCOUNTERR;   
            }   
            else                     // 4 bit received    
            {   
               MRcvBuffer[0] &= 0x0f; // mask out upper nibble    
               switch(MRcvBuffer[0])   
               {   
                  case 0x00:    
                     status = MI_NOTAUTHERR;   
                     break;   
                  case 0x0a:   
                     status = MI_OK;   
                     break;   
                  case 0x05:   
                  case 0x01:   
                     status = MI_VALERR;   
                     break;   
                  default:   
                     status = MI_CODEERR;   
                     break;   
               }   
            }   
        }  
       */         
     }   
   
   return status;   
}   


///////////////////////////////////////////////////////////////////////
//          M I F A R E     H A L T
// 控制单元->射频卡
// Command: 0x50
// Len: 0
// 射频卡->控制单元
// Len: 0
// 将操作后的卡片置于halt 模式。如果又要对卡片操作，必须重新执行request 操作。
///////////////////////////////////////////////////////////////////////
char M500PiccHalt(void)
{
   char  status = MI_CODEERR;

   // ************* Cmd Sequence ********************************** 
   ResetInfo(MInfo);   
   MSndBuffer[0] = PICC_HALT ;     // Halt command code
   MSndBuffer[1] = 0x00;         // dummy address
   MInfo.nBytesToSend   = 2;
   status = M500PcdCmd(PCD_TRANSMIT,
                       MSndBuffer,
                       MRcvBuffer,
                       &MInfo);   
   if (status)
   {
     // timeout error ==> no NAK received ==> OK
     if (status == MI_NOTAGERR || status == MI_ACCESSTIMEOUT)
        status = MI_OK;
   }
   //reset command register - no response from tag
   WriteIO(RegCommand,PCD_IDLE);
   return status; 
}



//////////////////////////////////////////////////////////////////////
//                 M I F A R E    R E S E T 
///////////////////////////////////////////////////////////////////////
char M500PcdRfReset(unsigned char ms)
{
   char  status = MI_OK;

   if(ms)
   {
     ClearBitMask(RegTxControl,0x03);  	// 0x11---控制天线脚TX1 和TX2 的逻辑状态	Tx2RF-En, Tx1RF-En disablen
     //delay_1ms(ms);                		// Delay for 1 ms
     SetBitMask(RegTxControl,0x03);    	// Tx2RF-En, Tx1RF-En enable
   }
   else
     ClearBitMask(RegTxControl,0x03);  	// Tx2RF-En, Tx1RF-En disablen
     
   return status;
}



///////////////////////////////////////////////////////////////////////
//          E E P R O M   R E A D
// 从内部E2PROM 读出数据并将其放入FIFO 缓冲区
// 0x03	
// 输入:起始地址低字节
// 		起始地址高字节
// 		读出数据字节个数   
///////////////////////////////////////////////////////////////////////
char PcdReadE2(unsigned short startaddr,
               unsigned char length,
               unsigned char* _data)
{
   char status = MI_OK;

     // ************* Cmd Sequence ********************************** 
     ResetInfo(MInfo);   
     MSndBuffer[0] = startaddr & 0xFF;
     MSndBuffer[1] = (startaddr >> 8) & 0xFF;
     MSndBuffer[2] = length;
     MInfo.nBytesToSend   = 3;
     status = M500PcdCmd(PCD_READE2,
                         MSndBuffer,
                         MRcvBuffer,
                         &MInfo);
    if (status == MI_OK)
    {
       memcpy(_data,MRcvBuffer,length);
    }
    else   // Response Processing
    {
       _data[0] = 0;
    }
    return status ;
}


///////////////////////////////////////////////////////////////////////
//          E E P R O M   W R I T E 
// 从FIFO 缓冲区获得数据并写入内部E2PROM
// 0x01	
// 输入:起始地址低字节
// 		起始地址高字节
// 		读出数据字节个数   
///////////////////////////////////////////////////////////////////////
char PcdWriteE2(unsigned short startaddr,
                unsigned char length,
                unsigned char* _data)
{
   char status = MI_OK;

     // ************* Cmd Sequence ********************************** 
   ResetInfo(MInfo);   
   MSndBuffer[0] = startaddr & 0xFF;
   MSndBuffer[1] = (startaddr >> 8) & 0xFF;
   memcpy(MSndBuffer + 2,_data,length);

   MInfo.nBytesToSend   = length + 2;
         
   status = M500PcdCmd(PCD_WRITEE2,
                       MSndBuffer,
                       MRcvBuffer,
                       &MInfo); // write e2
   return status;
}  


///////////////////////////////////////////////////////////////////////
//          M I F A R E   R E M O T E   A N T E N N A
//  Configuration of slave module
///////////////////////////////////////////////////////////////////////
char M500PcdMfInOutSlaveConfig(void)
{
   char  status = MI_OK;

   FlushFIFO();    // empty FIFO
   ResetInfo(MInfo);   
   MSndBuffer[0] = 0x10; // addr low byte
   MSndBuffer[1] = 0x00; // addr high byte

   MSndBuffer[2] = 0x00; // Page
   MSndBuffer[3] = 0x7B; // RegTxControl modsource 11,InvTx2,Tx2RFEn,TX1RFEn
   MSndBuffer[4] = 0x3F; // RegCwConductance
   MSndBuffer[5] = 0x3F; // RFU13
   MSndBuffer[6] = 0x19; // RFU14
   MSndBuffer[7] = 0x13; // RegModWidth     
   MSndBuffer[8] = 0x00; // RFU16
   MSndBuffer[9] = 0x00; // RFU17
 
   MSndBuffer[10] = 0x00; // Page
   MSndBuffer[11] = 0x73; // RegRxControl1 
   MSndBuffer[12] = 0x08; // RegDecoderControl
   MSndBuffer[13] = 0x6c; // RegBitPhase     
   MSndBuffer[14] = 0xFF; // RegRxThreshold  
   MSndBuffer[15] = 0x00; // RFU1D
   MSndBuffer[16] = 0x00; // RegRxControl2   
   MSndBuffer[17] = 0x00; // RegClockQControl

   MSndBuffer[18] = 0x00; // Page
   MSndBuffer[19] = 0x06; // RegRxWait
   MSndBuffer[20] = 0x03; // RegChannelRedundancy
   MSndBuffer[21] = 0x63; // RegCRCPresetLSB    
   MSndBuffer[22] = 0x63; // RegCRCPresetMSB    
   MSndBuffer[23] = 0x0;  // RFU25
   MSndBuffer[24] = 0x04; // RegMfOutSelect enable mfout = manchester HT
   MSndBuffer[25] = 0x00; // RFU27
     
   // PAGE 5      FIFO, Timer and IRQ-Pin Configuration
   MSndBuffer[26] = 0x00; // Page
   MSndBuffer[27] = 0x08; // RegFIFOLevel       
   MSndBuffer[28] = 0x07; // RegTimerClock      
   MSndBuffer[29] = 0x06; // RegTimerControl    
   MSndBuffer[30] = 0x0A; // RegTimerReload     
   MSndBuffer[31] = 0x02; // RegIRqPinConfig    
   MSndBuffer[32] = 0x00; // RFU    
   MSndBuffer[33] = 0x00; // RFU
   MInfo.nBytesToSend   = 34;
         
   status = M500PcdCmd(PCD_WRITEE2,
                       MSndBuffer,
                       MRcvBuffer,
                       &MInfo); // write e2
   return status;
}


///////////////////////////////////////////////////////////////////////
//          E E P R O M   M A S T E R   K E Y   L O A D 
///////////////////////////////////////////////////////////////////////
char M500PcdLoadKeyE2(unsigned char key_type,
                          unsigned char sector,
                          unsigned char *uncoded_keys)
{
   char  status = MI_OK;
   // eeprom address calculation
   // 0x80 ... offset
   // key_sector ... sector
   // 0x18 ... 2 * 12 = 24 = 0x18
   unsigned short  e2addr = 0x80 + sector * 0x18;
   unsigned char  *e2addrbuf = (unsigned char*)&e2addr;
   unsigned char  keycoded[12];

   if (key_type == PICC_AUTHENT1B)
      e2addr += 12; // key B offset   
   
   FlushFIFO();    // empty FIFO
   ResetInfo(MInfo);

   M500HostCodeKey(uncoded_keys,keycoded);
   memcpy(MSndBuffer,e2addrbuf,2); // write low and high byte of address
   MSndBuffer[2] = MSndBuffer[0];    // Move the LSB of the 2-bytes
   MSndBuffer[0] = MSndBuffer[1];    // address to the first byte
   MSndBuffer[1] = MSndBuffer[2];
   memcpy(&MSndBuffer[2],keycoded,12); // write 12 bytes of coded keys
   MInfo.nBytesToSend   = 14;
   
   // write load command
   status = M500PcdCmd(PCD_WRITEE2,
			MSndBuffer,
			MRcvBuffer,
			&MInfo);         
   
   return status;
}


///////////////////////////////////////////////////////////////////////
//          M I F A R E   R E M O T E   A N T E N N A
//  Configuration of master module
///////////////////////////////////////////////////////////////////////
char M500PcdMfInOutMasterConfig(void)
{
   WriteIO(RegRxControl2,0x42);
   WriteIO(RegTxControl,0x10);
   WriteIO(RegBitPhase,0x11);

   return MI_OK;
}   


///////////////////////////////////////////////////////////////////////
//          C O N F I G   M F O U T   S E L E C T 
///////////////////////////////////////////////////////////////////////
char M500PcdMfOutSelect(unsigned char type)
{
   WriteIO(RegMfOutSelect,type&0x7);
   return MI_OK;
}
//----------------------------------------------------------------------------
//                                                                           
// FUNCTION:     start_timeout                                                   
//                                                                           
// IN:	         _50us                                                        
// OUT:	       	 -                                                           
//                                                                           
// COMMENT:	 Using OS to generate timeout with a resolution of 1ms.
//		 Timeout is calculated in the interrupt routine.                  
//                                                                           
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//                                                                           
// FUNCTION:     stop_timeout                                                   
//                                                                           
// IN:        	-                                                        
// OUT:       	-                                                           
//                                                                           
// COMMENT:  	Stop OS and clear timeout state                       
//                                                                           
//----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////
//                 T Y P E B   F U N C T I O N S
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//                    R E Q U E S T   B
//////////////////////////////////////////////////////////////////////
char M531PiccRequestB(unsigned char req_code, 
		      unsigned char AFI, 
		      unsigned char N, 
		      unsigned char *ATQB)
{
   char   status = MI_OK;
   unsigned char rec_len;

   WriteIO(RegChannelRedundancy,0x2C); // RxCRC and TxCRC enable, parity 
				       // disable, ISO/IEC3390 enable	
   ClearBitMask(RegControl,0x08);      // disable crypto 1 unit   
   SetBitMask(RegTxControl,0x03);      // Tx2RF-En, Tx1RF-En enable

   M500PcdSetTmo(5);

   MSndBuffer[0] = 0x05;     	       // APf code
   MSndBuffer[1] = AFI;                // 
   MSndBuffer[2] = (req_code&0x08)|(N&0x07);  // PARAM
 
   status = ExchangeByteStream(PCD_TRANSCEIVE,
                               MSndBuffer,
                               3,
                               ATQB,
                               &rec_len);

   if (status!=MI_OK && status!=MI_NOTAGERR) status = MI_COLLERR; // collision occurs
   
   if (status == MI_OK) M500PcdSetTmo(ATQB[11]>>4); // set FWT 
   	
   return status;
}                      

//////////////////////////////////////////////////////////////////////
//                    S L O T - M A R K E R
//////////////////////////////////////////////////////////////////////
char M531PiccSlotMarker(unsigned char N, 
		        unsigned char *ATQB)
{
   char   status = MI_OK;
   unsigned char rec_len;

   WriteIO(RegChannelRedundancy,0x2C); // RxCRC and TxCRC enable, parity 
				       // disable, ISO/IEC3390 enable	
   
   M500PcdSetTmo(5);

   if(!N || N>15) status=MI_WRONG_PARAMETER_VALUE;	
   else
   {
     MSndBuffer[0] = 0x05|(N<<4); // APn code
   
     status = ExchangeByteStream(PCD_TRANSCEIVE,
                                 MSndBuffer,
                                 1,
                                 ATQB,
                                 &rec_len);

     if (status == MI_CRCERR) status = MI_COLLERR; // collision occurs

     if (status == MI_OK) M500PcdSetTmo(ATQB[11]>>4); // set FWT 
   }
   return status;
}                      

//////////////////////////////////////////////////////////////////////
//                    A T T R I B
//////////////////////////////////////////////////////////////////////
char M531PiccAttrib(unsigned char *PUPI, 
		    unsigned char CID, 
		    unsigned char brTx, 
		    unsigned char brRx,
		    unsigned char PARAM3)
{
   char   status = MI_OK;
   unsigned char rec_len,dummy[4];

   WriteIO(RegChannelRedundancy,0x2C); // RxCRC and TxCRC enable, parity 
				       // disable, ISO/IEC3390 enable	

   MSndBuffer[0] = 0x1d;     	    // command
   memcpy(&MSndBuffer[1],PUPI,4);   // Identifier
   MSndBuffer[5] = 0x00;  	    // EOF/SOF required, default TR0/TR1
   MSndBuffer[6] = 0x07|((brTx&3)<<4)|((brRx&3)<<6); // Max frame 128 
   MSndBuffer[7] = PARAM3;  	    // Param3, ISO/IEC 14443-4 compliant?
   MSndBuffer[8] = CID&0x0f;  	    // CID
 
  status = ExchangeByteStream(PCD_TRANSCEIVE,
                              MSndBuffer,
                              9,
                              dummy,
                              &rec_len);

  
  
  
   return status;
} 


//////////////////////////////////////////////////////////////////////
//           E X C H A N G E   B Y T E   S T R E A M
///////////////////////////////////////////////////////////////////////
char ExchangeByteStream(unsigned char Cmd,
                        volatile unsigned char *send_data,
                        unsigned char send_bytelen,
                        unsigned char *rec_data,  
                       	unsigned char *rec_bytelen)
{
   char status = MI_OK;
   
   FlushFIFO();    // empty FIFO
   ResetInfo(MInfo); // initialise ISR Info structure

   if (send_bytelen > 0)
   {
      memcpy(MSndBuffer,send_data,send_bytelen); // write n bytes 
      MInfo.nBytesToSend = send_bytelen;
      // write load command
      status = M500PcdCmd(Cmd,
                      	  MSndBuffer,
                      	  MRcvBuffer,
                      	  &MInfo);
      if ( status == MI_OK )
      {
         *rec_bytelen = MInfo.nBytesReceived;
         if (*rec_bytelen)
         { 
            memcpy(rec_data,MRcvBuffer,MInfo.nBytesReceived);
         }
      }
   }
   else
   {
      status = MI_WRONG_PARAMETER_VALUE;
   }
   return status;
}  


void 	test_function(void)
{
  unsigned char counter,counter2;
  unsigned char tt1[2];
  unsigned char status1;
  unsigned char blockwrite[16]={0x96,0x00,0x00,0x00,0x69,0xff,0xff,0xff,0x96,0x00,0x00,0x00,0x05,0xfa,0x05,0xfa};
  unsigned char key[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
  unsigned char mfout=2;
  unsigned char cardserialno[7];
  unsigned char *sak1;
  unsigned char blockdata[16];
  unsigned char value_data[4]={0x03,0x00,0x00,0x00};
  int i;
  
  rc531_init();
  M500PcdConfig();  	// Initialise the RC500

  M500PcdMfOutSelect(mfout);

  while(1)//for (counter=0;counter<10;counter++)
  {
	  	
  

	  	
	  	
	  	status1 = M500PiccRequest(PICC_REQSTD, tt1);
	  	
	  	//status1 = M500PiccRequest(PICC_REQALL, tt1);
	  	
	  	if (status1==MI_OK)
		{
		       for(i=0; i<2; i++)
		    	   printf("0x%x ", tt1[i]);
		       printf("\n\n");
		      	 		       
		       status1=M500PiccAnticoll(PICC_ANTICOLL1, 0, cardserialno);

		       if(status1==MI_OK)
		       {
		           status1=M500PiccSelect(PICC_ANTICOLL1, cardserialno, sak1);
		         
		           for(i=0; i<4; i++)
		    	 	  printf("0x%x ", cardserialno[i]);
		           printf("\n\n");
		          
		           status1=M500PiccAnticoll(PICC_ANTICOLL2,0, cardserialno);
		           
		           //if(status1==MI_OK)
		           {
		           		status1=M500PiccSelect(PICC_ANTICOLL2, cardserialno, sak1);
	          	   		for(i=0; i<4; i++)
		    	 	 		printf("0x%x ", cardserialno[i]);
		          		printf("\n\n");
	          	   }
		       }
		       else
		       {
		          printf("Anticoll error\n");
		          
		          M500PcdConfig();  	// Initialise the RC500

                  M500PcdMfOutSelect(mfout);
		       }
	   	}
	   	else
	   	{
	   	      printf("Request error\n");
	   	      
	   	        M500PcdConfig();  	// Initialise the RC500

                M500PcdMfOutSelect(mfout);
	   	
	   	}
	   	
	  
	  //if (status1==MI_OK)
	      //status1=M500PiccSelect(cardserialno, sak1);
	     
	     
	   
	    	
	    status1= M500PiccHalt();	
	    
	    delay(1000);

   }
   /*	
  	if (status1==MI_OK)
	  	status1 = M500PiccAuth(PICC_AUTHENT1A, cardserialno, key, 7);
   
    if (status1 ==MI_OK)
      	status1=M500PiccRead(5, blockdata);
   
    if (status1 ==MI_OK)
	  	status1= M500PiccWrite(5,blockwrite);
       
    if (status1 ==MI_OK)
      	status1=M500PiccRead(5, blockdata);
      	
    if (status1 ==MI_OK)  
       status1 = M500PiccValue(PICC_INCREMENT, 5, value_data, 5);
    
    if (status1 ==MI_OK)

     status1=M500PiccRead(5, blockdata);
    
   if (status1 == MI_OK)   
     
     status1= M500PiccHalt();						// Halt操作
    
     	
  }
  
  while(1);*/
}
                                        
