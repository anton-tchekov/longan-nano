#include "spi0.h"
#include "gd32vf103_gpio.h"

void spi0_init(void)
{
	/* SPI0 GPIO: NSS/PA4, SCK/PA5, MOSI/PA7 */
	spi_parameter_struct spi_init_struct;
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
 	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_SPI0);
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
	spi_struct_para_init(&spi_init_struct);
	spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
	spi_init_struct.device_mode = SPI_MASTER;
	spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
	spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
	spi_init_struct.nss = SPI_NSS_SOFT;
	spi_init_struct.prescale = SPI_PSC_8;
	spi_init_struct.endian = SPI_ENDIAN_MSB;
	spi_init(SPI0, &spi_init_struct);
	spi_crc_polynomial_set(SPI0, 7);
	spi_enable(SPI0);
}

u8 spi0_rxtx(u8 data)
{
	while(spi_i2s_flag_get(SPI0, SPI_FLAG_TBE) == RESET) ;
	spi_i2s_data_transmit(SPI0, data);
	while(spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE) == RESET) ;
	return spi_i2s_data_receive(SPI0);
}

