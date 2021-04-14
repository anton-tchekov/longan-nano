#include <string.h>
#include "sd.h"
#include "fat.h"

#define BPB_SECTORS_PER_CLUSTER   13
#define BPB_RESERVED_SECTOR_COUNT 14
#define PBP_NUM_FATS              16
#define PBP_ROOT_ENTRY_COUNT      17
#define BPB_FAT_SIZE_16           22
#define BPB_FAT_SIZE_32           36
#define BPB_TOTAL_SECTORS_16      19
#define BPB_TOTAL_SECTORS_32      32
#define BPB_ROOT_CLUSTER          44
#define BS_FILESYSTEM_TYPE        54
#define BS_FILESYSTEM_TYPE_32     82
#define MBR_TABLE                446
#define DIR_NAME                   0
#define DIR_ATTR                  11
#define DIR_CLUSTER_HI            20
#define DIR_CLUSTER_LO            26
#define DIR_FILESIZE              28
#define AM_VOL                      0x08
#define AM_DIR                      0x10
#define AM_MASK                     0x3F

static struct
{
	u8 csize;
	u16 n_rootdir;
	u32 n_fatent;
	u32 fatbase;
	u32 dirbase;
	u32 database;
	u32 org_clust;
	u32 curr_clust;
	u32 dsect;
} _fs;

u32 fat_fsize;
u32 fat_ftell;

static u32 ld_u32(const u8 *p)
{
	return ((u32)p[0]) | (((u32)p[1]) << 8) |
		(((u32)p[2]) << 16) | (((u32)p[3]) << 24);
}

static u16 ld_u16(const u8 *p)
{
	return ((u16)p[0]) | ((u16)(p[1]) << 8);
}

static u32 get_fat(u32 cluster)
{
	u8 buf[4];
	if(cluster < 2 || cluster >= _fs.n_fatent)
	{
		return 1;
	}

	if(sd_read(buf, _fs.fatbase + cluster / 128,
		((u16)cluster % 128) * 4, 4))
	{
		return 1;
	}

	return ld_u32(buf) & 0x0FFFFFFF;
}

static u32 clust2sect(u32 cluster)
{
	cluster -= 2;
	if(cluster >= (_fs.n_fatent - 2))
	{
		return 0;
	}

	return cluster * _fs.csize + _fs.database;
}

static u32 get_cluster(u8 *dir)
{
	u32 cluster;
	cluster = ld_u16(dir + DIR_CLUSTER_HI);
	cluster <<= 16;
	cluster |= ld_u16(dir + DIR_CLUSTER_LO);
	return cluster;
}

static u8 create_name(dir_t *dj, const char **path)
{
	u8 c, ni, si, i, *sfn;
	const char *p;
	sfn = dj->fn;
	memset(sfn, ' ', 11);
	si = 0;
	i = 0;
	ni = 8;
	p = *path;
	for(;;)
	{
		c = p[si++];
		if(c <= ' ' || c == '/')
		{
			break;
		}

		if (c == '.' || i >= ni)
		{
			if(ni != 8 || c != '.')
			{
				break;
			}

			i = 8;
			ni = 11;
			continue;
		}

		/* Convert character to uppercase */
		if(c >= 'a' && c <= 'z')
		{
			c -= 'a' - 'A';
		}

		sfn[i++] = c;
	}

	*path = &p[si];
	sfn[11] = (c <= ' ');
	return 0;
}

static u8 dir_rewind(dir_t *dj)
{
	u32 cluster;
	dj->index = 0;
	cluster = dj->sclust;
	if(cluster == 1 || cluster >= _fs.n_fatent)
	{
		return 1;
	}

	if(!cluster)
	{
		cluster = (u32)_fs.dirbase;
	}

	dj->clust = cluster;
	dj->sect = clust2sect(cluster);
	return 0;
}

static u8 dir_next(dir_t *dj)
{
	u32 clst;
	u16 i;
	if(!(i = dj->index + 1) || !dj->sect)
	{
		return 1;
	}

	if(!(i % 16))
	{
		dj->sect++;
		if(dj->clust == 0)
		{
			if(i >= _fs.n_rootdir)
			{
				return 1;
			}
		}
		else
		{
			if(((i / 16) & (_fs.csize - 1)) == 0)
			{
				clst = get_fat(dj->clust);
				if(clst <= 1)
				{
					return 1;
				}

				if(clst >= _fs.n_fatent)
				{
					return 1;
				}

				dj->clust = clst;
				dj->sect = clust2sect(clst);
			}
		}
	}

	dj->index = i;
	return 0;
}

static u8 dir_find(dir_t *dj, u8 *dir)
{
	u8 res;
	if((res = dir_rewind(dj)))
	{
		return res;
	}

	do
	{
		if((res = sd_read(dir, dj->sect, (dj->index % 16) * 32, 32)))
		{
			break;
		}

		if(dir[DIR_NAME] == 0)
		{
			res = 1;
			break;
		}

		if(!(dir[DIR_ATTR] & AM_VOL) && !memcmp(dir, dj->fn, 11))
		{
			break;
		}

		res = dir_next(dj);
	} while(!res);
	return res;
}

static u8 follow_path(dir_t *dj, u8 *dir, const char *path)
{
	u8 res;
	while(*path == ' ')
	{
		path++;
	}

	if(*path == '/')
	{
		path++;
	}

	dj->sclust = 0;
	if((u8)*path < ' ')
	{
		res = dir_rewind(dj);
		dir[0] = 0;
	}
	else
	{
		for(;;)
		{
			if((res = create_name(dj, &path)))
			{
				break;
			}

			if((res = dir_find(dj, dir)))
			{
				break;
			}

			if(dj->fn[11])
			{
				break;
			}

			if(!(dir[DIR_ATTR] & AM_DIR))
			{
				res = 1;
				break;
			}

			dj->sclust = get_cluster(dir);
		}
	}

	return res;
}

static u8 check_fs(u8 *buf, u32 sect)
{
	if(sd_read(buf, sect, 510, 2))
	{
		return 3;
	}

	if(ld_u16(buf) != 0xAA55)
	{
		return 2;
	}

	if(!sd_read(buf, sect, BS_FILESYSTEM_TYPE_32, 2)
		&& ld_u16(buf) == 0x4146)
	{
		return 0;
	}

	return 1;
}

u8 fat_mount(void)
{
	u8 fmt, buf[36];
	u32 bsect, fsize, tsect, mclst;
	bsect = 0;
	if((fmt = check_fs(buf, bsect)))
	{
		if(!sd_read(buf, bsect, MBR_TABLE, 16))
		{
			if(buf[4])
			{
				bsect = ld_u32(&buf[8]);
				fmt = check_fs(buf, bsect);
			}
		}
	}

	if(fmt)
	{
		return 1;
	}

	if(sd_read(buf, bsect, 13, sizeof(buf)))
	{
		return 1;
	}

	if(!(fsize = ld_u16(buf + BPB_FAT_SIZE_16 - 13)))
	{
		fsize = ld_u32(buf + BPB_FAT_SIZE_32 - 13);
	}

	fsize *= buf[PBP_NUM_FATS - 13];
	_fs.fatbase = bsect + ld_u16(buf + BPB_RESERVED_SECTOR_COUNT - 13);
	_fs.csize = buf[BPB_SECTORS_PER_CLUSTER - 13];
	_fs.n_rootdir = ld_u16(buf + PBP_ROOT_ENTRY_COUNT - 13);
	if(!(tsect = ld_u16(buf + BPB_TOTAL_SECTORS_16 - 13)))
	{
		tsect = ld_u32(buf + BPB_TOTAL_SECTORS_32 - 13);
	}

	mclst = (tsect - ld_u16(buf + BPB_RESERVED_SECTOR_COUNT - 13) -
		fsize - _fs.n_rootdir / 16) / _fs.csize + 2;

	_fs.n_fatent = (u32)mclst;
	if(!(mclst >= 0xFFF7))
	{
		return 1;
	}

	_fs.dirbase = ld_u32(buf + (BPB_ROOT_CLUSTER - 13));
	_fs.database = _fs.fatbase + fsize + _fs.n_rootdir / 16;
	return 0;
}

u8 fat_fopen(const char *path)
{
	dir_t dj;
	u8 sp[12], dir[32];
	dj.fn = sp;
	if(follow_path(&dj, dir, path))
	{
		return 1;
	}

	if(!dir[0] || (dir[DIR_ATTR] & AM_DIR))
	{
		return 1;
	}

	_fs.org_clust = get_cluster(dir);
	fat_fsize = ld_u32(dir + DIR_FILESIZE);
	fat_ftell = 0;
	return 0;
}

u8 fat_fread(void *buf, u16 btr, u16 *br)
{
	u32 clst, sect, remain;
	u16 rcnt;
	u8 cs, *rbuf;
	rbuf = buf;
	*br = 0;
	remain = fat_fsize - fat_ftell;
	if(btr > remain)
	{
		btr = (u16)remain;
	}

	while(btr)
	{
		if((fat_ftell % 512) == 0)
		{
			if(!(cs = (u8)(fat_ftell / 512 & (_fs.csize - 1))))
			{
				if((clst = fat_ftell ?
					get_fat(_fs.curr_clust) : _fs.org_clust) <= 1)
				{
					return 1;
				}

				_fs.curr_clust = clst;
			}

			if(!(sect = clust2sect(_fs.curr_clust)))
			{
				return 1;
			}

			_fs.dsect = sect + cs;
		}

		if((rcnt = 512 - (u16)fat_ftell % 512) > btr)
		{
			rcnt = btr;
		}

		if(sd_read(rbuf, _fs.dsect, (u16)fat_ftell % 512, rcnt))
		{
			return 1;
		}

		fat_ftell += rcnt;
		btr -= rcnt;
		*br += rcnt;
		if(rbuf)
		{
			rbuf += rcnt;
		}
	}

	return 0;
}

u8 fat_fseek(u32 offset)
{
	u32 clst, bcs, sect, ifptr;
	if(offset > fat_fsize)
	{
		offset = fat_fsize;
	}

	ifptr = fat_ftell;
	fat_ftell = 0;
	if(offset > 0)
	{
		bcs = (u32)_fs.csize * 512;
		if(ifptr > 0 && (offset - 1) / bcs >= (ifptr - 1) / bcs)
		{
			fat_ftell = (ifptr - 1) & ~(bcs - 1);
			offset -= fat_ftell;
			clst = _fs.curr_clust;
		}
		else
		{
			clst = _fs.org_clust;
			_fs.curr_clust = clst;
		}

		while(offset > bcs)
		{
			clst = get_fat(clst);
			if(clst <= 1 || clst >= _fs.n_fatent)
			{
				return 1;
			}

			_fs.curr_clust = clst;
			fat_ftell += bcs;
			offset -= bcs;
		}

		fat_ftell += offset;
		if(!(sect = clust2sect(clst)))
		{
			return 1;
		}

		_fs.dsect = sect + (fat_ftell / 512 & (_fs.csize - 1));
	}

	return 0;
}

/* Directory */
static void get_fileinfo(dir_t *dj, u8 *dir, direntry_t *fno)
{
	u8 i, c;
	char *p;
	p = fno->name;
	if(dj->sect)
	{
		for(i = 0; i < 8; i++)
		{
			c = dir[i];
			if(c == ' ')
			{
				break;
			}

			if(c == 0x05)
			{
				c = 0xE5;
			}

			*p++ = c;
		}

		if(dir[8] != ' ')
		{
			*p++ = '.';
			for(i = 8; i < 11; i++)
			{
				c = dir[i];
				if (c == ' ')
				{
					break;
				}

				*p++ = c;
			}
		}

		fno->type = dir[DIR_ATTR] & AM_DIR;
		fno->size = ld_u32(dir + DIR_FILESIZE);
	}

	*p = 0;
}

static u8 dir_read(dir_t *dj, u8 *dir)
{
	u8 res, a, c;
	res = 2;
	while(dj->sect)
	{
		if((res = sd_read(dir, dj->sect, (dj->index % 16) * 32, 32)))
		{
			break;
		}

		c = dir[DIR_NAME];
		if(!c)
		{
			res = 2;
			break;
		}

		a = dir[DIR_ATTR] & AM_MASK;
		if(c != 0xE5 && c != '.' && !(a & AM_VOL))
		{
			break;
		}

		if((res = dir_next(dj)))
		{
			break;
		}
	}

	if(res)
	{
		dj->sect = 0;
	}

	return res;
}

u8 fat_opendir(dir_t *dj, const char *path)
{
	u8 res;
	u8 sp[12], dir[32];
	dj->fn = sp;
	if(!(res = follow_path(dj, dir, path)))
	{
		if(dir[0])
		{
			if(dir[DIR_ATTR] & AM_DIR)
			{
				dj->sclust = get_cluster(dir);
			}
			else
			{
				res = 2;
			}
		}

		if(!res)
		{
			res = dir_rewind(dj);
		}
	}

	return res;
}

u8 fat_readdir(dir_t *dj, direntry_t *fno)
{
	u8 res;
	u8 sp[12], dir[32];
	dj->fn = sp;
	if(!fno)
	{
		res = dir_rewind(dj);
	}
	else
	{
		if((res = dir_read(dj, dir)) == 2)
		{
			res = 0;
		}

		if(!res)
		{
			get_fileinfo(dj, dir, fno);
			if((res = dir_next(dj)) == 2)
			{
				res = 0;
			}
		}
	}

	return res;
}
