#include "TM4C123.h"                    // Device header
#include "spi.h"
#include <string.h>



SPI_handler spiHandler;


char rx[100];

	void callback (void* spiHandler,char*ptr)
	{
		if(ptr[0]=='1')
		{
	 gpio_write(GPIOF,pin2,gpio_pin_data_high);
		}
	}

	
	int main()
{	
gpio_clk_enable(portF);
gpio_digital_enable(GPIOF,pin2);
gpio_set_direction(GPIOF,pin2,gpio_pin_dir_out);
	
spiHandler.config.MasterSlaveMode=SPI_Mode_Slave;
spiHandler.config.ClkSource=SPI_sysclk_Source;
spiHandler.config.FrameFormat=SPI_Freescale_Foramt;
spiHandler.config.ClkPolarityPhaseMode=SPI_ClkPolarityPhase_Mode_1;
spiHandler.config.DataSize=SPI_DataSize_8Bits;
spiHandler.config.PreScaler=128;
spiHandler.SSIx=SSI1;
spiHandler.PeripherlaNumber=SPI_1;
SPI_rxDone_RegisterCB(&spiHandler,callback);
SPI_Init(&spiHandler);
__enable_irq();
	 
SPI_Slave_Start_Receive(&spiHandler,rx,100);	

while(1)
{
 	
}


}		


	