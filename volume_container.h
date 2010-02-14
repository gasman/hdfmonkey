#ifndef __VOLUME_CONTAINER_H
#define __VOLUME_CONTAINER_H

#include <unistd.h> /* for ssize_t */

/* An abstract representation of something acting as a disk; a resource with
data chunks that can be read/written. */

typedef struct st_volume_container {
	ssize_t (*read) (struct st_volume_container *v, off_t position, void *buf, size_t count);
	ssize_t (*write) (struct st_volume_container *v, off_t position, void *buf, size_t count);
	int (*close) (struct st_volume_container *v);
	unsigned int bytes_per_sector;
	unsigned long sector_count;
	union {
		struct st_volume_container_file {
			int fd;
			off_t data_offset;
		} file;
		struct st_volume_container_partition {
			struct st_volume_container *parent;
			off_t data_offset;
		} partition;
	} data;
} volume_container;

#endif /* #ifdef __VOLUME_CONTAINER_H */

