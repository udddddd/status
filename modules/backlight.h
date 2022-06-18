#pragma once

typedef struct BacklightOptions {
	const char *format;
	const char *name;
} BacklightOptions;

void backlight(char *output, void *arg);
