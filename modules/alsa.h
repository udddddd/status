#pragma once

typedef enum AlsaStatus {
	HEADPHONE_UNPLUGGED = 0,
	HEADPHONE_PLUGGED = 1,
	MUTED = 2,
} AlsaStatus;

typedef struct AlsaOptions {
	const char *card;
	const char *speaker;
	const char *headphone;
	const char *jack;
	const char *format[4];
} AlsaOptions;

void alsa(char *output, void *arg);
