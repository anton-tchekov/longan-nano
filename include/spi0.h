#ifndef __SPI0_H__
#define __SPI0_H__

#include "types.h"

void spi0_init(void);
u8 spi0_rxtx(u8 data);

#endif

