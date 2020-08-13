#include "TM4C123.h"                    // Device header
#include "spi.h"
#include <string.h>



SPI_handler spi;



char rx[8];
	


	
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
spi.config.PreScaler=64;
spi.SSIx=SSI1;
spi.PeripherlaNumber=SPI_1;
//SPI_rxDone_RegisterCB(&spi,callback);
SPI_Init(&spi);
//__enable_irq();	
SPI_Slave_Receive(&spi,rx,8);
while(1)
{
if(rx[1]=='1')
{
gpio_write(GPIOF,pin2,gpio_pin_data_high);
}
  
}


}		




	