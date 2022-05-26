#pragma once

typedef struct BrightnessOptions {
	const char *format;
	const char *name;
} BrightnessOptions;

void brightness(char *output, void *arg);
