/* Low-level disk operations required by the FAT driver */

#include <fcntl.h>

#include "diskio.h"

static volume_container *volume_containers[8];

/* Associate a volume_container structure with a drive number so that it can
be addressed by the FAT driver */
int disk_map(BYTE drive_number, volume_container *vol)
{
	volume_containers[drive_number] = vol;
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */

DSTATUS disk_initialize (BYTE drv)
{
	DSTATUS stat;
	int result;

	return 0; /* status = success */
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	return 0; /* status = success */
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	volume_container *vol;
	size_t size_requested;
	size_t result;
	
	vol = volume_containers[drv];
	size_requested = count * vol->bytes_per_sector;
	
	result = vol->read(vol, sector * vol->bytes_per_sector, (void *)buff,
		size_requested);
	
	if (result == size_requested) {
		return RES_OK;
	} else {
		return RES_PARERR;
	}
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	volume_container *vol;
	size_t size_requested;
	size_t result;
	
	vol = volume_containers[drv];
	size_requested = count * vol->bytes_per_sector;
	
	result = vol->write(vol, sector * vol->bytes_per_sector, (void *)buff,
		size_requested);
	
	if (result == size_requested) {
		return RES_OK;
	} else {
		return RES_PARERR;
	}
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (ctrl) {
		case CTRL_SYNC:
			/* syncing happens automatically on write */
			return RES_OK;
		case GET_SECTOR_SIZE:
			*((WORD *)buff) = volume_containers[drv]->bytes_per_sector;
			return RES_OK;
		case GET_SECTOR_COUNT:
			*((DWORD *)buff) = volume_containers[drv]->sector_count;
			return RES_OK;
		case GET_BLOCK_SIZE:
			*((DWORD *)buff) = 1;
			return RES_OK;
		default:
			return RES_PARERR;
	}
}

