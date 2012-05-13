#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "volume_container.h"
#include "image_file.h"

static ssize_t image_file_read(volume_container *v, off_t position, void *buf, size_t count) {
	int fd = v->data.file.fd;
	int done = 0;
	int res;
	
	if (res = lseek(fd, position + v->data.file.data_offset, SEEK_SET) < 0) {
		return res;
	}

	while (count > 0) {
		res = read(fd, (void *) &(((char *) buf)[done]), count);
		if (res <= 0) {	// 0 indicates EOF, and it should never happen here.
			fprintf(stderr,"read() error. line: %d\n",__LINE__);
			return -1;
		} else {
			done += res;
			count -= res;
		}
	}
	return done;

}

static ssize_t image_file_write(volume_container *v, off_t position, void *buf, size_t count) {
	int fd = v->data.file.fd;
	int done = 0;
	int res;
	
	if (res = lseek(fd, position + v->data.file.data_offset, SEEK_SET) < 0) {
		return res;
	}

	while (count > 0) {
		res = write(fd, buf + done, count);
		if (res < 0) {
			perror("write() error");
			return -1;
		} else {
			done += res;
			count -= res;
		}
	}
	return done;	

}

static int image_file_close(volume_container *v) {
	close(v->data.file.fd);
	return 0;
}

int raw_image_open(volume_container *v, char *pathname, int writeable) {
	int fd;
	struct stat file_stat;

	if (writeable) {
		if ( (fd = open(pathname, O_RDWR | O_SYNC)) == -1 ) {
			perror("open() (RDWR) error");
			return -1;
		}
	} else {
		if ( (fd = open(pathname, O_RDONLY)) == -1 ) {
			perror("open() (RDONLY) error");
			return -1;
		}
	}
	
	if ( fstat(fd, &file_stat) == -1 ) {
		perror("fstat() error");
		return -1;
	}

	v->data.file.fd = fd;
	v->data.file.data_offset = 0;
	v->bytes_per_sector = 512;
	v->sector_count = file_stat.st_size / 512;
	v->read = &image_file_read;
	v->write = &image_file_write;
	v->close = &image_file_close;
	return 0;
}

int raw_image_create(volume_container *v, char *pathname, unsigned long sector_count) {
	int fd;
	
	if ( (fd = open(pathname,
			O_RDWR | O_CREAT | O_TRUNC | O_SYNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1 ) {
		perror("open() (RDWR) error");
		return -1;
	}
	if ( ftruncate(fd, sector_count * 512) == -1 ) {
		perror("ftruncate() error");
		return -1;
	}
	
	v->data.file.fd = fd;
	v->data.file.data_offset = 0;
	v->bytes_per_sector = 512;
	v->sector_count = sector_count;
	v->read = &image_file_read;
	v->write = &image_file_write;
	v->close = &image_file_close;
	return 0;
}

static char *hdf_signature = "RS-IDE\x1a";
#define HDF_SIGNATURE_LENGTH 7

int hdf_image_open(volume_container *v, char *pathname, int writeable) {
	int fd;
	struct stat file_stat;
	unsigned char hdf_header[11];

	if (writeable) {
		if ( (fd = open(pathname, O_RDWR | O_SYNC)) == -1 ) {
			perror("open() (RDWR) error");
			return -1;
		}
	} else {
		if ( (fd = open(pathname, O_RDONLY)) == -1 ) {
			perror("open() (RDONLY) error");
			return -1;
		}
	}

	if ( fstat(fd, &file_stat) == -1 ) {
		perror("fstat() error");
		return -1;
	}
	
	if (read(fd, hdf_header, 11) != 11) {
		close(fd);
		perror("Error reading HDF header");
		return -1;
	}

	v->data.file.fd = fd;
	v->data.file.data_offset = hdf_header[0x09] | (hdf_header[0x0a] << 8);
	if (hdf_header[0x08] & 0x01) {
		v->bytes_per_sector = 256;
	} else {
		v->bytes_per_sector = 512;
	}
	v->sector_count = (file_stat.st_size - v->data.file.data_offset) / v->bytes_per_sector;
	v->read = &image_file_read;
	v->write = &image_file_write;
	v->close = &image_file_close;
	return 0;
}

const char *HDF_PREAMBLE = "RS-IDE\x1a\x11\x00\x16\x02"; /* version 1.1, sector data not halved, data offset 0x216 */
const size_t HDF_PREAMBLE_LENGTH = 11;
const size_t HDF_HEADER_SIZE = 0x0216;

/* "Created by hdfmonkey", byte-reversed because HDF is little-endian while ATA format is implicitly big-endian */
static const char *MODEL_NUMBER = "rCaeet dybh fdomknye                    ";
static const size_t MODEL_NUMBER_LENGTH = 40;

int hdf_write_header(int fd, unsigned long sector_count) {
	char *header, *identity;
	unsigned long head_count, cyl_count, sectors_per_head, sectors_per_track;
	int res;
	int written;
	
	header = calloc( 1, HDF_HEADER_SIZE );
	
	memcpy( header, HDF_PREAMBLE, HDF_PREAMBLE_LENGTH );
	
	identity = header + 0x16;
	if ( sector_count >= 16383 * 16 * 63 ) {
		/* image > 8GB; use dummy 'large disk' CHS values */
		cyl_count = 16383;
		head_count = 16;
		sectors_per_track = 63;
	} else {
		/* find largest factor <= 16 to use as the head count */
		for( head_count = 16; head_count > 1; head_count-- ) {
			if( sector_count % head_count == 0 ) break;
		}
		sectors_per_head = sector_count / head_count;
		/* find largest factor <= 63 to use as sectors_per_track */
		for( sectors_per_track = 63; sectors_per_track > 1; sectors_per_track-- ) {
			if( sectors_per_head % sectors_per_track == 0 ) break;
		}
		cyl_count = sectors_per_head / sectors_per_track;
		if( cyl_count > 16384 ) {
			/* attempt to factorise into sensible CHS values failed dismally;
			fall back on dummy 'large disk' values */
			cyl_count = 16383;
			head_count = 16;
			sectors_per_track = 63;
		}
	}
	
	/* word 1: cylinder count */
	identity[2] = cyl_count & 0xff;
	identity[3] = cyl_count >> 8;
	/* word 3: head count */
	identity[6] = head_count;
	/* word 6: sectors per track */
	identity[12] = sectors_per_track;
	
	/* words 27-46: model number */
	memcpy( identity + 54, MODEL_NUMBER, MODEL_NUMBER_LENGTH );  
	
	/* word 49: Capabilities (bit 9 = 'LBA supported' flag) */
	identity[99] = 0x02;
	
	/* words 60-61: total number of sectors */
	identity[120] = sector_count & 0xff;
	identity[121] = (sector_count >> 8) & 0xff;
	identity[122] = (sector_count >> 16) & 0xff;
	identity[123] = (sector_count >> 24) & 0xff;
	
	written = 0;
	while (written < HDF_HEADER_SIZE) {
		res = write(fd, header + written, HDF_HEADER_SIZE - written);
		if (res < 0) {
			free(header);
			perror("write() error");
			return -1;
		} else {
			written += res;
		}
	}

	free(header);
	return 0;
}

int hdf_image_create(volume_container *v, char *pathname, unsigned long sector_count) {
	int fd;
	
	if ( (fd = open(pathname,
			O_RDWR | O_CREAT | O_TRUNC | O_SYNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1 ) {
		perror("open() (RDWR) error");
		return -1;
	}
	if ( ftruncate(fd, sector_count * 512 + HDF_HEADER_SIZE) == -1 ) {
		perror("ftruncate() error");
		return -1;
	}
	
	if ( hdf_write_header(fd, sector_count) == -1 ) {
		return -1;
	}
	
	v->data.file.fd = fd;
	v->data.file.data_offset = HDF_HEADER_SIZE;
	v->bytes_per_sector = 512;
	v->sector_count = sector_count;
	v->read = &image_file_read;
	v->write = &image_file_write;
	v->close = &image_file_close;
	return 0;
}

int image_file_is_hdf(char *pathname) {
	int fd;
	char actual_signature[HDF_SIGNATURE_LENGTH];
	
	if ( (fd = open(pathname, O_RDONLY)) == -1 ) {
		perror("open() (RDONLY) error");
		return 0;
	}
	if (read(fd, actual_signature, HDF_SIGNATURE_LENGTH) != HDF_SIGNATURE_LENGTH) {
		close(fd);
		return 0; /* EOF or error */
	}
	close(fd);
	return (memcmp(hdf_signature, actual_signature, HDF_SIGNATURE_LENGTH) == 0);
}

