#ifndef __MBR_H
#define __MBR_H

#include "volume_container.h"

typedef struct st_partition_info {
	volume_container *volume;
	unsigned char status;
	unsigned char type;
	unsigned long start_sector;
	unsigned long sector_count;
} partition_info;

int volume_is_bootable(volume_container *v);
int mbr_partition_info(volume_container *v, int partition_number, partition_info *p);
int partition_info_is_fat(partition_info *p);

int partition_open(partition_info *p, volume_container *partition);
int partition_close(volume_container *v);

#endif /* #ifdef __MBR_H */

