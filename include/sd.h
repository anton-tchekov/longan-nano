#ifndef __SD_H__
#define __SD_H__

#include "types.h"

u8 sd_init(void);
u8 sd_read(u8 *buffer, u32 block, u16 offset, u16 count);

#endif /* __SD_H__ */
