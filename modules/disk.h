#pragma once

typedef struct DiskOptions {
	const char *format;
	const char *mountpoint;
	unsigned int interval;
}  DiskOptions;

void disk(char *output, void *arg);
