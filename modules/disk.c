#include <sys/statvfs.h>

#define MODULE_NAME "disk"

#include "../common.h"
#include "disk.h"

static void print(char *output, DiskOptions *opts, struct statvfs *vfs) {
	unsigned long freespace = vfs->f_bfree * vfs->f_bsize;
	unsigned long totalspace = vfs->f_blocks * vfs->f_bsize;
	unsigned long usedspace = totalspace - freespace;
	unsigned int percent = (double)usedspace / (double)totalspace * 100.f;
	fmt_opt fmtopts[] = {
		{ 'f', FmtTypeHuman,   &freespace  },
		{ 'p', FmtTypeInteger, &percent    },
		{ 't', FmtTypeHuman,   &totalspace },
		{ 'u', FmtTypeHuman,   &usedspace  },
	};
	msnfmt(output, opts->format, fmtopts, LEN(fmtopts));
}

void disk(char *output, void *arg) {
	DiskOptions *opts = arg;
	LOG("startup");
	for(;;) {
		struct statvfs vfs;
		if(statvfs(opts->mountpoint, &vfs) == -1) {
			ERR("failed to stat mountpoint '%s'", opts->mountpoint);
			break;
		}
		print(output, opts, &vfs);
		struct timespec ts = { .tv_sec = opts->interval };
		if(nanosleep(&ts, NULL) == -1)
			break;
	}
	LOG("shutdown");
}
