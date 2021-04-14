#ifndef __EXTRAM_H__
#define __EXTRAM_H__

#include "types.h"

void extram_init(void);
void *extram_read(u32 addr, void *data, u32 size);
void extram_write(u32 addr, void *data, u32 size);

#endif

