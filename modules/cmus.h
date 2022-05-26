#pragma once

typedef enum CmusStatus { CMUS_STOPPED, CMUS_PAUSED, CMUS_PLAYING } CmusStatus;

typedef struct CmusOptions {
	const char *format[3];
} CmusOptions;

void cmus(char *output, void *arg);
