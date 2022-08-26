#include <libudev.h>
#include <poll.h>

#define MODULE_NAME "backlight"

#include "../common.h"
#include "backlight.h"

static int readint(struct udev_device *dev, const char *sysattr) {
    const char *txt = udev_device_get_sysattr_value(dev, sysattr);
    return txt ? atoi(txt) : -1;
}

static void print_label(char *output, struct udev_device *dev,
 const char *fmt) {
	int a, m, p;
	fmt_opt fmtopts[] = { { 'a', FmtTypeInteger, &a },
	                      { 'm', FmtTypeInteger, &m },
	                      { 'p', FmtTypeInteger, &p },
	                    };
	a = readint(dev, "brightness");
	m = readint(dev, "max_brightness");
	p = (a + 1) * 100 / m;
	msnfmt(output, fmt, fmtopts, LEN(fmtopts));
}

static void monitor(struct udev *udev, char *output, const char *target,
 const char *fmt) {
    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "backlight", NULL);
    udev_monitor_enable_receiving(mon);
    struct pollfd fd = { .fd = udev_monitor_get_fd(mon), .events = POLLIN };
	int ret;
	while((ret = poll(&fd, 1, -1)) != -1) {
		if(fd.revents & POLLHUP)
			break;
        if(fd.revents & POLLIN) {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if(!strcmp(target, udev_device_get_syspath(dev)))
				print_label(output, dev, fmt);
            udev_device_unref(dev);
        }
    }
    udev_monitor_unref(mon);
}

void backlight(char *output, void *arg) {
	LOG("startup");
	BacklightOptions *opts = arg;
    struct udev *udev = udev_new();
    struct udev_device *dev =
		udev_device_new_from_subsystem_sysname(udev, "backlight", opts->name);
    if(!dev) {
		ERR("failed to open backlight '%s'", opts->name);
        return;
    }
    print_label(output, dev, opts->format);
    monitor(udev, output, udev_device_get_syspath(dev), opts->format);
    udev_device_unref(dev);
    udev_unref(udev);
	LOG("shutdown");
    return;
}
