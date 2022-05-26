#pragma once

typedef enum WpaState { WPA_DISCONNECTED, WPA_CONNECTED } WpaState;

typedef struct WpaOptions {
	const char *format[2];
	const char *ifpath;
} WpaOptions;

void wpa(char *output, void *arg);
