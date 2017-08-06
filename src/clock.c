#include <config.h>
#include <time.h>
#include "integer.h"

/* Used by the FAT driver; return the current date/time in 32-bit FAT timestamp format */
DWORD get_fattime()
{
	time_t epoch_time;
	struct tm t;
	
	epoch_time = time(NULL);

#ifdef COMPAT_WIN32
	gmtime_s(&t, &epoch_time);
#else
	gmtime_r(&epoch_time, &t);
#endif

	return (
		(((t.tm_year + 1900 - 1980) & 0x7f) << 25)
		| ((t.tm_mon + 1) << 21)
		| (t.tm_mday << 16)
		| (t.tm_hour << 11)
		| (t.tm_min << 5)
		| (t.tm_sec >> 1)
	);
}
