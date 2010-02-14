#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

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

int raw_image_open(volume_container *v, char *pathname) {
	int fd;
	struct stat file_stat;

	if ( (fd = open(pathname, O_RDWR | O_SYNC)) == -1 ) {
		perror("open() (RDWR) error");
		return -1;
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

static char *hdf_signature = "RS-IDE\x1a";
#define HDF_SIGNATURE_LENGTH 7

int hdf_image_open(volume_container *v, char *pathname) {
	int fd;
	struct stat file_stat;
	unsigned char hdf_header[11];

	if ( (fd = open(pathname, O_RDWR | O_SYNC)) == -1 ) {
		perror("open() (RDWR) error");
		return -1;
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

