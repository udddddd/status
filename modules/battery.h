#pragma once

typedef enum BatteryStatus {
	BATTERY_DISCHARGING, BATTERY_CHARGING, BATTERY_FULL, BATTERY_LAST
} BatteryStatus;

typedef struct BatteryOptions {
	const char *format[BATTERY_LAST];
	const char *name;
	const char *adapter;
	int interval;
} BatteryOptions;

void battery(char *output, void *arg);
