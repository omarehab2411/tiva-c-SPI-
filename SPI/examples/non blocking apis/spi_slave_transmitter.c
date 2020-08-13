#include "TM4C123.h"                    // Device header
#include "spi.h"
#include <string.h>



SPI_handler spi;


char tx[]="123456789";

	void callback (void* spiHandler,char*ptr)
	{
	 gpio_write(GPIOF,pin2,gpio_pin_data_high);
	  SPI_Slave_Start_Transmit(&spi,tx,strlen(tx));	
	}

	
	int main()
{	
gpio_clk_enable(portF);
gpio_digital_enable(GPIOF,pin2);
gpio_set_direction(GPIOF,pin2,gpio_pin_dir_out);
	
spi.config.MasterSlaveMode=SPI_Mode_Slave;
spi.config.ClkSource=SPI_sysclk_Source;
spi.config.FrameFormat=SPI_Freescale_Foramt;
spi.config.ClkPolarityPhaseMode=SPI_ClkPolarityPhase_Mode_1;
spi.config.DataSize=SPI_DataSize_8Bits;
spi.config.PreScaler=128;
spi.SSIx=SSI1;
spi.PeripherlaNumber=SPI_1;
SPI_txDone_RegsiterCB(&spi,callback);
SPI_Init(&spi);
__enable_irq();
SPI_Slave_Start_Transmit(&spi,tx,strlen(tx));		
while(1)
{
	
 
  
}


}		




	