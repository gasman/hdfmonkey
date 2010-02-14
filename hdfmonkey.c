#include <stdio.h>

#include "image_file.h"
#include "diskio.h"
#include "ff.h"
#include "ffconf.h"

int main(int argc, char *argv[]) {
	char *pathname;
	volume_container vol_container;
	int res;
	
	FATFS fatfs;
	DIR dir;
	FRESULT result;
	FILINFO file_info;
	
	XCHAR* dirname = "";
	
	if (argc != 2) {
		printf("Usage: hdfmonkey filename.hdf\n");
		return 0;
	}
	
	pathname = argv[1];
	
	if (image_file_is_hdf(pathname)) {
		/* HDF image file found */;
		res = hdf_image_open(&vol_container, pathname);
	} else {
		/* Raw image file found */
		res = raw_image_open(&vol_container, pathname);
	}
	if (res) return -1;
	
	disk_map(0, &vol_container);
	
	if (f_mount(0, &fatfs) != FR_OK) {
		printf("mount failed\n");
		return -1;
	}
	if (result = f_opendir(&dir, dirname) != FR_OK) {
		printf("opendir failed with result %d\n", result);
		return -1;
	}
	
	while(1) {
		if (result = f_readdir(&dir, &file_info) != FR_OK) {
			printf("readdir failed with result %d\n", result);
			return -1;
		}
		if (file_info.fname[0] == '\0') break;
#if _USE_LFN
		printf("%s\n", file_info.lfname);
#else
		printf("%s\n", file_info.fname);
#endif
	}
	
	vol_container.close(&vol_container);

	return 0;
}
