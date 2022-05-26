TimeDateOptions time1 = {
	.format = "Time: %H:%M",
};

XKBOptions xkb1 = {
	.format = "Layout: %s",
};

BrightnessOptions bri1 = {
	.format = "Brightness: %p%%",
	.name   = "amdgpu_bl0",
};

BatteryOptions bat1 = {
	.format = {
		[BATTERY_DISCHARGING] = "Discharging %p%%",
		[BATTERY_CHARGING]    = "Charging %p%%"
	},
	.name           = "BAT0",
	.interval       = 30,
};

AlsaOptions alsa1 = {
	.card = "default",
	.format = {
		[HEADPHONE_UNPLUGGED] = "Headphone: %p%%",
		[HEADPHONE_PLUGGED] = "Speaker: %p%%",
	},
	.speaker   = "Speaker",
	.headphone = "Headphone",
	.jack      = "Headphone Jack",
};

CmusOptions cmus1 = {
	.format = {
		[CMUS_STOPPED] = "Not playing",
		[CMUS_PAUSED]  = "|| %a - %t (%A)",
		[CMUS_PLAYING] = " > %a - %t (%A)",
	},
};

WpaOptions wpa1 = {
	.ifpath = "/var/run/wpa_supplicant/wlp1s0",
	.format = {
		[WPA_DISCONNECTED] = "Disconnected",
		[WPA_CONNECTED]    = "%s %i",
	}
};

const char separator[] = " ";

Module modules[] = {
	{ date,       &time1 },
	{ xkb,        &xkb1  },
	{ brightness, &bri1  },
	{ battery,    &bat1  },
	{ alsa,       &alsa1 },
	{ cmus,       &cmus1 },
	{ wpa,        &wpa1  },
};
