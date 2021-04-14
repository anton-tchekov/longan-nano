#ifndef __USART_H__
#define __USART_H__

#include "types.h"

void usart_init(u32 baud);
i8 usart_tx(u8 b);
i16 usart_rx(void);

#endif

