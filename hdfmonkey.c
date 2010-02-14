#include <stdio.h>

#include "image_file.h"
#include "diskio.h"
#include "ff.h"
#include "ffconf.h"

/* Open the file at pathname as an HDF or raw disk image, populating the passed
volume container and opening it as disk 0 for the FAT driver */
static int open_image(char *pathname, volume_container *vol, FATFS *fatfs) {
	int res;
	
	if (image_file_is_hdf(pathname)) {
		/* HDF image file found */;
		res = hdf_image_open(vol, pathname);
	} else {
		/* Raw image file found */
		res = raw_image_open(vol, pathname);
	}
	if (res) return -1;
	
	disk_map(0, vol);
	
	if (f_mount(0, fatfs) != FR_OK) {
		printf("mount failed\n");
		return -1;
	}
	
	return 0;
}

int main(int argc, char *argv[]) {
	char *pathname;
	volume_container vol_container;
	
	FATFS fatfs;
	DIR dir;
	FRESULT result;
	FILINFO file_info;
	
	XCHAR* dirname = "";
	
#if _USE_LFN
	XCHAR lfname[255];
#endif
	
	if (argc != 2) {
		printf("Usage: hdfmonkey filename.hdf\n");
		return 0;
	}
	
	pathname = argv[1];
	
	if (open_image(pathname, &vol_container, &fatfs) == -1) {
		return -1;
	}
	
	if (result = f_opendir(&dir, dirname) != FR_OK) {
		printf("opendir failed with result %d\n", result);
		return -1;
	}
	
#if _USE_LFN
	file_info.lfname = lfname;
	file_info.lfsize = 255;
#endif
	while(1) {
		if (result = f_readdir(&dir, &file_info) != FR_OK) {
			printf("readdir failed with result %d\n", result);
			return -1;
		}
		if (file_info.fname[0] == '\0') break;
#if _USE_LFN
		if (file_info.lfname[0]) {
			printf("%s\n", file_info.lfname);
		} else {
			printf("%s\n", file_info.fname);
		}
#else
		printf("%s\n", file_info.fname);
#endif
	}
	
	vol_container.close(&vol_container);

	return 0;
}
