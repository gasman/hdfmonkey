#include <stdio.h>
#include <string.h>

#include "image_file.h"
#include "diskio.h"
#include "ff.h"
#include "ffconf.h"

#define READ_BUFFER_SIZE 2048

/* Print an error message for an error returned from the FAT driver */
static void fat_perror(char *custom_message, FRESULT result) {
	char *error_message;
	switch (result) {
		case FR_OK:
			error_message = "No error"; /* Not an error. Obviously. */
			break;
		case FR_DISK_ERR:
			error_message = "Low-level disk error";
			break;
		case FR_INT_ERR:
			error_message = "Internal error";
			break;
		case FR_NOT_READY:
			error_message = "Drive not ready";
			break;
		case FR_NO_FILE:
			error_message = "File not found";
			break;
		case FR_NO_PATH:
			error_message = "Path not found";
			break;
		case FR_INVALID_NAME:
			error_message = "File / directory name is invalid";
			break;
		case FR_DENIED:
			error_message = "Access denied";
			break;
		case FR_EXIST:
			error_message = "File / directory already exists";
			break;
		case FR_INVALID_OBJECT:
			error_message = "Invalid object";
			break;
		case FR_WRITE_PROTECTED:
			error_message = "Drive is write-protected";
			break;
		case FR_INVALID_DRIVE:
			error_message = "Invalid drive number";
			break;
		case FR_NOT_ENABLED:
			error_message = "Work area not initialised";
			break;
		case FR_NO_FILESYSTEM:
			error_message = "No FAT filesystem found";
			break;
		case FR_MKFS_ABORTED:
			error_message = "Disk is unsuitable for formatting";
			break;
		case FR_TIMEOUT:
			error_message = "Timeout";
			break;
		default:
			error_message = "Unknown error code";
	}
	printf("%s: %s\n", custom_message, error_message);
}

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

static int cmd_cat(int argc, char *argv[]) {
	char *image_filename;
	char *filename;
	volume_container vol;
	FATFS fatfs;
	FRESULT result;
	FIL file;
	
	char buffer[READ_BUFFER_SIZE];
	UINT bytes_read;
	
	if (argc < 3) {
		printf("No image filename supplied\n");
		return -1;
	}
	image_filename = argv[2];

	if (argc < 4) {
		printf("No source filename supplied\n");
		return -1;
	}
	filename = argv[3];
	
	if (open_image(image_filename, &vol, &fatfs) == -1) {
		return -1;
	}
	
	result = f_open(&file, filename, FA_READ | FA_OPEN_EXISTING);
	if (result != FR_OK) {
		fat_perror("Error opening file", result);
		return -1;
	}
	
	do {
		result = f_read(&file, buffer, READ_BUFFER_SIZE, &bytes_read);
		if (result != FR_OK) {
			fat_perror("Error reading file", result);
			f_close(&file);
			return -1;
		}
		fwrite(buffer, 1, bytes_read, stdout);
	} while (bytes_read == READ_BUFFER_SIZE);
}

static int cmd_ls(int argc, char *argv[]) {
	char *image_filename;
	volume_container vol_container;
	
	FATFS fatfs;
	DIR dir;
	FRESULT result;
	FILINFO file_info;
	
	XCHAR* dirname;
	
#if _USE_LFN
	XCHAR lfname[255];
#endif
	
	if (argc < 3) {
		printf("No image filename supplied\n");
		return -1;
	}
	
	image_filename = argv[2];
	
	if (open_image(image_filename, &vol_container, &fatfs) == -1) {
		return -1;
	}
	
	if (argc > 3) {
		/* explicit path specified */
		dirname = argv[3];
	} else {
		/* no path specified - use root */
		dirname = "";
	}
	
	if (result = f_opendir(&dir, dirname) != FR_OK) {
		fat_perror("Error opening dir", result);
		return -1;
	}
	
#if _USE_LFN
	file_info.lfname = lfname;
	file_info.lfsize = 255;
#endif
	while(1) {
		if (result = f_readdir(&dir, &file_info) != FR_OK) {
			fat_perror("Error reading dir", result);
			return -1;
		}
		if (file_info.fname[0] == '\0') break;
		
		/* indicate whether file is a dir or a regular file */
		if (file_info.fattrib & AM_DIR) {
			printf("[DIR]\t");
		} else {
			printf("%d\t", file_info.fsize);
		}
		
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

static int cmd_help(int argc, char *argv[]) {
	if (argc < 3) {
		printf("hdfmonkey: utility for manipulating HDF disk images\n\n");
		printf("usage: hdfmonkey <command> [args]\n\n");
		printf("Type 'hdfmonkey help <command>' for help on a specific command.\n");
		printf("Available commands:\n");
		printf("\tcat\thelp\n\tls\n");
	} else if (strcmp(argv[2], "cat") == 0) {
		printf("cat: Write a file to standard output\n");
		printf("usage: hdfmonkey cat <imagefile> <file>\n");
	} else if (strcmp(argv[2], "help") == 0) {
		printf("help: Describe the usage of this program or its commands.\n");
		printf("usage: hdfmonkey help [command]\n");
	} else if (strcmp(argv[2], "ls") == 0) {
		printf("ls: Show a directory listing\n");
		printf("usage: hdfmonkey ls <imagefile> [path]\n");
		printf("Will list the root directory if no path is specified.\n");
	} else {
		printf("Unknown command: '%s'\n", argv[2]);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		/* fall through to help prompt */
	} else if (strcmp(argv[1], "cat") == 0) {
		return cmd_cat(argc, argv);
	} else if (strcmp(argv[1], "help") == 0) {
		return cmd_help(argc, argv);
	} else if (strcmp(argv[1], "ls") == 0) {
		return cmd_ls(argc, argv);
	} else {
		printf("Unknown command: '%s'\n", argv[1]);
	}
	printf("Type 'hdfmonkey help' for usage.\n");
	return 0;
}
