#include "TM4C123.h"                    // Device header
#include "spi.h"
#include <string.h>



SPI_handler spi;


char tx[]="123456789";
char rx[8];
	
void callback (void* spiHandler,char*ptr)
{
   if(ptr[1]=='1')
	 {gpio_write(GPIOF,pin2,gpio_pin_data_high);
	 }
}

	
	int main()
{	
gpio_clk_enable(portF);
gpio_digital_enable(GPIOF,pin2);
gpio_set_direction(GPIOF,pin2,gpio_pin_dir_out);
	
spi.config.MasterSlaveMode=SPI_Mode_Master;
spi.config.ClkSource=SPI_sysclk_Source;
spi.config.FrameFormat=SPI_Freescale_Foramt;
spi.config.ClkPolarityPhaseMode=SPI_ClkPolarityPhase_Mode_1;
spi.config.DataSize=SPI_DataSize_8Bits;
spi.config.PreScaler=64;
spi.SSIx=SSI1;
spi.PeripherlaNumber=SPI_1;
SPI_rxDone_RegisterCB(&spi,callback);
SPI_Init(&spi);
__enable_irq();
SPI_Master_Start_Receive(&spi,rx,8);	
while(1)
{

  
}


}		




	