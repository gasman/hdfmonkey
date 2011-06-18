#ifndef __IMAGE_FILE_H
#define __IMAGE_FILE_H

#include "volume_container.h"

int raw_image_open(volume_container *v, char *pathname);
int raw_image_create(volume_container *v, char *pathname, unsigned long sector_count);

int hdf_image_open(volume_container *v, char *pathname);
int hdf_image_create(volume_container *v, char *pathname, unsigned long sector_count);
int image_file_is_hdf(char *pathname);

#endif /* #ifdef __IMAGE_FILE_H */

