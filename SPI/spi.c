#include "spi.h"

//*******************************************************
//                   private data
//*******************************************************
static uint8_t interruptHandler[4]={7,34,57,58};
static SPI_handler *spihandler[4];
//*******************************************************

//*********************************************************************
//             Helper Functions Prototypes
//*********************************************************************
static void SPI_MasterSlave_Set(SSI0_Type* SSIx,uint8_t MasterSlaveMode);
static void SPI_FrameFormat_Set(SSI0_Type* SSIx,uint8_t FrameFormat);
static void SPI_ClockPolarityPhase_set(SSI0_Type* SSIx,uint8_t clockPolarityPhaseMode);
static void SPI_DataSize_set(SSI0_Type* SSIx,uint8_t datasize);
static void SPI_ClockSource_Set (SSI0_Type* SSIx,uint8_t source);
static void SPI_ClockDivider_Set (SSI0_Type* SSIx,uint8_t PrescalerValue);
static SPI_Bus_State SPI_Bustate_get(SSI0_Type* SSIx);
static SPI_Fifo_State SPI_TxFifoState_get(SSI0_Type* SSIx);
static SPI_Fifo_State SPI_RxFifoState_get(SSI0_Type* SSIx);
static void SPI_Gpio_configure (uint8_t spiNumber);
static uint8_t SPI_ClockDivider_Get(SSI0_Type* SSIx);
static uint8_t SPI_MasterSlave_Get(SSI0_Type* SSIx);

static uint8_t SPI_InterruptStatus(SSI0_Type* SSIx,uint8_t interrupt);
static uint8_t SPI_MaskedInterruptStatus(SSI0_Type* SSIx,uint8_t interrupt);
static void SPI_InterruptClear(SSI0_Type* SSIx,uint8_t interrupt);
static void SPI_InterruptMask(SSI0_Type* SSIx,uint8_t interrupt);
static void SPI_InterruptUnMask(SSI0_Type* SSIx,uint8_t interrupt);
static void spi_delay(void);
//*********************************************************************



//*****************************************************************
//                    Exposed APIS
//*****************************************************************

void SPI_Init(SPI_handler *spiHandler)
{
SPI_Clock_enable(spiHandler->PeripherlaNumber);       
SPI_Gpio_configure(spiHandler->PeripherlaNumber);     //check
SPI_Disable(spiHandler);                              
SPI_MasterSlave_Set(spiHandler->SSIx,spiHandler->config.MasterSlaveMode); //check
SPI_ClockSource_Set(spiHandler->SSIx,spiHandler->config.ClkSource);            
SPI_ClockDivider_Set(spiHandler->SSIx,spiHandler->config.PreScaler);       
SPI_FrameFormat_Set(spiHandler->SSIx,spiHandler->config.FrameFormat);
SPI_ClockPolarityPhase_set(spiHandler->SSIx,spiHandler->config.ClkPolarityPhaseMode);
SPI_DataSize_set(spiHandler->SSIx,spiHandler->config.DataSize);
spihandler[spiHandler->PeripherlaNumber]=spiHandler;
SPI_Enable(spiHandler);
}

void SPI_Master_Transmit (SPI_handler *spiHandler,char* pTxBuffer,uint8_t dataLength)
{
	for(int i=0;i<dataLength;i++)
	{
while((spiHandler->SSIx->SR & 2) == 0);       
spiHandler->SSIx->DR =pTxBuffer[i];                  
while(spiHandler->SSIx->SR & 0x10);	
	}
}

void SPI_Master_Receive  (SPI_handler *spiHandler,char* pRxBuffer,uint8_t dataLength)
{
for(int i=0;i<dataLength;i++)
	{
while(((spiHandler->SSIx->SR & (1<<0) ) == 0));       
 spiHandler->SSIx->DR =0xff;                 
while((spiHandler->SSIx->SR & (1<<2))==0);
pRxBuffer[i]=spiHandler->SSIx->DR;
spi_delay();		
	}
	while(((spiHandler->SSIx->SR & (1<<4) ) == 1));
}


void SPI_Slave_Transmit  (SPI_handler *spiHandler,char* pTxBuffer,uint8_t dataLength)
{
	for(int i=0;i<dataLength;i++)
	{
while((spiHandler->SSIx->SR & 2) == 0);       
spiHandler->SSIx->DR =pTxBuffer[i];                  
while(spiHandler->SSIx->SR & 0x10);	
	}
}

void SPI_Slave_Receive   (SPI_handler *spiHandler,char* pRxBuffer,uint8_t dataLength)
{
	for(int i=0;i<dataLength;i++)
	{
while((spiHandler->SSIx->SR & (1<<2))==0);
pRxBuffer[spiHandler->RxCounter]=spiHandler->SSIx->DR;
spiHandler->RxCounter++;
spi_delay();		
	}
	while(((spiHandler->SSIx->SR & (1<<4) ) == 1));
}


void MastersendFromInterrupt(SPI_handler *spiHandler)
{	
	if(SPI_InterruptStatus(spiHandler->SSIx,SPI_TxFifo_InterruptStatus))
	{
	uint8_t remainingBits=spiHandler->Tx_length-spiHandler->Txcounter;
	if(remainingBits!=0)
	{
	spiHandler->SSIx->DR=spiHandler->pTx[spiHandler->Txcounter];
	spiHandler->Txcounter++;
 }

else if(remainingBits==0)
	 { spiHandler->Txcounter=0;
	   SPI_InterruptMask(spiHandler->SSIx,SPI_TxFifo_bit);
		//add call back here
		 spiHandler->DataTransmitted_CallBack(&spiHandler,spiHandler->pTx);
	 }
 } 
}


void SPI_Master_Start_Trasnmit(SPI_handler *spiHandler,char* pTxBuffer,uint8_t dataLength)
{
	spiHandler->pTx=pTxBuffer;
	spiHandler->Tx_length=dataLength;
	spiHandler->Txcounter=0;
	SPI_Interrupt_Enable(spiHandler);
	if(dataLength<=FifoSize)
	{
	 for(int i=0;i<dataLength;i++)
		{
		spiHandler->SSIx->DR=pTxBuffer[i];
		spiHandler->Txcounter++;
		}
	}
	else if(dataLength>FifoSize)
	{
	for(int i=0;i<FifoSize;i++)
		{
		spiHandler->SSIx->DR=pTxBuffer[i];
		spiHandler->Txcounter++;
		}
	}
SPI_InterruptUnMask(spiHandler->SSIx,SPI_TxFifo_bit);
}



void MasterReceiveFromInterrupt(SPI_handler *spiHandler)
{  uint8_t remainingBits;
	 uint8_t bit2Rx;
	if( ( ((spiHandler->SSIx->RIS&(1<<2))>>2) |  ((spiHandler->SSIx->RIS&(1<<3))>>3) )  )
	{//tx fifo is all trasnmited or rx fifo is half full

 bit2Rx=spiHandler->Txcounter-spiHandler->RxCounter;
  
 	if(bit2Rx>=FifoSize)
	{
	for(int i=0;i<FifoSize/2;i++)
	{
	spiHandler->pRx[spiHandler->RxCounter]=spiHandler->SSIx->DR;
	spiHandler->RxCounter++;
	spi_delay();
	}
	}
	else if(bit2Rx<FifoSize && bit2Rx!=0)
	{
	for(int i=0;i<bit2Rx;i++)
	{
	spiHandler->pRx[spiHandler->RxCounter]=spiHandler->SSIx->DR;
	spiHandler->RxCounter++;
	spi_delay();
	}
	}
	
	remainingBits=spiHandler->Rx_length-spiHandler->RxCounter;
	if(remainingBits>FifoSize)
	{
		for(int i=0;i<FifoSize/2;i++)
		{
		spiHandler->SSIx->DR=0xff;
	  spiHandler->Txcounter++;
	  spi_delay();
		}
 }
else if(remainingBits<FifoSize && remainingBits!=0)
{
	for(int i=0;i<remainingBits;i++)
	{
	spiHandler->SSIx->DR=0xff;
	  spiHandler->Txcounter++;
	  spi_delay();
	}
}

else if(remainingBits==0)
	 { spiHandler->RxCounter=0;
		 spiHandler->Txcounter=0;
		 SPI_InterruptMask(spiHandler->SSIx,SPI_RxFifo_bit);
		 SPI_InterruptMask(spiHandler->SSIx,SPI_TxFifo_bit);
		//add call back here
		spiHandler->DataReceived_CallBack(&spiHandler,spiHandler->pRx);
	 }
 }
}



void SPI_Master_Start_Receive (SPI_handler *spiHandler,char* pRxBuffer,uint8_t dataLength)
{
  spiHandler->pRx=pRxBuffer;
	spiHandler->Rx_length=dataLength;
	spiHandler->RxCounter=0;
	spiHandler->Txcounter=0;
	for(int i=0;i<FifoSize/2;i++)
		{
		spiHandler->SSIx->DR=0xff;
		spiHandler->Txcounter++;
		spi_delay();
		}
SPI_InterruptUnMask(spiHandler->SSIx,SPI_RxFifo_bit);
SPI_InterruptUnMask(spiHandler->SSIx,SPI_TxFifo_bit);
SPI_Interrupt_Enable(spiHandler);
}




void SlaveTransmitFromInterrupt(SPI_handler *spiHandler)
{ 
	uint8_t remainingBits=spiHandler->Tx_length-spiHandler->Txcounter;
	if(remainingBits>=FifoSize)
	{
		for(int i=0;i<FifoSize;i++)
		{
	spiHandler->SSIx->DR=spiHandler->pTx[spiHandler->Txcounter];
	spiHandler->Txcounter++;
		}
 }
else if(remainingBits<FifoSize && remainingBits!=0)
{
for(int i=0;i<remainingBits;i++)
		{
	spiHandler->SSIx->DR=spiHandler->pTx[spiHandler->Txcounter];
	spiHandler->Txcounter++;
		}
}
else if(remainingBits==0)
	 { spiHandler->Txcounter=0;
	   SPI_InterruptMask(spiHandler->SSIx,SPI_TxFifo_bit);
		//add call back here
		spiHandler->DataTransmitted_CallBack(&spiHandler,spiHandler->pTx);
	 }
 }	 



void SPI_Slave_Start_Transmit (SPI_handler *spiHandler,char* pTxBuffer,uint8_t dataLength)
{ 
  spiHandler->pTx=pTxBuffer;
	spiHandler->Tx_length=dataLength;
	spiHandler->Txcounter=0;
	if(dataLength<=FifoSize)
	{
	 for(int i=0;i<dataLength;i++)
		{
		spiHandler->SSIx->DR=pTxBuffer[i];
		spiHandler->Txcounter++;
		}
	}
	else if(dataLength>FifoSize)
	{
	for(int i=0;i<FifoSize;i++)
		{
		spiHandler->SSIx->DR=pTxBuffer[i];
		spiHandler->Txcounter++;
		}
	}
SPI_Interrupt_Enable(spiHandler);
SPI_InterruptUnMask(spiHandler->SSIx,SPI_TxFifo_bit);
}


void SlaveReceiveFromInterrupt(SPI_handler *spiHandler)
{
 if(SPI_InterruptStatus(spiHandler->SSIx,SPI_RxFifo_InterruptStatus))
 {
	uint8_t bit2Rx;
	//rx fifo half full 
  for(int i=0;i<FifoSize/2;i++)
  {	
	spiHandler->pRx[spiHandler->RxCounter]=spiHandler->SSIx->DR;
	spiHandler->RxCounter++;
	}
	
bit2Rx=spiHandler->Rx_length-spiHandler->RxCounter;
	if(bit2Rx==0)
	{
	spiHandler->RxCounter=0;
	SPI_InterruptMask(spiHandler->SSIx,SPI_RxFifo_bit);
	spiHandler->DataReceived_CallBack(&spiHandler,spiHandler->pRx);
	}
}
}

void SPI_Slave_Start_Receive  (SPI_handler *spiHandler,char* pRxBuffer,uint8_t dataLength)
{
  spiHandler->pRx=pRxBuffer;
	spiHandler->Rx_length=dataLength;
	spiHandler->RxCounter=0;
	SPI_InterruptUnMask(spiHandler->SSIx,SPI_RxFifo_bit);
	SPI_Interrupt_Enable(spiHandler);
}


void SPI_Interrupt_Enable  (SPI_handler *spiHandler)
{
NVIC_EnableIRQ(interruptHandler[spiHandler->PeripherlaNumber]);
}

void SPI_Interrupt_Disable (SPI_handler *spiHandler)
{
NVIC_DisableIRQ(interruptHandler[spiHandler->PeripherlaNumber]);
}


void SPI_Enable(SPI_handler *spiHandler)
{
spiHandler->SSIx->CR1|=SPI_EnableDisable_Bit;
}

void SPI_Disable(SPI_handler *spiHandler)
{
spiHandler->SSIx->CR1&=~SPI_EnableDisable_Bit;
}


void SPI_txDone_RegsiterCB(SPI_handler *spiHandler ,void (*DataTransmitted_CallBack) (void* spihandler,char* ptr) )
{
spiHandler->DataTransmitted_CallBack=DataTransmitted_CallBack;
}
void SPI_rxDone_RegisterCB(SPI_handler *spiHandler ,void (*DataReceived_CallBack) (void* spihandler,char* ptr) )
{
spiHandler->DataReceived_CallBack=DataReceived_CallBack;
}


void SPI_txDone_UnRegsiterCB(SPI_handler *spiHandler  )
{
spiHandler->DataTransmitted_CallBack=( (void*)0);
}
void SPI_rxDone_UnRegisterCB(SPI_handler *spiHandler  )
{
spiHandler->DataReceived_CallBack=( (void*)0);
}



//***********************************************************************



//************************************************************
//                   Helper Functions
//************************************************************
static void SPI_MasterSlave_Set(SSI0_Type* SSIx,uint8_t MasterSlaveMode)
{
if(MasterSlaveMode==SPI_Mode_Master)
{
SSIx->CR1=0x00000000;
}
else if(MasterSlaveMode==SPI_Mode_Slave)
{
SSIx->CR1=0x00000004;
}
}

static uint8_t SPI_MasterSlave_Get(SSI0_Type* SSIx)
{
return (SSIx->CR1&SPI_MasterSlave_Bit);
}

static void SPI_FrameFormat_Set(SSI0_Type* SSIx,uint8_t FrameFormat)
{
switch(FrameFormat)
{
	case SPI_Freescale_Foramt:
		SSIx->CR0&=~SPI_Format_Bits;
	  SSIx->CR0|=SPI_Freescale_Foramt_set;
		break;
	
case SPI_TI_Foramt:
	  SSIx->CR0&=~SPI_Format_Bits;
	  SSIx->CR0|=SPI_TI_Foramt_set;
		break;
	
case SPI_MicroWire_Foramt:
   	SSIx->CR0&=~SPI_Format_Bits;
	  SSIx->CR0|=SPI_MicroWire_Foramt_set;
		break;
	
default:
	break;
}
}

static void SPI_ClockPolarityPhase_set(SSI0_Type* SSIx,uint8_t clockPolarityPhaseMode)
{
switch(clockPolarityPhaseMode)
{

	case SPI_ClkPolarityPhase_Mode_1:
		SSIx->CR0&=~(SPI_ClockPhase_Bit|SPI_ClockPolarity_Bit);
		break;
	
	
	case SPI_ClkPolarityPhase_Mode_2:
	SSIx->CR0&=~(SPI_ClockPhase_Bit|SPI_ClockPolarity_Bit);
	SSIx->CR0|=SPI_ClkPolarityPhase_Mode_2_set;
		break;
	
	case SPI_ClkPolarityPhase_Mode_3:
	SSIx->CR0&=~(SPI_ClockPhase_Bit|SPI_ClockPolarity_Bit);
	SSIx->CR0|=SPI_ClkPolarityPhase_Mode_3_set;
		break;
	
	case SPI_ClkPolarityPhase_Mode_4:
	SSIx->CR0&=~(SPI_ClockPhase_Bit|SPI_ClockPolarity_Bit);
	SSIx->CR0|=SPI_ClkPolarityPhase_Mode_4_set;
		break;
	
	default:
		break;
}
}
static void SPI_DataSize_set(SSI0_Type* SSIx,uint8_t datasize)
{
SSIx->CR0&=~(0x0000000F);
SSIx->CR0|=datasize;	
}

static void SPI_ClockSource_Set (SSI0_Type* SSIx,uint8_t source)
{
if(source==SPI_sysclk_Source)
{
SSIx->CC=SPI_sysclk_Source;
}
else if(source==SPI_Piosc_Source)
{
SSIx->CC=SPI_Piosc_Source;
}
}

static void SPI_ClockDivider_Set (SSI0_Type* SSIx,uint8_t PrescalerValue)
{
 if((PrescalerValue&0x1)==0)
 {
 if(PrescalerValue>=2 && PrescalerValue<=254)
 {
 SSIx->CPSR=PrescalerValue;
 }
else 
	{
  PrescalerValue=2;
	SSIx->CPSR=PrescalerValue;
	}		
}
 
 else if((PrescalerValue&0x1)==1)
 {
 	PrescalerValue+=1;
	SSIx->CPSR=PrescalerValue;
 }
}

static uint8_t SPI_ClockDivider_Get(SSI0_Type* SSIx)
{
return (SSIx->CPSR&(0xf));
}


	


static void SPI_Gpio_configure (uint8_t spiNumber)
{	
	switch(spiNumber)
	{
		case SPI_0:
			//gpio pins is set by default as spi
		  gpio_clk_enable(portA);	
		  gpio_digital_enable(GPIOA,pin2|pin3|pin4|pin5);
			break;
	
		case SPI_1:
			gpio_clk_enable(portD);
		  gpio_set_mode(GPIOD,pin0|pin3|pin1|pin2,gpio_pin_alternateMode);
      gpio_set_alternateFunction(GPIOD,0,2);
		  gpio_set_alternateFunction(GPIOD,1,2);
		  gpio_set_alternateFunction(GPIOD,2,2);
      gpio_set_alternateFunction(GPIOD,3,2);	
		  gpio_digital_enable(GPIOD,pin0|pin1|pin3|pin2);
			break;
	
		case SPI_2:
		gpio_clk_enable(portB);
		  gpio_set_mode(GPIOB,pin4|pin5|pin6|pin7,gpio_pin_alternateMode);
      gpio_set_alternateFunction(GPIOB,4,2);
		  gpio_set_alternateFunction(GPIOB,5,2);
		  gpio_set_alternateFunction(GPIOB,6,2);
      gpio_set_alternateFunction(GPIOB,7,2);		
		  gpio_digital_enable(GPIOB,pin4|pin5|pin3|pin6|pin7);
			break;
		
		case SPI_3:
			gpio_clk_enable(portD);
		  gpio_set_mode(GPIOD,pin0|pin3|pin1|pin2,gpio_pin_alternateMode);
      gpio_set_alternateFunction(GPIOD,0,1);
		  gpio_set_alternateFunction(GPIOD,1,1);
		  gpio_set_alternateFunction(GPIOD,2,1);
      gpio_set_alternateFunction(GPIOD,3,1);		
		  gpio_digital_enable(GPIOD,pin0|pin1|pin3|pin2);
			break;
		
		default:
			break;
	}
}

static SPI_Bus_State SPI_Bustate_get(SSI0_Type* SSIx)
{
 if((SSIx->SR&1)==0)
 {
 return Idle;
 }
 else if(SSIx->SR&1)
 {
 return Busy;
 }
}

static SPI_Fifo_State SPI_TxFifoState_get(SSI0_Type* SSIx)
{
if((SSIx->SR&(1<<1))==0)
{

return Full;
}
else if((SSIx->SR&(1<<1))==1)
{
if((SSIx->SR&(1<<0))==1)
{
return Empty;
}
else 
{return NotFullOrEmpty;
}
}
}


static SPI_Fifo_State SPI_RxFifoState_get(SSI0_Type* SSIx)
{
if((SSIx->SR&(1<<3))==1)
{

return Full;
}
else if((SSIx->SR&(1<<3))==0)
{
if((SSIx->SR&(1<<0))==0)
{
return Empty;
}
else 
{return NotFullOrEmpty;
}
}
}

static uint8_t SPI_InterruptStatus(SSI0_Type* SSIx,uint8_t interrupt)
{
return (SSIx->RIS&interrupt);
}
static void SPI_InterruptClear(SSI0_Type* SSIx,uint8_t interrupt)
{
SSIx->ICR&=~interrupt;
}

static void SPI_InterruptMask(SSI0_Type* SSIx,uint8_t interrupt)
{
SSIx->IM&=~interrupt;  /*mask interrupt*/
}

static void SPI_InterruptUnMask(SSI0_Type* SSIx,uint8_t interrupt)
{
SSIx->IM|=interrupt;  /*mask interrupt*/
}


static uint8_t SPI_MaskedInterruptStatus(SSI0_Type* SSIx,uint8_t interrupt)
{

return (SSIx->MIS&interrupt);
}



static void spi_delay(void)
{
for(int i=0;i<500;i++)
	{
	for(int i=0;i<100;i++)
		{}
	}
}
//***************************************************************************************************************************




//*************************************************************************************************
//                                     ISR
//************************************************************************************************* 

void SSI0_Handler (void)
{ 
if(spihandler[0]->config.MasterSlaveMode==SPI_Mode_Master)
{
	if(SPI_InterruptStatus(spihandler[0]->SSIx,SPI_TxFifo_InterruptStatus))
	{
    MastersendFromInterrupt(spihandler[0]);	
	}
	
	else if(SPI_InterruptStatus(spihandler[0]->SSIx,SPI_RxFifo_InterruptStatus))
	{
MasterReceiveFromInterrupt(spihandler[0]);	
	}
	
}

else if(spihandler[0]->config.MasterSlaveMode==SPI_Mode_Slave)
{
	if(SPI_InterruptStatus(spihandler[0]->SSIx,SPI_RxFifo_InterruptStatus))
	{
	SlaveReceiveFromInterrupt(spihandler[0]);
	}
	
	else if(SPI_InterruptStatus(spihandler[0]->SSIx,SPI_TxFifo_InterruptStatus) )
	{
	SlaveTransmitFromInterrupt(spihandler[0]);
	}

}

}






void SSI1_Handler (void)
{ 
if(spihandler[1]->config.MasterSlaveMode==SPI_Mode_Master)
{
	if(SPI_InterruptStatus(spihandler[1]->SSIx,SPI_TxFifo_InterruptStatus))
	{
    MastersendFromInterrupt(spihandler[1]);	
	}
	
	else if(SPI_InterruptStatus(spihandler[1]->SSIx,SPI_RxFifo_InterruptStatus))
	{
MasterReceiveFromInterrupt(spihandler[1]);	
	}
	
}

else if(spihandler[1]->config.MasterSlaveMode==SPI_Mode_Slave)
{
	if(SPI_InterruptStatus(spihandler[1]->SSIx,SPI_RxFifo_InterruptStatus))
	{
	SlaveReceiveFromInterrupt(spihandler[1]);
	}
	
	else if(SPI_InterruptStatus(spihandler[1]->SSIx,SPI_TxFifo_InterruptStatus) )
	{
	SlaveTransmitFromInterrupt(spihandler[1]);
	}

}

}






void SSI2_Handler (void)
{ 
if(spihandler[2]->config.MasterSlaveMode==SPI_Mode_Master)
{
	if(SPI_InterruptStatus(spihandler[2]->SSIx,SPI_TxFifo_InterruptStatus))
	{
    MastersendFromInterrupt(spihandler[2]);	
	}
	
	else if(SPI_InterruptStatus(spihandler[2]->SSIx,SPI_RxFifo_InterruptStatus))
	{
MasterReceiveFromInterrupt(spihandler[2]);	
	}
	
}

else if(spihandler[1]->config.MasterSlaveMode==SPI_Mode_Slave)
{
	if(SPI_InterruptStatus(spihandler[2]->SSIx,SPI_RxFifo_InterruptStatus))
	{
	SlaveReceiveFromInterrupt(spihandler[2]);
	}
	
	else if(SPI_InterruptStatus(spihandler[2]->SSIx,SPI_TxFifo_InterruptStatus) )
	{
	SlaveTransmitFromInterrupt(spihandler[2]);
	}

}

}







void SSI3_Handler (void)
{ 
if(spihandler[3]->config.MasterSlaveMode==SPI_Mode_Master)
{
	if(SPI_InterruptStatus(spihandler[3]->SSIx,SPI_TxFifo_InterruptStatus))
	{
    MastersendFromInterrupt(spihandler[3]);	
	}
	
	else if(SPI_InterruptStatus(spihandler[3]->SSIx,SPI_RxFifo_InterruptStatus))
	{
MasterReceiveFromInterrupt(spihandler[3]);	
	}
	
}

else if(spihandler[3]->config.MasterSlaveMode==SPI_Mode_Slave)
{
	if(SPI_InterruptStatus(spihandler[3]->SSIx,SPI_RxFifo_InterruptStatus))
	{
	SlaveReceiveFromInterrupt(spihandler[3]);
	}
	
	else if(SPI_InterruptStatus(spihandler[3]->SSIx,SPI_TxFifo_InterruptStatus) )
	{
	SlaveTransmitFromInterrupt(spihandler[3]);
	}

}

}







//***************************************************************************************************


