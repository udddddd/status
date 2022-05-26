#pragma once

typedef struct TimeDateOptions {
	char *format;
	//unsigned int interval;
} TimeDateOptions;

void date(char *output, void *arg);
