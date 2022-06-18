#include <libudev.h>
#include <string.h>
#include <poll.h>

#define MODULE_NAME "battery"

#include "../common.h"
#include "battery.h"

#define getsupply(udev, name) \
 udev_device_new_from_subsystem_sysname(udev, "power_supply", name)

static int readint(struct udev_device *dev, const char *sysattr) {
    const char *txt = udev_device_get_sysattr_value(dev, sysattr);
    return txt ? atoi(txt) : -1;
}

static BatteryStatus get_status(struct udev_device *dev) {
	const char *status = udev_device_get_sysattr_value(dev, "status");
	if(strcmp(status, "Discharging") == 0)
		return BATTERY_DISCHARGING;
	return BATTERY_CHARGING;
}

static void refresh(char *output, struct udev_device *dev, const char *fmt) {
	int capacity = readint(dev, "capacity");
	const char *name = udev_device_get_sysname(dev);
	fmt_opt fmtopts[] = {
		{ 'n', FmtTypeString,   name     },
		{ 'p', FmtTypeInteger, &capacity },
	};
	msnfmt(output, fmt, fmtopts, LEN(fmtopts));
}

static void monitor(struct udev *udev, char *output, BatteryOptions *opts) {
    struct udev_device *bat;
    if(!(bat = getsupply(udev, opts->name))) {
        ERR("failed to open battery '%s'", opts->name);
        return;
    }
	refresh(output, bat, opts->format[get_status(bat)]);
	udev_device_unref(bat);
    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
    udev_monitor_enable_receiving(mon);
    struct pollfd fd = { .fd = udev_monitor_get_fd(mon), .events = POLLIN };
	int ret;
	while((ret = poll(&fd, 1, opts->interval * 1000)) != -1) {
		int shouldrefresh = !ret;
        if(fd.revents & POLLIN) {
            bat = udev_monitor_receive_device(mon);
			shouldrefresh |= !strcmp(udev_device_get_sysname(bat), opts->name);
        } else {
			bat = getsupply(udev, opts->name);
		}
		if(shouldrefresh)
			refresh(output, bat, opts->format[get_status(bat)]);
		udev_device_unref(bat);
    }
    udev_monitor_unref(mon);
}

void battery(char *output, void *arg) {
	LOG("startup");
	BatteryOptions *opts = arg;
    struct udev *udev = udev_new();
    monitor(udev, output, opts);
    udev_unref(udev);
	LOG("shutdown");
    return;
}
