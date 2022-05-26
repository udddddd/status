#pragma once

typedef struct XKBOptions {
	char *format;
} XKBOptions;

void xkb(char *output, void *arg);
