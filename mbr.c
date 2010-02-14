#include <stdio.h>

#include "volume_container.h"
#include "mbr.h"

int volume_is_bootable(volume_container *v) {
	unsigned char signature[2];

	if (v->bytes_per_sector < 0x200) {
		/* bootable sectors must be at least 512b in size */
		return 0;
	}

	if (v->read(v, 0x1fe, signature, 2) != 2) {
		perror("Error reading volume boot record");
		return 0;
	}

	return (signature[0] == 0x55 && signature[1] == 0xaa);
}

int mbr_partition_info(volume_container *v, int partition_number, partition_info *p) {
	unsigned char record_data[16];
	if (!volume_is_bootable(v)) {
		perror("Cannot fetch partition info - volume does not have an MBR");
		return -1;
	}
	if (v->read(v, 0x1be + partition_number * 16, record_data, 16) != 16) {
		perror("Error reading volume boot record");
		return -1;
	}
	p->volume = v;
	p->status = record_data[0x00];
	p->type = record_data[0x04];
	p->start_sector = record_data[0x08] | (record_data[0x09] << 8) | (record_data[0x0a] << 16) | (record_data[0x0b] << 24);
	p->sector_count = record_data[0x0c] | (record_data[0x0d] << 8) | (record_data[0x0e] << 16) | (record_data[0x0f] << 24);
	return 0;
}

int partition_info_is_fat(partition_info *p) {
	unsigned char t = p->type;
	return (p->status == 0x80 && (t == 0x01 || t == 0x04 || t == 0x05 || t == 0x06 || t == 0x0b || t == 0x0c || t == 0x0e));
}

static ssize_t partition_read(volume_container *partition, off64_t position, void *buf, size_t count) {
	volume_container *volume = partition->data.partition.parent;
	return volume->read(volume, position + partition->data.partition.data_offset, buf, count);
}
static ssize_t partition_write(volume_container *partition, off64_t position, void *buf, size_t count) {
	volume_container *volume = partition->data.partition.parent;
	return volume->write(volume, position + partition->data.partition.data_offset, buf, count);
}

int partition_open(partition_info *p, volume_container *partition) {
	partition->read = &partition_read;
	partition->write = &partition_write;
	partition->bytes_per_sector = p->volume->bytes_per_sector;
	partition->data.partition.parent = p->volume;
	partition->data.partition.data_offset = p->start_sector * p->volume->bytes_per_sector;
	return 0;
}
int partition_close(volume_container *partition) {
	return 0;
}

