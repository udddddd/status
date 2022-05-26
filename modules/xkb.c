#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/poll.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#define MODULE_NAME "xkb"

#include "../common.h"
#include "xkb.h"

static int isvalidlayout(char *sym) {
	size_t i;
	/* invalid symbols from xkb rules config */
	static const char *invalid[] = { "evdev", "inet", "pc", "base" };
	for (i = 0; i < LEN(invalid); i++) {
		if (!strncmp(sym, invalid[i], strlen(invalid[i]))) {
			return 0;
		}
	}
	return 1;
}

static void supper(char *s) {
	while(*s) {
		*s = toupper(*s);
		++s;
	}
}

static char *getlayout(char *syms, int grp_num) {
	char *tok, *layout;
	int grp;

	layout = NULL;
	tok = strtok(syms, "+:");
	for (grp = 0; tok && grp <= grp_num; tok = strtok(NULL, "+:")) {
		if (!isvalidlayout(tok)) {
			continue;
		} else if (strlen(tok) == 1 && isdigit(tok[0])) {
			/* ignore :2, :3, :4 (additional layout groups) */
			continue;
		}
		layout = tok;
		grp++;
	}
	return layout;
}

void xkb(char *output, void *arg) {
	LOG("startup");
	XKBOptions *opts = arg;
	Display *dpy;
	XkbDescRec *desc;
	XkbStateRec state;
	char *symbols;

	if(!(dpy = XOpenDisplay(NULL))) {
		ERR("can't open display");
		return;
	}
	desc = XkbAllocKeyboard();
	XEvent ev;

	XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbStateNotify,
		XkbAllStateComponentsMask, XkbGroupStateMask | XkbModifierLockMask);
	struct pollfd fd = { .fd = ConnectionNumber(dpy), .events = POLLIN };
	do {
		XkbGetNames(dpy, XkbSymbolsNameMask, desc);
		XkbGetState(dpy, XkbUseCoreKbd, &state);
		symbols = XGetAtomName(dpy, desc->names->symbols);
		char *layout = getlayout(symbols, state.group);
		supper(layout);
		fmt_opt fmtopts[] = { { 's', FmtTypeString, layout } };
		msnfmt(output, opts->format, fmtopts, LEN(fmtopts));
		while(XPending(dpy))
			XNextEvent(dpy, &ev);
	} while(poll(&fd, 1, -1) != -1);
	XFree(symbols);
	XkbFreeKeyboard(desc, XkbSymbolsNameMask, 1);
	XCloseDisplay(dpy);
	LOG("shutdown");
}
