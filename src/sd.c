#include "sd.h"
#include "systick.h"
#include "gd32vf103_gpio.h"

/* Set SCLK = PCLK2 / 64 */
#define FCLK_SLOW() SPI_CTL0(SPI1) = (SPI_CTL0(SPI1) & ~0x38) | 0x28

/* Set SCLK = PCLK2 / 2 */
#define FCLK_FAST() SPI_CTL0(SPI1) = (SPI_CTL0(SPI1) & ~0x38) | 0x00

#define DESELECT()  gpio_bit_set(GPIOB, GPIO_PIN_12);
#define SELECT()    gpio_bit_reset(GPIOB, GPIO_PIN_12);

#define CMD_GO_IDLE_STATE      0x00
#define CMD_SEND_OP_COND       0x01
#define CMD_SEND_IF_COND       0x08
#define CMD_SEND_CSD           0x09
#define CMD_SEND_CID           0x0A
#define CMD_SET_BLOCKLEN       0x10
#define CMD_READ_SINGLE_BLOCK  0x11
#define CMD_WRITE_SINGLE_BLOCK 0x18
#define CMD_SD_SEND_OP_COND    0x29
#define CMD_APP                0x37
#define CMD_READ_OCR           0x3A

#define IDLE_STATE             (1 << 0)
#define ILLEGAL_CMD            (1 << 2)

#define SD_1                   (1 << 0)
#define SD_2                   (1 << 1)
#define SD_HC                  (1 << 2)

static u8 _card_type;

static void spi1_init(void)
{
	/* SPI1_GPIO: SCK/PB13, MISO/PB14, MOSI/PB15, CS/PB12 */
	spi_parameter_struct spi_init_struct;
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_SPI1);
	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13 | GPIO_PIN_15);
	gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_14);
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
	DESELECT();
	spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
	spi_init_struct.device_mode = SPI_MASTER;
	spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
	spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
	spi_init_struct.nss = SPI_NSS_SOFT;
	spi_init_struct.prescale = SPI_PSC_32;
	spi_init_struct.endian = SPI_ENDIAN_MSB;
	spi_init(SPI1, &spi_init_struct);
	spi_crc_polynomial_set(SPI1, 7);
	spi_enable(SPI1);
}

static u8 spi1_rxtx(i8 data)
{
	while(spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET);
	spi_i2s_data_transmit(SPI1, data);
	while(spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET);
	return spi_i2s_data_receive(SPI1);
}

static u8 sd_command(u8 cmd, u32 arg)
{
	u8 i, response;
	spi1_rxtx(0xFF);
	spi1_rxtx(0x40 | cmd);
	spi1_rxtx((arg >> 24) & 0xFF);
	spi1_rxtx((arg >> 16) & 0xFF);
	spi1_rxtx((arg >> 8) & 0xFF);
	spi1_rxtx((arg >> 0) & 0xFF);
	switch(cmd)
	{
		case CMD_GO_IDLE_STATE:
			spi1_rxtx(0x95);
			break;

		case CMD_SEND_IF_COND:
			spi1_rxtx(0x87);
			break;

		default:
			spi1_rxtx(0xFF);
			break;
	}

	for(i = 0; i < 10 && ((response = spi1_rxtx(0xFF)) == 0xFF); ++i) ;
	return response;
}

u8 sd_init(void)
{
	u8 response;
	u16 i;
	u32 arg;

	spi1_init();
	DESELECT();
	FCLK_SLOW();
	_card_type = 0;
	for(i = 0; i < 10; ++i)
	{
		spi1_rxtx(0xFF);
	}

	SELECT();
	for(i = 0; ; ++i)
	{
		if(sd_command(CMD_GO_IDLE_STATE, 0) == IDLE_STATE)
		{
			break;
		}

		if(i == 0x1ff)
		{
			DESELECT();
			return 1;
		}
	}

	if((sd_command(CMD_SEND_IF_COND, 0x1AA) & ILLEGAL_CMD) == 0)
	{
		spi1_rxtx(0xFF);
		spi1_rxtx(0xFF);
		if(((spi1_rxtx(0xFF) & 0x01) == 0) ||
			(spi1_rxtx(0xFF) != 0xAA))
		{
			return 1;
		}

		_card_type |= SD_2;
	}
	else
	{
		sd_command(CMD_APP, 0);
		if((sd_command(CMD_SD_SEND_OP_COND, 0) & ILLEGAL_CMD) == 0)
		{
			_card_type |= SD_1;
		}
	}

	for(i = 0; ; ++i)
	{
		if(_card_type & (SD_1 | SD_2))
		{
			arg = 0;
			if(_card_type & SD_2)
			{
				arg = 0x40000000;
			}

			sd_command(CMD_APP, 0);
			response = sd_command(CMD_SD_SEND_OP_COND, arg);
		}
		else
		{
			response = sd_command(CMD_SEND_OP_COND, 0);
		}

		if((response & IDLE_STATE) == 0)
		{
			break;
		}

		if(i == 0x7FFF)
		{
			DESELECT();
			return 1;
		}
	}

	if(_card_type & SD_2)
	{
		if(sd_command(CMD_READ_OCR, 0))
		{
			DESELECT();
			return 1;
		}

		if(spi1_rxtx(0xFF) & 0x40)
		{
			_card_type |= SD_HC;
		}

		spi1_rxtx(0xFF);
		spi1_rxtx(0xFF);
		spi1_rxtx(0xFF);
	}

	if(sd_command(CMD_SET_BLOCKLEN, 512))
	{
		DESELECT();
		return 1;
	}

	DESELECT();
	FCLK_FAST();
	delay_ms(20);
	return 0;
}

u8 sd_read(u8 *buffer, u32 block, u16 offset, u16 count)
{
	u16 i;
	SELECT();
	if(sd_command(CMD_READ_SINGLE_BLOCK,
		_card_type & SD_HC ? block : (block >> 9)))
	{
		DESELECT();
		return 1;
	}

	for(i = 0; spi1_rxtx(0xFF) != 0xFE; ++i)
	{
		if(i == 0xFFFF)
		{
			DESELECT();
			return 1;
		}
	}

	for(i = 0; i < offset; ++i)
	{
		spi1_rxtx(0xFF);
	}

	for(; i < offset + count; ++i)
	{
		*buffer++ = spi1_rxtx(0xFF);
	}

	for(; i < 512; ++i)
	{
		spi1_rxtx(0xFF);
	}

	spi1_rxtx(0xFF);
	spi1_rxtx(0xFF);
	DESELECT();
	spi1_rxtx(0xFF);
	return 0;
}

